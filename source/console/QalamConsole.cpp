#include "QalamConsole.h"
#include "Constants.h"
#include "ui/QalamTheme.h"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QKeyEvent>
#include <QTextBlockFormat>
#include <QApplication>
#include <QMutexLocker>
#include <QFontDatabase>
#include <QTextBlock>
#include <QTextOption>
#include <QProcessEnvironment>
#include <utility>

namespace {
QColor ansiBaseColor(int index)
{
    static const QColor colors[] = {
        QColor(0, 0, 0),
        QColor(205, 49, 49),
        QColor(13, 188, 121),
        QColor(229, 229, 16),
        QColor(36, 114, 200),
        QColor(188, 63, 188),
        QColor(17, 168, 205),
        QColor(229, 229, 229),
        QColor(102, 102, 102),
        QColor(241, 76, 76),
        QColor(35, 209, 139),
        QColor(245, 245, 67),
        QColor(59, 142, 234),
        QColor(214, 112, 214),
        QColor(41, 184, 219),
        QColor(255, 255, 255),
    };
    return colors[qBound(0, index, 15)];
}

QColor ansi256Color(int index)
{
    index = qBound(0, index, 255);
    if (index < 16) return ansiBaseColor(index);

    if (index < 232) {
        static const int levels[] = {0, 95, 135, 175, 215, 255};
        const int cubeIndex = index - 16;
        return QColor(levels[cubeIndex / 36],
                      levels[(cubeIndex / 6) % 6],
                      levels[cubeIndex % 6]);
    }

    const int gray = 8 + ((index - 232) * 10);
    return QColor(gray, gray, gray);
}

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
}

QalamConsole::QalamConsole(QWidget *parent)
    : QWidget(parent),
    m_output(new QPlainTextEdit(this)),
    m_input(new QLineEdit(this)),
    m_process(new QProcess(this)),
    m_flushTimer(new QTimer(this)),
    m_historyIndex(-1),
    m_autoscroll(true)
{
    // UI
    m_output->setObjectName(QStringLiteral("consoleOutput"));
    m_input->setObjectName(QStringLiteral("consoleInput"));
    m_output->setReadOnly(true);
    m_output->setUndoRedoEnabled(false);
    m_output->setWordWrapMode(QTextOption::WordWrap);
    // simple monospace font
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPixelSize(Constants::Fonts::ConsoleSize);
    m_output->setFont(f);
    m_input->setFont(f);

    setStyleSheet(QalamTheme::consoleStyleSheet());

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);
    lay->addWidget(m_output);
    lay->addWidget(m_input);
    setLayout(lay);

    setLayoutDirection(Qt::RightToLeft);
    m_input->setLayoutDirection(Qt::RightToLeft);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &QalamConsole::processStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &QalamConsole::processStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &QalamConsole::processFinished);

    connect(m_input, &QLineEdit::returnPressed, this, &QalamConsole::onInputReturn);

    m_input->installEventFilter(this);

    m_flushTimer->setInterval(Constants::Timing::FlushInterval);
    connect(m_flushTimer, &QTimer::timeout, this, &QalamConsole::flushPending);
    m_flushTimer->start();
}

QalamConsole::~QalamConsole()
{
    stopCmd();
}

void QalamConsole::startCmd()
{
    if (m_process->state() != QProcess::NotRunning) return;

#if defined(Q_OS_WIN)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("TERM", "dumb");
    m_process->setProcessEnvironment(env);
    m_process->start("cmd.exe", {"/Q", "/K", "chcp 65001 > nul"});
#elif defined(Q_OS_MACOS)
    QStringList args;
    args << "-i" << "-l"; // Interactive login shell
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PROMPT_EOL_MARK", ""); // Remove '%' mark from zsh
    m_process->setProcessEnvironment(env);
    m_process->start("zsh", args);
#elif defined(Q_OS_LINUX)
    QStringList args;
    args << "-i"; // Interactive mode
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("TERM", "dumb"); // Disable advanced terminal features
    env.insert("PS1", "\\u@\\h:\\w$ "); // Simple prompt
    m_process->setProcessEnvironment(env);
    m_process->start("/bin/bash", args);
#endif
}

void QalamConsole::stopCmd()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(500)) {
            m_process->kill();
            m_process->waitForFinished(200);
        }
    }
}

void QalamConsole::clear()
{
    m_output->clear();
    m_ansiRemainder.clear();
    m_ansiFormat = QTextCharFormat{};
    QMutexLocker locker(&m_pendingMutex);
    m_pending.clear();
}

