require "mscr"

local s
local timeout = 2000
local url, fields
local d = stm32.dp105      
local r = stm32.sircs
local tmrid = 1
local refresh_sec
local cdict = {}

local function create_default_config()
  local cfile = io.open( "/mmc/pchbconf.lua", "wb" )
  cfile:write( 'return { "http://www.pachube.com/api/feeds/6797.xml", "29399b301c18078b3914cb0e2444660d4410366af8e933b99e937e24dae4f01d", "0,1,2", "120" }' )
  cfile:close()
end

local function send_config()
  local f = io.open( "/mmc/pachube.htm", "rb" )
  if not f then
    print "Unable to open HTML file"
    return
  end
  local confdict = {}
  wiz.app_web_set( "pachube", f, cdict )
  f:close()  
end

local function write_config()
  local f = io.open( "/mmc/pchbconf.lua", "wb" )
  if f then
    f:write( string.format( 'return { "%s", "%s", "%s", "%s" }', cdict[ 1 ], cdict[ 2 ], cdict[ 3 ], cdict[ 4 ] ) )
    f:close()
  end
end

local function get_config()
  collectgarbage( 'collect' )
  if not cdict[ 1 ] then
    local cfile = io.open( "/mmc/pchbconf.lua", "rb" )
    if not cfile then
      create_default_config()
      cfile = io.open( "/mmc/pchbconf.lua", "rb" )
    else
      cfile:close()
    end
    local fconf = assert( loadfile( "/mmc/pchbconf.lua" ) )
    cdict = fconf()
  end
  url = string.format( "%s?key=%s", cdict[ 1 ], cdict[ 2 ] )
  fields = {}
  refresh_sec = tonumber( cdict[ 4 ] )
  for f in cdict[ 3 ]:gmatch( "%d+" ) do
    fields[ #fields + 1 ] = f
  end
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
    
    -- Get environment title
    wiz.expect( s, "<title>", timeout )
    temp = wiz.expect_and_read( s, "</title>", timeout )
    if not temp then
      errflag = true
      break
    end
    scr:settext( 1, temp )
    
    -- Get all data
    for k, v in ipairs( fields ) do
      if k > 3 then break end
      if wiz.expect( s, string.format('<data id="%d">', v), timeout ) == false then
        errflag = true
        break
      end  
      -- Get tag, value and unit
      wiz.expect( s, "<tag>" )
      temp1 = wiz.expect_and_read( s, "</tag>" )
      wiz.expect( s, "<value" )
      wiz.expect( s, ">" )
      temp2 = wiz.expect_and_read( s, "</value>" ) 
      wiz.expect( s, "<unit" )
      temp3 = wiz.expect_and_read( s, "</data>" )
      if temp3:match( "/>" ) then
        temp3 = temp3:match( 'symbol="(.-)"')
      elseif temp3:match("</unit>" ) then
        temp3 = temp3:match( '>(.-)</unit>' ) 
      else 
        temp3 = "N/A"
      end
      scr:settext( 1 + k, string.format( '%s: %s (%s)', temp1, temp2, temp3 ) )
    end
    scr:draw()
    wiz.close(s)
    
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
        cdict[ 1 ], cdict[ 2 ], cdict[ 3 ], cdict[ 4 ] = pdict.url, pdict.apikey, pdict.fields, pdict.refresh
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
