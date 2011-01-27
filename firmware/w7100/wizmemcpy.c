/* wizmemcopy.c */

/* W7100 memory-to-memory transfer for communicating with the TCP/IP core.
 */

/* History:
 *  2009/09/06 DT   Copy from W7100 example code, reformat.
 */

/*****************************************************************************
 * Global information
 */

#define GLOBAL extern
#include "w7100.h"
#include "types.h"
#include "wizmemcpy.h"
#include "stm32conf.h"

/*****************************************************************************
 * Private constants, types, data and functions
 */

#define ENTRY_FCOPY  0x07A2

/*****************************************************************************
 * Public function bodies
 */

/*---------------------------------------------------------------------------*/
void wizmemcpy (uint32 fsrc, uint32 fdst, uint16 len) small
{
	uint8  eaback = EA;

	DPX0BK = DPH0;
	DPX1BK = DPL0;

	DPX0 = (uint8)(fsrc>>16);
	DPH0 = (uint8)(fsrc>>8);
	DPL0 = (uint8)(fsrc);

	DPX1 = (uint8)(fdst>>16);
	DPH1 = (uint8)(fdst>>8);
	DPL1 = (uint8)(fdst);

	RAMEA16 = len;	 	 

  STM32_CTS_PIN = 1;
	EA = 0;
	WCONF &= ~(0x40);     /* Enable ISP Entry */
	((void(code*)(void))ENTRY_FCOPY)();
	DPH = DPX0BK;
	DPL = DPX1BK;

	DPX0 = 00;
	DPX1 = 00;
	WCONF |= 0x40;        /* Disable ISP Entry */

	EA = eaback;
  if( eaback )
  {
    while( RI );
    STM32_CTS_PIN = 0;
  }
}

/*---------------------------------------------------------------------------*/
