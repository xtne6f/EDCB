--EPG情報をTextに変換(EpgTimerUtil.cppから移植)
function _ConvertEpgInfoText2(onidOrEpg, tsid, sid, eid)
  local s, v = '', (type(onidOrEpg)=='table' and onidOrEpg or edcb.SearchEpg(onidOrEpg, tsid, sid, eid))
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
      s=s..'詳細情報\n'..DecorateUri(v.extInfo.text_char)..'\n\n'
    end
    if v.contentInfoList then
      s=s..'ジャンル : \n'
      for i,w in ipairs(v.contentInfoList) do
        --0x0E01はCS拡張用情報
        nibble=w.content_nibble==0x0E01 and w.user_nibble+0x7000 or w.content_nibble
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
    s=s..'\n'..((v.onid<0x7880 or 0x7FE8<v.onid) and (v.freeCAFlag and '有料放送\n' or '無料放送\n') or '')
      ..string.format('OriginalNetworkID:%d(0x%04X)\n', v.onid, v.onid)
      ..string.format('TransportStreamID:%d(0x%04X)\n', v.tsid, v.tsid)
      ..string.format('ServiceID:%d(0x%04X)\n', v.sid, v.sid)
      ..string.format('EventID:%d(0x%04X)\n', v.eid, v.eid)
  end
  return s
end

