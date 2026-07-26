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

QTEST_MAIN(TestEditorSemanticRequests)
#include "TestEditorSemanticRequests.moc"
