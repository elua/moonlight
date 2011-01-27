
#include "delay.h"

void wait_1us(unsigned int cnt)
{
	volatile unsigned int i;
	
	for(i = 0; i<cnt; i++) {
#pragma ASM
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
#pragma ENDASM
		}
}

void wait_1ms(unsigned int cnt)
{
	volatile unsigned int i;

	for (i = 0; i < cnt; i++) wait_1us(1000);
}

void wait_10ms(unsigned int cnt)
{
	volatile unsigned int i;
	for (i = 0; i < cnt; i++) wait_1ms(10);
}

