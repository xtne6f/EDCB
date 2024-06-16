package.path=mg.script_name:gsub('[^\\/]*$','')..'?.lua'
mg.write('HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n',
  require('uuid4').getUUID():lower(), '\n# This is "PSEUDO"random UUID, see https://github.com/tcjennings/LUA-RFC-4122-UUID-Generator\n')
