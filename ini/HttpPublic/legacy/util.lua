--情報通知ログの表示を許可するかどうか
SHOW_NOTIFY_LOG=true
--デバッグ出力の表示を許可するかどうか
SHOW_DEBUG_LOG=false

--設定メニューからの設定の変更を許可するかどうか
ALLOW_SETTING=false

--※true/falseの設定は例えばリモートアドレスと比較して接続元の限定も可能
--ALLOW_SETTING=mg.request_info.remote_addr=='127.0.0.1' or mg.request_info.remote_addr=='::1'

--メニューに「システムスタンバイ」ボタンを表示するかどうか(Windows専用)
INDEX_ENABLE_SUSPEND=false
--メニューの「システムスタンバイ」ボタンを「システム休止」にするかどうか
INDEX_SUSPEND_USE_HIBERNATE=false

--「プロセス管理」に表示するプロセス名のリスト(非Windows専用)
PROCESS_MANAGEMENT_LIST={
  'EpgDataCap_Bon',
  'ffmpeg',
  'nvencc',
  'qsvencc',
  'vceencc',
}

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
--番組表の番組を絞り込みたいときはメモ欄かNOTキーワードの先頭を"#EPG_CUST_1"にした自動EPG予約を作る

--ライブラリに表示するフォルダをドキュメントルートから'/'区切りの相対パスで指定
--指定フォルダとその1階層下のフォルダにあるメディアファイルまでが表示対象
LIBRARY_LIST={
  'video',
}

--ライブラリなどに表示するメディアファイルの拡張子を指定
--EpgTimerSrv設定の「TSファイルの拡張子」はあらかじめ指定されている
MEDIA_EXTENSION_LIST={
  '.mp4',
  '.webm',
}

--HLS(HTTP Live Streaming)を許可するかどうか。する場合はtsmemseg.exeを用意すること。IE非対応
ALLOW_HLS=true
--ネイティブHLS非対応環境でもhls.jsを使ってHLS再生するかどうか
ALWAYS_USE_HLS=true
--HLS再生時にトランスコーダーから受け取ったMPEG2-TSをMP4に変換するかどうか。有効時はHEVCトランスコードに対応
--※Android版Firefoxでは不具合があるため無効扱いになる
USE_MP4_HLS=true
--視聴機能(viewボタン)でLowLatencyHLSにするかどうか。再生遅延が小さくなる。ネイティブHLS環境ではHTTP/2が要求されるためhls.js使用時のみ有用
USE_MP4_LLHLS=true

--倍速再生(fastボタン)の速度
XCODE_FAST=1.25

