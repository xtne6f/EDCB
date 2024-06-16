--Browse対象の拡張子とメタ情報。旧protocolInfo.txtと同等 ([拡張子] = {<upnp:class>の値, DLNA.ORG_PNの値})
PROTOCOL_INFO={
  ['.mp4']  = {'object.item.videoItem', nil},
  ['.ts']   = {'object.item.videoItem', nil},
  ['.m2t']  = {'object.item.videoItem', nil},
  ['.m2ts'] = {'object.item.videoItem', nil},
  ['.mts']  = {'object.item.videoItem', nil},
  ['.mp3']  = {'object.item.audioItem', 'MP3'},
  ['.m4a']  = {'object.item.audioItem', 'AAC_ISO'},
  ['.jpg']  = {'object.item.imageItem', 'JPEG_LRG'},
  ['.jpeg'] = {'object.item.imageItem', 'JPEG_LRG'},
}
--Browse対象のディレクトリ(ドキュメントルート基準。複数可。sub=サブディレクトリも見るか)
BROWSE_DIR={
  {dir='/video', sub=false},
}
--Hostヘッダがないときに使うこのPCのホスト名。LAN内のIPアドレスやドメイン名を指定する
HOSTNAME='127.0.0.1'

dirSep=package.config:match('^/') or '\\'

function GetUpdateID(d)
  --自分とサブディレクトリの中で最新の更新日時を調べる
  local updateID=1
  for i,v in ipairs(edcb.FindFile(mg.document_root:gsub('/+$','')..(d.dir..(d.sub and '/*' or '')):gsub('/',dirSep),10000) or {}) do
    if v.isdir and v.name~='..' then
      updateID=math.max(updateID,os.time(v.mtime) or 1)
    end
  end
  return updateID
end

function EdcbHtmlEscape(s)
  return edcb.Convert('utf-8','utf-8',s)
