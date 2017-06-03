--Browseのメタ情報。旧protocolInfo.txtと同等 (拡張子, <upnp:class>の値, MimeType, DLNA.ORG_PN)
PROTOCOL_INFO={
  {'.mp4',  'object.item.videoItem', 'video/mp4',  nil},
  {'.ts',   'object.item.videoItem', 'video/mpeg', nil},
  {'.mp3',  'object.item.audioItem', 'audio/mpeg', 'MP3'},
  {'.m4a',  'object.item.audioItem', 'audio/mp4',  'AAC_ISO'},
  {'.jpg',  'object.item.imageItem', 'image/jpeg', 'JPEG_LRG'},
  {'.jpeg', 'object.item.imageItem', 'image/jpeg', 'JPEG_LRG'}
}
--このPCのホスト名。必要に応じてLAN内のIPアドレスやドメイン名を指定する
HOSTNAME='127.0.0.1'

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
  xmlnsU=' xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1"'
  res=nil
  if post:find('[<:]GetSearchCapabilities[ >]') then
    res=soapStart..'<u:GetSearchCapabilitiesResponse'..xmlnsU..'><SearchCaps></SearchCaps></u:GetSearchCapabilitiesResponse>'..soapEnd
  elseif post:find('[<:]GetSortCapabilities[ >]') then
    res=soapStart..'<u:GetSortCapabilitiesResponse'..xmlnsU..'><SortCaps>dc:title</SortCaps></u:GetSortCapabilitiesResponse>'..soapEnd
  elseif post:find('[<:]GetSystemUpdateID[ >]') then
    res=soapStart..'<u:GetSystemUpdateIDResponse'..xmlnsU..'><Id>1</Id></u:GetSystemUpdateIDResponse>'..soapEnd
  elseif post:find('[<:]Browse[ >]') then
    objectID=post:match('[<:]ObjectID[^>]->(%x+)</')
    startIndex=post:match('[<:]StartingIndex[^>]->(%d+)</')
    requestedCount=post:match('[<:]RequestedCount[^>]->(%d+)</')
    if objectID and startIndex and requestedCount then
      startIndex=0+startIndex
      requestedCount=0+requestedCount
      if post:find('[<:]BrowseFlag[^>]->BrowseMetadata</') then
        if objectID=='0' then
          res='<container id="0" restricted="1" parentID=""><dc:title></dc:title><upnp:class>object.container</upnp:class></container>'
          num={1,1}
        elseif objectID=='1' then
          res='<container id="1" restricted="1" parentID="0"><dc:title>PublicFile</dc:title><upnp:class>object.container</upnp:class></container>'
          num={1,1}
        else
          edcb.htmlEscape=15
          for i,v in ipairs(edcb.FindFile(mg.document_root..'\\dlna\\dms\\PublicFile\\*.*', 1000) or {}) do
            if not v.isdir and mg.md5(v.name)==objectID then
              for j,w in ipairs(PROTOCOL_INFO) do
                if v.name:sub(-#w[1]):lower()==w[1] then
                  --TODO: 長さを調べて原作のようにdurationプロパティを入れるとなお良いかも
                  --TODO: 公開フォルダはとりあえず/dlna/dms/PublicFile/に固定
                  res='<item id="'..objectID..'" restricted="1" parentID="1"><dc:title>'..v.name..'</dc:title><upnp:class>'..w[2]..'</upnp:class>'
                    ..'<res size="'..v.size..'" protocolInfo="http-get:*:'..w[3]..':DLNA.ORG_OP=01;DLNA.ORG_FLAGS=01500000000000000000000000000000'..(w[4] and ';DLNA.ORG_PN='..w[4] or '')..'">'
                    ..'http://'..HOSTNAME..':'..mg.request_info.server_port..'/dlna/dms/PublicFile/'..v.name..'</res></item>'
                  num={1,1}
                  break
                end
              end
              break
            end
          end
        end
      elseif post:find('[<:]BrowseFlag[^>]->BrowseDirectChildren</') then
        if objectID=='0' then
          res='<container id="1" restricted="1" parentID="0"><dc:title>PublicFile</dc:title><upnp:class>object.container</upnp:class></container>'
          num={1,1}
        elseif objectID=='1' then
          res=''
          num={0,0}
          edcb.htmlEscape=15
          a=edcb.FindFile(mg.document_root..'\\dlna\\dms\\PublicFile\\*.*', 1000) or {}
          table.sort(a, function(a,b) return a.name < b.name end)
          for i,v in ipairs(a) do
            for j,w in ipairs(PROTOCOL_INFO) do
              if not v.isdir and v.name:sub(-#w[1]):lower()==w[1] then
                if startIndex<=num[2] and num[1]<requestedCount then
                  res=res..'<item id="'..mg.md5(v.name)..'" restricted="1" parentID="1"><dc:title>'..v.name..'</dc:title><upnp:class>'..w[2]..'</upnp:class>'
                    ..'<res size="'..v.size..'" protocolInfo="http-get:*:'..w[3]..':DLNA.ORG_OP=01;DLNA.ORG_FLAGS=01500000000000000000000000000000'..(w[4] and ';DLNA.ORG_PN='..w[4] or '')..'">'
                    ..'http://'..HOSTNAME..':'..mg.request_info.server_port..'/dlna/dms/PublicFile/'..v.name..'</res></item>'
                  num[1]=num[1]+1
                end
                num[2]=num[2]+1
                break
              end
            end
          end
        end
      end
      if res then
        res='<DIDL-Lite xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">'..res..'</DIDL-Lite>'
        res=soapStart..'<u:BrowseResponse'..xmlnsU..'>'
          ..'<Result>'..res:gsub('&','&amp;'):gsub('<','&lt;'):gsub('>','&gt;'):gsub('"','&quot;')..'</Result>'
          ..'<NumberReturned>'..num[1]..'</NumberReturned>'
          ..'<TotalMatches>'..num[2]..'</TotalMatches>'
          ..'<UpdateID>1</UpdateID>'
          ..'</u:BrowseResponse>'..soapEnd
      end
    end
  elseif post:find('[<:]CreateObject[ >]') then
    res=''
  elseif post:find('[<:]DestroyObject[ >]') then
    res=''
  end
  if res then
    mg.write('HTTP/1.1 200 OK\r\n',
      'Content-Type: text/xml; charset="UTF-8"\r\n',
      'Server: '..mg.system:gsub(' ','/')..' UPnP/1.1 EpgTimerSrv/0.10\r\n',
      'EXT: \r\n',
      'Content-Length: '..#res..'\r\n',
      'Connection: close\r\n\r\n', res)
    ok=true
  end
end
if not ok then
  mg.write('HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n')
end
