#include "QalamConsole.h"

#include <QPlainTextEdit>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QTextCursor>
#include <QToolButton>
#include <QtTest>

namespace {
QPlainTextEdit *consoleOutput(QalamConsole &console)
{
    return console.findChild<QPlainTextEdit *>(QStringLiteral("consoleOutput"));
}

void flushConsole(QalamConsole &console)
{
    QVERIFY(QMetaObject::invokeMethod(&console, "flushPending", Qt::DirectConnection));
}

QTextCharFormat formatAt(QPlainTextEdit *output, int position)
{
    QTextCursor cursor(output->document());
    cursor.setPosition(position);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    return cursor.charFormat();
}
}

class TestQalamConsole : public QObject
{
    Q_OBJECT

private slots:
    void preservesArabicAcrossSplitAnsiSequences();
    void rendersStandard256AndTrueColorSgr();
    void resetsFormattingAndClearsPendingSequences();
    void boundsLargeOutputWithoutLosingTheTail();
    void presentsAnIntegratedInteractiveTerminal();
};

void TestQalamConsole::preservesArabicAcrossSplitAnsiSequences()
{
    QalamConsole console;
    QPlainTextEdit *output = consoleOutput(console);
    QVERIFY(output);

    console.appendPlainTextThreadSafe(QStringLiteral("قبل \x1b[38;2;10"));
    flushConsole(console);
    QCOMPARE(output->toPlainText(), QStringLiteral("قبل "));

    console.appendPlainTextThreadSafe(QStringLiteral(";20;30mنص\x1b[0m بعد"));
    flushConsole(console);
    QCOMPARE(output->toPlainText(), QStringLiteral("قبل نص بعد"));
    QVERIFY(not output->toPlainText().contains(QChar(0x1b)));

    const int coloredPosition = output->toPlainText().indexOf(QStringLiteral("نص"));
    QCOMPARE(formatAt(output, coloredPosition).foreground().color(), QColor(10, 20, 30));
}

void TestQalamConsole::rendersStandard256AndTrueColorSgr()
{
    QalamConsole console;
    QPlainTextEdit *output = consoleOutput(console);
    QVERIFY(output);

    console.appendPlainTextThreadSafe(
        QStringLiteral("\x1b[1;91mأ\x1b[22;38;5;196mب"
                       "\x1b[48;2;12;34;56mج\x1b[0mد"));
    flushConsole(console);

    QCOMPARE(output->toPlainText(), QStringLiteral("أبجد"));
    const QTextCharFormat bright = formatAt(output, 0);
    QCOMPARE(bright.fontWeight(), static_cast<int>(QFont::Bold));
    QCOMPARE(bright.foreground().color(), QColor(241, 76, 76));

    const QTextCharFormat indexed = formatAt(output, 1);
    QCOMPARE(indexed.fontWeight(), static_cast<int>(QFont::Normal));
    QCOMPARE(indexed.foreground().color(), QColor(255, 0, 0));

    const QTextCharFormat trueColor = formatAt(output, 2);
    QCOMPARE(trueColor.foreground().color(), QColor(255, 0, 0));
    QCOMPARE(trueColor.background().color(), QColor(12, 34, 56));

    const QTextCharFormat reset = formatAt(output, 3);
    QVERIFY(not reset.hasProperty(QTextFormat::ForegroundBrush));
    QVERIFY(not reset.hasProperty(QTextFormat::BackgroundBrush));
}

void TestQalamConsole::resetsFormattingAndClearsPendingSequences()
{
    QalamConsole console;
    QPlainTextEdit *output = consoleOutput(console);
    QVERIFY(output);

    console.appendPlainTextThreadSafe(QStringLiteral("\x1b[32mقديم\x1b["));
    flushConsole(console);
    QCOMPARE(output->toPlainText(), QStringLiteral("قديم"));

    console.clear();
    console.appendPlainTextThreadSafe(QStringLiteral("جديد"));
    flushConsole(console);
    QCOMPARE(output->toPlainText(), QStringLiteral("جديد"));
    QVERIFY(not formatAt(output, 0).hasProperty(QTextFormat::ForegroundBrush));
}

