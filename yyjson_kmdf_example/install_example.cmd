@echo off
setlocal
set SCRIPT_DIR=%~dp0
set SYS_PATH=%~1
if "%SYS_PATH%"=="" set SYS_PATH=%SCRIPT_DIR%example\x64\Debug\example.sys
if not exist "%SYS_PATH%" (
  echo example.sys not found: "%SYS_PATH%"
  exit /b 1
)
sc.exe stop example >nul 2>&1
sc.exe delete example >nul 2>&1
sc.exe create example type= kernel start= demand binPath= "%SYS_PATH%"
if errorlevel 1 exit /b %errorlevel%
sc.exe start example
