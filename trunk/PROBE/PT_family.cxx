#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
// #include <malloc.h>
#include <PT_server.h>
#include <struct_man.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "probe_tree.hxx"
#include "pt_prototypes.h"
#include <arbdbt.h>

// overloaded functions to avoid problems with type-punning:
inline void aisc_link(dll_public *dll, PT_family_list *family)   { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(family)); }

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Increment match_count for a matched chain */

struct mark_all_matches_chain_handle {
    int operator()(int name, int /*pos*/, int /*rpos*/) {
        psg.data[name].stat.match_count++;
        return 0;
    }
};

/* Increment match_count for every match */
static int mark_all_matches( PT_local *locs,
                             POS_TREE *pt,
                             char     *probe,
                             int       length,
                             int       mismatches,
                             int       height,
                             int       max_mismatches)
{
    int       ref_pos, ref2_pos, ref_name;
    int       base;
    int       type_of_node;
    POS_TREE *pt_son;
    
    if (pt == NULL || mismatches > max_mismatches) {
        return 0;
    }
    type_of_node = PT_read_type(pt);
    if ((type_of_node = PT_read_type(pt)) != PT_NT_NODE) {
        /*
         * Found unique solution
         */
        if (type_of_node == PT_NT_LEAF) {
            ref2_pos = PT_read_rpos(psg.ptmain,pt);
            ref_pos = ref2_pos + height;
            ref_name = PT_read_name(psg.ptmain,pt);
            /*
             * Check rest of probe
             */
            while (mismatches <= max_mismatches && *probe) {
                if (psg.data[ref_name].data[ref_pos++] != *probe) {
                    mismatches++;
                }
                probe++;
                height++;
            }
            /*
             * Increment count if probe matches
             */
            if (mismatches <= max_mismatches)
                psg.data[ref_name].stat.match_count++;
            return 0;
        } else {        /* type_of_node == CHAIN !! */
            psg.mismatches = mismatches;
            psg.height = height;
            psg.length = length;
            psg.probe = probe;
            PT_read_chain(psg.ptmain,pt,mark_all_matches_chain_handle());
            return 0;
        }
    }
    for (base = PT_N; base < PT_B_MAX; base++) {
        if ( (pt_son = PT_read_son(psg.ptmain, pt, (PT_BASES)base))) {
            if (*probe) {
                if (*probe != base) {
                    mark_all_matches(locs, pt_son, probe+1, length,
                                     mismatches + 1, height + 1, max_mismatches);
                } else {
                    mark_all_matches(locs, pt_son, probe+1, length,
                                     mismatches, height + 1, max_mismatches);
                }
            } else {
                mark_all_matches(locs, pt_son, probe, length,
                                 mismatches, height + 1, max_mismatches);
            }
        }
    }
    return 0;
}

/* Clear all informations in psg.data[i].stat */
static void clear_statistic(){
    int i;
    for (i = 0; i < psg.data_count; i++)
        memset((char *) &psg.data[i].stat,0,sizeof(struct probe_statistic));
}


/* Calculate the statistic informations for the family */
static void make_match_statistic(int probe_len){
    int i;
    /*
     * compute statistic for all species in family
     */
    for (i = 0; i < psg.data_count; i++) {
        int all_len = psg.data[i].size - probe_len + 1;
        if (all_len <= 0){
            psg.data[i].stat.rel_match_count = 0;
        }else{
            psg.data[i].stat.rel_match_count = psg.data[i].stat.match_count / (double) (all_len);
        }
    }
}

extern "C" {
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
}

/*  Make sorted list of family members */
static int make_PT_family_list(PT_local *locs) {
    // Sort the data
    struct probe_input_data **my_list = (struct probe_input_data**) calloc(sizeof(void *),psg.data_count);
    int i;
    for (i = 0; i < psg.data_count; i++) my_list[i] = &psg.data[i];

    if (locs->ff_sort_type == 0) {
        std::sort(my_list, my_list + psg.data_count, cmp_probe_abs());
    }
    else {
        std::sort(my_list, my_list + psg.data_count, cmp_probe_rel());
    }
    
    // destroy old list
    while(locs->ff_fl) destroy_PT_family_list(locs->ff_fl);

    // build new list
    int real_hits = 0;

    for (i = 0; i < psg.data_count; i++) {
        if (my_list[i]->stat.match_count != 0) {
            PT_family_list *fl = create_PT_family_list();

            fl->name        = strdup(my_list[i]->name);
            fl->matches     = my_list[i]->stat.match_count;
            fl->rel_matches = my_list[i]->stat.rel_match_count;

            aisc_link(&locs->pff_fl, fl);
            real_hits++;
        }
    }
    free((char *)my_list);

    locs->ff_list_size = real_hits;

    return 0;
}

/* Check the probe for inconsitencies */
inline int probe_is_ok(char *probe, int probe_len, char first_c, char second_c)
{
    int i;
    if (probe_len < 2 || probe[0] != first_c || probe[1] != second_c)
        return 0;
    for (i = 2; i < probe_len; i++)
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

/* make sorted list of family members of species */
extern "C" int ff_find_family(PT_local *locs, bytestring *species) {
    int probe_len   = locs->ff_pr_len;
    int mismatch_nr = locs->ff_mis_nr;
    int complement  = locs->ff_compl; // any combination of: 1 = forward, 2 = reverse, 4 = reverse-complement, 8 = complement 

    char *sequence     = species->data;
    int   sequence_len = probe_compress_sequence(sequence);

    clear_statistic();

    // if ff_find_type > 0 -> search only probes starting with 'A' (quick but less accurate)
    char last_first_c = locs->ff_find_type ? PT_A : PT_T;

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
                            mark_all_matches(locs,psg.pt,probe,probe_len,0,0,mismatch_nr);
                        }
                    }
                }
            }
        }
    }

    make_match_statistic(locs->ff_pr_len);
    make_PT_family_list(locs);
    
    free(species->data);
    return 0;
}
