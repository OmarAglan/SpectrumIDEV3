add_library(qalam_core STATIC
    # Core platform
    core/CommandRegistry.cpp
    core/CommandRegistry.h
    # Language / IDE services
    language/BaaLanguageClient.cpp
    language/BaaLanguageClient.h
    language/BaaCodeAction.h
    language/BaaDocumentSymbol.h
    language/BaaCompletionItem.h
    language/BaaHover.h
    language/BaaInlayHint.h
    language/BaaFoldingRange.h
    language/BaaLocation.h
    language/BaaLogEvent.cpp
    language/BaaLogEvent.h
    language/BaaSignatureHelp.h
    language/BaaSemanticToken.h
    language/BaaSelectionRange.h
    language/BaaWorkspaceEdit.h
    language/BaaWorkspaceEdit.cpp
    language/BaaWorkspaceSymbol.h
    language/Diagnostic.h
    language/DiagnosticParser.cpp
    language/DiagnosticParser.h
    language/DiagnosticsModel.cpp
    language/DiagnosticsModel.h
    language/LspMessageFramer.cpp
    language/LspMessageFramer.h
    language/TakweenProtocol.cpp
    language/TakweenProtocol.h
    # Workspace services
    workspace/WorkspaceIndexer.cpp
    workspace/WorkspaceIndexer.h
    workspace/ProjectSearchService.cpp
    workspace/ProjectSearchService.h
    # Debug services
    debug/BreakpointModel.cpp
    debug/BreakpointModel.h
    # Text Editor
    texteditor/QalamEditor.cpp
    texteditor/QalamBracketHandler.cpp
    texteditor/QalamAutoSave.cpp
    texteditor/QalamSnippetManager.cpp
    texteditor/highlighter/QalamLexer.cpp
    texteditor/highlighter/QalamSyntaxDefinition.cpp
    texteditor/highlighter/QalamSyntaxHighlighter.cpp
    texteditor/autocomplete/AutoCompleteUI.cpp
    # Components
    components/QalamFlatButton.cpp
    components/QalamSearchPanel.cpp
    components/QalamCommandPalette.cpp
    components/QalamCommandPalette.h
    # Console
    console/QalamConsole.cpp
    console/ProcessWorker.cpp
    # Menu
    menubar/QalamMenuBar.cpp
    # Settings
    settings/QalamSettings.cpp
    # Pages
    pages/QalamWelcomePage.cpp
    # Sidebar
    sidebar/QalamExplorerView.cpp
    sidebar/QalamSearchView.cpp
    sidebar/QalamSymbolOutlineView.cpp
    sidebar/QalamSymbolOutlineView.h
    # UI Components
    ui/QalamTheme.cpp
    ui/QalamTitleBar.cpp
    ui/QalamWindow.cpp
    ui/QalamActivityBar.cpp
    ui/QalamSidebar.cpp
    ui/QalamBreadcrumb.cpp
    ui/QalamPanelArea.cpp
    ui/QalamStatusBar.cpp
    # Managers
    managers/FileManager.cpp
    managers/BuildManager.cpp
    managers/SessionManager.cpp
    managers/LayoutManager.cpp
)

target_include_directories(qalam_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../qalam
    texteditor
    texteditor/highlighter
    texteditor/autocomplete
    core
    language
    workspace
    debug
    components
    console
    menubar
    pages
    settings
    sidebar
    ui
    managers
)

target_link_libraries(qalam_core PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core Qt6::Svg)

if(WIN32)
    target_link_libraries(qalam_core PUBLIC dwmapi user32)
endif()
