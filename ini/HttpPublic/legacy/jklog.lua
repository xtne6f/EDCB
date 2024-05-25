-- メディアファイルのメタデータをもとに実況のログを取得する
dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

-- TOT時刻の範囲とネットワークIDとサービスIDを取得する
function ExtractTotListAndServiceIDFromPsiData(f)
  local totList,nid,sid={},nil,nil
  local lastPids={}
  local lastDict={}
  local initTime=-1
  while true do
    local buf=f:read(32)
    if not buf or #buf~=32 or buf:sub(1,8)~='Pssc\x0d\x0a\x9a\x0a' then break end
    local timeListLen=GetLeNumber(buf,11,2)
    local dictionaryLen=GetLeNumber(buf,13,2)
    local dictionaryWindowLen=GetLeNumber(buf,15,2)
    local dictionaryDataSize=GetLeNumber(buf,17,4)
    local dictionaryBuffSize=GetLeNumber(buf,21,4)
    local codeListLen=GetLeNumber(buf,25,4)
    if dictionaryWindowLen<dictionaryLen or
       dictionaryBuffSize<dictionaryDataSize or
       dictionaryWindowLen>65536-4096 then
      return nil
    end
    local timeBuf=f:read(timeListLen*4+dictionaryLen*2)
    if not timeBuf or #timeBuf~=timeListLen*4+dictionaryLen*2 then return nil end

    local pos=timeListLen*4+1
    local remain=dictionaryDataSize
    local pids={}
    local dict={}
    for i=1,dictionaryLen do
      local codeOrSize=GetLeNumber(timeBuf,pos,2)-4096
      if codeOrSize>=0 then
        if codeOrSize>=#lastPids or lastPids[codeOrSize+1]<0 then return nil end
        pids[i]=lastPids[codeOrSize+1]
        dict[i]=lastDict[codeOrSize+1]
        lastPids[codeOrSize+1]=-1
      else
        remain=remain-2
        buf=f:read(2)
        if not buf or #buf~=2 or remain<0 then return nil end
        pids[i]=codeOrSize
        dict[i]=GetLeNumber(buf,1,2)%0x2000
      end
      pos=pos+2
    end

    for i=1,dictionaryLen do
      if pids[i]<0 then
        local size=pids[i]+4097
        remain=remain-size
        buf=f:read(size)
        if not buf or #buf~=size or remain<0 then return nil end
        pids[i]=dict[i]
        -- PATとNITとTDT,TOT以外いらない
        if pids[i]==0 or pids[i]==16 or pids[i]==20 then
          dict[i]=buf
        end
      end
    end
    for i=1,dictionaryWindowLen-dictionaryLen do
      if i>#lastPids then return nil end
      if lastPids[i]>=0 then
        pids[#pids+1]=lastPids[i]
        dict[#dict+1]=lastDict[i]
      end
    end
    lastPids=pids
    lastDict=dict
    remain=remain+dictionaryDataSize%2
    -- 古いMinGW(おそらく2018年以前)の_fseeki64,_ftelli64(特にSEEK_CUR)にバグがあるのでシークは使わない
    while remain>0 do
      local size=math.min(remain,1024)
      buf=f:read(size)
      if not buf or #buf~=size then return nil end
      remain=remain-size
    end

    local currTime=-1
    for timeListPos=1,timeListLen*4,4 do
      local absTime=GetLeNumber(timeBuf,timeListPos,4)
      if absTime==0xffffffff then
        currTime=-1
      elseif absTime>=0x80000000 then
        currTime=absTime%0x40000000
        if initTime<0 then
          initTime=currTime
        end
      else
        if currTime>=0 then
          currTime=currTime+GetLeNumber(timeBuf,timeListPos,2)
        end
        local n=GetLeNumber(timeBuf,timeListPos+2,2)+1
        buf=f:read(n*2)
        if not buf or #buf~=n*2 then return nil end
        for i=1,n do
          local code=GetLeNumber(buf,(i-1)*2+1,2)-4096
          if code<0 or code>=#pids then return nil end
          local pid=pids[code+1]
          local psi=dict[code+1]
          if not sid and pid==0 and #psi>=16 then
            -- PAT
            sid=psi:byte(9)*256+psi:byte(10)
            if #psi>=20 and sid==0 then
              sid=psi:byte(13)*256+psi:byte(14)
            end
            if sid==0 then
              sid=nil
            end
          elseif not nid and pid==16 and #psi>=5 then
            -- NIT
            nid=psi:byte(4)*256+psi:byte(5)
          elseif pid==20 and #psi>=8 and currTime>=0 then
            -- TDT,TOT
            local sec=(currTime+0x40000000-initTime)%0x40000000/11250
            local mjd=psi:byte(4)*256+psi:byte(5)
            local h=psi:byte(6)
            local m=psi:byte(7)
            local s=psi:byte(8)
            local tot=((mjd*24+math.floor(h/16)*10+h%16)*60+math.floor(m/16)*10+m%16)*60+math.floor(s/16)*10+s%16-3506749200
            local back=#totList>0 and totList[#totList]
            if (back and (sec<back.secEnd or tot<back.totEnd)) or (not back and sec>60) then
              -- 時刻の巻き戻り、または間隔が空きすぎている
              return nil
            end
            if not back or math.abs((sec-back.sec)-(tot-back.tot))>5 then
              -- PCRとTOTの増分量に差があるので分割する
              totList[#totList+1]={sec=sec,secEnd=sec,tot=tot,totEnd=tot}
            else
              back.secEnd=sec
              back.totEnd=tot
            end
          end
        end
      end
    end
    local trailerSize=4-(dictionaryLen*2+math.ceil(dictionaryDataSize/2)*2+codeListLen*2)%4
    buf=f:read(trailerSize)
    if not buf or #buf~=trailerSize then return nil end
  end
  return totList,nid,sid
end

code=500
if JKRDLOG_PATH then
  code=404
  fpath=mg.get_var(mg.request_info.query_string,'fname')
  if fpath then
    fpath=DocumentToNativePath(fpath)
    if fpath then
      f=edcb.io.open(fpath:gsub('%.[0-9A-Za-z]+$','')..'.psc','rb')
      if f then
        code=500
        totList,nid,sid=ExtractTotListAndServiceIDFromPsiData(f)
        if totList and nid and sid then
          code=404
          id=GetJikkyoID(nid,sid)
          if id then
            code=200
          end
        end
        f:close()
      end
    end
  end
end

if code==200 then
  ct=CreateContentBuilder(GZIP_THRESHOLD_BYTE)
  currSec=1
  for i,v in ipairs(totList) do
    if v.tot<v.totEnd then
      cmd=QuoteCommandArgForPath(JKRDLOG_PATH)..' '..id..' '..v.tot..' '..v.totEnd
      f=edcb.io.popen(WIN32 and '"'..cmd..'"' or cmd)
      if f then
        while true do
          buf=ReadJikkyoChunk(f)
          if not buf then break end
          if currSec<v.sec then
            -- ヘッダを余分に挿入してチャンクの数と秒数を一致させる
            extraHead=buf:match('^<!%-%- J=[0-9]+')
            if extraHead then
              extraHead=extraHead..';T='..v.tot..';L=0;N=0'
              extraHead=extraHead..(' '):rep(76-#extraHead)..'-->\n'
              while currSec<v.sec do
                ct:Append(extraHead)
                currSec=currSec+1
              end
            end
          end
          ct:Append(buf)
          currSec=currSec+1
        end
        f:close()
      end
    end
  end
  ct:Finish()
  mg.write(ct:Pop(Response(200,mg.get_mime_type('a.txt'),'utf-8',ct.len,ct.gzip)..'\r\n'))
else
  mg.write(Response(code,nil,nil,0)..'\r\n')
end
