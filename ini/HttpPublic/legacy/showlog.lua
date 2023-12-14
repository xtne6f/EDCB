dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

if mg.get_var(mg.request_info.query_string,'t')=='d' then
  if SHOW_DEBUG_LOG then
    t='\\EpgTimerSrvDebugLog.txt'
  end
else
  if SHOW_NOTIFY_LOG then
    t='\\EpgTimerSrvNotify.log'
  end
end
if t then
  f=edcb.io.open(EdcbModulePath()..t,'rb')
end

if not f then
  mg.write(Response(404,nil,nil,0)..'\r\n')
else
  ct=CreateContentBuilder(GZIP_THRESHOLD_BYTE)
  c=GetVarInt(mg.request_info.query_string,'c',0,1e7) or 1e7
  fsize=f:seek('end')
  if fsize>=2 then
    ofs=math.floor(math.max(fsize/2-1-c,0))
    f:seek('set',2+ofs*2)
    if ofs~=0 then
      repeat
        buf=f:read(2)
      until not buf or #buf<2 or buf=='\n\0'
    end
    ct:Append(edcb.Convert('utf-8','utf-16le',f:read('*a') or '') or '')
  end
  f:close()
  ct:Finish()
  mg.write(ct:Pop(Response(200,'text/plain','utf-8',ct.len)..(ct.gzip and 'Content-Encoding: gzip\r\n' or '')..'\r\n'))
end
