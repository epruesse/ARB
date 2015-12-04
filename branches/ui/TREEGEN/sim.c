#include "rns.h"
#include <time.h>
#include <stdlib.h>

int ARB_main(int argc, char *argv[]) {
    RNS origin;

    if (argc!=4) error("Usage: arb_treegen <cfg> <tree> <seq>\n"
                       "An evolution simulator - (C) 1995 by Ralf Westram\n"
                       "\n"
                       "Environment specification is read from <cfg> (will be generated if missing)\n"
                       "\n"
                       "<tree> file were the simulated phylogeny is saved to (as tree)\n"
                       "<seq>  file were the simulated species are saved to (as sequences)\n"
                       );

    srand((unsigned)time(NULL));
    initBaseLookups();

    readSimCfg(argv[1]);
    origin = createOriginRNS();

    topo = fopen(argv[2], "w"); if (!topo) { perror(argv[2]); exit(1); }
    seq = fopen(argv[3], "w"); if (!seq) { perror(argv[3]); exit(1); }

    splitRNS(-1, origin, 0.0, timeSteps, 0);
    dumpDepths();

    fclose(seq);
    fclose(topo);

    free(origin);
    return 0;
}



