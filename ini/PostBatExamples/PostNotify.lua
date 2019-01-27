-- 30秒ごとにプロンプトを表示してみるスクリプト
-- _EDCBX_NOTIFY_INTERVAL_=30
if env.NotifyID == '0' then
  edcb.os.execute('echo PostNotifyのテスト & ping -n 4 127.0.0.1 >nul', true)
end
