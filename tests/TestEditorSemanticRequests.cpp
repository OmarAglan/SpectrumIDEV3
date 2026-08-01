#include "TEditor.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class TestEditorSemanticRequests : public QObject
{
    Q_OBJECT

private slots:
    void requestsSignaturesFromArabicEditingTriggers();
    void keepsPairsAndSelectionsInLogicalDocumentOrder();
    void expandsAndShrinksCompilerOwnedSelections();
    void keepsLiteralBracesOutOfLocalFolding();
};

void TestEditorSemanticRequests::requestsSignaturesFromArabicEditingTriggers()
{
    QTemporaryDir workspace(QStringLiteral("qalam-semantic-مسار-XXXXXX"));
    QVERIFY(workspace.isValid());

    TEditor editor;
    editor.setFilePath(workspace.filePath(QStringLiteral("رئيسي.baa")));
    editor.setPlainText(QStringLiteral("اجمع"));
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);

    QSignalSpy requests(&editor, &TEditor::signatureHelpRequested);

    QTest::keyClicks(&editor, QStringLiteral("("));
    QCOMPARE(editor.toPlainText(), QStringLiteral("اجمع()"));
    QCOMPARE(requests.size(), 1);
    QCOMPARE(requests.constLast().at(1).toInt(), 0);
    QCOMPARE(requests.constLast().at(2).toInt(), 5);

    QKeyEvent arabicComma(QEvent::KeyPress, Qt::Key_Comma,
                          Qt::NoModifier, QString(QChar(0x060c)));
    QCoreApplication::sendEvent(&editor, &arabicComma);
    QCOMPARE(editor.toPlainText(), QStringLiteral("اجمع(،)"));
    QCOMPARE(requests.size(), 2);
    QCOMPARE(requests.constLast().at(2).toInt(), 6);

    const QString beforeShortcut = editor.toPlainText();
    QTest::keyClick(&editor, Qt::Key_Space,
                    Qt::ControlModifier | Qt::ShiftModifier);
    QCOMPARE(requests.size(), 3);
    QCOMPARE(editor.toPlainText(), beforeShortcut);
}

void TestEditorSemanticRequests::keepsPairsAndSelectionsInLogicalDocumentOrder()
{
    TEditor editor;
    editor.setPlainText(QStringLiteral("مرحبا"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);

    QTest::keyClicks(&editor, QStringLiteral("("));
    QCOMPARE(editor.toPlainText(), QStringLiteral("(مرحبا)"));
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("مرحبا"));

    editor.setPlainText(QStringLiteral("اجمع()"));
    cursor = editor.textCursor();
    cursor.setPosition(5);
    editor.setTextCursor(cursor);
    QTest::keyClicks(&editor, QStringLiteral(")"));
    QCOMPARE(editor.toPlainText(), QStringLiteral("اجمع()"));
    QCOMPARE(editor.textCursor().position(), 6);

    editor.setPlainText(QStringLiteral("باء"));
    cursor = editor.textCursor();
    cursor.setPosition(0);
    cursor.setPosition(3, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    QTest::keyClicks(&editor, QStringLiteral("\""));
    QCOMPARE(editor.toPlainText(), QStringLiteral("\"باء\""));
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("باء"));
}

void TestEditorSemanticRequests::expandsAndShrinksCompilerOwnedSelections()
{
    TEditor editor;
    editor.setFilePath(QStringLiteral("رئيسي.baa"));
    const QString source =
        QStringLiteral("صحيح الرئيسية() {\n    صحيح س = ١.\n}\n");
    editor.setPlainText(source);
    const QString declarationLine = source.split('\n').at(1);
    const int character = declarationLine.indexOf(QStringLiteral("س"));
    QTextCursor cursor(editor.document()->findBlockByNumber(1));
    cursor.setPosition(cursor.block().position() + character);
    editor.setTextCursor(cursor);

    QSignalSpy requests(&editor, &TEditor::selectionRangeRequested);
    QTest::keyClick(&editor, Qt::Key_Right,
                    Qt::ShiftModifier | Qt::AltModifier);
    QCOMPARE(requests.size(), 1);
    QCOMPARE(requests.first().at(1).toInt(), 1);
    QCOMPARE(requests.first().at(2).toInt(), character);

    editor.applySemanticSelectionRanges({
        {1, character, 1, character + 1},
        {1, 4, 1, static_cast<int>(declarationLine.size())},
        {0, 16, 2, 1},
        {0, 0, 3, 0}
    }, 1, character);
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("س"));

    QTest::keyClick(&editor, Qt::Key_Right,
                    Qt::ShiftModifier | Qt::AltModifier);
    QCOMPARE(editor.textCursor().selectedText(), declarationLine.mid(4));
    QCOMPARE(requests.size(), 1);

    QTest::keyClick(&editor, Qt::Key_Left,
                    Qt::ShiftModifier | Qt::AltModifier);
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("س"));
    QTest::keyClick(&editor, Qt::Key_Left,
                    Qt::ShiftModifier | Qt::AltModifier);
    QVERIFY(not editor.textCursor().hasSelection());

    QTest::keyClick(&editor, Qt::Key_Right,
                    Qt::ShiftModifier | Qt::AltModifier);
    QTest::keyClick(&editor, Qt::Key_Right,
                    Qt::ShiftModifier | Qt::AltModifier);
    QTest::keyClick(&editor, Qt::Key_Right,
                    Qt::ShiftModifier | Qt::AltModifier);
    QTest::keyClick(&editor, Qt::Key_Right,
                    Qt::ShiftModifier | Qt::AltModifier);
    QCOMPARE(editor.textCursor().selectionStart(), 0);
    QCOMPARE(editor.textCursor().selectionEnd(), static_cast<int>(source.size()));

    QTest::keyClick(&editor, Qt::Key_Right,
                    Qt::ShiftModifier | Qt::AltModifier);
    QCOMPARE(requests.size(), 1);
}

void TestEditorSemanticRequests::keepsLiteralBracesOutOfLocalFolding()
{
    TEditor editor;
    editor.setFilePath(QStringLiteral("رئيسي.baa"));
    editor.setPlainText(QStringLiteral(
        "صحيح الرئيسية() {\n"
        "    نص قيمة = \"{\nليس نطاقا}\".\n"
        "}\n"));
    editor.useLocalFoldingRanges();
    QCOMPARE(editor.foldingRangeCount(), 1);

    editor.setFoldingRanges({
        {0, 16, 3, 1, QStringLiteral("region")}
    });
    QCOMPARE(editor.foldingRangeCount(), 1);
}

QTEST_MAIN(TestEditorSemanticRequests)
#include "TestEditorSemanticRequests.moc"
