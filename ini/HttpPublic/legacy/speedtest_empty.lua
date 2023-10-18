dofile(mg.script_name:gsub('[^\\/]*$','')..'util.lua')

if mg.request_info.request_method=='POST' then
  n=0
  repeat
    s=mg.read()
    n=n+(s and #s or 0)
    assert(n<1024*1024*1024)
  until not s
end
mg.write(Response(200,nil,nil,0)..'\r\n')