--トランスコードオプション
--HLSのときはセグメント長約4秒、最大8MBytes(=1秒あたり16Mbits)を想定しているので、オプションもそれに合わせること
--HLSでないときはフラグメントMP4などを使ったプログレッシブダウンロード。字幕は適当な重畳手法がまだないので未対応
--name:表示名
--xcoder:トランスコーダーのToolsフォルダからの相対パス。'|'で複数候補を指定可。見つからなければ最終候補にパスが通っているとみなす
--       Windows以外では".exe"が除去されて最終候補のみ参照される
--option:$OUTPUTは必須、再生時に適宜置換される。標準入力からMPEG2-TSを受け取るようにオプションを指定する
--filter*Fast:倍速再生用、未定義でもよい
--editorFast:単独で倍速再生にできないトランスコーダーの手前に置く編集コマンド。指定方法はxcoderと同様
--editorOptionFast:標準入出力ともにMPEG2-TSで倍速再生になるようにオプションを指定する
XCODE_OPTIONS={
  {
    --ffmpegの例。-b:vでおおよその最大ビットレートを決め、-qminで動きの少ないシーンのデータ量を節約する
    name='360p/h264/ffmpeg',
    xcoder='ffmpeg\\ffmpeg.exe|ffmpeg.exe',
    option='-f mpegts -analyzeduration 1M -i - -map 0:v?:0 -vcodec libx264 -flags:v +cgop -profile:v main -level 31 -b:v 1888k -qmin 23 -maxrate 4M -bufsize 4M -preset veryfast $FILTER -s 640x360 -map 0:a:$AUDIO -acodec aac -ac 2 -b:a 160k $CAPTION -max_interleave_delta 500k $OUTPUT',
    filter='-g 120 -vf yadif=0:-1:1',
    filterCinema='-g 96 -vf pullup -r 24000/1001',
    filterFast='-g 120 -vf yadif=0:-1:1,setpts=PTS/'..XCODE_FAST..' -af atempo='..XCODE_FAST..' -bsf:s setts=ts=TS/'..XCODE_FAST,
    filterCinemaFast='-g 96 -vf pullup,setpts=PTS/'..XCODE_FAST..' -af atempo='..XCODE_FAST..' -bsf:s setts=ts=TS/'..XCODE_FAST..' -r 24000/1001',
    captionNone='-sn',
    captionHls='-map 0:s? -scodec copy',
    output={'mp4','-f mp4 -movflags frag_keyframe+empty_moov -'},
    outputHls={'m2t','-f mpegts -'},
  },
  {
    name='720p/h264/ffmpeg-nvenc',
    xcoder='ffmpeg\\ffmpeg.exe|ffmpeg.exe',
    option='-f mpegts -analyzeduration 1M -i - -map 0:v?:0 -vcodec h264_nvenc -profile:v main -level 41 -b:v 3936k -qmin 23 -maxrate 8M -bufsize 8M -preset medium $FILTER -s 1280x720 -map 0:a:$AUDIO -acodec aac -ac 2 -b:a 160k $CAPTION -max_interleave_delta 500k $OUTPUT',
    filter='-g 120 -vf yadif=0:-1:1',
    filterCinema='-g 96 -vf pullup -r 24000/1001',
    filterFast='-g 120 -vf yadif=0:-1:1,setpts=PTS/'..XCODE_FAST..' -af atempo='..XCODE_FAST..' -bsf:s setts=ts=TS/'..XCODE_FAST,
    filterCinemaFast='-g 96 -vf pullup,setpts=PTS/'..XCODE_FAST..' -af atempo='..XCODE_FAST..' -bsf:s setts=ts=TS/'..XCODE_FAST..' -r 24000/1001',
    captionNone='-sn',
    captionHls='-map 0:s? -scodec copy',
    output={'mp4','-f mp4 -movflags frag_keyframe+empty_moov -'},
    outputHls={'m2t','-f mpegts -'},
  },
  {
    --ffmpegのh264_qsvは環境によって異常にビットレートが高くなったりしてあまり質が良くない。要注意
    name='720p/h264/ffmpeg-qsv',
    xcoder='ffmpeg\\ffmpeg.exe|ffmpeg.exe',
    option='-f mpegts -analyzeduration 1M -i - -map 0:v?:0 -vcodec h264_qsv -profile:v main -level 41 -b:v 3936k -min_qp_i 23 -min_qp_p 26 -min_qp_b 30 -maxrate 8M -bufsize 8M -preset medium $FILTER -s 1280x720 -map 0:a:$AUDIO -acodec aac -ac 2 -b:a 160k $CAPTION -max_interleave_delta 500k $OUTPUT',
    filter='-g 120 -vf yadif=0:-1:1',
    filterCinema='-g 96 -vf pullup -r 24000/1001',
    filterFast='-g 120 -vf yadif=0:-1:1,setpts=PTS/'..XCODE_FAST..' -af atempo='..XCODE_FAST..' -bsf:s setts=ts=TS/'..XCODE_FAST,
    filterCinemaFast='-g 96 -vf pullup,setpts=PTS/'..XCODE_FAST..' -af atempo='..XCODE_FAST..' -bsf:s setts=ts=TS/'..XCODE_FAST..' -r 24000/1001',
    captionNone='-sn',
    captionHls='-map 0:s? -scodec copy',
    output={'mp4','-f mp4 -movflags frag_keyframe+empty_moov -'},
    outputHls={'m2t','-f mpegts -'},
  },
  {
    name='360p/webm/ffmpeg',
    xcoder='ffmpeg\\ffmpeg.exe|ffmpeg.exe',
    option='-f mpegts -analyzeduration 1M -i - -map 0:v?:0 -vcodec libvpx -b:v 1888k -quality realtime -cpu-used 1 $FILTER -s 640x360 -map 0:a:$AUDIO -acodec libvorbis -ac 2 -b:a 160k $CAPTION -max_interleave_delta 500k $OUTPUT',
    filter='-vf yadif=0:-1:1',
    filterCinema='-vf pullup -r 24000/1001',
    filterFast='-vf yadif=0:-1:1,setpts=PTS/'..XCODE_FAST..' -af atempo='..XCODE_FAST,
    filterCinemaFast='-vf pullup,setpts=PTS/'..XCODE_FAST..' -af atempo='..XCODE_FAST..' -r 24000/1001',
    captionNone='-sn',
    output={'webm','-f webm -'},
  },
  {
    --NVEncCの例。倍速再生にはffmpegも必要
    name='720p/h264/NVEncC',
    xcoder='NVEncC\\NVEncC64.exe|NVEncC\\NVEncC.exe|NVEncC64.exe|nvencc.exe',
    option='--input-format mpegts --input-analyze 1 --input-probesize 4M -i - --avhw --profile main --level 4.1 --vbr 3936 --qp-min 23:26:30 --max-bitrate 8192 --vbv-bufsize 8192 --preset default $FILTER --output-res 1280x720 --audio-stream $AUDIO?:stereo --audio-codec $AUDIO?aac --audio-bitrate $AUDIO?160 --audio-disposition $AUDIO?default $CAPTION -m max_interleave_delta:500k $OUTPUT',
    audioStartAt=1,
    filter='--gop-len 120 --interlace tff --vpp-deinterlace normal',
    filterCinema='--gop-len 96 --interlace tff --vpp-deinterlace normal --vpp-decimate',
    filterFast='--fps '..math.floor(30000*XCODE_FAST+0.5)..'/1001 --gop-len '..math.floor(120*XCODE_FAST)..' --interlace tff --vpp-deinterlace normal',
    filterCinemaFast='--fps '..math.floor(30000*XCODE_FAST+0.5)..'/1001 --gop-len '..math.floor(96*XCODE_FAST)..' --interlace tff --vpp-deinterlace normal --vpp-decimate',
    editorFast='ffmpeg\\ffmpeg.exe|ffmpeg.exe',
    editorOptionFast='-f mpegts -analyzeduration 1M -i - -bsf:v setts=ts=TS/'..XCODE_FAST..' -map 0:v?:0 -vcodec copy -af atempo='..XCODE_FAST..' -bsf:s setts=ts=TS/'..XCODE_FAST..' -map 0:a -acodec ac3 -ac 2 -b:a 640k -map 0:s? -scodec copy -max_interleave_delta 300k -f mpegts -',
    captionNone='',
    captionHls='--sub-copy',
    output={'mp4','-f mp4 --no-mp4opt -m movflags:frag_keyframe+empty_moov -o -'},
    outputHls={'m2t','-f mpegts -o -'},
  },
  {
    --QSVEncCの例。倍速再生にはffmpegも必要
    name='720p/h264/QSVEncC',
    xcoder='QSVEncC\\QSVEncC64.exe|QSVEncC\\QSVEncC.exe|QSVEncC64.exe|qsvencc.exe',
    option='--input-format mpegts --input-analyze 1 --input-probesize 4M -i - --avhw --profile main --level 4.1 --qvbr 3936 --qvbr-quality 26 --fallback-rc --max-bitrate 8192 --vbv-bufsize 8192 $FILTER --output-res 1280x720 --audio-stream $AUDIO?:stereo --audio-codec $AUDIO?aac --audio-bitrate $AUDIO?160 --audio-disposition $AUDIO?default $CAPTION -m max_interleave_delta:500k $OUTPUT',
    audioStartAt=1,
    filter='--gop-len 120 --interlace tff --vpp-deinterlace normal',
    filterCinema='--gop-len 96 --interlace tff --vpp-deinterlace normal --vpp-decimate',
    filterFast='--fps '..math.floor(30000*XCODE_FAST+0.5)..'/1001 --gop-len '..math.floor(120*XCODE_FAST)..' --interlace tff --vpp-deinterlace normal',
    filterCinemaFast='--fps '..math.floor(30000*XCODE_FAST+0.5)..'/1001 --gop-len '..math.floor(96*XCODE_FAST)..' --interlace tff --vpp-deinterlace normal --vpp-decimate',
    editorFast='ffmpeg\\ffmpeg.exe|ffmpeg.exe',
    editorOptionFast='-f mpegts -analyzeduration 1M -i - -bsf:v setts=ts=TS/'..XCODE_FAST..' -map 0:v?:0 -vcodec copy -af atempo='..XCODE_FAST..' -bsf:s setts=ts=TS/'..XCODE_FAST..' -map 0:a -acodec ac3 -ac 2 -b:a 640k -map 0:s? -scodec copy -max_interleave_delta 300k -f mpegts -',
    captionNone='',
    captionHls='--sub-copy',
    output={'mp4','-f mp4 --no-mp4opt -m movflags:frag_keyframe+empty_moov -o -'},
    outputHls={'m2t','-f mpegts -o -'},
  },
}

