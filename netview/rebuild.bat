@echo off
REM ========================================
REM NetView - Rebuild Script
REM ========================================

echo.
echo ========================================
echo NetView - Complete Rebuild
echo ========================================
echo.

REM Clean first
call clean.bat

REM Then build
call build.bat
