#include "BaaLanguageClient.h"

#include "Constants.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>
#include <limits>

namespace {
QString normalizedFilePath(const QString &filePath)
{
    return QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
}

QString severityName(int severity)
{
    return severity == 2 ? QStringLiteral("warning") : QStringLiteral("error");
}
}

BaaLanguageClient::BaaLanguageClient(QObject *parent)
    : QObject(parent)
{
    m_changeTimer.setSingleShot(true);
    m_changeTimer.setInterval(75);
    connect(&m_changeTimer, &QTimer::timeout,
            this, &BaaLanguageClient::flushDocumentChanges);

    m_stopTimer.setSingleShot(true);
    m_stopTimer.setInterval(750);
    connect(&m_stopTimer, &QTimer::timeout, this, [this]() {
        if (m_process and m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            if (not m_process->waitForFinished(250)) m_process->kill();
        }
    });
}

BaaLanguageClient::~BaaLanguageClient()
{
    stop();
    if (m_process and m_process->state() != QProcess::NotRunning) {
        if (not m_process->waitForFinished(300)) m_process->kill();
    }
}

int BaaLanguageClient::synchronizeDocument(const QString &filePath,
                                           const QString &text,
                                           int editorRevision,
                                           const QString &workspaceRoot)
{
    if (not isBaaSourcePath(filePath)) return 0;
    const QString path = normalizedFilePath(filePath);
    ensureStarted(workspaceRoot.isEmpty() ? QFileInfo(path).absolutePath() : workspaceRoot);

    auto it = m_documents.find(path);
    if (it == m_documents.end()) {
        Document document;
        document.filePath = path;
        document.uri = QUrl::fromLocalFile(path).toString();
        document.text = text;
        document.editorRevision = editorRevision;
        document.version = 1;
        it = m_documents.insert(path, document);
        if (m_state == State::Ready) sendDidOpen(it.value());
        return it->version;
    }

    if (it->text == text and it->editorRevision == editorRevision) return it->version;
    cancelPendingSymbolRequests(path);
    cancelPendingTokenRequests(path);
    cancelPendingCompletionRequests(path);
    cancelPendingFormattingRequests(path);
    cancelPendingSemanticRequests(path);
    m_documentSymbols.remove(path);
    m_symbolRequestedVersions.remove(path);
    m_tokenRequestedVersions.remove(path);
    it->text = text;
    it->editorRevision = editorRevision;
    ++it->version;
    if (it->opened and m_state == State::Ready) {
        m_pendingChanges.insert(path);
        m_changeTimer.start();
    }
    return it->version;
}

void BaaLanguageClient::requestDocumentSymbols(const QString &filePath)
{
    if (m_state != State::Ready or not m_documentSymbolProvider) return;
    const QString path = normalizedFilePath(filePath);
    const auto document = m_documents.constFind(path);
    if (document == m_documents.cend() or not document->opened) return;
    if (m_symbolRequestedVersions.value(path) == document->version) return;

    cancelPendingSymbolRequests(path);
    const qint64 requestId = sendRequest(
        QStringLiteral("textDocument/documentSymbol"),
        QJsonObject{{QStringLiteral("textDocument"),
                     QJsonObject{{QStringLiteral("uri"), document->uri}}}});
    m_pendingSymbolRequests.insert(requestId, {path, document->version});
    m_symbolRequestedVersions.insert(path, document->version);
}

void BaaLanguageClient::requestSemanticTokens(const QString &filePath)
{
    if (m_state != State::Ready or not m_semanticTokenProvider) return;
    const QString path = normalizedFilePath(filePath);
    auto document = m_documents.find(path);
    if (document == m_documents.end() or not document->opened) return;
    if (m_tokenRequestedVersions.value(path) == document->version) return;

    flushPendingChange(path, document.value());
    cancelPendingTokenRequests(path);
    const qint64 requestId = sendRequest(
        QStringLiteral("textDocument/semanticTokens/full"),
        QJsonObject{{QStringLiteral("textDocument"),
                     QJsonObject{{QStringLiteral("uri"), document->uri}}}});
    m_pendingTokenRequests.insert(requestId, {path, document->version});
    m_tokenRequestedVersions.insert(path, document->version);
}

void BaaLanguageClient::requestWorkspaceSymbols(const QString &query)
{
    if (m_state != State::Ready or not m_workspaceSymbolProvider) return;
    flushDocumentChanges();
    for (auto request = m_pendingWorkspaceSymbolRequests.cbegin();
         request != m_pendingWorkspaceSymbolRequests.cend(); ++request) {
        sendNotification(
            QStringLiteral("$/cancelRequest"),
            QJsonObject{{QStringLiteral("id"), request.key()}});
    }
    m_pendingWorkspaceSymbolRequests.clear();
    const qint64 requestId = sendRequest(
        QStringLiteral("workspace/symbol"),
        QJsonObject{{QStringLiteral("query"), query}});
    m_pendingWorkspaceSymbolRequests.insert(requestId, {query});
}

void BaaLanguageClient::requestCompletion(const QString &filePath,
                                          int line,
                                          int character)
{
    if (m_state != State::Ready or not m_completionProvider or
        line < 0 or character < 0) return;
    const QString path = normalizedFilePath(filePath);
    auto document = m_documents.find(path);
    if (document == m_documents.end() or not document->opened) return;

    flushPendingChange(path, document.value());

    cancelPendingCompletionRequests(path);
    const qint64 requestId = sendRequest(
        QStringLiteral("textDocument/completion"),
        QJsonObject{
            {QStringLiteral("textDocument"),
             QJsonObject{{QStringLiteral("uri"), document->uri}}},
            {QStringLiteral("position"), QJsonObject{
                {QStringLiteral("line"), line},
                {QStringLiteral("character"), character}
            }},
            {QStringLiteral("context"),
             QJsonObject{{QStringLiteral("triggerKind"), 1}}}
        });
    m_pendingCompletionRequests.insert(
        requestId, {path, document->version, line, character});
}

void BaaLanguageClient::requestHover(const QString &filePath,
                                     int line,
                                     int character)
{
    requestSemantic(filePath, line, character, SemanticRequestKind::Hover);
}

void BaaLanguageClient::requestSignatureHelp(const QString &filePath,
                                             int line,
                                             int character)
{
    requestSemantic(
        filePath, line, character, SemanticRequestKind::SignatureHelp);
}

void BaaLanguageClient::requestDefinition(const QString &filePath,
                                          int line,
                                          int character)
{
    requestSemantic(filePath, line, character, SemanticRequestKind::Definition);
}

void BaaLanguageClient::requestReferences(const QString &filePath,
                                          int line,
                                          int character,
                                          bool includeDeclaration)
{
    requestSemantic(filePath, line, character, SemanticRequestKind::References,
                    includeDeclaration);
}

void BaaLanguageClient::requestCodeActions(const QString &filePath,
                                           int line,
                                           int character)
{
    requestSemantic(filePath, line, character,
                    SemanticRequestKind::CodeAction);
}

void BaaLanguageClient::requestFormatting(const QString &filePath)
{
    if (m_state != State::Ready or not m_documentFormattingProvider) return;
    const QString path = normalizedFilePath(filePath);
    auto document = m_documents.find(path);
    if (document == m_documents.end() or not document->opened) return;

    flushPendingChange(path, document.value());
    cancelPendingFormattingRequests(path);
    const qint64 requestId = sendRequest(
        QStringLiteral("textDocument/formatting"),
        QJsonObject{
            {QStringLiteral("textDocument"),
             QJsonObject{{QStringLiteral("uri"), document->uri}}},
            {QStringLiteral("options"), QJsonObject{
                {QStringLiteral("tabSize"), 4},
                {QStringLiteral("insertSpaces"), true}
            }}
        });
    m_pendingFormattingRequests.insert(
        requestId, {path, document->version});
}

void BaaLanguageClient::requestPrepareRename(const QString &filePath,
                                             int line,
                                             int character)
{
    requestSemantic(
        filePath, line, character, SemanticRequestKind::PrepareRename);
}

