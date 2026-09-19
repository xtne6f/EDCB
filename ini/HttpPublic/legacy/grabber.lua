-- TSファイルの指定位置のIフレームを取得するスクリプト
dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

fpath=mg.get_var(mg.request_info.query_string,'fname')
if fpath then
  fpath=DocumentToNativePath(fpath)
end
ofssec=GetVarInt(mg.request_info.query_string,'ofssec',0,100000) or 0

stream=nil
if fpath then
  ext=fpath:match('%.[0-9A-Za-z]+$') or ''
  extts=edcb.GetPrivateProfile('SET','TSExt','.ts','EpgTimerSrv.ini')
  -- 拡張子を限定
  if IsEqualPath(ext,extts) then
    f=edcb.io.open(fpath,'rb')
    if f then
      if ofssec==0 then
        stream=GetIFrameVideoStream(f)
      else
        -- 時間シーク
        fsec,fsize=GetDurationSec(f)
        if fsec>=ofssec then
          SeekSec(f,ofssec,fsec,fsize)
          stream=GetIFrameVideoStream(f)
        end
      end
      f:close()
    end
  end
end

if not stream then
  mg.write(Response(404,nil,nil,0)..'\r\n')
elseif mg.request_info.request_method=='HEAD' then
  mg.write(Response(200,'application/octet-stream',nil,0)..'\r\n')
else
  mg.write(Response(200,'application/octet-stream',nil,#stream)..'\r\n')
  mg.write(stream)
end
