#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QTreeView>
#include <QFileSystemModel>
#include <QLabel>
#include <QPushButton>
#include <QEvent>
#include <QPoint>

#include "BaaDocumentSymbol.h"

class QalamSymbolOutlineView;

/**
 * @brief Explorer View - File tree with collapsible sections
 * 
 * Shows:
 * - Open Editors section (collapsible)
 * - Folder tree section (collapsible)
 */
class QalamExplorerView : public QWidget
{
    Q_OBJECT

public:
    explicit QalamExplorerView(QWidget *parent = nullptr);
    ~QalamExplorerView() = default;

    void setRootPath(const QString &path);
    QString rootPath() const { return m_rootPath; }
    
    void addOpenEditor(const QString &filePath, bool modified = false);
    void removeOpenEditor(const QString &filePath);
    void updateOpenEditor(const QString &filePath, bool modified);
    void clearOpenEditors();
    void setOutlineSymbols(
        const QString &filePath,
        const QVector<BaaDocumentSymbol> &symbols);
    void clearOutlineSymbols();

    QTreeView* treeView() const { return m_treeView; }
    QFileSystemModel* fileSystemModel() const { return m_fileSystemModel; }
    QalamSymbolOutlineView *outlineView() const { return m_outlineView; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void fileDoubleClicked(const QString &filePath);
    void openEditorClicked(const QString &filePath);
    void openEditorCloseRequested(const QString &filePath);
    void closeOtherEditorsRequested(const QString &filePath);
    void closeAllEditorsRequested();
    void openFolderRequested();  // Emitted when "Open Folder" button is clicked
    void createFileRequested(const QString &directoryPath);
    void createFolderRequested(const QString &directoryPath);
    void renameEntryRequested(const QString &entryPath);
    void deleteEntryRequested(const QString &entryPath);
    void outlineSymbolActivated(
        const QString &filePath, int line, int column);

private:
    void setupUi();
    void applyStyles();
    void showTreeContextMenu(const QPoint &position);
    void showOpenEditorContextMenu(QWidget *item, const QPoint &position);
    QWidget* createSectionHeader(const QString &title, bool expanded = true);
    
    QString m_rootPath;
    
    QVBoxLayout *m_mainLayout = nullptr;
    
    // Open Editors section
    QWidget *m_openEditorsHeader = nullptr;
    QWidget *m_openEditorsContent = nullptr;
    QVBoxLayout *m_openEditorsLayout = nullptr;
    bool m_openEditorsExpanded = true;
    
    // Folder section
    QWidget *m_folderHeader = nullptr;
    QLabel *m_folderNameLabel = nullptr;
    QTreeView *m_treeView = nullptr;
    QFileSystemModel *m_fileSystemModel = nullptr;
    bool m_folderExpanded = true;

    QWidget *m_outlineHeader{};
    QalamSymbolOutlineView *m_outlineView{};
    bool m_outlineExpanded{true};
    
    // No folder open state
    QWidget *m_noFolderWidget = nullptr;
};
