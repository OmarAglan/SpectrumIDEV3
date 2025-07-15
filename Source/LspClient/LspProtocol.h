#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMutex>
#include <QQueue>
#include <QTimer>
#include <QMap>
#include <QFuture>
#include <QFutureWatcher>
#include <QPromise>
#include <QTcpSocket>
#include <functional>
#include <queue>

// Forward declarations
class LspProcess;
class QTcpSocket;

/**
 * @brief Message priority levels
 */
enum class MessagePriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief LSP request types
 */
enum class LspRequestType {
    Initialize,
    Shutdown,
    TextDocumentDidOpen,
    TextDocumentDidChange,
    TextDocumentDidClose,
    TextDocumentCompletion,
    TextDocumentHover,
    TextDocumentDefinition,
    TextDocumentReferences,
    TextDocumentDocumentSymbol,
    WorkspaceSymbol,
    Custom
};

/**
 * @brief Pending request information
 */
struct PendingRequest {
    int id;
    LspRequestType type;
    QJsonObject request;
    QTimer* timeoutTimer;
    std::function<void(const QJsonObject&)> successCallback;
    std::function<void(const QString&)> errorCallback;
    MessagePriority priority;
    qint64 timestamp;
};

/**
 * @brief Queued message with priority
 */
struct QueuedMessage {
    QJsonObject message;
    MessagePriority priority;
    qint64 timestamp;

    bool operator<(const QueuedMessage& other) const {
        if (priority != other.priority) {
            return priority < other.priority;  // Higher priority first
        }
        return timestamp > other.timestamp;  // Older messages first for same priority
    }
};

/**
 * @brief Handles JSON-RPC communication with the LSP server
 * 
 * This class manages:
 * - JSON-RPC message serialization/deserialization
 * - Request/response correlation
 * - Asynchronous communication
 * - Message queuing and prioritization
 */