--フォーム上の各オプションのデフォルト選択状態を指定する
XCODE_SELECT_OPTION=1
XCODE_CHECK_CINEMA=false
XCODE_CHECK_FAST=false
XCODE_CHECK_CAPTION=false
XCODE_CHECK_JIKKYO=false

--トランスコード時、初期値ミュートで再生するかどうか
--自動再生が無効になるブラウザが多いため、一時停止しつづけるとタイムアウトするトランスコード時はミュートを推奨
XCODE_VIDEO_MUTED=true

--非トランスコード時、初期値ミュートで再生するかどうか
VIDEO_MUTED=false

--音量の初期値。0～1、nilのとき未指定
VIDEO_VOLUME=nil

--字幕表示のオプション https://github.com/monyone/aribb24.js#options
ARIBB24_JS_OPTION=[=[
  normalFont:'"Rounded M+ 1m for ARIB","Yu Gothic Medium",sans-serif',
  drcsReplacement:true
]=]

--字幕表示にSVGRendererを使うかどうか。描画品質が上がる(ただし一部ブラウザで背景に線が入る)。IE非対応
ARIBB24_USE_SVG=false

--データ放送表示機能を使うかどうか。トランスコード中に表示する場合はpsisiarc.exeを用意すること。IE非対応
USE_DATACAST=true

--ライブ実況表示機能を使うかどうか(Windows専用)
--利用には実況を扱うツール側の対応(NicoJKの場合はcommentShareMode)が必要
USE_LIVEJK=true

--実況ログ表示機能を使う場合、jkrdlog.exeの絶対パス
JKRDLOG_PATH=nil
--JKRDLOG_PATH='C:\\Path\\to\\jkrdlog.exe'

--実況コメントの文字の高さ(px)
JK_COMMENT_HEIGHT=32

--実況コメントの表示時間(秒)
JK_COMMENT_DURATION=5

--実況ログ表示機能のデジタル放送のサービスIDと、実況の番号(jk?)
--キーの下4桁の16進数にサービスID、上1桁にネットワークID(ただし地上波は15=0xF)を指定
--指定しないサービスにはjkrdlogの既定値が使われる
JK_CHANNELS={
  --例:テレビ東京(0x0430)をjk7と対応づけたいとき
  --[0xF0430]=7,
  --例:NHKBS1(0x0065)とデフォルト(jk101)との対応付けを解除したいとき
  --[0x40065]=-1,
}

