@echo off
rem 予約情報更新のタイミングでしょぼいカレンダーに予約をアップロードするサンプルバッチ
rem _EDCBX_DIRECT_
rem _EDCBX_#HIDE_ (#を取り除くとウィンドウ非表示になる)

rem 常にx86版のpowershellを使用する(EpgTimer.exeのアーキテクチャに合わせる)
set PsPath="powershell"
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set PsPath="%SystemRoot%\SysWOW64\WindowsPowerShell\v1.0\powershell"

rem 直接実行か予約情報更新のときだけ
if not defined NotifyID set NotifyID=2
if "%NotifyID%"=="2" (
  echo しょぼいカレンダーに予約をアップロードします。
  %PsPath% -NoProfile -ExecutionPolicy RemoteSigned -File ".\EdcbSchUploader.ps1" 【ユーザIDとパスワードをここに指定】

  rem 実戦投入時はこのpauseを取り除く
  pause
)
