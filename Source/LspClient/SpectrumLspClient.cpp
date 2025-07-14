#include "SpectrumLspClient.h"
#include "LspProcess.h"
#include "LspProtocol.h"
#include "LspFeatureManager.h"
#include "DocumentManager.h"
#include "ErrorManager.h"

#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QMutexLocker>
#include <QJsonArray>
#include <QFile>

// Static member definitions
std::unique_ptr<SpectrumLspClient> SpectrumLspClient::s_instance = nullptr;
QMutex SpectrumLspClient::s_instanceMutex;

SpectrumLspClient& SpectrumLspClient::instance()
{
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        s_instance = std::unique_ptr<SpectrumLspClient>(new SpectrumLspClient());
    }
    return *s_instance;
}

SpectrumLspClient::SpectrumLspClient(QObject* parent)
    : QObject(parent)
    , m_connectionState(ConnectionState::Disconnected)
    , m_gracefulDegradationEnabled(true)
    , m_connectionTimer(new QTimer(this))
    , m_healthTimer(new QTimer(this))
{
    qDebug() << "SpectrumLspClient: Initializing enhanced LSP client with error management";

    // Initialize error manager first
    m_errorManager = std::make_unique<ErrorManager>(this);

    // Initialize default feature states
    m_enabledFeatures["completion"] = true;
    m_enabledFeatures["hover"] = true;
    m_enabledFeatures["diagnostics"] = true;
    m_enabledFeatures["definition"] = true;
    m_enabledFeatures["references"] = false;  // Disabled by default
    m_enabledFeatures["symbols"] = false;     // Disabled by default

    // Setup timers
    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->setInterval(10000); // 10 second timeout
    connect(m_connectionTimer, &QTimer::timeout, this, &SpectrumLspClient::onConnectionTimeout);

    m_healthTimer->setInterval(30000); // 30 second health check
    connect(m_healthTimer, &QTimer::timeout, this, &SpectrumLspClient::onHealthCheck);

    // Connect error manager signals
    connect(m_errorManager.get(), &ErrorManager::criticalErrorOccurred,
            this, &SpectrumLspClient::onCriticalError);
    connect(m_errorManager.get(), &ErrorManager::componentDegraded,
            this, &SpectrumLspClient::onComponentDegraded);

    qDebug() << "SpectrumLspClient: Initialization complete";
}

SpectrumLspClient::~SpectrumLspClient()
{
    qDebug() << "SpectrumLspClient: Destructor called";
    stop();
    cleanup();
}

bool SpectrumLspClient::initialize(const QString& alsServerPath, const QString& workspaceRoot)
{
    QMutexLocker locker(&m_stateMutex);
    
    qDebug() << "SpectrumLspClient: Initializing with server path:" << alsServerPath
             << "workspace:" << workspaceRoot;

    if (m_connectionState != ConnectionState::Disconnected) {
        qWarning() << "SpectrumLspClient: Already initialized or connecting";
        return false;
    }

    // Validate server path
    if (alsServerPath.isEmpty() || !QFile::exists(alsServerPath)) {
        QString error = QString("ALS server not found: %1").arg(alsServerPath);
        qCritical() << "SpectrumLspClient:" << error;

        if (m_errorManager) {
            m_errorManager->reportError(ErrorSeverity::Critical, ErrorCategory::ConfigurationError,
                                       "SpectrumLspClient", error,
                                       QString("Server path: %1").arg(alsServerPath));
        }

        emit errorOccurred(error);
        return false;
    }

    // Validate workspace root
    if (workspaceRoot.isEmpty() || !QDir(workspaceRoot).exists()) {
        qCritical() << "SpectrumLspClient: Workspace root not found:" << workspaceRoot;
        emit errorOccurred(QString("Workspace root not found: %1").arg(workspaceRoot));
        return false;
    }

    m_alsServerPath = alsServerPath;
    m_workspaceRoot = workspaceRoot;

    // Initialize core components
    try {
        m_process = std::make_unique<LspProcess>(this);
        m_protocol = std::make_unique<LspProtocol>(this);
        m_featureManager = std::make_unique<LspFeatureManager>(this);
        m_documentManager = std::make_unique<DocumentManager>(this);

        // Connect component signals
        connect(m_process.get(), &LspProcess::stateChanged,
                this, &SpectrumLspClient::onServerProcessStateChanged);
        connect(m_process.get(), &LspProcess::errorOccurred,
                this, &SpectrumLspClient::errorOccurred);
        connect(m_process.get(), &LspProcess::processUnresponsive,
                this, &SpectrumLspClient::onProcessUnresponsive);
        connect(m_process.get(), &LspProcess::memoryThresholdExceeded,
                this, &SpectrumLspClient::onMemoryThresholdExceeded);
        connect(m_process.get(), &LspProcess::maxRestartsReached,
                this, &SpectrumLspClient::onMaxRestartsReached);

        connect(m_protocol.get(), &LspProtocol::initializeResponseReceived,
                this, &SpectrumLspClient::onInitializeResponse);

        // Initialize protocol with process
        m_protocol->initialize(m_process.get());

        qDebug() << "SpectrumLspClient: Core components initialized successfully";
        return true;

    } catch (const std::exception& e) {
        qCritical() << "SpectrumLspClient: Failed to initialize components:" << e.what();
        emit errorOccurred(QString("Initialization failed: %1").arg(e.what()));
        cleanup();
        return false;
    }
}

