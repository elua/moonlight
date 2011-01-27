-- Moonlight demo launcher

local d = stm32.dp105
local r = stm32.sircs

local prefix = "/rom"
local flist = {}

-- Get list of demos files
local f = elua.ls( prefix, ".lua" )
for d2, name in ipairs( f ) do
  if name:match( "^d_" ) then flist[ #flist + 1 ] = name end
end  
f = nil
collectgarbage( 'collect' )

local firstidx = 1
local nlines = 4
local scrno, selline = 1, -1
local first, last 

local function cleanup()
  for i = 0, 3 do wiz.close( i ) end
  d.set_font( d.FONT_5x7 )
  d.clrscr( 0 )
  d.draw()
  collectgarbage( 'collect' )
end

local function drawscr()
  first = ( scrno - 1 ) * nlines + 1 
  last = first + nlines - 1
  last = last <= #flist and last or #flist
  d.clrscr( 0 )
  for i = first, last do
    d.putstr( 8, ( i - first ) * 8, flist[ i ]:sub( 3 ), 1 )
  end 
  d.draw()
end

local function clrsel()
  if selline ~= -1 then
    d.filled_rectangle( 2, ( selline - first ) * 8, 7, ( selline - first ) * 8 + 7, 0 )
    d.draw()
  end
end

local function runfile( fname )
  local func = assert( loadfile( fname ) )
  local newg = {}
  setmetatable( newg, { __index = _G } )
  setfenv( func, newg )
  func()
  package.loaded.mscr = nil
  mscr = nil  
end

runfile( "/rom/intro.lua" )
cleanup()
-- Main loop: show list of demos, choose a demo
while true do
  if selline == -1 then
    drawscr()
    selline = 1
  end  
  
  -- Draw indicator
  d.putstr( 2, ( selline - first ) * 8, ">", 1 )
  d.draw()
  
  -- Interpret user action
  local dummy, code = r.getkey( true )
  if code == r.KEY_PLAY_PAUSE then
    -- Launch current file
    collectgarbage( 'collect' )
    -- Run file in a different global environment
    runfile( prefix .. "/" .. flist[ selline ] )
    cleanup()
    drawscr()
  elseif code == r.KEY_NEXT then
    clrsel()
    selline = selline < #flist and selline + 1 or selline
    if selline > last then 
      scrno = scrno + 1 
      drawscr()
    end 
  elseif code == r.KEY_PREV then
    clrsel()
    selline = selline > 1 and selline - 1 or selline
    if selline < first then 
      scrno = scrno - 1 
      drawscr()
    end  
  elseif code == r.KEY_STOP then 
    break 
  end  
end

d.clrscr( 0 )
d.draw()
