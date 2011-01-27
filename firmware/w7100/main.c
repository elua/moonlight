#include <stdio.h>
#include <string.h>
#include "w7100.h"		  	// Definition file for 8051 SFR
#include "serial.h" 			// serial related functions
#include "socket.h" 			// W7100A driver file
#include "types.h"
#include "wizmemcpy.h"
#include "dhcp_app.h"
#include "lcd.h"
#include "vtmr.h"
#include "wizs.h"
#include "tftp.h"
#include "wiz.h"
#include "stm32conf.h"
#include "httpd.h"
#include "settings.h"

uint8 xdata temp = 0;

extern un_l2cval xdata lease_time;
extern uint32 xdata my_time;
volatile uint16 xdata lease_time_cnt = 0;	

uint8 xdata DHCPisSuccess;
uint8 xdata Enable_DHCP_Timer;

void Init_Sock(void);

void Init_iMCU(void)
{ 
	EA = 0; 		// Disable all interrupts 
	CKCON = 0x02;		// External Memory Access Time
	WTST = 0x03;
	WCONF = 0x40;

  socket_init();
	ser_init(); 	  // Initialize serial port (Refer to serial.c)	
  vtmr_init();	  // initialize virtual timers
	EA = 1;
}


void Init_Net_DHCP(void)
{
	uint8 xdata str[17];
	
	if (DHCP_SetIP())
	{ //DHCP success => Init Net
		DHCPisSuccess = 1;
		sprintf(str,"%.3bu.%.3bu.%.3bu.%.3bu ",
			wiz_read8 (SIPR0+0), wiz_read8 (SIPR0+1), 
			wiz_read8 (SIPR0+2), wiz_read8 (SIPR0+3));
               evb_set_lcd_text(1,str);	
		printf("> DHCP Success.\r\n");
	}
	else
	{	// DHCP Fail	=> Not init Net
		DHCPisSuccess = 0;
			printf("\n\r> DHCP Fail.")	 ;

		((void (code *)(void)) 0x0000)();

	}
}

// ****************************************************************************
// Program entry point

void main()
{
	Init_iMCU();		
	lcd_init();     // Initialize Charater LCD
  settings_init();
	evb_set_lcd_text(0,(uint8 *)" DHCP in W7100! ");

  // Reset the STM32 in the proper mode
  STM32_BOOT0_PIN = 0;
  STM32_RESET_PIN = 0;
  vtmr_delay( 32 );
  STM32_RESET_PIN = 1;

  // Initialize RPC server
  wizs_init();	

  // Initialize HTTP server
  httpd_init();

  // Enable CTS (receive serial data from STM32 board)
  STM32_CTS_PIN = 0;             
   
	evb_set_lcd_text(1,(uint8 *)"Wait for linking");
	
	/*If you use sotfware reset(such as watchdog reset), 
	please don't use this code or you may can't escape this while loop!!*/	
  if( !WTRF )
  {
	  while(!(EIF & 0x02));
	  EIF &= ~0x02;
  }
  else
    WTRF = 0;
	
	evb_set_lcd_text(1,(uint8 *)"----LINKING!----");	

	//DHCP mode flag

	if( settings_get()->static_mode == 0 )
	{
		Init_Net_Default();		// Init IP,G/W, and S/M by 0.0.0.0
		Init_Net_DHCP();
	}
	
	else {
		Init_Net();			// Init 
  }

  if ((settings_get()->static_mode == 0 ) && (DHCPisSuccess == 1)) 
    check_dhcp();
  
  // Setup socket 0 for TFTP
  socket( WIZ_SOCK_TFTP, PROTOCOL_UDP, TFTP_PORT, 0 );

  // Main program loop
  P3_2 = 0;
  P0_5 = 0;
  while( 1 )
  {    
    // Check for TFTP request
    if( wiz_getSn_RX_RSR( WIZ_SOCK_TFTP ) > 0 ) 
      tftp_server_run();

    // Check for RPC request from STM32 board
    if( ser_got_rpc_request() )
      wizs_execute_request();

    // Run the HTTP server
    httpd_run();
  }
}
