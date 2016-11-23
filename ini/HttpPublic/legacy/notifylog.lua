f=io.open(edcb.Convert('cp932','utf-8',edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')..'\\EpgTimerSrvNotifyLog.txt'),'rb')
if not f then
  mg.write('HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nNot Found or Forbidden.\r\n')
else
  mg.write('HTTP/1.1 200 OK\r\nX-Content-Type-Options: nosniff\r\nContent-Type: text/plain; charset=utf-8\r\nConnection: close\r\n\r\n')
  c=tonumber(mg.get_var(mg.request_info.query_string,'c')) or -1
  fsize=f:seek('end')
  ofs=c<0 and 0 or math.max(fsize-c,0)
  f:seek('set',ofs)
  for a in f:lines('*L') do
    if ofs==0 then
      mg.write(edcb.Convert('utf-8','cp932',a))
    end
    ofs=0
  end
  f:close()
end
