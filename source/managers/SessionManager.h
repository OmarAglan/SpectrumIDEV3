#pragma once

#include "QalamEditor.h"
#include "Constants.h"
#include <QObject>
#include <QTabWidget>

class QalamExplorerView;

/**
 * @brief Manages session persistence — saving/restoring open tabs,
 *        folder state, user preferences, and sidebar sync.
 */
class SessionManager : public QObject {
    Q_OBJECT

public:
    /// Data returned by restoreSession() for the caller to act on
    struct SessionData {
        QStringList openFiles;
        int activeTabIndex = -1;
        QString folderPath;
        QByteArray windowGeometry;
    };

    explicit SessionManager(QTabWidget *tabWidget, QObject *parent = nullptr);

    /// Save the current session state (open files, active tab, folder, geometry)
    void saveSession(const QString &folderPath, const QByteArray &windowGeometry);

    /// Load session data from settings (caller decides how to apply it)
    SessionData restoreSession() const;

    /// Save user preferences (font, theme) from the current editor state
    void savePreferences(QalamEditor *editor, int themeIndex);

    /// Sync the list of open editors to the given explorer view
    void syncOpenEditors(QalamExplorerView *explorerView);

private:
    QTabWidget *m_tabWidget{};
};
