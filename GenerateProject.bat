@echo off
setlocal

pushd "%~dp0" || exit /b 1

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "Premake\EnsurePremake.ps1"
if errorlevel 1 goto :Fail

"Premake\premake5.exe" vs2022
if errorlevel 1 goto :Fail

popd
exit /b 0

:Fail
echo ERROR: Project generation failed.
popd
exit /b 1
