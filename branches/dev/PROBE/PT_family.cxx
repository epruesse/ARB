// =============================================================== //
//                                                                 //
//   File      : PT_family.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "probe.h"
#include <struct_man.h>
#include <PT_server_prototypes.h>
#include "probe_tree.hxx"
#include "pt_prototypes.h"

#include <arbdbt.h>

#include <algorithm>


// overloaded functions to avoid problems with type-punning:
inline void aisc_link(dll_public *dll, PT_family_list *family)   { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(family)); }

class MatchMarker { // @@@ quite useless class atm - will be filled later
public:
    MatchMarker() {}

    void mark_all(POS_TREE *pt, const char *probe, int length, int height, int accept_mismatches);
};

struct mark_all_matches_chain_handle {
    int operator()(int name, int /* pos */, int /* rpos */) {
        /*! Increment match_count for a matched chain */

        // @@@ Does not check the "rest of probe" (like below for PT_NT_LEAF)
        // -> accepts anything when a chain is reached (-> invalid matches)
        psg.data[name].stat.match_count++;
        return 0;
    }
};

void MatchMarker::mark_all(POS_TREE *pt, const char *probe, int length, int height, int accept_mismatches) {
    /*! Traverse pos(sub)tree and increment match_count for each match of 'probe' */

    if (pt == NULL || accept_mismatches<0) {
        // pt_assert(0); // COVERED!
        return;
    }

    int type_of_node = PT_read_type(pt);
    if (type_of_node != PT_NT_NODE) {
        // Found unique solution
        if (type_of_node == PT_NT_LEAF) {
            int ref_pos = PT_read_rpos(psg.ptmain, pt);
            int ref_name = PT_read_name(psg.ptmain, pt);

            // Check rest of probe
            while (*probe && accept_mismatches >= 0) {
                // @@@ does not abort, when enough bases have been found
                // -> may reach max. mismatches and then discard a good match 
                
                if (*probe != psg.data[ref_name].data[ref_pos+height]) {
                    accept_mismatches--;
                }
                probe++;
                height++;
                // pt_assert(0); // !COVERED
            }
            // Increment count if probe matches
            if (accept_mismatches >= 0) {
                // pt_assert(0); // !COVERED
                psg.data[ref_name].stat.match_count++;
            }
        }
        else { // type_of_node == CHAIN
            // pt_assert(0); // !COVERED
            pt_assert(type_of_node == PT_NT_CHAIN);
            PT_read_chain(psg.ptmain, pt, mark_all_matches_chain_handle());
        }
    }
    else {
        // type_of_node == PT_NT_NODE
        for (int base = PT_N; base < PT_B_MAX; base++) {
            POS_TREE *pt_son = PT_read_son(psg.ptmain, pt, (PT_BASES)base);
            if (pt_son) {
                if (*probe != PT_QU) {
                    if (*probe != base) {
                        // pt_assert(0); // !COVERED
                        mark_all(pt_son, probe+1, length, height + 1, accept_mismatches-1);
                    }
                    else {
                        // pt_assert(0); // !COVERED
                        mark_all(pt_son, probe+1, length, height + 1, accept_mismatches);
                    }
                }
                else {
                    // probe contains a '.'
                    // (only true for right end of sequence)

                    // pt_assert(0); // !COVERED
                    // @@@ call seems unreasonable:
                    mark_all(pt_son, probe, length, height + 1, accept_mismatches);
                }
            }
        }
    }
}

static void clear_statistic() {
    /*! Clear all information in psg.data[i].stat */

    for (int i = 0; i < psg.data_count; i++) {
        memset((char *) &psg.data[i].stat, 0, sizeof(probe_statistic));
    }
}



static void make_match_statistic(int probe_len, int sequence_length) {
    /*! Calculate the statistic information for the family */

    // compute statistic for all species in family
    for (int i = 0; i < psg.data_count; i++) {
        int all_len = std::min(psg.data[i].size, sequence_length) - probe_len + 1;
        if (all_len <= 0) {
            psg.data[i].stat.rel_match_count = 0;
        }
        else {
            psg.data[i].stat.rel_match_count = psg.data[i].stat.match_count / (double) (all_len);
        }
    }
}

struct cmp_probe_abs {
    bool operator()(const struct probe_input_data* a, const struct probe_input_data* b) {
        return b->stat.match_count < a->stat.match_count;
    }
};

struct cmp_probe_rel {
    bool operator()(const struct probe_input_data* a, const struct probe_input_data* b) {
        return b->stat.rel_match_count < a->stat.rel_match_count;
    }
};

