@echo off
setlocal EnableExtensions

rem Build USBPcap with the installed Windows 11 WDK and package it for
rem Windows Server 2025 (x64 and ARM64). The driver is compiled against
rem NTDDI_WIN11_GE, which is the Windows 11 24H2 / Server 2025 DDI.

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo vswhere.exe was not found. Install Visual Studio 2026 with the Windows Driver Kit component.
    exit /b 1
)

set "VSINSTALL="
"%VSWHERE%" -latest -version "[18.0,19.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\usbpcap-vsinstall.txt"
if errorlevel 1 (
    echo Visual Studio 2026 with the MSVC toolset was not found.
    exit /b 1
)
set /p VSINSTALL=<"%TEMP%\usbpcap-vsinstall.txt"
if not defined VSINSTALL (
    echo Visual Studio 2026 with the MSVC toolset was not found.
    exit /b 1
)

call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -no_logo -arch=amd64 -host_arch=amd64
if errorlevel 1 exit /b 1

set "KITBIN="
for /f "delims=" %%D in ('dir /b /ad /o-n "%ProgramFiles(x86)%\Windows Kits\10\bin\10.*"') do (
    if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\%%D\x86\inf2cat.exe" (
        set "KITBIN=%ProgramFiles(x86)%\Windows Kits\10\bin\%%D\x86"
        goto :kit_found
    )
)
echo inf2cat.exe was not found. Install the Windows SDK and WDK that match Visual Studio 2026.
exit /b 1

:kit_found
echo Using kit tools from %KITBIN%

set "ROOT=%~dp0"
msbuild "%ROOT%USBPcap.sln" /nologo /m /v:m /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto :failed
msbuild "%ROOT%USBPcap.sln" /nologo /m /v:m /p:Configuration=Release /p:Platform=ARM64
if errorlevel 1 goto :failed

call :package x64 amd64 "Server2025_X64,10_GE_X64,10_25H2_X64" USBPcapamd64.cat
if errorlevel 1 goto :failed
call :package ARM64 arm64 "Server2025_ARM64,10_GE_ARM64,10_25H2_ARM64" USBPcaparm64.cat
if errorlevel 1 goto :failed

echo.
echo Packages are in Release\Server2025\x64 and Release\Server2025\ARM64.
echo They are not production-signed. Enable test signing to load them, or
echo submit the package for attestation signing.
exit /b 0

:package
set "OUT=%ROOT%Release\Server2025\%~1"
if not exist "%OUT%\USBPcap.sys" (
    echo Missing %OUT%\USBPcap.sys
    exit /b 1
)
copy /Y "%ROOT%USBPcapDriver\USBPcap.inx" "%OUT%\USBPcap.inf" >nul
if errorlevel 1 exit /b 1
"%KITBIN%\stampinf.exe" -f "%OUT%\USBPcap.inf" -d "*" -a %~2 -v 1.5.4.0
if errorlevel 1 exit /b 1
"%KITBIN%\inf2cat.exe" /driver:"%OUT%" /os:%~3
if errorlevel 1 exit /b 1
if not exist "%OUT%\%~4" (
    echo inf2cat did not create %OUT%\%~4
    exit /b 1
)
exit /b 0

:failed
echo ===== BUILD FAILED! =====
exit /b 1
