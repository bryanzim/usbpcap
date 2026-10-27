@echo off
setlocal EnableExtensions
cd /d %~dp0

rem Build the cabinet files for a Hardware Dev Center attestation submission.
rem Microsoft applies the attestation signature after the cabinet is signed
rem with an EV code signing certificate and uploaded to Partner Center.
rem Attestation signatures are for Windows 10 and Windows 11 desktop.
rem Windows Server 2016 and later do not load an attestation-signed filter.

set "VERSION=1.5.4.0"
set "OUT=%~dp0Release\attestation"
if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%"

call :CreateCab x64 USBPcapamd64.cat
if errorlevel 1 goto :failed
call :CreateCab ARM64 USBPcaparm64.cat
if errorlevel 1 goto :failed

if exist "%~dp0setup.inf" del /q "%~dp0setup.inf"
if exist "%~dp0setup.rpt" del /q "%~dp0setup.rpt"
if exist "%~dp0disk1" rmdir /s /q "%~dp0disk1"

echo.
echo Attestation cabinets are in Release\attestation.
echo Sign each cabinet with an EV code signing certificate, then submit it
echo in Partner Center. Leave the dashboard test-signing options unchecked.
echo Microsoft replaces the catalog and returns the signed driver package.
exit /b 0

:CreateCab
set "ARCH=%~1"
set "CATNAME=%~2"
set "SRC=%~dp0Release\Server2025\%ARCH%"
set "STAGE=%TEMP%\usbpcap-attest-%ARCH%"
if not exist "%SRC%\USBPcap.sys" (
    echo Missing %SRC%\USBPcap.sys. Run build_server2025.bat first.
    exit /b 1
)
if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%"
copy /Y "%SRC%\USBPcap.inf" "%STAGE%\USBPcap.inf" >nul
copy /Y "%SRC%\USBPcap.sys" "%STAGE%\USBPcap.sys" >nul
copy /Y "%SRC%\USBPcap.pdb" "%STAGE%\USBPcap.pdb" >nul
copy /Y "%SRC%\usbpcap*.cat" "%STAGE%\%CATNAME%" >nul
if errorlevel 1 exit /b 1

(
echo .OPTION EXPLICIT
echo .Set CabinetFileCountThreshold=0
echo .Set FolderFileCountThreshold=0
echo .Set FolderSizeThreshold=0
echo .Set MaxCabinetSize=0
echo .Set MaxDiskFileCount=0
echo .Set MaxDiskSize=0
echo .Set CompressionType=MSZIP
echo .Set Cabinet=on
echo .Set Compress=on
echo .Set CabinetNameTemplate=USBPcap-%VERSION%-%ARCH%.cab
echo .Set DiskDirectoryTemplate=%OUT%
echo .Set DestinationDir=USBPcap%ARCH%
echo %STAGE%\USBPcap.inf
echo %STAGE%\USBPcap.sys
echo %STAGE%\USBPcap.pdb
echo %STAGE%\%CATNAME%
) > "%STAGE%\package.ddf"

MakeCab /F "%STAGE%\package.ddf"
if errorlevel 1 exit /b 1
if not exist "%OUT%\USBPcap-%VERSION%-%ARCH%.cab" (
    echo MakeCab did not create USBPcap-%VERSION%-%ARCH%.cab
    exit /b 1
)
exit /b 0

:failed
echo ===== ATTESTATION PACKAGE FAILED! =====
exit /b 1
