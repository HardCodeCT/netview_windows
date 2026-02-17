@echo off
REM ========================================
REM NetView - Clean Build Script
REM ========================================

echo.
echo ========================================
echo Cleaning build artifacts...
echo ========================================
echo.

REM Remove build directory
if exist "build" (
    echo Removing build directory...
    rmdir /s /q build
    echo [OK] Build directory removed
) else (
    echo [INFO] Build directory does not exist
)

REM Remove any CMake cache files in root
if exist "CMakeCache.txt" (
    echo Removing CMakeCache.txt...
    del /f /q CMakeCache.txt
)

if exist "CMakeFiles" (
    echo Removing CMakeFiles directory...
    rmdir /s /q CMakeFiles
)

echo.
echo ========================================
echo Clean completed!
echo ========================================
echo.
echo You can now run build.bat to rebuild the project.
echo.

pause
