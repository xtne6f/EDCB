@echo off
rem _EDCBX_DIRECT_
rem _EDCBX_#HIDE_
set
if not defined Title set Title=x
if not "%RecMode%"=="4" (
echo "Åúò^âÊäJénÅF%ServiceName% %SDYYYY%/%SDM%/%SDD% %STH%:%STM% Å` %ETH%:%ETM% %Title:"='%"
rem | C:\Windows\Sysnative\msg.exe console
)
pause
