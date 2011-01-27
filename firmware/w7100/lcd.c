
#include <string.h>
#include "lcd.h"
#include "serial.h"
#include "delay.h"

#define BUSY		0x80

char lcd_ready(void)
{

   vuint16 xdata cnt=0;
   LCD_RSOFF();
   LCD_RWON();  
	  while(1)  
	    {
	      P2_7 = 1;
	      if(P2_7 == 0) break;
	      if(cnt++ > 0xFFFE)
		     return 0;  
	    }
  return 1;
}

void lcd_command(uint8 xdata Value)
{
  
  
  LCD_DATA(Value);	

  LCD_RSOFF();								  
  LCD_RWOFF();

  wait_1us(1);
  LCD_ENABLE();	
  wait_1ms(3);
  LCD_DISABLE();
  
  

}



void lcd_datas(uint8 xdata ch)
{
 
  
  LCD_DATA(ch);

  LCD_RSON();						
  LCD_RWOFF();	

  wait_1us(1);
  LCD_ENABLE();	
  wait_1ms(3);
  LCD_DISABLE();
  //wait_1ms(3);

}


char lcd_init(void)
{
  LCD_DISABLE();
  LCD_RSOFF();	
  LCD_RWOFF();
  

  lcd_command(LCD_SET_SYSTEM | LCD_SYS_8BIT | LCD_SYS_2LINE);
  //  wait_1us(320);  // wait  for Function Set
      	
	lcd_command(LCD_SET_DISPLAY | LCD_DISP_DISPLAY_ON);
	lcd_ready();
	lcd_command(LCD_CLEAR);

	lcd_ready();	   
	lcd_command(LCD_CURSOR_HOME);

 
	return 1;
}


void lcd_gotoxy( uint8 x,   uint8 y )
{
  switch(y)
    {
    case 0 : lcd_command(0x80 + x); break;
    case 1 : lcd_command(0xC0 + x); break;
    case 2 : lcd_command(0x94 + x); break;
    case 3 : lcd_command(0xD4 + x); break;
    }
}



char * lcd_puts(uint8 * str)
{
	uint8 xdata i;
	for (i=0; str[i] != '\0'; i++){
//	  wait_1ms(4);
	  lcd_datas(str[i]);

	}
	return str;
}


/*
void lcd_putch(uint8 xdata ch)
{
  lcd_datas(ch);
}
*/

void evb_set_lcd_text(uint8 row, uint8 * lcd)
{

  lcd_gotoxy(0,row);
  lcd_puts((char*)lcd);
}


