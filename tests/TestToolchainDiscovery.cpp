#include "ToolchainDiscovery.h"
#include "Constants.h"

#include <QtTest>
#include <QCoreApplication>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

class TestToolchainDiscovery : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void settingsOverrideEnvironmentAndPath();
    void environmentOverridesPath();
    void pathOverridesPortableFallback();
    void invalidExplicitSettingRemainsVisible();
    void buildsEnvironmentForConfiguredTools();

private:
    QString makeExecutable(const QString &directory, const QString &name);
    std::unique_ptr<QTemporaryDir> m_filesDirectory;
    QByteArray m_originalPath;
};

void TestToolchainDiscovery::initTestCase()
{
    m_filesDirectory = std::make_unique<QTemporaryDir>();
    QVERIFY(m_filesDirectory->isValid());
    m_originalPath = qgetenv("PATH");
}

void TestToolchainDiscovery::cleanup()
{
    QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                       Constants::OrgName, Constants::AppName);
    settings.clear();
    settings.sync();
    qunsetenv("QALAM_BAA_PATH");
    qunsetenv("QALAM_TAKWEEN_PATH");
    qunsetenv("QALAM_NAZM_PATH");
    qputenv("PATH", m_originalPath);
}

QString TestToolchainDiscovery::makeExecutable(const QString &directory,
                                               const QString &name)
{
    if (not QDir().mkpath(directory)) {
        QTest::qFail("Could not create temporary tool directory", __FILE__, __LINE__);
        return {};
    }
    const QString destination = QDir(directory).filePath(name);
    if (not QFile::copy(QCoreApplication::applicationFilePath(), destination)) {
        QTest::qFail("Could not create temporary tool executable", __FILE__, __LINE__);
        return {};
    }
    QFile::setPermissions(destination, QFile::permissions(destination) |
                                      QFileDevice::ExeOwner |
                                      QFileDevice::ExeGroup |
                                      QFileDevice::ExeOther);
    return destination;
}

void TestToolchainDiscovery::settingsOverrideEnvironmentAndPath()
{
#if defined(Q_OS_WIN)
    const QString name = QStringLiteral("baa.exe");
#else
    const QString name = QStringLiteral("baa");
#endif
    const QString configured = makeExecutable(
        QDir(m_filesDirectory->path()).filePath(QStringLiteral("configured")), name);
    const QString environment = makeExecutable(
        QDir(m_filesDirectory->path()).filePath(QStringLiteral("environment")), name);
    const QString pathProgram = makeExecutable(
        QDir(m_filesDirectory->path()).filePath(QStringLiteral("path")), name);

    QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                       Constants::OrgName, Constants::AppName);
    settings.setValue(Constants::SettingsKeyCompilerPath, configured);
    settings.sync();
    qputenv("QALAM_BAA_PATH", environment.toUtf8());
    qputenv("PATH", QFileInfo(pathProgram).absolutePath().toUtf8());

    const QalamToolResolution result = ToolchainDiscovery::resolve(QalamToolKind::Baa);
    QCOMPARE(result.source, QalamToolSource::Settings);
    QCOMPARE(QDir::cleanPath(result.program), QDir::cleanPath(configured));
    QVERIFY(result.isAvailable());
}

void TestToolchainDiscovery::environmentOverridesPath()
{
#if defined(Q_OS_WIN)
    const QString name = QStringLiteral("nazm.exe");
#else
    const QString name = QStringLiteral("nazm");
#endif
    const QString environment = makeExecutable(
        QDir(m_filesDirectory->path()).filePath(QStringLiteral("nazm-env")), name);
    const QString pathProgram = makeExecutable(
        QDir(m_filesDirectory->path()).filePath(QStringLiteral("nazm-path")), name);
    qputenv("QALAM_NAZM_PATH", environment.toUtf8());
    qputenv("PATH", QFileInfo(pathProgram).absolutePath().toUtf8());

    const QalamToolResolution result = ToolchainDiscovery::resolve(QalamToolKind::Nazm);
    QCOMPARE(result.source, QalamToolSource::Environment);
    QCOMPARE(QDir::cleanPath(result.program), QDir::cleanPath(environment));
}

