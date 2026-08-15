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
    # Debug services
    debug/BreakpointModel.cpp
    debug/BreakpointModel.h
    # Text Editor
    texteditor/TEditor.cpp
    texteditor/TBracketHandler.cpp
    texteditor/TAutoSave.cpp
    texteditor/TSnippetManager.cpp
    texteditor/highlighter/TLexer.cpp
    texteditor/highlighter/TSyntaxDefinition.cpp
    texteditor/highlighter/TSyntaxHighlighter.cpp
    texteditor/autocomplete/AutoCompleteUI.cpp
    # Components
    components/TFlatButton.cpp
    components/TSearchPanel.cpp
    components/TCommandPalette.cpp
    components/TCommandPalette.h
    # Console
    console/TConsole.cpp
    console/ProcessWorker.cpp
    # Menu
    menubar/TMenu.cpp
    # Settings
    settings/TSettings.cpp
    # Pages
    pages/TWelcomePage.cpp
    # Sidebar
    sidebar/TExplorerView.cpp
    sidebar/TSearchView.cpp
    sidebar/TSymbolOutlineView.cpp
    sidebar/TSymbolOutlineView.h
    # UI Components
    ui/QalamTheme.cpp
    ui/TTitleBar.cpp
    ui/QalamWindow.cpp
    ui/TActivityBar.cpp
    ui/TSidebar.cpp
    ui/TBreadcrumb.cpp
    ui/TPanelArea.cpp
    ui/TStatusBar.cpp
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

target_link_libraries(qalam_core PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core)

if(WIN32)
    target_link_libraries(qalam_core PUBLIC dwmapi user32)
endif()
