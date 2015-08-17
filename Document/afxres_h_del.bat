@echo off
echo This script is only for Visual Studio EXPRESS build:
pause

cd "%~dp0"
for %%i in (EpgDataCap3 EpgTimerPlugIn RecName_Macro SendTSTCP Write_AVIVO Write_Default) do (
  echo Delete ..\%%i\%%i\afxres.h
  del ..\%%i\%%i\afxres.h
)
pause
