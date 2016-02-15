-- ファイルをチャンク転送するスクリプト
-- このスクリプトと同じ場所のファイルをタイムシフト再生できる(容量確保録画には未対応): http://localhost:5510/video/chunk.lua?0=foo.ts
-- ディレクトリインデックス(http://localhost:5510/video/)に↓みたいなブックマークレットを適用すると便利
-- javascript:void((function(){var x=document.links;for(var i=0;i<x.length;i++){x[i].href=x[i].href.replace(/\/(?!chunk\.lua)([^\/]{4,})$/,'/chunk.lua?0=$1')}})())

-- トランスコードするかどうか(する場合はreadex.exeとffmpeg.exeをパスの通ったどこかに用意すること)
XCODE=false
-- 変換コマンド
XCMD='ffmpeg -i pipe:0 -vcodec vp8 -s 512x288 -r 15000/1001 -acodec libvorbis -f webm -'
-- 変換後のメディアタイプ
XMTYPE=mg.get_mime_type('a.webm')

fname=nil
for i=0,99 do
  -- 変数名は転送開始位置(99分率)
  fname=mg.get_var(mg.request_info.query_string, ''..i)
  if fname then
    -- トラバーサル対策
    fname=string.match(edcb.Convert('utf-8', 'cp932', edcb.Convert('cp932', 'utf-8', fname)), '^[^\\/:*?"<>|]+$')
    offset=i
    break
  end
end

f=nil
if fname then
  fpath=edcb.Convert('cp932', 'utf-8', mg.script_name:gsub('[0-9A-Za-z.]*$', '')..fname)
  f=io.open(fpath, 'rb')
  if f then
    offset=math.floor((f:seek('end', 0) or 0) * offset / 99 / 188) * 188
    if XCODE then
      f:close()
      f=io.popen('readex '..offset..' 4 "'..fpath..'" | '..XCMD, 'rb')
      mtype=XMTYPE
    else
      f:seek('set', offset)
      mtype=mg.get_mime_type(fname)
    end
  end
end

if not f then
  mg.write('HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n')
else
  mg.write('HTTP/1.1 200 OK\r\nContent-Type: '..mtype..'\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n')
  retry=0
  while true do
    buf=f:read(188 * 64)
    if buf and #buf ~= 0 then
      -- チャンク転送
      retry=0
      if not mg.write(string.format('%x\r\n', #buf), buf, '\r\n') then
        -- キャンセルされた
        mg.cry('canceled')
        break
      end
    else
      -- 終端に達した。4秒間この状態が続けば対象ファイルへの追記が終息したとみなす
      retry=retry+1
      if XCODE or retry > 20 then
        mg.cry('end')
        mg.write('0\r\n\r\n')
        break
      end
      edcb.Sleep(200)
    end
  end
  f:close()
end
