--このサイズ以上のときページ圧縮する(nilのとき常に非圧縮)
GZIP_THRESHOLD_BYTE=4096

--EPG情報をTextに変換(EpgTimerUtil.cppから移植)
function ConvertEpgInfoText2(onidOrEpg, tsid, sid, eid)
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
  local s='録画モード: <select name="recMode">'
  for i=1,#RecModeTextList() do
    s=s..'<option value="'..(i-1)..'"'..(rs.recMode==i-1 and ' selected' or '')..'>'..RecModeTextList()[i]
  end
  s=s..'</select><br>\n'
    ..'イベントリレー追従: <select name="tuijyuuFlag">'
    ..'<option value="0"'..(not rs.tuijyuuFlag and ' selected' or '')..'>しない'
    ..'<option value="1"'..(rs.tuijyuuFlag and ' selected' or '')..'>する</select><br>\n'
    ..'優先度: <select name="priority">'
  for i=1,5 do
    s=s..'<option value="'..i..'"'..(rs.priority==i and ' selected' or '')..'>'..i
  end
  --デフォルト値
  local rsdef=(edcb.GetReserveData(0x7FFFFFFF) or {}).recSetting
  s=s..'</select><br>\n'
    ..'ぴったり（？）録画: <select name="pittariFlag">'
    ..'<option value="0"'..(not rs.pittariFlag and ' selected' or '')..'>しない'
    ..'<option value="1"'..(rs.pittariFlag and ' selected' or '')..'>する</select><br>\n'
    ..'録画マージン: <input type="checkbox" name="useDefMarginFlag" value="1"'..(rs.startMargin and '' or ' checked')..'>デフォルト || '
    ..'開始（秒） <input type="text" name="startMargin" value="'..(rs.startMargin or rsdef and rsdef.startMargin or 0)..'" size="5"> '
    ..'終了（秒） <input type="text" name="endMargin" value="'..(rs.endMargin or rsdef and rsdef.endMargin or 0)..'" size="5"><br>\n'
    ..'指定サービス対象データ: <input type="checkbox" name="serviceMode" value="1"'..(rs.serviceMode%2==0 and ' checked' or '')..'>デフォルト || '
    ..'<input type="checkbox" name="serviceMode_1" value="1"'
      ..(math.floor(rs.serviceMode%2~=0 and rs.serviceMode/16 or rsdef and rsdef.serviceMode/16 or 0)%2~=0 and ' checked' or '')..'>字幕を含める '
    ..'<input type="checkbox" name="serviceMode_2" value="1"'
      ..(math.floor(rs.serviceMode%2~=0 and rs.serviceMode/32 or rsdef and rsdef.serviceMode/32 or 0)%2~=0 and ' checked' or '')..'>データカルーセルを含める<br>\n'
    ..'<table><tr><td>録画フォルダ</td><td>出力PlugIn</td><td>ファイル名PlugIn</td><td>部分受信</td></tr>\n'
  for i,v in ipairs(rs.recFolderList) do
    s=s..'<tr><td>'..v.recFolder..'</td><td>'..v.writePlugIn..'</td><td>'..v.recNamePlugIn..'</td><td>いいえ</td></tr>\n'
  end
  for i,v in ipairs(rs.partialRecFolder) do
    s=s..'<tr><td>'..v.recFolder..'</td><td>'..v.writePlugIn..'</td><td>'..v.recNamePlugIn..'</td><td>はい</td></tr>\n'
  end
  s=s..'</table>（プリセットによる変更のみ対応）<br>\n'
    ..'<input type="checkbox" name="partialRecFlag" value="1"'..(rs.partialRecFlag~=0 and ' checked' or '')..'>部分受信（ワンセグ）を別ファイルに同時出力する<br>\n'
    ..'<input type="checkbox" name="continueRecFlag" value="1"'..(rs.continueRecFlag and ' checked' or '')..'>後ろの予約を同一ファイルで出力する<br>\n'
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
    ..'<input type="checkbox" name="rebootFlag" value="1"'
      ..((rs.suspendMode==0 and rsdef and rsdef.rebootFlag or rs.suspendMode~=0 and rs.rebootFlag) and ' checked' or '')..'>復帰後再起動する<br>\n'
    ..'録画後実行bat（プリセットによる変更のみ対応）: '..(#rs.batFilePath==0 and '（なし）' or rs.batFilePath)..'<br>\n'
  return s
end

function RecModeTextList()
  return {'全サービス','指定サービス','全サービス（デコード処理なし）','指定サービス（デコード処理なし）','視聴','無効'}
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
    end
  end
  return 0,fsize
end

--ファイルの先頭からsec秒だけシークする
function SeekSec(f,sec)
  local dur,fsize=GetDurationSec(f)
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

--レスポンスを生成する
function Response(code,ctype,charset,cl)
  return 'HTTP/1.1 '..code..' '..mg.get_response_code_text(code)
    ..'\r\nX-Frame-Options: SAMEORIGIN'
    ..(ctype and '\r\nX-Content-Type-Options: nosniff\r\nContent-Type: '..ctype..(charset and '; charset='..charset or '') or '')
    ..(cl and '\r\nContent-Length: '..cl or '')
    ..(mg.keep_alive(not not cl) and '\r\n' or '\r\nConnection: close\r\n')
end

--コンテンツを連結するオブジェクトを生成する
function CreateContentBuilder(thresh)
  local self={ct={''},len=0,thresh_=thresh}
  function self:Append(s)
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
  function self:Finish()
    if self.gzip and self.stream_ then
      self.ct[#self.ct+1]=self.stream_()
      self.len=self.len+#self.ct[#self.ct]
    end
    self.stream_=nil
  end
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

--CSRFトークンを取得する
--※このトークンを含んだコンテンツを圧縮する場合はBREACH攻撃に少し気を配る
function CsrfToken(t)
  --メッセージに時刻をつける
  local m='legacy:'..(math.floor(os.time()/3600/12)+(t or 0))
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
  assert(mg.get_var(qs,'ctok')==CsrfToken() or mg.get_var(qs,'ctok')==CsrfToken(-1))
end