void TestQalamConsole::boundsLargeOutputWithoutLosingTheTail()
{
    QalamConsole console;
    QPlainTextEdit *output = consoleOutput(console);
    QVERIFY(output);

    QString largeOutput;
    for (int index = 0; index < 10500; ++index) {
        largeOutput += QStringLiteral("\x1b[38;5;33mسطر %1\x1b[0m\n").arg(index);
    }
    console.appendPlainTextThreadSafe(largeOutput);
    flushConsole(console);

    QVERIFY(output->document()->blockCount() <= 10000);
    QVERIFY(not output->toPlainText().contains(QStringLiteral("سطر 0\n")));
    QVERIFY(output->toPlainText().contains(QStringLiteral("سطر 10499")));
    QVERIFY(not output->toPlainText().contains(QChar(0x1b)));
}

void TestQalamConsole::presentsAnIntegratedInteractiveTerminal()
{
    QalamConsole console;
    auto *toolbar = console.findChild<QWidget *>(QStringLiteral("consoleToolbar"));
    auto *inputFrame = console.findChild<QFrame *>(QStringLiteral("consoleInputFrame"));
    auto *input = console.findChild<QLineEdit *>(QStringLiteral("consoleInput"));
    auto *session = console.findChild<QLabel *>(QStringLiteral("consoleSessionLabel"));
    auto *state = console.findChild<QLabel *>(QStringLiteral("consoleStateLabel"));
    auto *prompt = console.findChild<QLabel *>(QStringLiteral("consolePrompt"));
    auto *clearButton =
        console.findChild<QToolButton *>(QStringLiteral("consoleClearButton"));
    auto *restartButton =
        console.findChild<QToolButton *>(QStringLiteral("consoleRestartButton"));
    auto *stopButton =
        console.findChild<QToolButton *>(QStringLiteral("consoleStopButton"));

    QVERIFY(toolbar);
    QVERIFY(inputFrame);
    QVERIFY(input);
    QVERIFY(session);
    QVERIFY(state);
    QVERIFY(prompt);
    QVERIFY(clearButton);
    QVERIFY(restartButton);
    QVERIFY(stopButton);
    QCOMPARE(console.focusProxy(), input);
    QVERIFY(not input->placeholderText().isEmpty());
    QCOMPARE(input->alignment(), Qt::Alignment(Qt::AlignRight));
    QVERIFY(not input->placeholderText().contains(QStringLiteral("Enter")));
    QVERIFY(not clearButton->icon().isNull());
    QVERIFY(not restartButton->icon().isNull());
    QVERIFY(not stopButton->icon().isNull());

    console.resize(760, 320);
    console.show();
    QTest::qWait(20);
    // The RTL input caret belongs beside the prompt on the physical right.
    QVERIFY(prompt->geometry().left() > input->geometry().left());
    QVERIFY(session->geometry().left() > restartButton->geometry().left());

    console.beginTask(QStringLiteral("تشغيل ملف باء"));
    QCOMPARE(session->text(), QStringLiteral("تشغيل ملف باء"));
    QCOMPARE(state->property("state").toString(), QStringLiteral("busy"));
    QVERIFY(input->placeholderText().contains(QStringLiteral("بيانات البرنامج")));

    console.appendPlainTextThreadSafe(QStringLiteral("ناتج\n"));
    flushConsole(console);
    QVERIFY(not consoleOutput(console)->toPlainText().isEmpty());
    clearButton->click();
    QVERIFY(consoleOutput(console)->toPlainText().isEmpty());
}

QTEST_MAIN(TestQalamConsole)
#include "TestQalamConsole.moc"
