@echo off
rem Process watchdog script

rem Process name
if "%~1"=="" goto label9
rem Process commandline like
if "%~2"=="" goto label9
rem Last-access-time filename. The file must contain the number of 100000+(hour*60+minute)*60+second
if "%~3"=="" goto label9
rem The allowed difference for last-access-time
if "%~4"=="" goto label9

:label0
timeout 2 /nobreak >nul
wmic process where "name='%~1' and commandline like '%~2'" get processid 2>nul | findstr /b [1-9] >nul
if %errorlevel% neq 0 goto label9

set acc=
for /f "tokens=1" %%i in (%~3) do set acc=%%i
if not defined acc goto label0
if %acc% lss 100000 goto label0
if %acc% geq 186400 goto label0

for /f %%i in ('wmic path win32_utctime get /format:list ^| find "="') do set %%i
if not defined hour goto label9
if not defined minute goto label9
if not defined second goto label9

set /a accdiff=((hour*60+minute)*60+second+86400-(acc-100000))%%86400
if %accdiff% lss %~4 goto label0
if %accdiff% geq 86300 goto label0

wmic process where "name='%~1' and commandline like '%~2'" call terminate >nul
timeout 2 /nobreak >nul

:label9
