#include "BuildManager.h"

#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

class TestBuildManager : public QObject
{
    Q_OBJECT

private slots:
    void buildsValidatedTakweenArguments();
    void filtersTakweenTargetsByCapability();
    void classifiesCompilerCliExitCodes();
    void buildsOperationAwareExitDiagnostics();
    void findsNearestTakweenProjectRoot();
    void returnsEmptyRootOutsideTakweenProject();
    void recognizesNazmSourcesAndObjects();
};

void TestBuildManager::buildsValidatedTakweenArguments()
{
    QCOMPARE(BuildManager::takweenCommandArguments("build"), QStringList{"بناء"});
    QCOMPARE(BuildManager::takweenCommandArguments(" RUN "), QStringList{"تشغيل"});
    QCOMPARE(BuildManager::takweenCommandArguments("test", "اختبار_أ"),
             (QStringList{"اختبار", "اختبار_أ"}));
    QCOMPARE(BuildManager::takweenCommandArguments("clean"), QStringList{"تنظيف"});
    QVERIFY(BuildManager::takweenCommandArguments("clean", "تطبيق").isEmpty());
    QVERIFY(BuildManager::takweenCommandArguments("publish").isEmpty());
    QVERIFY(BuildManager::takweenCommandArguments("build & whoami").isEmpty());
}

void TestBuildManager::filtersTakweenTargetsByCapability()
{
    const QVector<TakweenTarget> targets = {
        {"تطبيق", "executable", "ready", true, true, false},
        {"اختبار_أ", "test", "ready", true, true, true},
        {"مكتبة", "library", "unsupported", false, false, false}
    };
    QCOMPARE(BuildManager::selectableTakweenTargets(targets, "build").size(), 2);
    QCOMPARE(BuildManager::selectableTakweenTargets(targets, "run").size(), 2);
    const auto tests = BuildManager::selectableTakweenTargets(targets, "test");
    QCOMPARE(tests.size(), 1);
    QCOMPARE(tests.first().name, QString("اختبار_أ"));
}

void TestBuildManager::classifiesCompilerCliExitCodes()
{
    using ExitClass = BuildManager::CompilerExitClass;
    QCOMPARE(BuildManager::classifyCompilerExitCode(0), ExitClass::Success);
    QCOMPARE(BuildManager::classifyCompilerExitCode(1), ExitClass::SourceError);
    QCOMPARE(BuildManager::classifyCompilerExitCode(2), ExitClass::InvalidInvocation);
    QCOMPARE(BuildManager::classifyCompilerExitCode(3), ExitClass::Unsupported);
    QCOMPARE(BuildManager::classifyCompilerExitCode(4), ExitClass::ToolchainError);
    QCOMPARE(BuildManager::classifyCompilerExitCode(5), ExitClass::InternalError);
    QCOMPARE(BuildManager::classifyCompilerExitCode(-2), ExitClass::Cancelled);
    QCOMPARE(BuildManager::classifyCompilerExitCode(-1), ExitClass::ProcessFailure);
    QCOMPARE(BuildManager::classifyCompilerExitCode(42), ExitClass::Unknown);
}

void TestBuildManager::buildsOperationAwareExitDiagnostics()
{
    QCOMPARE(BuildManager::compilerExitCodeId(4), QString("CLI_EXIT_4"));
    QCOMPARE(BuildManager::compilerExitCodeId(-1), QString("PROCESS_FAILURE"));
    QCOMPARE(BuildManager::compilerExitCodeId(-2), QString("TOOLING_CANCELLED"));
    QVERIFY(BuildManager::compilerExitSummary(1, "check").contains("1"));
    QVERIFY(BuildManager::compilerExitSummary(4, "build").contains("4"));
    QVERIFY(BuildManager::compilerExitSummary(5, "build").contains("5"));

    const QString runSummary = BuildManager::compilerExitSummary(1, "run");
    QVERIFY(runSummary.contains("run"));
    QVERIFY(runSummary.contains("1"));
    QVERIFY(BuildManager::compilerExitSummary(-2, "run").contains("أُلغيت"));
}

void TestBuildManager::findsNearestTakweenProjectRoot()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    QVERIFY(root.mkpath("source/nested"));

    QFile manifest(root.filePath("مشروع.تكوين"));
    QVERIFY(manifest.open(QIODevice::WriteOnly));
    manifest.write("name: test\n");
    manifest.close();

    const QString source = root.filePath("source/nested/main.baa");
    QCOMPARE(BuildManager::findTakweenProjectRoot(source), QDir::cleanPath(temp.path()));
}

void TestBuildManager::returnsEmptyRootOutsideTakweenProject()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QVERIFY(BuildManager::findTakweenProjectRoot(QDir(temp.path()).filePath("main.baa")).isEmpty());
}

void TestBuildManager::recognizesNazmSourcesAndObjects()
{
    QVERIFY(BuildManager::isNazmSourcePath(QStringLiteral("مصدر/بداية.نظم")));
    QVERIFY(not BuildManager::isNazmSourcePath(QStringLiteral("مصدر/بداية.baa")));
#if defined(Q_OS_WIN)
    QVERIFY(BuildManager::nazmObjectPath(QStringLiteral("C:/مشروع/بداية.نظم"))
                .endsWith(QStringLiteral("بداية.obj")));
#else
    QVERIFY(BuildManager::nazmObjectPath(QStringLiteral("/tmp/مشروع/بداية.نظم"))
                .endsWith(QStringLiteral("بداية.o")));
#endif
}

QTEST_MAIN(TestBuildManager)
#include "TestBuildManager.moc"
