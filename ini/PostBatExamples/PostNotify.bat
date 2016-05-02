@echo off
rem 予約情報更新のタイミングでしょぼいカレンダーに予約をアップロードするサンプルバッチ
rem _EDCBX_DIRECT_
rem _EDCBX_#HIDE_ (#を取り除くとウィンドウ非表示になる)

rem 直接実行か予約情報更新のときだけ
if not defined NotifyID set NotifyID=2
if "%NotifyID%"=="2" (
  echo しょぼいカレンダーに予約をアップロードします。
  rem デフォルトの送信先は http://cal.syoboi.jp/sch_upload 。HTTPSを使う場合は↓のようにする
  rem EdcbSchUploader.exe "user" "pass" "" 0 "https://cal.syoboi.jp/sch_upload"
  EdcbSchUploader.exe 【ユーザIDとパスワードをここに指定】

  rem 実戦投入時はこのpauseを取り除く
  pause
)
