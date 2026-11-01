@echo off
setlocal
pushd "%~dp0"
if not exist dist\gog mkdir dist\gog
for %%F in (ColdFearModern.exe ColdFearThirdPerson.exe ColdFearModernControls.dll README.md INSTALL.md CONTROLS.md LICENSE.md) do copy /y dist\%%F dist\gog\ >nul
for %%F in (Enable Disable Toggle Status) do copy /y scripts\%%F.cmd dist\gog\ >nul
copy /y config\Camera.ini dist\gog\Camera.ini >nul
popd
exit /b 0
:failed
popd
exit /b 1
