@echo off
rem 予約情報更新のタイミングでしょぼいカレンダーに予約をアップロードするサンプルバッチ
rem _EDCBX_DIRECT_
rem _EDCBX_#HIDE_ (#を取り除くとウィンドウ非表示になる)

rem v3.5以上のC#コンパイラのパスを指定する
set CSC_PATH=%SystemRoot%\Microsoft.NET\Framework\v4.0.30319\csc.exe

rem 直接実行か予約情報更新のときだけ
if not defined NotifyID set NotifyID=2
if "%NotifyID%"=="2" (
  if not exist EdcbSchUploader.exe (
    if not exist EdcbSchUploader.cs (
      echo エラー: EdcbSchUploader.cs が見つかりません。
      goto EOF
    )
    if not exist %CSC_PATH% (
      echo エラー: %CSC_PATH% が見つかりません。
      goto EOF
    )
    echo EdcbSchUploader.cs をコンパイルします。
    %CSC_PATH% /nologo /optimize EdcbSchUploader.cs
    if not exist EdcbSchUploader.exe (
      echo エラー: EdcbSchUploader.exe が見つかりません。
      goto EOF
    )
  )

  echo しょぼいカレンダーに予約をアップロードします。
  EdcbSchUploader.exe 【ユーザIDとパスワードをここに指定】

:EOF
  rem 実戦投入時はこのpauseを取り除く
  pause
)
