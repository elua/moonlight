// Access to the W7100 data memory
/*
**********************************************************************
* Description : Data FLASH Erase, Read and Write test
* Argument : 
* Returns : 
* Note : Just use for testing
**********************************************************************
*/
#pragma userclass(CODE = ISP)
#include <stdio.h>
#include "w7100.h"
#include "datamem.h"
#include "types.h"
#include "stm32conf.h"

unsigned char do_isp( unsigned char isp_id, unsigned short isp_addr, unsigned char isp_data )
{
  uint8 TMPR0 = 0;
  TMPR0 = EA;    // backup EA
  STM32_CTS_PIN = 1;
  EA = 0;    // disable EA
  WCONF &= ~(0x40);     // Enable ISP Entry
  ISPID = isp_id;
  ISPADDR16 = isp_addr;
  ISPDATA = isp_data;
  ((void(code*)(void))ISP_ENTRY)();    // call ISP Entry
  WCONF |= 0x40;        // Disable ISP Entry
  EA = TMPR0;    // restore EA
  if( TMPR0 )
  {
    while( RI ); 
    STM32_CTS_PIN = 0;
  }
  return ISPDATA;
}
