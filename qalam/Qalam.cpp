#include "Qalam.h"
#include "QalamWelcomePage.h"
#include "QalamConsole.h"
#include "QalamSearchPanel.h"
#include "Constants.h"

// VSCode-like UI components (needed to call methods on LayoutManager accessors)
#include "QalamActivityBar.h"
#include "QalamSidebar.h"
#include "QalamStatusBar.h"
#include "QalamPanelArea.h"
#include "QalamBreadcrumb.h"
#include "QalamExplorerView.h"

#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QShortcut>
#include <QGuiApplication>
#include <QScreen>
#include <QCoreApplication>
#include <QApplication>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>
#include <QStringConverter>
#include <QDirIterator>
#include <QRegularExpression>
#include <QKeyEvent>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QVector>
#include <QVariant>
#include <QTextBlock>
#include <algorithm>
#include "QalamSearchView.h"
#include "CommandRegistry.h"
#include "DiagnosticParser.h"
#include "DiagnosticsModel.h"
#include "BaaLanguageClient.h"
#include "WorkspaceIndexer.h"
#include "BreakpointModel.h"
#include "QalamCommandPalette.h"

Qalam::Qalam(const QString& filePath, QWidget *parent)
    : QalamWindow(parent)
{

    setAttribute(Qt::WA_DeleteOnClose);

    // ===================================================================
    // الخطوة 1: إنشاء المكونات الرئيسية
    // ===================================================================
    tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("MainTabs");
    tabWidget->setDocumentMode(true);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    menuBar = new QalamMenuBar(this);
    m_fileManager = new FileManager(tabWidget, this, this);
    m_buildManager = new BuildManager(this);
    m_languageClient = new BaaLanguageClient(this);
    m_languageClient->setTakweenProgram(BuildManager::resolveTakweenProgram());
    m_sessionManager = new SessionManager(tabWidget, this);
    m_commandRegistry = new CommandRegistry(this);
    for (const auto &command : CommandRegistry::defaultCommands()) {
        m_commandRegistry->registerCommand(command);
    }
    m_diagnosticsModel = new DiagnosticsModel(this);
    m_workspaceIndexer = new WorkspaceIndexer(this);
    m_breakpointModel = new BreakpointModel(this);

    searchBar = new SearchPanel(this);
    searchBar->hide();

    m_layoutManager = new LayoutManager(this, tabWidget, searchBar, this);

    // ===================================================================
    // الخطوة 2: إعداد النافذة وشريط القوائم
    // ===================================================================
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeo = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 800);
    const int margin = 100;
    const int widthFixedNum = 6;
    const int width = qMax(900, screenGeo.size().width() - margin * widthFixedNum);
    const int height = qMax(600, screenGeo.size().height() - margin);
    const int x = screenGeo.left() + qMax(0, (screenGeo.width() - width) / 2);
    const int y = screenGeo.top() + qMax(0, (screenGeo.height() - height) / 2);
    this->setGeometry(x, y, width, height);
    this->setCustomMenuBar(menuBar);

    // ===================================================================
    // الخطوة 3: إعداد الإعدادات
    // ===================================================================
    setting = new QalamSettings(this);

    // ===================================================================
    // الخطوة 4: إعداد التخطيط الجديد (VSCode-like)
    // ===================================================================
    m_layoutManager->setupLayout();

    // ===================================================================
    // الخطوة 5: ربط الإشارات والمقابس
    // ===================================================================
    connectSignals();
    onCurrentTabChanged();
    syncOpenEditors();
    
    // ===================================================================
    // الخطوة 6: تحميل الملف المبدئي أو استعادة الجلسة السابقة
    // ===================================================================
    installEventFilter(this);

    if (!filePath.isEmpty()) {
        // Explicit file passed (e.g. command-line argument or double-click)
        m_fileManager->openFile(filePath);
    } else {
        // Try to restore previous session
        auto session = m_sessionManager->restoreSession();

        // Restore window geometry
        if (not session.windowGeometry.isEmpty()) {
            restoreGeometry(session.windowGeometry);
        }

        // Restore folder
        if (not session.folderPath.isEmpty()) {
            loadFolder(session.folderPath);
        }

        // Restore open files
        bool restoredAny = false;
        for (const QString &file : session.openFiles) {
            if (QFile::exists(file)) {
                m_fileManager->openFile(file);
                restoredAny = true;
            }
        }

        // Restore active tab
        if (restoredAny and session.activeTabIndex >= 0
            and session.activeTabIndex < tabWidget->count()) {
            tabWidget->setCurrentIndex(session.activeTabIndex);
        }

        // If nothing was restored, show welcome or create a new empty tab
        if (not restoredAny) {
            if (shouldShowWelcome()) {
                showWelcomeTab();
            } else {
                m_fileManager->newFile();
            }
        }
    }
}

Qalam::~Qalam() {
    // Save session state
    m_sessionManager->saveSession(folderPath, saveGeometry());

    // Save user preferences
    m_sessionManager->savePreferences(currentEditor(), setting->getThemeCombo()->currentIndex());
}

