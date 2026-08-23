#include "LayoutManager.h"

#include "QalamActivityBar.h"
#include "QalamSidebar.h"
#include "QalamStatusBar.h"
#include "QalamPanelArea.h"
#include "QalamBreadcrumb.h"
#include "QalamExplorerView.h"
#include "QalamConsole.h"
#include "QalamSearchPanel.h"
#include "QalamEditor.h"
#include "Constants.h"

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStatusBar>
#include <QDir>
#include <QTabBar>

LayoutManager::LayoutManager(QMainWindow *window, QTabWidget *tabWidget,
                             QalamSearchPanel *searchBar, QObject *parent)
    : QObject(parent)
    , m_window(window)
    , m_tabWidget(tabWidget)
    , m_searchBar(searchBar)
{
}

// ===================================================================
// Build and install the full VSCode-like layout onto the main window
// ===================================================================
void LayoutManager::setupLayout()
{
    // --- Create the UI components ---
    m_activityBar = new QalamActivityBar(m_window);
    m_sidebar     = new QalamSidebar(m_window);
    m_statusBar   = new QalamStatusBar(m_window);
    m_breadcrumb  = new QalamBreadcrumb(m_window);
    m_panelArea   = new QalamPanelArea(m_window);

    // RTL-first workbench: Qalam targets Arabic-speaking developers, so the
    // Primary Side Bar and Activity Bar live on the right side. The editor text
    // and search UI remain RTL-aware, while the file paths/breadcrumbs can still
    // render naturally when they contain Latin segments.
    m_tabWidget->setLayoutDirection(Qt::RightToLeft);
    if (m_tabWidget->tabBar()) {
        m_tabWidget->tabBar()->setLayoutDirection(Qt::RightToLeft);
        m_tabWidget->tabBar()->setUsesScrollButtons(true);
    }
    m_searchBar->setLayoutDirection(Qt::RightToLeft);

    // =========================================================
    // Build the main layout - VS Code-like workbench, mirrored for RTL.
    // Editor/Panel region -> Primary Side Bar -> Activity Bar (far right).
    // =========================================================

    QWidget *centralContainer = new QWidget(m_window);
    // Keep the workbench shell itself LTR so Qt does not mirror our explicit
    // geometry order. Individual Arabic-facing widgets below still opt into RTL.
    // This fixes the broken state where the Activity Bar jumped to the far left
    // while the Primary Side Bar stayed on the right.
    centralContainer->setLayoutDirection(Qt::LeftToRight);

    QVBoxLayout *mainVLayout = new QVBoxLayout(centralContainer);
    mainVLayout->setContentsMargins(0, 0, 0, 0);
    mainVLayout->setSpacing(0);

    QSplitter *workbenchSplitter = new QSplitter(Qt::Horizontal);
    workbenchSplitter->setLayoutDirection(Qt::LeftToRight);
    workbenchSplitter->setHandleWidth(1);
    workbenchSplitter->setChildrenCollapsible(false);

    // Create editor + panel vertical splitter
    m_editorPanelSplitter = new QSplitter(Qt::Vertical);
    m_editorPanelSplitter->setHandleWidth(1);
    m_editorPanelSplitter->setChildrenCollapsible(false);

    // Create editor area container with breadcrumb
    QWidget *editorContainer = new QWidget();
    QVBoxLayout *editorVLayout = new QVBoxLayout(editorContainer);
    editorVLayout->setContentsMargins(0, 0, 0, 0);
    editorVLayout->setSpacing(0);

    // Add breadcrumb above editor tabs
    editorVLayout->addWidget(m_breadcrumb);

    // Add tabs and search bar to editor container
    editorVLayout->addWidget(m_tabWidget, 1);
    editorVLayout->addWidget(m_searchBar);

    // Add editor container and panel to splitter
    m_editorPanelSplitter->addWidget(editorContainer);
    m_editorPanelSplitter->addWidget(m_panelArea);
    m_editorPanelSplitter->setStretchFactor(0, 1);
    m_editorPanelSplitter->setStretchFactor(1, 0);
    m_editorPanelSplitter->setSizes({700, Constants::Layout::PanelDefaultHeight});

    connect(m_panelArea, &QalamPanelArea::maximizeRequested, this, [this]() {
        if (not m_editorPanelSplitter or not m_panelArea) return;

        if (not m_panelMaximized) {
            m_panelRestoreSizes = m_editorPanelSplitter->sizes();
            const int total = m_panelRestoreSizes.value(0)
                + m_panelRestoreSizes.value(1);
            const int editorHeight = qMax(Constants::Layout::PanelMinHeight,
                                          total / 5);
            m_editorPanelSplitter->setSizes({editorHeight,
                                             qMax(1, total - editorHeight)});
            m_panelMaximized = true;
        } else {
            m_editorPanelSplitter->setSizes(m_panelRestoreSizes.isEmpty()
                ? QList<int>{700, Constants::Layout::PanelDefaultHeight}
                : m_panelRestoreSizes);
            m_panelMaximized = false;
        }
        m_panelArea->setMaximizedState(m_panelMaximized);
    });

    connect(m_panelArea, &QalamPanelArea::closeRequested, this, [this]() {
        if (not m_panelMaximized or not m_editorPanelSplitter) return;
        m_editorPanelSplitter->setSizes(m_panelRestoreSizes.isEmpty()
            ? QList<int>{700, Constants::Layout::PanelDefaultHeight}
            : m_panelRestoreSizes);
        m_panelMaximized = false;
        m_panelArea->setMaximizedState(false);
    });

    // Explicit visual order for the RTL-first workbench:
    // [Editor + Panel] [Primary Side Bar] [Activity Bar]
    // The Activity Bar is now in the same splitter as the side bar, which makes
    // the right edge deterministic even when the global app direction is RTL.
    m_activityBar->setLayoutDirection(Qt::LeftToRight);
    workbenchSplitter->addWidget(m_editorPanelSplitter);
    workbenchSplitter->addWidget(m_sidebar);
    workbenchSplitter->addWidget(m_activityBar);
    workbenchSplitter->setCollapsible(0, false);
    workbenchSplitter->setCollapsible(1, false);
    workbenchSplitter->setCollapsible(2, false);
    workbenchSplitter->setStretchFactor(0, 1);
    workbenchSplitter->setStretchFactor(1, 0);
    workbenchSplitter->setStretchFactor(2, 0);
    workbenchSplitter->setSizes({1000,
                                 Constants::Layout::SidebarDefaultWidth,
                                 Constants::Layout::ActivityBarWidth});

    // Add workbench to main vertical layout.
    mainVLayout->addWidget(workbenchSplitter, 1);

    // Add status bar at bottom
    mainVLayout->addWidget(m_statusBar);

    // Set the central widget
    m_window->setCentralWidget(centralContainer);

    // --- Initial visibility ---
    m_activityBar->show();
    m_sidebar->show();       // VS Code opens with the Primary Side Bar visible.
    m_sidebar->setCurrentView(QalamActivityBar::ViewType::Explorer);
    m_statusBar->show();
    m_breadcrumb->hide();    // Hide by default to match VS Code top layout
    m_panelArea->hide();     // Start collapsed, like VS Code

    if (m_activityBar) {
        m_activityBar->setCurrentView(QalamActivityBar::ViewType::Explorer);
    }

    // --- Set initial status bar values ---
    m_statusBar->setCursorPosition(1, 1);
    m_statusBar->setEncoding("UTF-8");
    m_statusBar->setLineEnding("LF");
    m_statusBar->setLanguage("Baa");
    m_statusBar->setFolderOpen(false);

    // Hide the old QMainWindow status bar
    QStatusBar *oldStatusBar = m_window->statusBar();
    if (oldStatusBar) {
        oldStatusBar->hide();
    }

    // Initialize the panel terminal
    if (m_panelArea->terminal()) {
        m_panelArea->terminal()->setConsoleRTL();
        m_panelArea->terminal()->startCmd();
    }
}

