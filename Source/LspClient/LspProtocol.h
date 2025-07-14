#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutex>
#include <QQueue>
#include <QTimer>

// Forward declaration
class LspProcess;

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
     * @brief Initialize the protocol with process
     * @param process LSP process instance
     */
    void initialize(LspProcess* process);

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

private slots:
    /**
     * @brief Handle data from process stdout
     */
    void onDataReceived();

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

private:
    LspProcess* m_process;
    QByteArray m_buffer;
    int m_requestIdCounter;
    QMutex m_sendMutex;
    bool m_initialized;
};
