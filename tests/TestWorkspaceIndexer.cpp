#include "WorkspaceIndexer.h"

#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTextStream>

class TestWorkspaceIndexer : public QObject
{
    Q_OBJECT

private slots:
    void indexesAllowedFilesAndSkipsGeneratedFolders();
    void appliesNestedGitignoreAnchorsGlobsAndNegation();
    void refreshesAfterExternalWorkspaceChanges();
    void rejectsStaleIndexWhenSwitchingRoots();
    void indexesMultipleWorkspaceRootsWithoutDuplicates();
    void ignoresGeneratedDirectoriesPerWorkspaceRoot();
};

namespace {
void writeUtf8File(const QString &path, const QString &content)
{
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Text), qPrintable(file.errorString()));
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << content;
}
}

void TestWorkspaceIndexer::indexesAllowedFilesAndSkipsGeneratedFolders()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QDir root(tempDir.path());
    QVERIFY(root.mkpath("src"));
    QVERIFY(root.mkpath("build"));
    QVERIFY(root.mkpath(".git"));

    writeUtf8File(root.filePath("src/main.baa"), "دالة البداية()\n");
    writeUtf8File(root.filePath("README.md"), "# مشروع\n");
    writeUtf8File(root.filePath("build/generated.baa"), "دالة يجب_تجاهلها()\n");
    writeUtf8File(root.filePath(".git/config"), "[core]\n");
    writeUtf8File(root.filePath("image.png"), "not source\n");

    WorkspaceIndexer indexer;
    QSignalSpy spy(&indexer, &WorkspaceIndexer::indexUpdated);
    indexer.setRootPath(tempDir.path());

    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 5000);
    const QStringList files = indexer.files();
    QVERIFY(files.contains(QDir::cleanPath(root.filePath("src/main.baa"))));
    QVERIFY(files.contains(QDir::cleanPath(root.filePath("README.md"))));
    QVERIFY(std::none_of(files.begin(), files.end(), [](const QString &path) {
        return path.contains("/build/") || path.contains("/.git/") || path.endsWith("image.png");
    }));
}

void TestWorkspaceIndexer::appliesNestedGitignoreAnchorsGlobsAndNegation()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QDir root(tempDir.path());
    QVERIFY(root.mkpath(QStringLiteral("generated")));
    QVERIFY(root.mkpath(QStringLiteral("nested/private")));
    QVERIFY(root.mkpath(QStringLiteral("nested/cache/deep")));
    QVERIFY(root.mkpath(QStringLiteral("nested/other")));

    writeUtf8File(root.filePath(QStringLiteral(".gitignore")),
                  QStringLiteral(
                      "generated/*.baa\n"
                      "!generated/keep.baa\n"
                      "/root-only.baa\n"
                      "**/cache/**\n"));
    writeUtf8File(root.filePath(QStringLiteral("nested/.gitignore")),
                  QStringLiteral("private/\n"));
    writeUtf8File(root.filePath(QStringLiteral("generated/drop.baa")),
                  QStringLiteral("مرفوض\n"));
    writeUtf8File(root.filePath(QStringLiteral("generated/keep.baa")),
                  QStringLiteral("مسموح\n"));
    writeUtf8File(root.filePath(QStringLiteral("root-only.baa")),
                  QStringLiteral("مرفوض\n"));
    writeUtf8File(root.filePath(QStringLiteral("nested/root-only.baa")),
                  QStringLiteral("مسموح\n"));
    writeUtf8File(root.filePath(QStringLiteral("nested/private/secret.baa")),
                  QStringLiteral("مرفوض\n"));
    writeUtf8File(root.filePath(QStringLiteral("nested/cache/deep/data.baa")),
                  QStringLiteral("مرفوض\n"));
    writeUtf8File(root.filePath(QStringLiteral("nested/other/main.baa")),
                  QStringLiteral("مسموح\n"));

    WorkspaceIndexer indexer;
    QSignalSpy spy(&indexer, &WorkspaceIndexer::indexUpdated);
    indexer.setRootPath(tempDir.path());
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 5000);
    const QStringList files = indexer.files();

    QVERIFY(files.contains(QDir::cleanPath(
        root.filePath(QStringLiteral("generated/keep.baa")))));
    QVERIFY(files.contains(QDir::cleanPath(
        root.filePath(QStringLiteral("nested/root-only.baa")))));
    QVERIFY(files.contains(QDir::cleanPath(
        root.filePath(QStringLiteral("nested/other/main.baa")))));
    QVERIFY(not files.contains(QDir::cleanPath(
        root.filePath(QStringLiteral("generated/drop.baa")))));
    QVERIFY(not files.contains(QDir::cleanPath(
        root.filePath(QStringLiteral("root-only.baa")))));
    QVERIFY(not files.contains(QDir::cleanPath(
        root.filePath(QStringLiteral("nested/private/secret.baa")))));
    QVERIFY(not files.contains(QDir::cleanPath(
        root.filePath(QStringLiteral("nested/cache/deep/data.baa")))));
}

