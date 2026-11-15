@echo off
rem Membuat exe tunggal portabel: dist\SerialMon-v<versi>-portable.exe
rem Syarat: dist\SerialMon-v<versi>-win64.zip sudah ada (jalankan package.bat). Argumen 1 = versi.
setlocal
set "VER=%~1"
if "%VER%"=="" set "VER=1.0.0"
set "ROOT=%~dp0.."
set "DIST=%ROOT%\dist"
set "WORK=%DIST%\_portable"
if not exist "%DIST%\SerialMon-v%VER%-win64.zip" goto nozip
if not "%VCVARS%"=="" goto havevc
set "VSW=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSW%" goto novc
for /f "usebackq delims=" %%i in (`"%VSW%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
:havevc
if not exist "%VCVARS%" goto novc
call "%VCVARS%" >nul || exit /b 1
if exist "%WORK%" rmdir /s /q "%WORK%"
mkdir "%WORK%" || exit /b 1
copy /y "%DIST%\SerialMon-v%VER%-win64.zip" "%WORK%\payload.zip" >nul || exit /b 1
copy /y "%ROOT%\assets\favicon.ico" "%WORK%\favicon.ico" >nul || exit /b 1
copy /y "%~dp0launcher.c" "%WORK%\" >nul
copy /y "%~dp0launcher.rc" "%WORK%\" >nul
pushd "%WORK%"
rc /nologo /fo launcher.res launcher.rc || exit /b 1
cl /nologo /O2 /MT /W3 /DAPP_VER=L\"%VER%\" /Fe:"%DIST%\SerialMon-v%VER%-portable.exe" launcher.c launcher.res user32.lib /link /SUBSYSTEM:WINDOWS || exit /b 1
popd
rmdir /s /q "%WORK%"
echo.
echo Selesai: %DIST%\SerialMon-v%VER%-portable.exe
exit /b 0

:nozip
echo dist\SerialMon-v%VER%-win64.zip belum ada. Jalankan package.bat dulu.
exit /b 1

:novc
echo vcvars64.bat tidak ditemukan. Pasang Visual Studio dengan workload C++ atau set VCVARS.
exit /b 1
