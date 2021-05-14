dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

f=edcb.io.open(edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')..'\\EpgTimerSrvNotify.log','rb')
if not f then
  mg.write(Response(404,nil,nil,0)..'\r\n')
else
  ct=CreateContentBuilder(GZIP_THRESHOLD_BYTE)
  c=tonumber(mg.get_var(mg.request_info.query_string,'c')) or math.huge
  fsize=f:seek('end')
  if fsize>=2 then
    ofs=math.floor(math.max(fsize/2-1-math.min(math.max(c,0),1e7),0))
    f:seek('set',2+ofs*2)
    if ofs~=0 then
      repeat
        buf=f:read(2)
      until not buf or #buf<2 or buf=='\n\0'
    end
    --utf-16le to utf-8
    ct:Append((f:read('*a'):gsub('..',function (x)
      x=x:byte(1)+x:byte(2)*256
      if 0xd800<=x and x<=0xdbff then
        hi=x
        return ''
      end
      if 0xdc00<=x and x<=0xdfff and hi then
        x=0x10000+(hi-0xd800)*0x400+x-0xdc00
      end
      hi=nil
      return x<0x80 and string.char(x) or x<0x800 and string.char(0xc0+math.floor(x/64),0x80+x%64)
        or x<0x10000 and string.char(0xe0+math.floor(x/4096),0x80+math.floor(x/64)%64,0x80+x%64)
        or string.char(0xf0+math.floor(x/0x40000),0x80+math.floor(x/4096)%64,0x80+math.floor(x/64)%64,0x80+x%64)
    end)))
  end
  f:close()
  ct:Finish()
  mg.write(ct:Pop(Response(200,'text/plain','utf-8',ct.len)..(ct.gzip and 'Content-Encoding: gzip\r\n' or '')..'\r\n'))
end