--chatタグ表示前の置換(JavaScript)
JK_CUSTOM_REPLACE=[=[
  // 広告などを下コメにする
  tag = tag.replace(/^<chat(?![^>]*? mail=)/, '<chat mail=""');
  tag = tag.replace(/^(<chat[^>]*? premium="3"[^>]*?>\/nicoad )(\{[^<]*?"totalAdPoint":)(\d+)/, "$1$3$2");
  tag = tag.replace(/^<chat(?=[^>]*? premium="3")([^>]*? mail=")([^>]*?>)\/nicoad (\d*)\{[^<]*?"message":("[^<]*?")[,}][^<]*/, '<chat align="right"$1shita small yellow $2$4($3pt)');
  tag = tag.replace(/^<chat(?=[^>]*? premium="3")([^>]*? mail=")([^>]*?>)\/spi /, '<chat align="right"$1shita small white2 $2');
]=]

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

--このサイズ以上のときページ圧縮する(nilのとき常に非圧縮)
GZIP_THRESHOLD_BYTE=4096

--処理するPOSTリクエストボディの最大値
POST_MAX_BYTE=1024*1024

----------定数定義ここまで----------

--以下、関数名はパスカルケース、定数名はアッパースネークケースとし、変数は関数スコープに閉じ込めること

function Checkbox(b)
  return ' type="checkbox" value="1"'..(b and ' checked' or '')
end

function Selected(b)
  return b and ' selected' or ''
end

function GetTranscodeQueries(qs)
  local reload=(mg.get_var(qs,'reload') or ''):match('^'..('[0-9a-f]'):rep(16,'?')..'$')
  local loadKey=reload or (mg.get_var(qs,'load') or ''):match('^'..('[0-9a-f]'):rep(16,'?')..'$')
  return {
    option=GetVarInt(qs,'option',1,#XCODE_OPTIONS),
    offset=GetVarInt(qs,'offset',0,100),
    audio2=GetVarInt(qs,'audio2')==1,
    cinema=GetVarInt(qs,'cinema')==1,
    fast=GetVarInt(qs,'fast')==1,
    reload=not not reload,
    loadKey=loadKey,
    caption=(GetVarInt(qs,'caption') or XCODE_CHECK_CAPTION and 1)==1,
    jikkyo=(GetVarInt(qs,'jikkyo') or XCODE_CHECK_JIKKYO and 1)==1,
  }
end

function ConstructTranscodeQueries(xq)
  return (xq.option and '&amp;option='..xq.option or '')
    ..(xq.offset and '&amp;offset='..xq.offset or '')
    ..(xq.audio2 and '&amp;audio2=1' or '')
    ..(xq.cinema and '&amp;cinema=1' or '')
    ..(xq.fast and '&amp;fast=1' or '')
    ..(xq.loadKey and '&amp;'..(xq.reload and 're' or '')..'load='..xq.loadKey or '')
end

function VideoWrapperBegin()
  return '<div class="video-wrapper" id="vid-wrap">'
    ..'<div class="data-broadcasting-browser-container"><div class="data-broadcasting-browser-content"></div></div>'
    ..'<div class="video-full-container arib-video-invisible-container" id="vid-full">'
    ..'<div class="video-container arib-video-container" id="vid-cont">'
end

function VideoWrapperEnd()
  return '</div></div></div>'
end

function TranscodeSettingTemplate(xq,fsec)
  local s='<select name="option">'
  for i,v in ipairs(XCODE_OPTIONS) do
    if not ALLOW_HLS or not ALWAYS_USE_HLS or v.outputHls then
      s=s..'<option value="'..i..'"'..Selected((xq.option or XCODE_SELECT_OPTION)==i)..'>'..EdcbHtmlEscape(v.name)
    end
  end
  s=s..'</select>\n'
  if fsec then
    s=s..'offset: <select name="offset">'
    for i=0,100 do
      s=s..'<option value="'..i..'"'..Selected((xq.offset or 0)==i)..'>'
        ..(fsec>0 and ('%dm%02ds'):format(math.floor(fsec*i/100/60),fsec*i/100%60)..(i%5==0 and '|'..i..'%' or '') or i..'%')
    end
    s=s..'</select>\n'
  end
  s=s..'<label><input name="audio2"'..Checkbox(xq.audio2)..'>audio2</label>\n'
    ..'<label><input name="cinema"'..Checkbox(xq.cinema or not xq.option and XCODE_CHECK_CINEMA)..'>cinema</label>\n'
  if fsec then
    s=s..'<label><input name="fast"'..Checkbox(xq.fast or not xq.option and XCODE_CHECK_FAST)..'>fast</label>\n'
      ..'<span id="vid-offset"></span>'
  end
  s=s..'<span id="vid-bitrate"></span>\n'
    ..'<input type="hidden" name="caption" value="">\n'
    ..'<input type="hidden" name="jikkyo" value="">\n'
  return s
end

function OnscreenButtonsScriptTemplate()
  return [=[
<script src="script.js?ver=20241007"></script>
<script>
var vid=document.getElementById("vid");
var vcont=document.getElementById("vid-cont");
var vfull=document.getElementById("vid-full");
var vwrap=document.getElementById("vid-wrap");
var setSendComment;
var hideOnscreenButtons;
runOnscreenButtonsScript();
</script>
]=]
end

function WebBmlScriptTemplate(label)
  return USE_DATACAST and [=[
<div class="remote-control" style="display:none">
  <button type="button" id="key21">青</button><button
    type="button" id="key22">赤</button><button
    type="button" id="key23">緑</button><button
    type="button" id="key24">黄</button><button
    type="button" id="key1">↑</button><button
    type="button" id="key3">←</button><button
    type="button" id="key18">決定</button><button
    type="button" id="key4">→</button><button
    type="button" id="key2">↓</button><button
    type="button" id="key20">d</button><button
    type="button" id="key19">戻る</button><button
    type="button" id="key6">1</button><button
    type="button" id="key7">2</button><button
    type="button" id="key8">3</button><button
    type="button" id="key9">4</button><button
    type="button" id="key10">5</button><button
    type="button" id="key11">6</button><button
    type="button" id="key12">7</button><button
    type="button" id="key13">8</button><button
    type="button" id="key14">9</button><button
    type="button" id="key15">10</button><button
    type="button" id="key16">11</button><button
    type="button" id="key17">12</button><button
    type="button" id="key5">0</button>
  <span class="remote-control-receiving-status" style="display:none">Loading...</span>
  <div class="remote-control-indicator"></div>
</div>
<label><input id="cb-datacast" type="checkbox">]=]..label..[=[</label>
<script src="web_bml_play_ts.js"></script>
]=] or ''
end

function JikkyoScriptTemplate(live,jikkyo)
  return (live and USE_LIVEJK or not live and JKRDLOG_PATH) and [=[
<label><input id="cb-jikkyo"]=]..Checkbox(jikkyo)..[=[>jikkyo</label>
<label class="enabled-on-checked"><input id="cb-jikkyo-onscr" type="checkbox" checked>onscr</label>
<script src="danmaku.js"></script>
<script>
var onJikkyoStream=null;
var onJikkyoStreamError=null;
var checkJikkyoDisplay;
var toggleJikkyo;
runJikkyoScript(]=]..JK_COMMENT_HEIGHT..','..JK_COMMENT_DURATION..',function(tag){'..JK_CUSTOM_REPLACE..[=[
  return tag;});
</script>
]=] or [=[
<script>
var onJikkyoStream=null;
var onJikkyoStreamError=null;
var checkJikkyoDisplay=function(){};
</script>
]=]
end

function VideoScriptTemplate()
  return OnscreenButtonsScriptTemplate()..WebBmlScriptTemplate('datacast.psc')..JikkyoScriptTemplate(false,XCODE_CHECK_JIKKYO)..[=[
<label id="label-caption" style="display:none"><input id="cb-caption"]=]..Checkbox(XCODE_CHECK_CAPTION)..[=[>caption.vtt</label>
<script src="aribb24.js"></script>
<script>
]=]..(VIDEO_MUTED and 'vid.muted=true;\n' or '')..(VIDEO_VOLUME and 'vid.volume='..VIDEO_VOLUME..';\n' or '')..[=[
runVideoScript(]=]
  ..(ARIBB24_USE_SVG and 'true' or 'false')..',{'..ARIBB24_JS_OPTION..'},'
  ..(USE_DATACAST and 'true' or 'false')..','
  ..(JKRDLOG_PATH and 'true' or 'false')..[=[
);
</script>
]=]
end

function TranscodeScriptTemplate(live,caption,jikkyo,params)
  return OnscreenButtonsScriptTemplate()..WebBmlScriptTemplate('datacast')..JikkyoScriptTemplate(live,jikkyo)..[=[
<label id="label-caption" style="display:none"><input id="cb-caption"]=]..Checkbox(caption)..[=[>caption</label>
]=]..(live and '<label><input id="cb-live" type="checkbox">live</label>\n' or '')..[=[
<input id="vid-seek" type="range" style="display:none">
<span id="vid-seek-status"></span>
<button id="vid-unmute" type="button" style="display:none">🔊</button>
<script>
]=]..(XCODE_VIDEO_MUTED and 'vid.muted=true;\n' or '')..(VIDEO_VOLUME and 'vid.volume='..VIDEO_VOLUME..';\n' or '')..[=[
runTranscodeScript(]=]
  ..(USE_DATACAST and 'true' or 'false')..','
  ..(live and USE_LIVEJK and 'true' or 'false')..','
  ..(not live and JKRDLOG_PATH and 'true' or 'false')..','
  ..math.floor(params.ofssec or 0)..','
  ..(params.fast and XCODE_FAST or 1)..','
  ..'"'..(live and USE_LIVEJK and 'ctok='..CsrfToken('comment.lua')..'&n='..params.n..(params.id and '&id='..params.id or '') or '')..'"'..[=[
);
</script>
]=]
end

function HlsScriptTemplate(target)
  return [=[
<script src="aribb24.js"></script>
]=]..(ALWAYS_USE_HLS and [=[
<script src="hls.min.js"></script>
]=] or '')..[=[
<script>
runHlsScript(]=]
  ..(ARIBB24_USE_SVG and 'true' or 'false')..',{'..ARIBB24_JS_OPTION..'},'
  ..(ALWAYS_USE_HLS and 'true' or 'false')..','
  ..'"ctok='..CsrfToken(target)..'&open=1",'
  ..'"&hls='..(edcb.CreateRandom and edcb.CreateRandom(8) or os.time()%86400)..'",'
  ..'"'..(USE_MP4_HLS and '&hls4='..(USE_MP4_LLHLS and '2' or '1') or '')..'"'..[=[
);
</script>
]=]
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
function RecSettingTemplate(rs,setting)
  local s='<label><input name="recEnabled"'..Checkbox(rs.recMode~=5)..'>有効</label><br>\n'
    ..'録画モード: <select name="recMode">'
  for i=1,#RecModeTextList() do
    s=s..'<option value="'..(i-1)..'"'..Selected((rs.recMode~=5 and rs.recMode or rs.noRecMode or 1)==i-1)..'>'..RecModeTextList()[i]
  end
  s=s..'</select><br>\n'
    ..'<label><input name="tuijyuuFlag"'..Checkbox(rs.tuijyuuFlag)..'>イベントリレー追従</label><br>\n'
    ..'優先度: <select name="priority">'
  for i=1,5 do
    s=s..'<option value="'..i..'"'..Selected(rs.priority==i)..'>'..i..(i==1 and ' (低)' or i==5 and ' (高)' or '')
  end
  --デフォルト値
  local rsdef=(edcb.GetReserveData(0x7FFFFFFF) or {}).recSetting
  s=s..'</select><br>\n'
    ..'<label><input name="pittariFlag"'..Checkbox(rs.pittariFlag)..'>ぴったり（？）録画</label><br>\n'
    ..'録画マージン: <label><input name="useDefMarginFlag"'..Checkbox(not rs.startMargin)..'><span class="enabled-on-checked">デフォルト</span></label> || <span class="disabled-on-checked">'
    ..'開始（秒） <input type="text" name="startMargin" value="'..(rs.startMargin or rsdef and rsdef.startMargin or 0)..'" size="5"> '
    ..'終了（秒） <input type="text" name="endMargin" value="'..(rs.endMargin or rsdef and rsdef.endMargin or 0)..'" size="5"></span><br>\n'
    ..'指定サービス対象データ: <label><input class="" name="serviceMode"'..Checkbox(rs.serviceMode%2==0)..'><span class="enabled-on-checked">デフォルト</span></label> || <span class="disabled-on-checked">'
    ..'<label><input name="serviceMode_1"'..Checkbox(math.floor(rs.serviceMode%2~=0 and rs.serviceMode/16 or rsdef and rsdef.serviceMode/16 or 0)%2~=0)..'>字幕を含める</label> '
    ..'<label><input name="serviceMode_2"'..Checkbox(math.floor(rs.serviceMode%2~=0 and rs.serviceMode/32 or rsdef and rsdef.serviceMode/32 or 0)%2~=0)..'>データカルーセルを含める</label></span><br>\n'
    ..'<table><tr><td>録画フォルダ</td><td>出力PlugIn</td><td>ファイル名PlugIn</td><td>部分受信</td></tr>\n'
  for i,v in ipairs(rs.recFolderList) do
    s=s..'<tr><td>'..v.recFolder..'</td><td>'..v.writePlugIn..'</td><td>'..v.recNamePlugIn..'</td><td>いいえ</td></tr>\n'
  end
  for i,v in ipairs(rs.partialRecFolder) do
    s=s..'<tr><td>'..v.recFolder..'</td><td>'..v.writePlugIn..'</td><td>'..v.recNamePlugIn..'</td><td>はい</td></tr>\n'
  end
  s=s..'</table>'..(setting and '<a href="'..setting..'">録画フォルダを編集</a>' or '（プリセットによる変更のみ対応）')..'<br>\n'
    ..'<label><input name="partialRecFlag"'..Checkbox(rs.partialRecFlag~=0)..'>部分受信（ワンセグ）を別ファイルに同時出力する</label><br>\n'
    ..'<label><input name="continueRecFlag"'..Checkbox(rs.continueRecFlag)..'>後ろの予約を同一ファイルで出力する</label><br>\n'
    ..'使用チューナー強制指定: <select name="tunerID"><option value="0"'..Selected(rs.tunerID==0)..'>自動'
  local a=edcb.GetTunerReserveAll()
  for i=1,#a-1 do
    s=s..'<option value="'..a[i].tunerID..'"'..Selected(a[i].tunerID==rs.tunerID)..('>ID:%08X('):format(a[i].tunerID)..a[i].tunerName..')'
  end
  s=s..'</select><br>\n'
    ..'録画後動作: <select name="suspendMode">'
    ..'<option value="0"'..Selected(rs.suspendMode==0)..'>'..(rsdef and ({'スタンバイ','休止','シャットダウン','何もしない'})[rsdef.suspendMode] or '')..'（デフォルト）'
    ..'<option value="1"'..Selected(rs.suspendMode==1)..'>スタンバイ'
    ..'<option value="2"'..Selected(rs.suspendMode==2)..'>休止'
    ..'<option value="3"'..Selected(rs.suspendMode==3)..'>シャットダウン'
    ..'<option value="4"'..Selected(rs.suspendMode==4)..'>何もしない</select> '
    ..'<label><input name="rebootFlag"'..Checkbox(rs.suspendMode==0 and rsdef and rsdef.rebootFlag or rs.suspendMode~=0 and rs.rebootFlag)..'>復帰後再起動する</label><br>\n'
    ..'録画後実行bat[*タグ]'..(setting and '' or '（プリセットによる変更のみ対応）')..':<br>\n'
    ..'<input type="text" name="batFilePath" value="'..rs.batFilePath..'" style="width:95%"'..(setting and '' or ' readonly')..'><br>\n'
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
  return ('%d/%02d/%02d(%s) %02d:%02d'):format(t.year,t.month,t.day,({'日','月','火','水','木','金','土',})[t.wday],t.hour,t.min)
    ..(t.sec~=0 and ('<small>:%02d</small>'):format(t.sec) or '')
    ..(dur and ('～%02d:%02d'):format(math.floor(dur/3600)%24,math.floor(dur/60)%60)..(dur%60~=0 and ('<small>:%02d</small>'):format(dur%60) or '') or '')
end

--システムのタイムゾーンに影響されずに時間のテーブルを数値表現にする (timezone=0のとき概ねos.date('!*t')の逆関数)
function TimeWithZone(t,timezone)
  return os.time(t)+90000-os.time(os.date('!*t',90000))-(timezone or 0)
end

--Windowsかどうか
WIN32=not package.config:find('^/')

--OSのディレクトリ区切りとなる文字集合
DIR_SEPS=WIN32 and '\\/' or '/'

--OSの標準ディレクトリ区切り
DIR_SEP=WIN32 and '\\' or '/'

--io.popenのバイナリオープンモード
POPEN_BINARY=WIN32 and 'b' or ''

--パスを連結する
function PathAppend(path,more)
  return path:gsub('['..DIR_SEPS..']*$',DIR_SEP)..more:gsub('^['..DIR_SEPS..']+','')
end

--パスとして同一かどうか
function IsEqualPath(path1,path2)
  return (WIN32 and path1:upper()==path2:upper()) or (not WIN32 and path1==path2)
end

--ドキュメントルートへの相対パスを取得する
function PathToRoot()
  return ('../'):rep(#mg.script_name:gsub('[^'..DIR_SEPS..']*['..DIR_SEPS..']+[^'..DIR_SEPS..']*','N')-
                     #(mg.document_root..'/'):gsub('[^'..DIR_SEPS..']*['..DIR_SEPS..']+','N'))
end

--OSの絶対パスをドキュメントルートからの相対パスに変換する
function NativeToDocumentPath(path)
  local root=(mg.document_root..'/'):gsub('['..DIR_SEPS..']+','/')
  if IsEqualPath(path:gsub('['..DIR_SEPS..']+','/'):sub(1,#root),root) then
    return path:gsub('['..DIR_SEPS..']+','/'):sub(#root+1)
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
  if not path:find('[\0-\x1f\x7f'..(WIN32 and '\\:*?"<>|' or '')..']') and not path:find('%./') and not path:find('%.$') then
    return PathAppend(mg.document_root,path:gsub('/',DIR_SEP))
  end
  return nil
end

--EDCBフォルダのパス
function EdcbModulePath()
  return edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')
end

--プラグインファイル等のフォルダのパス (指定されている場合)
function EdcbLibPath()
  local dir=edcb.GetPrivateProfile('SET','ModuleLibPath','','Common.ini')
  return dir~='' and dir
end

--設定関係保存フォルダのパス
function EdcbSettingPath()
  local dir=edcb.GetPrivateProfile('SET','DataSavePath','','Common.ini')
  return dir~='' and dir or PathAppend(EdcbModulePath(),'Setting')
end

--録画保存フォルダのパスのリスト
function EdcbRecFolderPathList()
  local n=tonumber(edcb.GetPrivateProfile('SET','RecFolderNum',0,'Common.ini')) or 0
  local r={n>0 and edcb.GetPrivateProfile('SET','RecFolderPath0','','Common.ini') or ''}
  if r[1]=='' then
    --必ず返す
    r[1]=EdcbSettingPath()
  end
  for i=2,n do
    local dir=edcb.GetPrivateProfile('SET','RecFolderPath'..(i-1),'','Common.ini')
    --空要素は詰める
    if dir~='' then
      r[#r+1]=dir
    end
  end
  return r
end

--プラグインファイル名を列挙する
function EnumPlugInFileName(name)
  local esc=edcb.htmlEscape
  edcb.htmlEscape=0
  local pattern=PathAppend(EdcbLibPath() or PathAppend(EdcbModulePath(),name),name)..(WIN32 and '*.dll' or '*.so')
  edcb.htmlEscape=esc
  local r={}
  for i,v in ipairs(edcb.FindFile(pattern,0) or {}) do
    if not v.isdir then
      r[#r+1]=v.name
    end
  end
  return r
end

--現在の変換モードでHTMLエスケープする
function EdcbHtmlEscape(s)
  return edcb.Convert('utf-8','utf-8',s)
end

--単一のファイルに関する情報を探す
function EdcbFindFilePlain(path)
  local n=path:find('[^'..DIR_SEPS..']*$')
  if not path:find('[*?]',n) then
    --そのまま
    local ff=edcb.FindFile(path,1)
    return ff and ff[1]
  end
  --ワイルドカード文字を含むので、その効果を打ち消すために*を?にして候補を比較
  for i,v in ipairs(edcb.FindFile(path:sub(1,n-1)..path:sub(n):gsub('%*','?'),0) or {}) do
    if IsEqualPath(EdcbHtmlEscape(path:sub(n)),v.name) then return v end
  end
  return nil
end

--プロセス名(拡張子を除いたもの)とコマンドラインのパターン(部分一致)に一致するコマンドをすべて終了させる
function TerminateCommandlineLike(name,pattern)
  if not WIN32 then
    --拡張正規表現の記号はエスケープ
    name=name:gsub('[$()*+.?[\\%]^{|}]','\\%0')
    pattern=pattern:gsub('[$()*+.?[\\%]^{|}]','\\%0')
    edcb.os.execute('pkill -9 -xf "'..name..(pattern=='' and '( .*)?' or
      pattern:find('^ ') and '('..pattern..'| .*'..pattern..').*' or ' .*'..pattern..'.*')..'"')
  elseif pattern=='' then
    edcb.os.execute('taskkill /f /im "'..name..'.exe"')
  elseif not edcb.os.execute('wmic process where "name=\''..name..'.exe\' and commandline like \'%'..pattern:gsub('_','[_]')..'%\'" call terminate >nul') then
    --wmicがないとき
    edcb.os.execute('powershell -NoProfile -c "try{(gwmi win32_process -filter \\"name=\''..name..'.exe\' and commandline like \'%'
      ..pattern:gsub('_','[_]')..'%\'\\").terminate()}catch{}"')
  end
end

--コマンドラインのコマンド名として使うコマンドを探す
function FindToolsCommand(name)
  if not WIN32 then
    --そのまま。ただし親プロセスのシグナルマスクを継承しないようにする
    return 'env --default-signal '..name
  end
  --EDCBのToolsフォルダにあるものを優先する
  local esc=edcb.htmlEscape
  edcb.htmlEscape=0
  local path=PathAppend(EdcbModulePath(),PathAppend('Tools',name..'.exe'))
  edcb.htmlEscape=esc
  --拡張子をつけて引用符で囲む
  return '"'..(EdcbFindFilePlain(path) and path or name..'.exe')..'"'
end

--コマンドラインの引数として使うパスを引用符で囲む
--※Windowsでは引用符などパスとして不正な文字がpathに含まれていないことが前提
function QuoteCommandArgForPath(path)
  return WIN32 and '"'..path:gsub('[&%^]','^%0')..'"' or "'"..path:gsub("'","'\"'\"'").."'"
end

--SendTSTCPのストリーム取得用パイプのパス
function SendTSTCPPipePath(name,index)
  if WIN32 then
    --同時利用でも名前は同じ
    return '\\\\.\\pipe\\SendTSTCP_'..name
  end
  --同時利用のためのindexがつく
  local esc=edcb.htmlEscape
  edcb.htmlEscape=0
  local path=PathAppend(EdcbModulePath(),'SendTSTCP_'..name..'_'..index..'.fifo')
  edcb.htmlEscape=esc
  return path
end

--tsmemsegのストリーム取得用パイプのパス
function TsmemsegPipePath(name,suffix)
  if WIN32 then
    return '\\\\.\\pipe\\tsmemseg_'..name..suffix
  end
  local esc=edcb.htmlEscape
  edcb.htmlEscape=0
  local path=PathAppend(EdcbModulePath(),'tsmemseg_'..name..suffix..'.fifo')
  edcb.htmlEscape=esc
  return path
end

--tsmemsegのストリーム取得用パイプを開く
function OpenTsmemsegPipe(name,suffix)
  if WIN32 then
    return edcb.io.open(TsmemsegPipePath(name,suffix),'rb')
  end
  for retry=1,9 do
    local f=edcb.io.open(TsmemsegPipePath(name,suffix),'rb')
    if not f then break end
    --FIFOは同時に読めてしまうのでプロセス間でロックが必要
    if edcb.io._flock_nb(f) then
      --タイミングによっては途中から読んでしまう可能性があるので検証が必要
      local buf=f:read(suffix=='00' and 64 or 188)
      if buf and (suffix=='00' and #buf==64 and buf:find('^'..name) or
                  suffix~='00' and #buf==188 and buf:find('^....'..name)) then
        return f
      end
    end
    f:close()
    edcb.Sleep(10*retry)
  end
  return nil
end

--符号なし整数の時計算の差を計算する
function UintCounterDiff(a,b)
  return (a+0x100000000-b)%0x100000000
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
          return pcr,pid2,i*188
        end
      end
    end
  end
  return nil
end

--PCRをもとにファイルの長さを概算する
function GetDurationSec(f)
  local fsize=f:seek('end') or 0
  if fsize>1880000 and f:seek('set') then
    local pcr,pid=ReadToPcr(f)
    if pcr and f:seek('set',(math.floor(fsize/188)-10000)*188) then
      local pcr2,pid2,n=ReadToPcr(f,pid)
      if pcr2 then
        --終端まで読む
        local range=1880000
        while true do
          local dur=math.floor(UintCounterDiff(pcr2,pcr)/45000)
          range=range-n
          pcr2,pid2,n=ReadToPcr(f,pid)
          if not pcr2 or range<0 then
            return dur,fsize
          end
        end
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
          return math.floor(UintCounterDiff(pcr2,pcr)/45000),predicted
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
      --最終目標の3秒手前を目標に6ループまたは誤差が±3秒未満になるまで動画レートから概算シーク
      local pos,diff,rate=0,math.min(math.max(sec-3,0),dur)*45000,fsize/dur
      for i=1,6 do
        if math.abs(diff)<45000*3 then break end
        local approx=math.floor(math.min(math.max(pos+rate*diff/45000,0),fsize-1880000)/188)*188
        if not f:seek('set',approx) then return false end
        local pcr2=ReadToPcr(f,pid)
        if not pcr2 then return false end
        --移動分を差し引く
        local diff2=diff+(UintCounterDiff(pcr2,pcr)<0x80000000 and -UintCounterDiff(pcr2,pcr) or UintCounterDiff(pcr,pcr2))
        if math.abs(diff2)>=45000*3 and ((diff<0 and diff2>-diff/2) or (diff>0 and diff2<-diff/2)) then
          --移動しすぎているのでレートを下げてやり直し
          rate=rate/1.5
        else
          if (diff<0 and diff2*2<diff) or (diff>0 and diff2*2>diff) then
            --あまり移動していないのでレートを上げる
            rate=rate*1.5
          end
          pos=approx
          pcr=pcr2
          diff=diff2
        end
      end
      if math.abs(diff)<45000*3 then
        --最終目標まで進む
        diff=diff+45000*3
        local diff2=diff
        while diff2>22500 do
          if diff2>45000*6 then return false end
          local pcr2=ReadToPcr(f,pid)
          if not pcr2 then return false end
          diff2=diff+(UintCounterDiff(pcr2,pcr)<0x80000000 and -UintCounterDiff(pcr2,pcr) or UintCounterDiff(pcr,pcr2))
        end
      end
      return true
    end
  end
  return false
end

--ファイルの先頭のTOT時刻とネットワークIDとサービスIDを取得する
function GetTotAndServiceID(f)
  if f:seek('set') then
    local pcr,pcrPid=ReadToPcr(f)
    if pcr then
      local tot,nid,sid=nil,nil,nil
      for i=1,400000 do
        local buf=f:read(188)
        if not buf or #buf~=188 or buf:byte(1)~=0x47 then break end
        local adaptation=math.floor(buf:byte(4)/16)%4
        local adaptationLen=adaptation==1 and -1 or adaptation==3 and buf:byte(5) or 183
        --payload_unit_start_indicator
        if math.floor(buf:byte(2)/64)%2==1 and adaptationLen<183 then
          local pid=buf:byte(2)%32*256+buf:byte(3)
          local pointer=7+adaptationLen+buf:byte(6+adaptationLen)
          local id=pointer<=188 and buf:byte(pointer)
          if pid==0 and pointer+13<=188 and id==0x00 then
            --PAT
            local sectionLen=buf:byte(pointer+2)
            sid=buf:byte(pointer+8)*256+buf:byte(pointer+9)
            if sectionLen>=17 and sid==0 then
              sid=buf:byte(pointer+12)*256+buf:byte(pointer+13)
            end
            if sectionLen<13 or sid==0 then
              sid=nil
            end
          elseif pid==16 and pointer+4<=188 and id==0x40 then
            --NIT
            nid=buf:byte(pointer+3)*256+buf:byte(pointer+4)
          elseif pid==20 and pointer+7<=188 and (id==0x70 or id==0x73) and not tot then
            --TDT,TOT
            local pcr2=ReadToPcr(f,pcrPid)
            if not pcr2 then break end
            local mjd=buf:byte(pointer+3)*256+buf:byte(pointer+4)
            local h=buf:byte(pointer+5)
            local m=buf:byte(pointer+6)
            local s=buf:byte(pointer+7)
            tot=((mjd*24+math.floor(h/16)*10+h%16)*60+math.floor(m/16)*10+m%16)*60+math.floor(s/16)*10+s%16-
                3506749200-math.floor(UintCounterDiff(pcr2,pcr)/45000)
          end
          if tot and nid and sid then
            return tot,nid,sid
          end
        end
      end
    end
  end
  return nil
end

--ライブ実況やjkrdlogの出力のチャンクを1つだけ読み取る
function ReadJikkyoChunk(f)
  local head=f:read(80)
  if not head or #head~=80 then return nil end
  local payload=''
  local payloadSize=tonumber(head:match('L=([0-9]+)'))
  if not payloadSize then return nil end
  if payloadSize>0 then
    payload=f:read(payloadSize)
    if not payload or #payload~=payloadSize then return nil end
  end
  return head..payload
end

--jkrdlogに渡す実況のIDを取得する
function GetJikkyoID(nid,sid)
  --地上波のサービス種別とサービス番号はマスクする
  local id=NetworkType(nid)=='地デジ' and 0xf0000+bit32.band(sid,0xfe78) or nid*65536+sid
  return not JK_CHANNELS[id] and 'ns'..id or JK_CHANNELS[id]>0 and 'jk'..JK_CHANNELS[id]
end

--リトルエンディアンの値を取得する
function GetLeNumber(buf,pos,len)
  local n=0
  for i=pos+len-1,pos,-1 do n=n*256+buf:byte(i) end
  return n
end

DOCTYPE_HTML4_STRICT='<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">\n'

--HTTP日付の文字列を取得する
function ImfFixdate(t)
  return ('%s, %02d %s %d %02d:%02d:%02d GMT'):format(({'Sun','Mon','Tue','Wed','Thu','Fri','Sat'})[t.wday],t.day,
    ({'Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'})[t.month],t.year,t.hour,t.min,t.sec)
end

--レスポンスを生成する
function Response(code,ctype,charset,cl,cz,maxage)
  return 'HTTP/1.1 '..code..' '..mg.get_response_code_text(code)
    ..'\r\nDate: '..ImfFixdate(os.date('!*t'))
    ..'\r\nX-Frame-Options: SAMEORIGIN'
    ..(ctype and '\r\nX-Content-Type-Options: nosniff\r\nContent-Type: '..ctype..(charset and '; charset='..charset or '') or '')
    ..(cl and mg.request_info.request_method~='HEAD' and '\r\nContent-Length: '..cl or '')
    ..(cz and '\r\nContent-Encoding: gzip' or '')
    ..'\r\nCache-Control: private, max-age='..(maxage or 0)
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

if not WIN32 then
  INDEX_ENABLE_SUSPEND=false
  USE_LIVEJK=false
end
