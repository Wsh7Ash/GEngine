@echo off
REM ================================================================
REM  package_sdk.bat - Create distributable GEngine SDK package
REM ================================================================

setlocal enabledelayedexpansion

set SDK_VERSION=0.1.0
set SDK_NAME=GEngine-SDK-%SDK_VERSION%
set SDK_DIR=build\%SDK_NAME%

echo ========================================
echo  GEngine SDK Packager
echo  Version: %SDK_VERSION%
echo ========================================
echo.

REM Clean previous package
if exist build\%SDK_NAME%.zip (
    del build\%SDK_NAME%.zip
)

REM Create SDK directory structure
echo [1/5] Creating SDK directory structure...
rmdir /s /q "%SDK_DIR%" 2>nul
mkdir "%SDK_DIR%"
mkdir "%SDK_DIR%\include"
mkdir "%SDK_DIR%\lib"
mkdir "%SDK_DIR%\bin"
mkdir "%SDK_DIR%\examples"
mkdir "%SDK_DIR%\templates"
mkdir "%SDK_DIR%\cmake"
mkdir "%SDK_DIR%\assets"
echo.

REM Build engine
echo [2/5] Building engine in Release mode...
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build configuration failed!
    exit /b 1
)
cmake --build build --config Release --parallel
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    exit /b 1
)
echo.

REM Copy headers
echo [3/5] Copying public headers...
xcopy /E /I /Y include\GEngine "%SDK_DIR%\include\GEngine"
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Some headers may not have been copied
)
echo.

REM Copy libraries and binaries
echo [4/5] Copying libraries and binaries...
if exist build\Release\ge_core.lib (
    copy build\Release\ge_core.lib "%SDK_DIR%\lib\"
)
if exist build\bin\Release\GameEngine.exe (
    copy build\bin\Release\GameEngine.exe "%SDK_DIR%\bin\"
)
echo.

REM Copy SDK files
echo [5/5] Copying SDK examples and templates...
xcopy /E /I /Y sdk\examples "%SDK_DIR%\examples"
xcopy /E /I /Y sdk\templates "%SDK_DIR%\templates"
copy sdk\CMakeLists.txt "%SDK_DIR%\cmake\" 2>nul
copy cmake\GEngineConfig.cmake.in "%SDK_DIR%\cmake\" 2>nul
echo.

REM Create archive
echo Creating %SDK_NAME%.zip...
powershell -Command "Compress-Archive -Path '%SDK_DIR%' -DestinationPath 'build\%SDK_NAME%.zip' -Force"
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo  SDK Package created: build\%SDK_NAME%.zip
    echo ========================================
) else (
    echo.
    echo ========================================
    echo  SDK directory created: %SDK_DIR%
    echo  (zip creation failed - directory is ready)
    echo ========================================
)

exit /b 0
