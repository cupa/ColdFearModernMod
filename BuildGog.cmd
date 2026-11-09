@echo off
setlocal
pushd "%~dp0"
call scripts\SetupMSVC.cmd
if errorlevel 1 goto failed
if not exist dist\gog mkdir dist\gog
cl /nologo /W4 /WX /O2 /MT /EHsc /std:c++17 /LD /Fo:build\GogStartup.obj /Fe:dist\gog\d3d9.dll src\GogStartup.cpp /link /DEF:src\GogStartup.def /NOIMPLIB /MACHINE:X86 /DYNAMICBASE /NXCOMPAT /Brepro
if errorlevel 1 goto failed
move /y dist\gog\d3d9.exp build\ >nul
for %%F in (ColdFearThirdPerson.exe ColdFearModernControls.dll README.md INSTALL.md CONTROLS.md LICENSE.md) do (
    copy /y dist\%%F dist\gog\ >nul
    if errorlevel 1 goto failed
)
for %%F in (Enable Disable Toggle Status) do copy /y scripts\%%F.cmd dist\gog\ >nul
copy /y config\Camera.ini dist\gog\Camera.ini >nul
popd
exit /b 0
:failed
popd
exit /b 1