void QalamConsole::setConsoleRTL()
{
    setLayoutDirection(Qt::RightToLeft);

    QTextOption opt = m_output->document()->defaultTextOption();
    // Let each paragraph choose its own bidi direction. Arabic diagnostics
    // stay RTL, while Windows paths and shell prompts remain readable LTR.
    opt.setTextDirection(Qt::LayoutDirectionAuto);
    opt.setAlignment(Qt::AlignRight);
    m_output->document()->setDefaultTextOption(opt);
}

void QalamConsole::appendPlainTextThreadSafe(const QString &text)
{
    if (text.isEmpty()) return;

    QMutexLocker locker(&m_pendingMutex);
    m_pending.append(text);
}

void QalamConsole::processStdout()
{
    QByteArray d = m_process->readAllStandardOutput();
    QString s = decodeProcessBytes(d);
    appendPlainTextThreadSafe(s);

}

void QalamConsole::processStderr()
{
    QByteArray d = m_process->readAllStandardError();
    QString s = decodeProcessBytes(d);
    appendPlainTextThreadSafe(s);
}

void QalamConsole::processFinished(int code, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    appendPlainTextThreadSafe(QString("\n[Process finished with code %1]\n").arg(code));
}

void QalamConsole::onInputReturn()
{
    QString cmd = m_input->text();
    if (cmd.isEmpty()) {
        if (m_process->state() != QProcess::NotRunning) {
#if defined(Q_OS_WIN)
            m_process->write("\r\n");
#else
            m_process->write("\n");
#endif
        }
        return;
    }

    if (m_history.isEmpty() || m_history.last() != cmd) {
        m_history.append(cmd);
    }
    m_historyIndex = -1;

// echo the command locally (like terminal)
// appendPlainTextThreadSafe(cmd + "\n");
#if defined(Q_OS_WIN)
    appendPlainTextThreadSafe(cmd + "\n");
#endif

    // send to process (CRLF on Windows)
#if defined(Q_OS_WIN)
    if (m_process->state() != QProcess::NotRunning) {
        m_process->write((cmd + "\r\n").toUtf8());
    }
#else
    if (m_process->state() != QProcess::NotRunning) {
        m_process->write((cmd + "\n").toUtf8());
    }
#endif

    emit commandEntered(cmd);
    m_input->clear();
}

