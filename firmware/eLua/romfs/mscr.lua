module( ..., package.seeall )

local d = stm32.dp105
local nlines = 4

new = function( template )
  local self = {}
  self.lines = {}
  local maxw = 0
  for i = 1, nlines do
    self.lines[ i ] = { header = template[ i ] }
    local l = self.lines[ i ]
    local w = template[ i ] and d.get_text_extent( template[ i ] ) or 0
    if w > maxw then maxw = w end
    l.dx = w
    l.needs_scroll = false
    l.scroll_inc = -1
    l.data_width = 0
    l.data = "N/A"
  end
  for i = 1, nlines do
    self.lines[ i ].dx = maxw - self.lines[ i ].dx
    self.lines[ i ].crtx = maxw
  end
  self.dx = maxw
  self.scroll_idx = 0
  self.waitcnt = 0
  setmetatable( self, { __index = _M } )
  return self
end

draw = function( self, offset )
  local loffset = offset or 0 
  for i = 1, nlines do          
    local l = self.lines[ i ]
    d.putstr( l.dx, ( i - 1 ) * 8 + loffset, l.header )
    if loffset == 0 then
      d.filled_rectangle( self.dx, ( i - 1 ) * 8, 127, i * 8 - 1, 0 )
    end 
    d.putstr( self.dx, ( i - 1 ) * 8 + loffset, l.data ) 
  end
  if offset == nil then d.draw() end  
end

reset = function( self )
  for i = 1, nlines do
    local l = self.lines[ i ]
    l.crtx = self.dx
    l.scroll_inc = -1
  end
  self.scroll_idx = 0
end

settext = function( self, line, data )
  local l = self.lines[ line ]
  l.data = data
  l.crtx = self.dx
  local w = d.get_text_extent( data )
  l.data_width = w
  l.needs_scroll = w + self.dx >= 128 
  l.scroll_inc = -1
  if self.scroll_idx == line and l.needs_scroll == false then self.scroll_idx = 0 end
end

find_next_scroll = function( self, startline )
  for i = startline + 1, nlines do
    if self.lines[ i ].needs_scroll then return i end
  end
  return 0
end

on_timer = function( self )
  if self.waitcnt > 0 then
    self.waitcnt = self.waitcnt - 1
    return
  end
  if self.scroll_idx == 0 then self.scroll_idx = self:find_next_scroll( 0 ) end
  if self.scroll_idx == 0 then return end
  local l = self.lines[ self.scroll_idx ]
  l.crtx = l.crtx + l.scroll_inc
  if -l.crtx + 128 >= l.data_width then
    l.scroll_inc = 1
    l.crtx = l.crtx + 1
    self.waitcnt = 5
  end
  d.set_viewport( self.dx, ( self.scroll_idx - 1 ) * 8, 127, self.scroll_idx * 8 - 1, d.NORMAL )
  d.filled_rectangle( self.dx, ( self.scroll_idx - 1 ) * 8, 127, self.scroll_idx * 8 - 1, 0 )
  d.putstr( l.crtx, ( self.scroll_idx - 1 ) * 8, l.data, 1 )
  d.draw()
  d.set_viewport()
  if l.crtx == self.dx then
    l.scroll_inc = -1 
    self.scroll_idx = self:find_next_scroll( self.scroll_idx )
    self.waitcnt = 5 
  end
end



