#include "LspProcess.h"
#include <QDebug>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#include <mach/task.h>
#endif

LspProcess::LspProcess(QObject* parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_state(ProcessState::Stopped)
    , m_autoRestart(true)
    , m_restartAttempts(0)
    , m_maxRestartAttempts(MAX_RESTART_ATTEMPTS)
    , m_restartTimer(new QTimer(this))
    , m_healthCheckTimer(new QTimer(this))
    , m_memoryCheckTimer(new QTimer(this))
    , m_memoryThresholdKB(DEFAULT_MEMORY_THRESHOLD_KB)
    , m_isResponsive(true)
    , m_healthCheckFailures(0)
{
    qDebug() << "LspProcess: Initializing enhanced process manager";

    setupConnections();

    // Setup restart timer
    m_restartTimer->setSingleShot(true);
    m_restartTimer->setInterval(RESTART_DELAY_MS);
    connect(m_restartTimer, &QTimer::timeout, this, &LspProcess::onRestartTimer);

    // Setup health check timer
    m_healthCheckTimer->setInterval(HEALTH_CHECK_INTERVAL_MS);
    connect(m_healthCheckTimer, &QTimer::timeout, this, &LspProcess::onHealthCheckTimer);

    // Setup memory monitoring timer
    m_memoryCheckTimer->setInterval(MEMORY_CHECK_INTERVAL_MS);
    connect(m_memoryCheckTimer, &QTimer::timeout, this, &LspProcess::onMemoryCheckTimer);

    // Set default environment
    m_environment = QProcessEnvironment::systemEnvironment();
}

LspProcess::~LspProcess()
{
    qDebug() << "LspProcess: Destructor called";
    stop();
    cleanup();
}

bool LspProcess::start(const QString& serverPath, const QStringList& arguments)
{
    QMutexLocker locker(&m_stateMutex);
    
    qDebug() << "LspProcess: Asynchronously starting ALS server:" << serverPath;
    
    if (m_state != ProcessState::Stopped) {
        qWarning() << "LspProcess: Process already running or starting";
        return false;
    }
    
    if (serverPath.isEmpty() || !QFile::exists(serverPath)) {
        qCritical() << "LspProcess: Server executable not found:" << serverPath;
        // The onProcessError slot will handle the state change and signal emission
        return false;
    }
    
    m_serverPath = serverPath;
    m_arguments = arguments;  // Store arguments for potential restart
    setState(ProcessState::Starting);

    // === COMPREHENSIVE DEBUGGING START ===
    qDebug() << "=== LspProcess Debug Info ===";
    qDebug() << "Server path:" << serverPath;
    qDebug() << "Arguments:" << arguments;

    // Set working directory to the same directory as the executable
    QFileInfo serverInfo(serverPath);
    QString workingDir = serverInfo.absolutePath();

    qDebug() << "Working dir will be:" << workingDir;
    qDebug() << "File exists:" << serverInfo.exists();
    qDebug() << "Is executable:" << serverInfo.isExecutable();
    qDebug() << "File size:" << serverInfo.size();
    qDebug() << "File permissions:" << serverInfo.permissions();
    qDebug() << "Absolute file path:" << serverInfo.absoluteFilePath();

    // Check working directory
    QDir workDir(workingDir);
    qDebug() << "Working dir exists:" << workDir.exists();
    qDebug() << "Working dir is readable:" << workDir.isReadable();

    // Environment debugging
    qDebug() << "Current environment PATH:" << m_environment.value("PATH");
    qDebug() << "System environment PATH:" << qgetenv("PATH");

#ifdef Q_OS_WIN
    // Windows-specific debugging
    qDebug() << "=== Windows Debug Info ===";
    qDebug() << "Current user:" << qgetenv("USERNAME");
    qDebug() << "Current directory:" << QDir::currentPath();

    // Test basic command execution
    QProcess testProcess;
    testProcess.start("cmd", QStringList() << "/c" << "echo" << "Windows CMD test");
    if (testProcess.waitForFinished(2000)) {
        qDebug() << "CMD test successful, output:" << testProcess.readAllStandardOutput();
    } else {
        qDebug() << "CMD test failed:" << testProcess.errorString();
    }

    // Check if executable can be found by Windows
    QProcess whereProcess;
    whereProcess.start("cmd", QStringList() << "/c" << "where" << serverPath);
    if (whereProcess.waitForFinished(2000)) {
        qDebug() << "Where command output:" << whereProcess.readAllStandardOutput();
    } else {
        qDebug() << "Where command failed:" << whereProcess.errorString();
    }
    qDebug() << "=== End Windows Debug Info ===";
#endif

    m_process->setProcessEnvironment(m_environment);
    m_process->setWorkingDirectory(workingDir);

    qDebug() << "About to call QProcess::start()...";

    // The onProcessStarted and onProcessError slots will handle the result.
    m_process->start(serverPath, arguments);

    // Immediate state check
    qDebug() << "QProcess state immediately after start():" << m_process->state();
    qDebug() << "QProcess error immediately after start():" << m_process->error();
    qDebug() << "QProcess error string:" << m_process->errorString();

    // Check if the process started immediately
    if (m_process->state() == QProcess::NotRunning) {
        qCritical() << "LspProcess: Process failed to start immediately";
        qCritical() << "LspProcess: Process error:" << m_process->errorString();
        return false;
    }

    qDebug() << "Process start call completed, current state:" << m_process->state();
    qDebug() << "=== End Debug Info ===";

    // Add periodic state monitoring
    QTimer::singleShot(1000, this, [this]() {
        qDebug() << "QProcess state after 1s:" << m_process->state() << "Error:" << m_process->error();
    });

    QTimer::singleShot(3000, this, [this]() {
        qDebug() << "QProcess state after 3s:" << m_process->state() << "Error:" << m_process->error();
        if (m_process->state() == QProcess::Starting) {
            qWarning() << "Process still in Starting state after 3 seconds - likely hanging!";
        }
    });

    // The function now returns true if the start *attempt* was successful.
    // The actual success/failure is communicated via signals.
    return true;
}

void LspProcess::stop(int timeout)
{
    QMutexLocker locker(&m_stateMutex);
    
    qDebug() << "LspProcess: Stopping server process";
    
    if (m_state == ProcessState::Stopped || m_state == ProcessState::Stopping) {
        return;
    }
    
    setState(ProcessState::Stopping);

    // Stop all monitoring timers
    m_restartTimer->stop();
    m_healthCheckTimer->stop();
    m_memoryCheckTimer->stop();
    
    if (m_process->state() == QProcess::Running) {
        // Try graceful shutdown first
        m_process->terminate();
        
        if (!m_process->waitForFinished(timeout)) {
            qWarning() << "LspProcess: Graceful shutdown failed, killing process";
            m_process->kill();
            m_process->waitForFinished(2000);
        }
    }
    
    setState(ProcessState::Stopped);
    qDebug() << "LspProcess: Server process stopped";
}

LspProcess::ProcessState LspProcess::getState() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state;
}