end

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
  for k,v in pairs(mg.request_info.http_headers) do
    if k:lower()=='host' then
      HOSTNAME=v:match('^[0-9A-Za-z%-%.]+') or HOSTNAME
      break
    end
  end
  soapStart='<?xml version="1.0" encoding="UTF-8"?><s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body>'
  soapEnd='</s:Body></s:Envelope>'
  xmlnsU=' xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1"'
  res=nil
  if post:find('[<:]GetSearchCapabilities[ >]') then
    res=soapStart..'<u:GetSearchCapabilitiesResponse'..xmlnsU..'><SearchCaps></SearchCaps></u:GetSearchCapabilitiesResponse>'..soapEnd
  elseif post:find('[<:]GetSortCapabilities[ >]') then
    res=soapStart..'<u:GetSortCapabilitiesResponse'..xmlnsU..'><SortCaps>dc:title</SortCaps></u:GetSortCapabilitiesResponse>'..soapEnd
  elseif post:find('[<:]GetSystemUpdateID[ >]') then
    updateID=1
    for i,v in ipairs(BROWSE_DIR) do
      updateID=math.max(updateID,GetUpdateID(v))
    end
    res=soapStart..'<u:GetSystemUpdateIDResponse'..xmlnsU..'><Id>'..updateID..'</Id></u:GetSystemUpdateIDResponse>'..soapEnd
  elseif post:find('[<:]Browse[ >]') then
    objectID=post:match('[<:]ObjectID[^>]->(%x+)</')
    startIndex=tonumber(post:match('[<:]StartingIndex[^>]->(%d+)</'))
    requestedCount=tonumber(post:match('[<:]RequestedCount[^>]->(%d+)</'))
    if objectID and startIndex and requestedCount then
      meta=post:find('[<:]BrowseFlag[^>]->BrowseMetadata</')
      if meta or post:find('[<:]BrowseFlag[^>]->BrowseDirectChildren</') then
        if objectID=='0' then
          if meta then
            --ルート情報
            res='<container id="0" restricted="1" parentID=""><dc:title></dc:title><upnp:class>object.container</upnp:class></container>'
            num={1,1}
          else
            --ルート下の列挙
            res=''
            num={0,0}
            for i,v in ipairs(BROWSE_DIR) do
              if startIndex<=num[2] and num[1]<requestedCount then
                edcb.htmlEscape=15
                res=res..'<container id="'..i..'" restricted="1" parentID="0"><dc:title>'..EdcbHtmlEscape(v.dir)..'</dc:title><upnp:class>object.container</upnp:class></container>'
                num[1]=num[1]+1
              end
              num[2]=num[2]+1
            end
          end
          updateID=1
          for i,v in ipairs(BROWSE_DIR) do
            updateID=math.max(updateID,GetUpdateID(v))
          end
        elseif meta and objectID:find('^%d+$') and BROWSE_DIR[tonumber(objectID)] then
          --各々ディレクトリ情報
          dirID=tonumber(objectID)
          edcb.htmlEscape=15
          res='<container id="1" restricted="1" parentID="0"><dc:title>'..EdcbHtmlEscape(BROWSE_DIR[dirID].dir)..'</dc:title><upnp:class>object.container</upnp:class></container>'
          num={1,1}
          updateID=GetUpdateID(BROWSE_DIR[dirID])
        elseif ((meta and objectID:find('^%d+f')) or (not meta and objectID:find('^%d+$'))) and BROWSE_DIR[tonumber(objectID:match('^%d+'))] then
          --ファイル情報または各々ディレクトリ下の列挙
          dirID=tonumber(objectID:match('^%d+'))
          res={}
          num={0,0}
          edcb.htmlEscape=0
          ff=edcb.FindFile(mg.document_root:gsub('/+$','')..(BROWSE_DIR[dirID].dir..'/*'):gsub('/',dirSep),10000) or {}
          table.sort(ff,function(a,b) return a.name<b.name end)
          for i,v in ipairs(ff) do
            ff={v}
            if BROWSE_DIR[dirID].sub and v.isdir and v.name~='.' and v.name~='..' then
              --1階層だけサブディレクトリも見る
              edcb.htmlEscape=0
              ff=edcb.FindFile(mg.document_root:gsub('/+$','')..(BROWSE_DIR[dirID].dir..'/'..v.name..'/*'):gsub('/',dirSep),10000) or {}
              table.sort(ff,function(a,b) return a.name<b.name end)
            end
            for j,w in ipairs(ff) do
              if not w.isdir then
                fileName=(v==w and '' or v.name..'/')..w.name
                ext=(w.name:match('%.[0-9A-Za-z]+$') or ''):lower()
                if PROTOCOL_INFO[ext] and (not meta or dirID..'f'..mg.md5(fileName)==objectID) then
                  if startIndex<=num[2] and num[1]<requestedCount then
                    --TODO: 長さを調べて原作のようにdurationプロパティを入れるとなお良いかも
                    edcb.htmlEscape=15
                    res[#res+1]='<item id="'..dirID..'f'..mg.md5(fileName)..'" restricted="1" parentID="'..dirID..'"><dc:title>'
                      ..EdcbHtmlEscape(fileName)..'</dc:title><upnp:class>'..PROTOCOL_INFO[ext][1]..'</upnp:class><res size="'
                      ..w.size..'" protocolInfo="http-get:*:'..mg.get_mime_type('a'..ext)..':DLNA.ORG_OP=01;DLNA.ORG_FLAGS=01500000000000000000000000000000'
                      ..(PROTOCOL_INFO[ext][2] and ';DLNA.ORG_PN='..PROTOCOL_INFO[ext][2] or '')..'">'
                      ..'http://'..HOSTNAME..':'..mg.request_info.server_port..mg.url_encode(BROWSE_DIR[dirID].dir..'/'..fileName):gsub('%%2f','/')..'</res></item>'
                    num[1]=num[1]+1
                  end
                  num[2]=num[2]+1
                  if meta then break end
                end
              end
              if meta and num[2]>0 then break end
            end
          end
          res=table.concat(res)
          updateID=meta and 1 or GetUpdateID(BROWSE_DIR[dirID])
        end
      end
      if res then
        res='<DIDL-Lite xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">'..res..'</DIDL-Lite>'
        --WMP12では要素の順序を変えると動作しないので注意
        edcb.htmlEscape=15
        res=soapStart..'<u:BrowseResponse'..xmlnsU..'>'
          ..'<Result>'..EdcbHtmlEscape(res)..'</Result>'
          ..'<NumberReturned>'..num[1]..'</NumberReturned>'
          ..'<TotalMatches>'..num[2]..'</TotalMatches>'
          ..'<UpdateID>'..updateID..'</UpdateID>'
          ..'</u:BrowseResponse>'..soapEnd
      end
    end
  elseif post:find('[<:]CreateObject[ >]') then
    res=''
  elseif post:find('[<:]DestroyObject[ >]') then
    res=''
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
