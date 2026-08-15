#include "ProjectSearchService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class TestProjectSearchService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void searchesOneCharacterArabicWholeWordsInOpenOverlay();
    void preparesRegexReplacementAndPreservesUtf8Bom();
    void reportsSkippedFilesLimitsProgressAndCacheHits();
    void suppressesStaleCancelledResults();
};

namespace {

void writeBytes(const QString &filePath, const QByteArray &bytes)
{
    QFile file(filePath);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
    QCOMPARE(file.write(bytes), bytes.size());
}

void writeUtf8(const QString &filePath, const QString &text)
{
    writeBytes(filePath, text.toUtf8());
}

QString pathKey(const QString &filePath)
{
    QString path = QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
#ifdef Q_OS_WIN
    path = path.toCaseFolded();
#endif
    return path;
}

ProjectSearchResult takeSearchResult(QSignalSpy *spy)
{
    const QList<QVariant> arguments = spy->takeFirst();
    return qvariant_cast<ProjectSearchResult>(arguments.at(0));
}

ProjectReplacementPlan takeReplacementPlan(QSignalSpy *spy)
{
    const QList<QVariant> arguments = spy->takeFirst();
    return qvariant_cast<ProjectReplacementPlan>(arguments.at(0));
}

}

void TestProjectSearchService::initTestCase()
{
    qRegisterMetaType<ProjectSearchResult>();
    qRegisterMetaType<ProjectReplacementPlan>();
}

void TestProjectSearchService::searchesOneCharacterArabicWholeWordsInOpenOverlay()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("رئيسي.baa"));
    writeUtf8(filePath, QStringLiteral("نص قديم\n"));

    ProjectSearchRequest request;
    request.rootPath = directory.path();
    request.filePaths = {filePath};
    request.query = QStringLiteral("س");
    request.wholeWord = true;
    request.overlays.insert(
        pathKey(filePath),
        {QStringLiteral("س سطرس سَ، س\n"), 42});

    ProjectSearchService service;
    QSignalSpy finished(&service, &ProjectSearchService::searchFinished);
    service.search(request);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 5000);

    const ProjectSearchResult result = takeSearchResult(&finished);
    QVERIFY(result.error.isEmpty());
    QCOMPARE(result.scannedFiles, 1);
    QCOMPARE(result.fileCount, 1);
    QCOMPARE(result.matches.size(), 2);
    QCOMPARE(result.matches.at(0).line, 0);
    QCOMPARE(result.matches.at(0).character, 0);
    QCOMPARE(result.matches.at(1).matchedText, QStringLiteral("س"));
}

void TestProjectSearchService::preparesRegexReplacementAndPreservesUtf8Bom()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("قيم.baa"));
    QByteArray bytes("\xEF\xBB\xBF");
    bytes += QStringLiteral("قيمة12\nقيمة34\n").toUtf8();
    writeBytes(filePath, bytes);

    ProjectSearchRequest request;
    request.rootPath = directory.path();
    request.filePaths = {filePath};
    request.query = QStringLiteral("(قيمة)([0-9]+)");
    request.replacement = QStringLiteral("$2_$1");
    request.regularExpression = true;

    ProjectSearchService service;
    QSignalSpy prepared(&service,
                        &ProjectSearchService::replacementPrepared);
    service.prepareReplacement(request);
    QTRY_COMPARE_WITH_TIMEOUT(prepared.count(), 1, 5000);

    const ProjectReplacementPlan plan = takeReplacementPlan(&prepared);
    QVERIFY(plan.error.isEmpty());
    QCOMPARE(plan.replacementCount, 2);
    QCOMPARE(plan.files.size(), 1);
    const ProjectReplacementFile &file = plan.files.constFirst();
    QCOMPARE(file.sourceRevision, -1);
    QCOMPARE(file.originalBytes, bytes);
    QCOMPARE(file.updatedText, QStringLiteral("12_قيمة\n34_قيمة\n"));
    QVERIFY(file.updatedBytes.startsWith("\xEF\xBB\xBF"));
    QCOMPARE(file.updatedBytes.mid(3), file.updatedText.toUtf8());
}

void TestProjectSearchService::reportsSkippedFilesLimitsProgressAndCacheHits()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString validPath = directory.filePath(QStringLiteral("صالح.baa"));
    const QString invalidPath = directory.filePath(QStringLiteral("تالف.baa"));
    writeUtf8(validPath, QStringLiteral("س س\n"));
    writeBytes(invalidPath, QByteArray::fromHex("fffe"));

    ProjectSearchRequest request;
    request.rootPath = directory.path();
    request.filePaths = {validPath, invalidPath};
    request.query = QStringLiteral("س");
    request.maximumMatches = 10;

    ProjectSearchService service;
    QSignalSpy progress(&service, &ProjectSearchService::searchProgress);
    QSignalSpy finished(&service, &ProjectSearchService::searchFinished);
    service.search(request);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 5000);
    ProjectSearchResult result = takeSearchResult(&finished);
    QVERIFY(result.error.isEmpty());
    QCOMPARE(result.matches.size(), 2);
    QCOMPARE(result.skippedFiles, 1);
    QCOMPARE(result.scannedFiles, 2);
    QVERIFY(progress.count() >= 1);

    progress.clear();
    service.search(request);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 5000);
    result = takeSearchResult(&finished);
    QCOMPARE(result.matches.size(), 2);
    QCOMPARE(progress.count(), 0);

    request.maximumMatches = 1;
    service.search(request);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 5000);
    result = takeSearchResult(&finished);
    QCOMPARE(result.matches.size(), 1);
    QVERIFY(result.truncated);

    QSignalSpy replacement(&service,
                           &ProjectSearchService::replacementPrepared);
    request.maximumMatches = 10;
    service.prepareReplacement(request);
    QTRY_COMPARE_WITH_TIMEOUT(replacement.count(), 1, 5000);
    const ProjectReplacementPlan plan = takeReplacementPlan(&replacement);
    QVERIFY(not plan.error.isEmpty());
    QVERIFY(plan.files.isEmpty());
    QCOMPARE(plan.replacementCount, 0);
}

void TestProjectSearchService::suppressesStaleCancelledResults()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("كبير.baa"));
    writeUtf8(filePath, QString(1000000, QChar(0x0633)));

    ProjectSearchRequest first;
    first.rootPath = directory.path();
    first.filePaths = {filePath};
    first.query = QStringLiteral("س");
    first.maximumMatches = 1000000;

    ProjectSearchRequest second = first;
    second.query = QStringLiteral("غ");
    second.maximumMatches = 10;

    ProjectSearchService service;
    QSignalSpy finished(&service, &ProjectSearchService::searchFinished);
    const int firstId = service.search(first);
    const int secondId = service.search(second);
    QVERIFY(secondId > firstId);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);
    const ProjectSearchResult result = takeSearchResult(&finished);
    QCOMPARE(result.requestId, secondId);
    QVERIFY(result.matches.isEmpty());
    QTest::qWait(100);
    QCOMPARE(finished.count(), 0);
}

QTEST_MAIN(TestProjectSearchService)
#include "TestProjectSearchService.moc"
