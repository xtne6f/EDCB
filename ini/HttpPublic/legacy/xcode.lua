-- ファイルを転送するスクリプト
-- ファイルをタイムシフト再生できる: http://localhost:5510/xcode.lua?0=video/foo.ts

-- コマンドはEDCBのToolsフォルダにあるものを優先する
ffmpeg=edcb.GetPrivateProfile('SET', 'ModulePath', '', 'Common.ini')..'\\Tools\\ffmpeg.exe'
if not edcb.FindFile(ffmpeg, 1) then ffmpeg='ffmpeg.exe' end
readex=edcb.GetPrivateProfile('SET', 'ModulePath', '', 'Common.ini')..'\\Tools\\readex.exe'
if not edcb.FindFile(readex, 1) then readex='readex.exe' end

-- トランスコードするかどうか(する場合はreadex.exeとffmpeg.exeを用意すること)
XCODE=false
-- 変換コマンド
-- libvpxの例:リアルタイム変換と画質が両立するようにビットレート-bと計算量-cpu-usedを調整する
XCMD='"'..ffmpeg..'" -i pipe:0 -vcodec libvpx -b:v 896k -quality realtime -cpu-used 1 -vf yadif=0:-1:1 -s 512x288 -r 30000/1001 -acodec libvorbis -ab 128k -f webm -'
--XCMD='"'..ffmpeg..'" -i pipe:0 -vcodec libx264 -profile:v main -level 31 -b:v 896k -maxrate 4M -bufsize 4M -preset veryfast -g 120 -vf yadif=0:-1:1 -s 512x288 -r 30000/1001 -acodec aac -ab 128k -f mp4 -movflags frag_keyframe+empty_moov -'
-- 変換後の拡張子
XEXT='.webm'
--XEXT='.mp4'
-- 転送開始前に変換しておく量(bytes)
XPREPARE=nil

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

fpath=nil
for i=0,99 do
  -- 変数名は転送開始位置(99分率)
  fpath=mg.get_var(mg.request_info.query_string, ''..i)
  if fpath then
    fpath=DocumentToNativePath(fpath)
    offset=i
    break
  end
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
      if offset~=0 and offset~=99 and fsec>0 and SeekSec(f,fsec*offset/99) then
        offset=f:seek('cur',0) or 0
      else
        offset=math.floor(fsize*offset/99/188)*188
      end
      if XCODE then
        f:close()
        -- 容量確保の可能性があるときは周期188+同期語0x47(188*256+0x47=48199)で対象ファイルを終端判定する
        sync=edcb.GetPrivateProfile('SET','KeepDisk',0,'EpgTimerSrv.ini')~='0'
        f=edcb.io.popen('""'..readex..'" '..offset..(sync and ' 4p48199' or ' 4')..' "'..fpath..'" | '..XCMD..'"', 'rb')
        fname='xcode'..XEXT
      else
        -- 容量確保には未対応
        f:seek('set', offset)
        XPREPARE=nil
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
    buf=f:read(XPREPARE or 48128)
    XPREPARE=nil
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
