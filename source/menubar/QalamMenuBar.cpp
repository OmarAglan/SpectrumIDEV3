#include "QalamMenuBar.h"
#include <QMenu>

QalamMenuBar::QalamMenuBar(QWidget* parent) : QMenuBar(parent) {
    setLayoutDirection(Qt::LeftToRight);

    QMenu* fileMenu = addMenu("ملف");
    QMenu* editMenu = addMenu("تحرير");
    QMenu* viewMenu = addMenu("عرض");
    QMenu* runMenu = addMenu("تشغيل");
    QMenu* terminalMenu = addMenu("الطرفية");
    QMenu* helpMenu = addMenu("مساعدة");

    fileMenu->setMinimumWidth(220);
    editMenu->setMinimumWidth(220);
    viewMenu->setMinimumWidth(240);
    runMenu->setMinimumWidth(200);
    terminalMenu->setMinimumWidth(220);
    helpMenu->setMinimumWidth(200);
    newAction = new QAction("جديد", parent);
    openFileAction = new QAction("فتح ملف", parent);
    openFolderAction = new QAction("فتح مجلد", parent);
    saveAction = new QAction("حفظ", parent);
    saveAsAction = new QAction("حفظ باسم", parent);
    SettingsAction = new QAction("الإعدادات", parent);
    exitAction = new QAction("خروج", parent);

    buildAction = new QAction("بناء المشروع أو ملف نظم", parent);
    runAction = new QAction("تشغيل", parent);
    testAction = new QAction("اختبار مشروع تكوين", parent);
    cleanAction = new QAction("تنظيف مشروع تكوين", parent);

    commandPaletteAction = new QAction("لوحة الأوامر", parent);
    quickOpenAction = new QAction("فتح سريع", parent);
    findAction = new QAction("بحث في الملف", parent);
    findInFilesAction = new QAction("بحث في الملفات", parent);
    goToLineAction = new QAction("الذهاب إلى سطر", parent);
    toggleSidebarAction = new QAction("إظهار/إخفاء الشريط الجانبي", parent);
    togglePanelAction = new QAction("إظهار/إخفاء اللوحة", parent);
    problemsAction = new QAction("المشاكل", parent);
    debugPanelAction = new QAction("لوحة التصحيح", parent);
    goToDefinitionAction = new QAction("الانتقال إلى التعريف", parent);
    findReferencesAction = new QAction("البحث عن المراجع", parent);

    newAction->setShortcut(QKeySequence("Ctrl+N"));
    openFileAction->setShortcut(QKeySequence("Ctrl+O"));
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    saveAsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    buildAction->setShortcut(QKeySequence("Ctrl+Shift+B"));
    runAction->setShortcut(QKeySequence("F5"));
    commandPaletteAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
    quickOpenAction->setShortcut(QKeySequence("Ctrl+P"));
    findAction->setShortcut(QKeySequence("Ctrl+F"));
    findInFilesAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    goToLineAction->setShortcut(QKeySequence("Ctrl+G"));
    toggleSidebarAction->setShortcut(QKeySequence("Ctrl+B"));
    togglePanelAction->setShortcut(QKeySequence("Ctrl+J"));
    problemsAction->setShortcut(QKeySequence("Ctrl+Shift+M"));
    debugPanelAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
    goToDefinitionAction->setShortcut(QKeySequence("F12"));
    findReferencesAction->setShortcut(QKeySequence("Shift+F12"));

    aboutAction = new QAction("عن المحرر", parent);


    fileMenu->addAction(newAction);
    fileMenu->addAction(openFileAction);
    fileMenu->addAction(openFolderAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(SettingsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    editMenu->addAction(findAction);
    editMenu->addAction(findInFilesAction);
    editMenu->addAction(findReferencesAction);
    editMenu->addSeparator();
    editMenu->addAction(goToLineAction);
    editMenu->addAction(goToDefinitionAction);

    viewMenu->addAction(commandPaletteAction);
    viewMenu->addAction(quickOpenAction);
    viewMenu->addSeparator();
    viewMenu->addAction(toggleSidebarAction);
    viewMenu->addAction(togglePanelAction);
    viewMenu->addAction(problemsAction);
    viewMenu->addAction(debugPanelAction);

    runMenu->addAction(buildAction);
    runMenu->addAction(runAction);
    runMenu->addAction(testAction);
    runMenu->addSeparator();
    runMenu->addAction(cleanAction);

    terminalMenu->addAction(togglePanelAction);

    helpMenu->addAction(aboutAction);


    connect(newAction, &QAction::triggered, this, &QalamMenuBar::onNewAction);
    connect(openFileAction, &QAction::triggered, this, &QalamMenuBar::onOpenFileAction);
    connect(openFolderAction, &QAction::triggered, this, &QalamMenuBar::onOpenFolderAction);
    connect(saveAction, &QAction::triggered, this, &QalamMenuBar::onSaveAction);
    connect(saveAsAction, &QAction::triggered, this, &QalamMenuBar::onSaveAsAction);
    connect(SettingsAction, &QAction::triggered, this, &QalamMenuBar::onSettingsAction);
    connect(exitAction, &QAction::triggered, this, &QalamMenuBar::onExitApp);

    connect(buildAction, &QAction::triggered, this, &QalamMenuBar::onBuildAction);
    connect(runAction, &QAction::triggered, this, &QalamMenuBar::onRunAction);
    connect(testAction, &QAction::triggered, this, &QalamMenuBar::onTestAction);
    connect(cleanAction, &QAction::triggered, this, &QalamMenuBar::onCleanAction);
    connect(commandPaletteAction, &QAction::triggered, this, &QalamMenuBar::onCommandPaletteAction);
    connect(quickOpenAction, &QAction::triggered, this, &QalamMenuBar::onQuickOpenAction);
    connect(findAction, &QAction::triggered, this, &QalamMenuBar::onFindAction);
    connect(findInFilesAction, &QAction::triggered, this, &QalamMenuBar::onFindInFilesAction);
    connect(goToLineAction, &QAction::triggered, this, &QalamMenuBar::onGoToLineAction);
    connect(toggleSidebarAction, &QAction::triggered, this, &QalamMenuBar::onToggleSidebarAction);
    connect(togglePanelAction, &QAction::triggered, this, &QalamMenuBar::onTogglePanelAction);
    connect(problemsAction, &QAction::triggered, this, &QalamMenuBar::onProblemsAction);
    connect(debugPanelAction, &QAction::triggered, this, &QalamMenuBar::onDebugPanelAction);
    connect(goToDefinitionAction, &QAction::triggered, this, &QalamMenuBar::onGoToDefinitionAction);
    connect(findReferencesAction, &QAction::triggered, this, &QalamMenuBar::onFindReferencesAction);

    connect(aboutAction, &QAction::triggered, this, &QalamMenuBar::onAboutAction);
}

void QalamMenuBar::onNewAction() { emit newRequested(); }
void QalamMenuBar::onOpenFileAction() { emit openFileRequested(); }
void QalamMenuBar::onOpenFolderAction() { emit openFolderRequested(); }
void QalamMenuBar::onSaveAction() { emit saveRequested(); }
void QalamMenuBar::onSaveAsAction() { emit saveAsRequested(); }
void QalamMenuBar::onSettingsAction() { emit settingsRequest(); }
void QalamMenuBar::onExitApp() { emit exitRequested(); }
void QalamMenuBar::onBuildAction() { emit buildRequested(); }
void QalamMenuBar::onRunAction() { emit runRequested(); }
void QalamMenuBar::onTestAction() { emit testRequested(); }
void QalamMenuBar::onCleanAction() { emit cleanRequested(); }
void QalamMenuBar::onAboutAction() { emit aboutRequested(); }


void QalamMenuBar::onCommandPaletteAction() { emit commandPaletteRequested(); }
void QalamMenuBar::onQuickOpenAction() { emit quickOpenRequested(); }
void QalamMenuBar::onFindAction() { emit findRequested(); }
void QalamMenuBar::onFindInFilesAction() { emit findInFilesRequested(); }
void QalamMenuBar::onGoToLineAction() { emit goToLineRequested(); }
void QalamMenuBar::onToggleSidebarAction() { emit toggleSidebarRequested(); }
void QalamMenuBar::onTogglePanelAction() { emit togglePanelRequested(); }
void QalamMenuBar::onProblemsAction() { emit problemsRequested(); }

void QalamMenuBar::onDebugPanelAction() { emit debugPanelRequested(); }
void QalamMenuBar::onGoToDefinitionAction() { emit goToDefinitionRequested(); }
void QalamMenuBar::onFindReferencesAction() { emit findReferencesRequested(); }
