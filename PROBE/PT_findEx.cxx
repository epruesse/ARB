#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>

#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "probe_tree.hxx"
#include "pt_prototypes.h"

static bool findLeftmostProbe(POS_TREE *node, char *probe, int restlen, int height) {
    if (restlen==0) return true;

    switch (PT_read_type(node)) {
        case PT_NT_NODE:  {
            for (int i=PT_A; i<PT_B_MAX; ++i) {
                POS_TREE *son = PT_read_son(psg.ptmain, node, PT_BASES(i));
                if (son) {
                    probe[0] = PT_BASES(i); // write leftmost probe into result
                    bool found = findLeftmostProbe(son, probe+1, restlen-1, height+1);
                    pt_assert(!found || (strlen(probe) == (size_t)restlen));
                    if (found) return true;
                }
            }
            break;
        }
        case PT_NT_CHAIN: {
            // fprintf(stdout, "Reached chain in findLeftmostProbe() (restlen=%i)\n", restlen);
            pt_assert(0);  // unhandled yet
            break;
        }
        case PT_NT_LEAF:  {
            // here the probe-tree is cut off, because only one species matches
            int pos  = PT_read_rpos(psg.ptmain, node) + height;
            int name = PT_read_name(psg.ptmain, node);
            if (pos + restlen >= psg.data[name].size)
                break;          // at end-of-sequence -> no probe with wanted length here

            pt_assert(probe[restlen] == 0);
            const char *seq_data = psg.data[name].data;
            for (int r = 0; r<restlen; ++r) {
                int data = seq_data[pos+r];
                if (data == PT_QU || data == PT_N) return false; // ignore probes that contain 'N' or '.'
                probe[r] = data;
            }
            pt_assert(probe[restlen] == 0);
            pt_assert(strlen(probe) == (size_t)restlen);
            return true;
        }
        default : pt_assert(0); break; // oops
    }

    return false;
}
//  ---------------------------------------------------------------------
//      static bool findNextProbe(POS_TREE *node, char *probe, int restlen)
//  ---------------------------------------------------------------------
// searches next probe after 'probe' ('probe' itself may not exist)
// returns: true if next probe was found
// 'probe' is modified to next probe
static bool findNextProbe(POS_TREE *node, char *probe, int restlen, int height) {
    if (restlen==0) return false;  // in this case we found the recent probe
    // returning false upwards takes the next after

    switch (PT_read_type(node)) {
        case PT_NT_NODE:  {
            POS_TREE *son   = PT_read_son(psg.ptmain, node, PT_BASES(probe[0]));
            bool      found = (son != 0) && findNextProbe(son, probe+1, restlen-1, height+1);

            pt_assert(!found || (strlen(probe) == (size_t)restlen));

            if (!found) {
                for (int i=probe[0]+1; !found && i<PT_B_MAX; ++i) {
                    son = PT_read_son(psg.ptmain, node, PT_BASES(i));
                    if (son) {
                        probe[0] = PT_BASES(i); // change probe
                        found = findLeftmostProbe(son, probe+1, restlen-1, height+1);
                        pt_assert(!found || (strlen(probe) == (size_t)restlen));
                    }
                }
            }
            return found;
        }
        case PT_NT_CHAIN:
        case PT_NT_LEAF:  {
            // species list or single species reached
            // fprintf(stdout, "Reached chain or leaf in findNextProbe() (restlen=%i)\n", restlen);
            return false;
        }
        default : pt_assert(0); break; // oops
    }

    pt_assert(0);
    return false;
}

//  ------------------------------------------------------
//      extern "C" int PT_find_exProb(PT_exProb *pep)
//  ------------------------------------------------------
extern "C" int PT_find_exProb(PT_exProb *pep) {
    POS_TREE    *pt      = psg.pt; // start search at root
    void        *gbs_str = GBS_stropen(pep->numget*(pep->plength+1)+1);
    bool         first   = true;

    for (int c=0; c<pep->numget; ++c) {
        bool found = false;

        if (pep->restart) {
            pep->restart = 0;

            char *probe = (char*)malloc(pep->plength+1);
            memset(probe, 'N', pep->plength);
            probe[pep->plength] = 0; // EOS marker

            compress_data(probe);

            pep->next_probe.data = probe;
            pep->next_probe.size = pep->plength+1;

            found = findLeftmostProbe(pt, pep->next_probe.data, pep->plength, 0);

            pt_assert(pep->next_probe.data[pep->plength] == 0);
            pt_assert(strlen(pep->next_probe.data) == (size_t)pep->plength);
        }

        if (!found) {
            found = findNextProbe(pt, pep->next_probe.data, pep->plength, 0);

            pt_assert(pep->next_probe.data[pep->plength] == 0);
            pt_assert(strlen(pep->next_probe.data) == (size_t)pep->plength);
        }
        if (!found) break;

        // append the probe to the probe list

        if (!first) GBS_strcat(gbs_str, ";");
        first = false;
        GBS_strcat(gbs_str, pep->next_probe.data);
    }

    pep->result = GBS_strclose(gbs_str);

    return 0;
}

