#include "LspProcess.h"
#include <QDebug>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QDir>

LspProcess::LspProcess(QObject* parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_state(ProcessState::Stopped)
    , m_autoRestart(true)
    , m_restartAttempts(0)
    , m_maxRestartAttempts(MAX_RESTART_ATTEMPTS)
    , m_restartTimer(new QTimer(this))
{
    qDebug() << "LspProcess: Initializing process manager";
    
    setupConnections();
    
    m_restartTimer->setSingleShot(true);
    m_restartTimer->setInterval(RESTART_DELAY_MS);
    connect(m_restartTimer, &QTimer::timeout, this, &LspProcess::onRestartTimer);
}

LspProcess::~LspProcess()
{
    qDebug() << "LspProcess: Destructor called";
    stop();
    cleanup();
}

bool LspProcess::start(const QString& serverPath)
{
    QMutexLocker locker(&m_stateMutex);
    
    qDebug() << "LspProcess: Starting ALS server:" << serverPath;
    
    if (m_state != ProcessState::Stopped) {
        qWarning() << "LspProcess: Process already running or starting";
        return false;
    }
    
    if (serverPath.isEmpty() || !QFile::exists(serverPath)) {
        qCritical() << "LspProcess: Server executable not found:" << serverPath;
        emit errorOccurred(QString("Server executable not found: %1").arg(serverPath));
        return false;
    }
    
    m_serverPath = serverPath;
    setState(ProcessState::Starting);
    
    // Set up process environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    m_process->setProcessEnvironment(env);
    
    // Set working directory to server directory
    QFileInfo serverInfo(serverPath);
    m_process->setWorkingDirectory(serverInfo.absolutePath());
    
    // Start the process with stdio communication
    QStringList arguments;
    arguments << "--stdio";  // Use stdio for LSP communication
    
    qDebug() << "LspProcess: Starting process with arguments:" << arguments;
    m_process->start(serverPath, arguments);
    
    // Wait for process to start (with timeout)
    if (!m_process->waitForStarted(5000)) {
        qCritical() << "LspProcess: Failed to start server process:" << m_process->errorString();
        emit errorOccurred(QString("Failed to start server: %1").arg(m_process->errorString()));
        setState(ProcessState::Stopped);
        return false;
    }
    
    qDebug() << "LspProcess: Server process started successfully, PID:" << m_process->processId();
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
    m_restartTimer->stop();
    
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

void LspProcess::onProcessStarted()
{
    qDebug() << "LspProcess: Process started successfully";
    setState(ProcessState::Running);
    m_restartAttempts = 0; // Reset restart attempts on successful start
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
    
    qCritical() << "LspProcess: Process error:" << errorString;
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
    
    if (!m_serverPath.isEmpty()) {
        start(m_serverPath);
    } else {
        qCritical() << "LspProcess: Cannot restart - no server path";
        emit errorOccurred("Cannot restart server - no server path");
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
    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(2000);
        }
    }
}

void LspProcess::scheduleRestart()
{
    if (m_restartTimer && !m_restartTimer->isActive()) {
        int delay = RESTART_DELAY_MS * (m_restartAttempts + 1); // Exponential backoff
        m_restartTimer->setInterval(delay);
        m_restartTimer->start();
        qDebug() << "LspProcess: Restart scheduled in" << delay << "ms";
    }
}
