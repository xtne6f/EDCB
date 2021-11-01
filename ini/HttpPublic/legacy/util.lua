--情報通知ログの表示を許可するかどうか
SHOW_NOTIFY_LOG=true
--デバッグ出力の表示を許可するかどうか
SHOW_DEBUG_LOG=false

--メニューに「システムスタンバイ」ボタンを表示するかどうか
INDEX_ENABLE_SUSPEND=false
--メニューの「システムスタンバイ」ボタンを「システム休止」にするかどうか
INDEX_SUSPEND_USE_HIBERNATE=false

--各種一覧のいちどに表示する行数
RESERVE_PAGE_COUNT=50
RECINFO_PAGE_COUNT=50
AUTOADDEPG_PAGE_COUNT=50

--リスト番組表の非表示にしたいサービス
HIDE_SERVICES={
  --非表示にしたいサービスを['ONID-TSID-SID']=true,のように指定
  --['1-2345-6789']=true,
}

--番組表の1分あたりの番組高さ
EPG_ONE_MIN_PX=2
--番組表の番組の最低表示高さ
EPG_MINIMUM_PX=12
--番組表のサービスあたりの幅
EPG_SERVICE_PX=150
--番組表の時刻軸を入れる間隔
EPG_TIME_COLUMN=3
--番組表の番組を絞り込みたいときはNOTキーワードの先頭を"#EPG_CUST_1"にした自動EPG予約を作る

--ライブラリに表示するフォルダをドキュメントルートから'/'区切りの相対パスで指定
LIBRARY_LIST={
  'video',
}

--ライブラリなどに表示するメディアファイルの拡張子を指定
--EpgTimerSrv設定の「TSファイルの拡張子」はあらかじめ指定されている
MEDIA_EXTENSION_LIST={
  '.mp4',
  '.webm',
}

--HLS(HTTP Live Streaming)を許可するかどうか。する場合はtsmemseg.exeとnwtvclose.ps1を用意すること
ALLOW_HLS=true
--ネイティブHLS非対応環境でもhls.jsを使ってHLS再生するかどうか
ALWAYS_USE_HLS=true

