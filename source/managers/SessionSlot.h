#pragma once

#include <QString>

#include <memory>

class QLockFile;

// Owns one persistent, exclusively locked session file for a Qalam window.
// Multiple Qalam processes can therefore restore and save independently.
class SessionSlot final
{
public:
    static std::unique_ptr<SessionSlot> acquire(
        const QString &sessionDirectory = QString(),
        bool migrateLegacySession = true);

    ~SessionSlot();

    SessionSlot(const SessionSlot &) = delete;
    SessionSlot &operator=(const SessionSlot &) = delete;

    QString settingsFilePath() const { return m_settingsFilePath; }

private:
    SessionSlot(QString settingsFilePath,
                std::unique_ptr<QLockFile> lockFile);

    static QString defaultSessionDirectory();
    static void migrateLegacySettings(const QString &settingsFilePath);

    QString m_settingsFilePath;
    std::unique_ptr<QLockFile> m_lockFile;
};
