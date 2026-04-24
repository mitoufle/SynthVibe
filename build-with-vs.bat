@echo off
REM Bootstraps the VS 2026 developer environment and runs build.bat.
REM Created for subagent/bash-shell use where cmake isn't on the default PATH.
setlocal
set SCRIPT_DIR=%~dp0
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "%SCRIPT_DIR%"
call "%SCRIPT_DIR%build.bat"
set BUILD_RC=%ERRORLEVEL%
endlocal & exit /b %BUILD_RC%
