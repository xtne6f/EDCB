#!/bin/bash
# 予約情報更新のタイミングでしょぼいカレンダーに予約をアップロードするサンプルスクリプト

# 直接実行か予約情報更新のときだけ
if [ -z "$NotifyID" -o "$NotifyID" = 2 ]; then
  echo しょぼいカレンダーに予約をアップロードします。
  python3 EdcbSchUploader.py 【ユーザIDとパスワードをここに指定】
fi