bool SpectrumLspClient::start()
{
    QMutexLocker locker(&m_stateMutex);
    
    qDebug() << "SpectrumLspClient: Starting LSP client";

    if (m_connectionState != ConnectionState::Disconnected) {
        qWarning() << "SpectrumLspClient: Already started or starting";
        return false;
    }

    if (!m_process || m_alsServerPath.isEmpty()) {
        qCritical() << "SpectrumLspClient: Not properly initialized";
        emit errorOccurred("LSP client not properly initialized");
        return false;
    }

    setConnectionState(ConnectionState::Connecting);

    // Start the ALS server process
    if (!m_process->start(m_alsServerPath)) {
        qCritical() << "SpectrumLspClient: Failed to start ALS server";
        setConnectionState(ConnectionState::Disconnected);
        emit errorOccurred("Failed to start ALS server");
        return false;
    }

    // Start connection timeout
    m_connectionTimer->start();

    qDebug() << "SpectrumLspClient: Server startup initiated";
    return true;
}

void SpectrumLspClient::stop()
{
    QMutexLocker locker(&m_stateMutex);
    
    qDebug() << "SpectrumLspClient: Stopping LSP client";

    if (m_connectionState == ConnectionState::Disconnected) {
        return;
    }

    setConnectionState(ConnectionState::ShuttingDown);

    // Stop timers
    m_connectionTimer->stop();
    m_healthTimer->stop();

    // Shutdown components in reverse order
    if (m_documentManager) {
        m_documentManager->shutdown();
    }

    if (m_featureManager) {
        m_featureManager->shutdown();
    }

    if (m_protocol) {
        m_protocol->shutdown();
    }

    if (m_process) {
        m_process->stop();
    }

    setConnectionState(ConnectionState::Disconnected);
    emit serverUnavailable();

    qDebug() << "SpectrumLspClient: Stopped successfully";
}

bool SpectrumLspClient::isConnected() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_connectionState == ConnectionState::Connected;
}

SpectrumLspClient::ConnectionState SpectrumLspClient::getConnectionState() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_connectionState;
}

const SpectrumLspClient::ServerCapabilities& SpectrumLspClient::getServerCapabilities() const
{
    return m_serverCapabilities;
}

const QString& SpectrumLspClient::getWorkspaceRoot() const
{
    return m_workspaceRoot;
}

void SpectrumLspClient::setFeatureEnabled(const QString& feature, bool enabled)
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_enabledFeatures.value(feature) != enabled) {
        m_enabledFeatures[feature] = enabled;
        qDebug() << "SpectrumLspClient: Feature" << feature << (enabled ? "enabled" : "disabled");
        
        if (m_featureManager) {
            m_featureManager->setFeatureEnabled(feature, enabled);
        }
    }
}

bool SpectrumLspClient::isFeatureEnabled(const QString& feature) const
{
    QMutexLocker locker(&m_stateMutex);
    return m_enabledFeatures.value(feature, false);
}

void SpectrumLspClient::restartServer()
{
    qDebug() << "SpectrumLspClient: Restarting server requested";
    
    if (m_connectionState != ConnectionState::Disconnected) {
        stop();
    }
    
    // Restart after a brief delay
    QTimer::singleShot(1000, this, [this]() {
        start();
    });
}

void SpectrumLspClient::onConfigurationChanged()
{
    qDebug() << "SpectrumLspClient: Configuration changed, notifying components";

    if (m_featureManager) {
        m_featureManager->onConfigurationChanged();
    }

    if (m_documentManager) {
        m_documentManager->onConfigurationChanged();
    }
}

