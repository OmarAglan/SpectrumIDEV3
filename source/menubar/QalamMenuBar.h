#pragma once

#include <QMenuBar>
#include <QFileSystemModel>
#include <QTreeView>
#include <QAction>
#include <QKeySequence>



class QalamMenuBar : public QMenuBar {

	Q_OBJECT
public:
    QalamMenuBar(QWidget* parent = nullptr);

    QAction* newAction;
    QAction* openFileAction;
    QAction* openFolderAction;
    QAction* saveAction;
    QAction* saveAsAction;
    QAction* SettingsAction;
    QAction* exitAction;
    QAction* buildAction;
    QAction* runAction;
    QAction* testAction;
    QAction* cleanAction;
    QAction* aboutAction;
    QAction* commandPaletteAction;
    QAction* quickOpenAction;
    QAction* findAction;
    QAction* findInFilesAction;
    QAction* goToLineAction;
    QAction* toggleSidebarAction;
    QAction* togglePanelAction;
    QAction* problemsAction;
    QAction* debugPanelAction;
    QAction* goToDefinitionAction;
    QAction* findReferencesAction;

signals:
    void newRequested();
    void openFileRequested();
    void openFolderRequested();
    void saveRequested();
    void saveAsRequested();
    void settingsRequest();
    void exitRequested();
    void buildRequested();
    void runRequested();
    void testRequested();
    void cleanRequested();
    void aboutRequested();
    void commandPaletteRequested();
    void quickOpenRequested();
    void findRequested();
    void findInFilesRequested();
    void goToLineRequested();
    void toggleSidebarRequested();
    void togglePanelRequested();
    void problemsRequested();
    void debugPanelRequested();
    void goToDefinitionRequested();
    void findReferencesRequested();

private slots:

    void onNewAction();
    void onOpenFileAction();
    void onOpenFolderAction();
    void onSaveAction();
    void onSaveAsAction();
    void onSettingsAction();
    void onExitApp();
    void onBuildAction();
    void onRunAction();
    void onTestAction();
    void onCleanAction();
    void onAboutAction();
    void onCommandPaletteAction();
    void onQuickOpenAction();
    void onFindAction();
    void onFindInFilesAction();
    void onGoToLineAction();
    void onToggleSidebarAction();
    void onTogglePanelAction();
    void onProblemsAction();
    void onDebugPanelAction();
    void onGoToDefinitionAction();
    void onFindReferencesAction();
};
