#include "SessionSlot.h"

#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

class TestSessionSlot final : public QObject
{
    Q_OBJECT

private slots:
    void givesConcurrentWindowsIndependentPersistentSlots();
    void honoursIsolatedSessionDirectoryOverride();
};

void TestSessionSlot::givesConcurrentWindowsIndependentPersistentSlots()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    std::unique_ptr<SessionSlot> first = SessionSlot::acquire(
        directory.path(), false);
    std::unique_ptr<SessionSlot> second = SessionSlot::acquire(
        directory.path(), false);
    QVERIFY(first);
    QVERIFY(second);
    QVERIFY(first->settingsFilePath() != second->settingsFilePath());

    const QString firstPath = first->settingsFilePath();
    QSettings firstSettings(firstPath, QSettings::IniFormat);
    firstSettings.setValue(QStringLiteral("session/testIdentity"),
                           QStringLiteral("الأولى"));
    firstSettings.sync();
    first.reset();

    std::unique_ptr<SessionSlot> reused = SessionSlot::acquire(
        directory.path(), false);
    QVERIFY(reused);
    QCOMPARE(reused->settingsFilePath(), firstPath);
    QSettings reusedSettings(reused->settingsFilePath(),
                             QSettings::IniFormat);
    QCOMPARE(reusedSettings.value(QStringLiteral("session/testIdentity"))
                 .toString(),
             QStringLiteral("الأولى"));
}

void TestSessionSlot::honoursIsolatedSessionDirectoryOverride()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const bool hadOverride = qEnvironmentVariableIsSet(
        "QALAM_SESSION_DIR");
    const QByteArray previousOverride = qgetenv("QALAM_SESSION_DIR");
    QVERIFY(qputenv("QALAM_SESSION_DIR",
                    directory.path().toUtf8()));

    std::unique_ptr<SessionSlot> slot = SessionSlot::acquire(
        QString(), false);
    QVERIFY(slot);
    QCOMPARE(QFileInfo(slot->settingsFilePath()).absolutePath(),
             QFileInfo(directory.path()).absoluteFilePath());

    if (hadOverride) {
        QVERIFY(qputenv("QALAM_SESSION_DIR", previousOverride));
    } else {
        qunsetenv("QALAM_SESSION_DIR");
    }
}

QTEST_MAIN(TestSessionSlot)
#include "TestSessionSlot.moc"