void SpectrumLspClient::configureProcessManagement(int maxRestartAttempts, bool autoRestart)
{
    qDebug() << "SpectrumLspClient: Configuring process management - max restarts:"
             << maxRestartAttempts << "auto-restart:" << autoRestart;

    if (m_process) {
        m_process->setMaxRestartAttempts(maxRestartAttempts);
        m_process->setAutoRestart(autoRestart);
    }
}

QJsonObject SpectrumLspClient::getProcessStatistics() const
{
    QJsonObject stats;

    if (!m_process) {
        stats["error"] = "Process not initialized";
        return stats;
    }

    stats["state"] = static_cast<int>(m_connectionState);
    stats["processState"] = static_cast<int>(m_process->getState());
    stats["isRunning"] = m_process->isRunning();
    stats["isResponsive"] = m_process->isResponsive();
    stats["restartAttempts"] = m_process->getRestartAttempts();
    stats["autoRestartEnabled"] = m_process->isAutoRestartEnabled();
    stats["uptimeSeconds"] = m_process->getUptimeSeconds();
    stats["memoryUsageKB"] = m_process->getMemoryUsageKB();
    stats["processId"] = m_process->processId();
    stats["lastError"] = m_process->getLastError();

    return stats;
}

bool SpectrumLspClient::isServerResponsive() const
{
    if (!m_process) {
        return false;
    }

    return m_process->isResponsive() && isConnected();
}

ErrorManager* SpectrumLspClient::getErrorManager() const
{
    return m_errorManager.get();
}

void SpectrumLspClient::setGracefulDegradationEnabled(bool enabled)
{
    m_gracefulDegradationEnabled = enabled;
    qDebug() << "SpectrumLspClient: Graceful degradation" << (enabled ? "enabled" : "disabled");
}

bool SpectrumLspClient::isComponentDegraded(const QString& component) const
{
    if (!m_errorManager) {
        return false;
    }
    return m_errorManager->isComponentDegraded(component);
}

QJsonObject SpectrumLspClient::getSystemHealth() const
{
    QJsonObject health;

    // Connection status
    health["connected"] = isConnected();
    health["connectionState"] = static_cast<int>(m_connectionState);
    health["serverResponsive"] = isServerResponsive();

    // Process status
    if (m_process) {
        health["processRunning"] = m_process->isRunning();
        health["processState"] = static_cast<int>(m_process->getState());
        health["processUptime"] = m_process->getUptimeSeconds();
        health["processMemoryKB"] = m_process->getMemoryUsageKB();
        health["restartAttempts"] = m_process->getRestartAttempts();
    }

    // Error statistics
    if (m_errorManager) {
        health["errorStatistics"] = m_errorManager->getErrorStatistics();
    }

    // Feature status
    QJsonObject features;
    for (auto it = m_enabledFeatures.begin(); it != m_enabledFeatures.end(); ++it) {
        features[it.key()] = it.value();
    }
    health["features"] = features;

    // Server capabilities
    QJsonObject capabilities;
    capabilities["completion"] = m_serverCapabilities.completion;
    capabilities["hover"] = m_serverCapabilities.hover;
    capabilities["definition"] = m_serverCapabilities.definition;
    capabilities["references"] = m_serverCapabilities.references;
    capabilities["documentSymbol"] = m_serverCapabilities.documentSymbol;
    capabilities["workspaceSymbol"] = m_serverCapabilities.workspaceSymbol;
    capabilities["diagnostics"] = m_serverCapabilities.diagnostics;
    health["serverCapabilities"] = capabilities;

    return health;
}

void SpectrumLspClient::setConnectionState(ConnectionState state)
{
    if (m_connectionState != state) {
        ConnectionState oldState = m_connectionState;
        m_connectionState = state;
        
        qDebug() << "SpectrumLspClient: Connection state changed from" 
                 << static_cast<int>(oldState) << "to" << static_cast<int>(state);
        
        emit connectionStateChanged(state);
    }
}

void SpectrumLspClient::onServerProcessStateChanged()
{
    if (!m_process) return;

    LspProcess::ProcessState processState = m_process->getState();
    
    qDebug() << "SpectrumLspClient: Server process state changed to" << static_cast<int>(processState);

    switch (processState) {
        case LspProcess::ProcessState::Starting:
            // Process is starting, wait for it to be ready
            break;
            
        case LspProcess::ProcessState::Running:
            // Process is running, initiate LSP handshake
            if (m_connectionState == ConnectionState::Connecting) {
                setConnectionState(ConnectionState::Initializing);
                if (m_protocol) {
                    m_protocol->sendInitialize(m_workspaceRoot);
                }
            }
            break;
            
        case LspProcess::ProcessState::Stopping:
            // Process is being stopped gracefully
            qDebug() << "SpectrumLspClient: Server process is stopping";
            break;

        case LspProcess::ProcessState::Crashed:
        case LspProcess::ProcessState::Stopped:
            // Process stopped or crashed
            if (m_connectionState != ConnectionState::ShuttingDown) {
                qWarning() << "SpectrumLspClient: Server process unexpectedly stopped";
                setConnectionState(ConnectionState::Reconnecting);
                emit errorOccurred("ALS server process stopped unexpectedly");

                // Attempt restart after delay
                QTimer::singleShot(2000, this, &SpectrumLspClient::restartServer);
            }
            break;
    }
}

