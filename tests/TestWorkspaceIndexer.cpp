#include "WorkspaceIndexer.h"

#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

class TestWorkspaceIndexer : public QObject
{
    Q_OBJECT

private slots:
    void indexesAllowedFilesAndSkipsGeneratedFolders();
    void appliesNestedGitignoreAnchorsGlobsAndNegation();
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

    QCOMPARE(spy.count(), 1);
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
    indexer.setRootPath(tempDir.path());
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

QTEST_MAIN(TestWorkspaceIndexer)
#include "TestWorkspaceIndexer.moc"
