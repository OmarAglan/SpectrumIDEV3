#pragma once

#include "BaaCodeAction.h"
#include "BaaDocumentSymbol.h"
#include "BaaCompletionItem.h"
#include "BaaHover.h"
#include "BaaInlayHint.h"
#include "BaaFoldingRange.h"
#include "BaaLocation.h"
#include "BaaSemanticToken.h"
#include "BaaSelectionRange.h"
#include "BaaSignatureHelp.h"
#include "BaaWorkspaceEdit.h"
#include "BaaWorkspaceSymbol.h"
#include "Diagnostic.h"
#include "LspMessageFramer.h"

#include <QHash>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QSet>
#include <QStringList>
#include <QTimer>

class BaaLanguageClient : public QObject
{
    Q_OBJECT
public:
    enum class State {
        Stopped,
        Starting,
        Initializing,
        Ready,
        Restarting,
        Stopping,
        Error
    };
    Q_ENUM(State)

    explicit BaaLanguageClient(QObject *parent = nullptr);
    ~BaaLanguageClient() override;

    int synchronizeDocument(const QString &filePath,
                            const QString &text,
                            int editorRevision,
                            const QString &workspaceRoot = QString());
    void requestDocumentSymbols(const QString &filePath);
    void requestSemanticTokens(const QString &filePath);
    void requestFoldingRanges(const QString &filePath);
    void requestInlayHints(const QString &filePath);
    void requestSelectionRanges(const QString &filePath,
                                int line,
                                int character);
    void requestWorkspaceSymbols(const QString &query = QString());
    void requestCompletion(const QString &filePath, int line, int character);
    void requestHover(const QString &filePath, int line, int character);
    void requestSignatureHelp(const QString &filePath, int line, int character);
    void requestDefinition(const QString &filePath, int line, int character);
    void requestReferences(const QString &filePath, int line, int character,
                           bool includeDeclaration = true);
    void requestCodeActions(const QString &filePath, int line, int character);
    void requestFormatting(const QString &filePath);
    void requestPrepareRename(const QString &filePath,
                              int line,
                              int character);
    void requestRename(const QString &filePath,
                       int line,
                       int character,
                       const QString &newName);
    QVector<BaaDocumentSymbol> documentSymbols(const QString &filePath) const;
    void closeDocument(const QString &filePath);
    void stop();

    void setServerProgram(const QString &program);
    void setCompilerProgram(const QString &program);
    void setTakweenProgram(const QString &program);
    void setChangeDebounceInterval(int milliseconds);
    void setRestartPolicy(int maximumAttempts,
                          int initialDelayMilliseconds,
                          int resetAfterMilliseconds);

    State state() const;
    static bool isBaaSourcePath(const QString &filePath);

signals:
    void stateChanged(BaaLanguageClient::State state);
    void diagnosticsPublished(const QString &filePath,
                              int documentVersion,
                              const QVector<Diagnostic> &diagnostics);
    void documentSymbolsPublished(const QString &filePath,
                                  int documentVersion,
                                  const QVector<BaaDocumentSymbol> &symbols);
    void semanticTokensPublished(const QString &filePath,
                                 int documentVersion,
                                 const QVector<BaaSemanticToken> &tokens);
    void foldingRangesPublished(const QString &filePath,
                                int documentVersion,
                                const QVector<BaaFoldingRange> &ranges);
    void inlayHintsPublished(const QString &filePath,
                             int documentVersion,
                             const QVector<BaaInlayHint> &hints);
    void selectionRangesPublished(const QString &filePath,
                                  int documentVersion,
                                  int line,
                                  int character,
                                  const QVector<BaaSelectionRange> &ranges);
    void workspaceSymbolsPublished(
        const QString &query,
        const QVector<BaaWorkspaceSymbol> &symbols);
    void workspaceSymbolsFailed(
        const QString &query,
        int code,
        const QString &message);
    void completionPublished(const QString &filePath,
                             int documentVersion,
                             int line,
                             int character,
                             const QVector<BaaCompletionItem> &items);
    void hoverPublished(const QString &filePath,
                        int documentVersion,
                        int line,
                        int character,
                        const BaaHover &hover);
    void signatureHelpPublished(const QString &filePath,
                                int documentVersion,
                                int line,
                                int character,
                                const BaaSignatureHelp &signatureHelp);
    void definitionPublished(const QString &filePath,
                             int documentVersion,
                             int line,
                             int character,
                             const BaaLocation &definition);
    void referencesPublished(const QString &filePath,
                             int documentVersion,
                             int line,
                             int character,
                             const QVector<BaaLocation> &references);
    void codeActionsPublished(const QString &filePath,
                              int documentVersion,
                              int line,
                              int character,
                              const QVector<BaaCodeAction> &actions);
    void formattingPublished(const QString &filePath,
                             int documentVersion,
                             const BaaWorkspaceEdit &edit);
    void renamePrepared(const QString &filePath,
                        int documentVersion,
                        int line,
                        int character,
                        const QString &placeholder,
                        const BaaLocation &range);
    void renameEditPublished(const QString &filePath,
                             int documentVersion,
                             int line,
                             int character,
                             const BaaWorkspaceEdit &edit);
    void renameFailed(const QString &filePath, const QString &message);
    void logMessage(const QString &message, int type);

private slots:
    void flushDocumentChanges();

private:
    enum class SemanticRequestKind
    {
        Hover,
        SignatureHelp,
        Definition,
        References,
        CodeAction,
        PrepareRename,
        Rename
    };