bool LspProcess::isRunning() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state == ProcessState::Running;
}

qint64 LspProcess::processId() const
{
    if (m_process && m_process->state() == QProcess::Running) {
        return m_process->processId();
    }
    return -1;
}

qint64 LspProcess::write(const QByteArray& data)
{
    if (!isRunning()) {
        qWarning() << "LspProcess: Cannot write to stopped process";
        return -1;
    }
    
    qint64 bytesWritten = m_process->write(data);
    if (bytesWritten == -1) {
        qWarning() << "LspProcess: Failed to write to process:" << m_process->errorString();
    }
    
    return bytesWritten;
}

QByteArray LspProcess::readAllStandardOutput()
{
    if (m_process) {
        return m_process->readAllStandardOutput();
    }
    return QByteArray();
}

QByteArray LspProcess::readAllStandardError()
{
    if (m_process) {
        return m_process->readAllStandardError();
    }
    return QByteArray();
}

void LspProcess::setAutoRestart(bool enabled)
{
    QMutexLocker locker(&m_stateMutex);
    m_autoRestart = enabled;
    qDebug() << "LspProcess: Auto-restart" << (enabled ? "enabled" : "disabled");
}

bool LspProcess::isAutoRestartEnabled() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_autoRestart;
}

int LspProcess::getRestartAttempts() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_restartAttempts;
}

void LspProcess::resetRestartAttempts()
{
    QMutexLocker locker(&m_stateMutex);
    m_restartAttempts = 0;
    qDebug() << "LspProcess: Restart attempts reset";
}

void LspProcess::setMaxRestartAttempts(int maxAttempts)
{
    QMutexLocker locker(&m_stateMutex);
    m_maxRestartAttempts = maxAttempts;
    qDebug() << "LspProcess: Max restart attempts set to" << maxAttempts;
}

qint64 LspProcess::getUptimeSeconds() const
{
    QMutexLocker locker(&m_stateMutex);
    if (m_state != ProcessState::Running || !m_startTime.isValid()) {
        return -1;
    }
    return m_startTime.secsTo(QDateTime::currentDateTime());
}

qint64 LspProcess::getMemoryUsageKB() const
{
    if (!isRunning()) {
        return -1;
    }

    qint64 pid = processId();
    if (pid == -1) {
        return -1;
    }

#ifdef Q_OS_WIN
    // Windows implementation using Windows API
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL) {
        return -1;
    }

    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        CloseHandle(hProcess);
        return pmc.WorkingSetSize / 1024; // Convert to KB
    }
    CloseHandle(hProcess);
    return -1;