void SpectrumLspClient::onInitializeResponse(const QJsonObject& response)
{
    qDebug() << "SpectrumLspClient: Received initialize response";

    m_connectionTimer->stop();

    if (response.contains("error")) {
        QString error = response["error"].toObject()["message"].toString();
        qCritical() << "SpectrumLspClient: Initialize failed:" << error;
        emit errorOccurred(QString("LSP initialization failed: %1").arg(error));
        setConnectionState(ConnectionState::Disconnected);
        return;
    }

    // Parse server capabilities
    if (response.contains("result")) {
        QJsonObject result = response["result"].toObject();
        if (result.contains("capabilities")) {
            parseServerCapabilities(result["capabilities"].toObject());
        }
    }

    // Send initialized notification
    if (m_protocol) {
        m_protocol->sendInitialized();
    }

    // Setup health monitoring
    setupHealthMonitoring();

    setConnectionState(ConnectionState::Connected);
    emit serverReady();
    emit serverCapabilitiesReceived(m_serverCapabilities);

    qDebug() << "SpectrumLspClient: Successfully connected to ALS server";
}

void SpectrumLspClient::onConnectionTimeout()
{
    qWarning() << "SpectrumLspClient: Connection timeout";

    if (m_connectionState == ConnectionState::Connecting ||
        m_connectionState == ConnectionState::Initializing) {

        // Report timeout error
        if (m_errorManager) {
            m_errorManager->reportError(ErrorSeverity::Error, ErrorCategory::TimeoutError,
                                       "SpectrumLspClient", "Connection to ALS server timed out",
                                       QString("Connection state: %1").arg(static_cast<int>(m_connectionState)));
        }

        emit errorOccurred("Connection to ALS server timed out");
        setConnectionState(ConnectionState::Disconnected);

        if (m_process) {
            m_process->stop();
        }
    }
}

void SpectrumLspClient::onHealthCheck()
{
    if (!isConnected() || !m_process) {
        return;
    }

    // Check if process is still running
    if (m_process->getState() != LspProcess::ProcessState::Running) {
        qWarning() << "SpectrumLspClient: Health check failed - process not running";
        setConnectionState(ConnectionState::Reconnecting);
        emit errorOccurred("ALS server health check failed");
        restartServer();
        return;
    }

    // Send ping to server (if protocol supports it)
    if (m_protocol) {
        m_protocol->sendPing();
    }
}

void SpectrumLspClient::parseServerCapabilities(const QJsonObject& capabilities)
{
    qDebug() << "SpectrumLspClient: Parsing server capabilities";

    // Reset capabilities
    m_serverCapabilities = ServerCapabilities();

    // Parse text document sync
    if (capabilities.contains("textDocumentSync")) {
        QJsonValue syncValue = capabilities["textDocumentSync"];
        m_serverCapabilities.textDocumentSync = syncValue.isDouble() ?
            (syncValue.toInt() > 0) : syncValue.toBool();
    }

    // Parse completion provider
    if (capabilities.contains("completionProvider")) {
        QJsonValue completionValue = capabilities["completionProvider"];
        if (completionValue.isBool()) {
            m_serverCapabilities.completionProvider = completionValue.toBool();
        } else if (completionValue.isObject()) {
            m_serverCapabilities.completionProvider = true;
            QJsonObject completionObj = completionValue.toObject();

            if (completionObj.contains("triggerCharacters")) {
                QJsonArray triggers = completionObj["triggerCharacters"].toArray();
                for (const QJsonValue& trigger : triggers) {
                    m_serverCapabilities.completionTriggerCharacters.append(trigger.toString());
                }
            }
        }
    }

    // Parse other capabilities
    m_serverCapabilities.hoverProvider = capabilities["hoverProvider"].toBool();
    m_serverCapabilities.definitionProvider = capabilities["definitionProvider"].toBool();
    m_serverCapabilities.referencesProvider = capabilities["referencesProvider"].toBool();
    m_serverCapabilities.documentSymbolProvider = capabilities["documentSymbolProvider"].toBool();
    m_serverCapabilities.workspaceSymbolProvider = capabilities["workspaceSymbolProvider"].toBool();
    m_serverCapabilities.codeActionProvider = capabilities["codeActionProvider"].toBool();
    m_serverCapabilities.documentFormattingProvider = capabilities["documentFormattingProvider"].toBool();

    qDebug() << "SpectrumLspClient: Server capabilities parsed successfully";
    qDebug() << "  - Text Document Sync:" << m_serverCapabilities.textDocumentSync;
    qDebug() << "  - Completion:" << m_serverCapabilities.completionProvider;
    qDebug() << "  - Hover:" << m_serverCapabilities.hoverProvider;
    qDebug() << "  - Definition:" << m_serverCapabilities.definitionProvider;
}

