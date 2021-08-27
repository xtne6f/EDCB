-- 名前付きパイプ(SendTSTCPの送信先:0.0.0.1 ポート:0～65535)を転送するスクリプト

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

post=AssertPost()
if not post then
  -- POSTでなくてもよい
  post=mg.request_info.query_string
  AssertCsrf(post)
end

option=XCODE_OPTIONS[GetVarInt(post,'option',1,#XCODE_OPTIONS) or 1]
audio2=(GetVarInt(post,'audio2',0,1) or 0)+(option.audioStartAt or 0)
dual=GetVarInt(post,'dual',0,2)
dual=dual==1 and option.dualMain or dual==2 and option.dualSub or ''
filter=GetVarInt(post,'cinema')==1 and option.filterCinema or option.filter or ''
hls=GetVarInt(post,'hls',1)
caption=hls and GetVarInt(post,'caption')==1 and option.captionHls or option.captionNone or ''
output=hls and option.outputHls or option.output
n=GetVarInt(post,'n') or 0
onid,tsid,sid=(mg.get_var(post,'id') or ''):match('^(%d?%d?%d?%d?%d)%-(%d?%d?%d?%d?%d)%-(%d?%d?%d?%d?%d)$')
if hls and not (ALLOW_HLS and option.outputHls) then
  -- エラーを返す
  n=nil
  onid=nil
end

function OpenTranscoder(pipeName,nwtvclose)
  -- コマンドはEDCBのToolsフォルダにあるものを優先する
  local tools=edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')..'\\Tools'
  local asyncbuf=(edcb.FindFile(tools..'\\asyncbuf.exe',1) and tools..'\\' or '')..'asyncbuf.exe'
  local tsmemseg=(edcb.FindFile(tools..'\\tsmemseg.exe',1) and tools..'\\' or '')..'tsmemseg.exe'
  local xcoder=(edcb.FindFile(tools..'\\'..option.xcoder,1) and tools..'\\' or '')..option.xcoder

  local cmd='"'..xcoder..'" '..option.option
    :gsub('$SRC',pipeName)
    :gsub('$AUDIO',audio2)
    :gsub('$DUAL',(dual:gsub('%%','%%%%')))
    :gsub('$FILTER',(filter:gsub('%%','%%%%')))
    :gsub('$CAPTION',(caption:gsub('%%','%%%%')))
    :gsub('$OUTPUT',(output[2]:gsub('%%','%%%%')))
  if XCODE_LOG then
    local log=mg.script_name:gsub('[^\\/]*$','')..'log'
    if not edcb.FindFile(log,1) then
      edcb.os.execute('mkdir "'..log..'"')
    end
    -- 衝突しにくいログファイル名を作る
    log=log..'\\view-'..os.time()..'-'..mg.md5(cmd):sub(29)..'.txt'
    local f=edcb.io.open(log,'w')
    if f then
      f:write(cmd..'\n\n')
      f:close()
      cmd=cmd..' 2>>"'..log..'"'
    end
  end
  if hls then
    -- セグメント長は既定値(2秒)なので概ねキーフレーム(4～5秒)間隔
    -- プロセス終了時に対応するNetworkTVモードも終了させる
    cmd=cmd..' | "'..tsmemseg..'" -a 10 -m 8192 -d 3 '
      ..(nwtvclose and '-c "powershell -NoProfile -ExecutionPolicy RemoteSigned -File nwtvclose.ps1 '..nwtvclose..'" ' or '')..segmentKey..'_'
  elseif XCODE_BUF>0 then
    cmd=cmd..' | "'..asyncbuf..'" '..XCODE_BUF..' '..XCODE_PREPARE
  end
  -- プロセス検索用コメント
  cmd=cmd..' & rem view-lua'
  if hls then
    -- 極端に多く開けないようにする
    local indexCount=#(edcb.FindFile('\\\\.\\pipe\\tsmemseg_*_00',10) or {})
    if indexCount<10 and (not nwtvclose or edcb.FindFile(tools..'\\nwtvclose.ps1',1)) then
      edcb.os.execute('start "" /b cmd /s /c "'..(nwtvclose and 'cd /d "'..tools..'" && ' or '')..cmd..'"')
      for i=1,100 do
        local f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..segmentKey..'_00','rb')
        if f then
          return f
        end
        edcb.Sleep(100)
      end
      -- 失敗。プロセスが残っていたら終わらせる
      edcb.os.execute('wmic process where "name=\'tsmemseg.exe\' and commandline like \'% '..segmentKey..'[_]%\'" call terminate >nul')
    end
    return nil
  end
  return edcb.io.popen('"'..cmd..'"','rb')
end

f=nil
if onid then
  onid=tonumber(onid) or 0
  tsid=tonumber(tsid) or 0
  sid=tonumber(sid) or 0
  if sid==0 then
    -- NetworkTVモードを終了
    edcb.CloseNetworkTV(n)
  elseif 0<=n and n<100 then
    if hls then
      -- クエリのハッシュをキーとし、同一キーアクセスは出力中のインデックスファイルを返す
      segmentKey=mg.md5('view:'..hls..':nwtv'..n..':'..option.xcoder..':'..option.option..':'..audio2..':'..dual..':'..filter..':'..caption..':'..output[2])
      f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..segmentKey..'_00','rb')
    end
    if not f then
      openTime=os.time()
      edcb.WritePrivateProfile('NWTV','nwtv'..n..'open','@'..openTime,'Setting\\HttpPublic.ini')
      -- NetworkTVモードを開始
      ok,pid=edcb.OpenNetworkTV(2,onid,tsid,sid,n)
      if ok then
        -- 名前付きパイプができるまで待つ
        pipeName=nil
        for i=1,50 do
          ff=edcb.FindFile('\\\\.\\pipe\\SendTSTCP_*_'..pid, 1)
          if ff and ff[1].name:find('^[A-Za-z]+_%d+_%d+$') then
            pipeName='\\\\.\\pipe\\'..ff[1].name
            break
          elseif NWTV_FIND_BY_OPEN then
            -- ポートを予想して開いてみる
            for j=0,9 do
              ff=edcb.io.open('\\\\.\\pipe\\SendTSTCP_'..j..'_'..pid, 'rb')
              if ff then
                ff:close()
                -- 再び開けるようになるまで少しラグがある
                edcb.Sleep(2000)
                pipeName='\\\\.\\pipe\\SendTSTCP_'..j..'_'..pid
                break
              end
            end
            if ff then break end
          end
          edcb.Sleep(200)
        end
        if pipeName then
          f=OpenTranscoder(pipeName,n..' @'..openTime)
          fname='view.'..output[1]
        end
        if not f then
          edcb.CloseNetworkTV(n)
        end
      end
    end
  end