void TestWorkspaceIndexer::refreshesAfterExternalWorkspaceChanges()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());

    const QString originalPath =
        root.filePath(QStringLiteral("الأصل.baa"));
    writeUtf8File(originalPath, QStringLiteral("الأصل\n"));

    WorkspaceIndexer indexer;
    QSignalSpy updates(&indexer, &WorkspaceIndexer::indexUpdated);
    indexer.setRootPath(tempDir.path());
    QTRY_VERIFY_WITH_TIMEOUT(
        indexer.files().contains(QDir::cleanPath(originalPath)), 5000);

    QVERIFY(root.mkpath(QStringLiteral("خارجي/متداخل")));
    const QString createdPath = root.filePath(
        QStringLiteral("خارجي/متداخل/جديد.baa"));
    writeUtf8File(createdPath, QStringLiteral("جديد\n"));
    QTRY_VERIFY_WITH_TIMEOUT(
        indexer.files().contains(QDir::cleanPath(createdPath)), 5000);

    const QString renamedPath = root.filePath(
        QStringLiteral("خارجي/متداخل/مُعاد.baa"));
    QVERIFY(QFile::rename(createdPath, renamedPath));
    QTRY_VERIFY_WITH_TIMEOUT(
        not indexer.files().contains(QDir::cleanPath(createdPath)) and
        indexer.files().contains(QDir::cleanPath(renamedPath)),
        5000);

    const QString ignorePath = root.filePath(QStringLiteral(".gitignore"));
    writeUtf8File(ignorePath, QStringLiteral("**/مُعاد.baa\n"));
    QTRY_VERIFY_WITH_TIMEOUT(
        not indexer.files().contains(QDir::cleanPath(renamedPath)), 5000);

    writeUtf8File(ignorePath, QStringLiteral("# لا استثناءات\n"));
    QTRY_VERIFY_WITH_TIMEOUT(
        indexer.files().contains(QDir::cleanPath(renamedPath)), 5000);

    updates.clear();
    writeUtf8File(originalPath, QStringLiteral("تغيير خارجي\n"));
    QTRY_VERIFY_WITH_TIMEOUT(updates.count() >= 1, 5000);
    QVERIFY(indexer.files().contains(QDir::cleanPath(originalPath)));

    QVERIFY(QFile::remove(renamedPath));
    QTRY_VERIFY_WITH_TIMEOUT(
        not indexer.files().contains(QDir::cleanPath(renamedPath)), 5000);
}

void TestWorkspaceIndexer::rejectsStaleIndexWhenSwitchingRoots()
{
    QTemporaryDir firstDirectory;
    QTemporaryDir secondDirectory;
    QVERIFY(firstDirectory.isValid());
    QVERIFY(secondDirectory.isValid());

    const QString firstPath =
        firstDirectory.filePath(QStringLiteral("الأول.baa"));
    const QString secondPath =
        secondDirectory.filePath(QStringLiteral("الثاني.baa"));
    writeUtf8File(firstPath, QStringLiteral("الأول\n"));
    writeUtf8File(secondPath, QStringLiteral("الثاني\n"));

    WorkspaceIndexer indexer;
    QSignalSpy updates(&indexer, &WorkspaceIndexer::indexUpdated);
    indexer.setRootPath(firstDirectory.path());
    QVERIFY(indexer.isIndexing());
    indexer.setRootPath(secondDirectory.path());
    QVERIFY(indexer.isIndexing());

    QTRY_VERIFY_WITH_TIMEOUT(
        indexer.files().contains(QDir::cleanPath(secondPath)), 5000);
    QVERIFY(not indexer.isIndexing());
    QVERIFY(not indexer.files().contains(QDir::cleanPath(firstPath)));
    QCOMPARE(indexer.rootPath(), QDir::cleanPath(
        QFileInfo(secondDirectory.path()).absoluteFilePath()));
    QTest::qWait(200);
    QVERIFY(not indexer.files().contains(QDir::cleanPath(firstPath)));
    QVERIFY(updates.count() >= 1);
}

