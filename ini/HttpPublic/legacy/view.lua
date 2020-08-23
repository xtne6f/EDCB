-- 名前付きパイプ(SendTSTCPの送信先:0.0.0.1 ポート:0～65535)を転送するスクリプト

-- フィルタオプション
XFILTER='-vf yadif=0:-1:1'
XFILTER_CINEMA='-vf pullup -r 24000/1001'
-- ffmpeg変換オプション($FILTERはフィルタオプションに置換)
-- libvpxの例:リアルタイム変換と画質が両立するようにビットレート-bと計算量-cpu-usedを調整する
XOPT='-vcodec libvpx -b:v 896k -quality realtime -cpu-used 1 $FILTER -s 512x288 -acodec libvorbis -ab 128k -f webm -'
--XOPT='-vcodec libx264 -profile:v main -level 31 -b:v 896k -maxrate 4M -bufsize 4M -preset veryfast -g 120 $FILTER -s 512x288 -acodec aac -ab 128k -f mp4 -movflags frag_keyframe+empty_moov -'
--XOPT='-vcodec h264_nvenc -profile:v main -level 31 -b:v 1408k -maxrate 8M -bufsize 8M -preset medium -g 120 $FILTER -s 1280x720 -acodec aac -ab 128k -f mp4 -movflags frag_keyframe+empty_moov -'
-- 変換後の拡張子
XEXT='.webm'
--XEXT='.mp4'
-- 出力バッファの量(bytes。asyncbuf.exeを用意すること。変換負荷や通信のむらを吸収する)
XBUF=0
-- 転送開始前に変換しておく量(bytes)
XPREPARE=0
-- NetworkTVモードの名前付きパイプをFindFileで見つけられない場合(EpgTimerSrvのWindowsサービス化など？)に対応するか
FIND_BY_OPEN=false

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
if true then
  audio2=GetVarInt(post,'audio2',0,1) or 0
  dual=GetVarInt(post,'dual',0,2)
  dual=dual==1 and ' -dual_mono_mode main' or dual==2 and ' -dual_mono_mode sub' or ''
  filter=GetVarInt(post,'cinema')==1 and XFILTER_CINEMA or XFILTER
  n=GetVarInt(post,'n') or 0
  onid,tsid,sid=(mg.get_var(post,'id') or ''):match('^(%d?%d?%d?%d?%d)%-(%d?%d?%d?%d?%d)%-(%d?%d?%d?%d?%d)$')
  if onid then
    onid=tonumber(onid) or 0
    tsid=tonumber(tsid) or 0
    sid=tonumber(sid) or 0
    if sid==0 then
      -- NetworkTVモードを終了
      edcb.CloseNetworkTV(n)
    else
      -- NetworkTVモードを開始
      ok,pid=edcb.OpenNetworkTV(2,onid,tsid,sid,n)
      if ok then
        -- 名前付きパイプができるまで待つ
        for i=1,50 do
          ff=edcb.FindFile('\\\\.\\pipe\\SendTSTCP_*_'..pid, 1)
          if ff and ff[1].name:find('^[^_]+_%d+_%d+$') then
            f=edcb.io.popen('""'..ffmpeg..'" -f mpegts'..dual..' -i "\\\\.\\pipe\\'..ff[1].name..'" -map 0:v:0 -map 0:a:'..audio2..' '..XOPT:gsub('$FILTER',filter)
              ..(XBUF>0 and ' | "'..asyncbuf..'" '..XBUF..' '..XPREPARE or '')..'"', 'rb')
            fname='view'..XEXT
            break
          elseif FIND_BY_OPEN then
            -- ポートを予想して開いてみる
            for j=0,9 do
              ff=edcb.io.open('\\\\.\\pipe\\SendTSTCP_'..j..'_'..pid, 'rb')
              if ff then
                ff:close()
                -- 再び開けるようになるまで少しラグがある
                edcb.Sleep(4000)
                f=edcb.io.popen('""'..ffmpeg..'" -f mpegts'..dual..' -i "\\\\.\\pipe\\SendTSTCP_'..j..'_'..pid..'" -map 0:v:0 -map 0:a:'..audio2..' '..XOPT:gsub('$FILTER',filter)
                  ..(XBUF>0 and ' | "'..asyncbuf..'" '..XBUF..' '..XPREPARE or '')..'"', 'rb')
                fname='view'..XEXT
                break
              end
            end
            if ff then break end
          end
          edcb.Sleep(200)
        end
        if not f then
          edcb.CloseNetworkTV(n)
        end
      end
    end
  elseif n<0 then
    -- プロセスが残っていたらすべて終わらせる
    edcb.os.execute('wmic process where "name=\'ffmpeg.exe\' and commandline like \'%%SendTSTCP[_]%%[_]%%\'" call terminate >nul')
  elseif n<=65535 then
    -- 前回のプロセスが残っていたら終わらせる
    edcb.os.execute('wmic process where "name=\'ffmpeg.exe\' and commandline like \'%%SendTSTCP[_]'..n..'[_]%%\'" call terminate >nul')
    -- 名前付きパイプがあれば開く
    ff=edcb.FindFile('\\\\.\\pipe\\SendTSTCP_'..n..'_*', 1)
    if ff and ff[1].name:find('^[^_]+_%d+_%d+$') then
      f=edcb.io.popen('""'..ffmpeg..'" -f mpegts'..dual..' -i "\\\\.\\pipe\\'..ff[1].name..'" -map 0:v:0 -map 0:a:'..audio2..' '..XOPT:gsub('$FILTER',filter)
        ..(XBUF>0 and ' | "'..asyncbuf..'" '..XBUF..' '..XPREPARE or '')..'"', 'rb')
      fname='view'..XEXT
    end
  end
end

if not f then
  mg.write(Response(404,'text/html','utf-8')..'\r\n'
    ..'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">\n'
    ..'<title>view.lua</title><p><a href="index.html">メニュー</a></p>')
else
  mg.write(Response(200,mg.get_mime_type(fname))..'Content-Disposition: filename='..fname..'\r\n\r\n')
  while true do
    buf=f:read(48128)
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
  if onid then
    -- NetworkTVモードを終了
    edcb.CloseNetworkTV(n)
  end
end
