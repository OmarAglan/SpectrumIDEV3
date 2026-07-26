QT += core gui widgets

CONFIG += c++23

TARGET = Qalam
TEMPLATE = app

RESOURCES += resources.qrc

INCLUDEPATH += . \
    ../source/core \
    ../source/language \
    ../source/workspace \
    ../source/debug \
    ../source/components \
    ../source/console \
    ../source/managers \
    ../source/menubar \
    ../source/pages \
    ../source/settings \
    ../source/sidebar \
    ../source/texteditor \
    ../source/texteditor/autocomplete \
    ../source/texteditor/highlighter \
    ../source/ui

SOURCES += Qalam.cpp \
    main.cpp \
    ../source/core/CommandRegistry.cpp \
    ../source/language/BaaLanguageClient.cpp \
    ../source/language/BaaWorkspaceEdit.cpp \
    ../source/language/DiagnosticParser.cpp \
    ../source/language/DiagnosticsModel.cpp \
    ../source/language/LspMessageFramer.cpp \
    ../source/workspace/WorkspaceIndexer.cpp \
    ../source/debug/BreakpointModel.cpp \
    ../source/components/TFlatButton.cpp \
    ../source/components/TSearchPanel.cpp \
    ../source/components/TCommandPalette.cpp \
    ../source/console/ProcessWorker.cpp \
    ../source/console/TConsole.cpp \
    ../source/managers/BuildManager.cpp \
    ../source/managers/FileManager.cpp \
    ../source/managers/LayoutManager.cpp \
    ../source/managers/SessionManager.cpp \
    ../source/menubar/TMenu.cpp \
    ../source/pages/TWelcomePage.cpp \
    ../source/settings/TSettings.cpp \
    ../source/sidebar/TExplorerView.cpp \
    ../source/sidebar/TSearchView.cpp \
    ../source/texteditor/TAutoSave.cpp \
    ../source/texteditor/TBracketHandler.cpp \
    ../source/texteditor/TEditor.cpp \
    ../source/texteditor/TSnippetManager.cpp \
    ../source/texteditor/autocomplete/AutoCompleteUI.cpp \
    ../source/texteditor/highlighter/TLexer.cpp \
    ../source/texteditor/highlighter/TSyntaxDefinition.cpp \
    ../source/texteditor/highlighter/TSyntaxHighlighter.cpp \
    ../source/ui/QalamTheme.cpp \
    ../source/ui/QalamWindow.cpp \
    ../source/ui/TActivityBar.cpp \
    ../source/ui/TBreadcrumb.cpp \
    ../source/ui/TPanelArea.cpp \
    ../source/ui/TSidebar.cpp \
    ../source/ui/TStatusBar.cpp \
    ../source/ui/TTitleBar.cpp

HEADERS += Qalam.h \
    Constants.h \
    ../source/core/CommandRegistry.h \
    ../source/language/BaaLanguageClient.h \
    ../source/language/BaaDocumentSymbol.h \
    ../source/language/BaaCompletionItem.h \
    ../source/language/BaaHover.h \
    ../source/language/BaaLocation.h \
    ../source/language/BaaSignatureHelp.h \
    ../source/language/BaaWorkspaceEdit.h \
    ../source/language/Diagnostic.h \
    ../source/language/DiagnosticParser.h \
    ../source/language/DiagnosticsModel.h \
    ../source/language/LspMessageFramer.h \
    ../source/workspace/WorkspaceIndexer.h \
    ../source/debug/BreakpointModel.h \
    ../source/components/TFlatButton.h \
    ../source/components/TSearchPanel.h \
    ../source/components/TCommandPalette.h \
    ../source/console/ProcessWorker.h \
    ../source/console/TConsole.h \
    ../source/managers/BuildManager.h \
    ../source/managers/FileManager.h \
    ../source/managers/LayoutManager.h \
    ../source/managers/SessionManager.h \
    ../source/menubar/TMenu.h \
    ../source/pages/TWelcomePage.h \
    ../source/settings/TSettings.h \
    ../source/sidebar/TExplorerView.h \
    ../source/sidebar/TSearchView.h \
    ../source/texteditor/TAutoSave.h \
    ../source/texteditor/TBracketHandler.h \
    ../source/texteditor/TEditor.h \
    ../source/texteditor/TSnippetManager.h \
    ../source/texteditor/autocomplete/AutoComplete.h \
    ../source/texteditor/autocomplete/AutoCompleteUI.h \
    ../source/texteditor/highlighter/TLexer.h \
    ../source/texteditor/highlighter/TSyntaxDefinition.h \
    ../source/texteditor/highlighter/TSyntaxHighlighter.h \
    ../source/texteditor/highlighter/TSyntaxThemes.h \
    ../source/texteditor/highlighter/TToken.h \
    ../source/texteditor/highlighter/ThemeManager.h \
    ../source/ui/QalamTheme.h \
    ../source/ui/QalamWindow.h \
    ../source/ui/TActivityBar.h \
    ../source/ui/TBreadcrumb.h \
    ../source/ui/TPanelArea.h \
    ../source/ui/TSidebar.h \
    ../source/ui/TStatusBar.h \
    ../source/ui/TTitleBar.h

# Windows executable icon and native frame libraries
win32 {
    RC_ICONS += resources/QalamLogo.ico
    LIBS += -ldwmapi -luser32
}

# macOS bundle icon
macx:ICON = resources/QalamLogo.icns

# Default install path for Unix-like systems
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
