#include <string.h>
#include "w7100.h"
#include "serial.h"
#include "socket.h"
#include "init.h"
#include "util.h"



char atonum(char ch)
{
	ch -= '0';
	if (ch > 9) ch -= 7;
	if (ch > 15) ch -= 0x20;
	return(ch);
}

unsigned char gethex(uint8 b0, uint8 b1)
{
	uint8 xdata buf[2];

	buf[0]   = b0;
	buf[1]   = b1;
	buf[0]   = atonum(buf[0]);
	buf[0] <<= 4;
	buf[0]  += atonum(buf[1]);
	return(buf[0]);
}

void Display_prompt(void)
{
	PutString("\n\r;");
}

