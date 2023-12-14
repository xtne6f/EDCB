-- 局ロゴを転送するスクリプト

dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

-- EDCBのロゴフォルダにロゴがないときに検索する、LogoData.iniとLogoフォルダがあるフォルダの絶対パス
-- 未指定のときは公開フォルダ下の"img/logo/ONIDSID{.png|.bmp}"が使われる
--LOGO_DIR=EdcbModulePath()..'\\..\\TVTest'

onid=GetVarInt(mg.request_info.query_string,'onid',0,65535) or 0
sid=GetVarInt(mg.request_info.query_string,'sid',0,65535) or 0

-- ロゴ識別とServiceIDとの対応を調べる
ddid=tonumber(edcb.GetPrivateProfile('LogoIDMap',('%04X%04X'):format(onid,sid),'','Setting\\LogoData.ini'))
if ddid then
  dir=EdcbSettingPath()..'\\LogoData\\'
  ff=edcb.FindFile(dir..('%04X_%03X_*'):format(onid,ddid),6) or {}
  -- ファイル名の末尾2桁はロゴタイプ(STD-B21)
  for i,v in ipairs({'05%.png','02%.png','04%.png','01%.png','03%.png','00%.png'}) do
    for j,w in ipairs(ff) do
      if w.name:lower():find(v..'$') then
        fname=w.name
        break
      end
    end
    if fname then
      f=edcb.io.open(dir..fname,'rb')
      if f then
        logo=f:read('*a')
        f:close()
      end
      break
    end
  end
end

if not logo and LOGO_DIR then
  fname=nil
  f=edcb.io.open(LOGO_DIR..'\\LogoData.ini','rb')
  if f then
    -- ロゴ識別とServiceIDとの対応を調べる
    ddid=tonumber(f:read('*a'):upper():match(('\n%04X%04X=(%%d+)'):format(onid,sid)))
    f:close()
    if ddid then
      ff=edcb.FindFile(LOGO_DIR..('\\Logo\\%04X_%03X_*'):format(onid,ddid),12) or {}
      -- ファイル名の末尾2桁はロゴタイプ(STD-B21)であると期待
      for i,v in ipairs({'05%.png','05%.bmp','02%.png','02%.bmp','04%.png','04%.bmp','01%.png','01%.bmp','03%.png','03%.bmp','00%.png','00%.bmp'}) do
        for j,w in ipairs(ff) do
          if w.name:lower():find(v..'$') then
            fname=w.name
            break
          end
        end
        if fname then
          f=edcb.io.open(LOGO_DIR..'\\Logo\\'..fname,'rb')
          if f then
            logo=f:read('*a')
            f:close()
          end
          break
        end
      end
    end
  end
elseif not logo then
  fname=('%04X%04X.png'):format(onid,sid)
  f=edcb.io.open(DocumentToNativePath('img/logo/'..fname),'rb')
  if not f then
    fname=('%04X%04X.bmp'):format(onid,sid)
    f=edcb.io.open(DocumentToNativePath('img/logo/'..fname),'rb')
  end
  if f then
    logo=f:read('*a')
    f:close()
  end
end

ct=CreateContentBuilder()
if logo then
  ct:Append(logo)
  ct:Finish()
  mg.write(ct:Pop(Response(200,mg.get_mime_type(fname),nil,ct.len,3600)..'ETag: '..mg.md5(logo)..'\r\n\r\n'))
else
  -- 1x1gif
  ct:Append('GIF89a\1\0\1\0\x80\0\0\0\0\0\xFF\xFF\xFF\x21\xF9\4\1\0\0\0\0\x2C\0\0\0\0\1\0\1\0\0\2\1\x44\0\x3B')
  ct:Finish()
  mg.write(ct:Pop(Response(200,'image/gif',nil,ct.len,3600)..'ETag: 0\r\n\r\n'))
end