--トランスコードオプション
--HLSのときはセグメント長約4秒、最大8MBytes(=1秒あたり16Mbits)を想定しているので、オプションもそれに合わせること
--name:表示名
--xcoder:Toolsフォルダからの相対パス。Toolsフォルダになければパスが通っているとみなす
--option:$SRCと$OUTPUTは必須、再生時に適宜置換される
--filter*Fast:倍速再生用、未定義でもよい
XCODE_OPTIONS={
  {
    name='288p/h264/ffmpeg',
    xcoder='ffmpeg.exe',
    option='-f mpegts $DUAL -analyzeduration 1M -i $SRC -map 0:v:0 -vcodec libx264 -flags:v +cgop -profile:v main -level 31 -b:v 896k -maxrate 4M -bufsize 4M -preset veryfast $FILTER -s 512x288 -map 0:a:$AUDIO -acodec aac -ac 2 -b:a 128k $CAPTION $OUTPUT',
    dualMain='-dual_mono_mode main',
    dualSub='-dual_mono_mode sub',
    filter='-g 120 -vf yadif=0:-1:1',
    filterCinema='-g 96 -vf pullup -r 24000/1001',
    filterFast='-g 120 -vf yadif=0:-1:1,setpts=PTS/1.25 -af atempo=1.25 -bsf:s setts=ts=TS/1.25',
    filterCinemaFast='-g 96 -vf pullup,setpts=PTS/1.25 -af atempo=1.25 -bsf:s setts=ts=TS/1.25 -r 24000/1001',
    captionNone='-sn',
    captionHls='-map 0:s? -scodec copy',
    output={'mp4','-f mp4 -movflags frag_keyframe+empty_moov -'},
    outputHls={'m2t','-f mpegts -'},
  },
  {
    name='576p/h264/ffmpeg-nvenc',
    xcoder='ffmpeg.exe',
    option='-f mpegts $DUAL -analyzeduration 1M -i $SRC -map 0:v:0 -vcodec h264_nvenc -profile:v main -level 31 -b:v 1408k -maxrate 8M -bufsize 8M -preset medium $FILTER -s 1024x576 -map 0:a:$AUDIO -acodec aac -ac 2 -b:a 128k $CAPTION $OUTPUT',
    dualMain='-dual_mono_mode main',
    dualSub='-dual_mono_mode sub',
    filter='-g 120 -vf yadif=0:-1:1',
    filterCinema='-g 96 -vf pullup -r 24000/1001',
    filterFast='-g 120 -vf yadif=0:-1:1,setpts=PTS/1.25 -af atempo=1.25 -bsf:s setts=ts=TS/1.25',
    filterCinemaFast='-g 96 -vf pullup,setpts=PTS/1.25 -af atempo=1.25 -bsf:s setts=ts=TS/1.25 -r 24000/1001',
    captionNone='-sn',
    captionHls='-map 0:s? -scodec copy',
    output={'mp4','-f mp4 -movflags frag_keyframe+empty_moov -'},
    outputHls={'m2t','-f mpegts -'},
  },
  {
    name='288p/webm/ffmpeg',
    xcoder='ffmpeg.exe',
    option='-f mpegts $DUAL -analyzeduration 1M -i $SRC -map 0:v:0 -vcodec libvpx -b:v 896k -quality realtime -cpu-used 1 $FILTER -s 512x288 -map 0:a:$AUDIO -acodec libvorbis -ac 2 -b:a 128k $CAPTION $OUTPUT',
    dualMain='-dual_mono_mode main',
    dualSub='-dual_mono_mode sub',
    filter='-vf yadif=0:-1:1',
    filterCinema='-vf pullup -r 24000/1001',
    filterFast='-vf yadif=0:-1:1,setpts=PTS/1.25 -af atempo=1.25',
    filterCinemaFast='-vf pullup,setpts=PTS/1.25 -af atempo=1.25 -r 24000/1001',
    captionNone='-sn',
    output={'webm','-f webm -'},
  },
  {
    --NVEncCの例。フラグメントMP4の出し方が不明なのでHLS専用。倍速再生未対応。"aac_coder=twoloop"なしだと音質がとても悪い
    name='576p/h264/NVEncC',
    xcoder='NVEncC\\NVEncC.exe',
    option='--input-format mpegts --input-analyze 1 -i $SRC --avhw --profile main --level 3.1 --vbr 1408 --max-bitrate 8192 --vbv-bufsize 8192 --preset default $FILTER --output-res 1024x576 --audio-stream $AUDIO?:stereo --audio-codec $AUDIO?aac:aac_coder=twoloop$DUAL --audio-bitrate $AUDIO?128 --audio-disposition $AUDIO?default $CAPTION $OUTPUT',
    audioStartAt=1,
    dualMain='#dual_mono_mode=main',
    dualSub='#dual_mono_mode=sub',
    filter='--gop-len 120 --interlace tff --vpp-deinterlace normal',
    filterCinema='--gop-len 96 --interlace tff --vpp-afs preset=cinema,24fps=true,rff=true',
    captionNone='',
    captionHls='--sub-copy',
    output={'mp4','-f mp4 -o -'},
    outputHls={'m2t','-f mpegts -o -'},
  },
  {
    --QSVEncCの例。フラグメントMP4の出し方が不明なのでHLS専用。倍速再生未対応。"aac_coder=twoloop"なしだと音質がとても悪い
    name='576p/h264/QSVEncC',
    xcoder='QSVEncC\\QSVEncC.exe',
    option='--input-format mpegts --input-analyze 1 -i $SRC --avhw --profile main --level 3.1 --vbr 1408 --max-bitrate 8192 --vbv-bufsize 8192 --quality balanced $FILTER --output-res 1024x576 --audio-stream $AUDIO?:stereo --audio-codec $AUDIO?aac:aac_coder=twoloop$DUAL --audio-bitrate $AUDIO?128 --audio-disposition $AUDIO?default $CAPTION $OUTPUT',
    audioStartAt=1,
    dualMain='#dual_mono_mode=main',
    dualSub='#dual_mono_mode=sub',
    filter='--gop-len 120 --interlace tff --vpp-deinterlace normal',
    filterCinema='--gop-len 96 --interlace tff --vpp-afs preset=cinema,24fps=true',
    captionNone='',
    captionHls='--sub-copy',
    output={'mp4','-f mp4 -o -'},
    outputHls={'m2t','-f mpegts -o -'},
  },
}

--フォーム上の各オプションのデフォルト選択状態を指定する
XCODE_SELECT_OPTION=1
XCODE_CHECK_CINEMA=false
XCODE_CHECK_FAST=false
XCODE_CHECK_CAPTION=false

--トランスコードするかどうか。する場合はtsreadex.exeとトランスコーダー(ffmpeg.exeなど)を用意すること
XCODE=true
--トランスコードするプロセスを1つだけに制限するかどうか(並列処理できる余裕がシステムにない場合など)
XCODE_SINGLE=false
--ログを"log"フォルダに保存するかどうか
XCODE_LOG=false
--出力バッファの量(bytes)。asyncbuf.exeを用意すること。変換負荷や通信のむらを吸収する
XCODE_BUF=0
--転送開始前に変換しておく量(bytes)
XCODE_PREPARE=0

--NetworkTVモードの名前付きパイプをFindFileで見つけられない場合(EpgTimerSrvのWindowsサービス化など？)に対応するか
NWTV_FIND_BY_OPEN=false

