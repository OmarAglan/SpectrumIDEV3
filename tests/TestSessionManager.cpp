#include "SessionManager.h"

#include <QTest>

class TestSessionManager final : public QObject
{
    Q_OBJECT

private slots:
    void acceptsReachableDesktopGeometry();
    void acceptsGeometryOnSecondaryMonitor();
    void rejectsTinyGeometry();
    void rejectsOffscreenGeometry();
    void rejectsBarelyVisibleGeometry();
};

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

QTEST_APPLESS_MAIN(TestSessionManager)

#include "TestSessionManager.moc"
