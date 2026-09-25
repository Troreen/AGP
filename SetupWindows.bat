@echo off
setlocal

pushd "%~dp0" || exit /b 1
echo AGP Windows setup
echo.
echo Step 1 of 2: Check FMOD headers.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "Dependencies\SetupFmodSdk.ps1"
if errorlevel 1 goto :Fail

echo.
echo Step 2 of 2: Generate the Visual Studio solution.
call "GenerateProject.bat"
if errorlevel 1 goto :Fail

echo.
echo Setup is ready. Open Game.sln in Visual Studio 2026 and build Debug ^| x64.
echo Install the Desktop development with C++ workload and a Windows SDK if Visual Studio asks.
popd
exit /b 0

:Fail
echo.
echo Setup stopped. Fix the message above, then run SetupWindows.bat again.
popd
exit /b 1
