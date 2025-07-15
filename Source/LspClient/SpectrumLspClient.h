#pragma once

#include <QObject>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QFuture>
#include <QFutureWatcher>
#include <QTcpSocket>
#include <QTcpServer>
#include <memory>
#include <functional>

// Forward declarations
class LspProcess;
class LspProtocol;
class LspFeatureManager;
class DocumentManager;
class ErrorManager;
struct ErrorInfo;

/**
 * @brief Main LSP client orchestrator for SpectrumIDE
 * 
 * This singleton class manages the entire LSP client lifecycle including:
 * - ALS server process management
 * - JSON-RPC communication
 * - Feature coordination
 * - Document synchronization
 * - Error handling and recovery
 */
class SpectrumLspClient : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief LSP client connection states
     */
    enum class ConnectionState {
        Disconnected,    // Not connected to server
        Connecting,      // Attempting to connect
        Initializing,    // LSP initialize handshake in progress
        Connected,       // Fully connected and operational
        Reconnecting,    // Attempting to reconnect after failure
        ShuttingDown     // Graceful shutdown in progress
    };

    /**
     * @brief LSP server capabilities received during initialization
     */
    struct ServerCapabilities {
        bool textDocumentSync = false;
        bool completionProvider = false;
        bool hoverProvider = false;
        bool definitionProvider = false;
        bool referencesProvider = false;
        bool documentSymbolProvider = false;
        bool workspaceSymbolProvider = false;
        bool codeActionProvider = false;
        bool documentFormattingProvider = false;
        QStringList completionTriggerCharacters;
    };

    /**
     * @brief Get singleton instance
     * @return Reference to the singleton instance
     */
    static SpectrumLspClient& instance();

    /**
     * @brief Initialize the LSP client with configuration
     * @param alsServerPath Path to the ALS server executable
     * @param workspaceRoot Root directory of the workspace
     * @return true if initialization started successfully
     */
    bool initialize(const QString& alsServerPath, const QString& workspaceRoot);

    /**
     * @brief Start the LSP client and connect to server
     * @return true if startup initiated successfully
     */
    bool start();

    /**
     * @brief Stop the LSP client and disconnect from server
     */
    void stop();

    /**
     * @brief Check if client is connected and operational
     * @return true if connected and ready for requests
     */
    bool isConnected() const;

    /**
     * @brief Get current connection state
     * @return Current connection state
     */
    ConnectionState getConnectionState() const;

    /**
     * @brief Get server capabilities
     * @return Server capabilities structure
     */
    const ServerCapabilities& getServerCapabilities() const;

    /**
     * @brief Get workspace root directory
     * @return Workspace root path
     */
    const QString& getWorkspaceRoot() const;

    /**
     * @brief Enable or disable specific LSP features
     * @param feature Feature name (e.g., "completion", "hover", "diagnostics")
     * @param enabled Whether to enable the feature
     */
    void setFeatureEnabled(const QString& feature, bool enabled);

    /**
     * @brief Check if a feature is enabled
     * @param feature Feature name
     * @return true if feature is enabled
     */
    bool isFeatureEnabled(const QString& feature) const;

    /**
     * @brief Configure process management settings
     * @param maxRestartAttempts Maximum number of restart attempts
     * @param autoRestart Whether to enable automatic restart
     */
    void configureProcessManagement(int maxRestartAttempts = 3, bool autoRestart = true);

    /**
     * @brief Get process management statistics
     * @return JSON object with process stats
     */
    QJsonObject getProcessStatistics() const;

    /**
     * @brief Check if ALS server process is responsive
     * @return true if process is responsive
     */
    bool isServerResponsive() const;

    /**
     * @brief Get error manager instance
     * @return Error manager reference
     */
    ErrorManager* getErrorManager() const;

    /**
     * @brief Enable/disable graceful degradation
     * @param enabled Whether to enable graceful degradation
     */
    void setGracefulDegradationEnabled(bool enabled);

    /**
     * @brief Check if component is in degraded mode
     * @param component Component name
     * @return true if component is degraded
     */
    bool isComponentDegraded(const QString& component) const;

    /**
     * @brief Get system health status
     * @return JSON object with health information
     */
    QJsonObject getSystemHealth() const;

public slots:
    /**
     * @brief Request server restart (e.g., after configuration changes)
     */
    void restartServer();

    /**
     * @brief Handle configuration changes
     */
    void onConfigurationChanged();

