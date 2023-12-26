-- 名前付きパイプ(SendTSTCPの送信先:0.0.0.1 ポート:0～65535)を転送するスクリプト

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

query=AssertPost()
if not query then
  -- POSTでなくてもよい
  query=mg.request_info.query_string
  AssertCsrf(query)
end

option=XCODE_OPTIONS[GetVarInt(query,'option',1,#XCODE_OPTIONS) or 1]
audio2=(GetVarInt(query,'audio2',0,1) or 0)+(option.audioStartAt or 0)
filter=GetVarInt(query,'cinema')==1 and option.filterCinema or option.filter or ''
hls=GetVarInt(query,'hls',1)
hls4=GetVarInt(query,'hls4',0) or 0
caption=hls and option.captionHls or option.captionNone or ''
output=hls and option.outputHls or option.output
n=GetVarInt(query,'n') or 0
onid,tsid,sid=GetVarServiceID(query,'id')
if onid==0 and tsid==0 and sid==0 then
  onid=nil
end
if hls and not (ALLOW_HLS and option.outputHls) then
  -- エラーを返す
  n=nil
  onid=nil
end
psidata=GetVarInt(query,'psidata')==1
jikkyo=GetVarInt(query,'jikkyo')==1
hlsMsn=GetVarInt(query,'_HLS_msn',1)
hlsPart=GetVarInt(query,'_HLS_part',0)

-- フラグメント長の目安
partConfigSec=0.8

function OpenTranscoder(pipeName,searchName,nwtvclose,targetSID)
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
      cmd=cmd..' 2>>"'..log:gsub('[&%^]','^%0')..'"'
    end
  end
  if hls then
    -- セグメント長は既定値(2秒)なので概ねキーフレーム(4～5秒)間隔
    -- プロセス終了時に対応するNetworkTVモードも終了させる
    cmd=cmd..' | "'..tsmemseg..'"'..(hls4>0 and ' -4' or '')..' -a 10 -m 8192 -d 3 -p '..partConfigSec..' '
    if nwtvclose then
      if edcb.FindFile(tools..'\\nwtvclose.ps1',1) then
        cmd=cmd..'-c "powershell -NoProfile -ExecutionPolicy RemoteSigned -File nwtvclose.ps1 '..nwtvclose[1]..' '..nwtvclose[2]..'" '
      else
        cmd=cmd.."-c \"..\\EpgTimerSrv.exe /luapost if(edcb.GetPrivateProfile('NWTV','nwtv"..nwtvclose[1].."open','"..nwtvclose[2]
          .."','Setting\\\\HttpPublic.ini')=='"..nwtvclose[2].."')then;edcb.CloseNetworkTV("..nwtvclose[1]..");end\" "
      end
    end
    cmd=cmd..segmentKey..'_'
  elseif XCODE_BUF>0 then
    cmd=cmd..' | "'..asyncbuf..'" '..XCODE_BUF..' '..XCODE_PREPARE
  end

  -- コマンドが対応していればffmpeg暴走回避のオプションをつける
  local c5or1,stat,code=edcb.os.execute('"'..tsreadex..'" -n -1 -c 5 -h')
  c5or1=(c5or1 or (stat=='exit' and code==2)) and 5 or 1
  -- "-z"はプロセス検索用
  cmd='"'..tsreadex..'" -z edcb-legacy-'..searchName..' -t 10 -m 2 -x 18/38/39 -n '..(targetSID or -1)..' -a 9 -b 1 -c '..c5or1..' -u 2 '..pipeName..' | '..cmd
  if hls then
    -- 極端に多く開けないようにする
    local indexCount=#(edcb.FindFile('\\\\.\\pipe\\tsmemseg_*_00',10) or {})
    if indexCount<10 then
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

function OpenPsiDataArchiver(pipeName,targetSID)
  -- コマンドはEDCBのToolsフォルダにあるものを優先する
  local tools=EdcbModulePath()..'\\Tools'
  local tsreadex=(edcb.FindFile(tools..'\\tsreadex.exe',1) and tools..'\\' or '')..'tsreadex.exe'
  local psisiarc=(edcb.FindFile(tools..'\\psisiarc.exe',1) and tools..'\\' or '')..'psisiarc.exe'
  -- 3秒間隔で出力
  local cmd='"'..psisiarc..'" -r arib-data -n '..(targetSID or -1)..' -i 3 - -'
  cmd='"'..tsreadex..'" -t 10 -m 2 '..pipeName..' | '..cmd
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

function CreateHlsPlaylist(f)
  local a={'#EXTM3U\n'}
  local hasSeg=false
  local buf=f:read(16)
  if buf and #buf==16 then
    local segNum=buf:byte(1)
    local endList=buf:byte(9)~=0
    local segIncomplete=buf:byte(10)~=0
    local isMp4=buf:byte(11)~=0
    local partTarget=partConfigSec
    a[2]='#EXT-X-VERSION:'..(isMp4 and (hls4>1 and 9 or 6) or 3)..'\n#EXT-X-TARGETDURATION:6\n'
    buf=f:read(segNum*16)
    if not buf or #buf~=segNum*16 then
      segNum=0
    end
    for i=1,segNum do
      local segIndex=buf:byte(1)
      local fragNum=buf:byte(3)
      local segCount=GetLeNumber(buf,5,3)
      local segAvailable=buf:byte(8)==0
      local segDuration=GetLeNumber(buf,9,3)/1000
      local segTime=GetLeNumber(buf,13,4)
      local timeTag=os.date('!%Y-%m-%dT%H:%M:%S',os.time({year=2020,month=1,day=1})+math.floor(segTime/100))..('.%02d0+00:00'):format(segTime%100)
      local nextSegAvailable=i<segNum and buf:byte(16+8)==0
      local xbuf=f:read(fragNum*16)
      if not xbuf or #xbuf~=fragNum*16 then
        fragNum=0
      end
      for j=1,fragNum do
        local fragDuration=GetLeNumber(xbuf,1,3)/1000
        if hasSeg and hls4>1 then
          partTarget=math.max(fragDuration,partTarget)
          if timeTag then
            -- このタグがないとまずい環境があるらしい
            a[#a+1]='#EXT-X-PROGRAM-DATE-TIME:'..timeTag..'\n'
            timeTag=nil
          end
          a[#a+1]='#EXT-X-PART:DURATION='..fragDuration..',URI="segment.lua?c='..segmentKey..('_%02d_%d_%d"'):format(segIndex,segCount,j)
            ..(j==1 and ',INDEPENDENT=YES' or '')..'\n'
        end
        xbuf=xbuf:sub(17)
      end
      -- v1.4.12現在のhls.jsはプレイリストがセグメントで終わると再生時間がバグるので避ける
      --if segAvailable and (not segIncomplete or nextSegAvailable) then
      if segAvailable and (endList or nextSegAvailable) then
        if not hasSeg then
          a[#a+1]='#EXT-X-MEDIA-SEQUENCE:'..segCount..'\n'
            ..(isMp4 and '#EXT-X-MAP:URI="mp4init.lua?c='..segmentKey..'"\n' or '')
            ..(endList and '#EXT-X-ENDLIST\n' or '')
          hasSeg=true
        end
        if isMp4 and hls4>1 and timeTag then
          a[#a+1]='#EXT-X-PROGRAM-DATE-TIME:'..timeTag..'\n'
        end
        a[#a+1]='#EXTINF:'..segDuration..',\nsegment.lua?c='..segmentKey..('_%02d_%d\n'):format(segIndex,segCount)
      end
      buf=buf:sub(17)
    end
    if isMp4 and hls4>1 then
      -- PART-HOLD-BACKがPART-TARGETのちょうど3倍だとまずい環境があるらしい
      a[2]=a[2]..'#EXT-X-PART-INF:PART-TARGET='..partTarget
        ..'\n#EXT-X-SERVER-CONTROL:CAN-BLOCK-RELOAD=YES,PART-HOLD-BACK='..(partConfigSec*3.5)..'\n'
    end
  end
  return table.concat(a)
end

f=nil
if onid then
  if sid==0 then
    -- NetworkTVモードを終了
    edcb.CloseNetworkTV(n)
  elseif 0<=n and n<100 then
    if hls and not psidata and not jikkyo then
      -- クエリのハッシュをキーとし、同一キーアクセスは出力中のインデックスファイルを返す
      segmentKey=mg.md5('view:'..hls..':nwtv'..n..':'..option.xcoder..':'..option.option..':'..audio2..':'..filter..':'..caption..':'..output[2])
      f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..segmentKey..'_00','rb')
    end
    if not f then
      if psidata or jikkyo then
        ok,pid=edcb.IsOpenNetworkTV(n)
      else
        -- 前回のプロセスが残っていたら終わらせる
        edcb.os.execute('wmic process where "name=\'tsreadex.exe\' and commandline like \'% -z edcb-legacy-nwtv-'..n..' %\'" call terminate >nul')
        openTime=os.time()
        edcb.WritePrivateProfile('NWTV','nwtv'..n..'open','@'..openTime,'Setting\\HttpPublic.ini')
        -- NetworkTVモードを開始
        ok,pid=edcb.OpenNetworkTV(2,onid,tsid,sid,n)
      end
      if ok then
        -- 名前付きパイプができるまで待つ
        pipeName=nil
        for i=1,50 do
          ff=edcb.FindFile('\\\\.\\pipe\\SendTSTCP_*_'..pid, 1)
          if ff and ff[1].name:find('^[A-Za-z]+_%d+_%d+$') then
            pipeName='\\\\.\\pipe\\'..ff[1].name
            break
          elseif i%10==0 then
            -- FindFileで見つけられない環境があるかもしれないのでポートを予想して開いてみる
            for j=0,29 do
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
        if psidata or jikkyo then
          if pipeName then
            f={}
            if psidata then
              f.psi=OpenPsiDataArchiver(pipeName,sid)
              if not f.psi then
                f=nil
              end
            end
            if f and jikkyo then
              f.jk=edcb.io.open('\\\\.\\pipe\\chat_d7b64ac2_'..pid,'r')
              if not f.jk then
                if f.psi then f.psi:close() end
                f=nil
              end
            end
            fname='view.psc.txt'
          end
        else
          if pipeName then
            f=OpenTranscoder(pipeName,'nwtv-'..n,{n,'@'..openTime},sid)
            fname='view.'..output[1]
          end
          if not f then
            edcb.CloseNetworkTV(n)
          end
        end
      end
    end
  end
elseif n and n<0 then
  -- プロセスが残っていたらすべて終わらせる
  edcb.os.execute('wmic process where "name=\'tsreadex.exe\' and commandline like \'% -z edcb-legacy-view-%\'" call terminate >nul')
elseif n and n<=65535 then
  if hls and not psidata and not jikkyo then
    -- クエリのハッシュをキーとし、同一キーアクセスは出力中のインデックスファイルを返す
    segmentKey=mg.md5('view:'..hls..':'..n..':'..option.xcoder..':'..option.option..':'..audio2..':'..filter..':'..caption..':'..output[2])
    f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..segmentKey..'_00','rb')
  end
  if not f then
    if not psidata and not jikkyo then
      -- 前回のプロセスが残っていたら終わらせる
      edcb.os.execute('wmic process where "name=\'tsreadex.exe\' and commandline like \'% -z edcb-legacy-view-'..n..' %\'" call terminate >nul')
    end
    -- 名前付きパイプがあれば開く
    ff=edcb.FindFile('\\\\.\\pipe\\SendTSTCP_'..n..'_*', 1)
    if ff and ff[1].name:find('^[A-Za-z]+_%d+_%d+$') then
      if psidata or jikkyo then
        f={}
        if psidata then
          f.psi=OpenPsiDataArchiver('\\\\.\\pipe\\'..ff[1].name)
          if not f.psi then
            f=nil
          end
        end
        if f and jikkyo then
          f.jk=edcb.io.open('\\\\.\\pipe\\chat_d7b64ac2_'..ff[1].name:match('^[A-Za-z]+_%d+_(%d+)$'),'r')
          if not f.jk then
            if f.psi then f.psi:close() end
            f=nil
          end
        end
        fname='view.psc.txt'
      else
        f=OpenTranscoder('\\\\.\\pipe\\'..ff[1].name,'view-'..n)
        fname='view.'..output[1]
      end
    end
  end
end

if not f then
  ct=CreateContentBuilder()
  ct:Append('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">\n'
    ..'<title>view.lua</title><p><a href="index.html">メニュー</a></p>')
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
      -- 混合のときはPSI/SIのチャンクを3秒間隔で読む
      if psidata then
        buf,trailerSize,trailerRemainSize=ReadPsiDataChunk(f.psi,trailerSize,trailerRemainSize)
        failed=not buf or not mg.write(mg.base64_encode(buf))
        if failed then break end
      end
      if jikkyo then
        repeat
          -- 短い間隔(おおむね1秒以下)で読めることを仮定
          buf=ReadJikkyoChunk(f.jk)
          failed=not buf or not mg.write(buf)
          if failed then break end
          now=os.time()
          if math.abs(baseTime-now)>10 then baseTime=now end
        until now>=baseTime+3
        baseTime=baseTime+3
      end
    until failed
  end
  if f.psi then f.psi:close() end
  if f.jk then f.jk:close() end
elseif hls then
  -- インデックスファイルを返す
  i=1
  while true do
    m3u=CreateHlsPlaylist(f)
    f:close()
    if i>40 or not hlsMsn or m3u:find('EXT%-X%-ENDLIST') or not m3u:find('CAN%-BLOCK%-RELOAD') or
       not m3u:find('_'..(hlsMsn-1)..'\n') or
       m3u:find('_'..hlsMsn..'\n') or
       (hlsPart and m3u:find('_'..hlsMsn..'_'..(hlsPart+1)..'"')) then
      break
    end
    edcb.Sleep(200)
    f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..segmentKey..'_00','rb')
    if not f then break end
    i=i+1
  end
  ct=CreateContentBuilder()
  ct:Append(m3u)
  ct:Finish()
  mg.write(ct:Pop(Response(200,'application/vnd.apple.mpegurl','utf-8',ct.len)..'\r\n'))
else
  mg.write(Response(200,mg.get_mime_type(fname))..'Content-Disposition: filename='..fname..'\r\n\r\n')
  if mg.request_info.request_method~='HEAD' then
    while true do
      buf=f:read(188*128)
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
