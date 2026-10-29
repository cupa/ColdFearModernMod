@echo off
setlocal
pushd "%~dp0"
call scripts\SetupMSVC.cmd
if errorlevel 1 goto failed
if not exist build mkdir build
for %%T in (Controls Controller CursorConfinement Display) do (
    cl /nologo /W4 /WX /O2 /MT /EHsc /std:c++17 /Fo:build\Test%%T.obj /Fe:build\Test%%T.exe src\Test%%T.cpp /link /MACHINE:X86 /DYNAMICBASE /NXCOMPAT
    if errorlevel 1 goto failed
    build\Test%%T.exe
    if errorlevel 1 goto failed
)
echo All four test suites passed.
popd
exit /b 0
:failed
popd
exit /b 1