signals:
    /**
     * @brief Emitted when connection state changes
     * @param state New connection state
     */
    void connectionStateChanged(ConnectionState state);

    /**
     * @brief Emitted when server capabilities are received
     * @param capabilities Server capabilities
     */
    void serverCapabilitiesReceived(const ServerCapabilities& capabilities);

    /**
     * @brief Emitted when an error occurs
     * @param error Error message
     */
    void errorOccurred(const QString& error);

    /**
     * @brief Emitted when server is ready for requests
     */
    void serverReady();

    /**
     * @brief Emitted when server becomes unavailable
     */
    void serverUnavailable();

public:
    /**
     * @brief Destructor
     */
    ~SpectrumLspClient();

protected:
    /**
     * @brief Protected constructor for singleton pattern
     */
    explicit SpectrumLspClient(QObject* parent = nullptr);

private slots:
    /**
     * @brief Handle server process state changes
     */
    void onServerProcessStateChanged();

    /**
     * @brief Handle successful socket connection
     */
    void onSocketConnected();

    /**
     * @brief Handle socket connection errors
     */
    void onSocketError(QAbstractSocket::SocketError socketError);

    /**
     * @brief Handle LSP initialize response
     */
    void onInitializeResponse(const QJsonObject& response);

    /**
     * @brief Handle connection timeout
     */
    void onConnectionTimeout();

    /**
     * @brief Handle periodic health check
     */
    void onHealthCheck();

    /**
     * @brief Handle process becoming unresponsive
     */
    void onProcessUnresponsive();

    /**
     * @brief Handle memory threshold exceeded
     * @param memoryKB Current memory usage in KB
     */
    void onMemoryThresholdExceeded(qint64 memoryKB);

    /**
     * @brief Handle maximum restart attempts reached
     */
    void onMaxRestartsReached();

    /**
     * @brief Handle critical error from error manager
     * @param errorInfo Error information
     */
    void onCriticalError(const ErrorInfo& errorInfo);

    /**
     * @brief Handle component degradation
     * @param component Component name
     * @param reason Degradation reason
     */
    void onComponentDegraded(const QString& component, const QString& reason);

private:
    /**
     * @brief Set connection state and emit signal
     * @param state New connection state
     */
    void setConnectionState(ConnectionState state);

    /**
     * @brief Parse server capabilities from initialize response
     * @param capabilities JSON capabilities object
     */
    void parseServerCapabilities(const QJsonObject& capabilities);

    /**
     * @brief Setup health monitoring
     */
    void setupHealthMonitoring();

    /**
     * @brief Cleanup resources
     */
    void cleanup();

    /**
     * @brief Find an available port for socket communication
     * @return Available port number, or -1 if none found
     */
    int findAvailablePort();

    /**
     * @brief Get the LSP socket port from settings
     * @return Configured socket port (default: 8080)
     */
    static int getLspSocketPort();

private:
    // Core components
    std::unique_ptr<LspProcess> m_process;
    std::unique_ptr<LspProtocol> m_protocol;
    std::unique_ptr<LspFeatureManager> m_featureManager;
    std::unique_ptr<DocumentManager> m_documentManager;
    std::unique_ptr<ErrorManager> m_errorManager;

    // State management
    ConnectionState m_connectionState;
    ServerCapabilities m_serverCapabilities;
    QString m_alsServerPath;
    QString m_workspaceRoot;
    QMap<QString, bool> m_enabledFeatures;
    bool m_gracefulDegradationEnabled;

    // Timers and monitoring
    QTimer* m_connectionTimer;
    QTimer* m_healthTimer;

    // Socket communication
    std::unique_ptr<QTcpSocket> m_socket;
    int m_socketPort;

    // Thread safety
    mutable QMutex m_stateMutex;

    // Singleton instance
    static std::unique_ptr<SpectrumLspClient> s_instance;
    static QMutex s_instanceMutex;

    // Disable copy and assignment
    SpectrumLspClient(const SpectrumLspClient&) = delete;
    SpectrumLspClient& operator=(const SpectrumLspClient&) = delete;
};

// Convenience macros for accessing the singleton
#define LSP_CLIENT SpectrumLspClient::instance()
#define LSP_CONNECTED SpectrumLspClient::instance().isConnected()
