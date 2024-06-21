-- 30秒ごとに文字列を出力してみるスクリプト
-- _EDCBX_NOTIFY_INTERVAL_=30
if env.NotifyID == '0' then
  WIN32 = not package.config:find('^/')
  if WIN32 then
    -- 4秒間プロンプトを表示
    edcb.os.execute('echo PostNotifyのテスト & timeout 4', true)
  else
    -- 標準出力
    edcb.os.execute('echo PostNotifyのテスト')
  end
end
