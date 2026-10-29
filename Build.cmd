@echo off
setlocal
pushd "%~dp0"
call scripts\SetupMSVC.cmd
if errorlevel 1 goto failed
if not exist build mkdir build
if not exist dist mkdir dist
cl /nologo /W4 /WX /O2 /MT /EHsc /std:c++17 /LD /Fo:build\ModernControls.obj /Fe:dist\ColdFearModernControls.dll src\ModernControls.cpp /link /DEF:src\ModernControls.def /NOIMPLIB /MACHINE:X86 /DYNAMICBASE /NXCOMPAT /Brepro
if errorlevel 1 goto failed
cl /nologo /W4 /WX /O2 /MT /EHsc /std:c++17 /Fo:build\ColdFearThirdPerson.obj /Fe:dist\ColdFearThirdPerson.exe src\ColdFearThirdPerson.cpp /link /MACHINE:X86 /DYNAMICBASE /NXCOMPAT /Brepro
if errorlevel 1 goto failed
cl /nologo /W4 /WX /O2 /MT /EHsc /std:c++17 /LD /Fo:build\DisplayPlugin.obj /Fe:dist\dinput8.dll src\DisplayPlugin.cpp /link /DEF:src\DisplayPlugin.def /NOIMPLIB /MACHINE:X86 /DYNAMICBASE /NXCOMPAT /Brepro
if errorlevel 1 goto failed
move /y dist\ColdFearModernControls.exp build\ >nul
move /y dist\dinput8.exp build\ >nul
copy /y config\*.ini dist\ >nul
if errorlevel 1 goto failed
for %%F in (Enable Disable Toggle Status) do copy /y scripts\%%F.cmd dist\ >nul
for %%F in (README.md CONTROLS.md LICENSE.md INSTALL.md) do copy /y %%F dist\ >nul
echo Built player files in dist.
popd
exit /b 0
:failed
popd
exit /b 1
