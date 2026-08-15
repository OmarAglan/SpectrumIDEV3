#include "QalamConsole.h"

#include <QPlainTextEdit>
#include <QTextCursor>
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
    for (int index = 0; index < 2500; ++index) {
        largeOutput += QStringLiteral("\x1b[38;5;33mسطر %1\x1b[0m\n").arg(index);
    }
    console.appendPlainTextThreadSafe(largeOutput);
    flushConsole(console);

    QVERIFY(output->document()->blockCount() <= 2000);
    QVERIFY(not output->toPlainText().contains(QStringLiteral("سطر 0\n")));
    QVERIFY(output->toPlainText().contains(QStringLiteral("سطر 2499")));
    QVERIFY(not output->toPlainText().contains(QChar(0x1b)));
}

QTEST_MAIN(TestQalamConsole)
#include "TestQalamConsole.moc"
