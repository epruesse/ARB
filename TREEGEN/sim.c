#ifndef __RNS_H
    #include "rns.h"
#endif
#ifndef __SIMCFG_H
    #include "simcfg.h"
#endif
#ifndef __TIME_H
    #include <time.h>
#endif
#ifndef __STDLIB_H
    #include <stdlib.h>
#endif
#ifndef __BASE_H
    #include "base.h"
#endif

/* --------------------------------------------------------------------------- */
/*      int main(int argc, str argv[]) */
/* ------------------------------------------------------ 14.05.95 11.14 ----- */
int main(int argc, str argv[])
{
    RNS origin;

    if (argc!=4) error("Usage: SIM <cfg> <topo> <seq>");

    srand((unsigned)time(NULL));
    initBaseLookups();

    readSimCfg(argv[1]);
    origin = createOriginRNS();

    topo = fopen(argv[2], "w"); if (!topo) { perror(argv[2]); exit(1); }
    seq = fopen(argv[3], "w"); if (!seq) { perror(argv[3]); exit(1); }

    splitRNS(origin, 0.0, timeSteps, 0);
    dumpDepths();

    fclose(seq);
    fclose(topo);

    free(origin);
    return 0;
}



