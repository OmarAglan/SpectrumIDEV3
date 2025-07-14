#include "ErrorManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>

ErrorManager::ErrorManager(QObject* parent)
    : QObject(parent)
    , m_recoveryTimer(new QTimer(this))
    , m_autoRecoveryEnabled(true)
    , m_errorRateTimer(new QTimer(this))
    , m_totalErrors(0)
    , m_recoveryAttempts(0)
    , m_successfulRecoveries(0)
{
    qDebug() << "ErrorManager: Initializing error management system";
    
    // Setup recovery timer
    m_recoveryTimer->setInterval(RECOVERY_TIMER_INTERVAL_MS);
    connect(m_recoveryTimer, &QTimer::timeout, this, &ErrorManager::onRecoveryTimer);
    m_recoveryTimer->start();
    
    // Setup error rate monitoring timer
    m_errorRateTimer->setInterval(ERROR_RATE_CHECK_INTERVAL_MS);
    connect(m_errorRateTimer, &QTimer::timeout, this, &ErrorManager::onErrorRateTimer);
    m_errorRateTimer->start();
    
    // Set default error rate thresholds
    setErrorRateThreshold(ErrorCategory::ProcessError, 5, 60000);      // 5 errors per minute
    setErrorRateThreshold(ErrorCategory::CommunicationError, 10, 60000); // 10 errors per minute
    setErrorRateThreshold(ErrorCategory::ProtocolError, 15, 60000);    // 15 errors per minute
    setErrorRateThreshold(ErrorCategory::TimeoutError, 8, 60000);      // 8 errors per minute
}

ErrorManager::~ErrorManager()
{
    qDebug() << "ErrorManager: Shutting down error management system";
    
    if (m_recoveryTimer) {
        m_recoveryTimer->stop();
    }
    if (m_errorRateTimer) {
        m_errorRateTimer->stop();
    }
}

QString ErrorManager::reportError(ErrorSeverity severity, ErrorCategory category,
                                 const QString& component, const QString& message,
                                 const QString& technicalDetails,
                                 const QJsonObject& context)
{
    QMutexLocker locker(&m_errorMutex);
    
    // Create error info
    ErrorInfo errorInfo;
    errorInfo.id = generateErrorId();
    errorInfo.severity = severity;
    errorInfo.category = category;
    errorInfo.component = component;
    errorInfo.message = message;
    errorInfo.technicalDetails = technicalDetails;
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.context = context;
    errorInfo.retryCount = 0;
    errorInfo.isRecoverable = (severity != ErrorSeverity::Fatal);
    
    // Determine recovery strategy based on category and severity
    if (severity == ErrorSeverity::Fatal) {
        errorInfo.strategy = RecoveryStrategy::UserIntervention;
    } else if (category == ErrorCategory::ProcessError) {
        errorInfo.strategy = RecoveryStrategy::Restart;
    } else if (category == ErrorCategory::CommunicationError) {
        errorInfo.strategy = RecoveryStrategy::Reconnect;
    } else if (category == ErrorCategory::TimeoutError) {
        errorInfo.strategy = RecoveryStrategy::Retry;
    } else {
        errorInfo.strategy = RecoveryStrategy::Fallback;
    }
    
    // Add to error history
    m_errorHistory.enqueue(errorInfo);
    if (m_errorHistory.size() > MAX_ERROR_HISTORY) {
        m_errorHistory.dequeue();
    }
    
    // Update statistics
    m_totalErrors++;
    m_errorCounts[category]++;
    m_componentErrorCounts[component]++;
    
    // Track error timestamps for rate monitoring
    m_errorTimestamps[category].enqueue(errorInfo.timestamp);
    
    // Log error
    QString severityStr = severityToString(severity);
    QString categoryStr = categoryToString(category);
    
    qDebug() << QString("ErrorManager: [%1] [%2] [%3] %4")
                .arg(severityStr, categoryStr, component, message);
    
    if (!technicalDetails.isEmpty()) {
        qDebug() << "ErrorManager: Technical details:" << technicalDetails;
    }
    
    // Emit signals based on severity
    if (severity >= ErrorSeverity::Critical) {
        emit criticalErrorOccurred(errorInfo);
    }
    
    // Attempt automatic recovery if enabled
    if (m_autoRecoveryEnabled && errorInfo.isRecoverable) {
        attemptRecovery(errorInfo);
    }
    
    return errorInfo.id;
}

