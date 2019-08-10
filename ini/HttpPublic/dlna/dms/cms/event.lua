if mg.request_info.request_method=='SUBSCRIBE' then
  --UUIDを作って投げるだけ
  package.path=mg.script_name..'/../../?.lua'
  mg.write('HTTP/1.1 200 OK\r\n',
    'SID: uuid:'..require('uuid4').getUUID():lower()..'\r\n',
    'Server: UnknownOS/1.0 UPnP/1.1 EpgTimerSrv/0.10\r\n',
    'Timeout: Second-1800\r\n',
    'Content-Length: 0\r\n',
    'Connection: close\r\n\r\n')
else
  mg.write('HTTP/1.1 200 OK\r\n',
    'Content-Length: 0\r\n',
    'Connection: close\r\n\r\n')
end
