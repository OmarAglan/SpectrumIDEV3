#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutex>
#include <QTimer>
#include <QTextDocument>
#include <QTextCursor>
#include <QDateTime>

// Forward declarations
class LspProtocol;

/**
 * @brief Document change types
 */
enum class ChangeType {
    Insert,
    Delete,
    Replace
};

/**
 * @brief Represents a text change in a document
 */
struct TextChange {
    ChangeType type;
    int startLine;
    int startCharacter;
    int endLine;
    int endCharacter;
    QString text;
    qint64 timestamp;
};

/**
 * @brief Document state information
 */
struct DocumentState {
    QString uri;
    QString languageId;
    int version;
    QString content;
    QDateTime lastModified;
    QDateTime lastSynced;
    bool isDirty;
    bool isOpen;
    QList<TextChange> pendingChanges;
    QTextDocument* textDocument;  // Reference to editor's document
};

/**
 * @brief Manages document synchronization with the LSP server
 *
 * This class handles:
 * - Document lifecycle (open, change, close)
 * - Version control and change tracking
 * - Efficient text synchronization
 * - Incremental updates and full sync
 */
class DocumentManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit DocumentManager(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DocumentManager();

    /**
     * @brief Initialize with LSP protocol
     * @param protocol LSP protocol instance
     */
    void initialize(LspProtocol* protocol);

    /**
     * @brief Open a document
     * @param uri Document URI
     * @param languageId Language identifier (e.g., "alif")
     * @param content Initial document content
     * @param textDocument Reference to editor's QTextDocument
     * @return true if opened successfully
     */
    bool openDocument(const QString& uri, const QString& languageId,
                     const QString& content, QTextDocument* textDocument = nullptr);

    /**
     * @brief Close a document
     * @param uri Document URI
     * @return true if closed successfully
     */
    bool closeDocument(const QString& uri);

    /**
     * @brief Update document content (full replacement)
     * @param uri Document URI
     * @param content New content
     * @return true if updated successfully
     */
    bool updateDocument(const QString& uri, const QString& content);

    /**
     * @brief Apply incremental change to document
     * @param uri Document URI
     * @param startLine Start line (0-based)
     * @param startChar Start character (0-based)
     * @param endLine End line (0-based)
     * @param endChar End character (0-based)
     * @param text New text content
     * @return true if change applied successfully
     */
    bool applyChange(const QString& uri, int startLine, int startChar,
                    int endLine, int endChar, const QString& text);

    /**
     * @brief Sync document with server (send pending changes)
     * @param uri Document URI
     * @return true if sync initiated successfully
     */
    bool syncDocument(const QString& uri);

    /**
     * @brief Sync all open documents
     * @return Number of documents synced
     */
    int syncAllDocuments();

    /**
     * @brief Check if document is open
     * @param uri Document URI
     * @return true if document is open
     */
    bool isDocumentOpen(const QString& uri) const;

    /**
     * @brief Check if document has unsaved changes
     * @param uri Document URI
     * @return true if document is dirty
     */
    bool isDocumentDirty(const QString& uri) const;

    /**
     * @brief Get document version
     * @param uri Document URI
     * @return Document version or -1 if not found
     */
    int getDocumentVersion(const QString& uri) const;

    /**
     * @brief Get document content
     * @param uri Document URI
     * @return Document content or empty string if not found
     */
    QString getDocumentContent(const QString& uri) const;

    /**
     * @brief Get list of open documents
     * @return List of document URIs
     */
    QStringList getOpenDocuments() const;

    /**
     * @brief Get document statistics
     * @return JSON object with statistics
     */
    QJsonObject getDocumentStatistics() const;

    /**
     * @brief Set auto-sync interval
     * @param intervalMs Interval in milliseconds (0 to disable)
     */
    void setAutoSyncInterval(int intervalMs);

    /**
     * @brief Enable/disable incremental sync
     * @param enabled Whether to use incremental sync
     */
    void setIncrementalSyncEnabled(bool enabled);

    /**
     * @brief Handle configuration changes
     */
    void onConfigurationChanged();

    /**
     * @brief Shutdown the document manager
     */
    void shutdown();

signals:
    /**
     * @brief Emitted when a document is opened
     * @param uri Document URI
     */
    void documentOpened(const QString& uri);

    /**
     * @brief Emitted when a document is closed
     * @param uri Document URI
     */
    void documentClosed(const QString& uri);

    /**
     * @brief Emitted when a document is modified
     * @param uri Document URI
     * @param version New version number
     */
    void documentModified(const QString& uri, int version);

    /**
     * @brief Emitted when a document is synced with server
     * @param uri Document URI
     */
    void documentSynced(const QString& uri);

    /**
     * @brief Emitted when sync fails
     * @param uri Document URI
     * @param error Error message
     */
    void syncFailed(const QString& uri, const QString& error);

private slots:
    /**
     * @brief Handle auto-sync timer
     */
    void onAutoSyncTimer();

    /**
     * @brief Handle text document changes
     */
    void onTextDocumentChanged();

private:
    /**
     * @brief Create text change object from parameters
     */
    TextChange createTextChange(ChangeType type, int startLine, int startChar,
                               int endLine, int endChar, const QString& text);

    /**
     * @brief Convert text changes to LSP format
     * @param changes List of text changes
     * @return JSON array of LSP text document content change events
     */
    QJsonArray convertChangesToLsp(const QList<TextChange>& changes);

    /**
     * @brief Apply changes to document state
     * @param state Document state to modify
     * @param changes Changes to apply
     */
    void applyChangesToState(DocumentState& state, const QList<TextChange>& changes);

    /**
     * @brief Calculate incremental changes between two texts
     * @param oldText Previous text
     * @param newText Current text
     * @return List of changes
     */
    QList<TextChange> calculateIncrementalChanges(const QString& oldText,
                                                  const QString& newText);

    /**
     * @brief Send document changes to server
     * @param uri Document URI
     * @param changes Changes to send
     */
    void sendChangesToServer(const QString& uri, const QList<TextChange>& changes);

private:
    LspProtocol* m_protocol;
    QMap<QString, DocumentState> m_documents;
    mutable QMutex m_documentsMutex;

    // Configuration
    bool m_incrementalSyncEnabled;
    int m_autoSyncIntervalMs;

    // Auto-sync timer
    QTimer* m_autoSyncTimer;

    // Statistics
    int m_totalDocumentsOpened;
    int m_totalChangesSent;
    qint64 m_totalBytesSynced;

    // Constants
    static const int DEFAULT_AUTO_SYNC_INTERVAL_MS = 500;  // 500ms
    static const int MAX_PENDING_CHANGES = 100;
};
