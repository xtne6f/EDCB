-- ローカルUDPポート1234～1243またはTCPポート22230～22239を転送するスクリプト

-- ffmpeg変換オプション
-- ※UDPの場合はローカルでの安定した送受信も求められるので、システムに余力を残して調整すべき
-- libvpxの例:リアルタイム変換と画質が両立するようにビットレート-bと計算量-cpu-usedを調整する
XOPT='-vcodec libvpx -b 896k -quality realtime -cpu-used 1 -vf yadif=0:-1:1 -s 512x288 -r 15000/1001 -acodec libvorbis -ab 128k -f webm -'
-- 変換後の拡張子
XEXT='.webm'
-- 転送開始前に変換しておく量(bytes)
XPREPARE=nil

n=tonumber(mg.get_var(mg.request_info.query_string, 'n')) or 0
proto=n>=10 and 'tcp' or 'udp'
port=n>=10 and 22230+math.min(n-10, 9) or 1234+math.max(n, 0)
f=io.popen(
  -- 前回のプロセスが残っていたら終わらせる
  'wmic process where "name=\'ffmpeg.exe\' and commandline like \'%%'..proto..'://127.0.0.1:'..port..'%%\'" call terminate >nul & '..
  'ffmpeg.exe -f mpegts -i "'..proto..'://127.0.0.1:'..port..'?timeout=4000000'..(
    proto=='tcp' and '&listen=1&recv_buffer_size=481280&listen_timeout=4000' or '&pkt_size=48128&fifo_size=100000&overrun_nonfatal=1'
  )..'" '..XOPT, 'rb')
fname='chunk'..XEXT

if not f then
  mg.write('HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n')
else
  mg.write('HTTP/1.1 200 OK\r\nContent-Type: '..mg.get_mime_type(fname)..'\r\nContent-Disposition: filename='..fname..'\r\nConnection: close\r\n\r\n')
  while true do
    buf=f:read(XPREPARE or 48128)
    XPREPARE=nil
    if buf and #buf ~= 0 then
      if not mg.write(buf) then
        -- キャンセルされた
        mg.cry('canceled')
        break
      end
    else
      -- 終端に達した
      mg.cry('end')
      break
    end
  end
  f:close()
end
