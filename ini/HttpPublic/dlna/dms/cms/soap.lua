post=nil
if mg.request_info.request_method=='POST' then
  post=''
  repeat
    s=mg.read()
    post=post..(s or '')
  until not s
  if #post~=mg.request_info.content_length then
    post=nil
  end
end
ok=false
if post then
  soapStart='<?xml version="1.0" encoding="UTF-8"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body>'
  soapEnd='</s:Body></s:Envelope>'
  xmlnsU=' xmlns:u="urn:schemas-upnp-org:service:ConnectionManager:1"'
  res=nil
  if post:find('[<:]GetProtocolInfo[ >]') then
    --ミスだと思うので直したが、原作ではGetProtocolInfo(Responseなし)となっている
    res=soapStart..'<u:GetProtocolInfoResponse'..xmlnsU..'><Source>AAC_ISO,JPEG_LRG,MP3</Source><Sink></Sink></u:GetProtocolInfoResponse>'..soapEnd
  end
  if res then
    t=os.date('!*t')
    mg.write('HTTP/1.1 200 OK\r\n',
      'Content-Type: text/xml; charset="UTF-8"\r\n',
      ('Date: %s, %02d %s %d %02d:%02d:%02d GMT\r\n'):format(({'Sun','Mon','Tue','Wed','Thu','Fri','Sat'})[t.wday],t.day,
        ({'Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'})[t.month],t.year,t.hour,t.min,t.sec),
      'Server: UnknownOS/1.0 UPnP/1.1 EpgTimerSrv/0.10\r\n',
      'Content-Length: '..#res..'\r\n',
      'Connection: close\r\n\r\n', res)
    ok=true
  end
end
if not ok then
  mg.write('HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n')
end
