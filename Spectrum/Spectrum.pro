QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++23

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


TARGET = Spectrum

RESOURCES += \
    resources.qrc


# Include directories
INCLUDEPATH +=  ../Source/TextEditor \
                ../Source/MenuBar   \
                ../Source/Settings  \
                ../Source/Components    \
                ../Source/LspClient    \

SOURCES += \
    Spectrum.cpp \
    main.cpp     \
    ../Source/TextEditor/AlifComplete.cpp \
    ../Source/TextEditor/AlifLexer.cpp \
    ../Source/TextEditor/SPEditor.cpp \
    ../Source/TextEditor/SPHighlighter.cpp \
    ../Source/MenuBar/SPMenu.cpp    \
    ../Source/Settings/SPSettings.cpp   \
    ../Source/Components/FlatButton.cpp \
    ../Source/LspClient/SpectrumLspClient.cpp \
    ../Source/LspClient/LspProcess.cpp \
    ../Source/LspClient/LspProtocol.cpp \
    ../Source/LspClient/LspFeatureManager.cpp \
    ../Source/LspClient/DocumentManager.cpp \
    ../Source/LspClient/ErrorManager.cpp \
    

HEADERS += \
    Spectrum.h  \
    ../Source/TextEditor/AlifComplete.h \
    ../Source/TextEditor/AlifLexer.h \
    ../Source/TextEditor/SPEditor.h \
    ../Source/TextEditor/SPHighlighter.h \
    ../Source/MenuBar/SPMenu.h  \
    ../Source/Settings/SPSettings.h \
    ../Source/Components/FlatButton.h \
    ../Source/LspClient/SpectrumLspClient.h \
    ../Source/LspClient/LspProcess.h \
    ../Source/LspClient/LspProtocol.h \
    ../Source/LspClient/LspFeatureManager.h \
    ../Source/LspClient/DocumentManager.h \
    ../Source/LspClient/ErrorManager.h \



# Add the application icon (Windows)
win32:RC_ICONS += Resources/TaifLogo.ico

# Add the application icon (macOS/Linux)
macx:ICON = Resources/TaifLogo.icns
unix:!macx:ICON = Resources/TaifLogo.png


