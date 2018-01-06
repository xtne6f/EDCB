@echo off
rem 予約情報更新のタイミングでしょぼいカレンダーに予約をアップロードするサンプルバッチ
rem _EDCBX_DIRECT_
rem _EDCBX_#HIDE_ (#を取り除くとウィンドウ非表示になる)

rem 直接実行か予約情報更新のときだけ
if not defined NotifyID set NotifyID=2
if "%NotifyID%"=="2" (
  echo しょぼいカレンダーに予約をアップロードします。
  powershell -NoProfile -ExecutionPolicy RemoteSigned -File ".\EdcbSchUploader.ps1" 【ユーザIDとパスワードをここに指定】

  rem 実戦投入時はこのpauseを取り除く
  pause
)
