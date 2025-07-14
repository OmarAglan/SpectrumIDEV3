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
