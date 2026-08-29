#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include "QalamActivityBar.h"

class QalamExplorerView;
class QalamSearchView;

/**
 * @brief Sidebar container that shows different views based on Activity Bar selection
 * 
 * Contains a header with title and a stacked widget for different views:
 * - Explorer View (file tree)
 * - Search View (find in files)
 * - Settings will open a dialog, not shown in sidebar
 */
class QalamSidebar : public QWidget
{
    Q_OBJECT

public:
    explicit QalamSidebar(QWidget *parent = nullptr);
    ~QalamSidebar() = default;

    void setCurrentView(QalamActivityBar::ViewType view);
    QalamActivityBar::ViewType currentView() const { return m_currentView; }
    
    QalamExplorerView* explorerView() const { return m_explorerView; }
    QalamSearchView* searchView() const { return m_searchView; }

signals:
    void fileSelected(const QString &filePath);
    void searchRequested(const QString &query, bool caseSensitive, bool wholeWord, bool regex);
    void searchCancelled();
    void replaceRequested(const QString &query, const QString &replacement,
                          bool caseSensitive, bool wholeWord, bool regex);
    void openFolderRequested();  // Forward from explorer view
    void openEditorCloseRequested(const QString &filePath);
    void closeOtherEditorsRequested(const QString &filePath);
    void closeAllEditorsRequested();
    void createFileRequested(const QString &directoryPath);
    void createFolderRequested(const QString &directoryPath);
    void renameEntryRequested(const QString &entryPath);
    void deleteEntryRequested(const QString &entryPath);
    void removeRootRequested(const QString &rootPath);

private:
    void setupUi();
    void applyStyles();
    void updateHeader();
    QWidget* createPlaceholderView(const QString &title);

    QalamActivityBar::ViewType m_currentView = QalamActivityBar::ViewType::Explorer;
    
    QVBoxLayout *m_mainLayout = nullptr;
    QWidget *m_headerWidget = nullptr;
    QLabel *m_headerTitle = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    
    QalamExplorerView *m_explorerView = nullptr;
    QalamSearchView *m_searchView = nullptr;
    QWidget *m_sourceControlView = nullptr;
    QWidget *m_extensionsView = nullptr;
};