#elif defined(Q_OS_LINUX)
    // Linux implementation using /proc/[pid]/status
    QFile statusFile(QString("/proc/%1/status").arg(pid));
    if (!statusFile.open(QIODevice::ReadOnly)) {
        return -1;
    }

    QTextStream stream(&statusFile);
    QString line;
    while (stream.readLineInto(&line)) {
        if (line.startsWith("VmRSS:")) {
            QStringList parts = line.split(QRegularExpression("\\s+"));
            if (parts.size() >= 2) {
                return parts[1].toLongLong();
            }
        }
    }
    return -1;
#elif defined(Q_OS_MACOS)
    // macOS implementation using task_info
    task_t task;
    if (task_for_pid(mach_task_self(), pid, &task) != KERN_SUCCESS) {
        return -1;
    }

    task_basic_info_data_t info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    if (task_info(task, TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
        return info.resident_size / 1024; // Convert to KB
    }
    return -1;
#else
    // Fallback: not implemented for this platform
    return -1;
#endif
}

bool LspProcess::isResponsive() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_isResponsive;
}

bool LspProcess::sendHealthCheck()
{
    if (!isRunning()) {
        return false;
    }

    // For now, we'll just check if the process is still running
    // In a more advanced implementation, we could send a specific LSP request
    return m_process->state() == QProcess::Running;
}

QString LspProcess::getLastError() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_lastError;
}

void LspProcess::setEnvironment(const QProcessEnvironment& environment)
{
    QMutexLocker locker(&m_stateMutex);
    m_environment = environment;
    qDebug() << "LspProcess: Environment updated";
}

void LspProcess::setWorkingDirectory(const QString& workingDir)
{
    QMutexLocker locker(&m_stateMutex);
    m_workingDirectory = workingDir;
    qDebug() << "LspProcess: Working directory set to" << workingDir;
}

void LspProcess::setArguments(const QStringList& arguments)
{
    QMutexLocker locker(&m_stateMutex);
    m_arguments = arguments;
    qDebug() << "LspProcess: Arguments set to" << arguments;
}

void LspProcess::onProcessStarted()
{
    qDebug() << "*** LspProcess::onProcessStarted() CALLED! ***";
    qDebug() << "LspProcess: Process started successfully, PID:" << m_process->processId();
    qDebug() << "LspProcess: Process arguments were:" << m_arguments;
    qDebug() << "LspProcess: Process state:" << m_process->state();
    qDebug() << "LspProcess: Working directory:" << m_process->workingDirectory();

    setState(ProcessState::Running);
    m_restartAttempts = 0; // Reset restart attempts on successful start
    m_startTime = QDateTime::currentDateTime();

    qDebug() << "*** LspProcess::onProcessStarted() COMPLETED ***";
    m_isResponsive = true;
    m_healthCheckFailures = 0;
    m_lastError.clear();

    // Start monitoring timers
    m_healthCheckTimer->start();
    m_memoryCheckTimer->start();

    qDebug() << "LspProcess: Monitoring started for PID:" << m_process->processId();
}

void LspProcess::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "LspProcess: Process finished with exit code:" << exitCode 
             << "status:" << exitStatus;
    
    if (m_state == ProcessState::Stopping) {
        // Expected shutdown
        setState(ProcessState::Stopped);
    } else {
        // Unexpected shutdown
        setState(ProcessState::Crashed);
        
        if (m_autoRestart && m_restartAttempts < m_maxRestartAttempts) {
            qDebug() << "LspProcess: Scheduling restart attempt" << (m_restartAttempts + 1);
            scheduleRestart();
        } else {
            qWarning() << "LspProcess: Max restart attempts reached or auto-restart disabled";
            emit errorOccurred("Server process crashed and cannot be restarted");
        }
    }
    
    emit finished(exitCode, exitStatus);
}

void LspProcess::onProcessError(QProcess::ProcessError error)
{
    QString errorString;
    switch (error) {
        case QProcess::FailedToStart:
            errorString = "Failed to start";
            break;
        case QProcess::Crashed:
            errorString = "Process crashed";
            break;
        case QProcess::Timedout:
            errorString = "Process timed out";
            break;
        case QProcess::WriteError:
            errorString = "Write error";
            break;
        case QProcess::ReadError:
            errorString = "Read error";
            break;
        default:
            errorString = "Unknown error";
            break;
    }

    // Store last error
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastError = errorString;
        m_isResponsive = false;
    }

    qCritical() << "LspProcess: Process error:" << errorString;
    qCritical() << "LspProcess: Process state when error occurred:" << m_process->state();
    qCritical() << "LspProcess: Process exit code:" << m_process->exitCode();
    qCritical() << "LspProcess: Process exit status:" << m_process->exitStatus();
    qCritical() << "LspProcess: Working directory was:" << m_process->workingDirectory();
    qCritical() << "LspProcess: Server path was:" << m_serverPath;
    qCritical() << "LspProcess: Arguments were:" << m_arguments;

    emit errorOccurred(errorString);

    if (error == QProcess::FailedToStart || error == QProcess::Crashed) {
        setState(ProcessState::Crashed);
    }
}

