local d = stm32.dp105
local r = stm32.sircs

local function text_right( y, text )
  local w = d.get_text_extent( text )
  d.putstr( 128 - w, y, text )
end

d.clrscr( 0 )
d.set_font( d.FONT_5x7 )
d.filled_rectangle( 0, 0, 127, 10, 1 )
local w = d.get_text_extent( "MOONLIGHT" )
local px = ( 128 - w ) / 2
d.filled_rectangle( px - 2, 1, px + w, 9, 0 )
d.putstr( px, 2, "MOONLIGHT", 0 )

d.set_font( d.FONT_5x5_SQUARE )
text_right( 15, "IMCU DESIGN CONTEST" )
text_right( 24, "ENTRY #003185" )
d.draw()

r.getkey( true )
