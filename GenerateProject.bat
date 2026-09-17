@echo off
.\Premake\Premake5.exe vs2022

if %ERRORLEVEL% neq 0 (
    echo Premake failed!
    pause
    exit /b %ERRORLEVEL%
)