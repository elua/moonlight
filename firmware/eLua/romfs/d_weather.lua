require "mscr"

local s
local timeout = 2000
local url
local d = stm32.dp105      
local r = stm32.sircs
local tmrid = 1
local refresh_sec
local cdict = {}

local function create_default_config()
  local cfile = io.open( "/mmc/wth_conf.lua", "wb" )
  cfile:write( 'return { "868274", "c", "600" }' )
  cfile:close()
end

local function send_config()
  local f = io.open( "/mmc/weather.htm", "rb" )
  if not f then
    print "Unable to open HTML file"
    return
  end
  local confdict = {}
  confdict[ 1 ] = cdict[ 1 ]
  confdict[ 2 ] = cdict[ 2 ] == "c" and "selected" or ""
  confdict[ 3 ] = cdict[ 2 ] == "f" and "selected" or "" 
  confdict[ 4 ] = cdict[ 3 ] 
  wiz.app_web_set( "weather", f, confdict )
  f:close()  
end

local function write_config()
  local f = io.open( "/mmc/wth_conf.lua", "wb" )
  if f then
    f:write( string.format( 'return { "%s", "%s", "%s" }', cdict[ 1 ], cdict[ 2 ], cdict[ 3 ] ) )
    f:close()
  end
end

local function get_config()
  collectgarbage( 'collect' )
  if not cdict[ 1 ] then
    local cfile = io.open( "/mmc/wth_conf.lua", "rb" )
    if not cfile then
      create_default_config()
      cfile = io.open( "/mmc/wth_conf.lua", "rb" )
    else
      cfile:close()
    end
    local fconf = assert( loadfile( "/mmc/wth_conf.lua" ) )
    cdict = fconf()
  end
  url = string.format( "http://weather.yahooapis.com/forecastrss?w=%s&u=%s", cdict[ 1 ], cdict[ 2 ] )
  refresh_sec = tonumber( cdict[ 3 ] )
end

d.init()
local scr = mscr.new( { "", "", "", "" } )   
get_config()
send_config()
local errflag
while true do
  errflag = false
  if s then wiz.close( s ) end
  while true do
    collectgarbage( 'collect' )
    local tries = 0
    local temp, dummy1, dummy2, temp1, temp2, temp3, temp4
    local units
    local refcount = 0
    d.clrscr( 0 )
    
    -- Try 3 times to connect
    while tries < 3 do
      s = wiz.socket( wiz.SOCK_TCP )
      res = wiz.http_request( s, url, timeout )
      if res ~= 0 then
        print( res )
        wiz.close( s )  
        tries = tries + 1
        tmr.delay( tmrid, 500000 )
      else 
        break 
      end
    end
    if res ~= 0 then
      print "Unable to connect"
      do return end
    end
    
    wiz.expect_start()
    print "Starting iteration..."
    
    -- Get location
    wiz.expect( s, "<yweather:location ", timeout )
    temp = wiz.expect_and_read( s, "/>", timeout )
    if not temp then
      errflag = true
      break
    end
    dummy1, dummy2, temp1, temp2 = temp:find('city="(.-)".*country="(.-)"')
    if not temp1 or not temp2 then
      errflag = true
      break
    end
    scr:settext( 1, string.format( "%s (%s)", temp1, temp2 ) )
    
    -- Get units
    wiz.expect( s, '<yweather:units temperature="', timeout )
    units = wiz.expect_and_read( s, '"', timeout )
    if not units then
      errflag = true
      break
    end
    
    -- Get current conditions
    wiz.expect( s, '<yweather:condition ', timeout )
    temp = wiz.expect_and_read( s, "/>", timeout )
    if not temp then
      errflag = true
      break
    end    
    dummy1, dummy2, temp1, temp2 = temp:find('text="(.-)".*temp="(.-)"')
    scr:settext( 2, string.format( "%s (%s%s)", temp1, temp2, units ) )
    if not temp1 or not temp2 then
      errflag = true
      break
    end    
    
    -- Get conditions for the next day
    for i = 1, 2 do
      wiz.expect( s, '<yweather:forecast ', timeout )
      temp = wiz.expect_and_read( s, "/>", timeout ) 
      if not temp then
        errflag = true
        break
      end    
      dummy1, dummy2, temp1, temp2, temp3, temp4 = temp:find('day="(.-)".*low="(.-)".*high="(.-)".*text="(.-)"')
      scr:settext( 2 + i, string.format( "%s: %s (L: %s%s H: %s%s)", temp1, temp4, temp2, units, temp3, units ) )
      if not temp1 or not temp2 or not temp3 or not temp4 then
        errflag = true
        break
      end    
    end
    if errflag then break end
    scr:draw()
    
    wiz.close( s )
    
    -- Wait until the next update
    while refcount < refresh_sec * 25 do
      tmr.delay( tmrid, 40000 )  
      refcount = refcount + 1 
      scr:on_timer()
      local dummy, code = r.getkey( false )
      if code == r.KEY_STOP then
        refresh_sec = 0
        break
      elseif code == r.KEY_PLAY_PAUSE then 
        break 
      end   
      if wiz.app_has_cgi() then
        local pdict = wiz.app_get_cgi()
        cdict[ 1 ], cdict[ 2 ], cdict[ 3 ] = pdict.woeid, pdict.units, pdict.refresh
        send_config()
        write_config()
        get_config()
        break
      end          
    end  
    
    if refresh_sec == 0 then break end    
  end
  if not errflag then break end
end

d.clrscr( 0 )
d.draw()


