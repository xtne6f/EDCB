-- 名前付きパイプ(SendTSTCPの送信先:0.0.0.1 ポート:0～65535)を転送するスクリプト

-- コマンドはEDCBのToolsフォルダにあるものを優先する
ffmpeg=edcb.GetPrivateProfile('SET', 'ModulePath', '', 'Common.ini')..'\\Tools\\ffmpeg.exe'
if not edcb.FindFile(ffmpeg, 1) then ffmpeg='ffmpeg.exe' end

-- ffmpeg変換オプション
-- libvpxの例:リアルタイム変換と画質が両立するようにビットレート-bと計算量-cpu-usedを調整する
XOPT='-vcodec libvpx -b:v 896k -quality realtime -cpu-used 1 -vf yadif=0:-1:1 -s 512x288 -r 30000/1001 -acodec libvorbis -ab 128k -f webm -'
--XOPT='-vcodec libx264 -profile:v main -level 31 -b:v 896k -maxrate 4M -bufsize 4M -preset veryfast -g 120 -vf yadif=0:-1:1 -s 512x288 -r 30000/1001 -acodec aac -ab 128k -f mp4 -movflags frag_keyframe+empty_moov -'
-- 変換後の拡張子
XEXT='.webm'
--XEXT='.mp4'
-- 転送開始前に変換しておく量(bytes)
XPREPARE=nil

n=math.floor(tonumber(mg.get_var(mg.request_info.query_string, 'n')) or 0)
if n<0 then
  -- プロセスが残っていたらすべて終わらせる
  edcb.os.execute('wmic process where "name=\'ffmpeg.exe\' and commandline like \'%%SendTSTCP[_]%%[_]%%\'" call terminate >nul')
elseif n<=65535 then
  -- 前回のプロセスが残っていたら終わらせる
  edcb.os.execute('wmic process where "name=\'ffmpeg.exe\' and commandline like \'%%SendTSTCP[_]'..n..'[_]%%\'" call terminate >nul')
  -- 名前付きパイプがあれば開く
  ff=edcb.FindFile('\\\\.\\pipe\\SendTSTCP_'..n..'_*', 1)
  if ff and ff[1].name:find('^[^_]+_%d+_%d+$') then
    f=edcb.io.popen('""'..ffmpeg..'" -f mpegts -i "\\\\.\\pipe\\'..ff[1].name..'" '..XOPT..'"', 'rb')
    fname='view'..XEXT
  end
end

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
