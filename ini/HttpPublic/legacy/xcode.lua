-- ファイルを転送するスクリプト
-- ファイルをタイムシフト再生できる: http://localhost:5510/xcode.lua?fname=video/foo.ts

-- トランスコードするかどうか(する場合はreadex.exeとffmpeg.exeを用意すること)
XCODE=false
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

-- コマンドはEDCBのToolsフォルダにあるものを優先する
tools=edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')..'\\Tools\\'
ffmpeg=(edcb.FindFile(tools..'ffmpeg.exe',1) and tools or '')..'ffmpeg.exe'
readex=(edcb.FindFile(tools..'readex.exe',1) and tools or '')..'readex.exe'
asyncbuf=(edcb.FindFile(tools..'asyncbuf.exe',1) and tools or '')..'asyncbuf.exe'

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

fpath=mg.get_var(mg.request_info.query_string,'fname')
if fpath then
  fpath=DocumentToNativePath(fpath)
  offset=GetVarInt(mg.request_info.query_string,'offset',0,100) or 0
  audio2=GetVarInt(mg.request_info.query_string,'audio2',0,1) or 0
  dual=GetVarInt(mg.request_info.query_string,'dual',0,2)
  dual=dual==1 and ' -dual_mono_mode main' or dual==2 and ' -dual_mono_mode sub' or ''
  filter=GetVarInt(mg.request_info.query_string,'cinema')==1 and XFILTER_CINEMA or XFILTER
end

f=nil
if fpath then
  fname='xcode'..(fpath:match('%.[0-9A-Za-z]+$') or '')
  fnamets='xcode'..edcb.GetPrivateProfile('SET','TSExt','.ts','EpgTimerSrv.ini'):lower()
  -- 拡張子を限定
  if fname:lower()==fnamets then
    f=edcb.io.open(fpath, 'rb')
    if f then
      fsec,fsize=GetDurationSec(f)
      if offset~=0 and offset~=100 and fsec>0 and SeekSec(f,fsec*offset/100) then
        offset=f:seek('cur',0) or 0
      else
        offset=math.floor(fsize*offset/100/188)*188
      end
      if XCODE then
        f:close()
        -- 容量確保の可能性があるときは周期188+同期語0x47(188*256+0x47=48199)で対象ファイルを終端判定する
        sync=edcb.GetPrivateProfile('SET','KeepDisk',0,'EpgTimerSrv.ini')~='0'
        f=edcb.io.popen('""'..readex..'" '..offset..(sync and ' 4p48199' or ' 4')..' "'..fpath..'" | "'
          ..ffmpeg..'"'..dual..' -i pipe:0 -map 0:v:0 -map 0:a:'..audio2..' '..XOPT:gsub('$FILTER',filter)
          ..(XBUF>0 and ' | "'..asyncbuf..'" '..XBUF..' '..XPREPARE or '')..'"', 'rb')
        fname='xcode'..XEXT
      else
        -- 容量確保には未対応
        f:seek('set', offset)
      end
    end
  end
end

if not f then
  mg.write(Response(404,'text/html','utf-8')..'\r\n'
    ..'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">\n'
    ..'<title>xcode.lua</title><p><a href="index.html">メニュー</a></p>')
else
  mg.write(Response(200,mg.get_mime_type(fname))..'Content-Disposition: filename='..fname..'\r\n\r\n')
  retry=0
  while true do
    buf=f:read(48128)
    if buf and #buf ~= 0 then
      retry=0
      if not mg.write(buf) then
        -- キャンセルされた
        mg.cry('canceled')
        break
      end
    else
      -- 終端に達した。4秒間この状態が続けば対象ファイルへの追記が終息したとみなす
      retry=retry+1
      if XCODE or retry > 20 then
        mg.cry('end')
        break
      end
      edcb.Sleep(200)
    end
  end
  f:close()
end
