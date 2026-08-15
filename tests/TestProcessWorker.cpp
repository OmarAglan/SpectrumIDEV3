#include "ProcessWorker.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

class TestProcessWorker : public QObject
{
    Q_OBJECT

private slots:
    void tailsCompleteAndFinalJsonLines();
    void reportsRequestedCancellation();
    void suppliesBundledBaaHome();
    void runsFollowUpProcessAndCapturesOutput();
    void skipsFollowUpProcessAfterFailure();
};

void TestProcessWorker::tailsCompleteAndFinalJsonLines()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString eventPath = temporary.filePath("أحداث جزئية.jsonl");
    ProcessWorker worker(
        QCoreApplication::applicationFilePath(),
        {"--helper-events", eventPath},
        temporary.path(),
        eventPath);
    QSignalSpy events(&worker, &ProcessWorker::eventLineReady);
    QSignalSpy finished(&worker, &ProcessWorker::finished);

    worker.start();
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 5000);
    QCOMPARE(finished.first().first().toInt(), 0);
    QCOMPARE(events.size(), 2);
    QCOMPARE(events[0].first().toByteArray(), QByteArray("{\"sequence\":1}"));
    QCOMPARE(events[1].first().toByteArray(), QByteArray("{\"sequence\":2}"));
}

void TestProcessWorker::reportsRequestedCancellation()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    ProcessWorker worker(
        QCoreApplication::applicationFilePath(),
        {"--helper-wait"},
        temporary.path());
    QSignalSpy finished(&worker, &ProcessWorker::finished);

    worker.start();
    QTest::qWait(150);
    worker.stop();
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 5000);
    QCOMPARE(finished.first().first().toInt(), -2);
}

void TestProcessWorker::suppliesBundledBaaHome()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QVERIFY(QDir().mkpath(temporary.filePath(QStringLiteral("stdlib"))));

#if defined(Q_OS_WIN)
    const QString compilerPath = temporary.filePath(QStringLiteral("baa.exe"));
    const QString nazmPath = temporary.filePath(QStringLiteral("نظم.exe"));
    const QString helperProgram = qEnvironmentVariable("ComSpec");
    const QStringList helperArguments = {
        QStringLiteral("/d"), QStringLiteral("/s"), QStringLiteral("/c"),
        QStringLiteral("echo %BAA_HOME%& if \"%BAA_NAZM%\"==\"%1\" (echo MATCH) else (echo MISMATCH)")
            .arg(nazmPath)
    };
#else
    const QString compilerPath = temporary.filePath(QStringLiteral("baa"));
    const QString nazmPath = temporary.filePath(QStringLiteral("نظم"));
    const QString helperProgram = QStringLiteral("/bin/sh");
    const QStringList helperArguments = {
        QStringLiteral("-c"),
        QStringLiteral("printf '%s\\n%s\\n' \"$BAA_HOME\" \"$BAA_NAZM\"")
    };
#endif
    QVERIFY(QFileInfo(helperProgram).isExecutable());
    QVERIFY(QFile::copy(helperProgram, compilerPath));
    QVERIFY(QFile::copy(helperProgram, nazmPath));
    QFile::setPermissions(
        compilerPath,
        QFileInfo(compilerPath).permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup |
            QFileDevice::ExeOther);
    QFile::setPermissions(
        nazmPath,
        QFileInfo(nazmPath).permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup |
            QFileDevice::ExeOther);

    const QByteArray previousBaaHome = qgetenv("BAA_HOME");
    const bool hadBaaHome = qEnvironmentVariableIsSet("BAA_HOME");
    const QByteArray previousBaaNazm = qgetenv("BAA_NAZM");
    const bool hadBaaNazm = qEnvironmentVariableIsSet("BAA_NAZM");
    qunsetenv("BAA_HOME");
    qunsetenv("BAA_NAZM");

    ProcessWorker worker(
        compilerPath,
        helperArguments,
        temporary.path());
    QSignalSpy output(&worker, &ProcessWorker::outputReady);
    QSignalSpy finished(&worker, &ProcessWorker::finished);

    worker.start();
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 5000);

    if (hadBaaHome) {
        qputenv("BAA_HOME", previousBaaHome);
    } else {
        qunsetenv("BAA_HOME");
    }
    if (hadBaaNazm) {
        qputenv("BAA_NAZM", previousBaaNazm);
    } else {
        qunsetenv("BAA_NAZM");
    }

    QCOMPARE(finished.first().first().toInt(), 0);
    QString combinedOutput;
    for (const QList<QVariant> &arguments : output) {
        combinedOutput += arguments.first().toString();
    }
    const QStringList environmentLines =
        combinedOutput.trimmed().split(QLatin1Char('\n'));
    QCOMPARE(environmentLines.size(), 2);
    QCOMPARE(QDir::cleanPath(environmentLines.at(0).trimmed()),
             QDir::cleanPath(temporary.path()));
