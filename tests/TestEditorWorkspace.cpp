#include "FileManager.h"
#include "LayoutManager.h"
#include "QalamDocumentModel.h"
#include "QalamEditor.h"
#include "QalamEditorWorkspace.h"
#include "QalamPanelArea.h"
#include "QalamSearchPanel.h"

#include <QFile>
#include <QMainWindow>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>

class TestEditorWorkspace final : public QObject
{
    Q_OBJECT

private slots:
    void sharesContentPathAndUndoAcrossTwoGroups();
    void movesTabsAndNeverCreatesMoreThanTwoGroups();
    void keepsOneSharedBottomPanelAcrossEditorGroups();
};

void TestEditorWorkspace::sharesContentPathAndUndoAcrossTwoGroups()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString filePath = temporaryDirectory.filePath(
        QStringLiteral("مشترك.باء"));

    QalamEditorWorkspace workspace;
    FileManager fileManager(&workspace, &workspace);
    auto *first = new QalamEditor(&workspace);
    workspace.addTab(first, QStringLiteral("مشترك.باء"));
    first->setFilePath(filePath);

    QVERIFY(workspace.splitCurrent(Qt::Horizontal));
    QCOMPARE(workspace.groupCount(), 2);
    QCOMPARE(workspace.splitOrientation(), Qt::Horizontal);
    QCOMPARE(workspace.editors().size(), 2);

    QalamEditor *second = workspace.currentEditor();
    QVERIFY(second);
    QVERIFY(second != first);
    QCOMPARE(second->documentModel(), first->documentModel());
    QCOMPARE(second->document(), first->document());
    QCOMPARE(second->currentFilePath(), first->currentFilePath());

    QTextCursor cursor(first->document());
    cursor.insertText(QStringLiteral("نص مشترك"));
    QCOMPARE(second->toPlainText(), QStringLiteral("نص مشترك"));
    QVERIFY(first->document()->isModified());
    QVERIFY(second->document()->isModified());
    second->undo();
    QCOMPARE(first->toPlainText(), QString());
    first->redo();
    QCOMPARE(second->toPlainText(), QStringLiteral("نص مشترك"));

    workspace.setCurrentWidget(second);
    fileManager.saveFile();
    QVERIFY(not first->document()->isModified());
    QVERIFY(not second->document()->isModified());
    QFile saved(filePath);
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(saved.readAll()), QStringLiteral("نص مشترك"));

    const QString renamedPath = temporaryDirectory.filePath(
        QStringLiteral("اسم جديد.باء"));
    second->setFilePath(renamedPath);
    QCOMPARE(first->currentFilePath(), renamedPath);
}

void TestEditorWorkspace::movesTabsAndNeverCreatesMoreThanTwoGroups()
{
    QalamEditorWorkspace workspace;
    auto *first = new QalamEditor(&workspace);
    auto *second = new QalamEditor(&workspace);
    workspace.addTab(first, QStringLiteral("الأول.باء"));
    workspace.addTab(second, QStringLiteral("الثاني.باء"));
    workspace.setCurrentWidget(first);

    QVERIFY(workspace.splitCurrent(Qt::Vertical));
    QCOMPARE(workspace.groupCount(), 2);
    QCOMPARE(workspace.splitOrientation(), Qt::Vertical);
    QVERIFY(workspace.splitCurrent(Qt::Horizontal));
    QCOMPARE(workspace.groupCount(), 2);
    QCOMPARE(workspace.splitOrientation(), Qt::Horizontal);

    workspace.setCurrentWidget(second);
    QVERIFY(workspace.moveCurrentToOtherGroup(Qt::Vertical));
    QCOMPARE(workspace.groupIndexFor(second), 1);
    QCOMPARE(workspace.splitOrientation(), Qt::Vertical);

    workspace.closeSecondaryGroup();
    QCOMPARE(workspace.groupCount(), 1);
    QCOMPARE(workspace.count(), 3);
}

void TestEditorWorkspace::keepsOneSharedBottomPanelAcrossEditorGroups()
{
    QMainWindow window;
    QalamEditorWorkspace workspace;
    QalamSearchPanel searchPanel;
    LayoutManager layoutManager(&window, &workspace, &searchPanel, &window);
    layoutManager.setupLayout();

    auto *editor = new QalamEditor(&workspace);
    workspace.addTab(editor, QStringLiteral("لوحة مشتركة.باء"));
    QVERIFY(workspace.splitCurrent(Qt::Horizontal));
    QCOMPARE(workspace.groupCount(), 2);
    QCOMPARE(window.findChildren<QalamPanelArea*>().size(), 1);
    QCOMPARE(layoutManager.panelArea(),
             window.findChild<QalamPanelArea*>());
}

QTEST_MAIN(TestEditorWorkspace)

#include "TestEditorWorkspace.moc"
