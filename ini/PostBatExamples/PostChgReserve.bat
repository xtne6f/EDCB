@echo off
rem _EDCBX_DIRECT_
rem _EDCBX_#HIDE_
set
if not defined TitleOLD set TitleOLD=x
if not defined Title set Title=x
if not "%EID16%"=="FFFF" if not "%SDYYYYOLD% %SDMOLD% %SDDOLD% %STHOLD% %STMOLD% %STSOLD% %DUHOLD% %DUMOLD% %DUSOLD% %TitleOLD:"='%"=="%SDYYYY% %SDM% %SDD% %STH% %STM% %STS% %DUH% %DUM% %DUS% %Title:"='%" (
echo "Åúí«è]ÅF%ServiceNameOLD% %SDYYYYOLD%/%SDMOLD%/%SDDOLD% %STHOLD%:%STMOLD% Å` %ETHOLD%:%ETMOLD% EventID:0x%EID16OLD% Å® %SDYYYY%/%SDM%/%SDD% %STH%:%STM% Å` %ETH%:%ETM% EventID:0x%EID16% %Title:"='%"
rem | C:\Windows\Sysnative\msg.exe console
)
pause