// ===================================================================
// ربط جميع الإشارات والمقابس في مكان واحد
// ===================================================================
void Qalam::connectSignals()
{
    // --- Keyboard shortcuts ---
    // Global workbench shortcuts are owned by QalamMenuBar QActions so they show
    // in the menus and avoid duplicate Qt shortcut ambiguity. Editor-only
    // shortcuts stay here.

    auto *commentShortcut = new QShortcut(QKeySequence("Ctrl+/"), this);
    connect(commentShortcut, &QShortcut::activated, this, [this](){
        if (QalamEditor* editor = currentEditor()) editor->toggleComment();
    });

    auto *duplicateShortcut = new QShortcut(QKeySequence("Ctrl+D"), this);
    connect(duplicateShortcut, &QShortcut::activated, this, [this](){
        if (QalamEditor* editor = currentEditor()) editor->duplicateLine();
    });

    auto *moveUpShortcut = new QShortcut(QKeySequence("Alt+Up"), this);
    connect(moveUpShortcut, &QShortcut::activated, this, [this](){
        if (QalamEditor* editor = currentEditor()) editor->moveLineUp();
    });

    auto *moveDownShortcut = new QShortcut(QKeySequence("Alt+Down"), this);
    connect(moveDownShortcut, &QShortcut::activated, this, [this](){
        if (QalamEditor* editor = currentEditor()) editor->moveLineDown();
    });

    auto *stopToolingShortcut = new QShortcut(QKeySequence("Shift+F5"), this);
    connect(stopToolingShortcut, &QShortcut::activated, this, [this]() {
        if (m_buildManager and m_buildManager->isRunning()) m_buildManager->stop();
    });

    auto *renameShortcut = new QShortcut(QKeySequence("F2"), this);
    connect(renameShortcut, &QShortcut::activated,
            this, &Qalam::renameSymbol);

    auto *quickFixShortcut = new QShortcut(QKeySequence("Ctrl+."), this);
    connect(quickFixShortcut, &QShortcut::activated,
            this, &Qalam::quickFix);

    auto *formatShortcut = new QShortcut(
        QKeySequence("Shift+Alt+F"), this);
    connect(formatShortcut, &QShortcut::activated,
            this, &Qalam::formatDocument);

    auto *workspaceSymbolsShortcut =
        new QShortcut(QKeySequence("Ctrl+T"), this);
    connect(workspaceSymbolsShortcut, &QShortcut::activated,
            this, &Qalam::showWorkspaceSymbols);

    // --- Menu bar signals ---
    connect(menuBar, &QalamMenuBar::newRequested, this, &Qalam::newFileFromUi);
    connect(menuBar, &QalamMenuBar::openFileRequested, this, [this]() { openFileFromUi(QString()); });
    connect(menuBar, &QalamMenuBar::saveRequested, m_fileManager, &FileManager::saveFile);
    connect(menuBar, &QalamMenuBar::saveAsRequested, m_fileManager, &FileManager::saveFileAs);
    connect(menuBar, &QalamMenuBar::settingsRequest, this, &Qalam::openSettings);
    connect(menuBar, &QalamMenuBar::exitRequested, this, &Qalam::exitApp);
    connect(menuBar, &QalamMenuBar::buildRequested, this, &Qalam::buildTakweenProject);
    connect(menuBar, &QalamMenuBar::runRequested, this, &Qalam::runBaa);
    connect(menuBar, &QalamMenuBar::testRequested, this, &Qalam::testTakweenProject);
    connect(menuBar, &QalamMenuBar::cleanRequested, this, &Qalam::cleanTakweenProject);
    connect(menuBar, &QalamMenuBar::aboutRequested, this, &Qalam::aboutQalam);
    connect(menuBar, &QalamMenuBar::openFolderRequested, this, &Qalam::handleOpenFolderMenu);
    connect(menuBar, &QalamMenuBar::commandPaletteRequested, this, &Qalam::showCommandPalette);
    connect(menuBar, &QalamMenuBar::quickOpenRequested, this, &Qalam::showQuickOpen);
    connect(menuBar, &QalamMenuBar::findRequested, this, &Qalam::showFindBar);
    connect(menuBar, &QalamMenuBar::findInFilesRequested, this, &Qalam::focusSearchInFiles);
    connect(menuBar, &QalamMenuBar::goToLineRequested, this, &Qalam::goToLine);
    connect(menuBar, &QalamMenuBar::toggleSidebarRequested, this, &Qalam::toggleSidebar);
    connect(menuBar, &QalamMenuBar::togglePanelRequested, this, &Qalam::toggleConsole);
    connect(menuBar, &QalamMenuBar::problemsRequested, this, &Qalam::openProblemsPanel);
    connect(menuBar, &QalamMenuBar::debugPanelRequested, this, &Qalam::openDebugPanel);
    connect(menuBar, &QalamMenuBar::goToDefinitionRequested, this, &Qalam::goToDefinition);
    connect(menuBar, &QalamMenuBar::findReferencesRequested, this, &Qalam::findReferences);
    connect(this, &QalamWindow::commandCenterClicked, this, &Qalam::showCommandPalette);
    if (m_layoutManager and m_layoutManager->sidebar() and
        m_layoutManager->sidebar()->explorerView()) {
        connect(
            m_layoutManager->sidebar()->explorerView(),
            &QalamExplorerView::outlineSymbolActivated,
            this,
            [this](const QString &filePath, int line, int column) {
                goToLocation(filePath, line, column);
            });
    }

    connect(m_buildManager, &BuildManager::outputChunk, this, &Qalam::handleBuildOutput);
    connect(m_languageClient, &BaaLanguageClient::diagnosticsPublished,
            this, &Qalam::handleLanguageDiagnostics);
    connect(m_languageClient, &BaaLanguageClient::semanticTokensPublished,
            this, [this](const QString &filePath, int documentVersion,
                         const QVector<BaaSemanticToken> &tokens) {
        for (int index = 0; index < tabWidget->count(); ++index) {
            QalamEditor *editor =
                qobject_cast<QalamEditor*>(tabWidget->widget(index));
            if (not editor or
                editor->property("qalam.lsp.version").toInt() !=
                    documentVersion)
                continue;
            const QString editorPath = QDir::cleanPath(
                QFileInfo(editor->currentFilePath()).absoluteFilePath());
            if (editorPath == QDir::cleanPath(filePath)) {
                editor->setSemanticTokens(tokens);
                return;
            }
        }
    });
    connect(m_languageClient, &BaaLanguageClient::foldingRangesPublished,
            this, [this](const QString &filePath, int documentVersion,
                         const QVector<BaaFoldingRange> &ranges) {
        for (int index = 0; index < tabWidget->count(); ++index) {
            QalamEditor *editor =
                qobject_cast<QalamEditor*>(tabWidget->widget(index));
            if (not editor or
                editor->property("qalam.lsp.version").toInt() !=
                    documentVersion)
                continue;
            const QString editorPath = QDir::cleanPath(
                QFileInfo(editor->currentFilePath()).absoluteFilePath());
            if (editorPath == QDir::cleanPath(filePath)) {
                editor->setFoldingRanges(ranges);
                return;
            }
        }
    });
    connect(m_languageClient, &BaaLanguageClient::inlayHintsPublished,
            this, [this](const QString &filePath, int documentVersion,
                         const QVector<BaaInlayHint> &hints) {
        for (int index = 0; index < tabWidget->count(); ++index) {
            QalamEditor *editor =
                qobject_cast<QalamEditor*>(tabWidget->widget(index));
            if (not editor or
                editor->property("qalam.lsp.version").toInt() !=
                    documentVersion)
                continue;
            const QString editorPath = QDir::cleanPath(
                QFileInfo(editor->currentFilePath()).absoluteFilePath());
            if (editorPath == QDir::cleanPath(filePath)) {
                editor->setInlayHints(hints);
                return;
            }
        }
    });
    connect(m_languageClient, &BaaLanguageClient::selectionRangesPublished,
            this, [this](const QString &filePath, int documentVersion,
                         int line, int character,
                         const QVector<BaaSelectionRange> &ranges) {
        QalamEditor *editor = currentEditor();
        if (not editor or
            editor->property("qalam.lsp.version").toInt() != documentVersion)
            return;
        const QString editorPath = QDir::cleanPath(
            QFileInfo(editor->currentFilePath()).absoluteFilePath());
        if (editorPath != QDir::cleanPath(filePath)) return;
        editor->applySemanticSelectionRanges(ranges, line, character);
    });
    connect(m_languageClient, &BaaLanguageClient::completionPublished,
            this, &Qalam::handleLanguageCompletion);
    connect(m_languageClient, &BaaLanguageClient::hoverPublished,
            this, [this](const QString &filePath, int documentVersion,
                         int line, int character, const BaaHover &hover) {
        QalamEditor *editor = currentEditor();
        if (not editor ||
            editor->property("qalam.lsp.version").toInt() != documentVersion)
            return;
        const QString editorPath = QDir::cleanPath(
            QFileInfo(editor->currentFilePath()).absoluteFilePath());
        if (editorPath != QDir::cleanPath(filePath)) return;
        editor->showLanguageHover(hover, line, character);
    });
    connect(m_languageClient, &BaaLanguageClient::signatureHelpPublished,
            this, [this](const QString &filePath, int documentVersion,
                         int line, int character,
                         const BaaSignatureHelp &signatureHelp) {
        QalamEditor *editor = currentEditor();
        if (not editor ||
            editor->property("qalam.lsp.version").toInt() != documentVersion)
            return;
        const QString editorPath = QDir::cleanPath(
            QFileInfo(editor->currentFilePath()).absoluteFilePath());
        if (editorPath != QDir::cleanPath(filePath)) return;
        editor->showSignatureHelp(signatureHelp, line, character);
    });
    connect(m_languageClient, &BaaLanguageClient::definitionPublished,
            this, [this](const QString &filePath, int documentVersion,
                         int, int, const BaaLocation &definition) {
        if (not languageDocumentVersionIsCurrent(filePath, documentVersion)) return;
        if (definition.isValid()) {
            goToLocation(definition.filePath,
                         definition.line + 1,
                         definition.character + 1);
        } else if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral("لم يتم العثور على تعريف دلالي"), 3000);
        }
    });
    connect(m_languageClient, &BaaLanguageClient::referencesPublished,
            this, [this](const QString &filePath, int documentVersion,
                         int, int, const QVector<BaaLocation> &references) {
        if (not languageDocumentVersionIsCurrent(filePath, documentVersion)) return;
        focusSearchInFiles();
        auto *sidebar = m_layoutManager ? m_layoutManager->sidebar() : nullptr;
        auto *searchView = sidebar ? sidebar->searchView() : nullptr;
        if (not searchView) return;

        searchView->clearResults();
        QSet<QString> files;
        for (const BaaLocation &location : references) {
            if (not location.isValid()) continue;
            files.insert(QDir::cleanPath(location.filePath));
            const QString lineText = lineTextForLocation(location);
            const int length = location.line == location.endLine
                ? qMax(0, location.endCharacter - location.character) : 0;
            searchView->addResult(
                location.filePath, location.line + 1, location.character + 1,
                lineText, length > 0
                    ? lineText.mid(location.character, length) : QString());
        }
        searchView->setResultCount(files.size(), references.size());
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral("تم العثور على %1 مرجعاً دلالياً")
                    .arg(references.size()), 3000);
        }
    });
    connect(m_languageClient, &BaaLanguageClient::codeActionsPublished,
            this, [this](const QString &filePath, int documentVersion,
                         int, int, const QVector<BaaCodeAction> &actions) {
        if (not languageDocumentVersionIsCurrent(filePath, documentVersion))
            return;
        if (actions.isEmpty()) {
            if (m_layoutManager and m_layoutManager->statusBar()) {
                m_layoutManager->statusBar()->showMessage(
                    QStringLiteral("لا يوجد إصلاح آمن في هذا الموضع"), 3000);
            }
            return;
        }

        int selectedIndex = 0;
        if (actions.size() > 1) {
            QStringList titles;
            titles.reserve(actions.size());
            for (const BaaCodeAction &action : actions)
                titles.push_back(action.title);
            bool accepted = false;
            const QString selected = QInputDialog::getItem(
                this,
                QStringLiteral("إصلاحات باء الآمنة"),
                QStringLiteral("اختر الإصلاح المراد معاينته:"),
                titles,
                0,
                false,
                &accepted);
            if (not accepted) return;
            selectedIndex = titles.indexOf(selected);
            if (selectedIndex < 0) return;
        }

        const BaaCodeAction &action = actions.at(selectedIndex);
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            QStringLiteral("معاينة الإصلاح"),
            QStringLiteral(
                "%1\n\nسيطبق قلم %2 تعديلاً في %3 ملفاً بعد التحقق من "
                "إصدار المستند. هل تريد المتابعة؟")
                .arg(action.title)
                .arg(action.edit.editCount())
                .arg(action.edit.documents.size()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (answer != QMessageBox::Yes) return;

        QString error;
        if (not applyWorkspaceEdit(action.edit, &error)) {
            QMessageBox::warning(
                this, QStringLiteral("تعذر تطبيق الإصلاح"), error);
            return;
        }
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral("طُبق إصلاح باء الآمن: %1")
                    .arg(action.title), 4000);
        }
    });
    connect(m_languageClient, &BaaLanguageClient::formattingPublished,
            this, [this](const QString &filePath, int documentVersion,
                         const BaaWorkspaceEdit &edit) {
        if (not languageDocumentVersionIsCurrent(filePath, documentVersion))
            return;
        if (not edit.isValid()) {
            if (m_layoutManager and m_layoutManager->statusBar()) {
                m_layoutManager->statusBar()->showMessage(
                    QStringLiteral("المستند منسق بالفعل"), 3000);
            }
            return;
        }

        QString error;
        if (not applyWorkspaceEdit(edit, &error)) {
            QMessageBox::warning(
                this, QStringLiteral("تعذر تنسيق المستند"), error);
            return;
        }
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral(
                    "نُسق المستند وفق النمط الرسمي للغة باء"),
                4000);
        }
    });
    connect(m_languageClient, &BaaLanguageClient::renamePrepared,
            this, [this](const QString &filePath, int documentVersion,
                         int line, int character, const QString &placeholder,
                         const BaaLocation &) {
        if (not languageDocumentVersionIsCurrent(filePath, documentVersion))
            return;
        bool accepted = false;
        const QString newName = QInputDialog::getText(
            this,
            QStringLiteral("إعادة تسمية رمز باء"),
            QStringLiteral("الاسم العربي الجديد:"),
            QLineEdit::Normal,
            placeholder,
            &accepted).trimmed();
        if (not accepted or newName.isEmpty() or newName == placeholder)
            return;
        m_languageClient->requestRename(
            filePath, line, character, newName);
    });
    connect(m_languageClient, &BaaLanguageClient::renameEditPublished,
            this, [this](const QString &filePath, int documentVersion,
                         int, int, const BaaWorkspaceEdit &edit) {
        if (not languageDocumentVersionIsCurrent(filePath, documentVersion))
            return;
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            QStringLiteral("معاينة إعادة التسمية"),
            QStringLiteral(
                "سيتغير %1 موضعاً في %2 ملفاً.\n"
                "تحقق Baa-LSP من هوية الرمز والتعارضات. هل تريد المتابعة؟")
                .arg(edit.editCount())
                .arg(edit.documents.size()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (answer != QMessageBox::Yes) return;

        QString error;
        if (not applyWorkspaceEdit(edit, &error)) {
            QMessageBox::warning(
                this, QStringLiteral("تعذر تطبيق إعادة التسمية"), error);
            return;
        }
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral("اكتملت إعادة تسمية %1 موضعاً بأمان")
                    .arg(edit.editCount()), 4000);
        }
    });
    connect(m_languageClient, &BaaLanguageClient::renameFailed,
            this, [this](const QString &, const QString &message) {
        if (not message.isEmpty())
            QMessageBox::warning(
                this, QStringLiteral("تعذر إعادة التسمية"), message);
    });
    connect(m_languageClient, &BaaLanguageClient::documentSymbolsPublished,
            this, [this](const QString &filePath, int documentVersion,
                         const QVector<BaaDocumentSymbol> &symbols) {
        QalamEditor *editor = currentEditor();
        if (not editor or
            editor->property("qalam.lsp.version").toInt() != documentVersion) return;
        const QString editorPath = QDir::cleanPath(
            QFileInfo(editor->currentFilePath()).absoluteFilePath());
        if (editorPath != QDir::cleanPath(filePath)) return;
        if (m_layoutManager and m_layoutManager->sidebar() and
            m_layoutManager->sidebar()->explorerView()) {
            m_layoutManager->sidebar()->explorerView()->setOutlineSymbols(
                filePath, symbols);
        }
        if (editor->hasVisibleCompletion()) {
            const QTextCursor cursor = editor->textCursor();
            m_languageClient->requestCompletion(
                filePath,
                cursor.blockNumber(),
                cursor.positionInBlock());
        }
    });
    connect(m_languageClient, &BaaLanguageClient::logMessage,
            this, [this](const QString &message, int) {
                if (m_layoutManager and m_layoutManager->statusBar() and !message.isEmpty()) {
                    m_layoutManager->statusBar()->showMessage(message, 3500);
                }
            });
    connect(m_languageClient, &BaaLanguageClient::structuredLogReceived,
            this, [this](const BaaLogEvent &event) {
                if (m_layoutManager and m_layoutManager->panelArea()) {
                    m_layoutManager->panelArea()->appendOutput(
                        event.formattedLine() + QLatin1Char('\n'));
                }
                if (event.lspType() <= 2 and m_layoutManager and
                    m_layoutManager->statusBar()) {
                    m_layoutManager->statusBar()->showMessage(
                        event.arabicSummary(), 5000);
                }
            });
    connect(m_buildManager, &BuildManager::toolingFinished, this,
            [this](const QString &operation, int exitCode) {
        if (exitCode != 0 and exitCode != -2 and
            m_diagnosticsModel and m_diagnosticsModel->count() == 0) {
            QString filePath;
            if (QalamEditor *editor = currentEditor()) {
                filePath = editor->currentFilePath();
            }
            QVector<Diagnostic> runnerDiagnostics;
            Diagnostic diagnostic;
            diagnostic.file = filePath;
            diagnostic.severity = "error";
            diagnostic.code = BuildManager::compilerExitCodeId(exitCode);
            diagnostic.category = "tooling";
            diagnostic.message = BuildManager::compilerExitSummary(exitCode, operation);
            diagnostic.source = operation == "check" ? "compiler-cli-v1" : "tooling-exit";
            runnerDiagnostics.push_back(diagnostic);
            m_diagnosticsModel->addDiagnostics(runnerDiagnostics);
        }
        updateProblemsStatusBar();
    });
    connect(m_buildManager, &BuildManager::toolingProgress, this, [this](const QString &text) {
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(text, 2500);
        }
    });

    connect(m_diagnosticsModel, &DiagnosticsModel::diagnosticsChanged, this, [this]() {
        rebuildProblemsPanel();
        applyDiagnosticsToEditors();
        updateProblemsStatusBar();
    });

    // --- Tab widget signals ---
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &Qalam::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &Qalam::updateWindowTitle);
    connect(tabWidget, &QTabWidget::currentChanged, this, &Qalam::onCurrentTabChanged);

    // --- Search bar signals ---
    // Search logic is now handled internally by SearchPanel.
    // We only need to connect the close signal and sync the active editor.
    connect(searchBar, &SearchPanel::closed, this, &Qalam::hideFindBar);

    // --- FileManager signals ---
    connect(m_fileManager, &FileManager::fileStateChanged, this, &Qalam::updateWindowTitle);
    connect(m_fileManager, &FileManager::fileStateChanged, this, [this]() {
        // Update modification indicators for every editor, not only the active tab.
        for (int index = 0; index < tabWidget->count(); ++index) {
            QalamEditor *editor = qobject_cast<QalamEditor*>(tabWidget->widget(index));
            if (!editor) continue;

            const bool modified = editor->document()->isModified();
            QString tabText = tabWidget->tabText(index);
            if (modified and not tabText.endsWith("[*]")) {
                tabWidget->setTabText(index, tabText + "[*]");
            } else if (not modified and tabText.endsWith("[*]")) {
                tabWidget->setTabText(index, tabText.left(tabText.length() - 3));
            }
        }

    });
    connect(m_fileManager, &FileManager::openEditorsChanged, this, &Qalam::syncOpenEditors);

    // --- Layout component signals ---
    auto *activityBar = m_layoutManager->activityBar();
    auto *sidebar = m_layoutManager->sidebar();
    auto *panelArea = m_layoutManager->panelArea();
    auto *statusBar = m_layoutManager->statusBar();

    connect(activityBar, &QalamActivityBar::viewChanged, this, &Qalam::onActivityViewChanged);
    connect(activityBar, &QalamActivityBar::runRequested, this, &Qalam::runBaa);

    connect(activityBar, &QalamActivityBar::viewToggled, this, [this](QalamActivityBar::ViewType view, bool visible) {
        if (view == QalamActivityBar::ViewType::Settings) {
            openSettings();
            return;
        }
        if (!visible) {
            m_layoutManager->sidebar()->hide();
        } else {
            m_layoutManager->sidebar()->show();
            m_layoutManager->sidebar()->setCurrentView(view);
        }
    });

    connect(sidebar, &QalamSidebar::fileSelected, this, &Qalam::onSidebarFileSelected);
    connect(sidebar, &QalamSidebar::openFolderRequested, this, &Qalam::handleOpenFolderMenu);
    connect(sidebar, &QalamSidebar::openEditorCloseRequested, this, &Qalam::closeEditorByPath);
    connect(sidebar, &QalamSidebar::searchRequested, this, &Qalam::performProjectSearch);
    if (sidebar->searchView()) {
        connect(sidebar->searchView(), &QalamSearchView::resultClicked, this, &Qalam::goToLocation);
    }

    connect(statusBar, &QalamStatusBar::problemsClicked, this, [this]() {
        m_layoutManager->panelArea()->setCurrentTab(QalamPanelArea::Tab::Problems);
        m_layoutManager->panelArea()->show();
    });

    connect(panelArea, &QalamPanelArea::problemClicked, this, &Qalam::goToLocation);

    connect(panelArea, &QalamPanelArea::closeRequested, this, [this]() {
        m_layoutManager->panelArea()->hide();
    });

    connect(panelArea, &QalamPanelArea::tabChanged, this, [this](QalamPanelArea::Tab tab) {
        if (tab == QalamPanelArea::Tab::Terminal and m_layoutManager->panelArea()->terminal()) {
            m_layoutManager->panelArea()->terminal()->setFocus();
        }
    });
}

