@echo off
pushd "%~dp0"
for /f "usebackq tokens=*" %%i in (`git rev-parse --short^=7 HEAD`) do set git_hash=%%i
if "%git_hash%" == "" goto label9

set target=Common\CommonResource.h
find "%git_hash%" %target% >nul
if %errorlevel% neq 0 (
    echo #define EDCB_VERSION_EXTRA "[%git_hash%]">%target%.new
    findstr /vbc:"#define EDCB_VERSION_EXTRA " %target% >>%target%.new
    move /y %target%.new %target%
)

set target=EpgTimer\EpgTimer\AppVersion.cs
find "%git_hash%" %target% >nul
if %errorlevel% neq 0 (
    findstr /vbrc:" *}" /c:" *const string VERSION_EXTRA =" %target% >%target%.new
    echo         const string VERSION_EXTRA = "[%git_hash%]";>>%target%.new
    echo     }>>%target%.new
    echo }>>%target%.new
    move /y %target%.new %target%
)
:label9
popd