void BaaLanguageClient::requestRename(const QString &filePath,
                                      int line,
                                      int character,
                                      const QString &newName)
{
    requestSemantic(
        filePath, line, character, SemanticRequestKind::Rename,
        false, newName.trimmed());
}

void BaaLanguageClient::requestSemantic(const QString &filePath,
                                        int line,
                                        int character,
                                        SemanticRequestKind kind,
                                        bool includeDeclaration,
                                        const QString &newName)
{
    bool supported = false;
    QString method;
    switch (kind) {
        case SemanticRequestKind::Hover:
            supported = m_hoverProvider;
            method = QStringLiteral("textDocument/hover");
            break;
        case SemanticRequestKind::SignatureHelp:
            supported = m_signatureHelpProvider;
            method = QStringLiteral("textDocument/signatureHelp");
            break;
        case SemanticRequestKind::Definition:
            supported = m_definitionProvider;
            method = QStringLiteral("textDocument/definition");
            break;
        case SemanticRequestKind::References:
            supported = m_referencesProvider;
            method = QStringLiteral("textDocument/references");
            break;
        case SemanticRequestKind::CodeAction:
            supported = m_codeActionProvider;
            method = QStringLiteral("textDocument/codeAction");
            break;
        case SemanticRequestKind::PrepareRename:
            supported = m_renameProvider and m_prepareRenameProvider;
            method = QStringLiteral("textDocument/prepareRename");
            break;
        case SemanticRequestKind::Rename:
            supported = m_renameProvider and not newName.isEmpty();
            method = QStringLiteral("textDocument/rename");
            break;
    }
    if (m_state != State::Ready or not supported or
        line < 0 or character < 0) return;
    const QString path = normalizedFilePath(filePath);
    auto document = m_documents.find(path);
    if (document == m_documents.end() or not document->opened) return;

    flushPendingChange(path, document.value());
    QList<qint64> obsoleteRequests;
    for (auto pending = m_pendingSemanticRequests.cbegin();
         pending != m_pendingSemanticRequests.cend(); ++pending) {
        if (pending->filePath == path &&
            pending->kind == kind)
            obsoleteRequests.push_back(pending.key());
    }
    for (qint64 requestId : obsoleteRequests) {
        sendNotification(QStringLiteral("$/cancelRequest"),
                         QJsonObject{{QStringLiteral("id"), requestId}});
        m_pendingSemanticRequests.remove(requestId);
    }
    QJsonObject params{
        {QStringLiteral("textDocument"),
         QJsonObject{{QStringLiteral("uri"), document->uri}}},
        {QStringLiteral("position"), QJsonObject{
            {QStringLiteral("line"), line},
            {QStringLiteral("character"), character}
        }}
    };
    if (kind == SemanticRequestKind::CodeAction) {
        const QJsonObject position{
            {QStringLiteral("line"), line},
            {QStringLiteral("character"), character}
        };
        params.remove(QStringLiteral("position"));
        params.insert(
            QStringLiteral("range"),
            QJsonObject{
                {QStringLiteral("start"), position},
                {QStringLiteral("end"), position}
            });
        params.insert(
            QStringLiteral("context"),
            QJsonObject{
                {QStringLiteral("diagnostics"), QJsonArray{}},
                {QStringLiteral("only"),
                 QJsonArray{QStringLiteral("quickfix")}}
            });
    } else if (kind == SemanticRequestKind::References) {
        params.insert(
            QStringLiteral("context"),
            QJsonObject{{QStringLiteral("includeDeclaration"),
                         includeDeclaration}});
    } else if (kind == SemanticRequestKind::Rename) {
        params.insert(QStringLiteral("newName"), newName);
    }
    const qint64 requestId = sendRequest(method, params);
    m_pendingSemanticRequests.insert(
        requestId,
        {path, document->version, line, character, kind,
         includeDeclaration, newName});
}

QVector<BaaDocumentSymbol> BaaLanguageClient::documentSymbols(const QString &filePath) const
{
    if (filePath.isEmpty()) return {};
    return m_documentSymbols.value(normalizedFilePath(filePath));
}

void BaaLanguageClient::closeDocument(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    const QString path = normalizedFilePath(filePath);
    auto it = m_documents.find(path);
    if (it == m_documents.end()) return;
    cancelPendingSymbolRequests(path);
    cancelPendingTokenRequests(path);
    cancelPendingCompletionRequests(path);
    cancelPendingFormattingRequests(path);
    cancelPendingSemanticRequests(path);
    m_pendingChanges.remove(path);
    if (it->opened and m_state == State::Ready) {
        sendNotification(QStringLiteral("textDocument/didClose"), QJsonObject{
            {QStringLiteral("textDocument"), QJsonObject{
                {QStringLiteral("uri"), it->uri}
            }}
        });
    }
    m_documents.erase(it);
    m_documentSymbols.remove(path);
    m_symbolRequestedVersions.remove(path);
    m_tokenRequestedVersions.remove(path);
}

void BaaLanguageClient::stop()
{
    m_changeTimer.stop();
    if (not m_process or m_process->state() == QProcess::NotRunning) {
        setState(State::Stopped);
        return;
    }
    if (m_state == State::Stopping) return;
    setState(State::Stopping);
    if (m_initializeRequestId != 0 and m_shutdownRequestId == 0) {
        m_shutdownRequestId = sendRequest(QStringLiteral("shutdown"), QJsonValue::Null);
        m_stopTimer.start();
    } else {
        m_process->terminate();
        m_stopTimer.start();
    }
}

void BaaLanguageClient::setServerProgram(const QString &program)
{
    m_serverProgram = program.trimmed();
}

void BaaLanguageClient::setCompilerProgram(const QString &program)
{
    m_compilerProgram = program.trimmed();
}

void BaaLanguageClient::setTakweenProgram(const QString &program)
{
    m_takweenProgram = program.trimmed();
}

void BaaLanguageClient::setChangeDebounceInterval(int milliseconds)
{
    m_changeTimer.setInterval(qMax(0, milliseconds));
}

BaaLanguageClient::State BaaLanguageClient::state() const
{
    return m_state;
}

bool BaaLanguageClient::isBaaSourcePath(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    return suffix == QStringLiteral("baa") or suffix == QStringLiteral("baahd");
}

void BaaLanguageClient::flushDocumentChanges()
{
    if (m_state != State::Ready) return;
    const QSet<QString> pending = m_pendingChanges;
    m_pendingChanges.clear();
    for (const QString &path : pending) {
        auto it = m_documents.find(path);
        if (it == m_documents.end() or not it->opened) continue;
        sendNotification(QStringLiteral("textDocument/didChange"), QJsonObject{
            {QStringLiteral("textDocument"), QJsonObject{
                {QStringLiteral("uri"), it->uri},
                {QStringLiteral("version"), it->version}
            }},
            {QStringLiteral("contentChanges"), QJsonArray{
                QJsonObject{{QStringLiteral("text"), it->text}}
            }}
        });
    }
}

void BaaLanguageClient::ensureStarted(const QString &workspaceRoot)
{
    if (m_process and m_process->state() != QProcess::NotRunning) return;
    const QString program = resolveServerProgram();
    if (program.isEmpty() or not QFileInfo(program).isExecutable()) {
        setState(State::Error);
        emit logMessage(QStringLiteral("لم يُعثر على خادم لغة باء Baa-LSP."), 1);
        return;
    }

    m_workspaceRoot = workspaceRoot.isEmpty()
        ? QString()
        : QDir::cleanPath(QFileInfo(workspaceRoot).absoluteFilePath());
    m_framer.clear();
    m_initializeRequestId = 0;
    m_shutdownRequestId = 0;

    QProcess *process = new QProcess(this);
    m_process = process;
    process->setProgram(program);
    QStringList arguments;
    const QString compiler = resolveCompilerProgram();
    if (not compiler.isEmpty()) {
        arguments.append({QStringLiteral("--baa-path"), compiler});
    }
    if (not m_takweenProgram.isEmpty()) {
        arguments.append({QStringLiteral("--takween-path"), m_takweenProgram});
    }
    process->setArguments(arguments);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    setState(State::Starting);

    connect(process, &QProcess::started, this, [this, process]() {
        if (m_process != process) return;
        setState(State::Initializing);
        sendInitialize();
    });
    connect(process, &QProcess::readyReadStandardOutput,
            this, &BaaLanguageClient::consumeStdout);
    connect(process, &QProcess::readyReadStandardError,
            this, &BaaLanguageClient::consumeStderr);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BaaLanguageClient::handleProcessFinished);
    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
                if (m_process != process or error != QProcess::FailedToStart) return;
                setState(State::Error);
                emit logMessage(QStringLiteral("تعذر تشغيل خادم لغة باء: %1")
                                    .arg(process->errorString()), 1);
            });
    process->start();
}