    struct Document
    {
        QString filePath;
        QString uri;
        QString workspaceRoot;
        QString text;
        int editorRevision{};
        int version{};
        bool opened{};
    };

    struct PendingSymbolRequest
    {
        QString filePath;
        int documentVersion{};
    };
    struct PendingTokenRequest
    {
        QString filePath;
        int documentVersion{};
    };
    struct PendingFoldingRequest
    {
        QString filePath;
        int documentVersion{};
    };
    struct PendingInlayHintRequest
    {
        QString filePath;
        int documentVersion{};
    };
    struct PendingSelectionRequest
    {
        QString filePath;
        int documentVersion{};
        int line{};
        int character{};
    };
    struct PendingCompletionRequest
    {
        QString filePath;
        int documentVersion{};
        int line{};
        int character{};
    };
    struct PendingWorkspaceSymbolRequest
    {
        QString query;
    };
    struct PendingFormattingRequest
    {
        QString filePath;
        int documentVersion{};
    };
    struct PendingSemanticRequest
    {
        QString filePath;
        int documentVersion{};
        int line{};
        int character{};
        SemanticRequestKind kind{SemanticRequestKind::Hover};
        bool includeDeclaration{};
        QString newName;
    };

    void ensureStarted();
    QString resolveServerProgram() const;
    QString resolveCompilerProgram() const;
    void setState(State state);

    qint64 sendRequest(const QString &method, const QJsonValue &params);
    void sendNotification(const QString &method, const QJsonValue &params);
    void sendJson(const QJsonObject &message);
    void sendInitialize();
    void sendDidOpen(Document &document);
    void retainWorkspaceRoot(const QString &workspaceRoot);
    void releaseWorkspaceRoot(const QString &workspaceRoot);
    QStringList workspaceRoots() const;
    QJsonObject workspaceFolder(const QString &workspaceRoot) const;
    void sendWorkspaceFolderChanges(const QStringList &added,
                                    const QStringList &removed);
    void watchWorkspaceRoot(const QString &workspaceRoot);
    void unwatchWorkspaceRoot(const QString &workspaceRoot);
    void handleWorkspaceWatchPath(const QString &path);
    void sendWatchedFileChange(const QString &path, int type);
    void flushWatchedFileChanges();
    void cancelPendingSymbolRequests(const QString &filePath);
    void cancelPendingTokenRequests(const QString &filePath);
    void cancelPendingFoldingRequests(const QString &filePath);
    void cancelPendingInlayHintRequests(const QString &filePath);
    void cancelPendingStructureRequests(const QString &filePath);
    void cancelPendingCompletionRequests(const QString &filePath);
    void cancelPendingFormattingRequests(const QString &filePath);
    void cancelPendingSemanticRequests(const QString &filePath);
    void requestSemantic(const QString &filePath,
                         int line,
                         int character,
                         SemanticRequestKind kind,
                         bool includeDeclaration = false,
                         const QString &newName = QString());
    void flushPendingChange(const QString &path, Document &document);

