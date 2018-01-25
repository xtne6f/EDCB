@echo EpgTimerSrv.exeをサービスとしてインストール→開始します
@set /p x="ログオンアカウントを選択 (1=LocalSystem, 2=LocalService) : "
@if "%x%"=="1" goto label1
@if "%x%"=="2" goto label2
@goto label9

:label1
@echo.
@echo LocalSystemアカウントにはとても強い権限があります
@echo バグの影響が大きくなりうること、権限昇格の踏み台として利用できることに注意してください
@pause
cd /d "%~dp0"
sc create "EpgTimer Service" start= auto binPath= "%cd%\EpgTimerSrv.exe"
sc start "EpgTimer Service"
@pause
@goto label9

:label2
@echo.
@echo LocalServiceアカウントがEDCBや録画保存フォルダにアクセスできるよう注意してください
@pause
cd /d "%~dp0"
sc create "EpgTimer Service" start= auto binPath= "%cd%\EpgTimerSrv.exe" obj= "NT AUTHORITY\LocalService"
sc sidtype "EpgTimer Service" unrestricted
sc start "EpgTimer Service"
@pause
@goto label9

:label9
