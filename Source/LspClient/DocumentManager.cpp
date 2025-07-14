#include "DocumentManager.h"
#include "LspProtocol.h"
#include <QDebug>
#include <QMutexLocker>
#include <QTextBlock>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QFileInfo>
#include <QUrl>

DocumentManager::DocumentManager(QObject* parent)
    : QObject(parent)
    , m_protocol(nullptr)
    , m_incrementalSyncEnabled(true)
    , m_autoSyncIntervalMs(DEFAULT_AUTO_SYNC_INTERVAL_MS)
    , m_autoSyncTimer(new QTimer(this))
    , m_totalDocumentsOpened(0)
    , m_totalChangesSent(0)
    , m_totalBytesSynced(0)
{
    qDebug() << "DocumentManager: Initializing enhanced document manager";

    // Setup auto-sync timer
    m_autoSyncTimer->setSingleShot(false);
    m_autoSyncTimer->setInterval(m_autoSyncIntervalMs);
    connect(m_autoSyncTimer, &QTimer::timeout, this, &DocumentManager::onAutoSyncTimer);
}

DocumentManager::~DocumentManager()
{
    qDebug() << "DocumentManager: Destructor called";
    shutdown();
}

void DocumentManager::initialize(LspProtocol* protocol)
{
    qDebug() << "DocumentManager: Initializing with LSP protocol";

    m_protocol = protocol;

    // Start auto-sync timer if enabled
    if (m_autoSyncIntervalMs > 0) {
        m_autoSyncTimer->start();
    }
}

bool DocumentManager::openDocument(const QString& uri, const QString& languageId,
                                  const QString& content, QTextDocument* textDocument)
{
    qDebug() << "DocumentManager: Opening document" << uri;

    QMutexLocker locker(&m_documentsMutex);

    // Check if document is already open
    if (m_documents.contains(uri)) {
        qWarning() << "DocumentManager: Document already open:" << uri;
        return false;
    }

    // Create document state
    DocumentState state;
    state.uri = uri;
    state.languageId = languageId;
    state.version = 1;
    state.content = content;
    state.lastModified = QDateTime::currentDateTime();
    state.lastSynced = QDateTime::currentDateTime();
    state.isDirty = false;
    state.isOpen = true;
    state.textDocument = textDocument;

    // Connect to text document changes if provided
    if (textDocument) {
        connect(textDocument, &QTextDocument::contentsChanged,
                this, &DocumentManager::onTextDocumentChanged);
    }

    // Store document state
    m_documents[uri] = state;
    m_totalDocumentsOpened++;

    // Send didOpen notification to server
    if (m_protocol && m_protocol->isReady()) {
        m_protocol->sendTextDocumentDidOpen(uri, languageId, 1, content);
    }

    emit documentOpened(uri);
    qDebug() << "DocumentManager: Document opened successfully:" << uri;
    return true;
}

bool DocumentManager::closeDocument(const QString& uri)
{
    qDebug() << "DocumentManager: Closing document" << uri;

    QMutexLocker locker(&m_documentsMutex);

    auto it = m_documents.find(uri);
    if (it == m_documents.end()) {
        qWarning() << "DocumentManager: Document not found:" << uri;
        return false;
    }

    DocumentState& state = it.value();

    // Sync any pending changes before closing
    if (!state.pendingChanges.isEmpty()) {
        sendChangesToServer(uri, state.pendingChanges);
    }

    // Disconnect from text document
    if (state.textDocument) {
        state.textDocument->disconnect(this);
    }

    // Send didClose notification to server
    if (m_protocol && m_protocol->isReady()) {
        m_protocol->sendTextDocumentDidClose(uri);
    }

    // Remove from documents map
    m_documents.erase(it);

    emit documentClosed(uri);
    qDebug() << "DocumentManager: Document closed successfully:" << uri;
    return true;
}

void DocumentManager::onConfigurationChanged()
{
    qDebug() << "DocumentManager: Configuration changed";
    // TODO: Implement configuration handling in future tasks
}

