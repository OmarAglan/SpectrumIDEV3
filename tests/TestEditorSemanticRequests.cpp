#include "QalamEditor.h"

#include <QCoreApplication>
#include <QImage>
#include <QKeyEvent>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class TestEditorSemanticRequests : public QObject
{
    Q_OBJECT

private slots:
    void requestsSignaturesFromArabicEditingTriggers();
    void requestsCompletionInsideIncludePaths();
    void keepsPairsAndSelectionsInLogicalDocumentOrder();
    void expandsAndShrinksCompilerOwnedSelections();
    void keepsLiteralBracesOutOfLocalFolding();
    void rendersInlayHintsWithoutChangingSourceText();
};

void TestEditorSemanticRequests::requestsCompletionInsideIncludePaths()
{
    QalamEditor editor;
    editor.setFilePath(QStringLiteral("رئيسي.باء"));
    editor.setPlainText(QStringLiteral("#تضمين "));
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);
    QSignalSpy requests(&editor, &QalamEditor::completionRequested);

    QTest::keyClick(&editor, Qt::Key_QuoteDbl);
    QVERIFY(not requests.isEmpty());
    const int afterQuote = requests.size();
    editor.insertPlainText(QStringLiteral("واجهات"));
    QTest::keyClick(&editor, Qt::Key_Slash);
    QVERIFY(requests.size() > afterQuote);
    QCOMPARE(requests.constLast().at(1).toInt(), 0);
    QCOMPARE(requests.constLast().at(2).toInt(),
             editor.textCursor().positionInBlock());
}

void TestEditorSemanticRequests::requestsSignaturesFromArabicEditingTriggers()
{
    QTemporaryDir workspace(QStringLiteral("qalam-semantic-مسار-XXXXXX"));
    QVERIFY(workspace.isValid());

    QalamEditor editor;
    editor.setFilePath(workspace.filePath(QStringLiteral("رئيسي.baa")));
    editor.setPlainText(QStringLiteral("اجمع"));
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);

    QSignalSpy requests(&editor, &QalamEditor::signatureHelpRequested);

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
    QalamEditor editor;
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
    QalamEditor editor;
    editor.setFilePath(QStringLiteral("رئيسي.baa"));
    const QString source =
        QStringLiteral("صحيح الرئيسية() {\n    صحيح س = ١.\n}\n");
    editor.setPlainText(source);
    const QString declarationLine = source.split('\n').at(1);
    const int character = declarationLine.indexOf(QStringLiteral("س"));
    QTextCursor cursor(editor.document()->findBlockByNumber(1));
    cursor.setPosition(cursor.block().position() + character);
    editor.setTextCursor(cursor);

    QSignalSpy requests(&editor, &QalamEditor::selectionRangeRequested);
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
    QalamEditor editor;
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

void TestEditorSemanticRequests::rendersInlayHintsWithoutChangingSourceText()
{
    QalamEditor editor;
    editor.resize(640, 240);
    const QString source = QStringLiteral(
        "صحيح اجمع(صحيح أول، صحيح ثان) { إرجع أول + ثان. }\n"
        "صحيح الرئيسية() { إرجع اجمع(١، ٢). }\n");
    editor.setPlainText(source);

    QImage withoutHints(editor.size(), QImage::Format_ARGB32_Premultiplied);
    withoutHints.fill(Qt::transparent);
    editor.render(&withoutHints);

    editor.setInlayHints({
        {1, 27, QStringLiteral("أول:"), QStringLiteral("أول"), true, true},
        {1, 30, QStringLiteral("ثان:"), QStringLiteral("ثان"), true, true}
    });
    QCOMPARE(editor.inlayHintCount(), 2);
    QCOMPARE(editor.property("qalam.inlayHintCount").toInt(), 2);
    QVERIFY(editor.accessibleDescription().contains(QStringLiteral("2")));
    QCOMPARE(editor.toPlainText(), source);

    QImage rendered(editor.size(), QImage::Format_ARGB32_Premultiplied);
    rendered.fill(Qt::transparent);
    editor.render(&rendered);
    QVERIFY(not rendered.isNull());
    QVERIFY(rendered != withoutHints);
    QCOMPARE(editor.toPlainText(), source);

    editor.clearInlayHints();
    QCOMPARE(editor.inlayHintCount(), 0);
    QCOMPARE(editor.property("qalam.inlayHintCount").toInt(), 0);
    QVERIFY(editor.accessibleDescription().isEmpty());
}

QTEST_MAIN(TestEditorSemanticRequests)
#include "TestEditorSemanticRequests.moc"
