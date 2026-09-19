-- ファイルを転送するスクリプト
-- ファイルをタイムシフト再生できる: http://localhost:5510/xcode.lua?fname=video/foo.ts

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')
dofile(mg.script_name:gsub('[^\\/]*$','')..'jkconst.lua')

-- HLSの開始はPOSTでなければならない
query=AssertPost()
open=query and GetVarInt(query,'open')==1
query=mg.request_info.query_string
fpath=mg.get_var(query,'fname')
if fpath then
  fname=mg.md5(fpath:upper()):sub(27)
  fpath=DocumentToNativePath(fpath)
end

ofssec=GetVarInt(query,'ofssec',0,100000) or 0
option=XCODE_OPTIONS[GetVarInt(query,'option',1,#XCODE_OPTIONS) or 1]
audio2=(GetVarInt(query,'audio2',0,1) or 0)+(option.audioStartAt or 0)
fastRate=GetVarInt(query,'fast',1,#XCODE_FAST_RATES)
fastRate=fastRate and XCODE_FAST_RATES[fastRate] or 1
filter=fastRate~=1 and (GetVarInt(query,'cinema')==1 and option.filterCinemaFastFunc and option.filterCinemaFastFunc(fastRate) or
                        option.filterFastFunc and option.filterFastFunc(fastRate))
fastRate=filter and fastRate or 1
throttle=GetVarInt(query,'throttle')==1
filter=filter or (GetVarInt(query,'cinema')==1 and option.filterCinema or option.filter or '')
hlsKey=mg.get_var(query,'hls')
hls4=GetVarInt(query,'hls4',0) or 0
hlsCheckSrc=GetVarInt(query,'hls_x_src')==1
caption=hlsKey and option.captionHls or option.captionNone or ''
output=hlsKey and option.outputHls or option.output
if hlsKey and not (ALLOW_HLS and option.outputHls) then
  -- エラーを返す
  fpath=nil
end
psidata=GetVarInt(query,'psidata')==1
jikkyo=GetVarInt(query,'jikkyo')==1
jkID=GetVarInt(query,'jkid',1,65535)
jkTM=GetVarInt(query,'jktm',1)
reload=mg.get_var(query,'reload')
loadKey=reload or mg.get_var(query,'load') or ''

-- クエリのハッシュをキーとし、同一キーアクセスは出力中のインデックスファイルを返す
hlsKey=hlsKey and fpath and mg.md5('xcode:'..hlsKey..':'..fpath)

-- トランスコードを開始し、HLSの場合はインデックスファイルの情報、それ以外はMP4などのストリーム自体を返す
function OpenTranscoder(offset)
  local searchName='xcode-'..mg.md5(loadKey):sub(17)
  if XCODE_SINGLE then
    -- パイプラインの上流をすべて終わらせる
    TerminateCommandlineLike('tsreadex',' -z edcb-legacy-')
  elseif reload then
    -- リロード時は前回のプロセスを速やかに終わらせる
    TerminateCommandlineLike('tsreadex',' -z edcb-legacy-'..searchName..' ')
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
      :gsub('$CAPTION',(caption:gsub('%%','%%%%')))
      :gsub('$OUTPUT',(output[2]:gsub('%%','%%%%')))
  end

  if fastRate~=1 and option.editorFast and option.editorOptionFastFunc then
    local editor=''
    if WIN32 then
      for s in option.editorFast:gmatch('[^|]+') do
        editor=PathAppend(tools,s)
        if EdcbFindFilePlain(editor) then break end
        editor=s
      end
      editor='"'..editor..'"'
    else
      editor=('|'..option.editorFast:gsub('%.exe$','')):match('[\\/|]([0-9A-Za-z._-]+)$') or ':'
    end
    cmd=' | '..editor..' '..option.editorOptionFastFunc(fastRate)..cmd
  end
  if XCODE_LOG and cmd~='' then
    local log=mg.script_name:gsub('[^\\/]*$','')..'log'
    if not EdcbFindFilePlain(log) then
      edcb.os.execute('mkdir '..QuoteCommandArgForPath(log))
    end
    -- 衝突しにくいログファイル名を作る
    log=PathAppend(log,'xcode-'..os.time()..'-'..mg.md5(cmd):sub(29)..'.txt')
    local f=edcb.io.open(log,'w')
    if f then
      f:write(cmd:sub(4)..'\n\n')
      f:close()
      cmd=cmd..' 2>>'..QuoteCommandArgForPath(log,hlsKey)
    end
  end
  if hlsKey then
    -- セグメント長は既定値(2秒)なので概ねキーフレーム(4～5秒)間隔
    cmd=cmd..' | '..tsmemseg..(hls4>0 and ' -4' or '')..' -a 10 -r 100 -m 8192 -d 3 '..(WIN32 and '' or '-g '..QuoteCommandArgForPath(EdcbModulePath(),hlsKey)..' ')..hlsKey..'_'
  elseif XCODE_BUF>0 then
    cmd=cmd..' | '..asyncbuf..' '..XCODE_BUF..' '..XCODE_PREPARE
  end
  -- 容量確保の仕組みが異なるのでWindows以外では終端への配慮は不要
  local sync=WIN32 and edcb.GetPrivateProfile('SET','KeepDisk',0,'EpgTimerSrv.ini')~='0'

  -- "-z"はプロセス検索用
  cmd=tsreadex..' -z edcb-legacy-'..searchName..' -s '..offset..' -l 16384 -t 6'..(sync and ' -m 1' or '')..' -x 18/38/39 -n -1 -a 9 -b 1 -c 5 -u 2 '..QuoteCommandArgForPath(fpath,hlsKey)..cmd
  if hlsKey then
    -- 極端に多く開けないようにする
    local indexCount=#(edcb.FindFile(TsmemsegPipePath('*_','00'),10) or {})
    if indexCount<10 then
      edcb.os.execute(WIN32 and 'start "" /b cmd /c "'..cmd..'"' or cmd..' &')
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

function OpenPsiDataArchiver(offset)
  local tsreadex=FindToolsCommand('tsreadex')
  local psisiarc=FindToolsCommand('psisiarc')
  -- 容量確保の仕組みが異なるのでWindows以外では終端への配慮は不要
  local sync=WIN32 and edcb.GetPrivateProfile('SET','KeepDisk',0,'EpgTimerSrv.ini')~='0'
  -- 3秒間隔で出力
  local cmd=psisiarc..' -r arib-data -i 3 - -'
  cmd=tsreadex..' -s '..offset..' -l 16384 -t 6'..(sync and ' -m 1' or '')..' '..QuoteCommandArgForPath(fpath)..' | '..cmd
  return edcb.io.popen(WIN32 and '"'..cmd..'"' or cmd,'r'..POPEN_BINARY)
end

function OpenJikkyoReader(tot,jkID,nid,sid)
  if JKRDLOG_PATH then
    jkID=jkID or GetJikkyoID(nid,sid)
    if not jkID then
      return 'Unable to determine Jikkyo ID.'
    end
    local cmd=(WIN32 and QuoteCommandArgForPath(JKRDLOG_PATH) or FindToolsCommand(JKRDLOG_PATH))..' -r '..(fastRate*100)..' jk'..jkID..' '..(tot+ofssec)..' 0'
    return edcb.io.popen(WIN32 and '"'..cmd..'"' or cmd)
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
            ..(isMp4 and '#EXT-X-MAP:URI="mp4init.lua?c='..hlsKey..'"\n' or '')
            ..(endList and '#EXT-X-ENDLIST\n' or '')
          hasSeg=true
        end
        a[#a+1]='#EXTINF:'..segDuration..',\nsegment.lua?c='..hlsKey..('_%02d_%d\n'):format(segIndex,segCount)
      end
      buf=buf:sub(17)
    end
  end
  return table.concat(a)
end

f=nil
if fpath then
  if hlsKey and not open and not psidata and not jikkyo then
    f=OpenTsmemsegPipe(hlsKey..'_','00')
  else
    ext=fpath:match('%.[0-9A-Za-z]+$') or ''
    extts=edcb.GetPrivateProfile('SET','TSExt','.ts','EpgTimerSrv.ini')
    -- 拡張子を限定
    if IsEqualPath(ext,extts) then
      f=edcb.io.open(fpath,'rb')
      if f then
        if ofssec~=0 then
          -- 時間シーク
          fsec,fsize=GetDurationSec(f)
          ofssec=math.min(ofssec,fsec)
          -- 応答性向上のためPSI/SIは6秒(チャンク2つ)だけ手前から読む
          SeekSec(f,ofssec-(psidata and 6 or 0),fsec,fsize)
        end
        offset=f:seek('cur',0) or 0
        if psidata or jikkyo then
          if jikkyo then
            tot,nid,sid=GetTotAndServiceID(f)
          end
          f:close()
          f={}
          if psidata then
            f.psi=OpenPsiDataArchiver(offset)
            if not f.psi then
              f=nil
            end
          end
          if f and jikkyo then
            f.jk=tot and OpenJikkyoReader(jkTM or tot,jkID,nid,sid)
            if not f.jk then
              if f.psi then f.psi:close() end
              f=nil
            end
          end
          fname=fname..'.psc.txt'
        else
          f:close()
          f=OpenTranscoder(offset)
          fname=fname..'.'..output[1]
        end
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
      if psidata then
        -- 3/fastRate秒間隔でチャンクを読めば主ストリームと等速になる。初回だけ3つ読む
        for i=(trailerSize==0 and 1 or 3),3 do
          buf,trailerSize,trailerRemainSize=ReadPsiDataChunk(f.psi,trailerSize,trailerRemainSize)
          failed=not buf or not mg.write(mg.base64_encode(buf))
          if failed then break end
        end
        if failed then break end
      end
      if jikkyo and type(f.jk)=='string' then
        -- メッセージを送って混合でなければ終了
        failed=not mg.write('<!-- M='..f.jk..' -->\n') or not psidata
        f.jk=nil
      end
      if jikkyo and f.jk then
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
  if f.jk and type(f.jk)~='string' then f.jk:close() end
elseif hlsKey then
  -- インデックスファイルを返す
  i=1
  repeat
    m3u=CreateHlsPlaylist(f)
    f:close()
    if not m3u:find('#EXT%-X%-MEDIA%-SEQUENCE:') then
      -- 最初のセグメントができるまでは2秒だけ応答保留する
      if i>10 then break end
    else
      if hlsCheckSrc then
        -- 現在のファイルの長さをコメントとして追記する
        srcf=edcb.io.open(fpath,'rb')
        if srcf then
          fsec,fsize=GetDurationSec(srcf)
          srcf:close()
          m3u=m3u:gsub('#EXT%-X%-MEDIA%-SEQUENCE:','#X-SRC-DURATION:'..fsec..','..fsize..'\n%0')
        end
      end
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
    bufRemain=''
    throttle=throttle and fname:find('%.m2t$')
    ts={}
    baseTime=0
    basePcr=0
    stat=nil
    while true do
      buf=f:read(188*128-#bufRemain)
      if not buf or #buf==0 then
        -- 終端に達した
        break
      end
      if throttle then
        -- 常に188バイト単位にする
        if #bufRemain~=0 then
          buf=bufRemain..buf
          bufRemain=''
        end
        if #buf%188~=0 then
          bufRemain=buf:sub(-(#buf%188))
          buf=buf:sub(1,#buf-#bufRemain)
        end
        -- 送信速度をfastRateまでに制御
        for i=1,#buf,188 do
          if not ParseTsPacket(ts,buf,i) then
            ts=nil
            break
          end
          pcr=GetPcrFromTsPacket(ts.adaptation,buf,i)
          if not ts.err and pcr then
            timeDiff=math.floor(os.time()*fastRate-baseTime)
            pcrDiff=math.floor(UintCounterDiff(pcr,basePcr)/45000)
            if math.abs(timeDiff)>60 or pcrDiff>60 then
              -- 制御をリセット。30秒ほど先読みを許す
              baseTime=os.time()*fastRate+(baseTime==0 and 0 or 30)
              basePcr=pcr
            else
              if timeDiff>=0 then
                baseTime=baseTime+math.min(pcrDiff,timeDiff)
                basePcr=(basePcr+math.min(pcrDiff,timeDiff)*45000)%0x100000000
              end
              if pcrDiff>timeDiff+30 then
                edcb.Sleep(1000/fastRate)
              end
            end
            break
          end
        end
        if not ts then
          mg.cry('throttling failed')
          break
        end
      end
      if fname:find('%.m2t$') then
        -- 現在のファイルの長さを1000パケット毎にNullパケットで伝える
        now=os.time()
        stat=stat or {now=now-4,count=0,fsec=0,fsize=0,cycle=0}
        -- 変化が無さそうなら打ち切る
        if stat.count<3 and math.abs(stat.now-now)>=4 then
          stat.now=now
          stat.count=stat.count+1
          srcf=edcb.io.open(fpath,'rb')
          if srcf then
            fsec,fsize=GetDurationSec(srcf)
            srcf:close()
            if stat.fsec~=fsec or stat.fsize~=fsize then
              stat.fsec=fsec
              stat.fsize=fsize
              stat.count=0
            end
          end
        end
        if stat.cycle+#buf>=188000 then
          buf=buf:sub(1,188000-stat.cycle)..'\x47\x1f\xff\x10STAT'
            ..('%06d%012.0f'):format(math.min(stat.fsec,999999),math.min(stat.fsize,999999999999))
            ..('\0'):rep(188-26)..buf:sub(188000-stat.cycle+1)
          stat.cycle=stat.cycle-188188
        end
        stat.cycle=stat.cycle+#buf
      end
      if #buf~=0 and not mg.write(buf) then
        -- キャンセルされた
        break
      end
    end
  end
  f:close()
end
