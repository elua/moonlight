require "mscr"

local s
local timeout = 2000
local url
local d = stm32.dp105      
local r = stm32.sircs
local tmrid = 1
local refresh_sec
local cdict = {}
local numlines, maxscreens = 4, 3
local maxtitles = maxscreens * numlines

local function create_default_config()
  local cfile = io.open( "/mmc/rss_conf.lua", "wb" )
  cfile:write( 'return { "http://rss.cnn.com/rss/cnn_world.rss", "600" }' )
  cfile:close()
end

local function send_config()
  local f = io.open( "/mmc/rss.htm", "rb" )
  if not f then
    print "Unable to open HTML file"
    return
  end
  wiz.app_web_set( "RSS", f, cdict )
  f:close()  
end

local function write_config()
  local f = io.open( "/mmc/rss_conf.lua", "wb" )
  if f then
    f:write( string.format( 'return { "%s", "%s" }', cdict[ 1 ], cdict[ 2 ] ) )
    f:close()
  end
end

local function get_config()
  collectgarbage( 'collect' )
  if not cdict[ 1 ] then
    local cfile = io.open( "/mmc/rss_conf.lua", "rb" )
    if not cfile then
      create_default_config()
      cfile = io.open( "/mmc/rss_conf.lua", "rb" )
    else
      cfile:close()
    end
    local fconf = assert( loadfile( "/mmc/rss_conf.lua" ) )
    cdict = fconf()
  end
  url, refresh_sec = cdict[ 1 ], tonumber( cdict[ 2 ] )
end

d.init()
local scr = {}
for i = 1, maxscreens do
  scr[ i ] = mscr.new( { "", "", "", "" } )
end 
get_config()
send_config()
local errflag
while true do
  errflag = false
  if s then wiz.close( s ) end
  while true do
    collectgarbage( 'collect' )
    local tries = 0
    local units
    local refcount = 0
    local totscreens = 0
    local titles = {}
    local crtscr = 0
        
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
      errflag = true
      break
    end
    
    wiz.expect_start()
    print "Starting iteration..."
    
    -- Read titles
    while not errflag and #titles < maxtitles do
      if not wiz.expect( s, "<item>", timeout ) then break  end
      if not wiz.expect( s, "<title>", timeout ) then break end
      local data = wiz.expect_and_read( s, "</title>", timeout )
      if not data then break end        
      if not wiz.expect( s, "</item>", timeout ) then break end
      titles[ #titles + 1 ] = data
    end    
    wiz.close( s )
    if #titles == 0 then break end
    
    -- Compute the total number of screens
    totscreens = math.ceil( ( #titles - 1 ) / maxscreens )
    if totscreens > maxscreens then totscreens = maxscreens end
     
    -- Write titles
    local t = 1
    for s = 1, totscreens do 
      for i = 1, numlines do
        scr[ s ]:settext( i, titles[ t ] or "" )
        t = t + 1
      end
    end
    scr[ 1 ]:draw()
    crtscr = 1
    
    -- Wait until the next update, processing remote control keys
    while refcount < refresh_sec * 40 do
      tmr.delay( tmrid, 25000 )  
      refcount = refcount + 1 
      scr[ crtscr ]:on_timer()
      local dummy, code = r.getkey( false )
      if code == r.KEY_STOP then
        refresh_sec = 0
        break
      elseif code == r.KEY_PLAY_PAUSE then 
        break 
      elseif code then
        local newscr = crtscr
        if code == r.KEY_PREV then
          newscr = crtscr == 1 and totscreens or crtscr - 1
        elseif code == r.KEY_NEXT then
          newscr = crtscr == totscreens and 1 or crtscr + 1
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
      if wiz.app_has_cgi() then
        local pdict = wiz.app_get_cgi()
        cdict[ 1 ], cdict[ 2 ] = pdict.url, pdict.refresh
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



