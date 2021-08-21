dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

c=mg.get_var(mg.request_info.query_string,'c') or ''
key=c:match('^[0-9a-f]+_[0-9][0-9]')

buf=nil
if key and #key==35 and not key:sub(-2)~='00' then
  f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..key,'rb')
  if f then
    buf=f:read(188)
    if buf and #buf==188 and c==key..'_'..((buf:byte(7)*256+buf:byte(6))*256+buf:byte(5)) then
      if mg.request_info.request_method=='HEAD' then
        buf=''
      else
        segSize=((buf:byte(11)*256+buf:byte(10))*256+buf:byte(9))*188
        buf=f:read(segSize)
        if buf and #buf~=segSize then
          buf=nil
        end
      end
    else
      buf=nil
    end
    f:close()
  end
end
if buf then
  mg.write(Response(200,mg.get_mime_type('a.m2t'),nil,#buf)..'Content-Disposition: filename=segment.m2t\r\n\r\n',buf)
else
  mg.write(Response(404,nil,nil,0)..'\r\n')
end
