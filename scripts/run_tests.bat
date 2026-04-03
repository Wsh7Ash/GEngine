@echo off
REM ================================================================
REM  run_tests.bat - Build and run all tests
REM ================================================================

setlocal enabledelayedexpansion

echo ========================================
echo  GEngine Test Runner
echo ========================================
echo.

REM Configure
echo [1/3] Configuring build with tests enabled...
cmake -B build -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed!
    exit /b 1
)
echo.

REM Build
echo [2/3] Building tests...
cmake --build build --config Debug --parallel
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    exit /b 1
)
echo.

REM Run tests
echo [3/3] Running tests...
ctest --test-dir build --output-on-failure --verbose
set TEST_RESULT=%ERRORLEVEL%
echo.

if %TEST_RESULT% EQU 0 (
    echo ========================================
    echo  ALL TESTS PASSED
    echo ========================================
) else (
    echo ========================================
    echo  SOME TESTS FAILED (exit code: %TEST_RESULT%)
    echo ========================================
)

exit /b %TEST_RESULT%
