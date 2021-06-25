-- 名前付きパイプ(SendTSTCPの送信先:0.0.0.1 ポート:0～65535)を転送するスクリプト

-- フィルタオプション
XFILTER='-vf yadif=0:-1:1'
XFILTER_CINEMA='-vf pullup -r 24000/1001'
-- ffmpeg変換オプション($FILTERはフィルタオプションに置換。ライブストリーミング方式では -f オプション以降は上書きされる)
XOPT='-vcodec libx264 -profile:v main -level 31 -b:v 896k -maxrate 4M -bufsize 4M -preset veryfast -g 120 $FILTER -s 512x288 -acodec aac -b:a 128k -f mp4 -movflags frag_keyframe+empty_moov -'
-- NVENCの例:対応GPUが必要
--XOPT='-vcodec h264_nvenc -profile:v main -level 31 -b:v 1408k -maxrate 8M -bufsize 8M -preset medium -g 120 $FILTER -s 1280x720 -acodec aac -b:a 128k -f mp4 -movflags frag_keyframe+empty_moov -'
-- libvpxの例:リアルタイム変換と画質が両立するようにビットレート-bと計算量-cpu-usedを調整する
--XOPT='-vcodec libvpx -b:v 896k -quality realtime -cpu-used 1 $FILTER -s 512x288 -acodec libvorbis -b:a 128k -f webm -'

-- 出力バッファの量(bytes。asyncbuf.exeを用意すること。変換負荷や通信のむらを吸収する)
XBUF=0
-- 転送開始前に変換しておく量(bytes)
XPREPARE=0
-- NetworkTVモードの名前付きパイプをFindFileで見つけられない場合(EpgTimerSrvのWindowsサービス化など？)に対応するか
FIND_BY_OPEN=false

-- 変換後の拡張子
xext=XOPT:find(' %-f webm ') and '.webm' or '.mp4'

-- コマンドはEDCBのToolsフォルダにあるものを優先する
tools=edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')..'\\Tools\\'
ffmpeg=(edcb.FindFile(tools..'ffmpeg.exe',1) and tools or '')..'ffmpeg.exe'
asyncbuf=(edcb.FindFile(tools..'asyncbuf.exe',1) and tools or '')..'asyncbuf.exe'

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

post=AssertPost()
if not post then
  -- POSTでなくてもよい
  post=mg.request_info.query_string
  AssertCsrf(post)
end

audio2=GetVarInt(post,'audio2',0,1) or 0
dual=GetVarInt(post,'dual',0,2)
dual=dual==1 and ' -dual_mono_mode main' or dual==2 and ' -dual_mono_mode sub' or ''
filter=GetVarInt(post,'cinema')==1 and XFILTER_CINEMA or XFILTER
hls=ALLOW_HLS and GetVarInt(post,'hls',1)
segmentsDir=mg.script_name:gsub('[^\\/]*$','')..'segments'
n=GetVarInt(post,'n') or 0
onid,tsid,sid=(mg.get_var(post,'id') or ''):match('^(%d?%d?%d?%d?%d)%-(%d?%d?%d?%d?%d)%-(%d?%d?%d?%d?%d)$')

function OpenTranscoder(pipeName,nwtvclose)
  local cmd=' -map 0:v:0 -map 0:a:'..audio2..' '..XOPT:gsub('$FILTER',filter)
  if hls then
    -- 出力指定をHLSに置換。セグメント長は既定値(2秒)なので概ねキーフレーム(4～5秒)間隔
    cmd=cmd:gsub(' %-f .*',' -f hls -hls_segment_type mpegts -hls_flags delete_segments'
      ..' -hls_segment_filename '..segmentKey..'_%%04d.m2t '..segmentKey..'.m3u8')
  elseif XBUF>0 then
    cmd=cmd..' | "'..asyncbuf..'" '..XBUF..' '..XPREPARE
  end
  cmd='"'..ffmpeg..'" -f mpegts'..dual..' -i "'..pipeName..'"'..cmd
  if hls then
    -- 極端に多く開けないようにする
    local indexCount=#(edcb.FindFile(segmentsDir..'/*.m3u8',10) or {})
    if indexCount<10 and edcb.FindFile(tools..'pwatch.bat',1) and (not nwtvclose or edcb.FindFile(tools..'nwtvclose.ps1',1)) then
      if edcb.FindFile(segmentsDir..'/loading.m2t',1) then
        -- タイムアウト回避のため仮のインデックスファイルを置く
        local f=edcb.io.open(segmentsDir..'/'..segmentKey..'.m3u8','wb')
        if f then
          f:write('#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:4\n#EXT-X-MEDIA-SEQUENCE:0\n#EXTINF:8.008000,\nloading.m2t\n')
          f:close()
        end
      end
      edcb.os.execute('cd /d "'..segmentsDir..'" && start "" /b cmd /c "'..cmd..'"')
      for i=1,100 do
        local f=edcb.io.open(segmentsDir..'/'..segmentKey..'.m3u8','rb')
        if f then
          -- インデックスファイルにアクセスがなければ終了するように監視
          -- 対応するNetworkTVモードも終了する
          edcb.os.execute('cd /d "'..segmentsDir..'" && start "" /b cmd /c "timeout 2 /nobreak >nul & "'
            ..tools..'pwatch.bat" ffmpeg.exe %%'..segmentKey..'[_]%% '..segmentKey..'.acc 10 & del '..segmentKey..'*.*'
            ..(nwtvclose and ' & powershell -NoProfile -ExecutionPolicy RemoteSigned -File "'..tools..'nwtvclose.ps1" '..nwtvclose or '')..'"')
          return f
        end
        edcb.Sleep(100)
      end
      -- 失敗。プロセスが残っていたら終わらせる
      edcb.os.execute('wmic process where "name=\'ffmpeg.exe\' and commandline like \'%%'..segmentKey..'[_]%%\'" call terminate >nul')
    end
    return nil
  end
  return edcb.io.popen('"'..cmd..'"','rb')
