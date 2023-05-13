dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

c=mg.get_var(mg.request_info.query_string,'c') or ''
key=c:match('^[0-9a-f]+')

init=nil
if key and #key==32 then
  f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..key..'_00','rb')
  if f then
    buf=f:read(16)
    if buf and #buf==16 and buf:byte(11)~=0 then
      segNum=buf:byte(1)
      extraSize=segNum*16+GetLeNumber(buf,13,4)
      buf=f:read(extraSize)
      if buf and #buf==extraSize then
        for i=1,segNum do
          extraSize=extraSize-16-buf:byte((i-1)*16+3)*16
        end
        if extraSize>0 then
          if mg.request_info.request_method=='HEAD' then
            init=''
          else
            init=buf:sub(-extraSize)
          end
        end
      end
    end
    f:close()
  end
end
if init then
  mg.write(Response(200,mg.get_mime_type('a.mp4'),nil,#init)..'Content-Disposition: filename=mp4init.mp4\r\n\r\n',init)
else
  mg.write(Response(404,nil,nil,0)..'\r\n')
end