void Qalam::closeEvent(QCloseEvent *event) {
    if (!maybeSaveAllModified()) {
        event->ignore();
        return;
    }

    event->accept();
}

bool Qalam::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_F6) {
            toggleConsole();
            return true;
        }
    }
    return QalamWindow::eventFilter(object, event);
}

void Qalam::goToLine()
{
    QalamEditor *editor = currentEditor();
    if (!editor) return;

    bool ok;
    int maxLine = editor->blockCount();

    int lineNumber = QInputDialog::getInt(this, "الذهاب إلى سطر",
                                          QString("أدخل رقم السطر (1 - %1):").arg(maxLine),
                                          1, 1, maxLine, 1, &ok);

    if (ok) {
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(0);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, lineNumber - 1);
        editor->setTextCursor(cursor);
        editor->centerCursor();
        editor->setFocus();
    }
}

void Qalam::showFindBar() {
    searchBar->setEditor(currentEditor());
    searchBar->show();
    searchBar->setFocusToInput();
}

void Qalam::hideFindBar() {
    searchBar->hide();
    if (QalamEditor* editor = currentEditor()) {
        editor->setFocus();
    }
}

void Qalam::toggleConsole()
{
    m_layoutManager->toggleConsole(currentEditor());
}

void Qalam::loadFolder(const QString &path)
{
    this->folderPath = path;
    if (m_workspaceIndexer) {
        m_workspaceIndexer->setRootPath(path);
    }
    m_layoutManager->loadFolder(path);
}