void QalamConsole::flushPending()
{
    QString chunk;
    {
        QMutexLocker locker(&m_pendingMutex);
        if (m_pending.isEmpty()) return;
        chunk = std::move(m_pending);
        m_pending.clear();
    }

    appendAnsiText(chunk);

    const int excessBlocks = m_output->document()->blockCount() - m_maxLines;
    if (excessBlocks > 0) {
        QTextCursor trimCursor(m_output->document());
        trimCursor.movePosition(QTextCursor::Start);
        for (int i = 0; i < excessBlocks; ++i) {
            trimCursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        }
        trimCursor.removeSelectedText();
        trimCursor.deleteChar();
    }

    if (m_autoscroll) {
        QScrollBar *sb = m_output->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
}

void QalamConsole::appendAnsiText(const QString &text)
{
    const QString input = std::exchange(m_ansiRemainder, QString{}) + text;
    QTextCursor cursor(m_output->document());
    cursor.movePosition(QTextCursor::End);

    int plainStart = 0;
    int index = 0;
    while (index < input.size()) {
        if (input.at(index) != QChar(0x1b)) {
            ++index;
            continue;
        }

        if (index > plainStart) {
            cursor.insertText(input.mid(plainStart, index - plainStart), m_ansiFormat);
        }

        if (index + 1 >= input.size()) {
            m_ansiRemainder = input.mid(index);
            return;
        }

        if (input.at(index + 1) != QLatin1Char('[')) {
            ++index;
            plainStart = index;
            continue;
        }

        int sequenceEnd = index + 2;
        while (sequenceEnd < input.size()) {
            const ushort value = input.at(sequenceEnd).unicode();
            if (value >= 0x40 and value <= 0x7e) break;
            ++sequenceEnd;
        }

        if (sequenceEnd >= input.size()) {
            m_ansiRemainder = input.mid(index);
            return;
        }

        if (input.at(sequenceEnd) == QLatin1Char('m')) {
            applySgr(input.mid(index + 2, sequenceEnd - index - 2));
        }
        index = sequenceEnd + 1;
        plainStart = index;
    }

    if (plainStart < input.size()) {
        cursor.insertText(input.mid(plainStart), m_ansiFormat);
    }
}

void QalamConsole::applySgr(const QString &parameters)
{
    const QStringList parts = parameters.isEmpty()
        ? QStringList{QStringLiteral("0")}
        : parameters.split(QLatin1Char(';'), Qt::KeepEmptyParts);

    for (int index = 0; index < parts.size(); ++index) {
        bool ok = false;
        const int code = parts.at(index).isEmpty() ? 0 : parts.at(index).toInt(&ok);
        if (not ok and not parts.at(index).isEmpty()) continue;

        if (code == 0) {
            m_ansiFormat = QTextCharFormat{};
        } else if (code == 1) {
            m_ansiFormat.setFontWeight(QFont::Bold);
        } else if (code == 3) {
            m_ansiFormat.setFontItalic(true);
        } else if (code == 4) {
            m_ansiFormat.setFontUnderline(true);
        } else if (code == 22) {
            m_ansiFormat.setFontWeight(QFont::Normal);
        } else if (code == 23) {
            m_ansiFormat.setFontItalic(false);
        } else if (code == 24) {
            m_ansiFormat.setFontUnderline(false);
        } else if (code >= 30 and code <= 37) {
            m_ansiFormat.setForeground(ansiBaseColor(code - 30));
        } else if (code == 39) {
            m_ansiFormat.clearForeground();
        } else if (code >= 40 and code <= 47) {
            m_ansiFormat.setBackground(ansiBaseColor(code - 40));
        } else if (code == 49) {
            m_ansiFormat.clearBackground();
        } else if (code >= 90 and code <= 97) {
            m_ansiFormat.setForeground(ansiBaseColor(code - 90 + 8));
        } else if (code >= 100 and code <= 107) {
            m_ansiFormat.setBackground(ansiBaseColor(code - 100 + 8));
        } else if ((code == 38 or code == 48) and index + 1 < parts.size()) {
            const bool foreground = code == 38;
            bool modeOk = false;
            const int mode = parts.at(++index).toInt(&modeOk);
            QColor color;

            if (modeOk and mode == 5 and index + 1 < parts.size()) {
                bool colorOk = false;
                const int colorIndex = parts.at(++index).toInt(&colorOk);
                if (colorOk and colorIndex >= 0 and colorIndex <= 255) {
                    color = ansi256Color(colorIndex);
                }
            } else if (modeOk and mode == 2 and index + 3 < parts.size()) {
                bool redOk = false;
                bool greenOk = false;
                bool blueOk = false;
                const int red = parts.at(++index).toInt(&redOk);
                const int green = parts.at(++index).toInt(&greenOk);
                const int blue = parts.at(++index).toInt(&blueOk);
                if (redOk and greenOk and blueOk
                    and red >= 0 and red <= 255
                    and green >= 0 and green <= 255
                    and blue >= 0 and blue <= 255) {
                    color = QColor(red, green, blue);
                }
            }

            if (color.isValid()) {
                if (foreground) m_ansiFormat.setForeground(color);
                else m_ansiFormat.setBackground(color);
            }
        }
    }
}

bool QalamConsole::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_input && ev->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(ev);
        if (ke->key() == Qt::Key_Up) {
            if (m_history.isEmpty()) return true;
            if (m_historyIndex == -1) m_historyIndex = m_history.size() - 1;
            else m_historyIndex = qMax(0, m_historyIndex - 1);
            m_input->setText(m_history[m_historyIndex]);
            return true;
        } else if (ke->key() == Qt::Key_Down) {
            if (m_history.isEmpty()) return true;
            if (m_historyIndex == -1) return true;
            if (m_historyIndex >= m_history.size() - 1) {
                m_historyIndex = -1;
                m_input->clear();
            } else {
                ++m_historyIndex;
                m_input->setText(m_history[m_historyIndex]);
            }
            return true;
        } else if (ke->matches(QKeySequence::Copy)) {
            return QWidget::eventFilter(obj, ev);
        } else if (ke->key() == Qt::Key_C && (ke->modifiers() & Qt::ControlModifier)) {
            return false;
        } else if (ke->key() == Qt::Key_L && (ke->modifiers() & Qt::ControlModifier)) {
            clear();
            return true;
        } else if (ke->key() == Qt::Key_Tab) {
            return true;
        }
    }
    return QWidget::eventFilter(obj, ev);
}
