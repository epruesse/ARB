#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>

#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "probe_tree.hxx"
#include "pt_prototypes.h"

//  -----------------------------------------------------------------------------
//      static bool findLeftmostProbe(POS_TREE *node, char *probe, int plen)
//  -----------------------------------------------------------------------------
static bool findLeftmostProbe(POS_TREE *node, char *probe, int plen) {
    if (plen==0) return true;
    for (int i=PT_A; i<PT_B_MAX; ++i) {
        POS_TREE *son = PT_read_son(psg.ptmain, node, PT_BASES(i));
        if (son) {
            probe[0] = PT_BASES(i); // write leftmost probe into result
            bool found = findLeftmostProbe(son, probe+1, plen-1);
            if (found) return true;
        }
    }
    return false;
}
//  ---------------------------------------------------------------------
//      static bool findNextProbe(POS_TREE *node, char *probe, int plen)
//  ---------------------------------------------------------------------
// searches next probe after 'probe' ('probe' itself may not exist)
// returns: true if next probe was found
// 'probe' is modified to next probe
static bool findNextProbe(POS_TREE *node, char *probe, int plen) {
    if (plen==0) return false;

    bool found = false;
    POS_TREE *son = PT_read_son(psg.ptmain, node, PT_BASES(probe[0]));
    if (son) found = findNextProbe(son, probe+1, plen-1);
    if (!found) {
        for (int i=probe[0]+1; !found && i<PT_B_MAX; ++i) {
            son = PT_read_son(psg.ptmain, node, PT_BASES(i));
            if (son) {
                probe[0] = PT_BASES(i); // change probe
                found = findLeftmostProbe(son, probe+1, plen-1);
            }
        }
    }
    return found;
}

//  ------------------------------------------------------
//      extern "C" int PT_find_exProb(PT_exProb *pep)
//  ------------------------------------------------------
extern "C" int PT_find_exProb(PT_exProb *pep) {
    POS_TREE *pt = psg.pt; // start search at root
    void *gbs_str = GBS_stropen(pep->numget*(pep->plength+1)+1);
    bool first = true;

    for (int c=0; c<pep->numget; ++c) {
        bool found = false;

        if (pep->restart) {
            pep->restart = 0;

            char *probe = (char*)malloc(pep->plength+1);
            memset(probe, 'N', pep->plength);
            probe[pep->plength] = 0;

            compress_data(probe);

            pep->next_probe.data = strdup(probe);
            pep->next_probe.size = strlen(probe)+1;

            found = findLeftmostProbe(pt, pep->next_probe.data, pep->next_probe.size-1);
        }
        if (!found) found = findNextProbe(pt, pep->next_probe.data, pep->next_probe.size-1);

        if (found) {
            if (!first) GBS_strcat(gbs_str, ";");
            first = false;
            GBS_strcat(gbs_str, pep->next_probe.data);
        }
    }

    pep->result = GBS_strclose(gbs_str, 0);

    return 0;
}