void Qalam::handleOpenFolderMenu()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "اختر مجلد", QDir::homePath());
    if (folderPath.isEmpty()) return;

    loadFolder(folderPath);

    if (not hasAnyEditorTabs()) {
        m_fileManager->newFile();
    }

    removeWelcomeTabIfPresent();
}

void Qalam::toggleSidebar()
{
    m_layoutManager->toggleSidebar();
}

void Qalam::openSettings() {
    if (setting and setting->isVisible()) return;
 
    connect(setting, &QalamSettings::fontSizeChanged, this, [this](int size){
        for (int i = 0; i < tabWidget->count(); ++i) {
            QalamEditor* editor = qobject_cast<QalamEditor*>(tabWidget->widget(i));
            if (editor) editor->updateFontSize(size);
        }
    }, Qt::UniqueConnection);
    connect(setting, &QalamSettings::fontTypeChanged, this, [this](QString font){
        for (int i = 0; i < tabWidget->count(); ++i) {
            QalamEditor* editor = qobject_cast<QalamEditor*>(tabWidget->widget(i));
            if (editor) editor->updateFontType(font);
        }
    }, Qt::UniqueConnection);
    connect(setting, &QalamSettings::highlighterThemeChanged, this, [this](int themeIdx){
        auto theme = ThemeManager::getThemeByIndex(themeIdx);
        for (int i = 0; i < tabWidget->count(); ++i) {
            QalamEditor* editor = qobject_cast<QalamEditor*>(tabWidget->widget(i));
            if (editor) editor->updateHighlighterTheme(theme);
        }
    }, Qt::UniqueConnection);
 
    setting->show();
}


void Qalam::exitApp() {
    close();
}

void Qalam::onCurrentTabChanged()
{
    updateWindowTitle();
    updateCursorPosition();

    QalamEditor* editor = currentEditor();

    // Disconnect the previous editor to avoid accumulating connections.
    // Also clear the raw pointer so closing the last editor does not leave a dangling reference.
    if (m_lastConnectedEditor) {
        disconnect(m_lastConnectedEditor, &QPlainTextEdit::cursorPositionChanged, this, &Qalam::updateCursorPosition);
        m_lastConnectedEditor = nullptr;
    }

    if (editor) {
        connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &Qalam::updateCursorPosition);
        connect(editor, &QObject::destroyed, this, [this, editor]() {
            if (m_lastConnectedEditor == editor) {
                m_lastConnectedEditor = nullptr;
            }
        }, Qt::UniqueConnection);
        m_lastConnectedEditor = editor;
        attachAnalysisToEditor(editor);
        scheduleEditorAnalysis(editor);

        // Keep search panel pointing at the active editor
        searchBar->setEditor(editor);
        if (m_layoutManager->breadcrumb()) {
            m_layoutManager->breadcrumb()->setVisible(!editor->currentFilePath().isEmpty());
        }
        applyDiagnosticsToEditors();
        if (m_layoutManager and m_layoutManager->sidebar() and
            m_layoutManager->sidebar()->explorerView()) {
            const QString filePath = QDir::cleanPath(
                QFileInfo(editor->currentFilePath()).absoluteFilePath());
            if (BaaLanguageClient::isBaaSourcePath(filePath)) {
                m_layoutManager->sidebar()->explorerView()
                    ->setOutlineSymbols(
                        filePath,
                        m_languageClient
                            ? m_languageClient->documentSymbols(filePath)
                            : QVector<BaaDocumentSymbol>{});
            } else {
                m_layoutManager->sidebar()->explorerView()
                    ->clearOutlineSymbols();
            }
        }
    } else {
        searchBar->setEditor(nullptr);
        if (m_layoutManager->breadcrumb()) {
            m_layoutManager->breadcrumb()->hide();
        }
        if (m_layoutManager and m_layoutManager->sidebar() and
            m_layoutManager->sidebar()->explorerView()) {
            m_layoutManager->sidebar()->explorerView()
                ->clearOutlineSymbols();
        }
    }
}

void Qalam::updateCursorPosition()
{
    QalamEditor* editor = currentEditor();
    if (editor) {
        const QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int column = cursor.columnNumber() + 1;

        // Update status bar
        if (m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->setCursorPosition(line, column);
        }
        
        // Update breadcrumb with current file
        if (m_layoutManager->breadcrumb()) {
            QString filePath = editor->currentFilePath();
            m_layoutManager->breadcrumb()->setFilePath(filePath);
        }
    }
}


/* ----------------------------------- Run Menu Button ----------------------------------- */

void Qalam::runBaa() {
    QalamEditor *editor = currentEditor();
    if (!editor) return;

    QString filePath = editor->currentFilePath();
    if (filePath.isEmpty() or editor->document()->isModified()) {
        QMessageBox::warning(this, "تنبيه", "يجب حفظ الملف قبل التشغيل.");
        m_fileManager->saveFile();
        filePath = editor->currentFilePath();
        if (filePath.isEmpty() or editor->document()->isModified()) return;
    }

    if (not BuildManager::findTakweenProjectRoot(filePath).isEmpty()) {
        runTakweenProjectCommand("run");
        return;
    }

    // Show the terminal tab because Baa programs may ask for input.
    auto *panelArea = m_layoutManager->panelArea();
    if (!panelArea) return;
    panelArea->clearProblems();
    if (m_diagnosticsModel) {
        m_diagnosticsModel->clear();
    } else {
        applyDiagnosticsToEditors();
        updateProblemsStatusBar();
    }
    panelArea->setCurrentTab(QalamPanelArea::Tab::Terminal);
    panelArea->show();
    panelArea->setCollapsed(false);

    // Delegate build to BuildManager
    QalamConsole *console = panelArea->terminal();
    m_buildManager->runBaa(filePath, console);
}

void Qalam::buildTakweenProject()
{
    runTakweenProjectCommand("build");
}

void Qalam::testTakweenProject()
{
    runTakweenProjectCommand("test");
}

void Qalam::cleanTakweenProject()
{
    runTakweenProjectCommand("clean");
}

void Qalam::runTakweenProjectCommand(const QString &command)
{
    QalamEditor *editor = currentEditor();
    if (not editor or editor->currentFilePath().isEmpty()) {
        QMessageBox::information(this, "مشروع تكوين", "افتح ملفًا محفوظًا داخل مشروع تكوين أولًا.");
        return;
    }

    if (editor->document()->isModified()) {
        m_fileManager->saveFile();
        if (editor->document()->isModified()) return;
    }

    QString targetName;
    const QString normalized = command.trimmed().toLower();
    if (normalized != "clean") {
        QString discoveryError;
        const QVector<TakweenTarget> targets = BuildManager::selectableTakweenTargets(
            m_buildManager->discoverTakweenTargets(editor->currentFilePath(), &discoveryError),
            normalized);
        if (not discoveryError.isEmpty()) {
            QMessageBox::warning(this, "أهداف تكوين", discoveryError);
            return;
        }
        if (targets.isEmpty()) {
            QMessageBox::warning(this, "أهداف تكوين", "لا يوجد هدف يدعم العملية المطلوبة.");
            return;
        }

        if (targets.size() == 1) {
            targetName = targets.first().name;
        } else {
            QStringList names;
            if (normalized == "test") names << "كل أهداف الاختبار";
            for (const TakweenTarget &target : targets) names << target.name;
            bool accepted = false;
            const QString selected = QInputDialog::getItem(
                this, "أهداف تكوين", "اختر الهدف:", names, 0, false, &accepted);
            if (not accepted) return;
            if (normalized != "test" or selected != "كل أهداف الاختبار") {
                targetName = selected;
            }
        }
    }

    auto *panelArea = m_layoutManager ? m_layoutManager->panelArea() : nullptr;
    if (not panelArea) return;
    panelArea->clearProblems();
    if (m_diagnosticsModel) m_diagnosticsModel->clear();
    panelArea->setCurrentTab(QalamPanelArea::Tab::Terminal);
    panelArea->show();
    panelArea->setCollapsed(false);

    if (not m_buildManager->runTakweenCommand(
            editor->currentFilePath(), command, panelArea->terminal(), targetName)) {
        QMessageBox::warning(
            this,
            "مشروع تكوين",
            "لم يُعثر على مشروع.تكوين أو على برنامج تكوين القابل للتنفيذ.");
    }
}

