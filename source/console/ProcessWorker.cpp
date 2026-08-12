#include "ProcessWorker.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QProcessEnvironment>

namespace {
QString decodeProcessBytes(const QByteArray &data)
{
#if defined(Q_OS_WIN)
    const QString utf8 = QString::fromUtf8(data);
    if (!utf8.contains(QChar::ReplacementCharacter)) {
        return utf8;
    }
    return QString::fromLocal8Bit(data);
#else
    return QString::fromUtf8(data);
#endif
}

bool bundledBaaEnvironment(const QString &program,
                           QProcessEnvironment *environment)
{
    const QFileInfo programInfo(program);
    if (programInfo.completeBaseName().compare(QStringLiteral("baa"), Qt::CaseInsensitive) != 0) {
        return false;
    }

    *environment = QProcessEnvironment::systemEnvironment();
    const QString compilerHome = programInfo.absolutePath();
    if (not environment->contains(QStringLiteral("BAA_HOME")) and
        QFileInfo(QDir(compilerHome).filePath(QStringLiteral("stdlib"))).isDir()) {
        environment->insert(QStringLiteral("BAA_HOME"), compilerHome);
    }
    if (not environment->contains(QStringLiteral("BAA_NAZM"))) {
#if defined(Q_OS_WIN)
        const QString nazm = QDir(compilerHome).filePath(QStringLiteral("نظم.exe"));
#else
        const QString nazm = QDir(compilerHome).filePath(QStringLiteral("نظم"));
#endif
        if (QFileInfo(nazm).isExecutable()) {
            environment->insert(QStringLiteral("BAA_NAZM"), nazm);
        }
    }
    return true;
}
}

ProcessWorker::ProcessWorker(const QString &program,
                             const QStringList &args,
                             const QString &workingDir,
                             const QString &eventFilePath)
    : program(program),
      args(args),
      workingDir(workingDir),
      eventFilePath(eventFilePath),
      process(nullptr),
      flushTimer(nullptr)
{
    // Do NOT create timer here - it must be created in the worker thread
    // Timer will be created in start() which runs in the correct thread
}

void ProcessWorker::start() {
    // Create timer in the worker thread (correct thread affinity)
    if (!flushTimer) {
        flushTimer = new QTimer(this);
        flushTimer->setInterval(20);
        connect(flushTimer, &QTimer::timeout, this, &ProcessWorker::flushBuffers);
    }

    // Clean up any existing process
    if (process) {
        disconnect(process, nullptr, this, nullptr);
        process->deleteLater();
        process = nullptr;
    }
    process = new QProcess(this);
    m_finishedEmitted = false;
    m_cancelRequested = false;
    m_eventOffset = 0;
    m_eventBuffer.clear();

    process->setProgram(program);
    process->setArguments(args);
    process->setWorkingDirectory(workingDir);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    QProcessEnvironment environment;
    if (bundledBaaEnvironment(program, &environment)) {
        process->setProcessEnvironment(environment);
    }

    // Connect read signals
    connect(process, &QProcess::readyReadStandardOutput,
            this, &ProcessWorker::onReadyReadOutput);
    connect(process, &QProcess::readyReadStandardError,
            this, &ProcessWorker::onReadyReadError);

    // Start timer when process starts
    connect(process, &QProcess::started, this, [this]() {
        if (flushTimer && !flushTimer->isActive())
            flushTimer->start();
    });

    // Stop timer and emit finished when process ends
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus status) {
                if (flushTimer && flushTimer->isActive())
                    flushTimer->stop();

                // Flush any remaining buffered data
                flushBuffers();
                drainEventFile(true);

                emitFinishedOnce(m_cancelRequested
                    ? -2
                    : (status == QProcess::NormalExit ? code : -1));
            });

    connect(process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            emit errorReady("❌ فشل بدء العملية: " + process->errorString());
            if (flushTimer && flushTimer->isActive()) {
                flushTimer->stop();
            }
            emitFinishedOnce(-1);
        }
    });

    // Start the process (non-blocking)
    process->start();
}
void ProcessWorker::onReadyReadOutput() {
    QMutexLocker locker(&bufferMutex);
    QString s = decodeProcessBytes(process->readAllStandardOutput());
    outputBuffer.append(s);
}

void ProcessWorker::onReadyReadError() {
    QMutexLocker locker(&bufferMutex);
    QString s = decodeProcessBytes(process->readAllStandardError());
    errorBuffer.append(s);
}

void ProcessWorker::flushBuffers() {
    QString outCopy, errCopy;
    {
        QMutexLocker locker(&bufferMutex);
        outCopy = outputBuffer;
        errCopy = errorBuffer;
        outputBuffer.clear();
        errorBuffer.clear();
    }
    if (!outCopy.isEmpty()) {
        emit outputReady(outCopy);
    }
    if (!errCopy.isEmpty()) {
        emit errorReady(errCopy);
    }
    drainEventFile();
}

void ProcessWorker::drainEventFile(bool finalRead)
{
    if (eventFilePath.isEmpty()) return;

    QFile file(eventFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        if (file.size() < m_eventOffset) {
            m_eventOffset = 0;
            m_eventBuffer.clear();
        }
        if (file.seek(m_eventOffset)) {
            const QByteArray bytes = file.readAll();
            m_eventOffset += bytes.size();
            m_eventBuffer += bytes;
        }
    }

    qsizetype newline = -1;
    while ((newline = m_eventBuffer.indexOf('\n')) >= 0) {
        QByteArray line = m_eventBuffer.left(newline);
        m_eventBuffer.remove(0, newline + 1);
        if (line.endsWith('\r')) line.chop(1);
        if (not line.trimmed().isEmpty()) emit eventLineReady(line);
    }
    if (finalRead and not m_eventBuffer.trimmed().isEmpty()) {
        emit eventLineReady(m_eventBuffer);
        m_eventBuffer.clear();
    }
}

void ProcessWorker::emitFinishedOnce(int exitCode)
{
    if (m_finishedEmitted) {
        return;
    }
    m_finishedEmitted = true;
    emit finished(exitCode);
}

void ProcessWorker::stop() {
    if (process && process->state() != QProcess::NotRunning) {
        m_cancelRequested = true;
        process->terminate();
        if (!process->waitForFinished(3000)) {
            process->kill();
            process->waitForFinished(500);
        }
        flushBuffers();
        drainEventFile(true);
    }
}

void ProcessWorker::sendInput(const QString &text) {
    if (process && process->state() == QProcess::Running) {
#if defined(Q_OS_WIN)
        process->write((text + "\r\n").toUtf8());
#else
        process->write((text + "\n").toUtf8());
#endif
    }
}