static int make_PT_family_list(PT_local *locs) {
    /*!  Make sorted list of family members */

    // Sort the data
    struct probe_input_data *my_list[psg.data_count];
    for (int i = 0; i < psg.data_count; i++) {
        my_list[i] = &psg.data[i];
    }

    bool sort_all = locs->ff_sort_max == 0 || locs->ff_sort_max >= psg.data_count;

    if (locs->ff_sort_type == 0) {
        if (sort_all) {
            std::sort(my_list, my_list + psg.data_count, cmp_probe_abs());
        }
        else {
            std::partial_sort(my_list, my_list + locs->ff_sort_max, my_list + psg.data_count, cmp_probe_abs());
        }
    }
    else {
        if (sort_all) {
            std::sort(my_list, my_list + psg.data_count, cmp_probe_rel());
        }
        else {
            std::partial_sort(my_list, my_list + locs->ff_sort_max, my_list + psg.data_count, cmp_probe_rel());
        }
    }

    // destroy old list
    while (locs->ff_fl) destroy_PT_family_list(locs->ff_fl);

    // build new list
    int real_hits = 0;

    int end = (sort_all) ? psg.data_count : locs->ff_sort_max;
    for (int i = 0; i < end; i++) {
        if (my_list[i]->stat.match_count != 0) {
            PT_family_list *fl = create_PT_family_list();

            fl->name        = strdup(my_list[i]->name);
            fl->matches     = my_list[i]->stat.match_count;
            fl->rel_matches = my_list[i]->stat.rel_match_count;

            aisc_link(&locs->pff_fl, fl);
            real_hits++;
        }
    }

    locs->ff_list_size = real_hits;

    return 0;
}

inline int probe_is_ok(char *probe, int probe_len, char first_c, char second_c)
{
    /*! Check the probe for inconsistencies */
    if (probe_len < 2 || probe[0] != first_c || probe[1] != second_c)
        return 0;
    for (int i = 2; i < probe_len; i++)
        if (probe[i] < PT_A || probe[i] > PT_T)
            return 0;
    return 1;
}

inline void revert_sequence(char *seq, int len) {
    int i = 0;
    int j = len-1;

    while (i<j) std::swap(seq[i++], seq[j--]);
}

inline void complement_sequence(char *seq, int len) {
    PT_BASES complement[PT_B_MAX];
    for (PT_BASES b = PT_QU; b<PT_B_MAX; b = PT_BASES(b+1)) { complement[b] = b; }

    std::swap(complement[PT_A], complement[PT_T]);
    std::swap(complement[PT_C], complement[PT_G]);

    for (int i = 0; i<len; i++) seq[i] = complement[int(seq[i])];
}

extern "C" int ff_find_family(PT_local *locs, bytestring *species) {
    /*! make sorted list of family members of species */

    int probe_len   = locs->ff_pr_len;
    int mismatch_nr = locs->ff_mis_nr;
    int complement  = locs->ff_compl; // any combination of: 1 = forward, 2 = reverse, 4 = reverse-complement, 8 = complement

    char *sequence     = species->data; // sequence data passed by caller
    int   sequence_len = probe_compress_sequence(sequence, species->size);

    clear_statistic();

    // if ff_find_type > 0 -> search only probes starting with 'A' (quick but less accurate)
    char last_first_c = locs->ff_find_type ? PT_A : PT_T;

    // Note: code depends on order of ../AWTC/awtc_next_neighbours.hxx@FF_complement_dep
    for (int cmode = 1; cmode <= 8; cmode *= 2) {
        switch (cmode) {
            case 1:             // forward
                break;
            case 2:             // rev
            case 8:             // compl
                revert_sequence(sequence, sequence_len); // build reverse sequence
                break;
            case 4:             // revcompl
                complement_sequence(sequence, sequence_len); // build complement sequence
                break;
        }

        if ((complement&cmode) != 0) {
            for (char first_c = PT_A; first_c <= last_first_c; ++first_c) {
                for (char second_c = PT_A; second_c <= PT_T; ++second_c) {
                    char *last_probe = sequence+sequence_len-probe_len;
                    for (char *probe = sequence; probe < last_probe; ++probe) {
                        if (probe_is_ok(probe, probe_len, first_c, second_c)) {
                            MatchMarker mm;
                            mm.mark_all(psg.pt, probe, probe_len, 0, mismatch_nr);
                        }
                    }
                }
            }
        }
    }

    make_match_statistic(locs->ff_pr_len, sequence_len);
    make_PT_family_list(locs);

    free(species->data);
    return 0;
}