//----------------

QalamEditor* Qalam::currentEditor() {
    return qobject_cast<QalamEditor*>(tabWidget->currentWidget());
}

bool Qalam::shouldShowWelcome() const
{
    QSettings settings(Constants::OrgName, Constants::AppName);
    return settings.value(Constants::SettingsKeyShowWelcome, true).toBool();
}

bool Qalam::hasAnyEditorTabs() const
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (qobject_cast<QalamEditor*>(tabWidget->widget(i))) {
            return true;
        }
    }
    return false;
}

void Qalam::showWelcomeTab()
{
    if (m_welcomePage) {
        const int index = tabWidget->indexOf(m_welcomePage);
        if (index != -1) {
            m_welcomePage->refreshRecents();
            tabWidget->setCurrentIndex(index);
            return;
        }

        m_welcomePage->deleteLater();
        m_welcomePage = nullptr;
    }

    m_welcomePage = new QalamWelcomePage(tabWidget);

    connect(m_welcomePage, &QalamWelcomePage::newFileRequested, this, &Qalam::newFileFromUi);
    connect(m_welcomePage, &QalamWelcomePage::openFileRequested, this, [this]() { openFileFromUi(QString()); });
    connect(m_welcomePage, &QalamWelcomePage::openFolderRequested, this, &Qalam::handleOpenFolderMenu);
    connect(m_welcomePage, &QalamWelcomePage::recentFileRequested, this, &Qalam::openFileFromUi);
    connect(m_welcomePage, &QalamWelcomePage::cloneRepoRequested, this, [this]() {
        QMessageBox::information(this, "استنساخ", "هذه الميزة قيد التطوير.");
    });

    const int index = tabWidget->addTab(m_welcomePage, "الترحيب");
    tabWidget->setCurrentIndex(index);
}

void Qalam::removeWelcomeTabIfPresent()
{
    if (!m_welcomePage) return;

    const int index = tabWidget->indexOf(m_welcomePage);
    if (index == -1) return;

    tabWidget->removeTab(index);
    m_welcomePage->deleteLater();
    m_welcomePage = nullptr;
}

void Qalam::newFileFromUi()
{
    m_fileManager->newFile();
    if (hasAnyEditorTabs()) {
        removeWelcomeTabIfPresent();
    }
}

void Qalam::openFileFromUi(const QString &filePathOrEmpty)
{
    m_fileManager->openFile(filePathOrEmpty);
    if (hasAnyEditorTabs()) {
        removeWelcomeTabIfPresent();
    }
}

void Qalam::closeTab(int index)
{
    QWidget *tab = tabWidget->widget(index);

    if (!tab) return;

    if (auto *welcome = qobject_cast<QalamWelcomePage*>(tab)) {
        tabWidget->removeTab(index);
        if (welcome == m_welcomePage) {
            m_welcomePage->deleteLater();
            m_welcomePage = nullptr;
        } else {
            welcome->deleteLater();
        }

        if (tabWidget->count() == 0) {
            m_fileManager->newFile();
        }
        return;
    }

    QalamEditor* editor = qobject_cast<QalamEditor*>(tab);
    if (!editor) {
        tabWidget->removeTab(index);
        tab->deleteLater();
        return;
    }

    if (editor->document()->isModified()) {
        const int previousIndex = tabWidget->currentIndex();
        tabWidget->setCurrentIndex(index);
        auto saveResult = m_fileManager->needSave(editor);

        if (saveResult == FileManager::SaveAction::Cancel) {
            tabWidget->setCurrentIndex(previousIndex);
            return;
        }
        if (saveResult == FileManager::SaveAction::Save and !m_fileManager->saveEditor(editor)) {
            tabWidget->setCurrentIndex(previousIndex);
            return;
        }
    }

    if (m_languageClient and !editor->currentFilePath().isEmpty()) {
        m_languageClient->closeDocument(editor->currentFilePath());
    }
    if (m_diagnosticsModel and !editor->currentFilePath().isEmpty()) {
        const QString sourceId = "baa-lsp:" + QDir::cleanPath(editor->currentFilePath());
        m_diagnosticsModel->replaceDiagnosticsFromSource(sourceId, {});
    }
    tabWidget->removeTab(index);
    editor->deleteLater();
    syncOpenEditors();

    if (not hasAnyEditorTabs()) {
        if (shouldShowWelcome()) {
            showWelcomeTab();
        } else {
            m_fileManager->newFile();
        }
    }
}

bool Qalam::maybeSaveAllModified()
{
    const int previousIndex = tabWidget->currentIndex();

    for (int i = 0; i < tabWidget->count(); ++i) {
        QalamEditor *editor = qobject_cast<QalamEditor*>(tabWidget->widget(i));
        if (!editor or !editor->document()->isModified()) {
            continue;
        }

        tabWidget->setCurrentIndex(i);
        auto result = m_fileManager->needSave(editor);
        if (result == FileManager::SaveAction::Cancel) {
            tabWidget->setCurrentIndex(previousIndex);
            return false;
        }
        if (result == FileManager::SaveAction::Save and !m_fileManager->saveEditor(editor)) {
            tabWidget->setCurrentIndex(previousIndex);
            return false;
        }
    }

    if (previousIndex >= 0 and previousIndex < tabWidget->count()) {
        tabWidget->setCurrentIndex(previousIndex);
    }
    return true;
}

void Qalam::goToLocation(const QString &filePath, int line, int column)
{
    if (filePath.isEmpty()) return;

    openFileFromUi(filePath);
    QalamEditor *editor = currentEditor();
    if (!editor) return;

    QTextCursor cursor(editor->document());
    const int targetLine = qMax(1, line);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, targetLine - 1);
    const int targetColumn = qMax(1, column);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, targetColumn - 1);
    editor->setTextCursor(cursor);
    editor->centerCursor();
    editor->setFocus();
}

void Qalam::closeEditorByPath(const QString &filePath)
{
    const QString targetCanonical = QFileInfo(filePath).canonicalFilePath();
    const QString targetClean = QDir::cleanPath(filePath);

    for (int i = 0; i < tabWidget->count(); ++i) {
        QalamEditor *editor = qobject_cast<QalamEditor*>(tabWidget->widget(i));
        if (!editor) continue;

        const QString editorPath = editor->currentFilePath();
        const QString editorCanonical = QFileInfo(editorPath).canonicalFilePath();
        const QString editorClean = QDir::cleanPath(editorPath);

        const bool sameRealFile = !targetCanonical.isEmpty() and editorCanonical == targetCanonical;
        const bool sameUnsavedLabel = targetCanonical.isEmpty() and editorPath.isEmpty() and tabWidget->tabText(i) == filePath;
        const bool sameCleanPath = !targetClean.isEmpty() and !editorClean.isEmpty() and editorClean == targetClean;

        if (sameRealFile or sameCleanPath or sameUnsavedLabel) {
            closeTab(i);
            return;
        }
    }
}

void Qalam::performProjectSearch(const QString &query, bool caseSensitive, bool wholeWord, bool regex)
{
    auto *sidebar = m_layoutManager ? m_layoutManager->sidebar() : nullptr;
    auto *searchView = sidebar ? sidebar->searchView() : nullptr;
    if (!searchView) return;

    searchView->clearResults();

    const QString trimmedQuery = query.trimmed();
    if (trimmedQuery.isEmpty() or folderPath.isEmpty()) {
        searchView->setResultCount(0, 0);
        return;
    }

    QRegularExpression::PatternOptions options = QRegularExpression::UseUnicodePropertiesOption;
    if (!caseSensitive) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    QString pattern = regex ? trimmedQuery : QRegularExpression::escape(trimmedQuery);
    if (wholeWord) {
        pattern = QStringLiteral("(?<![\\p{L}\\p{N}_])(?:%1)(?![\\p{L}\\p{N}_])").arg(pattern);
    }

    QRegularExpression expression(pattern, options);
    if (!expression.isValid()) {
        searchView->setResultCount(0, 0);
        return;
    }

    int fileCount = 0;
    int matchCount = 0;

    const QStringList indexedFiles = collectProjectFiles();
    for (const QString &path : indexedFiles) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);

        bool fileHadMatch = false;
        int lineNumber = 0;
        while (!stream.atEnd()) {
            const QString lineText = stream.readLine();
            ++lineNumber;

            QRegularExpressionMatchIterator matches = expression.globalMatch(lineText);
            while (matches.hasNext()) {
                const QRegularExpressionMatch match = matches.next();
                if (!match.hasMatch()) continue;

                if (!fileHadMatch) {
                    fileHadMatch = true;
                    ++fileCount;
                }
                ++matchCount;
                searchView->addResult(path, lineNumber, match.capturedStart() + 1,
                                      lineText, match.captured(0));
            }
        }
    }

    searchView->setResultCount(fileCount, matchCount);
}


