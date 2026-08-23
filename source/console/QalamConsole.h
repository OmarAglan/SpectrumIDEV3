#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QProcess>
#include <QFrame>
#include <QTextCharFormat>
#include <QTimer>
#include <QToolButton>
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
    void beginTask(const QString &title);

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
    QWidget *m_toolbar{};
    QFrame *m_inputFrame{};
    QLabel *m_sessionLabel{};
    QLabel *m_stateLabel{};
    QLabel *m_promptLabel{};
    QToolButton *m_clearButton{};
    QToolButton *m_restartButton{};
    QToolButton *m_stopButton{};
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
    bool m_expectedStop{};

    // helpers
    void appendAnsiText(const QString &text);
    void applySgr(const QString &parameters);
    void restartShell();
    void setSessionState(const QString &title,
                         const QString &status,
                         const QString &state);
    bool eventFilter(QObject *obj, QEvent *ev) override;

    int m_maxLines{};
};