class LspProtocol : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit LspProtocol(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~LspProtocol();

    /**
     * @brief Initialize the protocol with process (legacy stdio mode)
     * @param process LSP process instance
     */
    void initialize(LspProcess* process);

    /**
     * @brief Initialize the protocol with socket (new socket mode)
     * @param socket TCP socket for communication
     */
    void initialize(QTcpSocket* socket);

    /**
     * @brief Send LSP initialize request
     * @param workspaceRoot Workspace root directory
     */
    void sendInitialize(const QString& workspaceRoot);

    /**
     * @brief Send LSP initialized notification
     */
    void sendInitialized();

    /**
     * @brief Send ping to server (for health check)
     */
    void sendPing();

    /**
     * @brief Send shutdown request
     */
    void sendShutdown();

    /**
     * @brief Send textDocument/didOpen notification
     * @param uri Document URI
     * @param languageId Language identifier
     * @param version Document version
     * @param text Document text content
     */
    void sendTextDocumentDidOpen(const QString& uri, const QString& languageId,
                                 int version, const QString& text);

    /**
     * @brief Send textDocument/didChange notification
     * @param uri Document URI
     * @param version Document version
     * @param changes Array of text changes
     */
    void sendTextDocumentDidChange(const QString& uri, int version,
                                  const QJsonArray& changes);

    /**
     * @brief Send textDocument/didClose notification
     * @param uri Document URI
     */
    void sendTextDocumentDidClose(const QString& uri);

    /**
     * @brief Send textDocument/completion request
     * @param uri Document URI
     * @param line Line number (0-based)
     * @param character Character position (0-based)
     * @param callback Success callback function
     * @param errorCallback Error callback function
     * @return Request ID
     */
    int sendTextDocumentCompletion(const QString& uri, int line, int character,
                                  std::function<void(const QJsonObject&)> callback = nullptr,
                                  std::function<void(const QString&)> errorCallback = nullptr);

    /**
     * @brief Send textDocument/hover request
     * @param uri Document URI
     * @param line Line number (0-based)
     * @param character Character position (0-based)
     * @param callback Success callback function
     * @param errorCallback Error callback function
     * @return Request ID
     */
    int sendTextDocumentHover(const QString& uri, int line, int character,
                             std::function<void(const QJsonObject&)> callback = nullptr,
                             std::function<void(const QString&)> errorCallback = nullptr);

    /**
     * @brief Send textDocument/definition request
     * @param uri Document URI
     * @param line Line number (0-based)
     * @param character Character position (0-based)
     * @param callback Success callback function
     * @param errorCallback Error callback function
     * @return Request ID
     */
    int sendTextDocumentDefinition(const QString& uri, int line, int character,
                                  std::function<void(const QJsonObject&)> callback = nullptr,
                                  std::function<void(const QString&)> errorCallback = nullptr);

    /**
     * @brief Send custom request with priority
     * @param method LSP method name
     * @param params Request parameters
     * @param priority Message priority
     * @param callback Success callback function
     * @param errorCallback Error callback function
     * @return Request ID
     */
    int sendRequest(const QString& method, const QJsonObject& params,
                   MessagePriority priority = MessagePriority::Normal,
                   std::function<void(const QJsonObject&)> callback = nullptr,
                   std::function<void(const QString&)> errorCallback = nullptr);

    /**
     * @brief Send notification (no response expected)
     * @param method LSP method name
     * @param params Notification parameters
     * @param priority Message priority
     */
    void sendNotification(const QString& method, const QJsonObject& params,
                         MessagePriority priority = MessagePriority::Normal);

    /**
     * @brief Cancel pending request
     * @param requestId Request ID to cancel
     * @return true if request was found and cancelled
     */
    bool cancelRequest(int requestId);

    /**
     * @brief Set default request timeout
     * @param timeoutMs Timeout in milliseconds
     */
    void setDefaultTimeout(int timeoutMs);

    /**
     * @brief Get pending request count
     * @return Number of pending requests
     */
    int getPendingRequestCount() const;

    /**
     * @brief Check if protocol is ready for requests
     * @return true if ready
     */
    bool isReady() const;

    /**
     * @brief Shutdown the protocol
     */
    void shutdown();

signals:
    /**
     * @brief Emitted when initialize response is received
     * @param response Initialize response object
     */
    void initializeResponseReceived(const QJsonObject& response);

    /**
     * @brief Emitted when a notification is received
     * @param method Notification method
     * @param params Notification parameters
     */
    void notificationReceived(const QString& method, const QJsonObject& params);

    /**
     * @brief Emitted when an error occurs
     * @param error Error message
     */
    void errorOccurred(const QString& error);

    /**
     * @brief Emitted when a request times out
     * @param requestId Request ID that timed out
     * @param method Request method
     */
    void requestTimedOut(int requestId, const QString& method);

    /**
     * @brief Emitted when completion response is received
     * @param response Completion response
     */
    void completionReceived(const QJsonObject& response);

    /**
     * @brief Emitted when hover response is received
     * @param response Hover response
     */
    void hoverReceived(const QJsonObject& response);

    /**
     * @brief Emitted when definition response is received
     * @param response Definition response
     */
    void definitionReceived(const QJsonObject& response);

    /**
     * @brief Emitted when diagnostics are published
     * @param uri Document URI
     * @param diagnostics Array of diagnostics
     */
    void diagnosticsReceived(const QString& uri, const QJsonArray& diagnostics);

private slots:
    /**
     * @brief Handle data from process stdout
     */
    void onDataReceived();

    /**
     * @brief Handle request timeout
     */
    void onRequestTimeout();

    /**
     * @brief Process message queue
     */
    void processMessageQueue();

private:
    /**
     * @brief Send JSON-RPC message
     * @param message JSON message object
     */
    void sendMessage(const QJsonObject& message);

    /**
     * @brief Process received data
     * @param data Raw data from server
     */
    void processReceivedData(const QByteArray& data);

    /**
     * @brief Parse LSP message from buffer
     * @return true if complete message was parsed
     */
    bool parseMessage();

    /**
     * @brief Generate next request ID
     * @return Unique request ID
     */
    int nextRequestId();

    /**
     * @brief Create LSP position object
     * @param line Line number (0-based)
     * @param character Character position (0-based)
     * @return JSON position object
     */
    QJsonObject createPosition(int line, int character);

    /**
     * @brief Create LSP text document identifier
     * @param uri Document URI
     * @return JSON text document identifier
     */
    QJsonObject createTextDocumentIdentifier(const QString& uri);

    /**
     * @brief Handle response message
     * @param response Response JSON object
     */
    void handleResponse(const QJsonObject& response);

    /**
     * @brief Handle notification message
     * @param notification Notification JSON object
     */
    void handleNotification(const QJsonObject& notification);

    /**
     * @brief Queue message for sending
     * @param message Message to queue
     * @param priority Message priority
     */
    void queueMessage(const QJsonObject& message, MessagePriority priority);

    /**
     * @brief Send queued message immediately
     * @param message Message to send
     */
    void sendMessageImmediate(const QJsonObject& message);

private:
    LspProcess* m_process;
    QTcpSocket* m_socket;
    QByteArray m_buffer;
    int m_requestIdCounter;
    mutable QMutex m_sendMutex;
    mutable QMutex m_requestMutex;
    bool m_initialized;
    bool m_ready;
    int m_defaultTimeoutMs;

    // Request management
    QMap<int, PendingRequest*> m_pendingRequests;

    // Message queue with priority
    std::priority_queue<QueuedMessage> m_messageQueue;
    QTimer* m_queueTimer;

    // Constants
    static const int DEFAULT_TIMEOUT_MS = 30000;  // 30 seconds
    static const int QUEUE_PROCESS_INTERVAL_MS = 10;  // 10ms
};
