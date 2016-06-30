@echo EpgTimerSrv.exeをサービスとしてインストール→開始します
@pause
cd /d "%~dp0"
sc create "EpgTimer Service" start= auto binPath= "%cd%\EpgTimerSrv.exe" displayName= "EpgTimer Service"
sc start "EpgTimer Service"
@pause
