@echo EpgTimer Serviceを停止→アンインストールします
@pause
sc stop "EpgTimer Service"
sc delete "EpgTimer Service"
@pause