void LspProcess::onProcessStateChanged(QProcess::ProcessState state)
{
    qDebug() << "LspProcess: QProcess state changed to:" << state;
    // QProcess state changes are handled by other slots
}

void LspProcess::onRestartTimer()
{
    qDebug() << "LspProcess: Attempting restart" << (m_restartAttempts + 1);

    m_restartAttempts++;
    emit restartAttempted(m_restartAttempts);

    if (!m_serverPath.isEmpty()) {
        start(m_serverPath, m_arguments);
    } else {
        qCritical() << "LspProcess: Cannot restart - no server path";
        emit errorOccurred("Cannot restart server - no server path");
    }
}

void LspProcess::onHealthCheckTimer()
{
    if (!isRunning()) {
        return;
    }

    qDebug() << "LspProcess: Performing health check";

    bool responsive = sendHealthCheck();

    {
        QMutexLocker locker(&m_stateMutex);
        if (!responsive) {
            m_healthCheckFailures++;
            m_isResponsive = false;

            qWarning() << "LspProcess: Health check failed" << m_healthCheckFailures
                      << "of" << MAX_HEALTH_CHECK_FAILURES;

            if (m_healthCheckFailures >= MAX_HEALTH_CHECK_FAILURES) {
                qCritical() << "LspProcess: Process unresponsive after"
                           << MAX_HEALTH_CHECK_FAILURES << "failed health checks";
                emit processUnresponsive();

                // Trigger restart if auto-restart is enabled
                if (m_autoRestart && m_restartAttempts < m_maxRestartAttempts) {
                    qDebug() << "LspProcess: Triggering restart due to unresponsiveness";
                    setState(ProcessState::Crashed);
                    scheduleRestart();
                }
            }
        } else {
            // Reset failure count on successful health check
            if (m_healthCheckFailures > 0) {
                qDebug() << "LspProcess: Health check recovered";
            }
            m_healthCheckFailures = 0;
            m_isResponsive = true;
        }
    }
}

void LspProcess::onMemoryCheckTimer()
{
    if (!isRunning()) {
        return;
    }

    qint64 memoryKB = getMemoryUsageKB();
    if (memoryKB > 0) {
        qDebug() << "LspProcess: Memory usage:" << memoryKB << "KB";

        if (memoryKB > m_memoryThresholdKB) {
            qWarning() << "LspProcess: Memory usage" << memoryKB
                      << "KB exceeds threshold" << m_memoryThresholdKB << "KB";
            emit memoryThresholdExceeded(memoryKB);
        }
    }
}

void LspProcess::setState(ProcessState state)
{
    if (m_state != state) {
        ProcessState oldState = m_state;
        m_state = state;
        
        qDebug() << "LspProcess: State changed from" << static_cast<int>(oldState) 
                 << "to" << static_cast<int>(state);
        
        emit stateChanged(state);
    }
}

void LspProcess::setupConnections()
{
    connect(m_process, &QProcess::started, this, &LspProcess::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LspProcess::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &LspProcess::onProcessError);
    connect(m_process, &QProcess::stateChanged, this, &LspProcess::onProcessStateChanged);
    connect(m_process, &QProcess::readyReadStandardOutput, 
            this, &LspProcess::readyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, 
            this, &LspProcess::readyReadStandardError);
}

void LspProcess::cleanup()
{
    // Stop all timers
    if (m_restartTimer) {
        m_restartTimer->stop();
    }
    if (m_healthCheckTimer) {
        m_healthCheckTimer->stop();
    }
    if (m_memoryCheckTimer) {
        m_memoryCheckTimer->stop();
    }

    // Cleanup process
    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(2000);
        }
    }

    // Reset state
    {
        QMutexLocker locker(&m_stateMutex);
        m_isResponsive = false;
        m_healthCheckFailures = 0;
        m_startTime = QDateTime();
    }
}

void LspProcess::scheduleRestart()
{
    if (m_restartAttempts >= m_maxRestartAttempts) {
        qCritical() << "LspProcess: Maximum restart attempts" << m_maxRestartAttempts << "reached";
        emit maxRestartsReached();
        return;
    }

    if (m_restartTimer && !m_restartTimer->isActive()) {
        int delay = RESTART_DELAY_MS * (m_restartAttempts + 1); // Exponential backoff
        m_restartTimer->setInterval(delay);
        m_restartTimer->start();
        qDebug() << "LspProcess: Restart scheduled in" << delay << "ms (attempt"
                 << (m_restartAttempts + 1) << "of" << m_maxRestartAttempts << ")";
    }
}