void TestWorkspaceIndexer::indexesMultipleWorkspaceRootsWithoutDuplicates()
{
    QTemporaryDir firstDirectory;
    QTemporaryDir secondDirectory;
    QVERIFY(firstDirectory.isValid());
    QVERIFY(secondDirectory.isValid());

    const QString firstPath = firstDirectory.filePath(
        QStringLiteral("الأول.باء"));
    const QString secondPath = secondDirectory.filePath(
        QStringLiteral("الثاني.باء"));
    writeUtf8File(firstPath, QStringLiteral("الأول\n"));
    writeUtf8File(secondPath, QStringLiteral("الثاني\n"));

    WorkspaceIndexer indexer;
    indexer.setRootPaths({firstDirectory.path(), secondDirectory.path(),
                          firstDirectory.path()});
    QTRY_VERIFY_WITH_TIMEOUT(
        indexer.files().contains(QDir::cleanPath(firstPath)) and
        indexer.files().contains(QDir::cleanPath(secondPath)), 5000);
    QCOMPARE(indexer.rootPaths().size(), 2);
    QCOMPARE(indexer.files().count(QDir::cleanPath(firstPath)), 1);
    QCOMPARE(indexer.files().count(QDir::cleanPath(secondPath)), 1);
}

void TestWorkspaceIndexer::ignoresGeneratedDirectoriesPerWorkspaceRoot()
{
    QTemporaryDir firstDirectory;
    QTemporaryDir secondDirectory;
    QVERIFY(firstDirectory.isValid());
    QVERIFY(secondDirectory.isValid());

    const QString firstSource = firstDirectory.filePath(
        QStringLiteral("مصدر/الأول.باء"));
    const QString ignoredFirst = firstDirectory.filePath(
        QStringLiteral("build/مولد.باء"));
    const QString secondSource = secondDirectory.filePath(
        QStringLiteral("مصدر/الثاني.باء"));
    const QString ignoredSecond = secondDirectory.filePath(
        QStringLiteral("dist/حزمة.باء"));
    QVERIFY(QDir().mkpath(QFileInfo(firstSource).absolutePath()));
    QVERIFY(QDir().mkpath(QFileInfo(ignoredFirst).absolutePath()));
    QVERIFY(QDir().mkpath(QFileInfo(secondSource).absolutePath()));
    QVERIFY(QDir().mkpath(QFileInfo(ignoredSecond).absolutePath()));
    writeUtf8File(firstSource, QStringLiteral("الأول\n"));
    writeUtf8File(ignoredFirst, QStringLiteral("مولد\n"));
    writeUtf8File(secondSource, QStringLiteral("الثاني\n"));
    writeUtf8File(ignoredSecond, QStringLiteral("حزمة\n"));

    WorkspaceIndexer indexer;
    indexer.setRootPaths({firstDirectory.path(), secondDirectory.path()});
    QTRY_VERIFY_WITH_TIMEOUT(
        indexer.files().contains(QDir::cleanPath(firstSource)) and
        indexer.files().contains(QDir::cleanPath(secondSource)), 5000);
    QVERIFY(not indexer.files().contains(QDir::cleanPath(ignoredFirst)));
    QVERIFY(not indexer.files().contains(QDir::cleanPath(ignoredSecond)));
    QVERIFY(indexer.isIgnoredPath(ignoredFirst));
    QVERIFY(indexer.isIgnoredPath(ignoredSecond));
}

QTEST_MAIN(TestWorkspaceIndexer)
#include "TestWorkspaceIndexer.moc"
