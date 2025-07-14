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

# ===================================================================
# Custom Build Step for Alif Language Server (als)
# ===================================================================

# Define the path to the als project relative to this .pro file's location
ALS_PROJECT_DIR = $$PWD/../als

# Define the build directory for the als project
ALS_BUILD_DIR = $$ALS_PROJECT_DIR/build

# 1. Define a phony target to ensure our custom commands always run
force_build_als.target = .phony_build_als
force_build_als.commands = @echo "Forcing ALS build..."
QMAKE_EXTRA_TARGETS += force_build_als

# 2. Define the main custom target to build the 'als' project
build_als.target = build_als_target
build_als.depends = $$force_build_als.target

# 3. Define the platform-specific commands to build the CMake project
win32 {
    # Windows (using cmd.exe)
    build_als.commands = \
        @if not exist \"$$shell_path($$ALS_BUILD_DIR)\" mkdir \"$$shell_path($$ALS_BUILD_DIR)\" && \
        cd \"$$shell_path($$ALS_BUILD_DIR)\" && \
        cmake .. -G "Visual Studio 17 2022" && \
        cmake --build . --config Release
} else:unix {
    # Linux and macOS
    build_als.commands = \
        mkdir -p $$ALS_BUILD_DIR && \
        cd $$ALS_BUILD_DIR && \
        cmake .. && \
        make -j$$(nproc)
}

# 4. Add this custom target as a dependency for the main Spectrum target
# This ensures 'build_als' runs before Spectrum is linked.
PRE_TARGETDEPS += $$build_als.target
QMAKE_EXTRA_TARGETS += build_als

# ===================================================================
# Copy the 'als' executable to the Spectrum build directory
# ===================================================================
win32 {
    ALS_EXECUTABLE = $$ALS_BUILD_DIR/Release/alif-language-server.exe
    DEST_PATH = $$OUT_PWD/release/alif-language-server.exe
    QMAKE_POST_LINK += $$QMAKE_COPY \"$$shell_path($$ALS_EXECUTABLE)\" \"$$shell_path($$DEST_PATH)\"
} else:unix {
    ALS_EXECUTABLE = $$ALS_BUILD_DIR/alif-language-server
    DEST_PATH = $$OUT_PWD/alif-language-server
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$ALS_EXECUTABLE) $$shell_path($$DEST_PATH)
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target




