local d = stm32.dp105   
local r = stm32.sircs   
local timerid = 1
local dummy, keycode

local tw, th = 0, 0
local ttitle
local tinc, actx = -1, 0
local towait = 100000
local host, thehost

d.init()
sock = wiz.socket( wiz.SOCK_UDP, 31000 )
while true do
  local data, host = wiz.recvfrom( sock, 128, wiz.NO_TIMEOUT )
  if #data > 0 and host and not thehost then
    thehost = host
  end
  local c = #data > 0 and data:byte( 1 ) or nil  
  if c == 255 then
    local s = data:sub( 2 )
    d.filled_rectangle( 0, 8, 74, 31, 0 )
    for i = 1, 75 do
      local c = s:byte( i )
      if c > 0 then
        d.line( i - 1, 31, i - 1, 31 - c - 1, 1 )
      end 
    end
    d.draw()
  elseif c == 254 then
    ttitle = data:sub( 2 )
    tw, th = d.get_text_extent( ttitle )
    d.filled_rectangle( 0, 1, 127, 8, 0 )
    d.putstr( 0, 1, ttitle, 1 )
    d.draw()
    actx, tinc = 0, -1 
    towait = 50000
    tmr.start( timerid )
  elseif c == 253 then
    local stime = data:sub( 2 )
    local tw = d.get_text_extent( stime )
    d.filled_rectangle( 78, 10, 127, 17, 0 )
    d.putstr( 128 - tw, 10, stime, 1 )
    d.draw()
  elseif c == 252 then
    local stime = data:sub( 2 )
    local tw = d.get_text_extent( stime )
    d.filled_rectangle( 78, 19, 127, 26, 0 )
    d.putstr( 128 - tw, 19, stime, 1 )
    d.draw()    
  end
  if tw > 128 and tmr.gettimediff( timerid, 0, tmr.read( timerid ) ) >= towait then
    -- Scroll title
    actx = actx + tinc
    if actx > 0 or -actx + 128 >= tw then
      tinc = -tinc
      actx = actx + 2 * tinc
    end
    d.filled_rectangle( 0, 1, 127, 8, 0 )
    d.putstr( actx, 1, ttitle, 1 )
    d.draw()
    tmr.start( timerid )
    towait = ( actx == 0 or -actx + 128 == tw - 1 ) and 1000000 or 50000
  end
  local dummy, keycode = r.getkey( false )
  if keycode then
    if keycode == r.KEY_REPEAT then
      break
    else
      if thehost then
        wiz.sendto( sock, thehost, 32000, keycode )
      end
    end
  end
end

wiz.close( sock )
d.clrscr( 0 )
d.draw()


