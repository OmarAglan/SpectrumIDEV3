#pragma once

#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>
#include <QMutex>

class ProcessWorker : public QObject
{
    Q_OBJECT
public:
    explicit ProcessWorker(const QString &program,
                           const QStringList &args,
                           const QString &workingDir,
                           const QString &eventFilePath = QString(),
                           const QString &followUpProgram = QString(),
                           const QStringList &followUpArgs = {},
                           const QString &followUpWorkingDir = QString(),
                           const QString &followUpHeading = QString(),
                           const QProcessEnvironment &environment = {});

signals:
    void outputReady(const QString &text);
    void errorReady(const QString &text);
    void eventLineReady(const QByteArray &line);
    void finished(int exitCode);

public slots:
    void start();
    void sendInput(const QString &text);
    void stop();

private slots:
    void onReadyReadOutput();
    void onReadyReadError();
    void flushBuffers();

private:
    QString program{};
    QStringList args{};
    QString workingDir{};
    QString eventFilePath{};
    QString followUpProgram{};
    QStringList followUpArgs{};
    QString followUpWorkingDir{};
    QString followUpHeading{};
    QProcessEnvironment processEnvironment{};
    QProcess *process{};
    QString outputBuffer{};
    QString errorBuffer{};
    QTimer *flushTimer{};
    QMutex bufferMutex{};  // Mutex for thread-safe buffer access
    bool m_finishedEmitted{};
    bool m_cancelRequested{};
    bool m_followUpStarted{};
    qint64 m_eventOffset{};
    QByteArray m_eventBuffer{};

    void emitFinishedOnce(int exitCode);
    void drainEventFile(bool finalRead = false);
    void startProcessPhase(const QString &phaseProgram,
                           const QStringList &phaseArgs,
                           const QString &phaseWorkingDir);
};