void Qalam::focusSearchInFiles()
{
    if (!m_layoutManager or !m_layoutManager->sidebar()) return;

    m_layoutManager->sidebar()->show();
    m_layoutManager->sidebar()->setCurrentView(QalamActivityBar::ViewType::Search);
    if (m_layoutManager->activityBar()) {
        m_layoutManager->activityBar()->setCurrentView(QalamActivityBar::ViewType::Search);
    }
    if (m_layoutManager->sidebar()->searchView()) {
        m_layoutManager->sidebar()->searchView()->focusSearchInput();
    }
}

void Qalam::openProblemsPanel()
{
    auto *panel = m_layoutManager ? m_layoutManager->panelArea() : nullptr;
    if (!panel) return;

    panel->setCurrentTab(QalamPanelArea::Tab::Problems);
    panel->show();
    panel->setCollapsed(false);
}


void Qalam::openDebugPanel()
{
    auto *panel = m_layoutManager ? m_layoutManager->panelArea() : nullptr;
    if (!panel) return;

    panel->setCurrentTab(QalamPanelArea::Tab::Debug);
    panel->show();
    panel->setCollapsed(false);
}

QStringList Qalam::collectProjectFiles() const
{
    if (m_workspaceIndexer) {
        return m_workspaceIndexer->quickOpenFiles();
    }
    return {};
}

bool Qalam::runCommandById(const QString &commandId)
{
    if (commandId == "file.new") { newFileFromUi(); return true; }
    if (commandId == "file.open") { openFileFromUi(QString()); return true; }
    if (commandId == "folder.open") { handleOpenFolderMenu(); return true; }
    if (commandId == "file.save") { m_fileManager->saveFile(); return true; }
    if (commandId == "file.saveAs") { m_fileManager->saveFileAs(); return true; }
    if (commandId == "view.search") { focusSearchInFiles(); return true; }
    if (commandId == "view.sidebar") { toggleSidebar(); return true; }
    if (commandId == "view.panel") { toggleConsole(); return true; }
    if (commandId == "view.problems") { openProblemsPanel(); return true; }
    if (commandId == "view.debug") { openDebugPanel(); return true; }
    if (commandId == "code.definition") { goToDefinition(); return true; }
    if (commandId == "code.references") { findReferences(); return true; }
    if (commandId == "code.workspaceSymbols") {
        showWorkspaceSymbols();
        return true;
    }
    if (commandId == "code.rename") { renameSymbol(); return true; }
    if (commandId == "code.quickFix") { quickFix(); return true; }
    if (commandId == "code.format") { formatDocument(); return true; }
    if (commandId == "code.expandSelection") {
        if (QalamEditor *editor = currentEditor()) editor->expandSemanticSelection();
        return true;
    }
    if (commandId == "code.shrinkSelection") {
        if (QalamEditor *editor = currentEditor()) editor->shrinkSemanticSelection();
        return true;
    }
    if (commandId == "project.build") { buildTakweenProject(); return true; }
    if (commandId == "project.test") { testTakweenProject(); return true; }
    if (commandId == "run.baa") { runBaa(); return true; }
    if (commandId == "project.stop") {
        if (m_buildManager and m_buildManager->isRunning()) m_buildManager->stop();
        return true;
    }
    if (commandId == "project.clean") { cleanTakweenProject(); return true; }
    if (commandId == "quick.open") { showQuickOpen(); return true; }
    if (commandId == "go.line") { goToLine(); return true; }
    if (commandId == "settings.open") { openSettings(); return true; }
    if (commandId == "help.about") { aboutQalam(); return true; }
    return false;
}

void Qalam::showCommandPalette()
{
    QVector<QalamCommandPalette::Entry> entries;
    if (m_commandRegistry) {
        for (const CommandRegistry::Command &command : m_commandRegistry->commands()) {
            entries.push_back({command.id, command.title, command.description, command.shortcut, QString()});
        }
    }

    auto *palette = new QalamCommandPalette(this);
    palette->setAttribute(Qt::WA_DeleteOnClose);
    palette->setWindowTitle("لوحة الأوامر");
    palette->setPlaceholderText("اكتب اسم الأمر...");
    palette->setEmptyText("لا يوجد أمر مطابق");
    palette->setEntries(entries);
    connect(palette, &QalamCommandPalette::entryActivated, this, [this](const QString &id, const QString &) {
        runCommandById(id);
    });
    palette->show();
}

void Qalam::showQuickOpen()
{
    const QStringList files = collectProjectFiles();
    if (files.isEmpty()) {
        if (folderPath.isEmpty()) {
            QMessageBox::information(this, "فتح سريع", "افتح مجلدًا أولًا لاستخدام الفتح السريع.");
        } else {
            QMessageBox::information(this, "فتح سريع", "لا توجد ملفات مناسبة داخل المجلد الحالي.");
        }
        return;
    }

    QVector<QalamCommandPalette::Entry> entries;
    entries.reserve(qMin(files.size(), 600));
    for (const QString &file : files) {
        const QString relative = QDir(folderPath).relativeFilePath(file);
        entries.push_back({"file.open.path", relative, QFileInfo(file).absolutePath(), QString(), file});
        if (entries.size() >= 600) break;
    }

    auto *palette = new QalamCommandPalette(this);
    palette->setAttribute(Qt::WA_DeleteOnClose);
    palette->setWindowTitle("فتح سريع");
    palette->setPlaceholderText("اكتب اسم الملف...");
    palette->setEmptyText("لا يوجد ملف مطابق");
    palette->setEntries(entries);
    connect(palette, &QalamCommandPalette::entryActivated, this, [this](const QString &id, const QString &payload) {
        if (id == "file.open.path" and !payload.isEmpty()) {
            openFileFromUi(payload);
        }
    });
    palette->show();
}

void Qalam::showWorkspaceSymbols()
{
    if (not m_languageClient) return;
    if (m_workspaceSymbolPalette)
        m_workspaceSymbolPalette->close();
    if (m_languageClient->state() != BaaLanguageClient::State::Ready) {
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral(
                    "افتح ملف باء وانتظر اتصال خادم اللغة قبل البحث عن الرموز."),
                4000);
        }
        return;
    }

    auto *palette = new QalamCommandPalette(this);
    m_workspaceSymbolPalette = palette;
    palette->setAttribute(Qt::WA_DeleteOnClose);
    palette->setWindowTitle(QStringLiteral("رموز مساحة العمل"));
    palette->setPlaceholderText(
        QStringLiteral("ابحث عن دالة أو نوع أو متغير عربي..."));
    palette->setEmptyText(
        QStringLiteral("جار فهرسة رموز مشروع تكوين..."));
    palette->setEntries({});
    connect(palette, &QObject::destroyed, this, [this, palette]() {
        if (m_workspaceSymbolPalette == palette)
            m_workspaceSymbolPalette = nullptr;
    });
    connect(
        palette,
        &QalamCommandPalette::entryActivated,
        this,
        [this](const QString &id, const QString &payload) {
            if (id != QStringLiteral("workspace.symbol")) return;
            const QJsonDocument document =
                QJsonDocument::fromJson(payload.toUtf8());
            if (not document.isObject()) return;
            const QJsonObject location = document.object();
            goToLocation(
                location.value(QStringLiteral("file")).toString(),
                location.value(QStringLiteral("line")).toInt() + 1,
                location.value(QStringLiteral("character")).toInt() + 1);
        });
    connect(
        m_languageClient,
        &BaaLanguageClient::workspaceSymbolsPublished,
        palette,
        [this, palette](
            const QString &,
            const QVector<BaaWorkspaceSymbol> &symbols) {
            if (m_workspaceSymbolPalette != palette) return;
            QVector<QalamCommandPalette::Entry> entries;
            entries.reserve(symbols.size());
            for (const BaaWorkspaceSymbol &symbol : symbols) {
                const QString relativePath = folderPath.isEmpty()
                    ? QFileInfo(symbol.filePath).fileName()
                    : QDir(folderPath).relativeFilePath(symbol.filePath);
                QStringList context;
                if (not symbol.containerName.isEmpty())
                    context.push_back(symbol.containerName);
                if (not symbol.detail.isEmpty())
                    context.push_back(symbol.detail);
                context.push_back(relativePath);
                const QJsonObject location{
                    {QStringLiteral("file"), symbol.filePath},
                    {QStringLiteral("line"), symbol.line},
                    {QStringLiteral("character"), symbol.character}
                };
                entries.push_back({
                    QStringLiteral("workspace.symbol"),
                    symbol.name,
                    context.join(QStringLiteral(" — ")),
                    QStringLiteral("سطر %1").arg(symbol.line + 1),
                    QString::fromUtf8(
                        QJsonDocument(location)
                            .toJson(QJsonDocument::Compact))
                });
            }
            palette->setEmptyText(
                QStringLiteral("لا توجد رموز مطابقة"));
            palette->setEntries(entries);
        });
    connect(
        m_languageClient,
        &BaaLanguageClient::workspaceSymbolsFailed,
        palette,
        [this, palette](const QString &, int code, const QString &) {
            if (m_workspaceSymbolPalette != palette) return;
            palette->setEmptyText(
                code == -32800 or code == -32801
                    ? QStringLiteral(
                          "تغير المستند أثناء الفهرسة؛ أغلق البحث وأعد فتحه.")
                    : QStringLiteral(
                          "تعذر تحميل رموز المشروع من خادم اللغة."));
            palette->setEntries({});
        });
    connect(
        m_languageClient,
        &BaaLanguageClient::stateChanged,
        palette,
        [this, palette](BaaLanguageClient::State state) {
            if (m_workspaceSymbolPalette != palette or
                state == BaaLanguageClient::State::Ready)
                return;
            palette->setEmptyText(
                QStringLiteral("انقطع اتصال خادم اللغة."));
            palette->setEntries({});
        });
    palette->show();
    m_languageClient->requestWorkspaceSymbols();
}

