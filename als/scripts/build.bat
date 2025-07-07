@echo off
REM Alif Language Server - Windows Build Script
REM Usage: build.bat [Debug|Release] [clean] [test] [install]

setlocal enabledelayedexpansion

REM Default configuration
set BUILD_TYPE=Release
set CLEAN_BUILD=0
set RUN_TESTS=0
set INSTALL_ALS=0
set BUILD_DIR=build
set GENERATOR="Visual Studio 17 2022"

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :done_parsing
if /i "%~1"=="Debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%~1"=="Release" (
    set BUILD_TYPE=Release
    shift
    goto :parse_args
)
if /i "%~1"=="clean" (
    set CLEAN_BUILD=1
    shift
    goto :parse_args
)
if /i "%~1"=="test" (
    set RUN_TESTS=1
    shift
    goto :parse_args
)
if /i "%~1"=="install" (
    set INSTALL_ALS=1
    shift
    goto :parse_args
)
echo Unknown argument: %~1
echo Usage: build.bat [Debug^|Release] [clean] [test] [install]
exit /b 1

:done_parsing

echo ========================================
echo Alif Language Server - Build Script
echo ========================================
echo Build Type: %BUILD_TYPE%
echo Clean Build: %CLEAN_BUILD%
echo Run Tests: %RUN_TESTS%
echo Install: %INSTALL_ALS%
echo ========================================

REM Check for required tools
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake 3.20 or higher
    exit /b 1
)

REM Check CMake version
for /f "tokens=3" %%i in ('cmake --version ^| findstr /r "cmake version"') do set CMAKE_VERSION=%%i
echo CMake Version: %CMAKE_VERSION%

REM Clean build if requested
if %CLEAN_BUILD%==1 (
    echo Cleaning build directory...
    if exist %BUILD_DIR% (
        rmdir /s /q %BUILD_DIR%
    )
)

REM Create build directory
if not exist %BUILD_DIR% (
    mkdir %BUILD_DIR%
)

cd %BUILD_DIR%

REM Configure project
echo Configuring project...
cmake .. ^
    -G %GENERATOR% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DALS_BUILD_TESTS=%RUN_TESTS% ^
    -DCMAKE_INSTALL_PREFIX="%CD%\install"

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed
    cd ..
    exit /b 1
)

REM Build project
echo Building project...
cmake --build . --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)

REM Run tests if requested
if %RUN_TESTS%==1 (
    echo Running tests...
    ctest --output-on-failure --build-config %BUILD_TYPE%
    if !ERRORLEVEL! neq 0 (
        echo WARNING: Some tests failed
    )
)

REM Install if requested
if %INSTALL_ALS%==1 (
    echo Installing...
    cmake --install . --config %BUILD_TYPE%
    if !ERRORLEVEL! neq 0 (
        echo ERROR: Installation failed
        cd ..
        exit /b 1
    )
    echo Installation completed in: %CD%\install
)

cd ..

echo ========================================
echo Build completed successfully!
echo ========================================
echo Executable: %BUILD_DIR%\%BUILD_TYPE%\als.exe
if %INSTALL_ALS%==1 (
    echo Installed to: %BUILD_DIR%\install
)
echo ========================================

REM Show next steps
echo Next steps:
echo   Run server: %BUILD_DIR%\%BUILD_TYPE%\als.exe
if %RUN_TESTS%==0 (
    echo   Run tests:  build.bat %BUILD_TYPE% test
)
echo   Clean build: build.bat clean
echo ========================================
