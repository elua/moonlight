local d = stm32.dp105
local tmrid = 1

local img = d.load_bitmap( "/rom/wiznet.img" )

-- First screen: wiznet logo, shown line by line by varying the viewport
d.clrscr( 0 )
local w, h = d.get_bitmap_size( img )
local ix, iy = ( 128 - w ) / 2, ( 32 - h ) / 2 
for i = 1, h do
  d.set_viewport( ix, iy, ix + w, iy + i, d.NORMAL )
  d.draw_bitmap( ( 128 - w ) / 2, ( 32 - h ) / 2, img )
  d.draw()
  tmr.delay( tmrid, 50000 )
end
d.set_viewport()
tmr.delay( tmrid, 1000000 )

-- Second screen: "AND" with large font, then fade away
d.clrscr( 0 )
local f = d.load_font( "/rom/m12_fnt.bin" )
d.set_font( f )
local w, h = d.get_text_extent( "AND" )
tmr.delay( tmrid, 500000 )
d.putstr( ( 128 - w ) / 2, ( 32 - h ) / 2, "AND", 1 )
d.draw()
for i = 15, 0, -1 do
  d.set_row_intensity( 0, i )
  d.set_row_intensity( 1, i )
  tmr.delay( tmrid, 100000 ) 
end 
d.clrscr( 0 )
d.draw()
for i = 0, 3 do d.set_row_intensity( i, 15 ) end
tmr.delay( tmrid, 500000 )

-- Third screen: "CIRCUIT CELLAR" on two lines (animated)
d.set_font( d.FONT_5x7 )
d.clrscr( 0 )
for i = 1, 2 do
  local str = i == 1 and "CIRCUIT" or "CELLAR"
  local w = d.get_text_extent( str )
  local p = ( 128 - w ) / 2  
  local y = i == 1 and 7 or 17
  for j = 127, p, -1 do
    d.filled_rectangle( j, y, 127, y + 8, 0 )
    d.putstr( j, y, str, 1 )
    d.draw()
    tmr.delay( tmrid, 20000 )           
  end
end
tmr.delay( tmrid, 1000000 )

-- Fourth screen : "PRESENT", typewriter effect
d.clrscr( 0 )
f = d.load_font( "/rom/h12_fnt.bin" )
d.set_font( f )
local str = "PRESENT"
for i = 1, #str do
  local s = str:sub( 1, i )
  local w, h = d.get_text_extent( s )
  local px, py = ( 128 - w ) / 2, ( 32 - h ) / 2
  d.filled_rectangle( 0, py, 127, py + 16, 0 )
  d.putstr( px, py, s, 1 )
  d.draw()
  tmr.delay( tmrid, 200000 )
end
tmr.delay( tmrid, 1000000 )

-- Fifth screen: "iMCU design contest 2010", different lines, animation
local px = { 10, 30, 50, 90 }
local py_final = { 0, 8, 16, 25 }
local py_current = { -8, -12, -16, -20 }
local strs = { "IMCU", "DESIGN", "CONTEST", "2010" }
local ydelta = 0
d.set_font( d.FONT_5x7 )
while true do
  d.clrscr( 0 )
  for i = 1, 4 do
    d.putstr( px[ i ], py_current[ i ], strs[ i ], 1 )
  end
  d.draw()
  for i = 1, 4 do
    if py_current[ i ] ~= py_final[ i ] then
      py_current[ i ] = py_current[ i ] + 1
    end
  end
  tmr.delay( tmrid, 40000 )
  if py_current[ 4 ] == py_final[ 4 ] then break end
end
for i = 1, 4 do tmr.delay( tmrid, 1000000 ) end

-- Done
d.clrscr( 0 )
d.draw()
