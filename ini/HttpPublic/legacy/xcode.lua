-- ファイルを転送するスクリプト
-- ファイルをタイムシフト再生できる(容量確保録画には未対応): http://localhost:5510/xcode.lua?0=video/foo.ts

-- トランスコードするかどうか(する場合はreadex.exeとffmpeg.exeをパスの通ったどこかに用意すること)
XCODE=false
-- 変換コマンド
-- libvpxの例:リアルタイム変換と画質が両立するようにビットレート-bと計算量-cpu-usedを調整する
XCMD='ffmpeg -i pipe:0 -vcodec libvpx -b 896k -quality realtime -cpu-used 1 -vf yadif=0:-1:1 -s 512x288 -r 30000/1001 -acodec libvorbis -ab 128k -f webm -'
-- 変換後の拡張子
XEXT='.webm'
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
  fpath=edcb.Convert('cp932', 'utf-8', fpath)
  f=io.open(fpath, 'rb')
  if f then
    offset=math.floor((f:seek('end', 0) or 0) * offset / 99 / 188) * 188
    if XCODE then
      f:close()
      f=io.popen('readex '..offset..' 4 "'..fpath..'" | '..XCMD, 'rb')
      fname='xcode'..XEXT
    else
      f:seek('set', offset)
      XPREPARE=nil
    end
  end
end

if not f then
  mg.write('HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n')
else
  mg.write('HTTP/1.1 200 OK\r\nContent-Type: '..mg.get_mime_type(fname)..'\r\nContent-Disposition: filename='..fname..'\r\nConnection: close\r\n\r\n')
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
