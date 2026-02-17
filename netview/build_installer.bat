@echo off
REM ═══════════════════════════════════════════════════════════════
REM   NetView - Installer Build Script
REM   Requires: Inno Setup 6.x
REM ═══════════════════════════════════════════════════════════════

echo.
echo ════════════════════════════════════════════════════════════
echo   NetView Installer Builder
echo ════════════════════════════════════════════════════════════
echo.

REM Check if executable exists
if not exist "build\bin\Release\NetView.exe" (
    echo ERROR: NetView.exe not found!
    echo.
    echo Please build the application first by running: build.bat
    echo.
    pause
    exit /b 1
)

REM Check for Inno Setup
set ISCC="C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not exist %ISCC% (
    echo ERROR: Inno Setup not found!
    echo.
    echo Please install Inno Setup 6 from:
    echo https://jrsoftware.org/isinfo.php
    echo.
    echo Default installation path expected:
    echo C:\Program Files ^(x86^)\Inno Setup 6\
    echo.
    pause
    exit /b 1
)

echo [1/2] Compiling installer script...
%ISCC% "installer\NetView_Setup.iss"

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Installer compilation failed!
    pause
    exit /b 1
)

echo [2/2] Installer created successfully!
echo.
echo ════════════════════════════════════════════════════════════
echo   Installer Ready!
echo ════════════════════════════════════════════════════════════
echo.
echo Location: output\NetView_Setup_v1.0.0.exe
echo.
echo You can now distribute this installer to users.
echo.
echo The installer will:
echo   ✓ Request admin privileges (one time only)
echo   ✓ Show license and readme
echo   ✓ Require acceptance checkbox
echo   ✓ Install NetView to Program Files
echo   ✓ Configure auto-start on boot
echo   ✓ Launch application after installation
echo.
pause