--このサイズ以上のときページ圧縮する(nilのとき常に非圧縮)
GZIP_THRESHOLD_BYTE=4096

--処理するPOSTリクエストボディの最大値
POST_MAX_BYTE=1024*1024

----------定数定義ここまで----------

function GetTranscodeQueries(qs)
  return {
    option=GetVarInt(qs,'option',1,#XCODE_OPTIONS),
    offset=GetVarInt(qs,'offset',0,100),
    audio2=GetVarInt(qs,'audio2')==1,
    dual=GetVarInt(qs,'dual',0,2),
    cinema=GetVarInt(qs,'cinema')==1,
    fast=GetVarInt(qs,'fast')==1,
    caption=GetVarInt(qs,'caption')==1,
  }
end

function ConstructTranscodeQueries(xq)
  return (xq.option and '&amp;option='..xq.option or '')
    ..(xq.offset and '&amp;offset='..xq.offset or '')
    ..(xq.audio2 and '&amp;audio2=1' or '')
    ..(xq.dual and '&amp;dual='..xq.dual or '')
    ..(xq.cinema and '&amp;cinema=1' or '')
    ..(xq.fast and '&amp;fast=1' or '')
    ..(xq.caption and '&amp;caption=1' or '')
end

function TranscodeSettingTemplete(xq,fsec)
  local s='<select name="option">'
  for i,v in ipairs(XCODE_OPTIONS) do
    if not ALLOW_HLS or not ALWAYS_USE_HLS or v.outputHls then
      s=s..'<option value="'..i..'"'..((xq.option or XCODE_SELECT_OPTION)==i and ' selected' or '')..'>'..EdcbHtmlEscape(v.name)
    end
  end
  s=s..'</select>\n'
  if fsec then
    s=s..'offset: <select name="offset">'
    local i=0
    while i<=100 do
      s=s..'<option value="'..i..'"'..((xq.offset or 0)==i and ' selected' or '')..'>'
        ..(fsec>0 and ('%dm%02ds|'):format(math.floor(fsec*i/100/60),fsec*i/100%60) or '')..i..'%'
      i=i<5 and i+1 or i+5
    end
    s=s..'</select>\n'
  end
  s=s..'<label><input type="checkbox" name="audio2" value="1"'..(xq.audio2 and ' checked' or '')..'>audio2</label>\n'
    ..'<select name="dual">'
    ..'<option value="0"'..(xq.dual~=1 and xq.dual~=2 and ' selected' or '')..'>*'
    ..'<option value="1"'..(xq.dual==1 and ' selected' or '')..'>dual-main'
    ..'<option value="2"'..(xq.dual==2 and ' selected' or '')..'>dual-sub'
    ..'</select>\n'
    ..'<label><input type="checkbox" name="cinema" value="1"'..((xq.cinema or not xq.option and XCODE_CHECK_CINEMA) and ' checked' or '')..'>cinema</label>\n'
    ..'<label><input type="checkbox" name="fast" value="1"'..((xq.fast or not xq.option and XCODE_CHECK_FAST) and ' checked' or '')..'>fast</label>\n'
  if ALLOW_HLS then
    s=s..'<label><input type="checkbox" name="caption" value="1"'..((xq.caption or not xq.option and XCODE_CHECK_CAPTION) and ' checked' or '')..'>caption</label>\n'
  end
  return s
end

function HlsScriptTemplete(caption)
  --フルスクリーンボタン
  local s=[=[
<script>
var vfull=document.getElementById("vid-full");
var vcont=document.getElementById("vid-cont");
var btn=document.createElement('button');
btn.innerText="full";
btn.onclick=function(){(vfull.requestFullscreen||vfull.webkitRequestFullscreen||vfull.webkitRequestFullScreen).call(vfull);};
var div=document.createElement('div');
div.className="full-control";
div.appendChild(btn);
vcont.appendChild(div);
btn=document.createElement('button');
btn.innerText="exit";
btn.onclick=function(){(document.exitFullscreen||document.webkitExitFullscreen||document.webkitCancelFullScreen).call(document);};
div=document.createElement('div');
div.className="exit-control";
div.appendChild(btn);
vcont.appendChild(div);
</script>
]=]
  local now=os.date('!*t')
  local hls='&hls='..(1+(now.hour*60+now.min)*60+now.sec)
  if ALWAYS_USE_HLS then
    s=s..'<script src="hls.min.js"></script>\n'
      ..(caption and '<script src="aribb24.js"></script>\n' or '')
      ..'<script>\n'
      ..'var vid=document.getElementById("vid");\n'
      ..(caption and 'var cap=new aribb24js.CanvasRenderer({enableAutoInBandMetadataTextTrackDetection:!Hls.isSupported()});\n'
           ..'cap.attachMedia(vid);\n' or '')
      ..'if(Hls.isSupported()){\n'
      ..'  var hls=new Hls();\n'
      ..'  hls.loadSource(document.getElementById("vidsrc").textContent+"'..hls..'");\n'
      ..'  hls.attachMedia(vid);\n'
      ..'  hls.on(Hls.Events.MANIFEST_PARSED,function(){vid.play();});\n'
      ..(caption and '  hls.on(Hls.Events.FRAG_PARSING_METADATA,function(event,data){\n'
           ..'    for(var s of data.samples){cap.pushID3v2Data(s.pts,s.data);}\n'
           ..'  });\n' or '')
      ..'}else if(vid.canPlayType("application/vnd.apple.mpegurl")){\n'
      ..'  vid.src=document.getElementById("vidsrc").textContent+"'..hls..'";\n'
      ..'}\n'
      ..'</script>'
  else
    s=s..(caption and '<script src="aribb24.js"></script>\n' or '')
      ..'<script>\n'
      ..'var vid=document.getElementById("vid");\n'
      ..(caption and 'var cap=new aribb24js.CanvasRenderer({enableAutoInBandMetadataTextTrackDetection:true});\n'
           ..'cap.attachMedia(vid);\n' or '')
      ..'vid.src=document.getElementById("vidsrc").textContent+(vid.canPlayType("application/vnd.apple.mpegurl")?"'..hls..'":"");\n'
      ..'</script>'
  end
  return s;
end

--EPG情報をTextに変換(EpgTimerUtil.cppから移植)
function ConvertProgramText(v)
  local s=''
  if v then
    s=s..(v.startTime and FormatTimeAndDuration(v.startTime, v.durationSecond)..(v.durationSecond and '' or '～未定') or '未定')..'\n'
    for i,w in ipairs(edcb.GetServiceList() or {}) do
      if w.onid==v.onid and w.tsid==v.tsid and w.sid==v.sid then
        s=s..w.service_name
        break
      end
    end
    s=s..'\n'
    if v.shortInfo then
      s=s..v.shortInfo.event_name..'\n\n'..DecorateUri(v.shortInfo.text_char)..'\n\n'
    end
    if v.extInfo then
      s=s..DecorateUri(('\n'..v.extInfo.text_char):gsub('\n%- ([^\n\r]*)','\n<span class="escape-text">- </span><b>%1</b>'):sub(2))..'\n\n'
    end
    if v.contentInfoList then
      s=s..'ジャンル : \n'
      for i,w in ipairs(v.contentInfoList) do
        --0x0E00は番組付属情報、0x0E01はCS拡張用情報
        local nibble=w.content_nibble==0x0E00 and w.user_nibble+0x6000 or
                     w.content_nibble==0x0E01 and w.user_nibble+0x7000 or w.content_nibble
        s=s..edcb.GetGenreName(math.floor(nibble/256)*256+255)..' - '..edcb.GetGenreName(nibble)..'\n'
      end
      s=s..'\n'
    end
    if v.componentInfo then
      s=s..'映像 : '..edcb.GetComponentTypeName(v.componentInfo.stream_content*256+v.componentInfo.component_type)..' '..v.componentInfo.text_char..'\n'
    end
    if v.audioInfoList then
      s=s..'音声 : '
      for i,w in ipairs(v.audioInfoList) do
        s=s..edcb.GetComponentTypeName(w.stream_content*256+w.component_type)..' '..w.text_char..'\nサンプリングレート : '
          ..(({[1]='16',[2]='22.05',[3]='24',[5]='32',[6]='44.1',[7]='48'})[w.sampling_rate] or '?')..'kHz\n'
      end
      s=s..'\n'
    end
    s=s..'\n'..(NetworkType(v.onid)=='地デジ' and '' or v.freeCAFlag and '有料放送\n' or '無料放送\n')
      ..('OriginalNetworkID:%d(0x%04X)\n'):format(v.onid,v.onid)
      ..('TransportStreamID:%d(0x%04X)\n'):format(v.tsid,v.tsid)
      ..('ServiceID:%d(0x%04X)\n'):format(v.sid,v.sid)
      ..('EventID:%d(0x%04X)\n'):format(v.eid,v.eid)
  end
  return s
end

--録画設定フォームのテンプレート
function RecSettingTemplate(rs)
  local s='<label><input type="checkbox" name="recEnabled" value="1"'..(rs.recMode~=5 and ' checked' or '')..'>有効</label><br>\n'
    ..'録画モード: <select name="recMode">'
  for i=1,#RecModeTextList() do
    s=s..'<option value="'..(i-1)..'"'..((rs.recMode~=5 and rs.recMode or rs.noRecMode or 1)==i-1 and ' selected' or '')..'>'..RecModeTextList()[i]
  end
  s=s..'</select><br>\n'
    ..'<label><input type="checkbox" name="tuijyuuFlag" value="1"'..(rs.tuijyuuFlag and ' checked' or '')..'>イベントリレー追従</label><br>\n'
    ..'優先度: <select name="priority">'
  for i=1,5 do
    s=s..'<option value="'..i..'"'..(rs.priority==i and ' selected' or '')..'>'..i..(i==1 and ' (低)' or i==5 and ' (高)' or '')
  end
  --デフォルト値
  local rsdef=(edcb.GetReserveData(0x7FFFFFFF) or {}).recSetting
  s=s..'</select><br>\n'
    ..'<label><input type="checkbox" name="pittariFlag" value="1"'..(rs.pittariFlag and ' checked' or '')..'>ぴったり（？）録画</label><br>\n'
    ..'録画マージン: <label><input type="checkbox" name="useDefMarginFlag" value="1"'..(rs.startMargin and '' or ' checked')..'>デフォルト</label> || '
    ..'開始（秒） <input type="text" name="startMargin" value="'..(rs.startMargin or rsdef and rsdef.startMargin or 0)..'" size="5"> '
    ..'終了（秒） <input type="text" name="endMargin" value="'..(rs.endMargin or rsdef and rsdef.endMargin or 0)..'" size="5"><br>\n'
    ..'指定サービス対象データ: <label><input type="checkbox" name="serviceMode" value="1"'..(rs.serviceMode%2==0 and ' checked' or '')..'>デフォルト</label> || '
    ..'<label><input type="checkbox" name="serviceMode_1" value="1"'
      ..(math.floor(rs.serviceMode%2~=0 and rs.serviceMode/16 or rsdef and rsdef.serviceMode/16 or 0)%2~=0 and ' checked' or '')..'>字幕を含める</label> '
    ..'<label><input type="checkbox" name="serviceMode_2" value="1"'
      ..(math.floor(rs.serviceMode%2~=0 and rs.serviceMode/32 or rsdef and rsdef.serviceMode/32 or 0)%2~=0 and ' checked' or '')..'>データカルーセルを含める</label><br>\n'
    ..'<table><tr><td>録画フォルダ</td><td>出力PlugIn</td><td>ファイル名PlugIn</td><td>部分受信</td></tr>\n'
  for i,v in ipairs(rs.recFolderList) do
    s=s..'<tr><td>'..v.recFolder..'</td><td>'..v.writePlugIn..'</td><td>'..v.recNamePlugIn..'</td><td>いいえ</td></tr>\n'
  end
  for i,v in ipairs(rs.partialRecFolder) do
    s=s..'<tr><td>'..v.recFolder..'</td><td>'..v.writePlugIn..'</td><td>'..v.recNamePlugIn..'</td><td>はい</td></tr>\n'
  end
  s=s..'</table>（プリセットによる変更のみ対応）<br>\n'
    ..'<label><input type="checkbox" name="partialRecFlag" value="1"'..(rs.partialRecFlag~=0 and ' checked' or '')..'>部分受信（ワンセグ）を別ファイルに同時出力する</label><br>\n'
    ..'<label><input type="checkbox" name="continueRecFlag" value="1"'..(rs.continueRecFlag and ' checked' or '')..'>後ろの予約を同一ファイルで出力する</label><br>\n'
    ..'使用チューナー強制指定: <select name="tunerID"><option value="0"'..(rs.tunerID==0 and ' selected' or '')..'>自動'
  local a=edcb.GetTunerReserveAll()
  for i=1,#a-1 do
    s=s..'<option value="'..a[i].tunerID..'"'..(a[i].tunerID==rs.tunerID and ' selected' or '')..string.format('>ID:%08X(', a[i].tunerID)..a[i].tunerName..')'
  end
  s=s..'</select><br>\n'
    ..'録画後動作: <select name="suspendMode">'
    ..'<option value="0"'..(rs.suspendMode==0 and ' selected' or '')..'>'..(rsdef and ({'スタンバイ','休止','シャットダウン','何もしない'})[rsdef.suspendMode] or '')..'（デフォルト）'
    ..'<option value="1"'..(rs.suspendMode==1 and ' selected' or '')..'>スタンバイ'
    ..'<option value="2"'..(rs.suspendMode==2 and ' selected' or '')..'>休止'
    ..'<option value="3"'..(rs.suspendMode==3 and ' selected' or '')..'>シャットダウン'
    ..'<option value="4"'..(rs.suspendMode==4 and ' selected' or '')..'>何もしない</select> '
    ..'<label><input type="checkbox" name="rebootFlag" value="1"'
      ..((rs.suspendMode==0 and rsdef and rsdef.rebootFlag or rs.suspendMode~=0 and rs.rebootFlag) and ' checked' or '')..'>復帰後再起動する</label><br>\n'
    ..'録画後実行bat（プリセットによる変更のみ対応）: '..(#rs.batFilePath==0 and '（なし）' or rs.batFilePath)..'<br>\n'
  return s
end

function RecModeTextList()
  return {'全サービス','指定サービス','全サービス（デコード処理なし）','指定サービス（デコード処理なし）','視聴'}
end

function NetworkType(onid)
  return not onid and {'地デジ','BS','CS1','CS2','CS3','その他'}
    or NetworkType()[0x7880<=onid and onid<=0x7FE8 and 1 or onid==4 and 2 or onid==6 and 3 or onid==7 and 4 or onid==10 and 5 or 6]
end

--表示するサービスを選択する
function SelectChDataList(a)
  local r={}
  for i,v in ipairs(a) do
    --EPG取得対象サービスのみ
    if v.epgCapFlag then
      r[#r+1]=v
    end
  end
  return r
end

--サービスをソートする
function SortServiceListInplace(r)
  local bsmin={}
  for i,v in ipairs(r) do
    if NetworkType(v.onid)=='BS' and (bsmin[v.tsid] or 65536)>v.sid then
      bsmin[v.tsid]=v.sid
    end
  end
  table.sort(r,function(a,b) return
    ('%04X%04X%04X%04X'):format((NetworkType(a.onid)~='地デジ' and 65535 or a.remote_control_key_id or 0),
                                a.onid,(NetworkType(a.onid)=='BS' and bsmin[a.tsid] or a.tsid),a.sid)<
    ('%04X%04X%04X%04X'):format((NetworkType(b.onid)~='地デジ' and 65535 or b.remote_control_key_id or 0),
                                b.onid,(NetworkType(b.onid)=='BS' and bsmin[b.tsid] or b.tsid),b.sid) end)
  return r
end

--URIをタグ装飾する
function DecorateUri(s)
  local hwhost='-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
  local hw='!#$%&()*+/:;=?@_~~'..hwhost
  local fwhost='－．０１２３４５６７８９ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ'
  local fw='！＃＄％＆（）＊＋／：；＝？＠＿～￣'..fwhost
  --sを半角置換
  local r,i={},1
  while i<=#s do
    local j=fw:find(s:sub(i,i+2),1,true)
    if i+2<=#s and j and j%3==1 then
      r[#r+1]=hw:sub((j+2)/3,(j+2)/3)
      i=i+2
    else
      r[#r+1]=s:sub(i,i)
    end
    i=i+1
  end
  r=table.concat(r)

  --置換後nにある文字がsのどこにあるか
  local spos=function(n)
    local i=1
    while i<=#s and n>1 do
      n=n-1
      local j=fw:find(s:sub(i,i+2),1,true)
      if i+2<=#s and j and j%3==1 then
        i=i+2
      end
      i=i+1
    end
    return i
  end

  local t,n,i='',1,1
  while i<=#r do
    --特定のTLDっぽい文字列があればホスト部分をさかのぼる
    local h=0
    if r:find('^%.com/',i) or r:find('^%.jp/',i) or r:find('^%.tv/',i) then
      while i-h>1 and hwhost:find(r:sub(i-h-1,i-h-1),1,true) do
        h=h+1
      end
    end
    if (h>0 and (i-h==1 or r:find('^[^/]',i-h-1))) or r:find('^https?://',i) then
      local j=i
      while j<=#r and hw:find(r:sub(j,j),1,true) do
        j=j+1
      end
      t=t..s:sub(spos(n),spos(i-h)-1)..'<a href="'..(h>0 and 'https://' or '')
        ..r:sub(i-h,j-1):gsub('&amp;','&'):gsub('&','&amp;')..'">'..s:sub(spos(i-h),spos(j)-1)..'</a>'
      n=j
      i=j-1
    end
    i=i+1
  end
  t=t..s:sub(spos(n))
  return t
end

--時間の文字列を取得する
function FormatTimeAndDuration(t,dur)
  dur=dur and (t.hour*3600+t.min*60+t.sec+dur)
  return string.format('%d/%02d/%02d(%s) %02d:%02d',t.year,t.month,t.day,({'日','月','火','水','木','金','土',})[t.wday],t.hour,t.min)
    ..(t.sec~=0 and string.format('<small>:%02d</small>',t.sec) or '')
    ..(dur and string.format('～%02d:%02d',math.floor(dur/3600)%24,math.floor(dur/60)%60)..(dur%60~=0 and string.format('<small>:%02d</small>',dur%60) or '') or '')
end

--ドキュメントルートへの相対パスを取得する
function PathToRoot()
  return ('../'):rep(#mg.script_name:gsub('[^\\/]*[\\/]+[^\\/]*','N')-#(mg.document_root..'/'):gsub('[^\\/]*[\\/]+','N'))
end

--OSの絶対パスをドキュメントルートからの相対パスに変換する
function NativeToDocumentPath(path)
  local root=(mg.document_root..'/'):gsub('[\\/]+','/')
  if path:gsub('[\\/]+','/'):sub(1,#root):lower()==root:lower() then
    return path:gsub('[\\/]+','/'):sub(#root+1)
  end
  return nil
end

--ドキュメントルートからの相対パスをOSの絶対パスに変換する
function DocumentToNativePath(path)
  --冗長表現の可能性を潰す
  local esc=edcb.htmlEscape
  edcb.htmlEscape=0
  path=edcb.Convert('utf-8','utf-8',path):gsub('/+','/')
  edcb.htmlEscape=esc
  --禁止文字と正規化のチェック
  if not path:find('[\0-\x1f\x7f\\:*?"<>|]') and not path:find('%./') and not path:find('%.$') then
    return mg.document_root..'\\'..path:gsub('/','\\')
  end
  return nil
end

--現在の変換モードでHTMLエスケープする
function EdcbHtmlEscape(s)
  return edcb.Convert('utf-8','utf-8',s)
end

--PCRまで読む
function ReadToPcr(f,pid)
  for i=1,10000 do
    local buf=f:read(188)
    if buf and #buf==188 and buf:byte(1)==0x47 then
      --adaptation_field_control and adaptation_field_length and PCR_flag
      if math.floor(buf:byte(4)/16)%4>=2 and buf:byte(5)>=5 and math.floor(buf:byte(6)/16)%2~=0 then
        local pcr=((buf:byte(7)*256+buf:byte(8))*256+buf:byte(9))*256+buf:byte(10)
        local pid2=buf:byte(2)%32*256+buf:byte(3)
        if not pid or pid==pid2 then
          return pcr,pid2
        end
      end
    end
  end
  return nil
end

--PCRをもとにファイルの長さを概算する(少なめに報告するかもしれない)
function GetDurationSec(f)
  local fsize=f:seek('end') or 0
  if fsize>1880000 and f:seek('set') then
    local pcr,pid=ReadToPcr(f)
    if pcr and f:seek('set',(math.floor(fsize/188)-10000)*188) then
      local pcr2=ReadToPcr(f,pid)
      if pcr2 then
        return math.floor((pcr2+0x100000000-pcr)%0x100000000/45000),fsize
      end
      --TSデータが存在する境目を見つける
      local predicted,range=math.floor(fsize/2/188)*188,fsize
      while range>1880000 and f:seek('set',predicted) do
        local buf=f:read(189)
        local valid=buf and #buf==189 and buf:byte(1)==0x47 and buf:byte(189)==0x47
        predicted=math.floor((predicted+(valid and range/4 or -range/4))/188)*188
        range=range/2
      end
      predicted=predicted-1880000
      if predicted>0 and f:seek('set',predicted) then
        pcr2=ReadToPcr(f,pid)
        if pcr2 then
          return math.floor((pcr2+0x100000000-pcr)%0x100000000/45000),predicted
        end
      end
    end
  end
  return 0,fsize
end

--ファイルの先頭からsec秒だけシークする
function SeekSec(f,sec,dur,fsize)
  if dur>0 and fsize>1880000 and f:seek('set') then
    local pcr,pid=ReadToPcr(f)
    if pcr then
      local pos,diff=0,sec*45000
      --5ループまたは誤差が2秒未満になるまで動画レートから概算シーク
      for i=1,5 do
        if math.abs(diff)<90000 then break end
        pos=math.floor(math.min(math.max(pos+fsize/dur*diff/45000,0),fsize-1880000)/188)*188
        if not f:seek('set',pos) then return false end
        local pcr2=ReadToPcr(f,pid)
        if not pcr2 then return false end
        --移動分を差し引く
        diff=diff+((pcr2+0x100000000-pcr)%0x100000000<0x80000000 and -((pcr2+0x100000000-pcr)%0x100000000) or (pcr+0x100000000-pcr2)%0x100000000)
        pcr=pcr2
      end
      return true
    end
  end
  return false
end

--HTTP日付の文字列を取得する
function ImfFixdate(t)
  return ('%s, %02d %s %d %02d:%02d:%02d GMT'):format(({'Sun','Mon','Tue','Wed','Thu','Fri','Sat'})[t.wday],t.day,
    ({'Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'})[t.month],t.year,t.hour,t.min,t.sec)
end

--レスポンスを生成する
function Response(code,ctype,charset,cl)
  return 'HTTP/1.1 '..code..' '..mg.get_response_code_text(code)
    ..'\r\nDate: '..ImfFixdate(os.date('!*t'))
    ..'\r\nX-Frame-Options: SAMEORIGIN'
    ..(ctype and '\r\nX-Content-Type-Options: nosniff\r\nContent-Type: '..ctype..(charset and '; charset='..charset or '') or '')
    ..(cl and mg.request_info.request_method~='HEAD' and '\r\nContent-Length: '..cl or '')
    ..(mg.keep_alive(not not cl) and '\r\n' or '\r\nConnection: close\r\n')
end

--コンテンツ(レスポンスボディ)を連結するオブジェクトを生成する
--※HEADリクエストでは何も追加されない
--※threshを省略すると圧縮は行われない
function CreateContentBuilder(thresh)
  local self={ct={''},len=0,thresh_=thresh}
  function self:Append(s)
    if mg.request_info.request_method=='HEAD' then
      return
    end
    if self.thresh_ and self.len+#s>=self.thresh_ and not self.stream_ then
      self.stream_=true
      --可能ならコンテンツをgzip圧縮する(lua-zlib(zlib.dll)が必要)
      for k,v in pairs(mg.request_info.http_headers) do
        if k:lower()=='accept-encoding' and v:lower():find('gzip') then
          local status,zlib=pcall(require,'zlib')
          if status then
            self.stream_=zlib.deflate(6,31)
            self.ct={'',(self.stream_(table.concat(self.ct)))}
            self.len=#self.ct[2]
            self.gzip=true
          end
          break
        end
      end
    end
    s=self.gzip and self.stream_(s) or s
    if #s>0 then
      self.ct[#self.ct+1]=s
      self.len=self.len+#s
    end
  end
  --コンテンツの連結を完了してlenを確定させる
  function self:Finish()
    if self.gzip and self.stream_ then
      self.ct[#self.ct+1]=self.stream_()
      self.len=self.len+#self.ct[#self.ct]
    end
    self.stream_=nil
  end
  --必要ならヘッダをつけて全体を取り出す
  function self:Pop(s)
    self:Finish()
    self.ct[1]=s or ''
    s=table.concat(self.ct)
    self.ct={''}
    self.len=0
    self.gzip=nil
    return s
  end
  return self
end

--POSTメッセージボディをすべて読む
function AssertPost()
  local post, s
  if mg.request_info.request_method=='POST' then
    post=''
    repeat
      s=mg.read()
      post=post..(s or '')
      assert(#post<POST_MAX_BYTE)
    until not s
    if #post~=mg.request_info.content_length then
      post=''
    end
    AssertCsrf(post)
  end
  return post
end

--クエリパラメータを整数チェックして取得する
function GetVarInt(qs,n,ge,le,occ)
  n=tonumber(mg.get_var(qs,n,occ))
  if n and n==math.floor(n) and n>=(ge or -2147483648) and n<=(le or 2147483647) then
    return n
  end
  return nil
end

--クエリパラメータからサービスのIDを取得する
function GetVarServiceID(qs,n,occ,leextra)
  local onid,tsid,sid,x=(mg.get_var(qs,n,occ) or ''):match('^([0-9]+)%-([0-9]+)%-([0-9]+)'..(leextra and '%-([0-9]+)' or '')..'$')
  onid=tonumber(onid)
  tsid=tonumber(tsid)
  sid=tonumber(sid)
  x=tonumber(x)
  if onid and onid==math.floor(onid) and onid>=0 and onid<=65535 and
     tsid and tsid==math.floor(tsid) and tsid>=0 and tsid<=65535 and
     sid and sid==math.floor(sid) and sid>=0 and sid<=65535 then
    if not leextra then
      return onid,tsid,sid,0
    elseif x and x==math.floor(x) and x>=0 and x<=leextra then
      return onid,tsid,sid,x
    end
  end
  --失敗
  return 0,0,0,0
end

--クエリパラメータから番組のIDを取得する
function GetVarEventID(qs,n,occ)
  return GetVarServiceID(qs,n,occ,65535)
end

--クエリパラメータから過去番組のIDを取得する
function GetVarPastEventID(qs,n,occ)
  return GetVarServiceID(qs,n,occ,4294967295)
end

--CSRFトークンを取得する
--※このトークンを含んだコンテンツを圧縮する場合はBREACH攻撃に少し気を配る
function CsrfToken(m,t)
  --メッセージに時刻をつける
  m=(m or mg.script_name:match('[^\\/]*$'):lower())..'/legacy/'..(math.floor(os.time()/3600/12)+(t or 0))
  local kip,kop=('\54'):rep(48),('\92'):rep(48)
  for k in edcb.serverRandom:sub(1,32):gmatch('..') do
    kip=string.char(bit32.bxor(tonumber(k,16),54))..kip
    kop=string.char(bit32.bxor(tonumber(k,16),92))..kop
  end
  --HMAC-MD5(hex)
  return mg.md5(kop..mg.md5(kip..m))
end

--CSRFトークンを検査する
--※サーバに変更を加える要求(POSTに限らない)を処理する前にこれを呼ぶべき
function AssertCsrf(qs)
  assert(mg.get_var(qs,'ctok')==CsrfToken() or mg.get_var(qs,'ctok')==CsrfToken(nil,-1))
end
