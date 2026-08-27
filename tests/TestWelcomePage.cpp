#include "QalamWelcomePage.h"
#include "Constants.h"

#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class TestWelcomePage final : public QObject
{
    Q_OBJECT

private slots:
    void separatesProjectsAndFilesAndReopensLatestProject();
    void removesAnExistingEntryFromRecents();
};

void TestWelcomePage::separatesProjectsAndFilesAndReopensLatestProject()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString projectPath = temporaryDirectory.filePath(
        QStringLiteral("مشروع باء"));
    QVERIFY(QDir().mkpath(projectPath));
    const QString filePath = temporaryDirectory.filePath(
        QStringLiteral("تجربة.باء"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    const QString settingsPath = temporaryDirectory.filePath(
        QStringLiteral("welcome.ini"));
    QSettings settings(settingsPath, QSettings::IniFormat);
    settings.setValue(Constants::SettingsKeyRecentFolders,
                      QStringList{projectPath});
    settings.setValue(Constants::SettingsKeyRecentFiles,
                      QStringList{filePath});
    settings.sync();

    QalamWelcomePage page(nullptr, settingsPath);
    auto *projects = page.findChild<QListWidget*>("welcomeRecentProjects");
    auto *files = page.findChild<QListWidget*>("welcomeRecentFiles");
    auto *reopen = page.findChild<QPushButton*>("welcomeReopenLastProject");
    QVERIFY(projects);
    QVERIFY(files);
    QVERIFY(reopen);
    QCOMPARE(projects->count(), 1);
    QCOMPARE(files->count(), 1);
    QCOMPARE(projects->item(0)->data(Qt::UserRole).toString(), projectPath);
    QCOMPARE(files->item(0)->data(Qt::UserRole).toString(), filePath);
    QVERIFY(reopen->isEnabled());

    QSignalSpy spy(&page, &QalamWelcomePage::reopenLastProjectRequested);
    QTest::mouseClick(reopen, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.constFirst().constFirst().toString(), projectPath);
}

void TestWelcomePage::removesAnExistingEntryFromRecents()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString projectPath = temporaryDirectory.filePath(
        QStringLiteral("مشروع للحذف"));
    QVERIFY(QDir().mkpath(projectPath));
    const QString settingsPath = temporaryDirectory.filePath(
        QStringLiteral("remove.ini"));
    QSettings settings(settingsPath, QSettings::IniFormat);
    settings.setValue(Constants::SettingsKeyRecentFolders,
                      QStringList{projectPath});
    settings.sync();

    QalamWelcomePage page(nullptr, settingsPath);
    QPushButton *removeButton{};
    const auto buttons = page.findChildren<QPushButton*>(
        "welcomeRemoveRecent");
    for (QPushButton *button : buttons) {
        if (button->property("recentPath").toString() == projectPath) {
            removeButton = button;
            break;
        }
    }
    QVERIFY(removeButton);

    QTest::mouseClick(removeButton, Qt::LeftButton);
    settings.sync();
    QCOMPARE(settings.value(Constants::SettingsKeyRecentFolders)
                 .toStringList(), QStringList{});

    auto *reopen = page.findChild<QPushButton*>("welcomeReopenLastProject");
    QVERIFY(reopen);
    QVERIFY(not reopen->isEnabled());
}

QTEST_MAIN(TestWelcomePage)

#include "TestWelcomePage.moc"
