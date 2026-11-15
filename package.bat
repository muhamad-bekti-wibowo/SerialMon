@echo off
rem Membuat satu file rilis: dist\SerialMon-v<versi>-win64.zip (exe + DLL Qt yang dibutuhkan, tanpa instalasi)
setlocal
if "%QT_DIR%"=="" set "QT_DIR=C:\Qt\6.11.1\msvc2022_64"
set VER=1.0.0
cd /d "%~dp0"
call "%~dp0build.bat" || exit /b 1
if exist dist rmdir /s /q dist
mkdir dist\SerialMon || exit /b 1
copy /y build\SerialMonitor.exe dist\SerialMon\ >nul || exit /b 1
"%QT_DIR%\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler ^
  --skip-plugin-types generic,networkinformation,tls,iconengines ^
  --exclude-plugins qgif,qjpeg,qpdf,qsvgicon ^
  dist\SerialMon\SerialMonitor.exe || exit /b 1
rem Runtime MSVC app-local, supaya jalan di PC tanpa Visual C++ Redistributable
for %%D in (vcruntime140.dll vcruntime140_1.dll msvcp140.dll msvcp140_1.dll msvcp140_2.dll) do if exist "%SystemRoot%\System32\%%D" copy /y "%SystemRoot%\System32\%%D" dist\SerialMon\ >nul
copy /y LICENSE dist\SerialMon\ >nul
copy /y README.md dist\SerialMon\ >nul
powershell -NoProfile -Command "Compress-Archive -Path dist\SerialMon -DestinationPath dist\SerialMon-v%VER%-win64.zip -Force" || exit /b 1
call "%~dp0portable\build_portable.bat" %VER% || exit /b 1
echo.
echo Selesai: %~dp0dist\SerialMon-v%VER%-win64.zip dan SerialMon-v%VER%-portable.exe