void SpectrumLspClient::setupHealthMonitoring()
{
    qDebug() << "SpectrumLspClient: Setting up health monitoring";
    m_healthTimer->start();
}

void SpectrumLspClient::onProcessUnresponsive()
{
    qWarning() << "SpectrumLspClient: ALS server process became unresponsive";

    if (m_connectionState == ConnectionState::Connected) {
        setConnectionState(ConnectionState::Reconnecting);
        emit errorOccurred("ALS server became unresponsive");
    }
}

void SpectrumLspClient::onMemoryThresholdExceeded(qint64 memoryKB)
{
    qWarning() << "SpectrumLspClient: ALS server memory usage exceeded threshold:" << memoryKB << "KB";

    // Log memory usage but don't take drastic action yet
    // In the future, we could implement memory-based restart policies
    emit errorOccurred(QString("ALS server high memory usage: %1 KB").arg(memoryKB));
}

void SpectrumLspClient::onMaxRestartsReached()
{
    qCritical() << "SpectrumLspClient: Maximum restart attempts reached for ALS server";

    // Report to error manager
    if (m_errorManager) {
        m_errorManager->reportError(ErrorSeverity::Critical, ErrorCategory::ProcessError,
                                   "LspProcess", "Maximum restart attempts reached",
                                   "ALS server failed to restart after maximum attempts");
    }

    setConnectionState(ConnectionState::Disconnected);
    emit errorOccurred("ALS server failed to restart after maximum attempts");
    emit serverUnavailable();

    // Disable auto-restart to prevent infinite restart loops
    if (m_process) {
        m_process->setAutoRestart(false);
    }
}

void SpectrumLspClient::onCriticalError(const ErrorInfo& errorInfo)
{
    qCritical() << "SpectrumLspClient: Critical error in" << errorInfo.component
                << ":" << errorInfo.message;

    // Handle critical errors based on component
    if (errorInfo.component == "LspProcess") {
        // Process-related critical error
        if (m_gracefulDegradationEnabled) {
            qDebug() << "SpectrumLspClient: Entering graceful degradation mode";
            setConnectionState(ConnectionState::Disconnected);
        }
    } else if (errorInfo.component == "LspProtocol") {
        // Protocol-related critical error
        if (m_gracefulDegradationEnabled) {
            qDebug() << "SpectrumLspClient: Protocol error - attempting reconnection";
            QTimer::singleShot(5000, this, &SpectrumLspClient::restartServer);
        }
    }

    // Emit error to UI
    emit errorOccurred(QString("Critical error in %1: %2")
                      .arg(errorInfo.component, errorInfo.message));
}

void SpectrumLspClient::onComponentDegraded(const QString& component, const QString& reason)
{
    qWarning() << "SpectrumLspClient: Component" << component << "degraded:" << reason;

    // Handle component degradation
    if (component == "LspProcess") {
        // Disable features that require the process
        if (m_gracefulDegradationEnabled) {
            setFeatureEnabled("completion", false);
            setFeatureEnabled("hover", false);
            setFeatureEnabled("diagnostics", false);

            emit errorOccurred(QString("ALS server unavailable - language features disabled"));
        }
    } else if (component == "DocumentManager") {
        // Document sync issues - warn user but continue
        emit errorOccurred(QString("Document synchronization issues detected"));
    }
}

void SpectrumLspClient::cleanup()
{
    qDebug() << "SpectrumLspClient: Cleaning up resources";

    // Stop timers
    if (m_connectionTimer) {
        m_connectionTimer->stop();
    }
    if (m_healthTimer) {
        m_healthTimer->stop();
    }

    // Reset components
    m_documentManager.reset();
    m_featureManager.reset();
    m_protocol.reset();
    m_process.reset();

    // Reset state
    m_serverCapabilities = ServerCapabilities();
    m_connectionState = ConnectionState::Disconnected;
}
