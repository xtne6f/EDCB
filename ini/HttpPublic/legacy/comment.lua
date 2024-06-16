-- 外部プロセスの機能を利用してコメントを投稿する
dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

query=AssertPost()

n=GetVarInt(query,'n') or 0
onid,tsid,sid=GetVarServiceID(query,'id')
if onid==0 and tsid==0 and sid==0 then
  onid=nil
end
comm=mg.get_var(query,'comm') or ''

pid=nil
if onid then
  if 0<=n and n<100 then
    ok,pid=edcb.IsOpenNetworkTV(n)
  end
elseif 0<=n and n<=65535 then
  ff=edcb.FindFile(SendTSTCPPipePath(n..'_*',0),1)
  if ff then
    pid=ff[1].name:match('^[^_]+_%d+_(%d+)')
  end
end

code=404
if pid then
  code=406
  comm=comm:match('^[^\n\r]*')
  if not comm:find('^%[') then
    comm='[]'..comm
  end
  -- 空コメントを除外
  if comm:match('^%[.-%](.)') then
    code=405
    f=edcb.io.open('\\\\.\\pipe\\post_d7b64ac2_'..pid,'w')
    if f then
      code=500
      if f:write(comm) then
        code=200
        for i=0,300 do
          -- 確実に書き込むために切断されるまで改行しつづける
          if not f:write('\n') or not f:flush() then break end
          edcb.Sleep(10)
        end
      end
      f:close()
    end
  end
end
mg.write(Response(code,nil,nil,0)..'\r\n')
