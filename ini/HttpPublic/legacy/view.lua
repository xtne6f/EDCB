-- 名前付きパイプ(SendTSTCPの送信先:0.0.0.1 ポート:0～65535)を転送するスクリプト

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')
dofile(mg.script_name:gsub('[^\\/]*$','')..'jkconst.lua')

-- HLSの開始はPOSTでなければならない
query=AssertPost()
open=query and GetVarInt(query,'open')==1
if open or not query then
  -- URLクエリにもCSRF対策トークンが必要
  query=mg.request_info.query_string
  AssertCsrf(query)
end

option=XCODE_OPTIONS[GetVarInt(query,'option',1,#XCODE_OPTIONS) or 1]
audio2=(GetVarInt(query,'audio2',0,1) or 0)+(option.audioStartAt or 0)
filter=GetVarInt(query,'cinema')==1 and (type(option.filterCinema)=='function' and option.filterCinema(1) or type(option.filterCinema)=='string' and option.filterCinema) or
                                        type(option.filter)=='function' and option.filter(1) or type(option.filter)=='string' and option.filter or ''
hlsKey=mg.get_var(query,'hls')
hls4=GetVarInt(query,'hls4',0) or 0
output=hlsKey and option.outputHls or option.output
n=GetVarInt(query,'n') or 0
onid,tsid,sid=GetVarServiceID(query,'id')
if onid==0 and tsid==0 and sid==0 then
  onid=nil
end
if hlsKey and not (ALLOW_HLS and option.outputHls) then
  -- エラーを返す
  n=nil
  onid=nil
end
psidata=GetVarInt(query,'psidata')==1
jikkyo=GetVarInt(query,'jikkyo')==1
jkID=GetVarInt(query,'jkid',1,65535)
hlsMsn=GetVarInt(query,'_HLS_msn',1)
hlsPart=GetVarInt(query,'_HLS_part',0)

-- クエリのハッシュをキーとし、同一キーアクセスは出力中のインデックスファイルを返す
hlsKey=hlsKey and n and mg.md5('view:'..hlsKey..(onid and ':nwtv' or ':')..n)

-- フラグメント長の目安
partConfigSec=0.8

-- トランスコードを開始し、HLSの場合はインデックスファイルの情報、それ以外はMP4などのストリーム自体を返す
function OpenTranscoder(pipeName,searchName,nwtvclose,targetSID)
  if XCODE_SINGLE then
    -- パイプラインの上流をすべて終わらせる
    TerminateCommandlineLike('tsreadex',' -z edcb-legacy-')
  end

  local tools=PathAppend(EdcbModulePath(),'Tools')
  local tsreadex=FindToolsCommand('tsreadex')
  local asyncbuf=FindToolsCommand('asyncbuf')
  local tsmemseg=FindToolsCommand('tsmemseg')
  local cmd=''
  if filter~=':' then
    local xcoder=''
    if WIN32 then
      for s in option.xcoder:gmatch('[^|]+') do
        xcoder=PathAppend(tools,s)
        if EdcbFindFilePlain(xcoder) then break end
        xcoder=s
      end
      xcoder='"'..xcoder..'"'
    else
      xcoder=('|'..option.xcoder:gsub('%.exe$','')):match('[\\/|]([0-9A-Za-z._-]+)$')
      xcoder=xcoder and FindToolsCommand(xcoder) or ':'
    end
    -- gsub('%%','%%%%')は置換文字列中の特殊文字を無効化するため
    cmd=' | '..xcoder..' '..option.option
      :gsub('$AUDIO',audio2)
      :gsub('$FILTER',(filter:gsub('%%','%%%%')))
      :gsub('$CAPTION','')
      :gsub('$OUTPUT',(output[2]:gsub('%%','%%%%')))
  end

  if XCODE_LOG and cmd~='' then
    local log=mg.script_name:gsub('[^\\/]*$','')..'log'
    if not EdcbFindFilePlain(log) then
      edcb.os.execute('mkdir '..QuoteCommandArgForPath(log))
    end
    -- 衝突しにくいログファイル名を作る
    log=PathAppend(log,'view-'..os.time()..'-'..mg.md5(cmd):sub(29)..'.txt')
    local f=edcb.io.open(log,'w')
    if f then
      f:write(cmd:sub(4)..'\n\n')
      f:close()
      cmd=cmd..' 2>>'..QuoteCommandArgForPath(log,hlsKey)
    end
  end
  if hlsKey then
    -- セグメント長は既定値(2秒)なので概ねキーフレーム(4～5秒)間隔
    -- プロセス終了時に対応するNetworkTVモードも終了させる
    cmd=cmd..' | '..tsmemseg..(hls4>0 and ' -4' or '')..' -a 10 -m 8192 -p '..partConfigSec..' '..(WIN32 and '' or '-g '..QuoteCommandArgForPath(EdcbModulePath(),hlsKey)..' ')
    if nwtvclose then
      cmd=cmd..(WIN32 and '-c "..\\EpgTimerSrv.exe /luapost ' or '-c "echo \\"')
        ..'ok,pid,openID=edcb.IsOpenNetworkTV('..nwtvclose[1]..');if(ok)and(openID=='..(nwtvclose[2] or 'nil')..')then;edcb.CloseNetworkTV('..nwtvclose[1]..');end'
        ..(WIN32 and '" ' or '\\" >>\\"'..PathAppend(EdcbModulePath(),'EpgTimerSrvLuaPost.fifo')..'\\"" ')
    end
    cmd=cmd..hlsKey..'_'
  elseif XCODE_BUF>0 then
    cmd=cmd..' | '..asyncbuf..' '..XCODE_BUF..' '..XCODE_PREPARE
  end

  -- "-z"はプロセス検索用
  cmd=tsreadex..' -z edcb-legacy-'..searchName..' -t 10 -m 2 -x 18/38/39 -n '..(targetSID or -1)..' -a 9 -b 1 -c '
    ..(XCODE_USING_OLD_TRANSCODER and not option.passthroughCaption and 2 or 5)..' -u 2 '..((XCODE_USING_OLD_TRANSCODER or option.passthroughCaption) and '' or '-d 1 ')
    ..QuoteCommandArgForPath(SendTSTCPPipePath(pipeName,0),hlsKey)..cmd
  if hlsKey then
    -- 極端に多く開けないようにする
    local indexCount=#(edcb.FindFile(TsmemsegPipePath('*_','00'),10) or {})
    if indexCount<10 then
      edcb.os.execute(WIN32 and 'start "" /b cmd /s /c "'..(nwtvclose and 'cd /d "'..tools..'" && ' or '')..cmd..'"' or cmd..' &')
      for i=1,100 do
        local f=OpenTsmemsegPipe(hlsKey..'_','00')
        if f then
          return f
        end
        edcb.Sleep(100)
      end
      -- 失敗。プロセスが残っていたら終わらせる
      TerminateCommandlineLike('tsreadex',' -z edcb-legacy-'..searchName..' ')
    end
    return nil
  end
  return edcb.io.popen(WIN32 and '"'..cmd..'"' or cmd,'r'..POPEN_BINARY)
end

function OpenPsiDataArchiver(pipeName,targetSID)
  local tsreadex=FindToolsCommand('tsreadex')
  local psisiarc=FindToolsCommand('psisiarc')
  -- 3秒間隔で出力
  local cmd=psisiarc..' -r arib-data -n '..(targetSID or -1)..' -i 3 - -'
  cmd=tsreadex..' -t 10 -m 2 '..QuoteCommandArgForPath(SendTSTCPPipePath(pipeName,1))..' | '..cmd
  return edcb.io.popen(WIN32 and '"'..cmd..'"' or cmd,'r'..POPEN_BINARY)
end

function OpenLiveJikkyo(pid,jkID,nid,sid)
  if JKCNSL_PATH then
    if not jkID and not nid and edcb.GetTunerProcessStatusAll then
      -- 起動中のチューナのONIDとTSIDを調べ、チャンネル情報から適当にSIDを得る
      local t=nil
      for i,v in ipairs(edcb.GetTunerProcessStatusAll()) do
        if v.processID==pid then
          t=v
          break
        end
      end
      if t and t.onid>=0 and t.tsid>=0 then
        t.sid=0
        local chList=edcb.GetChDataList()
        local chIndex=BinarySearchBound(chList,t,CompareFields('onid',false,'tsid',false,'sid'))
        if chIndex<=#chList and chList[chIndex].onid==t.onid and chList[chIndex].tsid==t.tsid then
          nid=t.onid
          sid=chList[chIndex].sid
        end
      end
    end
    jkID=jkID or (nid and GetJikkyoID(nid,sid))
    if not jkID then
      return 'Unable to determine Jikkyo ID.'
    end
    local chatName,chatID,refugeID=GetChatStreamName(jkID)
    if not (refugeID and JKCNSL_REFUGE_URI or chatID and (not JKCNSL_REFUGE_URI or JKCNSL_REFUGE_MIXING)) then
      return 'Unable to determine chat ID.'
    end
    local postName='jkcnsl_edcb_'..pid
    local cmd=(WIN32 and QuoteCommandArgForPath(JKCNSL_PATH) or FindToolsCommand(JKCNSL_PATH))..' -i -n '..postName..' -p '..pid
      ..(WIN32 and '' or ' -d '..QuoteCommandArgForPath(JKCNSL_UNIX_BASE_DIR))
      ..(JKCNSL_ANONYMITY and ' --post-as-anon' or '')..' --post-drop-dup --post-interval 2000 --post-maxlen 80'
    if refugeID and JKCNSL_REFUGE_URI then
      -- 避難所または避難所との混合接続
      local uri=JKCNSL_REFUGE_URI:gsub('{jkID}','jk'..jkID):gsub('{chatStreamID}',refugeID)
      local mix=JKCNSL_REFUGE_MIXING and chatID
      cmd=cmd..' -c "R'..((JKCNSL_DROP_FORWARDED_COMMENT or mix) and '2 ' or '1 ')..uri..(mix and ' '..chatID or '')..'"'
    else
      cmd=cmd..' -c "L'..chatID..'"'
    end
    local f=edcb.io.popen(WIN32 and '"'..cmd..'"' or cmd)
    if f then
      return f,jkID,function()
        -- .NETアプリは出力端を閉じたことをアプリ側で検知するのが難しいので、'q'コマンドを投稿して閉じる
        if WIN32 then
          local fpost=edcb.io.open('\\\\.\\pipe\\'..postName,'w')
          if fpost then
            for i=0,300 do
              if not fpost:write('q\n') or not fpost:flush() then break end
              edcb.Sleep(10)
            end
            fpost:close()
          end
        else
          local fpost
          for retry=1,9 do
            fpost=EdcbFindFilePlain(PathAppend(JKCNSL_UNIX_BASE_DIR,postName)) and
              edcb.io.open(PathAppend(JKCNSL_UNIX_BASE_DIR,postName),'a')
            -- なるべく同時に書き込まないようプロセス間でロックが必要
            if not fpost or edcb.io._flock_nb(fpost) or retry==9 then break end
            fpost:close()
            fpost=nil
            edcb.Sleep(10*retry)
          end
          if fpost then
            fpost:write('q\nq\n')
            fpost:flush()
            fpost:close()
          end
        end
        f:close()
      end
    end
  else
    local f=edcb.io.open(WIN32 and '\\\\.\\pipe\\chat_d7b64ac2_'..pid or PathAppend(JKTASK_BASE_DIR,'chat_'..pid..'.fifo'),'r')
    if not f then
      return 'No pipe found for reading comments.'
    end
    if not WIN32 and not edcb.io._flock_nb(f) then
      f:close()
      return 'Pipe for reading comments is already in use.'
    end
    return f,-1,function() f:close() end
  end
  return nil
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
          a[#a+1]='#EXT-X-PART:DURATION='..fragDuration..',URI="segment.lua?c='..hlsKey..('_%02d_%d_%d"'):format(segIndex,segCount,j)
            ..(j==1 and ',INDEPENDENT=YES' or '')..'\n'
        end
        xbuf=xbuf:sub(17)
      end
      -- v1.4.12現在のhls.jsはプレイリストがセグメントで終わると再生時間がバグるので避ける
      --if segAvailable and (not segIncomplete or nextSegAvailable) then
      if segAvailable and (endList or nextSegAvailable) then
        if not hasSeg then
          a[#a+1]='#EXT-X-MEDIA-SEQUENCE:'..segCount..'\n'
            ..(isMp4 and '#EXT-X-MAP:URI="mp4init.lua?c='..hlsKey..'"\n' or '')
            ..(endList and '#EXT-X-ENDLIST\n' or '')
          hasSeg=true
        end
        if isMp4 and hls4>1 and timeTag then
          a[#a+1]='#EXT-X-PROGRAM-DATE-TIME:'..timeTag..'\n'
        end
        a[#a+1]='#EXTINF:'..segDuration..',\nsegment.lua?c='..hlsKey..('_%02d_%d\n'):format(segIndex,segCount)
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
    if hlsKey and not open and not psidata and not jikkyo then
      f=OpenTsmemsegPipe(hlsKey..'_','00')
    else
      if psidata or jikkyo then
        ok,pid=edcb.IsOpenNetworkTV(n)
      else
        -- 前回のプロセスが残っていたら終わらせる
        TerminateCommandlineLike('tsreadex',' -z edcb-legacy-nwtv-'..n..' ')
        -- NetworkTVモードを開始
        ok,pid,myOpenID=edcb.OpenNetworkTV(2,onid,tsid,sid,n)
      end
      if ok then
        -- 名前付きパイプができるまで待つ
        pipeName=nil
        for i=1,50 do
          ff=edcb.FindFile(SendTSTCPPipePath('*_'..pid,0),1)
          if ff and ff[1].name:find('^[^_]+_%d+_%d+') then
            pipeName=ff[1].name:match('^[^_]+_(%d+_)%d+')..pid
            break
          elseif WIN32 and i%10==0 then
            -- FindFileで見つけられない環境があるかもしれないのでポートを予想して開いてみる
            for j=0,29 do
              ff=edcb.io.open(SendTSTCPPipePath(j..'_'..pid,0),'rb')
              if ff then
                ff:close()
                -- 再び開けるようになるまで少しラグがある
                edcb.Sleep(2000)
                pipeName=j..'_'..pid
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
              f.jk,f.jkID,f.closeJK=OpenLiveJikkyo(pid,jkID,onid,sid)
              if not f.jk then
                if f.psi then f.psi:close() end
                f=nil
              end
            end
            fname='view.psc.txt'
          end
        else
          if pipeName then
            f=OpenTranscoder(pipeName,'nwtv-'..n,{n,myOpenID},sid)
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
  TerminateCommandlineLike('tsreadex',' -z edcb-legacy-view-')
elseif n and n<=65535 then
  if hlsKey and not open and not psidata and not jikkyo then
    f=OpenTsmemsegPipe(hlsKey..'_','00')
  else
    if not psidata and not jikkyo then
      -- 前回のプロセスが残っていたら終わらせる
      TerminateCommandlineLike('tsreadex',' -z edcb-legacy-view-'..n..' ')
    end
    -- 名前付きパイプがあれば開く
    ff=edcb.FindFile(SendTSTCPPipePath(n..'_*',0),1)
    if ff and ff[1].name:find('^[^_]+_%d+_%d+') then
      pid=ff[1].name:match('^[^_]+_%d+_(%d+)')
      if psidata or jikkyo then
        f={}
        if psidata then
          f.psi=OpenPsiDataArchiver(n..'_'..pid)
          if not f.psi then
            f=nil
          end
        end
        if f and jikkyo then
          if tonumber(pid) then
            f.jk,f.jkID,f.closeJK=OpenLiveJikkyo(tonumber(pid),jkID)
          end
          if not f.jk then
            if f.psi then f.psi:close() end
            f=nil
          end
        end
        fname='view.psc.txt'
      else
        f=OpenTranscoder(n..'_'..pid,'view-'..n)
        fname='view.'..output[1]
      end
    end
  end
end

if not f then
  mg.write(Response(404,nil,nil,0)..'\r\n')
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
      if jikkyo and type(f.jk)=='string' then
        -- メッセージを送って混合でなければ終了
        failed=not mg.write('<!-- M='..f.jk..' -->\n') or not psidata
        f.jk=nil
      end
      if jikkyo and f.jk then
        repeat
          -- 短い間隔(おおむね1秒以下)で読めることを仮定
          buf=ReadJikkyoChunk(f.jk)
          -- ヘッダに実況のIDがないときは補う
          if buf and buf:find('^<!%-%- ') and not buf:find('^<!%-%- J=') then
            buf='<!-- J='..f.jkID..';'..buf:sub(6)
          end
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
  if f.closeJK then f.closeJK() end
elseif hlsKey then
  -- インデックスファイルを返す
  i=1
  repeat
    m3u=CreateHlsPlaylist(f)
    f:close()
    if not m3u:find('#EXT%-X%-MEDIA%-SEQUENCE:') then
      -- 最初のセグメントができるまでは2秒だけ応答保留する
      if i>10 then break end
    elseif i>40 or not hlsMsn or m3u:find('#EXT%-X%-ENDLIST') or not m3u:find('CAN%-BLOCK%-RELOAD') or
       not m3u:find('_'..(hlsMsn-1)..'\n') or
       m3u:find('_'..hlsMsn..'\n') or
       (hlsPart and m3u:find('_'..hlsMsn..'_'..(hlsPart+1)..'"')) then
      break
    end
    edcb.Sleep(200)
    f=OpenTsmemsegPipe(hlsKey..'_','00')
    i=i+1
  until not f
  ct=CreateContentBuilder()
  ct:Append(m3u)
  ct:Finish()
  mg.write(ct:Pop(Response(200,'application/vnd.apple.mpegurl','utf-8',ct.len)..'\r\n'))
else
  mg.write(Response(200,mg.get_mime_type(fname))..'Content-Disposition: attachment; filename='..fname..'\r\n\r\n')
  if mg.request_info.request_method~='HEAD' then
    while true do
      buf=f:read(188*128)
      if not buf or #buf==0 then
        -- 終端に達した
        break
      end
      if not mg.write(buf) then
        -- キャンセルされた
        break
      end
    end
  end
  f:close()
  if onid then
    -- NetworkTVモードを終了
    -- リロード時などの終了を防ぐ。厳密にはロックなどが必要だが概ねうまくいけば良い
    ok,pid,openID=edcb.IsOpenNetworkTV(n)
    if ok and openID==myOpenID then
      -- チャンネル変更のため終了を遅らせる
      edcb.os.execute((WIN32 and 'start "" /b cmd /s /c "timeout 5 & cd /d "'..EdcbModulePath()..'" && .\\EpgTimerSrv.exe /luapost ' or '(sleep 5 ; echo "')
        ..'ok,pid,openID=edcb.IsOpenNetworkTV('..n..');if(ok)and(openID=='..(myOpenID or 'nil')..')then;edcb.CloseNetworkTV('..n..');end'
        ..(WIN32 and '"' or '" >>"'..PathAppend(EdcbModulePath(),'EpgTimerSrvLuaPost.fifo')..'") &'))
    end
  end
end
