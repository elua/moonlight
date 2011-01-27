/* wizmemcpy.h */

/* W7100 memory-to-memory transfer for communicating with the TCP/IP core.
 */

/* History:
 *  2009/09/06 DT   Copy from W7100 example code, reformat.
 */

#ifndef __WIZMEMCPY_H
#define __WIZMEMCPY_H

/*****************************************************************************
 * Public constants, types and data
 */

/*****************************************************************************
 * Public Functions
 */

/*---------------------------------------------------------------------------*/
/* Special memory-to-memory copy routine in ROM that can access the full
 * 24-bit address space.
 */

void wizmemcpy (uint32 fsrc, uint32 fdst, uint16 len) small;
/*---------------------------------------------------------------------------*/
#endif
