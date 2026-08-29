#include "SessionSlot.h"

#include "Constants.h"

#include <QDir>
#include <QFileInfoList>
#include <QLockFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

#include <utility>

namespace {
constexpr auto SlotVersionKey = "session/slotVersion";
constexpr auto LegacyMigrationKey = "session/slotsMigratedV1";
// QSettings uses "<file>.lock" internally while syncing INI files.  The
// lifetime lock must therefore use a different name or the process deadlocks
// against its own QSettings write for roughly thirty seconds per slot.
constexpr auto WindowLockSuffix = ".window-lock";
}

SessionSlot::SessionSlot(QString settingsFilePath,
                         std::unique_ptr<QLockFile> lockFile)
    : m_settingsFilePath(std::move(settingsFilePath))
    , m_lockFile(std::move(lockFile))
{
}

SessionSlot::~SessionSlot() = default;

QString SessionSlot::defaultSessionDirectory()
{
    const QString overridePath = qEnvironmentVariable(
        "QALAM_SESSION_DIR").trimmed();
    if (not overridePath.isEmpty()) {
        return QDir::cleanPath(
            QFileInfo(overridePath).absoluteFilePath());
    }
    return QDir(QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation))
        .filePath(QStringLiteral("sessions"));
}

std::unique_ptr<SessionSlot> SessionSlot::acquire(
    const QString &sessionDirectory,
    bool migrateLegacySession)
{
    const QString directoryPath = sessionDirectory.isEmpty()
        ? defaultSessionDirectory()
        : QDir::cleanPath(QFileInfo(sessionDirectory).absoluteFilePath());
    QDir directory(directoryPath);
    if (not directory.exists() and not QDir().mkpath(directoryPath))
        return {};

    const QFileInfoList existingSlots = directory.entryInfoList(
        {QStringLiteral("*.ini")}, QDir::Files, QDir::Time);
    for (const QFileInfo &slotInfo : existingSlots) {
        auto lock = std::make_unique<QLockFile>(
            slotInfo.absoluteFilePath()
            + QString::fromLatin1(WindowLockSuffix));
        if (not lock->tryLock(0)) continue;
        return std::unique_ptr<SessionSlot>(new SessionSlot(
            slotInfo.absoluteFilePath(), std::move(lock)));
    }

    const QString settingsFilePath = directory.filePath(
        QUuid::createUuid().toString(QUuid::WithoutBraces)
        + QStringLiteral(".ini"));
    auto lock = std::make_unique<QLockFile>(
        settingsFilePath + QString::fromLatin1(WindowLockSuffix));
    if (not lock->tryLock(0)) return {};

    QSettings slotSettings(settingsFilePath, QSettings::IniFormat);
    slotSettings.setValue(QString::fromLatin1(SlotVersionKey), 1);
    slotSettings.sync();
    if (slotSettings.status() != QSettings::NoError) return {};

    if (sessionDirectory.isEmpty() and migrateLegacySession
        and qEnvironmentVariableIsEmpty("QALAM_SESSION_DIR")) {
        migrateLegacySettings(settingsFilePath);
    }

    return std::unique_ptr<SessionSlot>(new SessionSlot(
        settingsFilePath, std::move(lock)));
}

void SessionSlot::migrateLegacySettings(const QString &settingsFilePath)
{
    QSettings legacy(Constants::OrgName, Constants::AppName);
    if (legacy.value(QString::fromLatin1(LegacyMigrationKey), false).toBool())
        return;

    QSettings slot(settingsFilePath, QSettings::IniFormat);
    for (const QString &key : legacy.allKeys()) {
        if (key.startsWith(QStringLiteral("session/")) and
            key != QString::fromLatin1(LegacyMigrationKey)) {
            slot.setValue(key, legacy.value(key));
        }
    }
    slot.setValue(QString::fromLatin1(SlotVersionKey), 1);
    slot.sync();
    if (slot.status() == QSettings::NoError) {
        legacy.setValue(QString::fromLatin1(LegacyMigrationKey), true);
        legacy.sync();
    }
}
