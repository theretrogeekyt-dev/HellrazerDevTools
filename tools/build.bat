@echo off
setlocal EnableExtensions

set "WATCOMROOT=C:\WATCOM"
set "OWSETENV=%WATCOMROOT%\OWSETENV.BAT"

if not exist "%OWSETENV%" (
  echo ERROR: Cannot find "%OWSETENV%"
  echo        Edit WATCOMROOT in tools\build.bat to match your install path.
  exit /b 1
)

call "%OWSETENV%"

set "PROJ=%~dp0.."
set "SRC=%PROJ%\src\main.c"
set "OUTDIR=%PROJ%\build"
set "OUTEXE=%OUTDIR%\GAME.EXE"

if not exist "%OUTDIR%" mkdir "%OUTDIR%"

wcl -bt=dos -ml -wcd=138 "%SRC%" -fe="%OUTEXE%"

if errorlevel 1 (
  echo.
  echo Build FAILED.
  exit /b 1
)

echo.
echo Build OK: "%OUTEXE%"
exit /b 0