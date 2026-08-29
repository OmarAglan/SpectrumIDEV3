#pragma once

#include "QalamEditor.h"
#include "Constants.h"
#include <QList>
#include <QObject>
#include <QRect>
#include <QVector>

#include <memory>

class QalamExplorerView;
class QalamEditorWorkspace;
class FileManager;
class QSettings;

/**
 * @brief Manages session persistence — saving/restoring open tabs,
 *        folder state, user preferences, and sidebar sync.
 */
class SessionManager : public QObject {
    Q_OBJECT

public:
    struct DocumentData {
        QString filePath;
        QString displayName;
        QString recoveredContent;
        bool modified{};
        bool hasRecovery{};
    };

    struct ViewData {
        int documentIndex = -1;
        int groupIndex{};
        int tabIndex{};
        bool active{};
    };

    /// Data returned by restoreSession() for the caller to act on
    struct SessionData {
        QStringList openFiles;
        QVector<DocumentData> documents;
        QVector<ViewData> views;
        int activeTabIndex = -1;
        int activeGroupIndex{};
        Qt::Orientation splitOrientation{Qt::Horizontal};
        QList<int> splitSizes;
        QString folderPath;
        QStringList folderPaths;
        QByteArray windowGeometry;
        bool recoveredAfterInterruption{};
    };

    explicit SessionManager(QalamEditorWorkspace *editorWorkspace,
                            QObject *parent = nullptr,
                            const QString &settingsFilePath = QString());

    /// Save the current session state (open files, active tab, folder, geometry)
    void saveSession(const QString &folderPath,
                     const QByteArray &windowGeometry,
                     bool cleanShutdown = false);
    void saveSession(const QStringList &folderPaths,
                     const QByteArray &windowGeometry,
                     bool cleanShutdown = false);

    /// Load session data from settings (caller decides how to apply it)
    SessionData restoreSession() const;
    bool restoreWorkspace(const SessionData &session,
                          FileManager *fileManager);

    /// Mark the running process before edits begin so an interrupted session
    /// can be distinguished from a normal, user-confirmed shutdown.
    void markSessionRunning();

    /// Reject tiny or unreachable persisted window rectangles before restore.
    static bool isUsableWindowGeometry(
        const QRect &windowGeometry,
        const QList<QRect> &availableScreens);

    /// Save user preferences (font, theme) from the current editor state
    void savePreferences(QalamEditor *editor, int themeIndex);

    /// Sync the list of open editors to the given explorer view
    void syncOpenEditors(QalamExplorerView *explorerView);

private:
    QString recoveryDirectory() const;
    QString recoveryPath(const QString &identity) const;
    void removeUnusedRecoveryFiles(const QStringList &usedPaths) const;
    std::unique_ptr<QSettings> createSettings() const;

    QalamEditorWorkspace *m_editorWorkspace{};
    QString m_settingsFilePath;
};
