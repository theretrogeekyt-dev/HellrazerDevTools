@echo off
setlocal EnableExtensions

REM =========================
REM  DOSBox-X path
REM =========================
set "DOSBOXX=C:\Program Files\DOSBox-X\dosbox-x.exe"

if not exist "%DOSBOXX%" (
  echo ERROR: Cannot find DOSBox-X at:
  echo        "%DOSBOXX%"
  echo        Edit DOSBOXX in tools\run_dosboxx.bat to match your install path.
  exit /b 1
)

REM =========================
REM  Project paths
REM =========================
set "PROJ=%~dp0.."
set "BUILDDIR=%PROJ%\build"
set "EXE=%BUILDDIR%\GAME.EXE"

if not exist "%EXE%" (
  echo ERROR: "%EXE%" not found.
  echo        Run tools\build.bat first.
  exit /b 1
)

REM =========================
REM  Launch DOSBox-X
REM =========================
"%DOSBOXX%" ^
  -c "mount c %BUILDDIR%" ^
  -c "c:" ^
  -c "GAME.EXE" ^
  -c "exit"