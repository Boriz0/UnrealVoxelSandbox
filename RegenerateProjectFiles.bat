@echo off
setlocal

rem Optionally pass the Unreal Engine installation folder as the first argument.
set "EngineDir=D:\UE\UE_5.8"
if not "%~1"=="" set "EngineDir=%~1"
set "ProjectFile=%~dp0UnrealMinecraft.uproject"
set "BuildTool=%EngineDir%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"

if not exist "%ProjectFile%" (
    echo ERROR: UnrealMinecraft.uproject was not found beside this batch file.
    goto Failed
)
if not exist "%BuildTool%" (
    echo ERROR: UnrealBuildTool was not found in "%EngineDir%".
    echo Pass the engine installation folder as the first argument.
    goto Failed
)
if not exist "%EngineDir%\Engine\Build\BatchFiles\GetDotnetPath.bat" (
    echo ERROR: The engine's GetDotnetPath.bat was not found.
    goto Failed
)

echo Close Visual Studio before regenerating, then reopen UnrealMinecraft.sln.
echo Regenerating project files for "%ProjectFile%"...
call "%EngineDir%\Engine\Build\BatchFiles\GetDotnetPath.bat"
if errorlevel 1 goto Failed

dotnet "%BuildTool%" -projectfiles -project="%ProjectFile%" -game -engine
if errorlevel 1 goto Failed

echo.
echo Project files regenerated successfully. Reopen UnrealMinecraft.sln.
pause
exit /b 0

:Failed
echo.
echo Project file generation failed. Review the error above.
pause
exit /b 1
