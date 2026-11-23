@echo off
set "app=%~dp0build\windows-debug\PolyRhythmPro_artefacts\Debug\PolyRhythm Pro.exe"
set "release=%~dp0build\windows-release\PolyRhythmPro_artefacts\Release\PolyRhythm Pro.exe"
if /i not "%~1"=="--debug" if exist "%release%" set "app=%release%"

if not exist "%app%" (
    echo PolyRhythm Pro build not found:
    echo %app%
    echo Build it in Visual Studio before launching.
    exit /b 1
)

start "" "%app%"
