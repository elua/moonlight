-- Adapted from http://lua-users.org/wiki/SciteTicTacToe

local tx = 20
local d = stm32.dp105
local s = stm32.sircs
local snd = stm32.snd
local O, X = 1, 10
local t = {}
local userexit = false
local score = 0
local msg
local userstart = false
local ngames = 0

local snd_victory = ":-CEG+C"
local snd_defeat = ":C-C"
local snd_over = ":-CG"

---------- Table draw functions

local function draw_table_lines()
  d.line( tx + 10, 0, tx + 10, 31, 1 )
  d.line( tx + 21, 0, tx + 21, 31, 1 )
  d.line( tx, 10, tx + 31, 10, 1 )
  d.line( tx, 21, tx + 31, 21, 1 ) 
end

local function draw_piece( idx, piece )
  local x, y = ( idx - 1 ) % 3, math.floor( ( idx - 1 ) / 3 )
  local px = tx + x * 11 + 1
  local py = y * 11 + 1
  if piece == O then
    d.circle( px + 4, py + 4, 3, 1 )
  elseif piece == X then
    d.line( px, py + 1, px + 7, py + 8, 1 )
    d.line( px, py + 7, px + 7, py, 1 )
  end
end

local function draw_table()
  for i = 1, 9 do 
    if t[ i ] ~= 0 then
      draw_piece( i, t[ i ] )
    end
  end
  d.draw()
end

---------- Tic Tac Toe functions (implicitly uses O as computer, X as human)

local function check_for_win( player )   -- see who wins
  local wins = player * 3
  if t[1]+t[5]+t[9] == wins or
     t[3]+t[5]+t[7] == wins then return true end
  for i = 1,3 do
    local j = i * 3
    if t[i]+t[i+3]+t[i+6] == wins or
       t[j-2]+t[j-1]+t[j] == wins then return true end
  end
  return false
end

local function any_win()                 -- see if somebody won
  return check_for_win(t, X) or check_for_win(t, O)
end

local function move_count()              -- counts the number of moves
  local n = 0
  for i = 1, 9 do if t[i] == O or t[i] == X then n = n + 1 end end
  return n
end

-- not-bad movement evaluator (minimax can be easily made perfect)
-- (1) picks the obvious
-- (2) blocks the obvious
-- (3) otherwise pick randomly
local function move_simple(player)
  local mv, opponent       
  if player == X then opponent = O else opponent = X end
  for i = 1, 9 do -- (1)
    if t[i] == 0 then
      t[i] = player
      if check_for_win(player) then t[i] = player return end
      t[i] = 0
    end
  end
  for i = 1, 9 do -- (2)
    if t[i] == 0 then
      t[i] = opponent
      if check_for_win(opponent) then t[i] = player return end
      t[i] = 0
    end
  end
  -- if move_count(t) == 9 then Error(MSG.Borked) return end
  repeat mv = math.random(1, 9) until t[mv] == 0 -- (3)
  t[mv] = player
end
local evaluate = move_simple             -- select evaluator

local function computer_start()          -- computer may start
    t[math.random(1, 9)] = O
end

local function init_table()
  for i = 1, 9 do t[ i ]= 0 end
end

-- Remote key to position translation table
local rtable = 
{
  [ s.KEY_1 ] = 1,
  [ s.KEY_2 ] = 2,
  [ s.KEY_3 ] = 3,
  [ s.KEY_4 ] = 4,
  [ s.KEY_5 ] = 5,
  [ s.KEY_6 ] = 6,
  [ s.KEY_7 ] = 7,
  [ s.KEY_8 ] = 8,
  [ s.KEY_9 ] = 9
}

while true do
  d.clrscr( 0 )
  draw_table_lines()
  init_table()
  if not userstart then
    computer_start()
  end
  ngames = ngames + 1 
  d.putstr( 64, 2, string.format( "Score: %d", score ), 1 )
  d.putstr( 64, 12, string.format( "Games: %d", ngames ), 1 )  
  draw_table()
  local oldscore = score
  while true do
    local dummy, code = s.getkey( true )
    if code == s.KEY_STOP then
      userexit = true 
      break 
    end
    if rtable[ code ] and t[ rtable[ code ] ] == 0 then
      local n = rtable[ code ]
      -- User moves
      t[ n ] = X
      draw_table()        
      if check_for_win( X ) then
        score = score + 10
        msg = "YOU WIN" 
        break 
      end    
      -- Check for end of game
      if move_count() == 9 then
        msg = "GAME OVER"
        break
      end
            
      -- Computer moves
      evaluate( O )
      draw_table()    
      if check_for_win( O ) then
        score = score - 15
        msg = "I WIN! :)" 
        break 
      end      
      -- Check for end of game
      if move_count() == 9 then
        msg = "GAME OVER"
        break
      end
    end
  end
  
  if userexit then break end
  d.filled_rectangle( 64, 2, 127, 10, 0 )
  d.putstr( 64, 2, string.format( "Score: %d", score ), 1 )
  d.putstr( 64, 22, msg, 1 )
  d.draw()  
  if score > oldscore then
    snd.play( snd_victory )
  elseif score < oldscore then
    snd.play( snd_defeat )
  else
    snd.play( snd_over )
  end
  local dummy, code = s.getkey( true )
  if code == s.KEY_STOP then break end
  userstart = code == s.KEY_1
end

d.clrscr( 0 )
d.draw()
