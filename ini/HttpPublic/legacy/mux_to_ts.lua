-- MP4ファイルをトランスポートストリームに変換して転送するスクリプト
-- Range対応なので再生ソフトによってはシークが可能

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

-- 安全な(POST的でない)処理を意識するのでCSRF対策トークンは求めない
fpath=mg.get_var(mg.request_info.query_string,'fname')
if fpath then
  fpath=DocumentToNativePath(fpath)
end

f=nil
-- 拡張子を限定
if fpath and fpath:find('%.[Mm][Pp]4$') then
  seek=nil
  range=nil
  for k,v in pairs(mg.request_info.http_headers) do
    if k:lower()=='range' then
      -- マルチパートには未対応
      seek,range=v:match('^bytes=([0-9]+)%-([0-9]+)$')
      if seek then
        seek=tonumber(seek)
        range=tonumber(range)
        if seek and range and 0<=seek and seek<0x1000000000 and seek<=range and range<0x1000000000 then
          range=range-seek+1
        else
          seek=nil
          range=nil
        end
      else
        seek=v:match('^bytes=([0-9]+)%-$') or v:match('^bytes=(%-[1-9][0-9]*)$')
        if seek then
          seek=tonumber(seek)
          if not seek or not (-0x1000000000<seek and seek<0x1000000000) then
            seek=nil
          elseif seek<0 then
            seek=seek-1
          end
        end
      end
      break
    end
  end
  psisimux=FindToolsCommand('psisimux')
  while true do
    cmd=psisimux..' -s '..(seek or 0)..' -r '..(range or -1)..' -p -8 -x .vtt -y .psc '..QuoteCommandArgForPath(fpath)..' -'
    f=edcb.io.popen(WIN32 and '"'..cmd..'"' or cmd,'r'..POPEN_BINARY)
    if not f then break end
    buf=f:read(188)
    if not buf or #buf~=188 then
      f:close()
      f=nil
      break
    end
    fsize=GetLeNumber(buf,9,8)
    fpos=GetLeNumber(buf,17,8)
    if not seek or seek>=0 or fpos~=fsize then break end
    -- 範囲を超えた末尾リクエストは全体を返す
    seek=0
    f:close()
  end
end

if not f then
  ct=CreateContentBuilder()
  ct:Append(DOCTYPE_HTML4_STRICT..'<title>mux_to_ts.lua</title><p><a href="index.html">メニュー</a></p>')
  ct:Finish()
  mg.write(ct:Pop(Response(404,'text/html','utf-8',ct.len)..'\r\n'))
else
  fname='mux_to_ts'..edcb.GetPrivateProfile('SET','TSExt','.ts','EpgTimerSrv.ini')
  if fpos>=fsize then
    -- シーク範囲外
    mg.write(Response(416,mg.get_mime_type(fname),nil,0,false,3600)
      ..'Content-Disposition: filename='..fname..'\r\nETag: "'..fsize..'"\r\nAccept-Ranges: bytes\r\n'
      ..'Content-Range: bytes */'..fsize..'\r\n\r\n')
  else
    cl=range and math.min(range,fsize-fpos) or fsize-fpos
    mg.write(Response(seek and 206 or 200,mg.get_mime_type(fname),nil,cl,false,3600)
      ..'Content-Disposition: filename='..fname..'\r\nETag: "'..fsize..'"\r\nAccept-Ranges: bytes\r\n'
      ..(seek and 'Content-Range: bytes '..fpos..'-'..(fpos+cl-1)..'/'..fsize..'\r\n' or '')..'\r\n')
    if mg.request_info.request_method~='HEAD' then
      while cl>0 do
        buf=f:read(math.min(cl,188*128))
        if not buf or #buf==0 or not mg.write(buf) then
          break
        end
        cl=cl-#buf
      end
    end
  end
  f:close()
end