void ErrorManager::registerRecoveryAction(ErrorCategory category, RecoveryStrategy strategy,
                                         RecoveryAction action)
{
    qDebug() << "ErrorManager: Registering recovery action for category"
             << categoryToString(category) << "strategy" << static_cast<int>(strategy);
    
    m_recoveryActions[category] = qMakePair(strategy, action);
}

void ErrorManager::setErrorRateThreshold(ErrorCategory category, int maxErrors, int timeWindowMs)
{
    qDebug() << "ErrorManager: Setting error rate threshold for"
             << categoryToString(category) << ":" << maxErrors << "errors in" << timeWindowMs << "ms";
    
    m_errorRateThresholds[category] = qMakePair(maxErrors, timeWindowMs);
}

void ErrorManager::setAutoRecoveryEnabled(bool enabled)
{
    m_autoRecoveryEnabled = enabled;
    qDebug() << "ErrorManager: Auto-recovery" << (enabled ? "enabled" : "disabled");
}

QJsonObject ErrorManager::getErrorStatistics() const
{
    QMutexLocker locker(&m_errorMutex);
    
    QJsonObject stats;
    stats["totalErrors"] = m_totalErrors;
    stats["recoveryAttempts"] = m_recoveryAttempts;
    stats["successfulRecoveries"] = m_successfulRecoveries;
    stats["autoRecoveryEnabled"] = m_autoRecoveryEnabled;
    
    // Error counts by category
    QJsonObject categoryCounts;
    for (auto it = m_errorCounts.begin(); it != m_errorCounts.end(); ++it) {
        categoryCounts[categoryToString(it.key())] = it.value();
    }
    stats["errorsByCategory"] = categoryCounts;
    
    // Error counts by component
    QJsonObject componentCounts;
    for (auto it = m_componentErrorCounts.begin(); it != m_componentErrorCounts.end(); ++it) {
        componentCounts[it.key()] = it.value();
    }
    stats["errorsByComponent"] = componentCounts;
    
    // Degraded components
    QJsonArray degradedComponents;
    for (const QString& component : m_degradedComponents) {
        degradedComponents.append(component);
    }
    stats["degradedComponents"] = degradedComponents;
    
    return stats;
}

QList<ErrorInfo> ErrorManager::getRecentErrors(int maxCount) const
{
    QMutexLocker locker(&m_errorMutex);
    
    QList<ErrorInfo> recentErrors;
    int count = qMin(maxCount, m_errorHistory.size());
    
    // Get the most recent errors
    auto it = m_errorHistory.end() - count;
    while (it != m_errorHistory.end()) {
        recentErrors.append(*it);
        ++it;
    }
    
    return recentErrors;
}

bool ErrorManager::isComponentDegraded(const QString& component) const
{
    return m_degradedComponents.contains(component);
}

bool ErrorManager::triggerRecovery(const QString& component)
{
    qDebug() << "ErrorManager: Manual recovery triggered for component:" << component;
    
    // Find recent errors for this component
    QMutexLocker locker(&m_errorMutex);
    
    for (auto it = m_errorHistory.rbegin(); it != m_errorHistory.rend(); ++it) {
        if (it->component == component && it->isRecoverable) {
            locker.unlock();
            return attemptRecovery(*it);
        }
    }
    
    qWarning() << "ErrorManager: No recoverable errors found for component:" << component;
    return false;
}

void ErrorManager::clearErrorHistory()
{
    QMutexLocker locker(&m_errorMutex);
    
    m_errorHistory.clear();
    m_errorCounts.clear();
    m_componentErrorCounts.clear();
    m_errorTimestamps.clear();
    m_degradedComponents.clear();
    m_totalErrors = 0;
    
    qDebug() << "ErrorManager: Error history cleared";
}

