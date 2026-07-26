#include "TSnippetManager.h"

#include <QPlainTextEdit>
#include <QTest>

class TestSnippetManager : public QObject
{
    Q_OBJECT
private slots:
    void expandsStandardPlaceholdersAndPreservesIndentation();
};

void TestSnippetManager::expandsStandardPlaceholdersAndPreservesIndentation()
{
    QPlainTextEdit editor;
    editor.setPlainText(QStringLiteral("    ر"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(4);
    cursor.setPosition(5, QTextCursor::KeepAnchor);

    TSnippetManager manager(&editor);
    manager.insertSnippet(
        QStringLiteral("صحيح ${1:اسم}() {\n\t${0}\n}"), cursor);

    QCOMPARE(editor.toPlainText(),
             QStringLiteral("    صحيح اسم() {\n    \t\n    }"));
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("اسم"));
    QVERIFY(manager.hasActiveSnippet());
    QVERIFY(manager.processSnippetNavigation());
    QVERIFY(editor.textCursor().selectedText().isEmpty());
    QVERIFY(not manager.hasActiveSnippet());
}

QTEST_MAIN(TestSnippetManager)
#include "TestSnippetManager.moc"