--録画設定フォームのテンプレート
function RecSettingTemplate(rs)
  local s='録画モード: <select name="recMode">'
    ..'<option value="0"'..(rs.recMode==0 and ' selected="selected"' or '')..'>全サービス'
    ..'<option value="1"'..(rs.recMode==1 and ' selected="selected"' or '')..'>指定サービスのみ'
    ..'<option value="2"'..(rs.recMode==2 and ' selected="selected"' or '')..'>全サービス（デコード処理なし）'
    ..'<option value="3"'..(rs.recMode==3 and ' selected="selected"' or '')..'>指定サービスのみ（デコード処理なし）'
    ..'<option value="4"'..(rs.recMode==4 and ' selected="selected"' or '')..'>視聴'
    ..'<option value="5"'..(rs.recMode==5 and ' selected="selected"' or '')..'>無効</select><br>\n'
    ..'イベントリレー追従: <select name="tuijyuuFlag">'
    ..'<option value="0"'..(not rs.tuijyuuFlag and ' selected="selected"' or '')..'>しない'
    ..'<option value="1"'..(rs.tuijyuuFlag and ' selected="selected"' or '')..'>する</select><br>\n'
    ..'優先度: <select name="priority">'
    ..'<option value="1"'..(rs.priority==1 and ' selected="selected"' or '')..'>1'
    ..'<option value="2"'..(rs.priority==2 and ' selected="selected"' or '')..'>2'
    ..'<option value="3"'..(rs.priority==3 and ' selected="selected"' or '')..'>3'
    ..'<option value="4"'..(rs.priority==4 and ' selected="selected"' or '')..'>4'
    ..'<option value="5"'..(rs.priority==5 and ' selected="selected"' or '')..'>5</select><br>\n'
    ..'ぴったり（？）録画: <select name="pittariFlag">'
    ..'<option value="0"'..(not rs.pittariFlag and ' selected="selected"' or '')..'>しない'
    ..'<option value="1"'..(rs.pittariFlag and ' selected="selected"' or '')..'>する</select><br>\n'
    ..'録画マージン: <input type="checkbox" name="useDefMarginFlag" value="1"'..(rs.startMargin and '' or ' checked="checked"')..'>デフォルト || '
    ..'開始（秒） <input type="text" name="startMargin" value="'..(rs.startMargin or 0)..'"> '
    ..'終了（秒） <input type="text" name="endMargin" value="'..(rs.endMargin or 0)..'"><br>\n'
    ..'指定サービス対象データ: <input type="checkbox" name="serviceMode" value="0"'..(rs.serviceMode%2==0 and ' checked="checked"' or '')..'>デフォルト || '
    ..'<input type="checkbox" name="serviceMode_1" value="0"'..(rs.serviceMode%2~=0 and math.floor(rs.serviceMode/16)%2~=0 and ' checked="checked"' or '')..'>字幕を含める '
    ..'<input type="checkbox" name="serviceMode_2" value="0"'..(rs.serviceMode%2~=0 and math.floor(rs.serviceMode/32)%2~=0 and ' checked="checked"' or '')..'>データカルーセルを含める<br>\n'
    ..'<table><tr><td>録画フォルダ</td><td>出力PlugIn</td><td>ファイル名PlugIn</td><td>部分受信</td></tr>\n'
  for i,v in ipairs(rs.recFolderList) do
    s=s..'<tr><td>'..v.recFolder..'</td><td>'..v.writePlugIn..'</td><td>'..v.recNamePlugIn..'</td><td>いいえ</td></tr>\n'
  end
  for i,v in ipairs(rs.partialRecFolder) do
    s=s..'<tr><td>'..v.recFolder..'</td><td>'..v.writePlugIn..'</td><td>'..v.recNamePlugIn..'</td><td>はい</td></tr>\n'
  end
  s=s..'</table>（プリセットによる変更のみ対応）<br>\n'
    ..'<input type="checkbox" name="partialRecFlag" value="1"'..(rs.partialRecFlag~=0 and ' checked="checked"' or '')..'>部分受信（ワンセグ）を別ファイルに同時出力する<br>\n'
    ..'<input type="checkbox" name="continueRecFlag" value="1"'..(rs.continueRecFlag and ' checked="checked"' or '')..'>後ろの予約を同一ファイルで出力する<br>\n'
    ..'使用チューナー強制指定: <select name="tunerID"><option value="0"'..(rs.tunerID==0 and ' selected="selected"' or '')..'>自動'
  local a=edcb.GetTunerReserveAll()
  for i=1,#a-1 do
    s=s..'<option value="'..a[i].tunerID..'"'..(a[i].tunerID==rs.tunerID and ' selected="selected"' or '')..string.format('>ID:%08X(', a[i].tunerID)..a[i].tunerName..')'
  end
  s=s..'</select><br>\n'
    ..'録画後動作: <select name="suspendMode">'
    ..'<option value="0"'..(rs.suspendMode==0 and ' selected="selected"' or '')..'>デフォルト設定を使用'
    ..'<option value="1"'..(rs.suspendMode==1 and ' selected="selected"' or '')..'>スタンバイ'
    ..'<option value="2"'..(rs.suspendMode==2 and ' selected="selected"' or '')..'>休止'
    ..'<option value="3"'..(rs.suspendMode==3 and ' selected="selected"' or '')..'>シャットダウン'
    ..'<option value="4"'..(rs.suspendMode==4 and ' selected="selected"' or '')..'>何もしない</select> '
    ..'<input type="checkbox" name="rebootFlag" value="1"'..(rs.rebootFlag and ' checked="checked"' or '')..'>復帰後再起動する<br>\n'
    ..'録画後実行bat（プリセットによる変更のみ対応）: '..(#rs.batFilePath==0 and '（なし）' or rs.batFilePath)..'<br>\n'
  return s
end

--URIをタグ装飾する
function DecorateUri(s)
  local i=1
  while i<=#s do
    if s:find('^http',i) or s:find('^ｈｔｔｐ',i) then
      local hw='&/:;%#$?()~.=+-_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
      local fw='＆／：；％＃＄？（）￣．＝＋－＿０１２３４５６７８９ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ'
      local j,href=i,''
      while j<=#s do
        local k=hw:find(s:sub(j,j),1,true)
        if k then
          href=href..hw:sub(k,k)
          j=j+1
        else
          k=fw:find(s:sub(j,j+2),1,true)
          if j+2<=#s and k and k%3==1 then
            href=href..hw:sub((k+2)/3,(k+2)/3)..(k==1 and 'amp;' or '')
            j=j+3
          else
            break
          end
        end
      end
      if href:find('^https?://.') then
        href='<a href="'..href..'">'..s:sub(i,j-1)..'</a>'
        s=s:sub(1,i-1)..href..s:sub(j)
        i=i+#href-(j-i)
      end
    end
    i=i+1
  end
  return s
end

--時間の文字列を取得する
function FormatTimeAndDuration(t,dur)
  dur=dur and (t.hour*3600+t.min*60+t.sec+dur)
  return string.format('%d/%02d/%02d(%s) %02d:%02d',t.year,t.month,t.day,({'日','月','火','水','木','金','土',})[t.wday],t.hour,t.min)
    ..(dur and string.format('～%02d:%02d',math.floor(dur/3600)%24,math.floor(dur/60)%60) or '')
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
  path=edcb.Convert('utf-8','cp932',edcb.Convert('cp932','utf-8',path)):gsub('/+','/')
  edcb.htmlEscape=esc
  --禁止文字と正規化のチェック
  if not path:find('[\0-\x1f\x7f\\:*?"<>|]') and not path:find('%./') and not path:find('%.$') then
    return mg.document_root..'\\'..path:gsub('/','\\')
  end
  return nil
end

--レスポンスを生成する
function Response(code,ctype,charset,cl)
  return 'HTTP/1.1 '..code..' '..mg.get_response_code_text(code)
    ..(ctype and '\r\nX-Content-Type-Options: nosniff\r\nContent-Type: '..ctype..(charset and '; charset='..charset or '') or '')
    ..(cl and '\r\nContent-Length: '..cl or '')
    ..(mg.keep_alive(not not cl) and '\r\n' or '\r\nConnection: close\r\n')
end

--可能ならコンテンツをzlib圧縮する(lua-zlib(zlib.dll)が必要)
function Deflate(ct)
  local zl
  local trim
  for k,v in pairs(mg.request_info.http_headers) do
    if not zl and k:lower()=='accept-encoding' and v:lower():find('deflate') then
      local status, zlib = pcall(require, 'zlib')
      if status then
        zl=zlib.deflate()(ct, 'finish')
      end
    elseif k:lower()=='user-agent' and (v:find(' MSIE ') or v:find(' Trident/7%.') or v:find(' Edge/')) then
      --RFC2616非準拠のブラウザはzlibヘッダを取り除く
      trim=true
    end
  end
  if trim and zl and #zl >= 6 then
    zl=zl:sub(3, #zl-4)
  end
  return zl
end

--POSTメッセージボディをすべて読む
function ReadPost()
  local post, s
  if mg.request_info.request_method=='POST' then
    post=''
    repeat
      s=mg.read()
      post=post..(s or '')
    until not s
    if #post~=mg.request_info.content_length then
      post=nil
    end
  end
  return post
end

--CSRFトークンを取得する
--※このトークンを含んだコンテンツを圧縮する場合はBEAST攻撃に少し気を配る
function CsrfToken()
  return edcb.serverRandom and edcb.serverRandom:sub(1,16) or ''
end

--CSRFトークンを検査する
--※サーバに変更を加える要求(POSTに限らない)を処理する前にこれを呼ぶべき
function AssertCsrf(qs)
  assert(not edcb.serverRandom or mg.get_var(qs,'ctok')==edcb.serverRandom:sub(1,16))
end