elseif n and n<0 then
  -- プロセスが残っていたらすべて終わらせる
  pf=edcb.io.popen('wmic process where "commandline like \'% [r]em view-lua%\'" get processid 2>nul | findstr /b [1-9]')
  if pf then
    for pid in (pf:read('*a') or ''):gmatch('[1-9][0-9]*') do
      edcb.os.execute('wmic process where "parentprocessid = '..pid..' and commandline like \'%SendTSTCP[_]%[_]%\'" call terminate >nul')
    end
    pf:close()
  end
elseif n and n<=65535 then
  if hls then
    -- クエリのハッシュをキーとし、同一キーアクセスは出力中のインデックスファイルを返す
    segmentKey=mg.md5('view:'..hls..':'..n..':'..option.xcoder..':'..option.option..':'..audio2..':'..dual..':'..filter..':'..caption..':'..output[2])
    f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..segmentKey..'_00','rb')
  end
  if not f then
    -- 前回のプロセスが残っていたら終わらせる
    pf=edcb.io.popen('wmic process where "commandline like \'% [r]em view-lua%\'" get processid 2>nul | findstr /b [1-9]')
    if pf then
      for pid in (pf:read('*a') or ''):gmatch('[1-9][0-9]*') do
        edcb.os.execute('wmic process where "parentprocessid = '..pid..' and commandline like \'%SendTSTCP[_]'..n..'[_]%\'" call terminate >nul')
      end
      pf:close()
    end
    -- 名前付きパイプがあれば開く
    ff=edcb.FindFile('\\\\.\\pipe\\SendTSTCP_'..n..'_*', 1)
    if ff and ff[1].name:find('^[A-Za-z]+_%d+_%d+$') then
      f=OpenTranscoder('\\\\.\\pipe\\'..ff[1].name)
      fname='view.'..output[1]
    end
  end
end

if not f then
  ct=CreateContentBuilder()
  ct:Append('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">\n'
    ..'<title>view.lua</title><p><a href="index.html">メニュー</a></p>')
  ct:Finish()
  mg.write(ct:Pop(Response(404,'text/html','utf-8',ct.len)..'\r\n'))
elseif hls then
  -- インデックスファイルを返す
  ct=CreateContentBuilder()
  ct:Append('#EXTM3U\n#EXT-X-VERSION:3\n')
  hasSeg=false
  buf=f:read(16)
  if buf and #buf==16 then
    segNum=buf:byte(1)
    endList=buf:byte(9)~=0
    for i=1,segNum do
      buf=f:read(16)
      if not buf or #buf~=16 then
        break
      end
      segAvailable=buf:byte(8)==0
      if segAvailable then
        segIndex=buf:byte(1)
        segCount=(buf:byte(7)*256+buf:byte(6))*256+buf:byte(5)
        segDuration=((buf:byte(11)*256+buf:byte(10))*256+buf:byte(9))/1000
        if not hasSeg then
          ct:Append('#EXT-X-TARGETDURATION:6\n#EXT-X-MEDIA-SEQUENCE:'..segCount..'\n'
            ..(endList and '#EXT-X-ENDLIST\n' or ''))
          hasSeg=true
        end
        ct:Append('#EXTINF:'..segDuration..',\nsegment.lua?c='..segmentKey..('_%02d_'):format(segIndex)..segCount..'\n')
      end
    end
  end
  f:close()
  if not hasSeg then
    ct:Append('#EXT-X-TARGETDURATION:8\n#EXT-X-MEDIA-SEQUENCE:1\n#EXTINF:8.008000,\nloading.m2t\n')
  end
  ct:Finish()
  mg.write(ct:Pop(Response(200,'application/vnd.apple.mpegurl','utf-8',ct.len)..'\r\n'))
else
  mg.write(Response(200,mg.get_mime_type(fname))..'Content-Disposition: filename='..fname..'\r\n\r\n')
  if mg.request_info.request_method~='HEAD' then
    while true do
      buf=f:read(48128)
      if buf and #buf~=0 then
        if not mg.write(buf) then
          -- キャンセルされた
          mg.cry('canceled')
          break
        end
      else
        -- 終端に達した
        mg.cry('end')
        break
      end
    end
  end
  f:close()
  if onid then
    -- リロード時などの終了を防ぐ。厳密にはロックなどが必要だが概ねうまくいけば良い
    if edcb.GetPrivateProfile('NWTV','nwtv'..n..'open','@'..openTime,'Setting\\HttpPublic.ini')=='@'..openTime then
      -- NetworkTVモードを終了
      edcb.CloseNetworkTV(n)
    end
  end
end
