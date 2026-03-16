@echo off
REM Quick Download Script for Desktop-voice-assistant
REM This script downloads the program with external Flash loader

echo ========================================
echo  Desktop Voice Assistant - Quick Download
echo ========================================
echo.

set PROGRAMMER="D:\RT-ThreadStudio\repo\Extract\Debugger_Support_Packages\STMicroelectronics\ST-LINK_Debugger\2.11.0\tools\bin\STM32_Programmer_CLI.exe"
set LOADER="board\stldr\ART-Pi2_ST_winbond_64MB.stldr"
set ELF_FILE="Debug\rtthread.elf"

echo Checking if files exist...
if not exist %ELF_FILE% (
    echo ERROR: %ELF_FILE% not found!
    echo Please build the project first.
    pause
    exit /b 1
)

if not exist %LOADER% (
    echo ERROR: External Flash Loader not found!
    echo %LOADER%
    pause
    exit /b 1
)

echo.
echo Starting download with external Flash loader...
echo.

%PROGRAMMER% -c port=SWD mode=NORMAL -el %LOADER% -d %ELF_FILE% -hardRst -s

echo.
echo ========================================
echo  Download Complete!
echo ========================================
pause

