
#include "types.h"
#include "sockutil.h"


uint16 checksum(uint8 * data_buf, uint16 len)

{
  uint16 sum, tsum, i, j;
  uint32 lsum;
  
  j = len >> 1;
  
  lsum = 0;
  
  for (i = 0; i < j; i++) 
    {
      tsum = data_buf[i * 2];
      tsum = tsum << 8;
      tsum += data_buf[i * 2 + 1];
      lsum += tsum;
    }
  
  if (len % 2) 
    {
      tsum = data_buf[i * 2];
      lsum += (tsum << 8);
    }
  
  
  sum = lsum;
  sum = ~(sum + (lsum >> 16));
  return sum;	

}


uint16 htons( uint16 hostshort)
{
#if 0
  //#ifdef LITTLE_ENDIAN	
	uint16 netshort=0;
	netshort = (hostshort & 0xFF) << 8;

	netshort |= ((hostshort >> 8)& 0xFF);
	return netshort;
#else
	return hostshort;
#endif		
}
















