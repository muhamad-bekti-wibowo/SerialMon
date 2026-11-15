@echo off
rem Build SerialMonitor (Qt 6.11 MSVC 2022 64-bit). Hasil: build\SerialMonitor.exe (+ DLL Qt via windeployqt)
setlocal
set QT=C:\Qt\6.11.1\msvc2022_64
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT% || exit /b 1
cmake --build build || exit /b 1
"%QT%\bin\windeployqt.exe" --release --no-translations build\SerialMonitor.exe || exit /b 1
echo.
echo Selesai: %~dp0build\SerialMonitor.exe
