#include "QalamBracketHandler.h"

#include <QPlainTextEdit>
#include <QTest>
#include <QTextCursor>

namespace {
void placeCursor(QPlainTextEdit *editor, int position)
{
    QTextCursor cursor = editor->textCursor();
    cursor.setPosition(position);
    editor->setTextCursor(cursor);
}

void typeQuote(QalamBracketHandler *handler, QChar quote = QLatin1Char('"'))
{
    QKeyEvent event(QEvent::KeyPress, Qt::Key_QuoteDbl, Qt::NoModifier, QString(quote));
    QVERIFY(handler->handleAutoPairing(&event));
}
}

class TestBracketHandler : public QObject
{
    Q_OBJECT

private slots:
    void pairsAQuoteInCode();
    void closesAnAlreadyOpenStringOnlyOnce();
    void skipsAnExistingAutoClosingQuote();
    void respectsEscapesAndIgnoresCommentQuotes();
};

void TestBracketHandler::pairsAQuoteInCode()
{
    QPlainTextEdit editor;
    QalamBracketHandler handler(&editor);

    typeQuote(&handler);

    QCOMPARE(editor.toPlainText(), QStringLiteral("\"\""));
    QCOMPARE(editor.textCursor().position(), 1);
}

void TestBracketHandler::closesAnAlreadyOpenStringOnlyOnce()
{
    QPlainTextEdit editor;
    editor.setPlainText(QStringLiteral("اطبع \"مرحبا"));
    placeCursor(&editor, editor.toPlainText().size());
    QalamBracketHandler handler(&editor);

    typeQuote(&handler);

    QCOMPARE(editor.toPlainText(), QStringLiteral("اطبع \"مرحبا\""));
    QCOMPARE(editor.textCursor().position(), editor.toPlainText().size());
}

void TestBracketHandler::skipsAnExistingAutoClosingQuote()
{
    QPlainTextEdit editor;
    editor.setPlainText(QStringLiteral("اطبع \"مرحبا\""));
    placeCursor(&editor, editor.toPlainText().size() - 1);
    QalamBracketHandler handler(&editor);

    typeQuote(&handler);

    QCOMPARE(editor.toPlainText(), QStringLiteral("اطبع \"مرحبا\""));
    QCOMPARE(editor.textCursor().position(), editor.toPlainText().size());
}

void TestBracketHandler::respectsEscapesAndIgnoresCommentQuotes()
{
    QPlainTextEdit editor;
    editor.setPlainText(QStringLiteral("// \" داخل تعليق\nاطبع \"قال: \\\"مرحبا"));
    placeCursor(&editor, editor.toPlainText().size());
    QalamBracketHandler handler(&editor);

    typeQuote(&handler);

    QCOMPARE(editor.toPlainText(),
             QStringLiteral("// \" داخل تعليق\nاطبع \"قال: \\\"مرحبا\""));
}

QTEST_MAIN(TestBracketHandler)
#include "TestBracketHandler.moc"
