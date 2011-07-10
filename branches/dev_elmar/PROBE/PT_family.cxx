// =============================================================== //
//                                                                 //
//   File      : PT_family.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "PT_rangeCheck.h"
#include "pt_prototypes.h"

#include <struct_man.h>
#include <PT_server_prototypes.h>
#include <arbdbt.h>

#include <algorithm>

// overloaded functions to avoid problems with type-punning:
inline void aisc_link(dll_public *dll, PT_family_list *family)   { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(family)); }

class ProbeTraversal {
    static Range range;

    static void count_match(const DataLoc& match) {
        if (range.contains(match)) {
            ++psg.data[match.name].stat.match_count;
        }
    }

    // ----------------------------------------
    
    const char *probe;
    int         height;
    int         needed_positions;
    int         accept_mismatches;

    bool at_end() const { return *probe == PT_QU; }

    bool too_many_mismatches() const { return accept_mismatches<0; }

    bool did_match() const { return needed_positions <= 0 && !too_many_mismatches(); }
    bool need_match() const { return needed_positions > 0 && !too_many_mismatches(); }
    bool match_possible() const { return need_match() && !at_end(); }

    void match_one_char(char c) {
        pt_assert(match_possible()); // avoid unneeded calls 
        
        if (*probe++ != c) accept_mismatches--;
        needed_positions--;
        height++;
    }

    void match_rest_and_mark(const DataLoc& loc) {
        do match_one_char(psg.data[loc.name].data[loc.rpos+height]); while (match_possible());
        if (did_match()) count_match(loc);
    }

    void mark_all(POS_TREE *pt) const;

public:

    static void restrictMatchesToRegion(int start, int end, int probe_len) {
        range  = Range(start, end, probe_len);
    }
    static void unrestrictMatchesToRegion() {
        range  = Range(-1, -1, -1);
    }
    
    ProbeTraversal(const char *probe_, int needed_positions_, int accept_mismatches_)
        : probe(probe_),
          height(0),
          needed_positions(needed_positions_),
          accept_mismatches(accept_mismatches_)
    { }

    void mark_matching(POS_TREE *pt) const;

    int operator()(const DataLoc& loc) const { 
        /*! Increment match_count for matched postree-tips */
        if (did_match()) count_match(loc);
        else if (match_possible()) {
            ProbeTraversal(*this).match_rest_and_mark(loc);
        }
        return 0;
    }
};

Range ProbeTraversal::range(-1, -1, -1);

void ProbeTraversal::mark_matching(POS_TREE *pt) const {
    /*! Traverse pos(sub)tree through matching branches and increment 'match_count' */

    pt_assert(pt);
    pt_assert(!too_many_mismatches());
    pt_assert(!did_match()); 

    if (PT_read_type(pt) == PT_NT_NODE) {
        for (int base = PT_N; base < PT_B_MAX; base++) {
            POS_TREE *pt_son = PT_read_son(psg.ptmain, pt, (PT_BASES)base);
            if (pt_son && !at_end()) {
                ProbeTraversal sub(*this);
                sub.match_one_char(base);
                if (!sub.too_many_mismatches()) {
                    if (sub.did_match()) sub.mark_all(pt_son);
                    else sub.mark_matching(pt_son);
                }
            }
        }
    }
    else {
        PT_withall_tips(psg.ptmain, pt, *this); // calls operator() 
    }
}

void ProbeTraversal::mark_all(POS_TREE *pt) const {
    pt_assert(pt);
    pt_assert(!too_many_mismatches());
    pt_assert(did_match());

    if (PT_read_type(pt) == PT_NT_NODE) {
        for (int base = PT_N; base < PT_B_MAX; base++) {
            POS_TREE *pt_son = PT_read_son(psg.ptmain, pt, (PT_BASES)base);
            if (pt_son) mark_all(pt_son);
        }
    }
    else {
        PT_withall_tips(psg.ptmain, pt, *this); // calls operator() 
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

static int make_PT_family_list(PT_family *ffinder) {
    /*!  Make sorted list of family members */

    // Sort the data
    struct probe_input_data *my_list[psg.data_count];
    for (int i = 0; i < psg.data_count; i++) {
        my_list[i] = &psg.data[i];
    }

    bool sort_all = ffinder->sort_max == 0 || ffinder->sort_max >= psg.data_count;

    if (ffinder->sort_type == 0) {
        if (sort_all) {
            std::sort(my_list, my_list + psg.data_count, cmp_probe_abs());
        }
        else {
            std::partial_sort(my_list, my_list + ffinder->sort_max, my_list + psg.data_count, cmp_probe_abs());
        }
    }
    else {
        if (sort_all) {
            std::sort(my_list, my_list + psg.data_count, cmp_probe_rel());
        }
        else {
            std::partial_sort(my_list, my_list + ffinder->sort_max, my_list + psg.data_count, cmp_probe_rel());
        }
    }

    // destroy old list
    while (ffinder->fl) destroy_PT_family_list(ffinder->fl);

    // build new list
    int real_hits = 0;

    int end = (sort_all) ? psg.data_count : ffinder->sort_max;
    for (int i = 0; i < end; i++) {
        if (my_list[i]->stat.match_count != 0) {
            PT_family_list *fl = create_PT_family_list();

            fl->name        = strdup(my_list[i]->name);
            fl->matches     = my_list[i]->stat.match_count;
            fl->rel_matches = my_list[i]->stat.rel_match_count;

            aisc_link(&ffinder->pfl, fl);
            real_hits++;
        }
    }

    ffinder->list_size = real_hits;

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

int find_family(PT_family *ffinder, bytestring *species) {
    /*! make sorted list of family members of species */

    int probe_len   = ffinder->pr_len;
    int mismatch_nr = ffinder->mis_nr;
    int complement  = ffinder->complement; // any combination of: 1 = forward, 2 = reverse, 4 = reverse-complement, 8 = complement

    char *sequence     = species->data; // sequence data passed by caller
    int   sequence_len = probe_compress_sequence(sequence, species->size);

    clear_statistic();

    // if find_type > 0 -> search only probes starting with 'A' (quick but less accurate)
    char last_first_c = ffinder->find_type ? PT_A : PT_T;

    ProbeTraversal::restrictMatchesToRegion(ffinder->range_start, ffinder->range_end, probe_len);

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
                            ProbeTraversal(probe, probe_len, mismatch_nr).mark_matching(psg.pt);
                        }
                    }
                }
            }
        }
    }

    make_match_statistic(ffinder->pr_len, sequence_len);
    make_PT_family_list(ffinder);

    ProbeTraversal::unrestrictMatchesToRegion();
    
    free(species->data);
    return 0;
}

