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
ofssec=GetVarInt(query,'ofssec',0,100000)
option=XCODE_OPTIONS[GetVarInt(query,'option',1,#XCODE_OPTIONS) or 1]
audio2=(GetVarInt(query,'audio2',0,1) or 0)+(option.audioStartAt or 0)
filter=GetVarInt(query,'fast')==1 and (GetVarInt(query,'cinema')==1 and option.filterCinemaFast or option.filterFast)
fastRate=filter and XCODE_FAST or 1
filter=filter or (GetVarInt(query,'cinema')==1 and option.filterCinema or option.filter or '')
hls=GetVarInt(query,'hls',1)
hls4=GetVarInt(query,'hls4',0) or 0
caption=hls and option.captionHls or option.captionNone or ''
output=hls and option.outputHls or option.output
if hls and not (ALLOW_HLS and option.outputHls) then
  -- エラーを返す
  fpath=nil
end
psidata=GetVarInt(query,'psidata')==1
jikkyo=GetVarInt(query,'jikkyo')==1
reload=GetVarInt(query,'reload',0)
loadtime=reload or GetVarInt(query,'load',0) or 0

function OpenTranscoder()
  local searchName='xcode-'..mg.md5(fpath..':'..loadtime):sub(25)
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
  elseif reload then
    -- リロード時は前回のプロセスを速やかに終わらせる
    edcb.os.execute('wmic process where "name=\'tsreadex.exe\' and commandline like \'% -z edcb-legacy-'..searchName..' %\'" call terminate >nul')
  end

  -- コマンドはEDCBのToolsフォルダにあるものを優先する
  local tools=EdcbModulePath()..'\\Tools'
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
  if fastRate~=1 and option.editorFast then
    local editor=''
    for s in option.editorFast:gmatch('[^|]+') do
      editor=tools..'\\'..s
      if edcb.FindFile(editor,1) then break end
      editor=s
    end
    cmd='"'..editor..'" '..option.editorOptionFast..' | '..cmd
  end
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
      cmd=cmd..' 2>>"'..log:gsub('[&%^]','^%0')..'"'
    end
  end
  if hls then
    -- セグメント長は既定値(2秒)なので概ねキーフレーム(4～5秒)間隔
    cmd=cmd..' | "'..tsmemseg..'"'..(hls4>0 and ' -4' or '')..' -a 10 -r 100 -m 8192 -d 3 '..segmentKey..'_'
  elseif XCODE_BUF>0 then
    cmd=cmd..' | "'..asyncbuf..'" '..XCODE_BUF..' '..XCODE_PREPARE
  end
  local sync=edcb.GetPrivateProfile('SET','KeepDisk',0,'EpgTimerSrv.ini')~='0'

  -- コマンドが対応していればffmpeg暴走回避のオプションをつける
  local c5or1,stat,code=edcb.os.execute('"'..tsreadex..'" -n -1 -c 5 -h')
  c5or1=(c5or1 or (stat=='exit' and code==2)) and 5 or 1
  -- "-z"はプロセス検索用
  cmd='"'..tsreadex..'" -z edcb-legacy-'..searchName..' -s '..offset..' -l 16384 -t 6'..(sync and ' -m 1' or '')..' -x 18/38/39 -n -1 -a 9 -b 1 -c '..c5or1..' -u 2 "'..fpath:gsub('[&%^]','^%0')..'" | '..cmd
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
  local tools=EdcbModulePath()..'\\Tools'
  local tsreadex=(edcb.FindFile(tools..'\\tsreadex.exe',1) and tools..'\\' or '')..'tsreadex.exe'
  local psisiarc=(edcb.FindFile(tools..'\\psisiarc.exe',1) and tools..'\\' or '')..'psisiarc.exe'
  local sync=edcb.GetPrivateProfile('SET','KeepDisk',0,'EpgTimerSrv.ini')~='0'
  -- 3秒間隔で出力
  local cmd='"'..psisiarc..'" -r arib-data -i 3 - -'
  cmd='"'..tsreadex..'" -s '..offset..' -l 16384 -t 6'..(sync and ' -m 1' or '')..' "'..fpath:gsub('[&%^]','^%0')..'" | '..cmd
  return edcb.io.popen('"'..cmd..'"','rb')
end

function OpenJikkyoReader(tot,nid,sid)
  local id=GetJikkyoID(nid,sid)
  if not id or not JKRDLOG_PATH then return nil end
  local cmd='"'..JKRDLOG_PATH..'" -r '..(fastRate*100)..' '..id..' '..(tot+ofssec)..' 0'
  return edcb.io.popen('"'..cmd..'"','r')
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

function CreateHlsPlaylist(f)
  local a={'#EXTM3U\n'}
  local hasSeg=false
  local buf=f:read(16)
  if buf and #buf==16 then
    local segNum=buf:byte(1)
    local endList=buf:byte(9)~=0
    local segIncomplete=buf:byte(10)~=0
    local isMp4=buf:byte(11)~=0
    a[2]='#EXT-X-VERSION:'..(isMp4 and 6 or 3)..'\n#EXT-X-TARGETDURATION:6\n'
    buf=f:read(segNum*16)
    if not buf or #buf~=segNum*16 then
      segNum=0
    end
    for i=1,segNum do
      local segIndex=buf:byte(1)
      local segCount=GetLeNumber(buf,5,3)
      local segAvailable=buf:byte(8)==0
      local segDuration=GetLeNumber(buf,9,3)/1000
      local nextSegAvailable=i<segNum and buf:byte(16+8)==0
      if segAvailable and (not segIncomplete or nextSegAvailable) then
        if not hasSeg then
          a[#a+1]='#EXT-X-MEDIA-SEQUENCE:'..segCount..'\n'
            ..(isMp4 and '#EXT-X-MAP:URI="mp4init.lua?c='..segmentKey..'"\n' or '')
            ..(endList and '#EXT-X-ENDLIST\n' or '')
          hasSeg=true
        end
        a[#a+1]='#EXTINF:'..segDuration..',\nsegment.lua?c='..segmentKey..('_%02d_%d\n'):format(segIndex,segCount)
      end
      buf=buf:sub(17)
    end
  end
  return table.concat(a)
end

f=nil
if fpath then
  if hls and not psidata and not jikkyo then
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
        if ofssec then
          -- 時間シーク
          offset=0
          if ofssec~=0 then
            fsec,fsize=GetDurationSec(f)
            if SeekSec(f,ofssec,fsec,fsize) then
              offset=f:seek('cur',0) or 0
            end
          end
        else
          -- 比率シーク
          ofssec=0
          if offset~=0 then
            fsec,fsize=GetDurationSec(f)
            ofssec=math.floor(fsec*offset/100)
            if offset~=100 and SeekSec(f,ofssec,fsec,fsize) then
              offset=f:seek('cur',0) or 0
            else
              offset=math.floor(fsize*offset/100/188)*188
            end
          end
        end
        if psidata or jikkyo then
          if jikkyo then
            tot,nid,sid=GetTotAndServiceID(f)
          end
          f:close()
          f={}
          if psidata then
            f.psi=OpenPsiDataArchiver()
            if not f.psi then
              f=nil
            end
          end
          if f and jikkyo then
            f.jk=tot and OpenJikkyoReader(tot,nid,sid)
            if not f.jk then
              if f.psi then f.psi:close() end
              f=nil
            end
          end
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
elseif psidata or jikkyo then
  -- PSI/SI、実況、またはその混合データストリームを返す
  mg.write(Response(200,mg.get_mime_type(fname),'utf-8')..'Content-Disposition: filename='..fname..'\r\n\r\n')
  if mg.request_info.request_method~='HEAD' then
    trailerSize=0
    trailerRemainSize=0
    baseTime=0
    failed=false
    repeat
      if psidata then
        -- 3/fastRate秒間隔でチャンクを読めば主ストリームと等速になる
        buf,trailerSize,trailerRemainSize=ReadPsiDataChunk(f.psi,trailerSize,trailerRemainSize)
        failed=not buf or not mg.write(mg.base64_encode(buf))
        if failed then break end
      end
      if jikkyo then
        for i=1,3 do
          -- 1/fastRate秒間隔でブロックされる
          buf=ReadJikkyoChunk(f.jk)
          failed=not buf or not mg.write(buf)
          if failed then break end
        end
      else
        now=os.time()*fastRate
        if math.abs(baseTime-now)>10 then baseTime=now end
        edcb.Sleep(math.max((baseTime+3-now)/fastRate,0)*1000)
        baseTime=baseTime+3
      end
    until failed
  end
  if f.psi then f.psi:close() end
  if f.jk then f.jk:close() end
elseif hls then
  -- インデックスファイルを返す
  m3u=CreateHlsPlaylist(f)
  f:close()
  ct=CreateContentBuilder()
  ct:Append(m3u)
  ct:Finish()
  mg.write(ct:Pop(Response(200,'application/vnd.apple.mpegurl','utf-8',ct.len)..'\r\n'))
else
  mg.write(Response(200,mg.get_mime_type(fname))..'Content-Disposition: filename='..fname..'\r\n\r\n')
  if mg.request_info.request_method~='HEAD' then
    retry=0
    while true do
      buf=f:read(188*128)
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
