/*--------------------------------------------------------------------------
  W7100.H
 Registers definition for W7100
-------------------------------------------------------------------------- */
#ifndef _W7100_H_
#define _W7100_H_


/*---------------------------------------------------------------------------
  Defined registers  
---------------------------------------------------------------------------*/

/*  BYTE Register  */
  sfr	P0		= 0x80;   /* Port 0                    */
  sfr SP        	= 0x81;   /* Stack Pointer             */
  sfr16 DPTR0   	= 0x82;
  sfr DPL       	= 0x82;   /* Data Pointer 0 Low byte   */
  sfr DPL0      	= 0x82;
  sfr DPH       	= 0x83;   /* Data Pointer 0 High byte  */
  sfr DPH0      	= 0x83;
  sfr16 DPTR1   	= 0x84;
  sfr DPL1      	= 0x84;   /* Data Pointer 1 Low byte */
  sfr DPH1      	= 0x85;   /* Data Pointer 1 High byte */
  
  sfr DPS       	= 0x86;   /* Data Pointer Selector */
   
  sfr PCON      	= 0x87;   /* Power Configuration       */

  sfr TCON      	= 0x88;   /* Timer 0,1 Configuration   */
  sfr TMOD      	= 0x89;   /* Timer 0,1 Mode            */
  sfr TL0       	= 0x8A;   /* Timer 0 Low byte counter  */
  sfr TL1       	= 0x8B;   /* Timer 1 Low byte counter  */
  sfr TH0       	= 0x8C;   /* Timer 0 High byte counter */
  sfr TH1       	= 0x8D;   /* Timer 1 High byte counter */

  sfr CKCON     	= 0x8E;   /* XDATA Wait States         */
  sfr P1        	= 0x90;   /* Port 1                    */
  sfr EIF		= 0x91;   /* Extended Interrupt Flag   */
  sfr WTST      	= 0x92;   /* Program Wait States       */

  sfr DPX0      	= 0x93;   /* Data Page Pointer 0       */
  sfr DPX1      	= 0x95;
  sfr SCON      	= 0x98;   /* Serial 0 Configuration    */
  sfr SBUF      	= 0x99;   /* Serial 0 I/O Buffer       */
  sfr SBUF0     	= 0x99;

  sfr P2        	= 0xA0;   /* Port 2                    */
  sfr IE        	= 0xA8;   /* Interrupt Enable          */
  sfr P3        	= 0xB0;   /* Port 3                    */
  sfr IP        	= 0xB8;

  sfr TA        	= 0xC7;
  sfr T2CON		= 0xC8;
  sfr RLDL		= 0xCA;
  sfr RLDH		= 0xCB;
  sfr TL2		= 0xCC;
  sfr TH2		= 0xCD;
  
  sfr PSW       	= 0xD0;   /* Program Status Word       */
  sfr WDCON     	= 0xD8;
  
  sfr ACC       	= 0xE0;   /* Accumulator               */
  sfr EIE       	= 0xE8;   /* External Interrupt Enable */
  sfr MXAX      	= 0xEA;   /* MOVX @Ri High address     */
 
  sfr B         	= 0xF0;   /* B Working register        */
  sfr ISPID     	= 0xF1;   /* ID for ISP*/
  sfr16 ISPADDR16	= 0xF2; /* 16bit Address for ISP */
  sfr ISPDATA   	= 0xF4;
  sfr CKCBK     	= 0xF5;   /* CKCON backup register */
  sfr DPX0BK    	= 0xF6;   /* DPX0 backup Register   */
  sfr DPX1BK    	= 0xF7;
  sfr EIP		= 0xF8;   /* External Interrupt Priority */
  sfr DPSBK     	= 0xF9;
  sfr16 RAMBA16 	= 0xFA;
  sfr TMPR0     	= 0xFA;
  sfr TMPR1     	= 0xFB;
  sfr16 RAMEA16 	= 0xFC;
  sfr TMPR2     	= 0xFC;
  sfr TMPR3     	= 0xFD;

  sfr WCONF     	= 0xFF;   /* W7100 Config Register */
                          /* 7    6    5   4   3    2   1  0  */
                          /* RB ISPEN EM2 EM1 EM0 F64EN FB BS */
                          /* 
                             RB      : 1 - Reboot after ISP done(APP Entry(0xFFF7~0xFFFF) Enable), 0 - No reboot
                             ISPEN   : 0 - Enable ISP in Boot built in W7100 , 1 - Disable
                             EM[2:0] : External Memory Mode
                             F64EN   : 0 - Boot Mode, 1 - Flash 64Kbyte Enable
                             FB      : Flash Busy Flag for ISP
                             BS      : Boot Selection (0 - Apps Running, 1 - Boot Running)
                           */    


/*-------------------------------------------------------------------------
  BIT Register  
  -------------------------------------------------------------------------*/

/*  P0  */
  sbit P0_7     = P0^7;
  sbit P0_6     = P0^6;
  sbit P0_5     = P0^5;
  sbit P0_4     = P0^4;
  sbit P0_3     = P0^3;
  sbit P0_2     = P0^2;
  sbit P0_1     = P0^1;
  sbit P0_0     = P0^0;

