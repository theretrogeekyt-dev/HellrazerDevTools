@echo off
REM Build script for OpenWatcom compilation on MS-DOS

REM Set the path to the OpenWatcom compiler
set WATCOM_PATH=C:\WATCOM

REM Set the source and output file names
set SRC_FILE=hello.c
set OUT_FILE=hello.exe

REM Navigate to the directory containing the source file
cd /d %~dp0

REM Compile the source file using OpenWatcom
%WATCOM_PATH%\binw\wcl %SRC_FILE% -o%OUT_FILE%

REM Check if the compilation was successful
if %errorlevel% neq 0 (
    echo Compilation failed!
    exit /b %errorlevel%
)

echo Compilation completed successfully!