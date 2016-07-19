// ================================================================ //
//                                                                  //
//   File      : arb_flush_mem.cxx                                  //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2012   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <arbdb.h>
#include <arb_misc.h>
#include <arb_progress.h>

int ARB_main(int , char *[]) {
    GB_ULONG blocks_to_flush  = GB_get_usable_memory()*4; // no of 256b-blocks
    GB_ULONG mem_to_flush = blocks_to_flush*256;
    printf("Flushing %s of your memory\n", GBS_readable_size(mem_to_flush, "b"));

    unsigned char *mem = (unsigned char *)malloc(mem_to_flush);
    {
        arb_progress progress("writing mem", blocks_to_flush);
        for (GB_ULONG p = 0; p<mem_to_flush; ++p) {
            mem[p] = (unsigned char)p;
            if (!mem[p]) ++progress;
        }
    }
#if 0
    ulong sum = 0;
    {
        arb_progress progress("reading mem", blocks_to_flush);
        for (GB_ULONG p = 0; p<mem_to_flush; ++p) {
            sum += mem[p];
            if (!mem[p]) ++progress;
        }
    }
    printf("done with sum=%lu\n", sum);
#endif

    free(mem);

    return EXIT_SUCCESS;
}