// ===================================================================
// Toggle the console/panel area visibility
// ===================================================================
void LayoutManager::toggleConsole(QalamEditor *currentEditor)
{
    if (!m_panelArea) return;

    bool isVisible = !m_panelArea->isVisible();
    m_panelArea->setVisible(isVisible);

    if (isVisible) {
        m_panelArea->setCurrentTab(QalamPanelArea::Tab::Terminal);
        if (m_panelArea->terminal()) {
            m_panelArea->terminal()->setFocus();
        }
    } else {
        if (currentEditor) {
            currentEditor->setFocus();
        }
    }
}

// ===================================================================
// Toggle sidebar visibility
// ===================================================================
void LayoutManager::toggleSidebar()
{
    if (!m_sidebar) return;

    bool shouldBeVisible = !m_sidebar->isVisible();
    m_sidebar->setVisible(shouldBeVisible);

    // Update activity bar state
    if (m_activityBar) {
        if (shouldBeVisible) {
            m_activityBar->setCurrentView(QalamActivityBar::ViewType::Explorer);
        } else {
            m_activityBar->setCurrentView(QalamActivityBar::ViewType::None);
        }
    }
}

// ===================================================================
// Load a folder into the sidebar and update related components
// ===================================================================
void LayoutManager::loadFolder(const QString &path)
{
    if (!path.isEmpty() and QDir(path).exists()) {
        // Update sidebar with folder path and show it
        if (m_sidebar and m_sidebar->explorerView()) {
            m_sidebar->explorerView()->setRootPath(path);
            m_sidebar->setCurrentView(QalamActivityBar::ViewType::Explorer);
            m_sidebar->show();
        }

        // Sync activity bar state
        if (m_activityBar) {
            m_activityBar->setCurrentView(QalamActivityBar::ViewType::Explorer);
        }

        // Update breadcrumb project root
        if (m_breadcrumb) {
            m_breadcrumb->setProjectRoot(path);
        }

        // Update status bar
        if (m_statusBar) {
            m_statusBar->setFolderOpen(true);
        }
    } else {
        if (m_statusBar) {
            m_statusBar->setFolderOpen(false);
        }
    }
}

// ===================================================================
// Handle view changes from the activity bar
// ===================================================================
void LayoutManager::onActivityViewChanged(QalamActivityBar::ViewType view,
                                          const QString &folderPath)
{
    m_sidebar->setCurrentView(view);
    m_sidebar->show();

    // Update the sidebar root path when switching to Explorer
    if (view == QalamActivityBar::ViewType::Explorer and !folderPath.isEmpty()) {
        m_sidebar->explorerView()->setRootPath(folderPath);
    }
}
