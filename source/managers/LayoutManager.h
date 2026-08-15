#pragma once

#include "QalamActivityBar.h"
#include <QObject>

class QTabWidget;
class QMainWindow;
class SearchPanel;
class QalamSidebar;
class QalamStatusBar;
class QalamPanelArea;
class QalamBreadcrumb;
class QalamEditor;

/**
 * @brief Manages the VSCode-like layout — creates components,
 *        assembles the widget tree, and handles panel/sidebar toggling.
 */
class LayoutManager : public QObject {
    Q_OBJECT

public:
    explicit LayoutManager(QMainWindow *window, QTabWidget *tabWidget,
                           SearchPanel *searchBar, QObject *parent = nullptr);

    /// Build and install the full layout onto the main window
    void setupLayout();

    // --- Component accessors ---
    QalamActivityBar *activityBar() const { return m_activityBar; }
    QalamSidebar *sidebar() const { return m_sidebar; }
    QalamStatusBar *statusBar() const { return m_statusBar; }
    QalamPanelArea *panelArea() const { return m_panelArea; }
    QalamBreadcrumb *breadcrumb() const { return m_breadcrumb; }

    // --- Layout actions ---

    /// Toggle the console/panel area visibility
    void toggleConsole(QalamEditor *currentEditor);

    /// Toggle sidebar visibility
    void toggleSidebar();

    /// Load a folder into the sidebar and update related components
    void loadFolder(const QString &path);

    /// Handle view changes from the activity bar
    void onActivityViewChanged(QalamActivityBar::ViewType view, const QString &folderPath);

signals:
    /// Emitted when the sidebar file explorer requests opening a folder
    void openFolderRequested();

private:
    QMainWindow *m_window{};
    QTabWidget *m_tabWidget{};
    SearchPanel *m_searchBar{};

    QalamActivityBar *m_activityBar{};
    QalamSidebar *m_sidebar{};
    QalamStatusBar *m_statusBar{};
    QalamPanelArea *m_panelArea{};
    QalamBreadcrumb *m_breadcrumb{};
};