/*  P1  */
  sbit P1_7     = P1^7;
  sbit P1_6     = P1^6;
  sbit P1_5     = P1^5;
  sbit P1_4     = P1^4;
  sbit P1_3     = P1^3;
  sbit P1_2     = P1^2;
  sbit P1_1     = P1^1;
  sbit P1_0     = P1^0;

/*  P2  */
  sbit P2_7     = P2^7;
  sbit P2_6     = P2^6;
  sbit P2_5     = P2^5;
  sbit P2_4     = P2^4;
  sbit P2_3     = P2^3;
  sbit P2_2     = P2^2;
  sbit P2_1     = P2^1;
  sbit P2_0     = P2^0;

/*  P3  */
  sbit P3_7     = P3^7;
  sbit P3_6     = P3^6;
  sbit P3_5     = P3^5;
  sbit P3_4     = P3^4;
  sbit P3_3     = P3^3;
  sbit P3_2     = P3^2;
  sbit P3_1     = P3^1;
  sbit P3_0     = P3^0;

/*  TCON  */
  sbit IT0	= TCON^0;
  sbit IE0      = TCON^1;
  sbit IT1      = TCON^2;
  sbit IE1      = TCON^3;
  sbit TR0      = TCON^4;
  sbit TF0      = TCON^5;
  sbit TR1      = TCON^6;
  sbit TF1      = TCON^7;

 /* T2CON */
  sbit TF2	    = T2CON^0;
  sbit EXF2     = T2CON^1;
  sbit RCLK     = T2CON^2;
  sbit TCLK     = T2CON^3;
  sbit EXEN2   = T2CON^4;
  sbit TR2      = T2CON^5;
  sbit CT2      = T2CON^6;
  sbit CPRL2   = T2CON^7;



/*   SCON */
  sbit RI	= SCON^0;
  sbit TI       = SCON^1;
  sbit RB8      = SCON^2;
  sbit TB8      = SCON^3;
  sbit REN      = SCON^4;
  sbit SM2      = SCON^5;
  sbit SM1      = SCON^6;
  sbit SM0      = SCON^7;

/*  IE   */
  sbit EX0	= IE^0;
  sbit ET0      = IE^1;
  sbit EX1      = IE^2;
  sbit ET1      = IE^3;
  sbit ES0      = IE^4;
  sbit ES       = IE^4;
  sbit ET2 	= IE^5;
  sbit EA       = IE^7;

/*  IP   */ 
  sbit PX0	= IP^0;
  sbit PT0      = IP^1;
  sbit PX1      = IP^2;
  sbit PT1      = IP^3;
  sbit PS       = IP^4;
  sbit PT2      = IP^5;
  
/*  PSW   */
  sbit P        = PSW^0;
  sbit F1       = PSW^1;
  sbit OV       = PSW^2;
  sbit RS0      = PSW^3;
  sbit RS1      = PSW^4;
  sbit F0       = PSW^5;
  sbit AC       = PSW^6;
  sbit CY       = PSW^7;

/*  EIE   */
  sbit EINT2	= EIE^0;
  sbit EINT3	= EIE^1;
  sbit EINT5	= EIE^3;
  sbit EWDI	= EIE^4;

/*  EIP  */
  sbit PINT2	= EIP^0;
  sbit PINT3	= EIP^1;
  sbit PINT5	= EIP^3;
  sbit PWDI	= EIP^4;

  /* B */
  sbit B0 = B^0;
  sbit B1 = B^1;
  sbit B2 = B^2;
  sbit B3 = B^3;
  sbit B4 = B^4;
  sbit B5 = B^5;
  sbit B6 = B^6;
  sbit B7 = B^7;

   /* ACC0 */
  sbit ACC0 = ACC^0;
  sbit ACC1 = ACC^1;
  sbit ACC2 = ACC^2;
  sbit ACC3 = ACC^3;
  sbit ACC4 = ACC^4;
  sbit ACC5 = ACC^5;
  sbit ACC6 = ACC^6;
  sbit ACC7 = ACC^7;

  /* WDCON */
  sbit RWT = WDCON^0;
  sbit EWT = WDCON^1;
  sbit WTRF = WDCON^2;
  sbit WDIF = WDCON^3;

  /* */
/*-------------------------------------------------------------------------
  BIT Values  
  -------------------------------------------------------------------------*/

/* TMOD Bit Values */
  #define T0_M0_	0x01
  #define T0_M1_   	0x02
  #define T0_CT_   	0x04
  #define T0_GATE_ 	0x08
  #define T1_M0_   	0x10
  #define T1_M1_   	0x20
  #define T1_CT_   	0x40
  #define T1_GATE_ 	0x80

/* CKCON Bit Values  */
  #define MD_    	0x07
  #define T0M_   	0x08
  #define T1M_   	0x10
  #define T2M_   	0x20

#endif /*_W7100_H_*/
