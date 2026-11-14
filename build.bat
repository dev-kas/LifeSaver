@echo off
setlocal

set "BUILD_TYPE=%~1"
set "RUN_AFTER_BUILD=0"

if "%BUILD_TYPE%"=="" (
    set "BUILD_TYPE=debug"
    set "RUN_AFTER_BUILD=1"
)

if /i "%BUILD_TYPE%"=="debug" goto debug
if /i "%BUILD_TYPE%"=="release" goto release

echo [ERROR] unknown build configuration: %BUILD_TYPE%
echo.
echo Usage:
echo   build.bat          - debug build + run ^(default^)
echo   build.bat debug    - debug build
echo   build.bat release  - release build
exit /b 1


:debug
echo [BUILD] debug configuration...

if not exist ".builds\debug" mkdir ".builds\debug"

gcc main.c workspace.c audio.c ^
    -o .builds\debug\main.exe ^
    -O0 ^
    -g ^
    -DDEBUG ^
    -D_WIN32_WINNT=0x0A00 ^
    -ldwmapi -lole32 -luuid

if errorlevel 1 (
    echo [ERROR] debug build failed
    exit /b 1
)

if "%RUN_AFTER_BUILD%"=="1" (
    echo [RUN] starting main.exe...
    .\.builds\debug\main.exe
    exit /b %errorlevel%
)

echo [SUCCESS] debug build completed
exit /b 0


:release
echo [BUILD] release configuration...

if not exist ".builds\release" mkdir ".builds\release"

gcc main.c workspace.c audio.c ^
    -o .builds\release\main.exe ^
    -O3 ^
    -DNDEBUG ^
    -Werror ^
    -Wextra ^
    -mwindows ^
    -s ^
    -D_WIN32_WINNT=0x0A00 ^
    -ldwmapi -lole32 -luuid

if errorlevel 1 (
    echo [ERROR] release build failed
    exit /b 1
)

echo [SUCCESS] release build completed
exit /b 0
