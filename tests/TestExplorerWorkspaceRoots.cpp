#include "QalamExplorerView.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class TestExplorerWorkspaceRoots final : public QObject
{
    Q_OBJECT

private slots:
    void presentsIndependentRootsAndIgnoresDuplicates();
};

void TestExplorerWorkspaceRoots::presentsIndependentRootsAndIgnoresDuplicates()
{
    QTemporaryDir first;
    QTemporaryDir second;
    QVERIFY(first.isValid());
    QVERIFY(second.isValid());
    QFile firstFile(first.filePath(QStringLiteral("أول.باء")));
    QFile secondFile(second.filePath(QStringLiteral("ثان.باء")));
    QVERIFY(firstFile.open(QIODevice::WriteOnly));
    QVERIFY(secondFile.open(QIODevice::WriteOnly));

    QalamExplorerView explorer;
    explorer.setRootPaths({first.path(), second.path(), first.path()});

    QCOMPARE(explorer.rootPaths().size(), 2);
    QCOMPARE(explorer.rootPath(), QDir::cleanPath(
        QFileInfo(first.path()).canonicalFilePath()));
    int workspaceTreeCount = 0;
    for (QTreeView *tree : explorer.findChildren<QTreeView*>()) {
        if (not tree->property("workspaceRoot").toString().isEmpty())
            ++workspaceTreeCount;
    }
    QCOMPARE(workspaceTreeCount, 2);
    QVERIFY(explorer.treeView());
    QVERIFY(explorer.fileSystemModel());
}

QTEST_MAIN(TestExplorerWorkspaceRoots)
#include "TestExplorerWorkspaceRoots.moc"
