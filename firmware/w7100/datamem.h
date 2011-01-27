// Access to the W7100 data memory

#ifndef __DATAMEM_H__
#define __DATAMEM_H__

#include "types.h"
#include "w7100.h"

#define ISP_ENTRY 0x07FD
#define ISP_SERASE         0x30    // Sector Erase Command
#define ISP_MERASE         0x10    // Chip Erase Command
#define ISP_BPROG          0xA0    // Byte Program Command
#define ISP_SECPROG        0xA1    // Sector Program Command
#define ISP_CHIPROG        0xA2    // Chip Program Command
#define ISP_BREAD          0x00    // Byte Read Command
#define ISP_DATAERASE       0xD0    // Sector Erase Command for Data Flash
#define ISP_DATAPROG        0xD1    // Byte Program Command for Data Flash
#define ISP_DATAREAD        0xD2    // Read Command for Data Flash

#define ISP_chip_erase()                do_isp(ISP_MERASE,0,0)
#define ISP_sector_erase(FSADDR)        do_isp(ISP_SERASE,FSADDR,0)
#define ISP_write_byte(FBADDR,DAT)      do_isp(ISP_BPROG,FBADDR,DAT)
#define ISP_read_byte(FBADDR)           do_isp(ISP_BREAD,FBADDR,0)
#define ISP_data_erase()                do_isp(ISP_DATAERASE,0,0)
#define ISP_chip_prog(RSADDR,READDR,FSADDR) \
{\
   RAMBA16 = RSADDR; \
   RAMEA16 = READDR; \
   do_isp(ISP_CHIPROG,FSADDR,0); \
}

#define ISP_sector_prog(RSADDR,READDR,FSADDR) \
{\
   RAMBA16 = RSADDR; \
   RAMEA16 = READDR; \
   do_isp(ISP_SECPROG,FSADDR,0); \
}

#define ISP_data_sector_read(RSADDR) \
{\
   RAMBA16 = RSADDR; \
   do_isp(ISP_DATAREAD,0,0); \
}

#define ISP_data_sector_prog(RSADDR) \
{\
   RAMBA16 = RSADDR; \
   do_isp(ISP_DATAPROG,0,0); \
}

unsigned char do_isp( unsigned char isp_id, unsigned short isp_addr, unsigned char isp_data );

#endif
