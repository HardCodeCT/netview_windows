@echo off
REM ========================================
REM NetView - Run Script
REM ========================================

echo.
echo ========================================
echo Launching NetView Network Monitor...
echo ========================================
echo.

REM Check if executable exists
if exist "build\bin\Release\NetView.exe" (
    echo Starting NetView.exe...
    echo.
    start "" "build\bin\Release\NetView.exe"
    echo [OK] NetView launched!
    echo Look for the widget in the bottom-right corner of your screen.
) else if exist "build\bin\Debug\NetView.exe" (
    echo Starting NetView.exe (Debug build)...
    echo.
    start "" "build\bin\Debug\NetView.exe"
    echo [OK] NetView launched!
    echo Look for the widget in the bottom-right corner of your screen.
) else (
    echo [ERROR] NetView.exe not found!
    echo.
    echo Please build the project first using build.bat
    echo.
    pause
    exit /b 1
)

echo.
