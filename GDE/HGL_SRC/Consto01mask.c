#ifndef _GLOBAL_DEFS_H
#define _GLOBAL_DEFS_H
#include "global_defs.h"
#endif

main()
{
    Sequence tSeq;
    int ii, jj, rr;
    char acgt[128];

    for(ii = 0; ii < 128; ii++)
      acgt[ii] = '0';
    
    acgt[0x41] = acgt[0x43] = acgt[0x47] = acgt[0x54] = acgt[0x55] = '1';
    acgt[0x61] = acgt[0x63] = acgt[0x67] = acgt[0x74] = acgt[0x75] = '1';

    while((rr = ReadRecord(stdin, &tSeq)) != -1)
    {
	strcpy(tSeq.type, "MASK");
	for(ii = 0; ii < tSeq.seqlen; ii++)
	{
	    tSeq.c_elem[ii] = acgt[tSeq.c_elem[ii]];
	}
	WriteRecord(stdout, tSeq, NULL, 0);
    }
}
