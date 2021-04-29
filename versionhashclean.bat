@echo off
pushd "%~dp0"
set target=Common\CommonResource.h
findstr /bc:"#define EDCB_VERSION_EXTRA " %target% >nul
if %errorlevel% equ 0 (
    findstr /vbc:"#define EDCB_VERSION_EXTRA " %target% >%target%.new
    move /y %target%.new %target%
)

set target=EpgTimer\EpgTimer\AppVersion.cs
findstr /brc:" *const string VERSION_EXTRA = \"\";" %target% >nul
if %errorlevel% neq 0 (
    findstr /vbrc:" *}" /c:" *const string VERSION_EXTRA =" %target% >%target%.new
    echo         const string VERSION_EXTRA = "";>>%target%.new
    echo     }>>%target%.new
    echo }>>%target%.new
    move /y %target%.new %target%
)
popd