void DocumentManager::shutdown()
{
    qDebug() << "DocumentManager: Shutting down";

    // Stop auto-sync timer
    if (m_autoSyncTimer) {
        m_autoSyncTimer->stop();
    }

    // Close all open documents
    QMutexLocker locker(&m_documentsMutex);
    QStringList openDocs = m_documents.keys();
    locker.unlock();

    for (const QString& uri : openDocs) {
        closeDocument(uri);
    }

    // Clear state
    locker.relock();
    m_documents.clear();
    m_protocol = nullptr;
}

bool DocumentManager::updateDocument(const QString& uri, const QString& content)
{
    qDebug() << "DocumentManager: Updating document" << uri;

    QMutexLocker locker(&m_documentsMutex);

    auto it = m_documents.find(uri);
    if (it == m_documents.end()) {
        qWarning() << "DocumentManager: Document not found:" << uri;
        return false;
    }

    DocumentState& state = it.value();

    if (m_incrementalSyncEnabled && !state.content.isEmpty()) {
        // Calculate incremental changes
        QList<TextChange> changes = calculateIncrementalChanges(state.content, content);

        if (!changes.isEmpty()) {
            state.pendingChanges.append(changes);
            state.version++;
            state.content = content;
            state.lastModified = QDateTime::currentDateTime();
            state.isDirty = true;

            // Send changes immediately or queue for later
            sendChangesToServer(uri, changes);

            emit documentModified(uri, state.version);
        }
    } else {
        // Full document replacement
        state.content = content;
        state.version++;
        state.lastModified = QDateTime::currentDateTime();
        state.isDirty = true;

        // Send full content change
        if (m_protocol && m_protocol->isReady()) {
            QJsonArray changes;
            QJsonObject change;
            change["text"] = content;
            changes.append(change);

            m_protocol->sendTextDocumentDidChange(uri, state.version, changes);
            state.lastSynced = QDateTime::currentDateTime();
            state.isDirty = false;
            state.pendingChanges.clear();
        }

        emit documentModified(uri, state.version);
    }

    return true;
}

bool DocumentManager::applyChange(const QString& uri, int startLine, int startChar,
                                 int endLine, int endChar, const QString& text)
{
    qDebug() << "DocumentManager: Applying change to" << uri
             << "at" << startLine << ":" << startChar;

    QMutexLocker locker(&m_documentsMutex);

    auto it = m_documents.find(uri);
    if (it == m_documents.end()) {
        qWarning() << "DocumentManager: Document not found:" << uri;
        return false;
    }

    DocumentState& state = it.value();

    // Create text change
    TextChange change = createTextChange(ChangeType::Replace, startLine, startChar,
                                        endLine, endChar, text);

    // Add to pending changes
    state.pendingChanges.append(change);
    state.version++;
    state.lastModified = QDateTime::currentDateTime();
    state.isDirty = true;

    // Apply change to local content
    applyChangesToState(state, {change});

    // Send change to server if not batching
    if (state.pendingChanges.size() >= MAX_PENDING_CHANGES) {
        sendChangesToServer(uri, state.pendingChanges);
    }

    emit documentModified(uri, state.version);
    return true;
}

void DocumentManager::onAutoSyncTimer()
{
    qDebug() << "DocumentManager: Auto-sync timer triggered";

    // Sync all documents with pending changes
    QMutexLocker locker(&m_documentsMutex);

    for (auto it = m_documents.begin(); it != m_documents.end(); ++it) {
        DocumentState& state = it.value();
        if (!state.pendingChanges.isEmpty()) {
            locker.unlock();
            sendChangesToServer(state.uri, state.pendingChanges);
            locker.relock();
        }
    }
}

