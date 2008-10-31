#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "pt_prototypes.h"

/* /// "WriteBits()" */
ULONG WriteBits(UBYTE *adr, ULONG bitpos, ULONG code, UWORD len)
{
  UBYTE *badr = &adr[bitpos >> 3];
  ULONG newbitpos = bitpos+len;
  UWORD bitfrac = 8 - (bitpos & 7);

  /* initial first byte */
  if(bitfrac == 8)
  {
    *badr = 0;
  }
  if((len >= bitfrac) && (bitfrac < 8))
  {
    *badr++ |= code >> (len - bitfrac);
    len -= bitfrac;
    bitpos += bitfrac;
  } else {
    if(len < 8)
    {
      *badr |= code << (bitfrac - len);
      return(newbitpos);
    }
  }
  /* whole bytes */
  while(len > 7)
  {
    len -= 8;
    *badr++ = code >> len;
  }
  /* last fraction */
  if(len)
  {
    *badr = code << (8 - len);
  }
  return(newbitpos);
}
/* \\\ */

#define ReadBit(adr, bitpos) ((adr[bitpos >> 3] >> (7 - (bitpos & 7))) & 1)

/* /// "ReadBits()" */
ULONG ReadBits(UBYTE *adr, ULONG bitpos, UWORD len)
{
  UBYTE *badr = &adr[bitpos >> 3];
  UWORD bitfrac = bitpos & 7;
  ULONG res = 0;
  /*printf("BPos: %ld, Len %d, frac %d [%02x %02x %02x %02x %02x]\n",
         bitpos, len, bitfrac,
         badr[0], badr[1], badr[2], badr[3], badr[4]);*/
  /* initial first byte */
  if(len + bitfrac < 8)
  {
    return(((*badr << bitfrac) & 0xFF) >> (8 - len));
  }
  res = *badr++ & (0xFF >> bitfrac);
  //printf("First: %lx ", res);
  len -= (8 - bitfrac);
  while(len > 7)
  {
    res <<= 8;
    res |= *badr++;
    len -= 8;
  }
  //printf("Middle: %lx ", res);
  /* last fraction */
  if(len)
  {
    res <<= len;
    res |= *badr >> (8 - len);
  }
  //printf("Res: %08lx\n", res);
  return(res);
}
/* \\\ */