end

f=nil
if onid then
  onid=tonumber(onid) or 0
  tsid=tonumber(tsid) or 0
  sid=tonumber(sid) or 0
  if sid==0 then
    -- NetworkTVモードを終了
    edcb.CloseNetworkTV(n)
  elseif 0<=n and n<100 then
    if hls then
      -- クエリのハッシュをキーとし、同一キーアクセスは出力中のインデックスファイルを返す
      segmentKey=mg.md5('view:'..hls..':nwtv'..n..':'..audio2..':'..dual..':'..filter)
      f=edcb.io.open(segmentsDir..'/'..segmentKey..'.m3u8','rb')
    end
    if not f then
      openTime=os.time()
      edcb.WritePrivateProfile('NWTV','nwtv'..n..'open','@'..openTime,'Setting\\HttpPublic.ini')
      -- NetworkTVモードを開始
      ok,pid=edcb.OpenNetworkTV(2,onid,tsid,sid,n)
      if ok then
        -- 名前付きパイプができるまで待つ
        pipeName=nil
        for i=1,50 do
          ff=edcb.FindFile('\\\\.\\pipe\\SendTSTCP_*_'..pid, 1)
          if ff and ff[1].name:find('^[^_]+_%d+_%d+$') then
            pipeName='\\\\.\\pipe\\'..ff[1].name
            break
          elseif FIND_BY_OPEN then
            -- ポートを予想して開いてみる
            for j=0,9 do
              ff=edcb.io.open('\\\\.\\pipe\\SendTSTCP_'..j..'_'..pid, 'rb')
              if ff then
                ff:close()
                -- 再び開けるようになるまで少しラグがある
                edcb.Sleep(2000)
                pipeName='\\\\.\\pipe\\SendTSTCP_'..j..'_'..pid
                break
              end
            end
            if ff then break end
          end
          edcb.Sleep(200)
        end
        if pipeName then
          f=OpenTranscoder(pipeName,n..' @'..openTime)
          fname='view'..xext
        end
        if not f then
          edcb.CloseNetworkTV(n)
        end
      end
    end
  end
elseif n<0 then
  -- プロセスが残っていたらすべて終わらせる
  edcb.os.execute('wmic process where "name=\'ffmpeg.exe\' and commandline like \'%%SendTSTCP[_]%%[_]%%\'" call terminate >nul')
elseif n<=65535 then
  if hls then
    -- クエリのハッシュをキーとし、同一キーアクセスは出力中のインデックスファイルを返す
    segmentKey=mg.md5('view:'..hls..':'..n..':'..audio2..':'..dual..':'..filter)
    f=edcb.io.open(segmentsDir..'/'..segmentKey..'.m3u8','rb')
  end
  if not f then
    -- 前回のプロセスが残っていたら終わらせる
    edcb.os.execute('wmic process where "name=\'ffmpeg.exe\' and commandline like \'%%SendTSTCP[_]'..n..'[_]%%\'" call terminate >nul')
    -- 名前付きパイプがあれば開く
    ff=edcb.FindFile('\\\\.\\pipe\\SendTSTCP_'..n..'_*', 1)
    if ff and ff[1].name:find('^[^_]+_%d+_%d+$') then
      f=OpenTranscoder('\\\\.\\pipe\\'..ff[1].name)
      fname='view'..xext
    end
  end
end

if not f then
  ct=CreateContentBuilder()
  ct:Append('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">\n'
    ..'<title>view.lua</title><p><a href="index.html">メニュー</a></p>')
  ct:Finish()
  mg.write(ct:Pop(Response(404,'text/html','utf-8',ct.len)..'\r\n'))
elseif hls then
  -- アクセスを記録してインデックスファイルを返す
  ct=CreateContentBuilder()
  ct:Append((f:read('*a') or ''):gsub('[0-9A-Za-z_]+%.m2t','segments/%0'))
  f:close()
  f=edcb.io.open(segmentsDir..'/'..segmentKey..'.acc','wb')
  if f then
    now=os.date('!*t')
    f:write(''..(100000+(now.hour*60+now.min)*60+now.sec))
    f:close()
  end
  ct:Finish()
  mg.write(ct:Pop(Response(200,'application/vnd.apple.mpegurl','utf-8',ct.len)..'\r\n'))
else
  mg.write(Response(200,mg.get_mime_type(fname))..'Content-Disposition: filename='..fname..'\r\n\r\n')
  if mg.request_info.request_method~='HEAD' then
    while true do
      buf=f:read(48128)
      if buf and #buf~=0 then
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
  end
  f:close()
  if onid then
    -- リロード時などの終了を防ぐ。厳密にはロックなどが必要だが概ねうまくいけば良い
    if edcb.GetPrivateProfile('NWTV','nwtv'..n..'open','@'..openTime,'Setting\\HttpPublic.ini')=='@'..openTime then
      -- NetworkTVモードを終了
      edcb.CloseNetworkTV(n)
    end
  end
end
