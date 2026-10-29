@echo off
setlocal
pushd "%~dp0"
call scripts\SetupMSVC.cmd
if errorlevel 1 goto failed
if not defined MOD_PYTHON set "MOD_PYTHON=python"
if not exist build mkdir build
if not exist packages mkdir packages
"%MOD_PYTHON%" setup\Prepare.py --uninstaller
if errorlevel 1 goto failed
rc /nologo /fo build\Uninstall.res setup\Setup.rc
if errorlevel 1 goto failed
cl /nologo /W4 /WX /wd4505 /O2 /MT /EHsc /std:c++17 /DUNICODE /D_UNICODE /DUNINSTALLER /Fo:build\Uninstall.obj /Fe:dist\Uninstall.exe setup\Setup.cpp build\Uninstall.res /link /SUBSYSTEM:WINDOWS /MACHINE:X86 /DYNAMICBASE /NXCOMPAT /Brepro user32.lib shell32.lib advapi32.lib ole32.lib bcrypt.lib
if errorlevel 1 goto failed
"%MOD_PYTHON%" setup\Prepare.py
if errorlevel 1 goto failed
if not exist build mkdir build
if not exist packages mkdir packages
rc /nologo /fo build\Setup.res setup\Setup.rc
if errorlevel 1 goto failed
cl /nologo /W4 /WX /O2 /MT /EHsc /std:c++17 /DUNICODE /D_UNICODE /Fo:build\Setup.obj /Fe:packages\ColdFearModernThirdPerson-0.6.14-Setup.exe setup\Setup.cpp build\Setup.res /link /SUBSYSTEM:WINDOWS /MACHINE:X86 /DYNAMICBASE /NXCOMPAT /Brepro user32.lib shell32.lib advapi32.lib ole32.lib bcrypt.lib
if errorlevel 1 goto failed
packages\ColdFearModernThirdPerson-0.6.14-Setup.exe --verify
if errorlevel 1 goto failed
cl /nologo /W4 /WX /O2 /MT /EHsc /std:c++17 /DUNICODE /D_UNICODE /Fo:build\TestSetup.obj /Fe:build\TestSetup.exe setup\TestSetup.cpp build\Setup.res /link /MACHINE:X86 /DYNAMICBASE /NXCOMPAT user32.lib shell32.lib advapi32.lib ole32.lib bcrypt.lib
if errorlevel 1 goto failed
build\TestSetup.exe
if errorlevel 1 goto failed
echo Installer built and verified.
popd
exit /b 0
:failed
popd
exit /b 1
