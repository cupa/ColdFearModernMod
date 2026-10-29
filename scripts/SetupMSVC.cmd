@echo off
if /i "%VSCMD_ARG_TGT_ARCH%"=="x86" (
    where cl >nul 2>&1
    if not errorlevel 1 exit /b 0
)
set "modVswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%modVswhere%" (
    echo Install Visual Studio Build Tools with Desktop development with C++.
    exit /b 1
)
set "modVisualStudio="
for /f "usebackq tokens=*" %%I in (`"%modVswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "modVisualStudio=%%I"
if not defined modVisualStudio (
    echo No MSVC x86 toolchain found.
    exit /b 1
)
call "%modVisualStudio%\VC\Auxiliary\Build\vcvarsall.bat" x86
exit /b %errorlevel%
