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

QTEST_MAIN(TestWorkspaceIndexer)
#include "TestWorkspaceIndexer.moc"
