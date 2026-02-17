@echo off
REM NetView - Build Script for Visual Studio 2022

echo.
echo ════════════════════════════════════════════════════════════
echo   NetView - Build System (VS 2022)
echo ════════════════════════════════════════════════════════════
echo.

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake not found
    pause
    exit /b 1
)

echo [1/4] Cleaning...
if exist build rmdir /s /q build

echo [2/4] Creating build directory...
mkdir build
cd build

echo [3/4] Configuring (Visual Studio 17 2022)...
cmake .. -G "Visual Studio 17 2022" -A x64

if %errorlevel% neq 0 (
    echo ERROR: CMake failed!
    cd ..
    pause
    exit /b 1
)

echo [4/4] Building Release...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ════════════════════════════════════════════════════════════
echo   BUILD SUCCESSFUL!
echo ════════════════════════════════════════════════════════════
echo.
echo Executable: build\bin\Release\NetView.exe
echo.
echo UI Theme: BLACK with arrows
echo   • Data In: GREEN down arrow
echo   • Data Out: RED up arrow
echo.
pause
