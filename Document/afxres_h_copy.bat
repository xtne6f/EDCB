@echo off
echo This script is only for Visual Studio EXPRESS build:
pause

cd "%~dp0"
for %%i in (EpgDataCap3 EpgTimerPlugIn RecName_Macro SendTSTCP Write_AVIVO Write_Default) do (
  echo CopyTo ..\%%i\%%i\afxres.h
  > ..\%%i\%%i\afxres.h echo #pragma once
  >>..\%%i\%%i\afxres.h echo #include ^<Windows.h^>
  >>..\%%i\%%i\afxres.h echo #ifndef IDC_STATIC
  >>..\%%i\%%i\afxres.h echo #define IDC_STATIC ^(-1^)
  >>..\%%i\%%i\afxres.h echo #endif
)
pause
