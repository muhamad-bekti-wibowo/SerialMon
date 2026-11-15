@echo off
rem Build SerialMonitor - Qt 6 MSVC 64-bit, CMake + Ninja.
rem Opsional, lewat environment variable:
rem   QT_DIR = folder Qt, default C:\Qt\6.11.1\msvc2022_64
rem   VCVARS = path vcvars64.bat, default dicari otomatis lewat vswhere
setlocal
if "%QT_DIR%"=="" set "QT_DIR=C:\Qt\6.11.1\msvc2022_64"
if not exist "%QT_DIR%\bin\windeployqt.exe" goto noqt
if not "%VCVARS%"=="" goto havevc
set "VSW=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSW%" goto novc
for /f "usebackq delims=" %%i in (`"%VSW%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
:havevc
if not exist "%VCVARS%" goto novc
call "%VCVARS%" >nul || exit /b 1
cd /d "%~dp0"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%" || exit /b 1
cmake --build build || exit /b 1
echo.
echo Selesai: %~dp0build\SerialMonitor.exe - jalankan package.bat untuk membuat zip rilis
exit /b 0

:noqt
echo Qt tidak ditemukan di "%QT_DIR%". Set QT_DIR ke folder Qt MSVC 64-bit.
exit /b 1

:novc
echo vcvars64.bat tidak ditemukan. Pasang Visual Studio dengan workload C++ atau set VCVARS.
exit /b 1
