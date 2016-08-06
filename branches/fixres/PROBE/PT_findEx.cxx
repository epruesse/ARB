// =============================================================== //
//                                                                 //
//   File      : PT_findEx.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "probe.h"
#include <PT_server_prototypes.h>
#include "probe_tree.h"
#include "pt_prototypes.h"
#include <arb_strbuf.h>

static bool findLeftmostProbe(POS_TREE2 *node, char *probe, int restlen, int height) {
    if (restlen==0) return true;

    switch (node->get_type()) {
        case PT2_NODE:
            for (int i=PT_A; i<PT_BASES; ++i) { // Note: does not iterate probes containing N
                POS_TREE2 *son = PT_read_son(node, PT_base(i));
                if (son) {
                    probe[0] = PT_base(i); // write leftmost probe into result
                    bool found = findLeftmostProbe(son, probe+1, restlen-1, height+1);
                    pt_assert(implicated(found, strlen(probe) == (size_t)restlen));
                    if (found) return true;
                }
            }
            break;

        case PT2_CHAIN:
            // probe cut-off in index -> do not iterate
            break;

        case PT2_LEAF: {
            // here the probe-tree is cut off, because only one species matches
            DataLoc loc(node);
            int     pos  = loc.get_rel_pos() + height;
            int     name = loc.get_name();

            if (pos + restlen >= psg.data[name].get_size()) // @@@ superfluous ? 
                break;          // at end-of-sequence -> no probe with wanted length here

            pt_assert(probe[restlen] == 0);

            const probe_input_data& pid = psg.data[name];
            SmartCharPtr            seq = pid.get_dataPtr();

            for (int r = 0; r<restlen; ++r) {
                int rel_pos = pos+r;
                int data = pid.valid_rel_pos(rel_pos) ? PT_base((&*seq)[rel_pos]) : PT_QU;
                if (data == PT_QU || data == PT_N) return false; // ignore probes that contain 'N' or '.'
                probe[r] = data;
            }
            pt_assert(probe[restlen] == 0);
            pt_assert(strlen(probe) == (size_t)restlen);
            return true;
        }
    }

    return false;
}

static bool findNextProbe(POS_TREE2 *node, char *probe, int restlen, int height) {
    // searches next probe after 'probe' ('probe' itself may not exist)
    // returns: true if next probe was found
    // 'probe' is modified to next probe

    if (restlen==0) return false;  // in this case we found the recent probe
    // returning false upwards takes the next after

    switch (node->get_type()) {
        case PT2_NODE: {
            if (!is_std_base(probe[0])) return false;

            POS_TREE2 *son   = PT_read_son(node, PT_base(probe[0]));
            bool       found = (son != 0) && findNextProbe(son, probe+1, restlen-1, height+1);

            pt_assert(implicated(found, strlen(probe) == (size_t)restlen));

            if (!found) {
                for (int i=probe[0]+1; !found && i<PT_BASES; ++i) {
                    if (is_std_base(i)) {
                        son = PT_read_son(node, PT_base(i));
                        if (son) {
                            probe[0] = PT_base(i); // change probe
                            found = findLeftmostProbe(son, probe+1, restlen-1, height+1);
                            pt_assert(implicated(found, strlen(probe) == (size_t)restlen));
                        }
                    }
                }
            }
            return found;
        }
        case PT2_CHAIN:
        case PT2_LEAF: {
            // species list or single species reached
            return false;
        }
    }

    pt_assert(0);
    return false;
}

int PT_find_exProb(PT_exProb *pep, int) {
    POS_TREE2     *pt      = psg.TREE_ROOT2();  // start search at root
    GBS_strstruct *gbs_str = GBS_stropen(pep->numget*(pep->plength+1)+1);
    bool           first   = true;

    for (int c=0; c<pep->numget; ++c) {
        bool found = false;

        if (pep->restart) {
            pep->restart = 0;

            char *probe = (char*)ARB_alloc(pep->plength+1);
            memset(probe, 'N', pep->plength);
            probe[pep->plength] = 0; // EOS marker

            compress_data(probe);

            pep->next_probe.data = probe;
            pep->next_probe.size = pep->plength;

            found = findLeftmostProbe(pt, pep->next_probe.data, pep->plength, 0);
        }
        else {
            found = findNextProbe(pt, pep->next_probe.data, pep->plength, 0);
        }
        
        pt_assert(pep->next_probe.data[pep->plength] == 0);
        pt_assert(strlen(pep->next_probe.data) == (size_t)pep->plength);

        if (!found) break;

        // append the probe to the probe list

        if (!first) GBS_chrcat(gbs_str, (char)pep->separator);
        first = false;
        if (pep->readable) {
            char *readable = readable_probe(pep->next_probe.data, pep->next_probe.size, pep->tu);
            GBS_strcat(gbs_str, readable);
            free(readable);
        }
        else {
            GBS_strcat(gbs_str, pep->next_probe.data);
        }
    }

    pep->result = GBS_strclose(gbs_str);

    return 0;
}