void Qalam::updateProblemsStatusBar()
{
    auto *status = m_layoutManager ? m_layoutManager->statusBar() : nullptr;
    if (!status || !m_diagnosticsModel) return;
    status->setProblemsCount(m_diagnosticsModel->errorCount(), m_diagnosticsModel->warningCount());
}

void Qalam::rebuildProblemsPanel()
{
    auto *panel = m_layoutManager ? m_layoutManager->panelArea() : nullptr;
    if (!panel || !m_diagnosticsModel) return;

    panel->clearProblems();
    for (const Diagnostic &diagnostic : m_diagnosticsModel->diagnostics()) {
        panel->addProblem(diagnostic.displayMessage(), diagnostic.file,
                          diagnostic.line, diagnostic.column,
                          diagnostic.severity);
    }
}

void Qalam::handleBuildOutput(const QString &text)
{
    if (!m_diagnosticsModel) return;

    QString fallbackFile;
    if (QalamEditor *editor = currentEditor()) {
        fallbackFile = editor->currentFilePath();
    }

    const QVector<Diagnostic> diagnostics = DiagnosticParser::parseCompilerOutput(text, fallbackFile, folderPath);
    m_diagnosticsModel->addDiagnostics(diagnostics);
}

void Qalam::applyDiagnosticsToEditors()
{
    for (int index = 0; index < tabWidget->count(); ++index) {
        auto *editor = qobject_cast<QalamEditor*>(tabWidget->widget(index));
        if (!editor) continue;

        QVector<QalamEditor::Diagnostic> editorDiagnostics;
        if (m_diagnosticsModel) {
            for (const Diagnostic &diagnostic : m_diagnosticsModel->diagnosticsForFile(editor->currentFilePath())) {
                editorDiagnostics.push_back({diagnostic.file, diagnostic.line, diagnostic.column,
                                             diagnostic.severity, diagnostic.message});
            }
        }
        editor->setDiagnostics(editorDiagnostics);
    }
}

bool Qalam::languageDocumentVersionIsCurrent(const QString &filePath,
                                             int documentVersion) const
{
    const QString wanted = QDir::cleanPath(
        QFileInfo(filePath).absoluteFilePath());
    for (int index = 0; index < tabWidget->count(); ++index) {
        const QalamEditor *editor = qobject_cast<QalamEditor*>(tabWidget->widget(index));
        if (not editor) continue;
        const QString current = QDir::cleanPath(
            QFileInfo(editor->currentFilePath()).absoluteFilePath());
        if (current == wanted and
            editor->property("qalam.lsp.version").toInt() == documentVersion)
            return true;
    }
    return false;
}

QString Qalam::lineTextForLocation(const BaaLocation &location) const
{
    const QString wanted = QDir::cleanPath(
        QFileInfo(location.filePath).absoluteFilePath());
    for (int index = 0; index < tabWidget->count(); ++index) {
        const QalamEditor *editor = qobject_cast<QalamEditor*>(tabWidget->widget(index));
        if (not editor) continue;
        const QString current = QDir::cleanPath(
            QFileInfo(editor->currentFilePath()).absoluteFilePath());
        if (current != wanted) continue;
        const QTextBlock block = editor->document()->findBlockByNumber(location.line);
        return block.isValid() ? block.text() : QString();
    }

    QFile file(wanted);
    if (not file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    for (int line = 0; not stream.atEnd(); ++line) {
        const QString text = stream.readLine();
        if (line == location.line) return text;
    }
    return QString();
}

void Qalam::goToDefinition()
{
    QalamEditor *editor = currentEditor();
    if (not editor or not m_languageClient or
        not BaaLanguageClient::isBaaSourcePath(editor->currentFilePath())) {
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral("الانتقال الدلالي متاح لملفات باء"), 3000);
        }
        return;
    }
    scheduleEditorAnalysis(editor);
    const QTextCursor cursor = editor->textCursor();
    m_languageClient->requestDefinition(
        editor->currentFilePath(),
        cursor.blockNumber(),
        cursor.positionInBlock());
}

void Qalam::findReferences()
{
    QalamEditor *editor = currentEditor();
    if (not editor or not m_languageClient or
        not BaaLanguageClient::isBaaSourcePath(editor->currentFilePath())) {
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral("المراجع الدلالية متاحة لملفات باء"), 3000);
        }
        return;
    }
    scheduleEditorAnalysis(editor);
    const QTextCursor cursor = editor->textCursor();
    m_languageClient->requestReferences(
        editor->currentFilePath(),
        cursor.blockNumber(),
        cursor.positionInBlock(),
        true);
}

void Qalam::renameSymbol()
{
    QalamEditor *editor = currentEditor();
    if (not editor or not m_languageClient or
        not BaaLanguageClient::isBaaSourcePath(editor->currentFilePath())) {
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral("إعادة التسمية الدلالية متاحة لملفات باء"),
                3000);
        }
        return;
    }
    scheduleEditorAnalysis(editor);
    const QTextCursor cursor = editor->textCursor();
    m_languageClient->requestPrepareRename(
        editor->currentFilePath(),
        cursor.blockNumber(),
        cursor.positionInBlock());
}

void Qalam::quickFix()
{
    QalamEditor *editor = currentEditor();
    if (not editor or not m_languageClient or
        not BaaLanguageClient::isBaaSourcePath(editor->currentFilePath())) {
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral("الإصلاحات السريعة متاحة لملفات باء"),
                3000);
        }
        return;
    }
    scheduleEditorAnalysis(editor);
    const QTextCursor cursor = editor->textCursor();
    m_languageClient->requestCodeActions(
        editor->currentFilePath(),
        cursor.blockNumber(),
        cursor.positionInBlock());
}

void Qalam::formatDocument()
{
    QalamEditor *editor = currentEditor();
    if (not editor or not m_languageClient or
        not BaaLanguageClient::isBaaSourcePath(editor->currentFilePath())) {
        if (m_layoutManager and m_layoutManager->statusBar()) {
            m_layoutManager->statusBar()->showMessage(
                QStringLiteral("التنسيق الرسمي متاح لملفات باء"), 3000);
        }
        return;
    }
    scheduleEditorAnalysis(editor);
    m_languageClient->requestFormatting(editor->currentFilePath());
}

bool Qalam::applyWorkspaceEdit(const BaaWorkspaceEdit &workspaceEdit,
                               QString *error)
{
    if (error) error->clear();
    auto fail = [error](const QString &message) {
        if (error) *error = message;
        return false;
    };
    if (not workspaceEdit.isValid())
        return fail(QStringLiteral("خطة التعديل فارغة."));

    struct PreparedDocument
    {
        QString filePath;
        QalamEditor *editor{};
        QString originalText;
        QString updatedText;
        QVector<BaaTextEdit> edits;
    };
    QVector<PreparedDocument> prepared;
    prepared.reserve(workspaceEdit.documents.size());
    QSet<QString> seenFiles;

    auto editorForPath = [this](const QString &filePath) -> QalamEditor * {
        const QString wanted = QDir::cleanPath(
            QFileInfo(filePath).absoluteFilePath());
        for (int index = 0; index < tabWidget->count(); ++index) {
            QalamEditor *editor =
                qobject_cast<QalamEditor*>(tabWidget->widget(index));
            if (not editor) continue;
            const QString current = QDir::cleanPath(
                QFileInfo(editor->currentFilePath()).absoluteFilePath());
            if (current == wanted) return editor;
        }
        return nullptr;
    };
    for (const BaaDocumentEdit &documentEdit : workspaceEdit.documents) {
        if (not documentEdit.isValid())
            return fail(QStringLiteral("تحتوي الخطة على تعديل ملف غير صالح."));
        const QString path = QDir::cleanPath(
            QFileInfo(documentEdit.filePath).absoluteFilePath());
        if (seenFiles.contains(path))
            return fail(QStringLiteral("كررت الخطة ملفاً واحداً أكثر من مرة."));
        seenFiles.insert(path);

        PreparedDocument document;
        document.filePath = path;
        document.editor = editorForPath(path);
        if (documentEdit.version >= 0) {
            if (not document.editor)
                return fail(QStringLiteral(
                    "تغيرت حالة ملف مفتوح قبل تطبيق التعديل."));
            if (document.editor->property("qalam.lsp.version").toInt() !=
                documentEdit.version)
                return fail(QStringLiteral(
                    "تغير نص أحد الملفات بعد حساب التعديل."));
        }

        if (document.editor) {
            document.originalText = document.editor->toPlainText();
        } else {
            QFile input(path);
            if (not input.open(QIODevice::ReadOnly))
                return fail(QStringLiteral("تعذر قراءة الملف: %1").arg(path));
            const QByteArray bytes = input.readAll();
            document.originalText = QString::fromUtf8(bytes);
            if (document.originalText.toUtf8() != bytes)
                return fail(QStringLiteral(
                    "الملف ليس نص UTF-8 صالحاً: %1").arg(path));
        }

        QString editError;
        if (not applyBaaTextEdits(
                document.originalText,
                documentEdit.edits,
                &document.updatedText,
                &document.edits,
                &editError))
            return fail(editError);
        prepared.push_back(std::move(document));
    }

    for (const PreparedDocument &document : prepared) {
        if (document.editor) continue;
        QSaveFile output(document.filePath);
        if (not output.open(QIODevice::WriteOnly))
            return fail(QStringLiteral(
                "تعذر فتح الملف للكتابة الآمنة: %1")
                .arg(document.filePath));
        const QByteArray bytes = document.updatedText.toUtf8();
        if (output.write(bytes) != bytes.size() or not output.commit())
            return fail(QStringLiteral(
                "تعذر حفظ الملف بعد تطبيق التعديل: %1")
                .arg(document.filePath));
    }

    for (const PreparedDocument &document : prepared) {
        if (not document.editor) continue;
        QTextCursor cursor(document.editor->document());
        cursor.beginEditBlock();
        for (const BaaTextEdit &edit : document.edits) {
            const int start = baaUtf16TextOffset(
                document.originalText, edit.line, edit.character);
            const int end = baaUtf16TextOffset(
                document.originalText, edit.endLine, edit.endCharacter);
            cursor.setPosition(start);
            cursor.setPosition(end, QTextCursor::KeepAnchor);
            cursor.insertText(edit.newText);
        }
        cursor.endEditBlock();
    }
    return true;
}

