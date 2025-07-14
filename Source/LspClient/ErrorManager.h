#pragma once

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QDateTime>
#include <QJsonObject>
#include <QStringList>
#include <functional>

/**
 * @brief Error severity levels
 */
enum class ErrorSeverity {
    Info = 0,
    Warning = 1,
    Error = 2,
    Critical = 3,
    Fatal = 4
};

/**
 * @brief Error categories for classification
 */
enum class ErrorCategory {
    ProcessError,      // Process start/stop/crash errors
    CommunicationError, // LSP communication errors
    ProtocolError,     // JSON-RPC protocol errors
    DocumentError,     // Document synchronization errors
    ConfigurationError, // Configuration/setup errors
    ResourceError,     // Memory/disk/network errors
    TimeoutError,      // Timeout-related errors
    UnknownError       // Unclassified errors
};

/**
 * @brief Recovery strategies
 */
enum class RecoveryStrategy {
    None,              // No automatic recovery
    Retry,             // Simple retry
    Restart,           // Restart component/process
    Reconnect,         // Reconnect to server
    Fallback,          // Use fallback functionality
    GracefulDegradation, // Disable features gracefully
    UserIntervention   // Require user action
};

/**
 * @brief Error information structure
 */
struct ErrorInfo {
    QString id;                    // Unique error ID
    ErrorSeverity severity;        // Error severity
    ErrorCategory category;        // Error category
    QString component;             // Component that generated error
    QString message;               // Human-readable error message
    QString technicalDetails;     // Technical details for debugging
    QDateTime timestamp;           // When error occurred
    QJsonObject context;           // Additional context data
    RecoveryStrategy strategy;     // Suggested recovery strategy
    int retryCount;               // Number of retry attempts
    bool isRecoverable;           // Whether error can be recovered from
};

/**
 * @brief Recovery action function type
 */
using RecoveryAction = std::function<bool()>;

/**
 * @brief Manages error handling, recovery, and graceful degradation
 * 
 * This class provides:
 * - Centralized error collection and classification
 * - Automatic recovery strategies
 * - Error rate monitoring and circuit breaker patterns
 * - Graceful degradation when recovery fails
 * - Error reporting and logging
 */
class ErrorManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit ErrorManager(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ErrorManager();

    /**
     * @brief Report an error
     * @param severity Error severity
     * @param category Error category
     * @param component Component name
     * @param message Error message
     * @param technicalDetails Technical details
     * @param context Additional context
     * @return Error ID
     */
    QString reportError(ErrorSeverity severity, ErrorCategory category,
                       const QString& component, const QString& message,
                       const QString& technicalDetails = QString(),
                       const QJsonObject& context = QJsonObject());

    /**
     * @brief Register recovery action for error category
     * @param category Error category
     * @param strategy Recovery strategy
     * @param action Recovery action function
     */
    void registerRecoveryAction(ErrorCategory category, RecoveryStrategy strategy,
                               RecoveryAction action);

    /**
     * @brief Set error rate threshold for circuit breaker
     * @param category Error category
     * @param maxErrors Maximum errors in time window
     * @param timeWindowMs Time window in milliseconds
     */
    void setErrorRateThreshold(ErrorCategory category, int maxErrors, int timeWindowMs);

    /**
     * @brief Enable/disable automatic recovery
     * @param enabled Whether to enable automatic recovery
     */
    void setAutoRecoveryEnabled(bool enabled);

    /**
     * @brief Get error statistics
     * @return JSON object with error statistics
     */
    QJsonObject getErrorStatistics() const;

    /**
     * @brief Get recent errors
     * @param maxCount Maximum number of errors to return
     * @return List of recent errors
     */
    QList<ErrorInfo> getRecentErrors(int maxCount = 50) const;

    /**
     * @brief Check if component is in degraded mode
     * @param component Component name
     * @return true if component is degraded
     */
    bool isComponentDegraded(const QString& component) const;

    /**
     * @brief Manually trigger recovery for component
     * @param component Component name
     * @return true if recovery was attempted
     */
    bool triggerRecovery(const QString& component);

    /**
     * @brief Clear error history
     */
    void clearErrorHistory();

signals:
    /**
     * @brief Emitted when a critical error occurs
     * @param errorInfo Error information
     */
    void criticalErrorOccurred(const ErrorInfo& errorInfo);

    /**
     * @brief Emitted when recovery is attempted
     * @param component Component name
     * @param strategy Recovery strategy
     */
    void recoveryAttempted(const QString& component, RecoveryStrategy strategy);

    /**
     * @brief Emitted when recovery succeeds
     * @param component Component name
     */
    void recoverySucceeded(const QString& component);

    /**
     * @brief Emitted when recovery fails
     * @param component Component name
     * @param reason Failure reason
     */
    void recoveryFailed(const QString& component, const QString& reason);

    /**
     * @brief Emitted when component enters degraded mode
     * @param component Component name
     * @param reason Degradation reason
     */
    void componentDegraded(const QString& component, const QString& reason);

    /**
     * @brief Emitted when error rate threshold is exceeded
     * @param category Error category
     * @param errorRate Current error rate
     */
    void errorRateThresholdExceeded(ErrorCategory category, double errorRate);

private slots:
    /**
     * @brief Handle recovery timer timeout
     */
    void onRecoveryTimer();

    /**
     * @brief Handle error rate monitoring timer
     */
    void onErrorRateTimer();

private:
    /**
     * @brief Generate unique error ID
     * @return Unique error ID
     */
    QString generateErrorId();

    /**
     * @brief Attempt automatic recovery for error
     * @param errorInfo Error information
     * @return true if recovery was attempted
     */
    bool attemptRecovery(const ErrorInfo& errorInfo);

    /**
     * @brief Check error rate for category
     * @param category Error category
     * @return Current error rate (errors per second)
     */
    double checkErrorRate(ErrorCategory category);

    /**
     * @brief Enter degraded mode for component
     * @param component Component name
     * @param reason Degradation reason
     */
    void enterDegradedMode(const QString& component, const QString& reason);

    /**
     * @brief Convert error severity to string
     * @param severity Error severity
     * @return String representation
     */
    QString severityToString(ErrorSeverity severity) const;

    /**
     * @brief Convert error category to string
     * @param category Error category
     * @return String representation
     */
    QString categoryToString(ErrorCategory category) const;

private:
    // Error storage
    QQueue<ErrorInfo> m_errorHistory;
    mutable QMutex m_errorMutex;
    
    // Recovery management
    QMap<ErrorCategory, QPair<RecoveryStrategy, RecoveryAction>> m_recoveryActions;
    QTimer* m_recoveryTimer;
    bool m_autoRecoveryEnabled;
    
    // Circuit breaker pattern
    QMap<ErrorCategory, QPair<int, int>> m_errorRateThresholds; // maxErrors, timeWindowMs
    QMap<ErrorCategory, QQueue<QDateTime>> m_errorTimestamps;
    QTimer* m_errorRateTimer;
    
    // Component state tracking
    QStringList m_degradedComponents;
    
    // Statistics
    QMap<ErrorCategory, int> m_errorCounts;
    QMap<QString, int> m_componentErrorCounts;
    int m_totalErrors;
    int m_recoveryAttempts;
    int m_successfulRecoveries;
    
    // Configuration
    static const int MAX_ERROR_HISTORY = 1000;
    static const int RECOVERY_TIMER_INTERVAL_MS = 1000;
    static const int ERROR_RATE_CHECK_INTERVAL_MS = 5000;
    static const int DEFAULT_MAX_RETRY_ATTEMPTS = 3;
};
