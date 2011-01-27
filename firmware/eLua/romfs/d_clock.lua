local d = stm32.dp105
local r = stm32.sircs
local tmrid = 1
local server = "129.6.15.29"
local s
local ip
local delta = 2208988800 -- conversion from NIST epoch to Unix epoch
local h_delta = 3600 -- hour delta
local gmt_delta = 3 
local t, ts
local total_time
local resync_time = false
local f = d.load_font( "/rom/h14_fnt.bin" )
local sync_timer_indication = -1
local tx, ty = 30, 15
local cdict = {}

local function create_default_config()
  local cfile = io.open( "/mmc/clk_conf.lua", "wb" )
  cfile:write( 'return { "129.6.15.29", "3" }' )
  cfile:close()
end

local function send_config()
  local f = io.open( "/mmc/clock.htm", "rb" )
  if not f then
    print "Unable to open HTML file"
    return
  end
  wiz.app_web_set( "clock", f, cdict )
  f:close()  
end

local function write_config()
  local f = io.open( "/mmc/clk_conf.lua", "wb" )
  if f then
    f:write( string.format( 'return { "%s", "%s" }', cdict[ 1 ], cdict[ 2 ] ) )
    f:close()
  end
end

local function get_config()
  collectgarbage( 'collect' )
  if not cdict[ 1 ] then
    local cfile = io.open( "/mmc/clk_conf.lua", "rb" )
    if not cfile then
      create_default_config()
      cfile = io.open( "/mmc/clk_conf.lua", "rb" )
    else
      cfile:close()
    end
    local fconf = assert( loadfile( "/mmc/clk_conf.lua" ) )
    cdict = fconf()
  end
  ip = wiz.ip( cdict[ 1 ] )
  gmt_delta = tonumber( cdict[ 2 ] )
  print( wiz.unpackip( ip, "*s" ), gmt_delta ) 
end

local function acquire_time( timeout )
  wiz.sendto( s, ip, 37, "A" )  
  local datestr = wiz.recvfrom( s, 4, timeout )
  if #datestr > 0 then
    local dummy, v = pack.unpack( datestr, ">L" )
    return v - delta + h_delta * gmt_delta
  end
end

d.clrscr( 0 )
get_config()
send_config()

-- Initial time acquire
for i = 1, 5 do
  s = wiz.socket( wiz.SOCK_UDP )
  t = acquire_time( 500 )
  if t then break end
  wiz.close( s )
end
if not t then
  wiz.close( s )
  d.putstr( 0, 0, "CANNOT READ TIME" );
  d.putstr( 0, 8, "FROM SERVER" );
  d.draw()
  r.getkey( true )
  d.clrscr( 0 )
  d.draw()
  return
end

total_time = 0
while true do
  ts = tmr.start( tmrid )
  
  -- Check remote key
  local dummy, code = r.getkey( false )
  if code then break end 
  
  -- Read configuration
  if wiz.app_has_cgi() then
    print "CGI!"
    local pdict = wiz.app_get_cgi()
    print "AFTER CGI"
    cdict[ 1 ], cdict[ 2 ] = pdict.server, pdict.delta
    send_config()
    write_config()
    get_config()
    total_time = 5 * 60 - 1
  end  
  
  -- Show time
  d.filled_rectangle( tx, ty, tx + 80, ty + 16, 0 )
  d.set_font( f )  
  d.putstr( tx, ty, elua.strftime( "%H:%M:%S", t ), 1 )
  
  -- Show date
  d.set_font( d.FONT_5x5_ROUND )
  d.filled_rectangle( 0, 0, 127, 8, 0 )
  local datestr = elua.strftime("%m/%d/%Y", t )
  local w = d.get_text_extent( datestr )
  d.putstr( 128 - w, 0, datestr, 1 )
  datestr = elua.strftime( "%A", t )
  d.putstr( 0, 0, datestr, 1 )
  d.draw() 
  
  -- Increment time
  t = t + 1
  total_time = total_time + 1
  -- Check if we need to acquire time from the network again
  if total_time == 5 * 60 then
    total_time = 0
    resync_time = true
  end
  if resync_time then
    local temp = acquire_time( wiz.NO_TIMEOUT )
    if temp then
      t = temp
      resync_time = false
      d.filled_rectangle( 126, 30, 127, 31, 1 )
      d.draw()
      sync_timer_indication = 2
    end
  end
  if sync_timer_indication >= 0 then
    if sync_timer_indication == 0 then
      d.filled_rectangle( 126, 30, 127, 31, 0 )
      d.draw()
    end
    sync_timer_indication = sync_timer_indication - 1
  end
  while tmr.gettimediff( tmrid, tmr.read( tmrid ), ts ) < 1000000 do end  
end

wiz.close( s )
d.clrscr( 0 )
d.draw()
