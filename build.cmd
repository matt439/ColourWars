@echo off
rem Double-click to build and start ColourWars, or run "build.cmd -Preset x64-debug"
rem from a prompt. See scripts\build.ps1 for what it does and its other options.
setlocal
if "%~1"=="" goto double_click
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build.ps1" %*
exit /b %errorlevel%

:double_click
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build.ps1" -Run
set "status=%errorlevel%"
rem A double-click has no arguments: keep the window open to read the result.
pause
exit /b %status%