    void consumeStdout();
    void consumeStderr();
    void handleMessage(const QByteArray &jsonBody);
    void handleResponse(const QJsonObject &message);
    void handleNotification(const QJsonObject &message);
    void handlePublishDiagnostics(const QJsonObject &params);
    void clearServerSession();
    void scheduleRestart();
    QVector<BaaDocumentSymbol> parseDocumentSymbols(const QJsonArray &items) const;
    QVector<BaaSemanticToken> parseSemanticTokens(const QJsonValue &result,
                                                   bool *valid) const;
    QVector<BaaFoldingRange> parseFoldingRanges(const QJsonValue &result,
                                                bool *valid) const;
    QVector<BaaInlayHint> parseInlayHints(const QJsonValue &result,
                                          const QString &text,
                                          bool *valid) const;
    QVector<BaaSelectionRange> parseSelectionRanges(const QJsonValue &result,
                                                    bool *valid) const;
    QVector<BaaWorkspaceSymbol> parseWorkspaceSymbols(
        const QJsonValue &result) const;
    QVector<BaaCompletionItem> parseCompletionItems(const QJsonValue &result) const;
    BaaHover parseHover(const QJsonValue &result) const;
    BaaSignatureHelp parseSignatureHelp(const QJsonValue &result) const;
    BaaLocation parseLocation(const QJsonValue &result) const;
    QVector<BaaLocation> parseLocations(const QJsonValue &result) const;
    QVector<BaaCodeAction> parseCodeActions(const QJsonValue &result) const;
    QVector<BaaTextEdit> parseTextEdits(const QJsonValue &result,
                                        bool *valid) const;
    BaaWorkspaceEdit parseWorkspaceEdit(const QJsonValue &result,
                                        bool *valid) const;
    void handleProcessFinished(int exitCode, QProcess::ExitStatus status);

    QString m_serverProgram;
    QString m_compilerProgram;
    QString m_takweenProgram;
    State m_state{State::Stopped};
    QPointer<QProcess> m_process;
    LspMessageFramer m_framer;
    QHash<QString, Document> m_documents;
    QHash<QString, int> m_workspaceRootReferences;
    QSet<QString> m_serverWorkspaceRoots;
    QHash<QString, bool> m_watchedFileExistence;
    QHash<QString, int> m_pendingWatchedFileChanges;
    QFileSystemWatcher m_workspaceWatcher;
    QTimer m_workspaceWatchTimer;
    QHash<QString, QVector<BaaDocumentSymbol>> m_documentSymbols;
    QHash<QString, int> m_symbolRequestedVersions;
    QHash<qint64, PendingSymbolRequest> m_pendingSymbolRequests;
    QHash<QString, int> m_tokenRequestedVersions;
    QHash<qint64, PendingTokenRequest> m_pendingTokenRequests;
    QHash<QString, int> m_foldingRequestedVersions;
    QHash<qint64, PendingFoldingRequest> m_pendingFoldingRequests;
    QHash<QString, int> m_inlayHintRequestedVersions;
    QHash<qint64, PendingInlayHintRequest> m_pendingInlayHintRequests;
    QHash<qint64, PendingSelectionRequest> m_pendingSelectionRequests;
    QHash<qint64, PendingWorkspaceSymbolRequest>
        m_pendingWorkspaceSymbolRequests;
    QHash<qint64, PendingCompletionRequest> m_pendingCompletionRequests;
    QHash<qint64, PendingFormattingRequest> m_pendingFormattingRequests;
    QHash<qint64, PendingSemanticRequest> m_pendingSemanticRequests;
    QSet<QString> m_pendingChanges;
    QTimer m_changeTimer;
    QTimer m_stopTimer;
    QTimer m_restartTimer;
    QTimer m_restartResetTimer;
    qint64 m_nextRequestId{};
    qint64 m_initializeRequestId{};
    qint64 m_shutdownRequestId{};
    bool m_documentSymbolProvider{};
    bool m_semanticTokenProvider{};
    bool m_foldingRangeProvider{};
    bool m_inlayHintProvider{};
    bool m_selectionRangeProvider{};
    QStringList m_semanticTokenTypes;
    bool m_workspaceSymbolProvider{};
    bool m_workspaceFoldersSupported{};
    bool m_completionProvider{};
    bool m_hoverProvider{};
    bool m_signatureHelpProvider{};
    bool m_definitionProvider{};
    bool m_referencesProvider{};
    bool m_codeActionProvider{};
    bool m_documentFormattingProvider{};
    bool m_renameProvider{};
    bool m_prepareRenameProvider{};
    int m_maxRestartAttempts{3};
    int m_initialRestartDelayMilliseconds{250};
    int m_restartResetAfterMilliseconds{30000};
    int m_restartAttempts{};
    bool m_restartSuppressed{};
};
