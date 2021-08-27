-- ファイルを転送するスクリプト
-- ファイルをタイムシフト再生できる: http://localhost:5510/xcode.lua?fname=video/foo.ts

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

fpath=mg.get_var(mg.request_info.query_string,'fname')
if fpath then
  fpath=DocumentToNativePath(fpath)
  option=XCODE_OPTIONS[GetVarInt(mg.request_info.query_string,'option',1,#XCODE_OPTIONS) or 1]
  offset=GetVarInt(mg.request_info.query_string,'offset',0,100) or 0
  audio2=(GetVarInt(mg.request_info.query_string,'audio2',0,1) or 0)+(option.audioStartAt or 0)
  dual=GetVarInt(mg.request_info.query_string,'dual',0,2)
  dual=dual==1 and option.dualMain or dual==2 and option.dualSub or ''
  filter=GetVarInt(mg.request_info.query_string,'cinema')==1 and option.filterCinema or option.filter or ''
  hls=GetVarInt(mg.request_info.query_string,'hls',1)
  caption=hls and GetVarInt(mg.request_info.query_string,'caption')==1 and option.captionHls or option.captionNone or ''
  output=hls and option.outputHls or option.output
  if hls and not (ALLOW_HLS and option.outputHls) then
    -- エラーを返す
    fpath=nil
  end
end

function OpenTranscoder()
  -- コマンドはEDCBのToolsフォルダにあるものを優先する
  local tools=edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')..'\\Tools'
  local readex=(edcb.FindFile(tools..'\\readex.exe',1) and tools..'\\' or '')..'readex.exe'
  local asyncbuf=(edcb.FindFile(tools..'\\asyncbuf.exe',1) and tools..'\\' or '')..'asyncbuf.exe'
  local tsmemseg=(edcb.FindFile(tools..'\\tsmemseg.exe',1) and tools..'\\' or '')..'tsmemseg.exe'
  local xcoder=(edcb.FindFile(tools..'\\'..option.xcoder,1) and tools..'\\' or '')..option.xcoder

  local cmd='"'..xcoder..'" '..option.option
    :gsub('$SRC','-')
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
  -- 容量確保の可能性があるときは周期188+同期語0x47(188*256+0x47=48199)で対象ファイルを終端判定する
  local sync=edcb.GetPrivateProfile('SET','KeepDisk',0,'EpgTimerSrv.ini')~='0'
  cmd='"'..readex..'" '..offset..(sync and ' 4p48199' or ' 4')..' "'..fpath:gsub('[&%^]','^%0')..'" | '..cmd
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

f=nil
if fpath and hls then
  -- クエリのハッシュをキーとし、同一キーアクセスは出力中のインデックスファイルを返す
  segmentKey=mg.md5('xcode:'..hls..':'..fpath..':'..option.xcoder..':'..option.option..':'..offset..':'..audio2..':'..dual..':'..filter..':'..caption..':'..output[2])
  f=edcb.io.open('\\\\.\\pipe\\tsmemseg_'..segmentKey..'_00','rb')
end

if fpath and not f then
  fname='xcode'..(fpath:match('%.[0-9A-Za-z]+$') or '')
  fnamets='xcode'..edcb.GetPrivateProfile('SET','TSExt','.ts','EpgTimerSrv.ini'):lower()
  -- 拡張子を限定
  if fname:lower()==fnamets then
    f=edcb.io.open(fpath, 'rb')
    if f then
      fsec,fsize=GetDurationSec(f)
      if offset~=0 and offset~=100 and fsec>0 and SeekSec(f,fsec*offset/100) then
        offset=f:seek('cur',0) or 0
      else
        offset=math.floor(fsize*offset/100/188)*188
      end
      if XCODE then
        f:close()
        f=OpenTranscoder()
        fname='xcode.'..output[1]
      elseif hls then
        -- トランスコードなしのライブストリーミングには未対応
        f:close()
        f=nil
      else
        -- 容量確保には未対応
        f:seek('set', offset)
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