#if defined(Q_OS_WIN)
    QCOMPARE(environmentLines.at(1).trimmed(), QStringLiteral("MATCH"));
#else
    QCOMPARE(QDir::cleanPath(environmentLines.at(1).trimmed()),
             QDir::cleanPath(nazmPath));
#endif
}

void TestProcessWorker::runsFollowUpProcessAndCapturesOutput()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    ProcessWorker worker(
        QCoreApplication::applicationFilePath(),
        {"--helper-exit", "0"},
        temporary.path(),
        {},
        QCoreApplication::applicationFilePath(),
        {"--helper-output", "7"},
        temporary.path(),
        QStringLiteral("\n▶ مخرجات البرنامج:\n"));
    QSignalSpy output(&worker, &ProcessWorker::outputReady);
    QSignalSpy finished(&worker, &ProcessWorker::finished);

    worker.start();
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 5000);
    QCOMPARE(finished.first().first().toInt(), 7);

    QString combinedOutput;
    for (const QList<QVariant> &arguments : output) {
        combinedOutput += arguments.first().toString();
    }
    QVERIFY(combinedOutput.contains(QStringLiteral("مخرجات البرنامج")));
    QVERIFY(combinedOutput.contains(QStringLiteral("ناتج باء من البرنامج")));
}

void TestProcessWorker::skipsFollowUpProcessAfterFailure()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString markerPath = temporary.filePath(QStringLiteral("شُغّل.txt"));
    ProcessWorker worker(
        QCoreApplication::applicationFilePath(),
        {"--helper-exit", "4"},
        temporary.path(),
        {},
        QCoreApplication::applicationFilePath(),
        {"--helper-marker", markerPath},
        temporary.path());
    QSignalSpy finished(&worker, &ProcessWorker::finished);

    worker.start();
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 5000);
    QCOMPARE(finished.first().first().toInt(), 4);
    QVERIFY(not QFileInfo::exists(markerPath));
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    const QStringList arguments = application.arguments();
    const int eventHelper = arguments.indexOf("--helper-events");
    if (eventHelper >= 0 and eventHelper + 1 < arguments.size()) {
        QFile file(arguments[eventHelper + 1]);
        if (not file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return 20;
        file.write("{\"sequence\":1}\n");
        file.flush();
        QThread::msleep(100);
        file.write("{\"sequence\":2}");
        file.flush();
        return 0;
    }
    if (arguments.contains("--helper-wait")) {
        QThread::sleep(30);
        return 0;
    }
    const int exitHelper = arguments.indexOf("--helper-exit");
    if (exitHelper >= 0 and exitHelper + 1 < arguments.size()) {
        return arguments.at(exitHelper + 1).toInt();
    }
    const int outputHelper = arguments.indexOf("--helper-output");
    if (outputHelper >= 0 and outputHelper + 1 < arguments.size()) {
        QFile output;
        if (not output.open(stdout, QIODevice::WriteOnly)) return 21;
        output.write(QStringLiteral("ناتج باء من البرنامج\n").toUtf8());
        output.flush();
        return arguments.at(outputHelper + 1).toInt();
    }
    const int markerHelper = arguments.indexOf("--helper-marker");
    if (markerHelper >= 0 and markerHelper + 1 < arguments.size()) {
        QFile marker(arguments.at(markerHelper + 1));
        if (not marker.open(QIODevice::WriteOnly | QIODevice::Truncate)) return 22;
        marker.write("started");
        return 0;
    }
    TestProcessWorker test;
    return QTest::qExec(&test, argc, argv);
}

#include "TestProcessWorker.moc"