void DocumentManager::onTextDocumentChanged()
{
    QTextDocument* document = qobject_cast<QTextDocument*>(sender());
    if (!document) {
        return;
    }

    // Find the document URI for this QTextDocument
    QMutexLocker locker(&m_documentsMutex);

    for (auto it = m_documents.begin(); it != m_documents.end(); ++it) {
        DocumentState& state = it.value();
        if (state.textDocument == document) {
            // Document content changed - update our state
            QString newContent = document->toPlainText();

            if (newContent != state.content) {
                qDebug() << "DocumentManager: Text document changed for" << state.uri;

                // Calculate incremental changes if enabled
                if (m_incrementalSyncEnabled) {
                    QList<TextChange> changes = calculateIncrementalChanges(state.content, newContent);
                    if (!changes.isEmpty()) {
                        state.pendingChanges.append(changes);
                    }
                } else {
                    // Mark for full sync
                    state.isDirty = true;
                }

                state.content = newContent;
                state.version++;
                state.lastModified = QDateTime::currentDateTime();

                emit documentModified(state.uri, state.version);
            }
            break;
        }
    }
}

TextChange DocumentManager::createTextChange(ChangeType type, int startLine, int startChar,
                                           int endLine, int endChar, const QString& text)
{
    TextChange change;
    change.type = type;
    change.startLine = startLine;
    change.startCharacter = startChar;
    change.endLine = endLine;
    change.endCharacter = endChar;
    change.text = text;
    change.timestamp = QDateTime::currentMSecsSinceEpoch();
    return change;
}

QList<TextChange> DocumentManager::calculateIncrementalChanges(const QString& oldText,
                                                              const QString& newText)
{
    QList<TextChange> changes;

    // Simple implementation - for a production system, you'd want a more sophisticated diff algorithm
    if (oldText != newText) {
        // For now, create a single change that replaces the entire content
        // In a real implementation, you'd use algorithms like Myers' diff or similar
        TextChange change;
        change.type = ChangeType::Replace;
        change.startLine = 0;
        change.startCharacter = 0;
        change.endLine = oldText.split('\n').size() - 1;
        change.endCharacter = 0;
        change.text = newText;
        change.timestamp = QDateTime::currentMSecsSinceEpoch();

        changes.append(change);
    }

    return changes;
}

void DocumentManager::applyChangesToState(DocumentState& state, const QList<TextChange>& changes)
{
    // Apply changes to the document state
    for (const TextChange& change : changes) {
        // For this simple implementation, we just update the content
        // In a real implementation, you'd apply the specific changes
        if (change.type == ChangeType::Replace) {
            state.content = change.text;
        }
        // Add more change types as needed
    }

    state.lastModified = QDateTime::currentDateTime();
}

void DocumentManager::sendChangesToServer(const QString& uri, const QList<TextChange>& changes)
{
    if (!m_protocol || !m_protocol->isReady() || changes.isEmpty()) {
        return;
    }

    qDebug() << "DocumentManager: Sending" << changes.size() << "changes for" << uri;

    QMutexLocker locker(&m_documentsMutex);

    auto it = m_documents.find(uri);
    if (it == m_documents.end()) {
        return;
    }

    DocumentState& state = it.value();

    // Convert changes to LSP format
    QJsonArray lspChanges = convertChangesToLsp(changes);

    // Send didChange notification
    m_protocol->sendTextDocumentDidChange(uri, state.version, lspChanges);

    // Update state
    state.lastSynced = QDateTime::currentDateTime();
    state.isDirty = false;
    state.pendingChanges.clear();

    // Update statistics
    m_totalChangesSent += changes.size();
    m_totalBytesSynced += state.content.toUtf8().size();

    emit documentSynced(uri);
}

QJsonArray DocumentManager::convertChangesToLsp(const QList<TextChange>& changes)
{
    QJsonArray lspChanges;

    for (const TextChange& change : changes) {
        QJsonObject lspChange;

        if (change.type == ChangeType::Replace) {
            // Full document change
            lspChange["text"] = change.text;
        } else {
            // Incremental change
            QJsonObject range;

            QJsonObject start;
            start["line"] = change.startLine;
            start["character"] = change.startCharacter;

            QJsonObject end;
            end["line"] = change.endLine;
            end["character"] = change.endCharacter;

            range["start"] = start;
            range["end"] = end;

            lspChange["range"] = range;
            lspChange["text"] = change.text;
        }

        lspChanges.append(lspChange);
    }

    return lspChanges;
}
