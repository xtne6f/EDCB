-- ファイルを転送するスクリプト
-- ファイルをタイムシフト再生できる: http://localhost:5510/xcode.lua?fname=video/foo.ts

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

-- 安全な(POST的でない)処理を意識するのでCSRF対策トークンは求めない
query=mg.request_info.query_string
fpath=mg.get_var(query,'fname')
if fpath then
  fpath=DocumentToNativePath(fpath)
end

offset=GetVarInt(query,'offset',0,100) or 0
ofssec=GetVarInt(query,'ofssec',0,100000) or 0
option=XCODE_OPTIONS[GetVarInt(query,'option',1,#XCODE_OPTIONS) or 1]
audio2=(GetVarInt(query,'audio2',0,1) or 0)+(option.audioStartAt or 0)
filter=GetVarInt(query,'fast')==1 and (GetVarInt(query,'cinema')==1 and option.filterCinemaFast or option.filterFast)
filter=filter or (GetVarInt(query,'cinema')==1 and option.filterCinema or option.filter or '')
hls=GetVarInt(query,'hls',1)
caption=hls and GetVarInt(query,'caption')==1 and option.captionHls or option.captionNone or ''
output=hls and option.outputHls or option.output
if hls and not (ALLOW_HLS and option.outputHls) then
  -- エラーを返す
  fpath=nil
end
psidata=GetVarInt(query,'psidata')==1

function OpenTranscoder()
  if XCODE_SINGLE then
    -- トランスコーダーの親プロセスのリストを作る
    local pids=nil
    local pf=edcb.io.popen('wmic process where "name=\'tsreadex.exe\' and commandline like \'% -z edcb-legacy-%\'" get parentprocessid 2>nul | findstr /b [1-9]')
    if pf then
      for pid in (pf:read('*a') or ''):gmatch('[1-9][0-9]*') do
        pids=(pids and pids..' or ' or '')..'processid='..pid
      end
      pf:close()
    end
    -- パイプラインの上流を終わらせる
    edcb.os.execute('wmic process where "name=\'tsreadex.exe\' and commandline like \'% -z edcb-legacy-%\'" call terminate >nul')
    if pids then
      -- 親プロセスの終了を2秒だけ待つ。パイプラインの下流でストールしている可能性もあるので待ちすぎない
      for i=1,4 do
        edcb.Sleep(500)
        if i==4 or not edcb.os.execute('wmic process where "'..pids..'" get processid 2>nul | findstr /b [1-9] >nul') then
          break
        end
      end
    end
  end

  -- コマンドはEDCBのToolsフォルダにあるものを優先する
  local tools=edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')..'\\Tools'
  local tsreadex=(edcb.FindFile(tools..'\\tsreadex.exe',1) and tools..'\\' or '')..'tsreadex.exe'
  local asyncbuf=(edcb.FindFile(tools..'\\asyncbuf.exe',1) and tools..'\\' or '')..'asyncbuf.exe'
  local tsmemseg=(edcb.FindFile(tools..'\\tsmemseg.exe',1) and tools..'\\' or '')..'tsmemseg.exe'
  local xcoder=''
  for s in option.xcoder:gmatch('[^|]+') do
    xcoder=tools..'\\'..s
    if edcb.FindFile(xcoder,1) then break end
    xcoder=s
  end

  local cmd='"'..xcoder..'" '..option.option
    :gsub('$SRC','-')
    :gsub('$AUDIO',audio2)
    :gsub('$DUAL','')
    :gsub('$FILTER',(filter:gsub('%%','%%%%')))
    :gsub('$CAPTION',(caption:gsub('%%','%%%%')))
    :gsub('$OUTPUT',(output[2]:gsub('%%','%%%%')))
  if XCODE_LOG then
    local log=mg.script_name:gsub('[^\\/]*$','')..'log'
    if not edcb.FindFile(log,1) then
      edcb.os.execute('mkdir "'..log..'"')
    end
    -- 衝突しにくいログファイル名を作る
    log=log..'\\xcode-'..os.time()..'-'..mg.md5(cmd):sub(29)..'.txt'
    local f=edcb.io.open(log,'w')
    if f then
      f:write(cmd..'\n\n')
      f:close()
      cmd=cmd..' 2>>"'..log..'"'
    end
  end
  if hls then
    -- セグメント長は既定値(2秒)なので概ねキーフレーム(4～5秒)間隔
    cmd=cmd..' | "'..tsmemseg..'" -a 10 -r 100 -m 8192 -d 3 '..segmentKey..'_'
  elseif XCODE_BUF>0 then
    cmd=cmd..' | "'..asyncbuf..'" '..XCODE_BUF..' '..XCODE_PREPARE
  end
  local sync=edcb.GetPrivateProfile('SET','KeepDisk',0,'EpgTimerSrv.ini')~='0'
  -- "-z"はプロセス検索用
  cmd='"'..tsreadex..'" -z edcb-legacy-xcode -s '..offset..' -l 16384 -t 6'..(sync and ' -m 1' or '')..' -x 18/38/39 -n -1 -a 9 -b 1 -c 1 -u 2 "'..fpath:gsub('[&%^]','^%0')..'" | '..cmd
  if hls then
    -- 極端に多く開けないようにする
    local indexCount=#(edcb.FindFile('\\\\.\\pipe\\tsmemseg_*_00',10) or {})
    if indexCount<10 then
      edcb.os.execute('start "" /b cmd /c "'..cmd..'"')
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

function OpenPsiDataArchiver()
  -- コマンドはEDCBのToolsフォルダにあるものを優先する
  local tools=edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')..'\\Tools'
  local tsreadex=(edcb.FindFile(tools..'\\tsreadex.exe',1) and tools..'\\' or '')..'tsreadex.exe'
  local psisiarc=(edcb.FindFile(tools..'\\psisiarc.exe',1) and tools..'\\' or '')..'psisiarc.exe'
  local sync=edcb.GetPrivateProfile('SET','KeepDisk',0,'EpgTimerSrv.ini')~='0'
  -- 3秒間隔で出力
  local cmd='"'..psisiarc..'" -r arib-data -i 3 - -'
  cmd='"'..tsreadex..'" -s '..offset..' -l 16384 -t 6'..(sync and ' -m 1' or '')..' "'..fpath:gsub('[&%^]','^%0')..'" | '..cmd
  return edcb.io.popen('"'..cmd..'"','rb')
end

function ReadPsiDataChunk(f,trailerSize,trailerRemainSize)
  if trailerSize>0 then
    local buf=f:read(trailerSize)
    if not buf or #buf~=trailerSize then return nil end
  end
  local buf=f:read(32)
  if not buf or #buf~=32 then return nil end
  local timeListLen=GetLeNumber(buf,11,2)
  local dictionaryLen=GetLeNumber(buf,13,2)
  local dictionaryDataSize=GetLeNumber(buf,17,4)
  local codeListLen=GetLeNumber(buf,25,4)
  local payload=''
  local payloadSize=timeListLen*4+dictionaryLen*2+math.ceil(dictionaryDataSize/2)*2+codeListLen*2
  if payloadSize>0 then
    payload=f:read(payloadSize)
    if not payload or #payload~=payloadSize then return nil end
  end
  -- Base64のパディングを避けるため、トレーラを利用してbufのサイズを3の倍数にする
  local trailerConsumeSize=2-(trailerRemainSize+#buf+#payload+2)%3
  buf=('='):rep(trailerRemainSize)..buf..payload..('='):rep(trailerConsumeSize)
  return buf,2+(2+#payload)%4,2+(2+#payload)%4-trailerConsumeSize
end

f=nil
if fpath then
  if hls and not psidata then
    -- クエリのハッシュをキーとし、同一キーアクセスは出力中のインデックスファイルを返す
    segmentKey=mg.md5('xcode:'..hls..':'..fpath..':'..option.xcoder..':'..option.option..':'..offset..':'..audio2..':'..filter..':'..caption..':'..output[2])
    f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..segmentKey..'_00','rb')
  end
  if not f then
    fname='xcode'..(fpath:match('%.[0-9A-Za-z]+$') or '')
    fnamets='xcode'..edcb.GetPrivateProfile('SET','TSExt','.ts','EpgTimerSrv.ini'):lower()
    -- 拡張子を限定
    if fname:lower()==fnamets then
      f=edcb.io.open(fpath,'rb')
      if f then
        if offset~=0 or ofssec~=0 then
          fsec,fsize=GetDurationSec(f)
          if offset~=100 and SeekSec(f,fsec*offset/100+ofssec,fsec,fsize) then
            offset=f:seek('cur',0) or 0
          else
            offset=math.floor(fsize*offset/100/188)*188
          end
        end
        if psidata then
          f:close()
          f=OpenPsiDataArchiver()
          fname='xcode.psc.txt'
        elseif XCODE then
          f:close()
          f=OpenTranscoder()
          fname='xcode.'..output[1]
        elseif hls then
          -- トランスコードなしのライブストリーミングには未対応
          f:close()
          f=nil
        else
          -- 容量確保には未対応
          f:seek('set',offset)
        end
      end
    end
  end
end

if not f then
  ct=CreateContentBuilder()
  ct:Append('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">\n'
    ..'<title>xcode.lua</title><p><a href="index.html">メニュー</a></p>')
  ct:Finish()
  mg.write(ct:Pop(Response(404,'text/html','utf-8',ct.len)..'\r\n'))
elseif psidata then
  mg.write(Response(200,mg.get_mime_type(fname),'utf-8')..'Content-Disposition: filename='..fname..'\r\n\r\n')
  if mg.request_info.request_method~='HEAD' then
    trailerSize=0
    trailerRemainSize=0
    baseTime=0
    while true do
      -- 3秒間隔でチャンクを読めば等速になる
      buf,trailerSize,trailerRemainSize=ReadPsiDataChunk(f,trailerSize,trailerRemainSize)
      if not buf or not mg.write(mg.base64_encode(buf)) then break end
      now=os.time()
      if math.abs(baseTime-now)>10 then baseTime=now end
      edcb.Sleep(math.max(baseTime+3-now,0)*1000)
      baseTime=baseTime+3
    end
  end
  f:close()
elseif hls then
  -- インデックスファイルを返す
  ct=CreateContentBuilder()
  ct:Append('#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:6\n#EXT-X-MEDIA-SEQUENCE:')
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
        segCount=GetLeNumber(buf,5,3)
        segDuration=GetLeNumber(buf,9,3)/1000
        if not hasSeg then
          ct:Append((segCount==1 and '1\n#EXTINF:4.004,\nloading.m2t\n#EXT-X-DISCONTINUITY' or segCount+1)
            ..'\n'..(endList and '#EXT-X-ENDLIST\n' or ''))
          hasSeg=true
        end
        ct:Append('#EXTINF:'..segDuration..',\nsegment.lua?c='..segmentKey..('_%02d_'):format(segIndex)..segCount..'\n')
      end
    end
  end
  f:close()
  if not hasSeg then
    ct:Append('1\n#EXTINF:4.004,\nloading.m2t\n')
  end
  ct:Finish()
  mg.write(ct:Pop(Response(200,'application/vnd.apple.mpegurl','utf-8',ct.len)..'\r\n'))
else
  mg.write(Response(200,mg.get_mime_type(fname))..'Content-Disposition: filename='..fname..'\r\n\r\n')
  if mg.request_info.request_method~='HEAD' then
    retry=0
    while true do
      buf=f:read(48128)
      if buf and #buf~=0 then
        retry=0
        if not mg.write(buf) then
          -- キャンセルされた
          mg.cry('canceled')
          break
        end
      else
        -- 終端に達した。4秒間この状態が続けば対象ファイルへの追記が終息したとみなす
        retry=retry+1
        if XCODE or retry > 20 then
          mg.cry('end')
          break
        end
        edcb.Sleep(200)
      end
    end
  end
  f:close()
end
