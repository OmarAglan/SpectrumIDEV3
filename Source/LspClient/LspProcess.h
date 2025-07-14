#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QMutex>

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
    mutable QMutex m_stateMutex;

    static const int RESTART_DELAY_MS = 2000;
    static const int MAX_RESTART_ATTEMPTS = 3;
};

Q_DECLARE_METATYPE(LspProcess::ProcessState)
