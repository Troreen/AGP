@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SOLUTION=AGP.sln"
set "PLATFORM=x64"
set "APP_NAME=ModelViewer.exe"
set "APP_CONFIG=Debug"

set "OUTDIR=HandIn"
set "SOLUTION_OUT=%OUTDIR%\AGP_Solution"
set "APP_OUT=%OUTDIR%\AGP_App"
set "APP_RUNTIME_DIR=%APP_OUT%\Source\Application\ModelViewer"

pushd "%~dp0"

"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -Command "if (Get-Process -Name 'ModelViewer' -ErrorAction SilentlyContinue) { exit 1 }"
if errorlevel 1 (
    echo ERROR: "%APP_NAME%" is already running. Close it from the taskbar or Task Manager before packaging.
    goto :Fail
)

call :FindMSBuild
if errorlevel 1 goto :Fail

call :BuildConfig Debug
if errorlevel 1 goto :Fail

call :BuildConfig Release
if errorlevel 1 goto :Fail

if not exist "Bin\Debug\%APP_NAME%" (
    echo.
    echo ERROR: Expected output "Bin\Debug\%APP_NAME%" was not created.
    goto :Fail
)

if not exist "Bin\Release\%APP_NAME%" (
    echo.
    echo ERROR: Expected output "Bin\Release\%APP_NAME%" was not created.
    goto :Fail
)

echo.
echo Cleaning previous hand-in folders...
call :RemoveFolder "AGP_Solution"
if errorlevel 1 goto :Fail
call :RemoveFolder "AGP_App"
if errorlevel 1 goto :Fail
call :RemoveFolder "%OUTDIR%"
if errorlevel 1 goto :Fail
mkdir "%SOLUTION_OUT%"
if errorlevel 1 goto :Fail
mkdir "%APP_RUNTIME_DIR%"
if errorlevel 1 goto :Fail

echo.
echo Creating solution hand-in folder...
robocopy "." "%SOLUTION_OUT%" /E ^
    /XD ".git" ".vs" ".agents" ".codex" "AGP_Solution" "AGP_App" "%OUTDIR%" "Intermediate" "TemporaryShaders" "x64" "x86" ^
    /XF "%~nx0" "TGECleaner.bat" ".gitignore" ".gitattributes" ".editorconfig" "*.suo" "*.user" "*.userosscache" "*.sln.docstates" "*.VC.db" "*.opendb" "*.db" "*.cache" "*.tlog" "*.lastbuildstate" "*.idb" "*.ilk" "*.ipch" "*.obj" "*.res" "*.exp" "*.log" "*.pdb" "*.tmp"
if errorlevel 8 goto :CopyFail

echo.
echo Creating app-only hand-in folder...
robocopy "Bin\%APP_CONFIG%" "%APP_RUNTIME_DIR%" /E /XF ".gitignore" ".gitattributes" "*.idb" "*.ilk" "*.ipch" "*.obj" "*.res" "*.exp" "*.log" "*.pdb" "*.tmp"
if errorlevel 8 goto :CopyFail

robocopy "Assets" "%APP_OUT%\Assets" /E /XF ".gitignore" ".gitattributes" "*.tmp"
if errorlevel 8 goto :CopyFail

robocopy "Source\Application\ModelViewer\Materials" "%APP_OUT%\Source\Application\ModelViewer\Materials" /E /XF ".gitignore" ".gitattributes" "*.tmp"
if errorlevel 8 goto :CopyFail

call :WriteLauncher
if errorlevel 1 goto :Fail

echo.
echo Done.
echo Solution hand-in: "%CD%\%SOLUTION_OUT%"
echo App-only hand-in: "%CD%\%APP_OUT%" ^(%APP_CONFIG% runtime^)
popd
exit /B 0

:RemoveFolder
if not exist "%~1" exit /B 0
attrib -R -S -H "%~1\*" /S /D >nul 2>nul
rmdir /S /Q "%~1"
if exist "%~1" (
    echo ERROR: Could not remove existing "%~1" folder. Close anything running from it and try again.
    exit /B 1
)
exit /B 0

:FindMSBuild
set "MSBUILD_EXE="

for /F "delims=" %%I in ('where msbuild.exe 2^>nul') do (
    if not defined MSBUILD_EXE set "MSBUILD_EXE=%%I"
)

if not defined MSBUILD_EXE (
    set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!VSWHERE!" (
        for /F "usebackq delims=" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
            if not defined MSBUILD_EXE set "MSBUILD_EXE=%%I"
        )
    )
)

if not defined MSBUILD_EXE (
    echo ERROR: Could not find MSBuild. Install Visual Studio Build Tools or add MSBuild to PATH.
    exit /B 1
)

echo Using MSBuild: "%MSBUILD_EXE%"
exit /B 0

:BuildConfig
echo.
echo Building %~1^|%PLATFORM%...
set "SAVED_PATH=%PATH%"
set "PATH="
set "Path=%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem"
"%MSBUILD_EXE%" "%SOLUTION%" /m /t:Build /p:Configuration=%~1 /p:Platform=%PLATFORM% /v:minimal
set "BUILD_RESULT=%ERRORLEVEL%"
set "Path=%SAVED_PATH%"
if not "%BUILD_RESULT%"=="0" (
    echo ERROR: %~1^|%PLATFORM% build failed.
    exit /B %BUILD_RESULT%
)
exit /B 0

:WriteLauncher
(
    echo @echo off
    echo pushd "%%~dp0Source\Application\ModelViewer"
    echo start "" "%APP_NAME%"
    echo popd
) > "%APP_OUT%\RunModelViewer.bat"
if not exist "%APP_OUT%\RunModelViewer.bat" (
    echo ERROR: Could not write app launcher.
    exit /B 1
)
exit /B 0

:CopyFail
echo ERROR: A file copy operation failed.
goto :Fail

:Fail
echo.
echo Hand-in package was not completed.
popd
exit /B 1
