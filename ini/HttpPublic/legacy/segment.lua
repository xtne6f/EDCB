dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

c=mg.get_var(mg.request_info.query_string,'c') or ''
key=c:match('^[0-9a-f]+_[0-9][0-9]')

buf=nil
if key and #key==35 and not key:sub(-2)~='00' then
  f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..key,'rb')
  if f then
    buf=f:read(188)
    if buf and #buf==188 then
      segKey=key..'_'..GetLeNumber(buf,5,3)
      isMp4=buf:byte(13)~=0
      if c==segKey then
        if mg.request_info.request_method=='HEAD' then
          buf=''
        else
          segSize=GetLeNumber(buf,9,4)*(isMp4 and 1 or 188)
          buf=f:read(segSize)
          if buf and #buf~=segSize then
            buf=nil
          end
        end
      else
        fragPos=0
        for i=1,20 do
          fragSize=GetLeNumber(buf,33+(i-1)*4,4)
          if fragSize==0 or c==segKey..'_'..i then
            break
          end
          fragPos=fragPos+fragSize
          fragSize=0
        end
        buf=nil
        if fragSize>0 then
          if mg.request_info.request_method=='HEAD' then
            buf=''
          else
            buf=f:read(fragPos+fragSize)
            if buf and #buf==fragPos+fragSize then
              buf=buf:sub(fragPos+1)
            else
              buf=nil
            end
          end
        end
      end
    else
      buf=nil
    end
    f:close()
  end
end
if buf then
  mg.write(Response(200,mg.get_mime_type(isMp4 and 'a.mp4' or 'a.m2t'),nil,#buf)
             ..'Content-Disposition: filename=segment'..(isMp4 and '.mp4' or '.m2t')..'\r\n\r\n',buf)
else
  mg.write(Response(404,nil,nil,0)..'\r\n')
end
