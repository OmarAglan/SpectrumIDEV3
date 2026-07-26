set(QALAM_APP_SOURCES
    main.cpp
    Qalam.cpp
    Qalam.h
    resources.qrc
)

if(WIN32)
    list(APPEND QALAM_APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/Qalam.rc)
endif()

qt_add_executable(Qalam ${QALAM_APP_SOURCES})

set_target_properties(Qalam PROPERTIES
    OUTPUT_NAME "Qalam"
)

if(WIN32)
    set_target_properties(Qalam PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    set_target_properties(Qalam PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE QalamLogo.icns
    )
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/resources/QalamLogo.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(Qalam PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/resources/QalamLogo.icns)
endif()

target_link_libraries(Qalam PRIVATE qalam_core)
qalam_harden_windows_runtime(Qalam)

# Add the current directory (qalam/) to include paths for local app headers
target_include_directories(Qalam PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    set(qalam_deploy_after_build_default ON)
else()
    set(qalam_deploy_after_build_default OFF)
endif()
option(QALAM_DEPLOY_AFTER_BUILD
    "Run Qt deployment tool after building Qalam on Windows"
    ${qalam_deploy_after_build_default})

if(WIN32 AND QALAM_DEPLOY_AFTER_BUILD)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt windeployqt6
        HINTS ${CMAKE_PREFIX_PATH}/bin
    )

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET Qalam POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-system-dxc-compiler --dir "$<TARGET_FILE_DIR:Qalam>" "$<TARGET_FILE:Qalam>"
            COMMENT "Deploying Qt runtime with windeployqt"
            VERBATIM
        )
    else()
        message(WARNING "QALAM_DEPLOY_AFTER_BUILD is ON, but windeployqt was not found. Run scripts/package-windows.ps1 after building.")
    endif()
endif()

if(WIN32 AND QALAM_BUILD_TESTS)
    find_program(QALAM_POWERSHELL_EXECUTABLE
        NAMES pwsh.exe powershell.exe
        REQUIRED)
    add_test(
        NAME test_windows_runtime_independence
        COMMAND "${QALAM_POWERSHELL_EXECUTABLE}"
            -NoProfile
            -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/scripts/test-windows-runtime.ps1"
            -Executable "$<TARGET_FILE:Qalam>"
            -StartupSeconds 1
    )
endif()

install(TARGETS Qalam
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)