/* ----------------------------------- Help Menu Button ----------------------------------- */

void Qalam::aboutQalam() {
    QMessageBox messageDialog{};
    messageDialog.setWindowTitle("عن محرر قلم");
    messageDialog.setText(R"(
        محرر قلم (Qalam IDE)

        بيئة تطوير مبنية بـ Qt و C++ موجهة لدعم البرمجة ذات الصياغة العربية.

        • يدعم اتجاه الكتابة من اليمين إلى اليسار (RTL)
        • تلوين شيفرة (Syntax Highlighting) ومحرك ثيمات
        • إكمال تلقائي (Auto-complete) وحفظ تلقائي واستعادة النسخ الاحتياطية

        © Qalam IDE
                                    )"
                          );

    messageDialog.setStyleSheet("background: #03091A; color: white");

    messageDialog.exec();
}

/* ----------------------------------- Other Functions ----------------------------------- */

void Qalam::updateWindowTitle() {
    QalamEditor* editor = currentEditor();
    QString title{};

    if (!editor) {
        title = "قلم";
    } else {
        QString filePath = editor->currentFilePath();

        if (filePath.isEmpty()) {
            title = "غير معنون";
        } else {
            title = QFileInfo(filePath).fileName();
        }
        if (editor->document()->isModified()) {
            title += "[*]";
        }
        title += " - قلم";
    }
    setWindowTitle(title);
    setWindowModified(editor && editor->document()->isModified());
}

void Qalam::onActivityViewChanged(QalamActivityBar::ViewType view)
{
    m_layoutManager->onActivityViewChanged(view, folderPath);
}

void Qalam::onSidebarFileSelected(const QString &filePath)
{
    m_fileManager->openFile(filePath);
}

void Qalam::syncOpenEditors()
{
    auto *sidebar = m_layoutManager->sidebar();
    if (sidebar and sidebar->explorerView()) {
        m_sessionManager->syncOpenEditors(sidebar->explorerView());
    }

    for (int index = 0; index < tabWidget->count(); ++index) {
        if (QalamEditor *editor = qobject_cast<QalamEditor*>(tabWidget->widget(index))) {
            attachAnalysisToEditor(editor);
        }
    }
    scheduleEditorAnalysis(currentEditor());
}

void Qalam::attachAnalysisToEditor(QalamEditor *editor)
{
    if (!editor or !m_languageClient) return;

    if (!editor->property("qalam.lsp.attached").toBool()) {
        editor->setProperty("qalam.lsp.attached", true);
        connect(editor, &QPlainTextEdit::textChanged, this, [this, editor]() {
            scheduleEditorAnalysis(editor);
        });
        connect(editor, &QalamEditor::completionRequested, this,
                [this, editor](const QString &filePath, int line, int character) {
            if (not m_languageClient or
                not BaaLanguageClient::isBaaSourcePath(filePath)) return;
            scheduleEditorAnalysis(editor);
            m_languageClient->requestCompletion(filePath, line, character);
        });
        connect(editor, &QalamEditor::hoverRequested, this,
                [this, editor](const QString &filePath, int line, int character) {
            if (not m_languageClient or
                not BaaLanguageClient::isBaaSourcePath(filePath)) return;
            scheduleEditorAnalysis(editor);
            m_languageClient->requestHover(filePath, line, character);
        });
        connect(editor, &QalamEditor::signatureHelpRequested, this,
                [this, editor](const QString &filePath, int line, int character) {
            if (not m_languageClient or
                not BaaLanguageClient::isBaaSourcePath(filePath)) return;
            scheduleEditorAnalysis(editor);
            m_languageClient->requestSignatureHelp(filePath, line, character);
        });
        connect(editor, &QalamEditor::selectionRangeRequested, this,
                [this, editor](const QString &filePath,
                               int line,
                               int character) {
            if (not m_languageClient or
                not BaaLanguageClient::isBaaSourcePath(filePath)) return;
            scheduleEditorAnalysis(editor);
            m_languageClient->requestSelectionRanges(
                filePath, line, character);
        });
        connect(editor, &QalamEditor::quickFixRequested,
                this, &Qalam::quickFix);
        connect(editor, &QalamEditor::formatRequested,
                this, &Qalam::formatDocument);
    }
}

void Qalam::scheduleEditorAnalysis(QalamEditor *editor)
{
    if (!editor or !m_languageClient) return;
    const QString filePath = editor->currentFilePath();
    const QString normalizedPath = filePath.isEmpty()
        ? QString()
        : QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
    const QString previousPath = editor->property("qalam.lsp.path").toString();
    if (!previousPath.isEmpty() and previousPath != normalizedPath) {
        m_languageClient->closeDocument(previousPath);
        editor->clearSemanticTokens();
        editor->clearFoldingRanges();
        editor->clearInlayHints();
        if (m_diagnosticsModel) {
            m_diagnosticsModel->replaceDiagnosticsFromSource("baa-lsp:" + previousPath, {});
        }
    }
    editor->setProperty("qalam.lsp.path", normalizedPath);
    if (!BaaLanguageClient::isBaaSourcePath(filePath)) {
        editor->clearSemanticTokens();
        editor->clearFoldingRanges();
        editor->clearInlayHints();
        if (editor == currentEditor() and m_layoutManager and
            m_layoutManager->sidebar() and
            m_layoutManager->sidebar()->explorerView()) {
            m_layoutManager->sidebar()->explorerView()
                ->clearOutlineSymbols();
        }
        return;
    }

    const QString projectRoot =
        BuildManager::findTakweenProjectRoot(normalizedPath);
    const QString workspaceRoot =
        projectRoot.isEmpty() ? folderPath : projectRoot;
    const int previousVersion =
        editor->property("qalam.lsp.version").toInt();
    const int version = m_languageClient->synchronizeDocument(
        normalizedPath,
        editor->toPlainText(),
        editor->document()->revision(),
        workspaceRoot);
    if (version != previousVersion) {
        editor->clearSemanticTokens();
        editor->useLocalFoldingRanges();
        editor->clearInlayHints();
    }
    editor->setProperty("qalam.lsp.version", version);
    if (editor == currentEditor() and m_layoutManager and
        m_layoutManager->sidebar() and
        m_layoutManager->sidebar()->explorerView()) {
        m_layoutManager->sidebar()->explorerView()->setOutlineSymbols(
            normalizedPath,
            m_languageClient->documentSymbols(normalizedPath));
    }
}

void Qalam::handleLanguageDiagnostics(const QString &filePath,
                                      int documentVersion,
                                      const QVector<Diagnostic> &diagnostics)
{
    if (!m_diagnosticsModel or !m_languageClient) return;

    QalamEditor *matchingEditor = nullptr;
    for (int index = 0; index < tabWidget->count(); ++index) {
        QalamEditor *editor = qobject_cast<QalamEditor*>(tabWidget->widget(index));
        if (!editor) continue;
        const QString editorPath = QDir::cleanPath(
            QFileInfo(editor->currentFilePath()).absoluteFilePath());
        if (editorPath == QDir::cleanPath(filePath) and
            editor->property("qalam.lsp.version").toInt() == documentVersion) {
            matchingEditor = editor;
            break;
        }
    }
    if (!matchingEditor) return;
    const QString sourceId = "baa-lsp:" + QDir::cleanPath(filePath);
    m_diagnosticsModel->replaceDiagnosticsFromSource(sourceId, diagnostics);
}

void Qalam::handleLanguageCompletion(const QString &filePath,
                                     int documentVersion,
                                     int line,
                                     int character,
                                     const QVector<BaaCompletionItem> &items)
{
    for (int index = 0; index < tabWidget->count(); ++index) {
        QalamEditor *editor = qobject_cast<QalamEditor*>(tabWidget->widget(index));
        if (not editor) continue;
        const QString editorPath = QDir::cleanPath(
            QFileInfo(editor->currentFilePath()).absoluteFilePath());
        if (editorPath == QDir::cleanPath(filePath) and
            editor->property("qalam.lsp.version").toInt() == documentVersion) {
            editor->showLanguageCompletions(items, line, character);
            return;
        }
    }
}
