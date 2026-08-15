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
    ../source/language/BaaLogEvent.cpp \
    ../source/language/BaaWorkspaceEdit.cpp \
    ../source/language/DiagnosticParser.cpp \
    ../source/language/DiagnosticsModel.cpp \
    ../source/language/LspMessageFramer.cpp \
    ../source/workspace/WorkspaceIndexer.cpp \
    ../source/debug/BreakpointModel.cpp \
    ../source/components/QalamFlatButton.cpp \
    ../source/components/QalamSearchPanel.cpp \
    ../source/components/QalamCommandPalette.cpp \
    ../source/console/ProcessWorker.cpp \
    ../source/console/QalamConsole.cpp \
    ../source/managers/BuildManager.cpp \
    ../source/managers/FileManager.cpp \
    ../source/managers/LayoutManager.cpp \
    ../source/managers/SessionManager.cpp \
    ../source/menubar/QalamMenuBar.cpp \
    ../source/pages/QalamWelcomePage.cpp \
    ../source/settings/QalamSettings.cpp \
    ../source/sidebar/QalamExplorerView.cpp \
    ../source/sidebar/QalamSearchView.cpp \
    ../source/texteditor/QalamAutoSave.cpp \
    ../source/texteditor/QalamBracketHandler.cpp \
    ../source/texteditor/QalamEditor.cpp \
    ../source/texteditor/QalamSnippetManager.cpp \
    ../source/texteditor/autocomplete/AutoCompleteUI.cpp \
    ../source/texteditor/highlighter/QalamLexer.cpp \
    ../source/texteditor/highlighter/QalamSyntaxDefinition.cpp \
    ../source/texteditor/highlighter/QalamSyntaxHighlighter.cpp \
    ../source/ui/QalamTheme.cpp \
    ../source/ui/QalamWindow.cpp \
    ../source/ui/QalamActivityBar.cpp \
    ../source/ui/QalamBreadcrumb.cpp \
    ../source/ui/QalamPanelArea.cpp \
    ../source/ui/QalamSidebar.cpp \
    ../source/ui/QalamStatusBar.cpp \
    ../source/ui/QalamTitleBar.cpp

HEADERS += Qalam.h \
    Constants.h \
    ../source/core/CommandRegistry.h \
    ../source/language/BaaLanguageClient.h \
    ../source/language/BaaLogEvent.h \
    ../source/language/BaaCodeAction.h \
    ../source/language/BaaDocumentSymbol.h \
    ../source/language/BaaCompletionItem.h \
    ../source/language/BaaHover.h \
    ../source/language/BaaInlayHint.h \
    ../source/language/BaaLocation.h \
    ../source/language/BaaSignatureHelp.h \
    ../source/language/BaaWorkspaceEdit.h \
    ../source/language/Diagnostic.h \
    ../source/language/DiagnosticParser.h \
    ../source/language/DiagnosticsModel.h \
    ../source/language/LspMessageFramer.h \
    ../source/workspace/WorkspaceIndexer.h \
    ../source/debug/BreakpointModel.h \
    ../source/components/QalamFlatButton.h \
    ../source/components/QalamSearchPanel.h \
    ../source/components/QalamCommandPalette.h \
    ../source/console/ProcessWorker.h \
    ../source/console/QalamConsole.h \
    ../source/managers/BuildManager.h \
    ../source/managers/FileManager.h \
    ../source/managers/LayoutManager.h \
    ../source/managers/SessionManager.h \
    ../source/menubar/QalamMenuBar.h \
    ../source/pages/QalamWelcomePage.h \
    ../source/settings/QalamSettings.h \
    ../source/sidebar/QalamExplorerView.h \
    ../source/sidebar/QalamSearchView.h \
    ../source/texteditor/QalamAutoSave.h \
    ../source/texteditor/QalamBracketHandler.h \
    ../source/texteditor/QalamEditor.h \
    ../source/texteditor/QalamSnippetManager.h \
    ../source/texteditor/autocomplete/AutoComplete.h \
    ../source/texteditor/autocomplete/AutoCompleteUI.h \
    ../source/texteditor/highlighter/QalamLexer.h \
    ../source/texteditor/highlighter/QalamSyntaxDefinition.h \
    ../source/texteditor/highlighter/QalamSyntaxHighlighter.h \
    ../source/texteditor/highlighter/QalamSyntaxThemes.h \
    ../source/texteditor/highlighter/QalamToken.h \
    ../source/texteditor/highlighter/ThemeManager.h \
    ../source/ui/QalamTheme.h \
    ../source/ui/QalamWindow.h \
    ../source/ui/QalamActivityBar.h \
    ../source/ui/QalamBreadcrumb.h \
    ../source/ui/QalamPanelArea.h \
    ../source/ui/QalamSidebar.h \
    ../source/ui/QalamStatusBar.h \
    ../source/ui/QalamTitleBar.h

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