QString BaaLanguageClient::resolveServerProgram() const
{
    QString configured = m_serverProgram;
    if (configured.isEmpty()) configured = qEnvironmentVariable("BAA_LSP").trimmed();
    if (configured.isEmpty()) {
        QSettings settings(Constants::OrgName, Constants::AppName);
        configured = settings.value(Constants::SettingsKeyLanguageServerPath).toString().trimmed();
    }
    if (not configured.isEmpty()) {
        const QString found = QStandardPaths::findExecutable(configured);
        return found.isEmpty() ? configured : found;
    }

    const QString appDirectory = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
#if defined(Q_OS_WIN)
        QDir(appDirectory).filePath(QStringLiteral("baa-lsp/baa-lsp.exe")),
        QDir(appDirectory).filePath(QStringLiteral("baa-lsp.exe")),
        QStandardPaths::findExecutable(QStringLiteral("baa-lsp.exe")),
#else
        QDir(appDirectory).filePath(QStringLiteral("baa-lsp/baa-lsp")),
        QDir(appDirectory).filePath(QStringLiteral("baa-lsp")),
#endif
        QStandardPaths::findExecutable(QStringLiteral("baa-lsp"))
    };
    for (const QString &candidate : candidates) {
        if (not candidate.isEmpty() and QFileInfo(candidate).isExecutable()) return candidate;
    }
    return QString();
}

QString BaaLanguageClient::resolveCompilerProgram() const
{
    if (not m_compilerProgram.isEmpty()) return m_compilerProgram;
    QSettings settings(Constants::OrgName, Constants::AppName);
    return settings.value(Constants::SettingsKeyCompilerPath).toString().trimmed();
}

void BaaLanguageClient::setState(State state)
{
    if (m_state == state) return;
    m_state = state;
    emit stateChanged(state);
}

