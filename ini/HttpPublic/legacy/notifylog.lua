f=edcb.io.open(edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')..'\\EpgTimerSrvNotify.log','rb')
if not f then
  mg.write('HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nNot Found or Forbidden.\r\n')
else
  mg.write('HTTP/1.1 200 OK\r\nX-Content-Type-Options: nosniff\r\nContent-Type: text/plain; charset=utf-8\r\nConnection: close\r\n\r\n')
  c=tonumber(mg.get_var(mg.request_info.query_string,'c')) or math.huge
  fsize=f:seek('end')
  if fsize>=2 then
    ofs=math.floor(math.max(fsize/2-1-math.min(math.max(c,0),1e7),0))
    f:seek('set',2+ofs*2)
    a=f:read('*a')
    --utf-16le(without surrogates) to utf-8
    a=a:gsub('..',function (x)
      x=string.byte(x,1)+string.byte(x,2)*256
      return x<0x80 and string.char(x) or x<0x800 and string.char(0xc0+math.floor(x/64),0x80+x%64)
        or string.char(0xe0+math.floor(x/4096),0x80+math.floor(x/64)%64,0x80+x%64)
    end)
    if ofs~=0 then
      a=a:gsub('^[^\n]*\n','')
    end
    mg.write(a)
  end
  f:close()
end
