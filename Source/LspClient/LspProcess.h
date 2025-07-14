#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QMutex>
#include <QDateTime>
#include <QProcessEnvironment>
#include <QStringList>

/**
 * @brief Manages the ALS server process lifecycle
 * 
 * This class handles:
 * - Starting and stopping the ALS server process
 * - Process health monitoring
 * - Automatic restart on crashes
 * - Stdio communication management
 */
class LspProcess : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Process states
     */
    enum class ProcessState {
        Stopped,     // Process is not running
        Starting,    // Process is starting up
        Running,     // Process is running normally
        Crashed,     // Process crashed unexpectedly
        Stopping     // Process is being stopped
    };

    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit LspProcess(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~LspProcess();

    /**
     * @brief Start the ALS server process
     * @param serverPath Path to the ALS server executable
     * @return true if process started successfully
     */
    bool start(const QString& serverPath);

    /**
     * @brief Stop the ALS server process
     * @param timeout Timeout in milliseconds for graceful shutdown
     */
    void stop(int timeout = 5000);

    /**
     * @brief Get current process state
     * @return Current process state
     */
    ProcessState getState() const;

    /**
     * @brief Check if process is running
     * @return true if process is running
     */
    bool isRunning() const;

    /**
     * @brief Get process ID
     * @return Process ID or -1 if not running
     */
    qint64 processId() const;

    /**
     * @brief Write data to process stdin
     * @param data Data to write
     * @return Number of bytes written
     */
    qint64 write(const QByteArray& data);

    /**
     * @brief Read data from process stdout
     * @return Available data
     */
    QByteArray readAllStandardOutput();

    /**
     * @brief Read data from process stderr
     * @return Available error data
     */
    QByteArray readAllStandardError();

    /**
     * @brief Enable or disable automatic restart on crash
     * @param enabled Whether to enable auto-restart
     */
    void setAutoRestart(bool enabled);

    /**
     * @brief Check if auto-restart is enabled
     * @return true if auto-restart is enabled
     */
    bool isAutoRestartEnabled() const;

    /**
     * @brief Get restart attempt count
     * @return Number of restart attempts made
     */
    int getRestartAttempts() const;

    /**
     * @brief Reset restart attempt counter
     */
    void resetRestartAttempts();

    /**
     * @brief Set maximum restart attempts
     * @param maxAttempts Maximum number of restart attempts
     */
    void setMaxRestartAttempts(int maxAttempts);

    /**
     * @brief Get process uptime in seconds
     * @return Process uptime or -1 if not running
     */
    qint64 getUptimeSeconds() const;

    /**
     * @brief Get process memory usage in KB
     * @return Memory usage or -1 if unavailable
     */
    qint64 getMemoryUsageKB() const;

    /**
     * @brief Check if process is responsive
     * @return true if process is responding to signals
     */
    bool isResponsive() const;

    /**
     * @brief Send signal to process for health check
     * @return true if signal was sent successfully
     */
    bool sendHealthCheck();

    /**
     * @brief Get last error message
     * @return Last error that occurred
     */
    QString getLastError() const;

    /**
     * @brief Set process environment variables
     * @param environment Environment variables to set
     */
    void setEnvironment(const QProcessEnvironment& environment);

    /**
     * @brief Set process working directory
     * @param workingDir Working directory path
     */
    void setWorkingDirectory(const QString& workingDir);

    /**
     * @brief Set process arguments
     * @param arguments Command line arguments
     */
    void setArguments(const QStringList& arguments);

signals:
    /**
     * @brief Emitted when process state changes
     * @param state New process state
     */
    void stateChanged(ProcessState state);

    /**
     * @brief Emitted when data is available on stdout
     */
    void readyReadStandardOutput();

    /**
     * @brief Emitted when data is available on stderr
     */
    void readyReadStandardError();

    /**
     * @brief Emitted when process encounters an error
     * @param error Error description
     */
    void errorOccurred(const QString& error);

    /**
     * @brief Emitted when process finishes
     * @param exitCode Process exit code
     * @param exitStatus Exit status
     */
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

    /**
     * @brief Emitted when process becomes unresponsive
     */
    void processUnresponsive();

    /**
     * @brief Emitted when process memory usage exceeds threshold
     * @param memoryKB Current memory usage in KB
     */
    void memoryThresholdExceeded(qint64 memoryKB);

    /**
     * @brief Emitted when restart attempt is made
     * @param attemptNumber Current attempt number
     */
    void restartAttempted(int attemptNumber);

    /**
     * @brief Emitted when maximum restart attempts reached
     */
    void maxRestartsReached();

private slots:
    /**
     * @brief Handle process started signal
     */
    void onProcessStarted();

    /**
     * @brief Handle process finished signal
     */
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    /**
     * @brief Handle process error
     */
    void onProcessError(QProcess::ProcessError error);

    /**
     * @brief Handle process state change
     */
    void onProcessStateChanged(QProcess::ProcessState state);

    /**
     * @brief Handle restart timer timeout
     */
    void onRestartTimer();

    /**
     * @brief Handle health check timer timeout
     */
    void onHealthCheckTimer();

    /**
     * @brief Handle memory monitoring timer timeout
     */
    void onMemoryCheckTimer();

private:
    /**
     * @brief Set process state and emit signal
     * @param state New process state
     */
    void setState(ProcessState state);

    /**
     * @brief Setup process connections
     */
    void setupConnections();

    /**
     * @brief Cleanup process resources
     */
    void cleanup();

    /**
     * @brief Schedule automatic restart
     */
    void scheduleRestart();

private:
    QProcess* m_process;
    ProcessState m_state;
    QString m_serverPath;
    bool m_autoRestart;
    int m_restartAttempts;
    int m_maxRestartAttempts;
    QTimer* m_restartTimer;
    QTimer* m_healthCheckTimer;
    QTimer* m_memoryCheckTimer;
    mutable QMutex m_stateMutex;

    // Process configuration
    QProcessEnvironment m_environment;
    QString m_workingDirectory;
    QStringList m_arguments;

    // Monitoring data
    QDateTime m_startTime;
    QString m_lastError;
    qint64 m_memoryThresholdKB;
    bool m_isResponsive;
    int m_healthCheckFailures;

    // Constants
    static const int RESTART_DELAY_MS = 2000;
    static const int MAX_RESTART_ATTEMPTS = 3;
    static const int HEALTH_CHECK_INTERVAL_MS = 30000;  // 30 seconds
    static const int MEMORY_CHECK_INTERVAL_MS = 60000;  // 60 seconds
    static const qint64 DEFAULT_MEMORY_THRESHOLD_KB = 512 * 1024;  // 512 MB
    static const int MAX_HEALTH_CHECK_FAILURES = 3;
};

Q_DECLARE_METATYPE(LspProcess::ProcessState)
