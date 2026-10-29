-- 外部プロセスの機能を利用してコメントを投稿する
dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

query=AssertPost()

n=GetVarInt(query,'n') or 0
onid,tsid,sid=GetVarServiceID(query,'id')
if onid==0 and tsid==0 and sid==0 then
  onid=nil
end
refuge=GetVarInt(query,'refuge')==1
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
  mail,comm=comm:match('^(%[.-)(%].+)')
  if mail then
    -- 投稿先を明記
    comm=mail..(refuge and ' refuge' or ' nico')..comm
    code=405
    if JKCNSL_PATH then
      postName='jkcnsl_edcb_'..pid
      if WIN32 then
        fpost=edcb.io.open('\\\\.\\pipe\\'..postName,'w')
      else
        for retry=1,9 do
          fpost=EdcbFindFilePlain(PathAppend(JKCNSL_UNIX_BASE_DIR,postName)) and
            edcb.io.open(PathAppend(JKCNSL_UNIX_BASE_DIR,postName),'a')
          -- 同時に書き込まないようプロセス間でロックが必要
          if not fpost or edcb.io._flock_nb(fpost) then break end
          fpost:close()
          fpost=nil
          edcb.Sleep(10*retry)
        end
      end
    else
      if WIN32 then
        fpost=edcb.io.open('\\\\.\\pipe\\post_d7b64ac2_'..pid,'w')
      else
        for retry=1,9 do
          fpost=EdcbFindFilePlain(PathAppend(JKTASK_BASE_DIR,'post_'..pid..'.fifo')) and
            edcb.io.open(PathAppend(JKTASK_BASE_DIR,'post_'..pid..'.fifo'),'a')
          -- 同時に書き込まないようプロセス間でロックが必要
          if not fpost or edcb.io._flock_nb(fpost) then break end
          fpost:close()
          fpost=nil
          edcb.Sleep(10*retry)
        end
      end
    end
    if fpost then
      code=500
      if fpost:write((JKCNSL_PATH and '+' or '')..comm..'\n') and fpost:flush() then
        code=200
        for i=0,300 do
          -- 確実に書き込むために切断されるまで改行しつづける
          if not WIN32 or not fpost:write('\n') or not fpost:flush() then break end
          edcb.Sleep(10)
        end
      end
      fpost:close()
    end
  end
end
mg.write(Response(code,nil,nil,0)..'\r\n')