qint64 BaaLanguageClient::sendRequest(const QString &method, const QJsonValue &params)
{
    const qint64 id = ++m_nextRequestId;
    sendJson(QJsonObject{
        {QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
        {QStringLiteral("id"), id},
        {QStringLiteral("method"), method},
        {QStringLiteral("params"), params}
    });
    return id;
}

void BaaLanguageClient::sendNotification(const QString &method, const QJsonValue &params)
{
    sendJson(QJsonObject{
        {QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
        {QStringLiteral("method"), method},
        {QStringLiteral("params"), params}
    });
}

void BaaLanguageClient::sendJson(const QJsonObject &message)
{
    if (not m_process or m_process->state() == QProcess::NotRunning) return;
    const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
    m_process->write(LspMessageFramer::frame(body));
}

void BaaLanguageClient::sendInitialize()
{
    const QString rootUri = m_workspaceRoot.isEmpty()
        ? QString()
        : QUrl::fromLocalFile(m_workspaceRoot).toString();
    QJsonObject params{
        {QStringLiteral("processId"), static_cast<qint64>(QCoreApplication::applicationPid())},
        {QStringLiteral("clientInfo"), QJsonObject{
            {QStringLiteral("name"), QStringLiteral("Qalam")},
            {QStringLiteral("version"), QStringLiteral("3.3.0")}
        }},
        {QStringLiteral("rootUri"), rootUri.isEmpty() ? QJsonValue::Null : QJsonValue(rootUri)},
        {QStringLiteral("capabilities"), QJsonObject{
            {QStringLiteral("general"), QJsonObject{
                {QStringLiteral("positionEncodings"), QJsonArray{QStringLiteral("utf-16")}}
            }},
            {QStringLiteral("workspace"), QJsonObject{
                {QStringLiteral("workspaceEdit"), QJsonObject{
                    {QStringLiteral("documentChanges"), true}
                }},
                {QStringLiteral("symbol"), QJsonObject{
                    {QStringLiteral("dynamicRegistration"), false}
                }}
            }},
            {QStringLiteral("textDocument"), QJsonObject{
                {QStringLiteral("documentSymbol"), QJsonObject{
                    {QStringLiteral("hierarchicalDocumentSymbolSupport"), true}
                }},
                {QStringLiteral("semanticTokens"), QJsonObject{
                    {QStringLiteral("requests"), QJsonObject{
                        {QStringLiteral("full"), true}
                    }},
                    {QStringLiteral("tokenTypes"), QJsonArray{
                        QStringLiteral("type"),
                        QStringLiteral("macro"),
                        QStringLiteral("keyword"),
                        QStringLiteral("modifier"),
                        QStringLiteral("comment"),
                        QStringLiteral("string"),
                        QStringLiteral("number"),
                        QStringLiteral("operator")
                    }},
                    {QStringLiteral("tokenModifiers"), QJsonArray{}},
                    {QStringLiteral("formats"), QJsonArray{
                        QStringLiteral("relative")
                    }}
                }},
                {QStringLiteral("completion"), QJsonObject{
                    {QStringLiteral("completionItem"), QJsonObject{
                        {QStringLiteral("snippetSupport"), true}
                    }}
                }},
                {QStringLiteral("hover"), QJsonObject{
                    {QStringLiteral("contentFormat"), QJsonArray{
                        QStringLiteral("markdown"), QStringLiteral("plaintext")
                    }}
                }},
                {QStringLiteral("signatureHelp"), QJsonObject{
                    {QStringLiteral("signatureInformation"), QJsonObject{
                        {QStringLiteral("documentationFormat"), QJsonArray{
                            QStringLiteral("markdown"), QStringLiteral("plaintext")
                        }},
                        {QStringLiteral("parameterInformation"), QJsonObject{
                            {QStringLiteral("labelOffsetSupport"), false}
                        }}
                    }}
                }},
                {QStringLiteral("definition"), QJsonObject{
                    {QStringLiteral("linkSupport"), false}
                }},
                {QStringLiteral("references"), QJsonObject{
                    {QStringLiteral("dynamicRegistration"), false}
                }},
                {QStringLiteral("codeAction"), QJsonObject{
                    {QStringLiteral("dynamicRegistration"), false},
                    {QStringLiteral("codeActionLiteralSupport"), QJsonObject{
                        {QStringLiteral("codeActionKind"), QJsonObject{
                            {QStringLiteral("valueSet"),
                             QJsonArray{QStringLiteral("quickfix")}}
                        }}
                    }}
                }},
                {QStringLiteral("formatting"), QJsonObject{
                    {QStringLiteral("dynamicRegistration"), false}
                }},
                {QStringLiteral("rename"), QJsonObject{
                    {QStringLiteral("prepareSupport"), true}
                }}
            }}
        }}
    };
    if (not rootUri.isEmpty()) {
        params.insert(QStringLiteral("workspaceFolders"), QJsonArray{
            QJsonObject{{QStringLiteral("uri"), rootUri},
                        {QStringLiteral("name"), QFileInfo(m_workspaceRoot).fileName()}}
        });
    }
    m_initializeRequestId = sendRequest(QStringLiteral("initialize"), params);
}

void BaaLanguageClient::sendDidOpen(Document &document)
{
    sendNotification(QStringLiteral("textDocument/didOpen"), QJsonObject{
        {QStringLiteral("textDocument"), QJsonObject{
            {QStringLiteral("uri"), document.uri},
            {QStringLiteral("languageId"), QStringLiteral("baa")},
            {QStringLiteral("version"), document.version},
            {QStringLiteral("text"), document.text}
        }}
    });
    document.opened = true;
    m_pendingChanges.remove(document.filePath);
}

void BaaLanguageClient::cancelPendingSymbolRequests(const QString &filePath)
{
    QList<qint64> requestIds;
    for (auto it = m_pendingSymbolRequests.cbegin();
         it != m_pendingSymbolRequests.cend(); ++it) {
        if (it->filePath == filePath) requestIds.push_back(it.key());
    }
    for (qint64 requestId : requestIds) {
        sendNotification(QStringLiteral("$/cancelRequest"),
                         QJsonObject{{QStringLiteral("id"), requestId}});
        m_pendingSymbolRequests.remove(requestId);
    }
}

void BaaLanguageClient::cancelPendingTokenRequests(const QString &filePath)
{
    QList<qint64> requestIds;
    for (auto it = m_pendingTokenRequests.cbegin();
         it != m_pendingTokenRequests.cend(); ++it) {
        if (it->filePath == filePath) requestIds.push_back(it.key());
    }
    for (qint64 requestId : requestIds) {
        sendNotification(QStringLiteral("$/cancelRequest"),
                         QJsonObject{{QStringLiteral("id"), requestId}});
        m_pendingTokenRequests.remove(requestId);
    }
}

void BaaLanguageClient::cancelPendingCompletionRequests(const QString &filePath)
{
    QList<qint64> requestIds;
    for (auto it = m_pendingCompletionRequests.cbegin();
         it != m_pendingCompletionRequests.cend(); ++it) {
        if (it->filePath == filePath) requestIds.push_back(it.key());
    }
    for (qint64 requestId : requestIds) {
        sendNotification(QStringLiteral("$/cancelRequest"),
                         QJsonObject{{QStringLiteral("id"), requestId}});
        m_pendingCompletionRequests.remove(requestId);
    }
}

void BaaLanguageClient::cancelPendingFormattingRequests(
    const QString &filePath)
{
    QList<qint64> requestIds;
    for (auto it = m_pendingFormattingRequests.cbegin();
         it != m_pendingFormattingRequests.cend(); ++it) {
        if (it->filePath == filePath) requestIds.push_back(it.key());
    }
    for (qint64 requestId : requestIds) {
        sendNotification(QStringLiteral("$/cancelRequest"),
                         QJsonObject{{QStringLiteral("id"), requestId}});
        m_pendingFormattingRequests.remove(requestId);
    }
}

void BaaLanguageClient::cancelPendingSemanticRequests(const QString &filePath)
{
    QList<qint64> requestIds;
    for (auto it = m_pendingSemanticRequests.cbegin();
         it != m_pendingSemanticRequests.cend(); ++it) {
        if (it->filePath == filePath) requestIds.push_back(it.key());
    }
    for (qint64 requestId : requestIds) {
        sendNotification(QStringLiteral("$/cancelRequest"),
                         QJsonObject{{QStringLiteral("id"), requestId}});
        m_pendingSemanticRequests.remove(requestId);
    }
}

void BaaLanguageClient::flushPendingChange(const QString &path,
                                           Document &document)
{
    if (m_pendingChanges.remove(path) == 0) return;
    sendNotification(QStringLiteral("textDocument/didChange"), QJsonObject{
        {QStringLiteral("textDocument"), QJsonObject{
            {QStringLiteral("uri"), document.uri},
            {QStringLiteral("version"), document.version}
        }},
        {QStringLiteral("contentChanges"), QJsonArray{
            QJsonObject{{QStringLiteral("text"), document.text}}
        }}
    });
    if (m_pendingChanges.isEmpty()) m_changeTimer.stop();
}

void BaaLanguageClient::consumeStdout()
{
    if (not m_process) return;
    QString error;
    const QList<QByteArray> messages = m_framer.appendData(
        m_process->readAllStandardOutput(), &error);
    if (not error.isEmpty()) emit logMessage(error, 1);
    for (const QByteArray &message : messages) handleMessage(message);
}

void BaaLanguageClient::consumeStderr()
{
    if (not m_process) return;
    const QString output = QString::fromUtf8(m_process->readAllStandardError()).trimmed();
    if (not output.isEmpty()) emit logMessage(output, 4);
}

void BaaLanguageClient::handleMessage(const QByteArray &jsonBody)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(jsonBody, &error);
    if (error.error != QJsonParseError::NoError or not document.isObject()) {
        emit logMessage(QStringLiteral("أرسل خادم اللغة رسالة JSON غير صالحة."), 1);
        return;
    }
    const QJsonObject message = document.object();
    if (message.contains(QStringLiteral("id"))) handleResponse(message);
    else if (message.contains(QStringLiteral("method"))) handleNotification(message);
}

void BaaLanguageClient::handleResponse(const QJsonObject &message)
{
    const qint64 id = message.value(QStringLiteral("id")).toInteger();
    if (id == m_initializeRequestId) {
        if (message.contains(QStringLiteral("error"))) {
            setState(State::Error);
            emit logMessage(QStringLiteral("رفض Baa-LSP طلب التهيئة."), 1);
            return;
        }
        const QJsonObject capabilities = message.value(QStringLiteral("result")).toObject()
                                             .value(QStringLiteral("capabilities")).toObject();
        if (capabilities.value(QStringLiteral("positionEncoding")).toString(
                QStringLiteral("utf-16")) != QStringLiteral("utf-16")) {
            setState(State::Error);
            emit logMessage(QStringLiteral("لا يستخدم Baa-LSP ترميز المواضع UTF-16 المتوقع."), 1);
            return;
        }
        const QJsonValue documentSymbolProvider =
            capabilities.value(QStringLiteral("documentSymbolProvider"));
        m_documentSymbolProvider = documentSymbolProvider.isObject() or
            documentSymbolProvider.toBool(false);
        m_semanticTokenProvider = false;
        m_semanticTokenTypes.clear();
        const QJsonObject semanticTokenProvider =
            capabilities.value(QStringLiteral("semanticTokensProvider"))
                .toObject();
        if (not semanticTokenProvider.isEmpty()) {
            const QJsonValue full =
                semanticTokenProvider.value(QStringLiteral("full"));
            const QJsonArray rawTypes =
                semanticTokenProvider.value(QStringLiteral("legend"))
                    .toObject()
                    .value(QStringLiteral("tokenTypes"))
                    .toArray();
            for (const QJsonValue &type : rawTypes) {
                if (not type.isString() or type.toString().isEmpty()) {
                    m_semanticTokenTypes.clear();
                    break;
                }
                m_semanticTokenTypes.push_back(type.toString());
            }
            m_semanticTokenProvider =
                (full.toBool(false) or full.isObject()) and
                not m_semanticTokenTypes.isEmpty();
        }
        const QJsonValue workspaceSymbolProvider =
            capabilities.value(QStringLiteral("workspaceSymbolProvider"));
        m_workspaceSymbolProvider = workspaceSymbolProvider.isObject() or
            workspaceSymbolProvider.toBool(false);
        const QJsonValue completionProvider =
            capabilities.value(QStringLiteral("completionProvider"));
        m_completionProvider = completionProvider.isObject() or
            completionProvider.toBool(false);
        const QJsonValue hoverProvider =
            capabilities.value(QStringLiteral("hoverProvider"));
        m_hoverProvider = hoverProvider.isObject() or hoverProvider.toBool(false);
        const QJsonValue signatureHelpProvider =
            capabilities.value(QStringLiteral("signatureHelpProvider"));
        m_signatureHelpProvider = signatureHelpProvider.isObject() or
            signatureHelpProvider.toBool(false);
        const QJsonValue definitionProvider =
            capabilities.value(QStringLiteral("definitionProvider"));
        m_definitionProvider = definitionProvider.isObject() or
            definitionProvider.toBool(false);
        const QJsonValue referencesProvider =
            capabilities.value(QStringLiteral("referencesProvider"));
        m_referencesProvider = referencesProvider.isObject() or
            referencesProvider.toBool(false);
        const QJsonValue codeActionProvider =
            capabilities.value(QStringLiteral("codeActionProvider"));
        m_codeActionProvider = codeActionProvider.isObject() or
            codeActionProvider.toBool(false);
        const QJsonValue documentFormattingProvider =
            capabilities.value(QStringLiteral("documentFormattingProvider"));
        m_documentFormattingProvider =
            documentFormattingProvider.isObject() or
            documentFormattingProvider.toBool(false);
        const QJsonValue renameProvider =
            capabilities.value(QStringLiteral("renameProvider"));
        m_renameProvider = renameProvider.isObject() or
            renameProvider.toBool(false);
        m_prepareRenameProvider = renameProvider.isObject() and
            renameProvider.toObject()
                .value(QStringLiteral("prepareProvider")).toBool(false);
        sendNotification(QStringLiteral("initialized"), QJsonObject{});
        setState(State::Ready);
        for (auto it = m_documents.begin(); it != m_documents.end(); ++it) {
            sendDidOpen(it.value());
        }
        return;
    }
    auto symbolRequest = m_pendingSymbolRequests.find(id);
    if (symbolRequest != m_pendingSymbolRequests.end()) {
        const PendingSymbolRequest pending = symbolRequest.value();
        m_pendingSymbolRequests.erase(symbolRequest);

        auto document = m_documents.constFind(pending.filePath);
        if (document == m_documents.cend() or
            document->version != pending.documentVersion) return;
        if (message.contains(QStringLiteral("error"))) {
            m_symbolRequestedVersions.remove(pending.filePath);
            const QJsonObject error = message.value(QStringLiteral("error")).toObject();
            const int code = error.value(QStringLiteral("code")).toInt();
            if (code != -32800 and code != -32801) {
                emit logMessage(error.value(QStringLiteral("message")).toString(), 2);
            }
            return;
        }
        const QJsonValue result = message.value(QStringLiteral("result"));
        if (not result.isArray()) {
            m_symbolRequestedVersions.remove(pending.filePath);
            emit logMessage(QStringLiteral("أعاد Baa-LSP نتيجة رموز غير صالحة."), 2);
            return;
        }
        const QVector<BaaDocumentSymbol> symbols = parseDocumentSymbols(result.toArray());
        m_documentSymbols.insert(pending.filePath, symbols);
        emit documentSymbolsPublished(pending.filePath,
                                      pending.documentVersion,
                                      symbols);
        return;
    }
    auto tokenRequest = m_pendingTokenRequests.find(id);
    if (tokenRequest != m_pendingTokenRequests.end()) {
        const PendingTokenRequest pending = tokenRequest.value();
        m_pendingTokenRequests.erase(tokenRequest);

        const auto document = m_documents.constFind(pending.filePath);
        if (document == m_documents.cend() or
            document->version != pending.documentVersion) return;
        if (message.contains(QStringLiteral("error"))) {
            m_tokenRequestedVersions.remove(pending.filePath);
            const QJsonObject error =
                message.value(QStringLiteral("error")).toObject();
            const int code = error.value(QStringLiteral("code")).toInt();
            if (code != -32800 and code != -32801) {
                emit logMessage(
                    error.value(QStringLiteral("message")).toString(), 2);
            }
            return;
        }
        bool valid = false;
        const QVector<BaaSemanticToken> tokens =
            parseSemanticTokens(message.value(QStringLiteral("result")),
                                &valid);
        if (not valid) {
            m_tokenRequestedVersions.remove(pending.filePath);
            emit logMessage(
                QStringLiteral(
                    "أعاد Baa-LSP رموز تلوين دلالي غير صالحة."),
                2);
            return;
        }
        emit semanticTokensPublished(
            pending.filePath, pending.documentVersion, tokens);
        return;
    }
    auto workspaceSymbolRequest =
        m_pendingWorkspaceSymbolRequests.find(id);
    if (workspaceSymbolRequest !=
        m_pendingWorkspaceSymbolRequests.end()) {
        const PendingWorkspaceSymbolRequest pending =
            workspaceSymbolRequest.value();
        m_pendingWorkspaceSymbolRequests.erase(workspaceSymbolRequest);
        if (message.contains(QStringLiteral("error"))) {
            const QJsonObject error =
                message.value(QStringLiteral("error")).toObject();
            const int code = error.value(QStringLiteral("code")).toInt();
            const QString errorMessage =
                error.value(QStringLiteral("message")).toString();
            if (code != -32800 and code != -32801) {
                emit logMessage(errorMessage, 2);
            }
            emit workspaceSymbolsFailed(
                pending.query, code, errorMessage);
            return;
        }
        const QVector<BaaWorkspaceSymbol> symbols =
            parseWorkspaceSymbols(message.value(QStringLiteral("result")));
        emit workspaceSymbolsPublished(pending.query, symbols);
        return;
    }
    auto completionRequest = m_pendingCompletionRequests.find(id);
    if (completionRequest != m_pendingCompletionRequests.end()) {
        const PendingCompletionRequest pending = completionRequest.value();
        m_pendingCompletionRequests.erase(completionRequest);

        const auto document = m_documents.constFind(pending.filePath);
        if (document == m_documents.cend() or
            document->version != pending.documentVersion) return;
        if (message.contains(QStringLiteral("error"))) {
            const QJsonObject error = message.value(QStringLiteral("error")).toObject();
            const int code = error.value(QStringLiteral("code")).toInt();
            if (code != -32800 and code != -32801) {
                emit logMessage(
                    error.value(QStringLiteral("message")).toString(), 2);
            }
            return;
        }
        const QVector<BaaCompletionItem> items = parseCompletionItems(
            message.value(QStringLiteral("result")));
        emit completionPublished(pending.filePath,
                                 pending.documentVersion,
                                 pending.line,
                                 pending.character,
                                 items);
        return;
    }
    auto formattingRequest = m_pendingFormattingRequests.find(id);
    if (formattingRequest != m_pendingFormattingRequests.end()) {
        const PendingFormattingRequest pending = formattingRequest.value();
        m_pendingFormattingRequests.erase(formattingRequest);

        const auto document = m_documents.constFind(pending.filePath);
        if (document == m_documents.cend() or
            document->version != pending.documentVersion) return;
        if (message.contains(QStringLiteral("error"))) {
            const QJsonObject error =
                message.value(QStringLiteral("error")).toObject();
            const int code = error.value(QStringLiteral("code")).toInt();
            if (code != -32800 and code != -32801) {
                emit logMessage(
                    error.value(QStringLiteral("message")).toString(), 2);
            }
            return;
        }

        bool valid = false;
        QVector<BaaTextEdit> edits = parseTextEdits(
            message.value(QStringLiteral("result")), &valid);
        if (not valid) {
            emit logMessage(
                QStringLiteral("أعاد Baa-LSP تعديلات تنسيق غير صالحة."), 2);
            return;
        }
        BaaWorkspaceEdit workspaceEdit;
        if (not edits.isEmpty()) {
            BaaDocumentEdit documentEdit;
            documentEdit.filePath = pending.filePath;
            documentEdit.version = pending.documentVersion;
            documentEdit.edits = std::move(edits);
            workspaceEdit.documents.push_back(std::move(documentEdit));
        }
        emit formattingPublished(
            pending.filePath, pending.documentVersion, workspaceEdit);
        return;
    }
    auto semanticRequest = m_pendingSemanticRequests.find(id);
    if (semanticRequest != m_pendingSemanticRequests.end()) {
        const PendingSemanticRequest pending = semanticRequest.value();
        m_pendingSemanticRequests.erase(semanticRequest);

        const auto document = m_documents.constFind(pending.filePath);
        if (document == m_documents.cend() or
            document->version != pending.documentVersion) return;
        if (message.contains(QStringLiteral("error"))) {
            const QJsonObject error = message.value(QStringLiteral("error")).toObject();
            const int code = error.value(QStringLiteral("code")).toInt();
            if (code != -32800 and code != -32801) {
                const QString errorMessage =
                    error.value(QStringLiteral("message")).toString();
                emit logMessage(errorMessage, 2);
                if (pending.kind == SemanticRequestKind::PrepareRename or
                    pending.kind == SemanticRequestKind::Rename)
                    emit renameFailed(pending.filePath, errorMessage);
            }
            return;
        }
        const QJsonValue result = message.value(QStringLiteral("result"));
        switch (pending.kind) {
            case SemanticRequestKind::Hover:
                emit hoverPublished(
                    pending.filePath, pending.documentVersion,
                    pending.line, pending.character, parseHover(result));
                break;
            case SemanticRequestKind::SignatureHelp:
                emit signatureHelpPublished(
                    pending.filePath, pending.documentVersion,
                    pending.line, pending.character, parseSignatureHelp(result));
                break;
            case SemanticRequestKind::Definition:
                emit definitionPublished(
                    pending.filePath, pending.documentVersion,
                    pending.line, pending.character, parseLocation(result));
                break;
            case SemanticRequestKind::References:
                emit referencesPublished(
                    pending.filePath, pending.documentVersion,
                    pending.line, pending.character, parseLocations(result));
                break;
            case SemanticRequestKind::CodeAction:
                emit codeActionsPublished(
                    pending.filePath, pending.documentVersion,
                    pending.line, pending.character, parseCodeActions(result));
                break;
            case SemanticRequestKind::PrepareRename:
            {
                const QJsonObject prepared = result.toObject();
                QJsonObject location = prepared;
                location.insert(
                    QStringLiteral("uri"),
                    QUrl::fromLocalFile(pending.filePath).toString());
                const BaaLocation range = parseLocation(location);
                const QString placeholder =
                    prepared.value(QStringLiteral("placeholder")).toString();
                if (not range.isValid() or placeholder.isEmpty()) {
                    emit renameFailed(
                        pending.filePath,
                        QStringLiteral(
                            "لا يوجد رمز باء قابل لإعادة التسمية عند هذا الموضع."));
                    break;
                }
                emit renamePrepared(
                    pending.filePath, pending.documentVersion,
                    pending.line, pending.character, placeholder, range);
                break;
            }
            case SemanticRequestKind::Rename:
            {
                bool valid = false;
                const BaaWorkspaceEdit edit =
                    parseWorkspaceEdit(result, &valid);
                if (not valid) {
                    emit renameFailed(
                        pending.filePath,
                        QStringLiteral(
                            "أعاد Baa-LSP خطة إعادة تسمية غير صالحة."));
                    break;
                }
                emit renameEditPublished(
                    pending.filePath, pending.documentVersion,
                    pending.line, pending.character, edit);
                break;
            }
        }
        return;
    }
    if (id == m_shutdownRequestId) {
        sendNotification(QStringLiteral("exit"), QJsonValue::Null);
        if (m_process) m_process->closeWriteChannel();
    }
}

void BaaLanguageClient::handleNotification(const QJsonObject &message)
{
    const QString method = message.value(QStringLiteral("method")).toString();
    const QJsonObject params = message.value(QStringLiteral("params")).toObject();
    if (method == QStringLiteral("textDocument/publishDiagnostics")) {
        handlePublishDiagnostics(params);
    } else if (method == QStringLiteral("window/logMessage")) {
        emit logMessage(params.value(QStringLiteral("message")).toString(),
                        params.value(QStringLiteral("type")).toInt(4));
    }
}

void BaaLanguageClient::handlePublishDiagnostics(const QJsonObject &params)
{
    const QString uri = params.value(QStringLiteral("uri")).toString();
    const QString path = normalizedFilePath(QUrl(uri).toLocalFile());
    auto it = m_documents.find(path);
    if (it == m_documents.end()) return;
    const int version = params.value(QStringLiteral("version")).toInt(it->version);
    if (version != it->version) return;

    QVector<Diagnostic> diagnostics;
    const QJsonArray items = params.value(QStringLiteral("diagnostics")).toArray();
    diagnostics.reserve(items.size());
    for (const QJsonValue &value : items) {
        if (not value.isObject()) continue;
        const QJsonObject item = value.toObject();
        const QString message = item.value(QStringLiteral("message")).toString().trimmed();
        if (message.isEmpty()) continue;

        Diagnostic diagnostic;
        diagnostic.file = path;
        diagnostic.severity = severityName(item.value(QStringLiteral("severity")).toInt(1));
        diagnostic.code = item.value(QStringLiteral("code")).toVariant().toString();
        diagnostic.message = message;
        diagnostic.source = QStringLiteral("baa-lsp");
        const QJsonObject range = item.value(QStringLiteral("range")).toObject();
        const QJsonObject start = range.value(QStringLiteral("start")).toObject();
        const QJsonObject end = range.value(QStringLiteral("end")).toObject();
        diagnostic.line = start.value(QStringLiteral("line")).toInt() + 1;
        diagnostic.column = start.value(QStringLiteral("character")).toInt() + 1;
        diagnostic.endLine = qMax(diagnostic.line,
            end.value(QStringLiteral("line")).toInt(start.value(QStringLiteral("line")).toInt()) + 1);
        diagnostic.endColumn = qMax(1, end.value(QStringLiteral("character")).toInt(
            start.value(QStringLiteral("character")).toInt()) + 1);
        const QJsonObject data = item.value(QStringLiteral("data")).toObject();
        diagnostic.category = data.value(QStringLiteral("category")).toString();
        diagnostic.hint = data.value(QStringLiteral("hint")).toString();
        diagnostics.push_back(diagnostic);
    }
    emit diagnosticsPublished(path, version, diagnostics);
    requestDocumentSymbols(path);
    requestSemanticTokens(path);
}

QVector<BaaSemanticToken> BaaLanguageClient::parseSemanticTokens(
    const QJsonValue &result,
    bool *valid) const
{
    if (valid) *valid = false;
    QVector<BaaSemanticToken> tokens;
    if (not result.isObject()) return tokens;
    const QJsonValue dataValue =
        result.toObject().value(QStringLiteral("data"));
    if (not dataValue.isArray()) return tokens;
    const QJsonArray data = dataValue.toArray();
    if (data.size() % 5 != 0) return tokens;

    int line = 0;
    int character = 0;
    tokens.reserve(data.size() / 5);
    for (qsizetype index = 0; index < data.size(); index += 5) {
        for (qsizetype field = 0; field < 5; ++field) {
            if (not data.at(index + field).isDouble()) return {};
        }
        const qint64 deltaLine = data.at(index).toInteger(-1);
        const qint64 deltaCharacter = data.at(index + 1).toInteger(-1);
        const qint64 length = data.at(index + 2).toInteger(-1);
        const qint64 typeIndex = data.at(index + 3).toInteger(-1);
        const qint64 modifiers = data.at(index + 4).toInteger(-1);
        if (deltaLine < 0 or deltaCharacter < 0 or length <= 0 or
            typeIndex < 0 or typeIndex >= m_semanticTokenTypes.size() or
            modifiers < 0)
            return {};
        if (deltaLine > 0) {
            if (deltaLine > std::numeric_limits<int>::max() or
                deltaLine > std::numeric_limits<int>::max() - line or
                deltaCharacter > std::numeric_limits<int>::max())
                return {};
            line += static_cast<int>(deltaLine);
            character = static_cast<int>(deltaCharacter);
        } else {
            if (deltaCharacter >
                std::numeric_limits<int>::max() - character)
                return {};
            character += static_cast<int>(deltaCharacter);
        }
        if (line < 0 or character < 0 or
            length > std::numeric_limits<int>::max() or
            modifiers > std::numeric_limits<int>::max())
            return {};
        BaaSemanticToken token;
        token.line = line;
        token.character = character;
        token.length = static_cast<int>(length);
        token.type = m_semanticTokenTypes.at(
            static_cast<qsizetype>(typeIndex));
        token.modifiers = static_cast<int>(modifiers);
        if (not token.isValid()) return {};
        tokens.push_back(std::move(token));
    }
    if (valid) *valid = true;
    return tokens;
}

QVector<BaaDocumentSymbol> BaaLanguageClient::parseDocumentSymbols(
    const QJsonArray &items) const
{
    QVector<BaaDocumentSymbol> symbols;
    symbols.reserve(items.size());
    for (const QJsonValue &value : items) {
        if (not value.isObject()) continue;
        const QJsonObject item = value.toObject();
        BaaDocumentSymbol symbol;
        symbol.name = item.value(QStringLiteral("name")).toString().trimmed();
        symbol.detail = item.value(QStringLiteral("detail")).toString().trimmed();
        symbol.kind = item.value(QStringLiteral("kind")).toInt();
        QJsonObject range = item.value(QStringLiteral("selectionRange")).toObject();
        if (range.isEmpty()) range = item.value(QStringLiteral("range")).toObject();
        const QJsonObject start = range.value(QStringLiteral("start")).toObject();
        const QJsonObject end = range.value(QStringLiteral("end")).toObject();
        symbol.line = start.value(QStringLiteral("line")).toInt() + 1;
        symbol.column = start.value(QStringLiteral("character")).toInt() + 1;
        symbol.endLine = qMax(symbol.line,
            end.value(QStringLiteral("line")).toInt(symbol.line - 1) + 1);
        symbol.endColumn = qMax(1,
            end.value(QStringLiteral("character")).toInt(symbol.column - 1) + 1);
        symbol.children = parseDocumentSymbols(
            item.value(QStringLiteral("children")).toArray());
        if (symbol.isValid()) symbols.push_back(std::move(symbol));
    }
    return symbols;
}

QVector<BaaWorkspaceSymbol> BaaLanguageClient::parseWorkspaceSymbols(
    const QJsonValue &result) const
{
    QVector<BaaWorkspaceSymbol> symbols;
    if (not result.isArray()) return symbols;
    const QJsonArray items = result.toArray();
    symbols.reserve(items.size());
    for (const QJsonValue &value : items) {
        if (not value.isObject()) continue;
        const QJsonObject item = value.toObject();
        const QJsonObject location =
            item.value(QStringLiteral("location")).toObject();
        const QJsonObject range =
            location.value(QStringLiteral("range")).toObject();
        const QJsonObject start =
            range.value(QStringLiteral("start")).toObject();
        const QJsonObject end =
            range.value(QStringLiteral("end")).toObject();
        BaaWorkspaceSymbol symbol;
        symbol.name =
            item.value(QStringLiteral("name")).toString().trimmed();
        symbol.kind = item.value(QStringLiteral("kind")).toInt();
        symbol.containerName =
            item.value(QStringLiteral("containerName")).toString().trimmed();
        symbol.filePath = QUrl(
            location.value(QStringLiteral("uri")).toString()).toLocalFile();
        symbol.line = start.value(QStringLiteral("line")).toInt(-1);
        symbol.character =
            start.value(QStringLiteral("character")).toInt(-1);
        symbol.endLine =
            end.value(QStringLiteral("line")).toInt(symbol.line);
        symbol.endCharacter =
            end.value(QStringLiteral("character")).toInt(symbol.character);
        symbol.detail = item.value(QStringLiteral("data")).toObject()
                            .value(QStringLiteral("detail")).toString().trimmed();
        if (symbol.isValid()) symbols.push_back(std::move(symbol));
    }
    return symbols;
}

QVector<BaaCompletionItem> BaaLanguageClient::parseCompletionItems(
    const QJsonValue &result) const
{
    const QJsonArray values = result.isArray()
        ? result.toArray()
        : result.toObject().value(QStringLiteral("items")).toArray();
    QVector<BaaCompletionItem> items;
    items.reserve(values.size());
    for (const QJsonValue &value : values) {
        if (not value.isObject()) continue;
        const QJsonObject source = value.toObject();
        BaaCompletionItem item;
        item.label = source.value(QStringLiteral("label")).toString().trimmed();
        item.detail = source.value(QStringLiteral("detail")).toString().trimmed();
        item.filterText = source.value(QStringLiteral("filterText")).toString(item.label);
        item.sortText = source.value(QStringLiteral("sortText")).toString(item.filterText);
        item.kind = source.value(QStringLiteral("kind")).toInt(1);
        item.insertTextFormat = source.value(QStringLiteral("insertTextFormat")).toInt(1);

        const QJsonObject textEdit = source.value(QStringLiteral("textEdit")).toObject();
        item.newText = textEdit.value(QStringLiteral("newText")).toString(
            source.value(QStringLiteral("insertText")).toString(item.label));
        const QJsonObject range = textEdit.value(QStringLiteral("range")).toObject();
        const QJsonObject start = range.value(QStringLiteral("start")).toObject();
        const QJsonObject end = range.value(QStringLiteral("end")).toObject();
        item.startLine = start.value(QStringLiteral("line")).toInt(-1);
        item.startCharacter = start.value(QStringLiteral("character")).toInt(-1);
        item.endLine = end.value(QStringLiteral("line")).toInt(-1);
        item.endCharacter = end.value(QStringLiteral("character")).toInt(-1);
        if (item.isValid()) items.push_back(std::move(item));
    }
    std::ranges::stable_sort(items, [](const BaaCompletionItem &left,
                                       const BaaCompletionItem &right) {
        if (left.sortText != right.sortText) return left.sortText < right.sortText;
        return left.label < right.label;
    });
    return items;
}

BaaHover BaaLanguageClient::parseHover(const QJsonValue &result) const
{
    BaaHover hover;
    if (not result.isObject()) return hover;
    const QJsonObject source = result.toObject();
    const QJsonValue contents = source.value(QStringLiteral("contents"));
    if (contents.isString()) {
        hover.contentKind = QStringLiteral("plaintext");
        hover.contents = contents.toString();
    } else if (contents.isObject()) {
        const QJsonObject markup = contents.toObject();
        hover.contentKind = markup.value(QStringLiteral("kind")).toString(
            QStringLiteral("plaintext"));
        hover.contents = markup.value(QStringLiteral("value")).toString();
    }

    const QJsonObject range = source.value(QStringLiteral("range")).toObject();
    const QJsonObject start = range.value(QStringLiteral("start")).toObject();
    const QJsonObject end = range.value(QStringLiteral("end")).toObject();
    hover.startLine = start.value(QStringLiteral("line")).toInt(-1);
    hover.startCharacter = start.value(QStringLiteral("character")).toInt(-1);
    hover.endLine = end.value(QStringLiteral("line")).toInt(-1);
    hover.endCharacter = end.value(QStringLiteral("character")).toInt(-1);
    return hover;
}

BaaSignatureHelp BaaLanguageClient::parseSignatureHelp(
    const QJsonValue &result) const
{
    BaaSignatureHelp signatureHelp;
    if (not result.isObject()) return signatureHelp;
    const QJsonObject source = result.toObject();
    const QJsonArray signatures = source.value(QStringLiteral("signatures")).toArray();
    if (signatures.isEmpty()) return signatureHelp;
    const int activeSignature = qBound(
        0, source.value(QStringLiteral("activeSignature")).toInt(),
        signatures.size() - 1);
    const QJsonObject signature = signatures.at(activeSignature).toObject();
    signatureHelp.label = signature.value(QStringLiteral("label")).toString();
    const QJsonArray parameters =
        signature.value(QStringLiteral("parameters")).toArray();
    signatureHelp.parameters.reserve(parameters.size());
    for (const QJsonValue &value : parameters) {
        const QJsonValue label = value.toObject().value(QStringLiteral("label"));
        if (label.isString()) signatureHelp.parameters.push_back(label.toString());
    }
    signatureHelp.activeParameter = qMax(
        0, source.value(QStringLiteral("activeParameter")).toInt());
    if (not signatureHelp.parameters.isEmpty()) {
        signatureHelp.activeParameter = qMin(
            signatureHelp.activeParameter,
            signatureHelp.parameters.size() - 1);
    }
    return signatureHelp;
}

BaaLocation BaaLanguageClient::parseLocation(const QJsonValue &result) const
{
    if (result.isArray()) {
        const QJsonArray locations = result.toArray();
        return locations.isEmpty() ? BaaLocation{}
                                   : parseLocation(locations.first());
    }
    BaaLocation location;
    if (not result.isObject()) return location;
    const QJsonObject source = result.toObject();
    const QUrl uri(source.value(QStringLiteral("uri")).toString());
    const QString localPath = uri.toLocalFile();
    if (localPath.isEmpty()) return location;

    const QJsonObject range = source.value(QStringLiteral("range")).toObject();
    const QJsonObject start = range.value(QStringLiteral("start")).toObject();
    const QJsonObject end = range.value(QStringLiteral("end")).toObject();
    location.filePath = normalizedFilePath(localPath);
    location.line = start.value(QStringLiteral("line")).toInt(-1);
    location.character = start.value(QStringLiteral("character")).toInt(-1);
    location.endLine = end.value(QStringLiteral("line")).toInt(-1);
    location.endCharacter = end.value(QStringLiteral("character")).toInt(-1);
    if (not location.isValid()) return BaaLocation{};
    return location;
}

QVector<BaaLocation> BaaLanguageClient::parseLocations(
    const QJsonValue &result) const
{
    QVector<BaaLocation> locations;
    if (not result.isArray()) return locations;
    const QJsonArray source = result.toArray();
    locations.reserve(source.size());
    for (const QJsonValue &value : source) {
        BaaLocation location = parseLocation(value);
        if (location.isValid()) locations.push_back(std::move(location));
    }
    return locations;
}

QVector<BaaCodeAction> BaaLanguageClient::parseCodeActions(
    const QJsonValue &result) const
{
    QVector<BaaCodeAction> actions;
    if (not result.isArray()) return actions;
    const QJsonArray source = result.toArray();
    actions.reserve(source.size());
    for (const QJsonValue &value : source) {
        if (not value.isObject()) continue;
        const QJsonObject object = value.toObject();
        BaaCodeAction action;
        action.id = object.value(QStringLiteral("data")).toObject()
                        .value(QStringLiteral("fixId")).toString();
        action.title =
            object.value(QStringLiteral("title")).toString().trimmed();
        action.kind =
            object.value(QStringLiteral("kind")).toString();
        action.preferred =
            object.value(QStringLiteral("isPreferred")).toBool(false);
        bool editValid = false;
        action.edit = parseWorkspaceEdit(
            object.value(QStringLiteral("edit")), &editValid);
        if (editValid and action.isValid())
            actions.push_back(std::move(action));
    }
    return actions;
}

QVector<BaaTextEdit> BaaLanguageClient::parseTextEdits(
    const QJsonValue &result,
    bool *valid) const
{
    if (valid) *valid = false;
    QVector<BaaTextEdit> edits;
    if (not result.isArray()) return edits;

    const QJsonArray rawEdits = result.toArray();
    edits.reserve(rawEdits.size());
    for (const QJsonValue &editValue : rawEdits) {
        if (not editValue.isObject()) return {};
        const QJsonObject editObject = editValue.toObject();
        const QJsonObject range =
            editObject.value(QStringLiteral("range")).toObject();
        const QJsonObject start =
            range.value(QStringLiteral("start")).toObject();
        const QJsonObject end =
            range.value(QStringLiteral("end")).toObject();
        BaaTextEdit edit;
        edit.line = start.value(QStringLiteral("line")).toInt(-1);
        edit.character =
            start.value(QStringLiteral("character")).toInt(-1);
        edit.endLine = end.value(QStringLiteral("line")).toInt(-1);
        edit.endCharacter =
            end.value(QStringLiteral("character")).toInt(-1);
        edit.newText =
            editObject.value(QStringLiteral("newText")).toString();
        if (not edit.isValid() or
            edit.endLine < edit.line or
            (edit.endLine == edit.line and
             edit.endCharacter < edit.character))
            return {};
        edits.push_back(std::move(edit));
    }
    if (valid) *valid = true;
    return edits;
}

BaaWorkspaceEdit BaaLanguageClient::parseWorkspaceEdit(
    const QJsonValue &result,
    bool *valid) const
{
    if (valid) *valid = false;
    BaaWorkspaceEdit workspaceEdit;
    if (not result.isObject()) return workspaceEdit;
    const QJsonArray documents = result.toObject()
        .value(QStringLiteral("documentChanges")).toArray();
    if (documents.isEmpty()) return workspaceEdit;

    workspaceEdit.documents.reserve(documents.size());
    for (const QJsonValue &documentValue : documents) {
        if (not documentValue.isObject()) return {};
        const QJsonObject documentChange = documentValue.toObject();
        const QJsonObject identifier =
            documentChange.value(QStringLiteral("textDocument")).toObject();
        const QUrl uri(identifier.value(QStringLiteral("uri")).toString());
        const QString filePath = normalizedFilePath(uri.toLocalFile());
        const QJsonArray rawEdits =
            documentChange.value(QStringLiteral("edits")).toArray();
        if (filePath.isEmpty() or rawEdits.isEmpty()) return {};

        BaaDocumentEdit document;
        document.filePath = filePath;
        const QJsonValue version =
            identifier.value(QStringLiteral("version"));
        document.version = version.isDouble()
            ? static_cast<int>(version.toInteger(-1)) : -1;
        document.edits.reserve(rawEdits.size());
        for (const QJsonValue &editValue : rawEdits) {
            if (not editValue.isObject()) return {};
            const QJsonObject editObject = editValue.toObject();
            const QJsonObject range =
                editObject.value(QStringLiteral("range")).toObject();
            const QJsonObject start =
                range.value(QStringLiteral("start")).toObject();
            const QJsonObject end =
                range.value(QStringLiteral("end")).toObject();
            BaaTextEdit edit;
            edit.line = start.value(QStringLiteral("line")).toInt(-1);
            edit.character =
                start.value(QStringLiteral("character")).toInt(-1);
            edit.endLine = end.value(QStringLiteral("line")).toInt(-1);
            edit.endCharacter =
                end.value(QStringLiteral("character")).toInt(-1);
            edit.newText =
                editObject.value(QStringLiteral("newText")).toString();
            if (not edit.isValid() or
                edit.endLine < edit.line or
                (edit.endLine == edit.line and
                 edit.endCharacter < edit.character))
                return {};
            document.edits.push_back(std::move(edit));
        }
        workspaceEdit.documents.push_back(std::move(document));
    }
    if (valid) *valid = workspaceEdit.isValid();
    return workspaceEdit;
}

void BaaLanguageClient::handleProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_stopTimer.stop();
    const bool expected = m_state == State::Stopping;
    if (m_process) m_process->deleteLater();
    m_process = nullptr;
    m_initializeRequestId = 0;
    m_shutdownRequestId = 0;
    m_documentSymbolProvider = false;
    m_semanticTokenProvider = false;
    m_semanticTokenTypes.clear();
    m_workspaceSymbolProvider = false;
    m_completionProvider = false;
    m_hoverProvider = false;
    m_signatureHelpProvider = false;
    m_definitionProvider = false;
    m_referencesProvider = false;
    m_codeActionProvider = false;
    m_documentFormattingProvider = false;
    m_renameProvider = false;
    m_prepareRenameProvider = false;
    m_pendingSymbolRequests.clear();
    m_pendingTokenRequests.clear();
    m_pendingWorkspaceSymbolRequests.clear();
    m_pendingCompletionRequests.clear();
    m_pendingFormattingRequests.clear();
    m_pendingSemanticRequests.clear();
    m_symbolRequestedVersions.clear();
    m_tokenRequestedVersions.clear();
    m_documentSymbols.clear();
    for (auto it = m_documents.begin(); it != m_documents.end(); ++it) it->opened = false;
    setState(expected ? State::Stopped : State::Error);
    if (not expected) {
        emit logMessage(QStringLiteral("توقف Baa-LSP بصورة غير متوقعة (الرمز %1، الحالة %2).")
                            .arg(exitCode)
                            .arg(status == QProcess::NormalExit ? 0 : 1), 1);
    }
}
