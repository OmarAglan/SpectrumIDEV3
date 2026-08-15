#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QProcess>
#include <QTextCharFormat>
#include <QTimer>
#include <QMutex>
#include <QVector>

class QalamConsole : public QWidget {
    Q_OBJECT
public:
    explicit QalamConsole(QWidget *parent = nullptr);
    ~QalamConsole() override;

    // actions
    void startCmd();                    // start cmd.exe (Windows)
    void stopCmd();                     // stop process
    void clear();                       // clear output
    void setConsoleRTL();               // force RTL on widgets
    void appendPlainTextThreadSafe(const QString &text); // thread-safe append

signals:
    void commandEntered(const QString &cmd); // emitted when user enters command

private slots:
    void processStdout();
    void processStderr();
    void processFinished(int code, QProcess::ExitStatus status);
    void onInputReturn();
    void flushPending();

private:
    QPlainTextEdit *m_output{};
    QLineEdit *m_input{};
    QProcess *m_process{};
    QTimer *m_flushTimer{};

    QMutex m_pendingMutex{};
    QString m_pending{}; // staging text waiting for GUI flush
    QString m_ansiRemainder{};
    QTextCharFormat m_ansiFormat{};

    // history
    QVector<QString> m_history{};
    int m_historyIndex{}; // -1 means not browsing

    // autoscroll
    bool m_autoscroll{};

    // helpers
    void appendAnsiText(const QString &text);
    void applySgr(const QString &parameters);
    bool eventFilter(QObject *obj, QEvent *ev) override;

    int m_maxLines = 2000;         // آخر 2000 سطر
};
