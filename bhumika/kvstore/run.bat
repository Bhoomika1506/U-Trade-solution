@echo off
REM =====================================================
REM    KVStore - Complete Build, Setup & Run Script
REM    Single file handles everything:
REM    - Checks/installs CMake if needed
REM    - Checks/installs C++ compiler if needed
REM    - Builds the project
REM    - Runs the server
REM =====================================================

setlocal enabledelayedexpansion

echo.
echo ========================================
echo     KVStore - Complete Setup & Run
echo ========================================
echo.

REM ===== STEP 1: CHECK/INSTALL CMAKE =====
echo [STEP 1] Checking for CMake...
where cmake >nul 2>nul
if !errorlevel! equ 0 (
    for /f "tokens=3" %%i in ('cmake --version ^| findstr /R "cmake version"') do set CMAKE_VER=%%i
    echo   ✓ CMake !CMAKE_VER! found
) else (
    echo   ✗ CMake not found - Installing...
    call :install_cmake
    if !errorlevel! neq 0 (
        echo   ERROR: CMake installation failed
        pause
        exit /b 1
    )
    echo   ✓ CMake installed successfully
)

REM ===== STEP 2: CHECK/INSTALL C++ COMPILER =====
echo.
echo [STEP 2] Checking for C++ Compiler...
where cl >nul 2>nul
if !errorlevel! equ 0 (
    echo   ✓ MSVC compiler found
) else (
    where g++ >nul 2>nul
    if !errorlevel! equ 0 (
        echo   ✓ GCC compiler found
    ) else (
        echo   ✗ No C++ compiler found - Installing Visual Studio Build Tools...
        call :install_vs_buildtools
        if !errorlevel! neq 0 (
            echo   WARNING: Could not auto-install Build Tools
            echo   Please install from: https://visualstudio.microsoft.com/downloads/
            echo   (Look for "Build Tools for Visual Studio 2022")
            pause
        )
    )
)

REM ===== STEP 3: BUILD PROJECT =====
echo.
echo [STEP 3] Building KVStore Project...
echo.

if not exist build mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
if !errorlevel! neq 0 (
    echo ERROR: CMake configuration failed
    cd ..
    pause
    exit /b 1
)

cmake --build . --config Release
if !errorlevel! neq 0 (
    echo ERROR: Build failed
    cd ..
    pause
    exit /b 1
)

cd ..

REM Find executable
set "EXE_PATH="
if exist "%CD%\build\bin\Release\kvstore.exe" set "EXE_PATH=%CD%\build\bin\Release\kvstore.exe"
if exist "%CD%\build\bin\kvstore.exe" set "EXE_PATH=%CD%\build\bin\kvstore.exe"
if exist "%CD%\build\Release\kvstore.exe" set "EXE_PATH=%CD%\build\Release\kvstore.exe"
if exist "%CD%\build\kvstore.exe" set "EXE_PATH=%CD%\build\kvstore.exe"

if "!EXE_PATH!"=="" (
    echo ERROR: Could not find kvstore executable after build
    echo Build output directories:
    if exist build dir /s build
    pause
    exit /b 1
)

echo ✓ Build successful!
echo   Executable: !EXE_PATH!
echo.

REM ===== STEP 4: RUN PROJECT =====
echo [STEP 4] Starting KVStore Server...
echo.
echo Choose operation mode:
echo   1 - STDIN mode   ^(interactive, type commands directly^)
echo   2 - TCP mode     ^(server on port 6379, accept client connections^)
echo.
set /p choice="Enter choice (1 or 2, default 1): "

if "%choice%"=="" set choice=1

if "%choice%"=="2" (
    echo.
    echo Starting TCP server on 127.0.0.1:6379
    echo To connect from another terminal, use:
    echo   nc localhost 6379    ^(or telnet localhost 6379^)
    echo.
    "!EXE_PATH!" tcp 6379
) else (
    "!EXE_PATH!" stdin
)

pause
exit /b 0

REM ===== SUBROUTINES =====

:install_cmake
echo.
echo Attempting to download and install CMake...
echo.

REM Try to download CMake using PowerShell
powershell -Command "try { $ProgressPreference = 'SilentlyContinue'; (New-Object System.Net.WebClient).DownloadFile('https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0-windows-x86_64.zip', '%TEMP%\cmake.zip'); exit 0 } catch { exit 1 }"

if !errorlevel! equ 0 (
    echo Extracting CMake...
    powershell -Command "Expand-Archive -Path '%TEMP%\cmake.zip' -DestinationPath '%TEMP%' -Force"
    
    REM Set PATH to include CMake
    for /d %%i in ("%TEMP%\cmake-*-windows-x86_64") do (
        set "CMAKE_PATH=%%i\bin"
        setx PATH "!CMAKE_PATH!;!PATH!"
    )
    
    del "%TEMP%\cmake.zip"
    exit /b 0
) else (
    echo WARNING: Could not auto-download CMake
    echo Please install CMake manually from: https://cmake.org/download/
    exit /b 1
)

:install_vs_buildtools
echo.
echo Attempting to download Visual Studio Build Tools...
echo.

REM Download VS Build Tools installer
powershell -Command "try { $ProgressPreference = 'SilentlyContinue'; (New-Object System.Net.WebClient).DownloadFile('https://aka.ms/vs/17/release/vs_BuildTools.exe', '%TEMP%\vs_BuildTools.exe'); exit 0 } catch { exit 1 }"

if !errorlevel! equ 0 (
    echo Running installer...
    "%TEMP%\vs_BuildTools.exe" --passive --wait --norestart --includeRecommended ^
        --add Microsoft.VisualStudio.Workload.VCTools ^
        --add Microsoft.VisualStudio.Component.Windows10SDK
    
    set INSTALL_RESULT=!errorlevel!
    del "%TEMP%\vs_BuildTools.exe"
    exit /b !INSTALL_RESULT!
) else (
    echo WARNING: Could not auto-download Build Tools
    exit /b 1
)

