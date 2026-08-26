#include "SessionManager.h"
#include "QalamEditor.h"
#include "Constants.h"

#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTest>
#include <QTemporaryDir>

class TestSessionManager final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void acceptsReachableDesktopGeometry();
    void acceptsGeometryOnSecondaryMonitor();
    void rejectsTinyGeometry();
    void rejectsOffscreenGeometry();
    void rejectsBarelyVisibleGeometry();
    void restoresInterruptedUntitledBuffer();
    void cleanShutdownDoesNotRestoreDiscardedUntitledBuffer();

private:
    QTemporaryDir m_settingsDirectory;
};

void TestSessionManager::initTestCase()
{
    QVERIFY(m_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
}

void TestSessionManager::acceptsReachableDesktopGeometry()
{
    QVERIFY(SessionManager::isUsableWindowGeometry(
        QRect(100, 100, 1280, 720), {QRect(0, 0, 1920, 1040)}));
}

void TestSessionManager::acceptsGeometryOnSecondaryMonitor()
{
    QVERIFY(SessionManager::isUsableWindowGeometry(
        QRect(-1700, 100, 1200, 700),
        {QRect(0, 0, 1920, 1040), QRect(-1920, 0, 1920, 1080)}));
}

void TestSessionManager::rejectsTinyGeometry()
{
    QVERIFY(not SessionManager::isUsableWindowGeometry(
        QRect(48, 982, 50, 50), {QRect(0, 0, 1920, 1040)}));
}

void TestSessionManager::rejectsOffscreenGeometry()
{
    QVERIFY(not SessionManager::isUsableWindowGeometry(
        QRect(2400, 1200, 1280, 720), {QRect(0, 0, 1920, 1040)}));
}

void TestSessionManager::rejectsBarelyVisibleGeometry()
{
    QVERIFY(not SessionManager::isUsableWindowGeometry(
        QRect(48, 982, 900, 600), {QRect(0, 0, 1920, 1040)}));
}

void TestSessionManager::restoresInterruptedUntitledBuffer()
{
    QTabWidget tabs;
    auto *editor = new QalamEditor(&tabs);
    tabs.addTab(editor, QStringLiteral("غير معنون"));
    editor->setPlainText(QStringLiteral("صحيح الرئيسية() {\n    ارجع ٠.\n}"));
    editor->document()->setModified(true);

    const QString settingsPath = m_settingsDirectory.filePath(
        QStringLiteral("interrupted-session.ini"));
    SessionManager manager(&tabs, nullptr, settingsPath);
    manager.saveSession(QStringLiteral("C:/مشروع"), QByteArray("geometry"));

    QSettings savedSettings(settingsPath, QSettings::IniFormat);
    QCOMPARE(savedSettings.value(Constants::SessionKeyFolderPath).toString(),
             QStringLiteral("C:/مشروع"));

    const SessionManager::SessionData restored = manager.restoreSession();
    QCOMPARE(restored.folderPath, QStringLiteral("C:/مشروع"));
    QCOMPARE(restored.activeTabIndex, 0);
    QCOMPARE(restored.documents.size(), 1);
    QVERIFY(restored.recoveredAfterInterruption);
    QVERIFY(restored.documents.constFirst().hasRecovery);
    QCOMPARE(restored.documents.constFirst().recoveredContent,
             editor->toPlainText());
}

void TestSessionManager::cleanShutdownDoesNotRestoreDiscardedUntitledBuffer()
{
    QTabWidget tabs;
    auto *editor = new QalamEditor(&tabs);
    tabs.addTab(editor, QStringLiteral("غير معنون"));
    editor->setPlainText(QStringLiteral("تعديل لن يعود"));
    editor->document()->setModified(true);

    SessionManager manager(
        &tabs, nullptr,
        m_settingsDirectory.filePath(QStringLiteral("clean-session.ini")));
    manager.saveSession(QString(), QByteArray(), true);

    const SessionManager::SessionData restored = manager.restoreSession();
    QVERIFY(restored.documents.isEmpty());
    QVERIFY(not restored.recoveredAfterInterruption);
}

QTEST_MAIN(TestSessionManager)

#include "TestSessionManager.moc"
