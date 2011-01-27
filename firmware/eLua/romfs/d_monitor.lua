local mscr = require "mscr"

local d = stm32.dp105
local r = stm32.sircs
local tmrid = 1
local nprops = 16
local sock, ip, port
local cdict = {}
local running, ntries = false, 0 
local nlines = 4

d.init()
local scr = {}
scr[ 1 ] = mscr.new( { "MEMORY: ", "CPU: ", "HDD C: ", "PROCS: " } )                
scr[ 1 ]:draw()
scr[ 2 ] = mscr.new( { "NAME: ", "USER: ", "OS: ", "IP: " } )
scr[ 3 ] = mscr.new( { "CPU: ", "FREQ: ", "CORES: ", "TIME: " } )

local function create_default_config()
  local cfile = io.open( "/mmc/mon_conf.lua", "wb" )
  cfile:write( 'return { "192.168.1.2", "25000" }' )
  cfile:close()
end

local function send_config()
  local f = io.open( "/mmc/monitor.htm", "rb" )
  if not f then
    print "Unable to open HTML file"
    return
  end
  wiz.app_web_set( "pcmonitor", f, cdict )
  f:close()  
end

local function write_config()
  local f = io.open( "/mmc/mon_conf.lua", "wb" )
  if f then
    f:write( string.format( 'return { "%s", "%s" }', cdict[ 1 ], cdict[ 2 ] ) )
    f:close()
  end
end

local function get_config()
  collectgarbage( 'collect' )
  if not cdict[ 1 ] then
    local cfile = io.open( "/mmc/mon_conf.lua", "rb" )
    if not cfile then
      create_default_config()
      cfile = io.open( "/mmc/mon_conf.lua", "rb" )
    else
      cfile:close()
    end
    local fconf = assert( loadfile( "/mmc/mon_conf.lua" ) )
    cdict = fconf()
  end
  ip = wiz.ip( cdict[ 1 ] )
  port = tonumber( cdict[ 2 ] )
  print( wiz.unpackip( ip, "*s" ), port ) 
end

sock = wiz.socket( wiz.SOCK_UDP, 31000 )
local crtscr, newscr, totalscr, ttime = 1, 1, 3, tmr.start( tmrid )    
local initialized = false
get_config()
send_config()
while true do
  if wiz.app_has_cgi() then
    print "CGI!"
    local pdict = wiz.app_get_cgi()
    print "AFTER CGI"
    cdict[ 1 ], cdict[ 2 ] = pdict.server, pdict.port
    initialized = false 
    running = false
    ntries = 0
    send_config()
    write_config()
  end
  
  if not initialized then                       
    if ntries < 3 then
      get_config()  
      while #( wiz.recvfrom( sock, 128, wiz.NO_TIMEOUT ) ) > 0 do end      
      if wiz.sendto( sock, ip, port, 0xA0, 300 ) ~= 1 then
        ntries = ntries + 1
      else
        running = true
        initialized = true                
      end
    else
      if ntries == 3 then
        for i = 1, totalscr do
          for j = 1, nlines do
            scr[ i ]:settext( j, "N/A" ) 
          end
          if i == crtscr then scr[ i ]:draw() end
        end
      end
      ntries = ntries + 1
    end      
  end
  
  -- Check for remote key
  local dummy, code = r.getkey( false )
  if code == r.KEY_STOP then break end
  if code then
    newscr = crtscr
    if code == r.KEY_PREV then
      newscr = crtscr == 1 and totalscr or crtscr - 1
    elseif code == r.KEY_NEXT then
      newscr = crtscr == totalscr and 1 or crtscr + 1
    end
    if newscr ~= crtscr then
      scr[ newscr ]:reset()
      scr[ crtscr ]:reset()
      for i = 0, 32 do
        d.clrscr( 0 )
        if code == r.KEY_NEXT then
          scr[ crtscr ]:draw( -i )
          scr[ newscr ]:draw( 32 - i )
        else
          scr[ crtscr ]:draw( i )
          scr[ newscr ]:draw( i - 32 )        
        end
        d.draw()
        tmr.delay( tmrid, 20000 )
      end  
      crtscr = newscr
      ttime = tmr.start( tmrid ) 
      scr[ crtscr ]:draw()   
    end 
  end
  
  if running then 
    -- Check for serial data
    local data, server = wiz.recvfrom( sock, 128, wiz.NO_TIMEOUT )
    local c = ( #data > 0 and server == ip ) and data:byte( 1 ) or nil
    if c and c >= 0xA0 then
      local newdata = data:sub( 2 )
      local dataid = c - 0xA0
      local datascr, lineid = bit.rshift( dataid, 2 ) + 1, bit.band( dataid, 3 ) + 1
      if datascr <= totalscr and lineid <= nlines then
        scr[ datascr ]:settext( lineid, newdata )
      end 
      if datascr == crtscr then scr[ crtscr ]:draw() end
    end 
        
    -- Scroll data if needed
    if tmr.gettimediff( tmrid, ttime, tmr.read( tmrid ) ) >= 40000 then
      scr[ crtscr ]:on_timer()
      ttime = tmr.start( tmrid )    
    end
  end
end

d.clrscr( 0 )
d.draw()
for i = 1, 3 do scr[ i ] = nil end
scr = nil
mscr = nil
wiz.close( sock )
wiz.app_web_unload()
