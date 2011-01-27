#ifndef _LCD_H
#define _LCD_H

#include "W7100.h"
#include "types.h"

#define LCD_MAX_COL		16
#define LCD_MAX_ROW		2

#define LCD_BASEADDR		0x8000
#define LCD_DELAY		    500


#define LCD_RWON()	    P0_1 = 1
#define LCD_RWOFF()	    P0_1 = 0

#define LCD_RSON()	P0_0 = 1
#define LCD_RSOFF()	P0_0 = 0

#define LCD_ENABLE()	P0_2 = 1
#define LCD_DISABLE()	P0_2 = 0

#define LCD_DATA(Data)	P2 = Data


//LCD Set System
#define LCD_SET_SYSTEM          0x20 //
#define LCD_SYS_8BIT            0x10 //
//#define LCD_SYS_4BIT            0x00 //
#define LCD_SYS_2LINE           0x08 //
#define LCD_DOT_5_11			0x04


//LCD Set Cursor Shift
//#define LCD_SET_CUR_SHIFT       0x10
//#define LCD_DISPLAY_SHIFT       0x08    //if bit clear -> Cursor Shift 
//#define LCD_CURSOR_SHIFT_RIGHT  0x04    //if bit clear -> Left Shift

//LCD Set Display
#define LCD_SET_DISPLAY         0x08 //
#define LCD_DISP_DISPLAY_ON     0x04 //
//#define LCD_DISP_CURSOR_ON      0x02
//#define LCD_DISP_CURSOR_FLASH   0x01

//LCD Set Input Mode
//#define LCD_INPUT_MODE          0x04
//#define LCD_INPUT_INCRENENT	    0x02    //if bit clear -> input decrement	
//#define LCD_INPUT_SHIFT         0x01

#define LCD_CURSOR_HOME         0x02
#define LCD_CLEAR               0x01
//#define LCD_SET_CGRAM           0x40
//#define LCD_SET_DDRAM           0x80	
	
char lcd_ready(void);			// Check for LCD to be ready
//void lcd_clrscr(void);			// Clear LCD. 
char lcd_init(void);			// LCD Init
void lcd_gotoxy(uint8 xdata x, uint8 xdata y);	// Output character string in current Cursor.
char* lcd_puts(uint8* str);		// Output character stream in current Cursor.
//void lcd_putch(uint8 xdata ch);		// Output 1 character in current Cursor.
void evb_set_lcd_text(uint8 row, uint8* lcd);
void lcd_busy(void);

void lcd_command(uint8 xdata Value)	;


#endif