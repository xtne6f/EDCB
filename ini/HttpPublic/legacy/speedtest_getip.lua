dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

ct=CreateContentBuilder()
ct:Append('{"processedString":"'..mg.request_info.remote_addr..'"}')
ct:Finish()
mg.write(ct:Pop(Response(200,'application/json','utf-8',ct.len)..'\r\n'))