void TestToolchainDiscovery::pathOverridesPortableFallback()
{
#if defined(Q_OS_WIN)
    const QString name = QStringLiteral("takween.exe");
#else
    const QString name = QStringLiteral("takween");
#endif
    const QString pathProgram = makeExecutable(
        QDir(m_filesDirectory->path()).filePath(QStringLiteral("takween-path")), name);
    const QString appDirectory = QDir(m_filesDirectory->path()).filePath(QStringLiteral("app"));
    makeExecutable(QDir(appDirectory).filePath(QStringLiteral("takween")), name);
    qputenv("PATH", QFileInfo(pathProgram).absolutePath().toUtf8());

    const QalamToolResolution result =
        ToolchainDiscovery::resolve(QalamToolKind::Takween, appDirectory);
    QCOMPARE(result.source, QalamToolSource::Path);
    QCOMPARE(QDir::cleanPath(result.program), QDir::cleanPath(pathProgram));
}

void TestToolchainDiscovery::invalidExplicitSettingRemainsVisible()
{
    QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                       Constants::OrgName, Constants::AppName);
    settings.setValue(Constants::SettingsKeyCompilerPath,
                      QDir(m_filesDirectory->path()).filePath(QStringLiteral("missing-baa")));
    settings.sync();

    const QalamToolResolution result = ToolchainDiscovery::resolve(QalamToolKind::Baa);
    QCOMPARE(result.source, QalamToolSource::Settings);
    QVERIFY(not result.isAvailable());
    QVERIFY(not result.requestedProgram.isEmpty());
    QCOMPARE(result.program, result.requestedProgram);
}

void TestToolchainDiscovery::buildsEnvironmentForConfiguredTools()
{
#if defined(Q_OS_WIN)
    const QString baaName = QStringLiteral("baa.exe");
    const QString nazmName = QStringLiteral("نظم.exe");
#else
    const QString baaName = QStringLiteral("baa");
    const QString nazmName = QStringLiteral("نظم");
#endif
    const QString baaDirectory = QDir(m_filesDirectory->path()).filePath(QStringLiteral("baa-home"));
    const QString baa = makeExecutable(baaDirectory, baaName);
    QVERIFY(QDir().mkpath(QDir(baaDirectory).filePath(QStringLiteral("stdlib"))));
    const QString nazm = makeExecutable(
        QDir(m_filesDirectory->path()).filePath(QStringLiteral("nazm-home")), nazmName);

    QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                       Constants::OrgName, Constants::AppName);
    settings.setValue(Constants::SettingsKeyCompilerPath, baa);
    settings.setValue(Constants::SettingsKeyNazmPath, nazm);
    settings.sync();

    const QProcessEnvironment environment = ToolchainDiscovery::processEnvironment();
    QCOMPARE(QDir::cleanPath(environment.value(QStringLiteral("BAA_NAZM"))),
             QDir::cleanPath(nazm));
    QCOMPARE(QDir::cleanPath(environment.value(QStringLiteral("BAA_HOME"))),
             QDir::cleanPath(baaDirectory));
    QVERIFY(environment.value(QStringLiteral("PATH")).contains(
        QDir::toNativeSeparators(baaDirectory), Qt::CaseInsensitive));
    QVERIFY(environment.value(QStringLiteral("PATH")).contains(
        QDir::toNativeSeparators(QFileInfo(nazm).absolutePath()),
        Qt::CaseInsensitive));
}

int main(int argc, char **argv)
{
    QTemporaryDir settingsDirectory;
    if (not settingsDirectory.isValid()) return 1;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settingsDirectory.path());
    QApplication application(argc, argv);
    TestToolchainDiscovery test;
    return QTest::qExec(&test, argc, argv);
}
#include "TestToolchainDiscovery.moc"