QString ErrorManager::generateErrorId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool ErrorManager::attemptRecovery(const ErrorInfo& errorInfo)
{
    if (!errorInfo.isRecoverable || errorInfo.retryCount >= DEFAULT_MAX_RETRY_ATTEMPTS) {
        return false;
    }
    
    auto it = m_recoveryActions.find(errorInfo.category);
    if (it == m_recoveryActions.end()) {
        qDebug() << "ErrorManager: No recovery action registered for category"
                 << categoryToString(errorInfo.category);
        return false;
    }
    
    RecoveryStrategy strategy = it.value().first;
    RecoveryAction action = it.value().second;
    
    qDebug() << "ErrorManager: Attempting recovery for" << errorInfo.component
             << "using strategy" << static_cast<int>(strategy);
    
    m_recoveryAttempts++;
    emit recoveryAttempted(errorInfo.component, strategy);
    
    bool success = false;
    try {
        success = action();
    } catch (const std::exception& e) {
        qCritical() << "ErrorManager: Recovery action threw exception:" << e.what();
        success = false;
    }
    
    if (success) {
        m_successfulRecoveries++;
        emit recoverySucceeded(errorInfo.component);
        
        // Remove component from degraded list if it was there
        m_degradedComponents.removeAll(errorInfo.component);
        
        qDebug() << "ErrorManager: Recovery succeeded for" << errorInfo.component;
    } else {
        emit recoveryFailed(errorInfo.component, "Recovery action failed");
        
        // Consider degrading the component if recovery keeps failing
        if (errorInfo.retryCount >= DEFAULT_MAX_RETRY_ATTEMPTS - 1) {
            enterDegradedMode(errorInfo.component, "Multiple recovery failures");
        }
        
        qWarning() << "ErrorManager: Recovery failed for" << errorInfo.component;
    }
    
    return success;
}

void ErrorManager::onRecoveryTimer()
{
    // This timer can be used for periodic recovery checks
    // For now, we'll just clean up old error timestamps
    QMutexLocker locker(&m_errorMutex);

    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-300); // 5 minutes ago

    for (auto& queue : m_errorTimestamps) {
        while (!queue.isEmpty() && queue.first() < cutoff) {
            queue.dequeue();
        }
    }
}

void ErrorManager::onErrorRateTimer()
{
    // Check error rates for all categories
    for (auto it = m_errorRateThresholds.begin(); it != m_errorRateThresholds.end(); ++it) {
        ErrorCategory category = it.key();
        double errorRate = checkErrorRate(category);

        int maxErrors = it.value().first;
        int timeWindowMs = it.value().second;
        double threshold = static_cast<double>(maxErrors) / (timeWindowMs / 1000.0);

        if (errorRate > threshold) {
            qWarning() << "ErrorManager: Error rate threshold exceeded for"
                      << categoryToString(category) << ":" << errorRate << "errors/sec";
            emit errorRateThresholdExceeded(category, errorRate);
        }
    }
}

double ErrorManager::checkErrorRate(ErrorCategory category)
{
    QMutexLocker locker(&m_errorMutex);

    auto it = m_errorTimestamps.find(category);
    if (it == m_errorTimestamps.end()) {
        return 0.0;
    }

    const QQueue<QDateTime>& timestamps = it.value();
    if (timestamps.isEmpty()) {
        return 0.0;
    }

    // Calculate error rate over the last minute
    QDateTime oneMinuteAgo = QDateTime::currentDateTime().addSecs(-60);
    int recentErrors = 0;

    for (const QDateTime& timestamp : timestamps) {
        if (timestamp > oneMinuteAgo) {
            recentErrors++;
        }
    }

    return recentErrors / 60.0; // errors per second
}

void ErrorManager::enterDegradedMode(const QString& component, const QString& reason)
{
    if (!m_degradedComponents.contains(component)) {
        m_degradedComponents.append(component);
        emit componentDegraded(component, reason);
        qWarning() << "ErrorManager: Component" << component << "entered degraded mode:" << reason;
    }
}

QString ErrorManager::severityToString(ErrorSeverity severity) const
{
    switch (severity) {
        case ErrorSeverity::Info: return "INFO";
        case ErrorSeverity::Warning: return "WARNING";
        case ErrorSeverity::Error: return "ERROR";
        case ErrorSeverity::Critical: return "CRITICAL";
        case ErrorSeverity::Fatal: return "FATAL";
        default: return "UNKNOWN";
    }
}

QString ErrorManager::categoryToString(ErrorCategory category) const
{
    switch (category) {
        case ErrorCategory::ProcessError: return "PROCESS";
        case ErrorCategory::CommunicationError: return "COMMUNICATION";
        case ErrorCategory::ProtocolError: return "PROTOCOL";
        case ErrorCategory::DocumentError: return "DOCUMENT";
        case ErrorCategory::ConfigurationError: return "CONFIGURATION";
        case ErrorCategory::ResourceError: return "RESOURCE";
        case ErrorCategory::TimeoutError: return "TIMEOUT";
        case ErrorCategory::UnknownError: return "UNKNOWN";
        default: return "UNDEFINED";
    }
}
