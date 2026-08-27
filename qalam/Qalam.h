#pragma once

#include "QalamEditor.h"
#include "QalamMenuBar.h"
#include "QalamSearchPanel.h"
#include "FileManager.h"
#include "BuildManager.h"
#include "SessionManager.h"
#include "LayoutManager.h"
#include "../ui/QalamWindow.h"

#include "QalamActivityBar.h"
#include <QPointer>
#include <QPoint>
#include <QStringList>
#include <QVector>

class BreakpointModel;
class BaaLanguageClient;
struct BaaWorkspaceEdit;
class CommandRegistry;
class DiagnosticsModel;
class QalamWelcomePage;
class QalamEditorWorkspace;
class QalamCommandPalette;
class ProjectSearchService;
class QTimer;
class WorkspaceIndexer;
struct BaaLocation;
struct Diagnostic;
struct ProjectReplacementPlan;
struct ProjectSearchRequest;

class Qalam : public QalamWindow
{
    Q_OBJECT

public:
    Qalam(const QString &filePath = "", QWidget *parent = nullptr);
    ~Qalam();
    void loadFolder(const QString &folderPath);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private slots:
    void handleOpenFolderMenu();
    void reopenLastProject();
    void openSettings();
    void exitApp();

    void newFileFromUi();
    void openFileFromUi(const QString &filePathOrEmpty = QString());
    void openRecentPath(const QString &path);

    void runBaa();
    void buildTakweenProject();
    void testTakweenProject();
    void cleanTakweenProject();
    void aboutQalam();

    void updateWindowTitle();
    void closeTab(int index);
    void closeAllTabs();
    void toggleSidebar();

    void toggleConsole();

    void updateCursorPosition();
    void onCurrentTabChanged();

    void showFindBar();
    void hideFindBar();

    void goToLine();
    void showCommandPalette();
    void showQuickOpen();
    void showWorkspaceSymbols();
    void focusSearchInFiles();
    void openProblemsPanel();
    void openDebugPanel();
    void handleBuildOutput(const QString &text);
    void goToDefinition();
    void findReferences();
    void renameSymbol();
    void quickFix();
    void formatDocument();
    bool applyWorkspaceEdit(const BaaWorkspaceEdit &edit,
                            QString *error = nullptr);
    
    // VSCode-like component slots
    void onActivityViewChanged(QalamActivityBar::ViewType view);
    void onSidebarFileSelected(const QString &filePath);
    void createWorkspaceFile(const QString &directoryPath);
    void createWorkspaceFolder(const QString &directoryPath);
    void renameWorkspaceEntry(const QString &entryPath);
    void deleteWorkspaceEntry(const QString &entryPath);
    void closeOtherEditorsByPath(const QString &filePath);

private:
    QalamEditor *currentEditor();
    bool shouldShowWelcome() const;
    bool hasAnyEditorTabs() const;
    void showWelcomeTab();
    void removeWelcomeTabIfPresent();

    void connectSignals();
    void syncOpenEditors();
    bool maybeSaveAllModified();
    void goToLocation(const QString &filePath, int line, int column);
    void performProjectSearch(const QString &query, bool caseSensitive, bool wholeWord, bool regex);
    void performProjectReplace(const QString &query, const QString &replacement,
                               bool caseSensitive, bool wholeWord, bool regex);
    ProjectSearchRequest projectSearchRequest(
        const QString &query, const QString &replacement,
        bool caseSensitive, bool wholeWord, bool regex) const;
    bool applyProjectReplacement(const ProjectReplacementPlan &plan,
                                 QString *error = nullptr);
    void closeEditorByPath(const QString &filePath);
    bool closeTabAt(int index, bool ensureReplacement);
    void closeOtherTabsExcept(int index);
    void showTabContextMenu(const QPoint &position);
    void removeEditorTabWithoutPrompt(QalamEditor *editor);
    void ensureEditorSurface();
    QVector<QalamEditor*> editorsForWorkspaceEntry(
        const QString &entryPath, bool directory) const;
    void refreshWorkspaceAfterFileOperation();
    QStringList collectProjectFiles() const;
    bool runCommandById(const QString &commandId);
    void updateProblemsStatusBar();
    void rebuildProblemsPanel();
    void applyDiagnosticsToEditors();
    void attachAnalysisToEditor(QalamEditor *editor);
    void scheduleEditorAnalysis(QalamEditor *editor);
    void handleLanguageDiagnostics(const QString &filePath,
                                   int documentVersion,
                                   const QVector<Diagnostic> &diagnostics);
    void handleLanguageCompletion(const QString &filePath,
                                  int documentVersion,
                                  int line,
                                  int character,
                                  const QVector<BaaCompletionItem> &items);
    bool languageDocumentVersionIsCurrent(const QString &filePath,
                                          int documentVersion) const;
    QString lineTextForLocation(const BaaLocation &location) const;
    void runTakweenProjectCommand(const QString &command);
    void refreshToolActions();
    void scheduleSessionSave();
    void saveSessionCheckpoint();
    void openFolderFromPath(const QString &path);
    bool ensureToolOperationAvailable(const QString &operation,
                                      const QString &filePath);

private:
    QalamEditorWorkspace *tabWidget{};
    QalamMenuBar *menuBar{};
    QalamSettings *setting{};
    QString folderPath{};

    FileManager *m_fileManager{};
    BuildManager *m_buildManager{};
    BaaLanguageClient *m_languageClient{};
    SessionManager *m_sessionManager{};
    LayoutManager *m_layoutManager{};
    CommandRegistry *m_commandRegistry{};
    DiagnosticsModel *m_diagnosticsModel{};
    WorkspaceIndexer *m_workspaceIndexer{};
    ProjectSearchService *m_projectSearchService{};
    BreakpointModel *m_breakpointModel{};

    QalamSearchPanel *searchBar{};
    QalamWelcomePage *m_welcomePage{};
    QPointer<QalamCommandPalette> m_workspaceSymbolPalette;
    QalamEditor *m_lastConnectedEditor{}; // Track editor for cursor position disconnect
    QTimer *m_sessionSaveTimer{};
    bool m_sessionSavedCleanly{};
};
