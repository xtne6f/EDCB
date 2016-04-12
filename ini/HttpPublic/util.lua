--EPG情報をTextに変換(EpgTimerUtil.cppから移植)
function _ConvertEpgInfoText2(onid, tsid, sid, eid)
  local s, v = '', edcb.SearchEpg(onid, tsid, sid, eid)
  if v then
    s=s..(v.startTime and os.date('%Y/%m/%d(%a) %H:%M～', os.time(v.startTime))
      ..(v.durationSecond and os.date('%H:%M', os.time(v.startTime)+v.durationSecond) or '未定') or '未定')..'\n'
    for i,w in ipairs(edcb.GetServiceList() or {}) do
      if w.onid==v.onid and w.tsid==v.tsid and w.sid==v.sid then
        s=s..w.service_name
        break
      end
    end
    s=s..'\n'
    if v.shortInfo then
      s=s..v.shortInfo.event_name..'\n\n'..v.shortInfo.text_char..'\n\n'
    end
    if v.extInfo then
      s=s..'詳細情報\n'..v.extInfo.text_char..'\n\n'
    end
    if v.contentInfoList then
      s=s..'ジャンル : \n'
      for i,w in ipairs(v.contentInfoList) do
        s=s..edcb.GetGenreName(math.floor(w.content_nibble/256)*256+255)..' - '..edcb.GetGenreName(w.content_nibble)..'\n'
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
    ..'追従: <select name="tuijyuuFlag">'
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
    ..'録画後動作: <select name="suspendMode">'
    ..'<option value="0"'..(rs.suspendMode==0 and ' selected="selected"' or '')..'>デフォルト設定を使用'
    ..'<option value="1"'..(rs.suspendMode==1 and ' selected="selected"' or '')..'>スタンバイ'
    ..'<option value="2"'..(rs.suspendMode==2 and ' selected="selected"' or '')..'>休止'
    ..'<option value="3"'..(rs.suspendMode==3 and ' selected="selected"' or '')..'>シャットダウン'
    ..'<option value="4"'..(rs.suspendMode==4 and ' selected="selected"' or '')..'>何もしない</select>\n'
    ..'<input type="checkbox" name="rebootFlag" value="1"'..(rs.rebootFlag and ' checked="checked"' or '')..'>復帰後再起動する<br>\n'
    ..'録画マージン: <input type="checkbox" name="useDefMarginFlag" value="1"'..(rs.startMargin and '' or ' checked="checked"')..'>デフォルト設定で使用\n'
    ..'開始<input type="text" name="startMargin" value="'..(rs.startMargin or 0)..'">\n'
    ..'終了<input type="text" name="endMargin" value="'..(rs.endMargin or 0)..'"><br>\n'
    ..'指定サービス対象データ: '
    ..'<input type="checkbox" name="serviceMode" value="0"'..(rs.serviceMode%2==0 and ' checked="checked"' or '')..'>デフォルト設定で使用'
    ..'<input type="checkbox" name="serviceMode_1" value="0"'..(rs.serviceMode%2~=0 and math.floor(rs.serviceMode/16)%2~=0 and ' checked="checked"' or '')..'>字幕データを含める'
    ..'<input type="checkbox" name="serviceMode_2" value="0"'..(rs.serviceMode%2~=0 and math.floor(rs.serviceMode/32)%2~=0 and ' checked="checked"' or '')..'>データカルーセルを含める<br>\n'
    ..'連続録画動作: '
    ..'<input type="checkbox" name="continueRecFlag" value="1"'..(rs.continueRecFlag and ' checked="checked"' or '')..'>後ろの予約を同一ファイルで出力する<br>\n'
    ..'使用チューナー強制指定: <select name="tunerID"><option value="0"'..(rs.tunerID==0 and ' selected="selected"' or '')..'>自動'
  local a=edcb.GetTunerReserveAll()
  for i=1,#a-1 do
    s=s..'<option value="'..a[i].tunerID..'"'..(a[i].tunerID==rs.tunerID and ' selected="selected"' or '')..string.format('>ID:%08X(', a[i].tunerID)..a[i].tunerName..')'
  end
  s=s..'</select><br>\n'
    ..'録画後実行bat（プリセットによる変更のみ対応）: '..rs.batFilePath..'<br>\n'
    ..'録画フォルダ、使用プラグイン（プリセットによる変更のみ対応）:<table border="1">'
    ..'<tr><td>フォルダ</td><td>出力PlugIn</td><td>ファイル名PlugIn</td></tr>\n'
  for i,v in ipairs(rs.recFolderList) do
    s=s..'<tr><td>'..v.recFolder..'</td><td>'..v.writePlugIn..'</td><td>'..v.recNamePlugIn..'</td></tr>\n'
  end
  s=s..'</table>'
    ..'部分受信サービス:<br><input type="checkbox" name="partialRecFlag" value="1"'..(rs.partialRecFlag~=0 and ' checked="checked"' or '')..'>別ファイルに同時出力する<br>\n'
    ..'録画フォルダ、使用プラグイン（プリセットによる変更のみ対応）:<table border="1">'
    ..'<tr><td>フォルダ</td><td>出力PlugIn</td><td>ファイル名PlugIn</td></tr>\n'
  for i,v in ipairs(rs.partialRecFolder) do
    s=s..'<tr><td>'..v.recFolder..'</td><td>'..v.writePlugIn..'</td><td>'..v.recNamePlugIn..'</td></tr>\n'
  end
  s=s..'</table><br>\n'
  return s
end

--可能ならコンテンツをzlib圧縮する(lua-zlib(zlib.dll)が必要)
function Deflate(ct)
  local zl
  for k,v in pairs(mg.request_info.http_headers) do
    if k:match('^[Aa]ccept%-[Ee]ncoding$') and v:find('deflate') then
      local status, zlib = pcall(require, 'zlib')
      if status then
        zl=zlib.deflate()(ct, 'finish')
      end
    elseif k:match('^[Uu]ser%-[Aa]gent$') and (v:find(' MSIE ') or v:find(' Trident/7%.')) then
      --IEのdeflate対応は腐っているので弾く
      return nil
    end
  end
  return zl
end
