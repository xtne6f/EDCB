@echo off
rem _EDCBX_DIRECT_
rem _EDCBX_#HIDE_
set
if not defined ReserveComment set ReserveComment=x
if not defined Title set Title=x
if "%ReserveComment:~0,3%"=="EPG" (
echo "Åúó\ñÒí«â¡ÅF%ServiceName% %SDYYYY%/%SDM%/%SDD% %STH%:%STM% Å` %ETH%:%ETM% %Title:"='%"
rem | C:\Windows\Sysnative\msg.exe console
)
pause
