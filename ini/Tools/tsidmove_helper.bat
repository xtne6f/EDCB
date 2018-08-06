@echo 予約ファイルに含まれるTransportStreamIDの情報を変更します。
@echo チャンネル再編などでTransportStreamIDが変更されたときに使います。
@echo ChSet4.txtやChSet5.txtはあらかじめチャンネルスキャンなどで更新してください。
@echo はじめに非破壊テストを行います。
@pause
"%~dp0tsidmove.exe" --dry-run
@if errorlevel 1 goto label1

@echo.
@echo テストは正常終了しました。実際に変更を行います。
@echo 必要なら予約ファイルをバックアップしてください。
@pause
"%~dp0tsidmove.exe" --run
@if errorlevel 1 goto label1
@goto label9

:label1
@echo.
@echo エラーが発生しました。終了します。

:label9
@pause
