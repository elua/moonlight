local timeout = 500
local d = stm32.dp105      
local r = stm32.sircs
local tmrid = 1
local cdict = {}
local refresh_sec = 2

local function create_default_config()
  local cfile = io.open( "/mmc/pingconf.lua", "wb" )
  cfile:write( 'return { "192.168.1.2", "192.168.1.1", "192.168.1.111", "192.168.1.3" }' )
  cfile:close()
end

local function send_config()
  local f = io.open( "/mmc/ping.htm", "rb" )
  if not f then
    print "Unable to open HTML file"
    return
  end
  wiz.app_web_set( "ping", f, cdict )
  f:close()  
end

local function write_config()
  local f = io.open( "/mmc/pingconf.lua", "wb" )
  if f then
    f:write( string.format( 'return { "%s", "%s", "%s", "%s" }', cdict[ 1 ], cdict[ 2 ], cdict[ 3 ], cdict[ 4 ] ) )
    f:close()
  end
end

local function get_config()
  collectgarbage( 'collect' )
  if not cdict[ 1 ] then
    local cfile = io.open( "/mmc/pingconf.lua", "rb" )
    if not cfile then
      create_default_config()
      cfile = io.open( "/mmc/pingconf.lua", "rb" )
    else
      cfile:close()
    end
    local fconf = assert( loadfile( "/mmc/pingconf.lua" ) )
    cdict = fconf()
  end
end

d.init()   
get_config()
send_config()
while true do
  collectgarbage( 'collect' )
  local ips = {}
  local refcount = 0
  d.clrscr( 0 )
  
  for i = 1, 4 do
    ips[ i ] = wiz.ip( cdict[ i ] )
  end
  
  for i = 1, 4 do
    if ips[ i ] ~= wiz.INVALID_IP then
      local replies = wiz.ping( ips[ i ], 4, timeout )  
      local pstr = replies > 0 and string.format( "%d%%", ( replies * 100 ) / 4 ) or "ERR"
      d.putstr( 0, ( i - 1 ) * 8, wiz.unpackip( ips[ i ], "*s" ) )
      d.putstr( 128 - ( d.get_text_extent( pstr ) ), ( i - 1 ) * 8, pstr )  
    end
  end
  d.draw()
  
  -- Wait until the next update
  while refcount < refresh_sec * 25 do
    tmr.delay( tmrid, 40000 )  
    refcount = refcount + 1 
    local dummy, code = r.getkey( false )
    if code == r.KEY_STOP then
      refresh_sec = 0
      break
    elseif code == r.KEY_PLAY_PAUSE then 
      break 
    end   
    if wiz.app_has_cgi() then
      local pdict = wiz.app_get_cgi()
      cdict[ 1 ], cdict[ 2 ], cdict[ 3 ], cdict[ 4 ] = pdict.ip1, pdict.ip2, pdict.ip3, pdict.ip4
      send_config()
      write_config()
      get_config()
      break
    end          
  end  
  
  if refresh_sec == 0 then break end    
end

d.clrscr( 0 )
d.draw()


