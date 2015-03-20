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
    now=os.time()
    nowGMT=os.date('!*t', now)
    mg.write('HTTP/1.1 200 OK\r\n',
      'Content-Type: text/xml; charset="UTF-8"\r\n',
      'Date: '..({'Sun','Mon','Tue','Wed','Thu','Fri','Sat'})[nowGMT.wday]..os.date('!, %d ', now)
        ..({'Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'})[nowGMT.month]
        ..os.date('! %Y %H:%M:%S GMT\r\n', now),
      'Server: '..mg.system:gsub(' ','/')..' UPnP/1.1 EpgTimerSrv/0.10\r\n',
      'Content-Length: '..#res..'\r\n',
      'Connection: close\r\n\r\n', res)
    ok=true
  end
end
if not ok then
  mg.write('HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n')
end
