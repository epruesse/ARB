// =============================================================== //
//                                                                 //
//   File      : PT_match.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "probe.h"
#include <PT_server_prototypes.h>
#include <struct_man.h>

#include "probe_tree.h"
#include "pt_prototypes.h"

#include <arb_strbuf.h>
#include <arb_defs.h>
#include <arb_sort.h>
#include <cctype>

// overloaded functions to avoid problems with type-punning:
inline void aisc_link(dll_public *dll, PT_probematch *match)   { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(match)); }

class MatchRequest;

class Mismatches {
    MatchRequest& req;

    int plain; // plain mismatch between 2 standard bases
    int ambig; // mismatch with N or '.' involved

    double weighted; // weighted mismatches

public:

    Mismatches(MatchRequest& req_) : req(req_), plain(0), ambig(0), weighted(0.0) {}
    Mismatches(const Mismatches& other) : req(other.req), plain(other.plain), ambig(other.ambig), weighted(other.weighted) {}
    DECLARE_ASSIGNMENT_OPERATOR(Mismatches);

    inline void add_plain(char probe, char seq, int height);
    inline void add_ambig(char probe, char seq, int height);

    inline bool too_many() const;

    int get_plain() const { return plain; }
    int get_ambig() const { return ambig; }
    double get_weighted() const { return weighted; }

    inline PT_local& get_PT_local() const;
};

class MatchRequest {
    PT_local& pt_local;
    int       accepted_N_mismatches[PT_POS_TREE_HEIGHT+PT_POS_SECURITY+1];

    void init_accepted_N_mismatches(int ignored_Nmismatches, int when_less_than_Nmismatches);

public:
    Mismatches global_mismatch; // @@@ make private (or better elim when PT_store_match_in is eliminated)

    explicit MatchRequest(PT_local& locs)
        : pt_local(locs),
          global_mismatch(*this)
    {
        init_accepted_N_mismatches(pt_local.pm_nmatches_ignored, pt_local.pm_nmatches_limit);
    }

    PT_local& get_PT_local() const { return pt_local; }

    bool hit_limit_reached() const {
        bool reached = pt_local.pm_max_hits>0 && pt_local.ppm.cnt >= pt_local.pm_max_hits;
        if (reached) pt_local.matches_truncated = 1;
        return reached;
    }

    int accept_N_mismatches(int ambig) const { return accepted_N_mismatches[ambig]; }

    void add_match(const DataLoc& at, const Mismatches& mismatch);
    int  add_children_as_matches(POS_TREE *pt);

    int collect_matches_for(const char *probe, POS_TREE *pt, const Mismatches& mismatch, int height);

    int allowed_mismatches() const { return pt_local.pm_max; }
};

void MatchRequest::init_accepted_N_mismatches(int ignored_Nmismatches, int when_less_than_Nmismatches) {
    // calculate table for PT_N mismatches
    //
    // 'ignored_Nmismatches' specifies, how many N-mismatches will be accepted as
    // matches, when overall number of N-mismatches is below 'when_less_than_Nmismatches'.
    //
    // above that limit, every N-mismatch counts as mismatch

    if ((when_less_than_Nmismatches-1)>PT_POS_TREE_HEIGHT) when_less_than_Nmismatches = PT_POS_TREE_HEIGHT+1;
    if (ignored_Nmismatches >= when_less_than_Nmismatches) ignored_Nmismatches = when_less_than_Nmismatches-1;

    accepted_N_mismatches[0] = 0;
    int mm;
    for (mm = 1; mm<when_less_than_Nmismatches; ++mm) {
        accepted_N_mismatches[mm] = mm>ignored_Nmismatches ? mm-ignored_Nmismatches : 0;
    }
    pt_assert(mm <= (PT_POS_TREE_HEIGHT+1));
    for (; mm <= PT_POS_TREE_HEIGHT; ++mm) {
        accepted_N_mismatches[mm] = mm;
    }
}

static double get_simple_wmismatch(const PT_bond *bond, char probe, char seq) {
    pt_assert(is_std_base(probe));
    pt_assert(is_std_base(seq));

    int complement = psg.get_complement(probe);

    int rowIdx = (complement-int(PT_A))*4;
    int maxIdx = rowIdx + probe-(int)PT_A;
    int newIdx = rowIdx + seq-(int)PT_A;

    pt_assert(maxIdx >= 0 && maxIdx < 16);
    pt_assert(newIdx >= 0 && newIdx < 16);

    double max_bind = bond[maxIdx].val;
    double new_bind = bond[newIdx].val;

    return (max_bind - new_bind);
}

static double get_weighted_N_mismatch_2nd(const PT_bond *bonds, char probe, char seq) {
    pt_assert(is_std_base(probe));
    if (is_std_base(seq)) {
        return get_simple_wmismatch(bonds, probe, seq);
    }
    double sum = 0.0;
    for (char s = PT_A; s <= PT_T; ++s) {
        sum += get_weighted_N_mismatch_2nd(bonds, probe, s);
    }
    return sum/4.0;
}

static double get_weighted_N_mismatch(const PT_bond *bonds, char probe, char seq) {
    if (is_std_base(probe)) {
        return get_weighted_N_mismatch_2nd(bonds, probe, seq);
    }
    double sum = 0.0;
    for (char p = PT_A; p <= PT_T; ++p) {
        sum += get_weighted_N_mismatch_2nd(bonds, p, seq);
    }
    return sum/4.0;
}

inline void Mismatches::add_plain(char probe, char seq, int height) {
    pt_assert(is_std_base(probe) && is_std_base(seq));
    pt_assert(probe != seq);
    plain++;
    weighted += get_simple_wmismatch(get_PT_local().bond, probe, seq) * psg.pos_to_weight[height];
}

inline void Mismatches::add_ambig(char probe, char seq, int height) {
    pt_assert(!is_std_base(probe) || !is_std_base(seq));
    ambig++;
    weighted += get_weighted_N_mismatch(get_PT_local().bond, probe, seq) * psg.pos_to_weight[height];
}

inline bool Mismatches::too_many() const {
    pt_assert(ambig <= PT_POS_TREE_HEIGHT);
    if (get_PT_local().sort_by == PT_MATCH_TYPE_INTEGER) {
        return (req.accept_N_mismatches(ambig)+plain) > req.allowed_mismatches();
    }
    return weighted > (req.allowed_mismatches()+0.5);
}

inline PT_local& Mismatches::get_PT_local() const {
    return req.get_PT_local();
}

void MatchRequest::add_match(const DataLoc& at, const Mismatches& mismatch) {
    PT_probematch *ml = create_PT_probematch();

    ml->name  = at.name;
    ml->b_pos = at.apos;
    ml->g_pos = -1;
    ml->rpos  = at.rpos;

    ml->mismatches   = mismatch.get_plain()  + accept_N_mismatches(mismatch.get_ambig());
    ml->wmismatches  = mismatch.get_weighted();
    ml->N_mismatches = mismatch.get_ambig();

    ml->sequence = psg.main_probe;
    ml->reversed = psg.reversed ? 1 : 0;

    aisc_link(&get_PT_local().ppm, ml);
}

struct PT_store_match_in { // @@@ inline; use ChainIterator instead
    MatchRequest& req;

    PT_store_match_in(MatchRequest& req_) : req(req_) {}

    int operator()(const DataLoc& matchLoc) {
        // if chain is reached copy data in locs structure

        Mismatches mismatch = req.global_mismatch;

        const char *probe = psg.probe;
        if (probe) {
            // @@@ code here is a duplicate of code in collect_matches_for (PT_NT_LEAF-branch)
            int pos    = matchLoc.rpos+psg.height;
            int height = psg.height;
            int base, ref;

            while ((base=probe[height]) && (ref = psg.data[matchLoc.name].get_data()[pos])) {
                pt_assert(base != PT_QU && ref != PT_QU); // both impl by loop-condition

                if (ref == PT_N || base == PT_N) {
                    mismatch.add_ambig(base, ref, height);
                }
                else if (ref != base) {
                    mismatch.add_plain(base, ref, height);
                }
                height++;
                pos++;
            }

            pt_assert(base == PT_QU || ref == PT_QU);

            while ((base = probe[height])) {
                mismatch.add_ambig(base, ref, height);
                height++;
            }
            if (mismatch.too_many()) return 0;
        }

        if (req.hit_limit_reached()) return 1;

        req.add_match(matchLoc, mismatch);
        return 0;
    }
};

int MatchRequest::add_children_as_matches(POS_TREE *pt) {
    //! go down the tree to chains and leafs; copy names, positions and mismatches in locs structure

    int error = 0;
    if (pt) {
        if (hit_limit_reached()) {
            error = 1;
        }
        else {
            switch (PT_read_type(pt)) {
                case PT_NT_LEAF:
                    add_match(DataLoc(pt), global_mismatch);
                    break;

                case PT_NT_CHAIN:
                    psg.probe = 0;
                    if (PT_forwhole_chain(pt, PT_store_match_in(*this))) error = 1;
                    break;

                case PT_NT_NODE:
                    for (int base = PT_QU; base < PT_B_MAX && !error; base++) {
                        error = add_children_as_matches(PT_read_son(pt, (PT_BASES)base));
                    }
                    break;

                default:
                    pt_assert(0);
                    break;
            }
        }
    }
    return error;
}

int MatchRequest::collect_matches_for(const char *probe, POS_TREE *pt, const Mismatches& mismatch, int height) {
    //! search down the tree to find matching species for the given probe

    if (!pt) return 0;
    if (mismatch.too_many()) return 0;

    if (PT_read_type(pt) == PT_NT_NODE && probe[height]) {
        for (int i=PT_N; i<PT_B_MAX; i++) {
            POS_TREE *pthelp = PT_read_son(pt, (PT_BASES)i);
            if (pthelp) {
                Mismatches new_mismatch(mismatch);
                int        base = probe[height];

                pt_assert(base != PT_QU && i != PT_QU); // impl by if-clause/loop

                if (base == PT_N || i == PT_N) {
                    new_mismatch.add_ambig(base, i, height);
                }
                else if (i != base) {
                    new_mismatch.add_plain(base, i, height);
                }
                int error = collect_matches_for(probe, pthelp, new_mismatch, height+1);
                if (error) return error;
            }
        }
        return 0;
    }

    global_mismatch = mismatch;

    if (probe[height]) {
        if (PT_read_type(pt) == PT_NT_LEAF) {
            // @@@ code here is duplicate of code in PT_store_match_in::operator()

            DataLoc loc(pt);
            int pos  = loc.rpos + height;
            int name = loc.name;

            // @@@ recursive use of strlen with constant result (argh!)
            if (pos + (int)(strlen(probe+height)) >= psg.data[name].get_size())       // end of sequence
                return 0;

            int base;
            while ((base = probe[height])) {
                int i = psg.data[name].get_data()[pos];

                pt_assert(base != PT_QU); // impl by loop

                if (i == PT_N || base == PT_N || i == PT_QU) {
                    global_mismatch.add_ambig(base, i, height);
                }
                else if (i != base) {
                    global_mismatch.add_plain(base, i, height);
                }
#if defined(DEVEL_RALF)
                pt_assert(i != PT_QU); // case not covered
#endif
                if (i != PT_QU) pos++; // dot reached -> act like sequence ends here
                height++;
            }
        }
        else {                // chain
            psg.probe  = probe;
            psg.height = height;
            PT_forwhole_chain(pt, PT_store_match_in(*this)); // @@@ why ignore result
            return 0;
        }
        if (global_mismatch.too_many()) return 0;
    }
    return add_children_as_matches(pt);
}

static int pt_sort_compare_match(const void *PT_probematch_ptr1, const void *PT_probematch_ptr2, void *) {
    const PT_probematch *mach1 = (const PT_probematch*)PT_probematch_ptr1;
    const PT_probematch *mach2 = (const PT_probematch*)PT_probematch_ptr2;

    if (psg.sort_by != PT_MATCH_TYPE_INTEGER) {
        if (mach1->wmismatches > mach2->wmismatches) return 1;
        if (mach1->wmismatches < mach2->wmismatches) return -1;
    }

    int cmp = mach1->mismatches - mach2->mismatches;
    if (!cmp) {
        cmp = mach1->N_mismatches - mach2->N_mismatches;
        if (!cmp) {
            if      (mach1->wmismatches < mach2->wmismatches) cmp = -1;
            else if (mach1->wmismatches > mach2->wmismatches) cmp = 1;
            else {
                cmp = mach1->b_pos - mach2->b_pos;
                if (!cmp) {
                    cmp = mach1->name - mach2->name;
                }
            }
        }
    }

    return cmp;
}

static void pt_sort_match_list(PT_local * locs) {
    if (locs->pm) {
        psg.sort_by = locs->sort_by;

        int list_len = locs->pm->get_count();
        if (list_len > 1) {
            PT_probematch **my_list = (PT_probematch **)calloc(sizeof(void *), list_len);
            {
                PT_probematch *match = locs->pm;
                for (int i=0; match; i++) {
                    my_list[i] = match;
                    match      = match->next;
                }
            }
            GB_sort((void **)my_list, 0, list_len, pt_sort_compare_match, 0);
            for (int i=0; i<list_len; i++) {
                aisc_unlink((dllheader_ext*)my_list[i]);
                aisc_link(&locs->ppm, my_list[i]);
            }
            free(my_list);
        }
    }
}
char *create_reversed_probe(char *probe, int len) {
    //! reverse order of bases in a probe
    char *rev_probe = GB_strduplen(probe, len);
    reverse_probe(rev_probe, len);
    return rev_probe;
}

static double calc_position_wmis(int pos, int seq_len, double y1, double y2) {
    return (double)(((double)(pos * (seq_len - 1 - pos)) / (double)((seq_len - 1) * (seq_len - 1)))* (double)(y2*4.0) + y1);
}

static void pt_build_pos_to_weight(PT_MATCH_TYPE type, const char *sequence) {
    delete [] psg.pos_to_weight;
    int slen = strlen(sequence);
    psg.pos_to_weight = new double[slen+1];
    int p;
    for (p=0; p<slen; p++) {
        if (type == PT_MATCH_TYPE_WEIGHTED_PLUS_POS) {
            psg.pos_to_weight[p] = calc_position_wmis(p, slen, 0.3, 1.0);
        }
        else {
            psg.pos_to_weight[p] = 1.0;
        }
    }
    psg.pos_to_weight[slen] = 0;
}

int probe_match(PT_local * locs, aisc_string probestring) {
    //! find out where a given probe matches

    freedup(locs->pm_sequence, probestring);
    psg.main_probe = locs->pm_sequence;

    compress_data(probestring);
    while (PT_probematch *ml = locs->pm) destroy_PT_probematch(ml);
    locs->matches_truncated = 0;

#if defined(DEBUG) && 0
    printf("Current bond values:\n");
    for (int y = 0; y<4; y++) {
        for (int x = 0; x<4; x++) {
            printf("%5.2f", locs->bond[y*4+x].val);
        }
        printf("\n");
    }
#endif // DEBUG

    int probe_len = strlen(probestring);
    if (probe_len<MIN_PROBE_LENGTH) {
        pt_export_error(locs, GBS_global_string("Min. probe length is %i", MIN_PROBE_LENGTH));
        return 0;
    }

    {
        int max_poss_mismatches = probe_len/2;
        pt_assert(max_poss_mismatches>0);
        if (locs->pm_max > max_poss_mismatches) {
            pt_export_error(locs, GBS_global_string("Max. %i mismatch%s are allowed for probes of length %i",
                                                    max_poss_mismatches,
                                                    max_poss_mismatches == 1 ? "" : "es",
                                                    probe_len));
            return 0;
        }
    }

    if (locs->pm_complement) {
        psg.complement_probe(probestring, probe_len);
    }
    psg.reversed = 0;

    freedup(locs->pm_sequence, probestring);
    psg.main_probe = locs->pm_sequence;

    pt_build_pos_to_weight((PT_MATCH_TYPE)locs->sort_by, probestring);

    MatchRequest req(*locs);

    pt_assert(req.allowed_mismatches() >= 0); // till [8011] value<0 was used to trigger "new match" (feature unused)
    req.collect_matches_for(probestring, psg.pt, Mismatches(req), 0);

    if (locs->pm_reversed) {
        psg.reversed  = 1;
        char *rev_pro = create_reversed_probe(probestring, probe_len);
        psg.complement_probe(rev_pro, probe_len);
        freeset(locs->pm_csequence, psg.main_probe = strdup(rev_pro));

        req.collect_matches_for(rev_pro, psg.pt, Mismatches(req), 0);
        free(rev_pro);
    }
    pt_sort_match_list(locs);
    free(probestring);

    return 0;
}

struct format_props {
    bool show_mismatches;       // whether to show 'mis' and 'N_mis'
    bool show_ecoli;            // whether to show 'ecoli' column
    bool show_gpos;             // whether to show 'gpos' column

    int name_width;             // width of 'name' column
    int gene_or_full_width;     // width of 'genename' or 'fullname' column
    int pos_width;              // max. width of pos column
    int gpos_width;             // max. width of gpos column
    int ecoli_width;            // max. width of ecoli column

    int rev_width() const { return 3; }
    int mis_width() const { return 3; }
    int N_mis_width() const { return 5; }
    int wmis_width() const { return 4; }
};

inline void set_max(const char *str, int &curr_max) {
    if (str) {
        int len = strlen(str);
        if (len>curr_max) curr_max = len;
    }
}

static format_props detect_format_props(const PT_local *locs, bool show_gpos) {
    PT_probematch *ml = locs->pm; // probe matches
    format_props   format;

    format.show_mismatches = (ml->N_mismatches >= 0);
    format.show_ecoli      = psg.ecoli; // display only if there is ecoli
    format.show_gpos       = show_gpos; // display only for gene probe matches

    // minimum values (caused by header widths) :
    format.name_width         = gene_flag ? 8 : 4; // 'organism' or 'name'
    format.gene_or_full_width = 8; // 'genename' or 'fullname'
    format.pos_width          = 3; // 'pos'
    format.gpos_width         = 4; // 'gpos'
    format.ecoli_width        = 5; // 'ecoli'

    for (; ml; ml = ml->next) {
        set_max(virt_name(ml), format.name_width);
        set_max(virt_fullname(ml), format.gene_or_full_width);
        set_max(GBS_global_string("%i", info2bio(ml->b_pos)), format.pos_width);
        if (show_gpos) set_max(GBS_global_string("%i", info2bio(ml->g_pos)), format.gpos_width);
        if (format.show_ecoli) set_max(GBS_global_string("%li", PT_abs_2_rel(ml->b_pos+1)), format.ecoli_width);
    }

    return format;
}

inline void cat_internal(GBS_strstruct *memfile, int len, const char *text, int width, char spacer, bool align_left) {
    if (len == width) {
        GBS_strcat(memfile, text); // text has exact len
    }
    else if (len > width) { // text to long
        char buf[width+1];
        memcpy(buf, text, width);
        buf[width] = 0;
        GBS_strcat(memfile, buf);
    }
    else {                      // text is too short -> insert spaces
        int  spaces = width-len;
        pt_assert(spaces>0);
        char sp[spaces+1];
        memset(sp, spacer, spaces);
        sp[spaces]  = 0;

        if (align_left) {
            GBS_strcat(memfile, text);
            GBS_strcat(memfile, sp);
        }
        else {
            GBS_strcat(memfile, sp);
            GBS_strcat(memfile, text);
        }
    }
    GBS_chrcat(memfile, ' '); // one space behind each column
}
inline void cat_spaced_left (GBS_strstruct *memfile, const char *text, int width) { cat_internal(memfile, strlen(text), text, width, ' ', true); }
inline void cat_spaced_right(GBS_strstruct *memfile, const char *text, int width) { cat_internal(memfile, strlen(text), text, width, ' ', false); }
inline void cat_dashed_left (GBS_strstruct *memfile, const char *text, int width) { cat_internal(memfile, strlen(text), text, width, '-', true); }
inline void cat_dashed_right(GBS_strstruct *memfile, const char *text, int width) { cat_internal(memfile, strlen(text), text, width, '-', false); }

char *get_match_overlay(const PT_probematch *ml) {
    int       pr_len = strlen(ml->sequence);
    PT_local *locs   = (PT_local *)ml->mh.parent->parent;

    char *ref = (char *)calloc(sizeof(char), 21+pr_len);
    memset(ref, '.', 10);

    for (int pr_pos  = 8, al_pos = ml->rpos-1;
         pr_pos     >= 0 && al_pos >= 0;
         pr_pos--, al_pos--)
    {
        if (!psg.data[ml->name].get_data()[al_pos]) break;
        ref[pr_pos] = base_2_readable(psg.data[ml->name].get_data()[al_pos]);
    }
    ref[9] = '-';

    pt_build_pos_to_weight((PT_MATCH_TYPE)locs->sort_by, ml->sequence);

    for (int pr_pos = 0, al_pos = ml->rpos;
         pr_pos < pr_len && al_pos < psg.data[ml->name].get_size();
         pr_pos++, al_pos++)
    {
        int a = ml->sequence[pr_pos];
        int b = psg.data[ml->name].get_data()[al_pos];
        if (a == b) {
            ref[pr_pos+10] = '=';
        }
        else {
            if (b) {
                int r = base_2_readable(b);
                if (is_std_base(a) && is_std_base(b)) {
                    double h = ptnd_check_split(locs, ml->sequence, pr_pos, b);
                    if (h>=0.0) r = tolower(r);
                }
                ref[pr_pos+10] = r;
            }
            else {
                // end of sequence reached (rest of probe was accepted by N-matches)
                for (; pr_pos < pr_len; pr_pos++) {
                    ref[pr_pos+10]  = '.';
                }
            }
        }

    }

    for (int pr_pos = 0, al_pos = ml->rpos+pr_len;
         pr_pos < 9 && al_pos < psg.data[ml->name].get_size();
         pr_pos++, al_pos++)
    {
        ref[pr_pos+11+pr_len] = base_2_readable(psg.data[ml->name].get_data()[al_pos]);
    }
    ref[10+pr_len] = '-';
    return ref;
}

static const char *get_match_info_formatted(PT_probematch  *ml, const format_props& format) {
    GBS_strstruct *memfile = GBS_stropen(256);
    GBS_strcat(memfile, "  ");

    cat_spaced_left(memfile, virt_name(ml), format.name_width);
    cat_spaced_left(memfile, virt_fullname(ml), format.gene_or_full_width);

    if (format.show_mismatches) {
        cat_spaced_right(memfile, GBS_global_string("%i", ml->mismatches), format.mis_width());
        cat_spaced_right(memfile, GBS_global_string("%i", ml->N_mismatches), format.N_mis_width());
    }
    cat_spaced_right(memfile, GBS_global_string("%.1f", ml->wmismatches), format.wmis_width());
    cat_spaced_right(memfile, GBS_global_string("%i", info2bio(ml->b_pos)), format.pos_width);
    if (format.show_gpos) {
        cat_spaced_right(memfile, GBS_global_string("%i", info2bio(ml->g_pos)), format.gpos_width);
    }
    if (format.show_ecoli) {
        cat_spaced_right(memfile, GBS_global_string("%li", PT_abs_2_rel(ml->b_pos+1)), format.ecoli_width);
    }
    cat_spaced_left(memfile, GBS_global_string("%i", ml->reversed), format.rev_width());

    char *ref = get_match_overlay(ml);
    GBS_strcat(memfile, ref);
    free(ref);

    static char *result = 0;
    freeset(result, GBS_strclose(memfile));
    return result;
}

static const char *get_match_hinfo_formatted(PT_probematch *ml, const format_props& format) {
    if (ml) {
        GBS_strstruct *memfile = GBS_stropen(500);
        GBS_strcat(memfile, "    "); // one space more than in get_match_info_formatted()

        cat_dashed_left(memfile, gene_flag ? "organism" : "name", format.name_width);
        cat_dashed_left(memfile, gene_flag ? "genename" : "fullname", format.gene_or_full_width);

        if (format.show_mismatches) {
            cat_dashed_right(memfile, "mis",   format.mis_width());
            cat_dashed_right(memfile, "N_mis", format.N_mis_width());
        }
        cat_dashed_right(memfile, "wmis", format.wmis_width());
        cat_dashed_right(memfile, "pos", format.pos_width);
        if (format.show_gpos) {
            cat_dashed_right(memfile, "gpos", format.gpos_width);
        }
        if (format.show_ecoli) {
            cat_dashed_right(memfile, "ecoli", format.ecoli_width);
        }
        cat_dashed_left(memfile, "rev", format.rev_width());

        if (ml->N_mismatches >= 0) { //
            char *seq = strdup(ml->sequence);
            probe_2_readable(seq, strlen(ml->sequence)); // @@@ maybe wrong if match contains PT_QU (see [9070])

            GBS_strcat(memfile, "         '");
            GBS_strcat(memfile, seq);
            GBS_strcat(memfile, "'");

            free(seq);
        }

        static char *result = 0;
        freeset(result, GBS_strclose(memfile));

        return result;
    }
    // Else set header of result
    return "There are no targets";
}

static void gene_rel_2_abs(PT_probematch *ml) {
    /*! after gene probe match all positions are gene-relative.
     * gene_rel_2_abs() makes them genome-absolute.
     */

    GB_transaction ta(psg.gb_main);

    for (; ml; ml = ml->next) {
        long gene_pos = psg.data[ml->name].get_abspos();
        if (gene_pos >= 0) {
            ml->g_pos  = ml->b_pos;
            ml->b_pos += gene_pos;
        }
        else {
            fprintf(stderr, "Error in gene-pt-server: gene w/o position info\n");
            pt_assert(0);
        }
    }
}

bytestring *match_string(const PT_local *locs) {
    /*! Create list of species where probe matches.
     *
     * header^1name^1info^1name^1info....^0
     *         (where ^0 and ^1 are ASCII 0 and 1)
     *
     * Implements server function 'MATCH_STRING'
     */
    static bytestring bs = { 0, 0 };
    GBS_strstruct *memfile;
    PT_probematch *ml;

    free(bs.data);

    char empty[] = "";
    bs.data      = empty;
    memfile      = GBS_stropen(50000);

    if (locs->pm) {
        if (gene_flag) gene_rel_2_abs(locs->pm);

        format_props format = detect_format_props(locs, gene_flag);

        GBS_strcat(memfile, get_match_hinfo_formatted(locs->pm, format));
        GBS_chrcat(memfile, (char)1);

        for (ml = locs->pm; ml;  ml = ml->next) {
            GBS_strcat(memfile, virt_name(ml));
            GBS_chrcat(memfile, (char)1);
            GBS_strcat(memfile, get_match_info_formatted(ml, format));
            GBS_chrcat(memfile, (char)1);
        }
    }

    bs.data = GBS_strclose(memfile);
    bs.size = strlen(bs.data)+1;
    return &bs;
}




bytestring *MP_match_string(const PT_local *locs) {
    /*! Create list of species where probe matches and #mismatches (for multiprobe)
     *
     * Format: header^1name^1#mismatch^1name^1#mismatch....^0
     *         (where ^0 and ^1 are ASCII 0 and 1)
     *
     * Implements server function 'MP_MATCH_STRING'
     */
    static bytestring bs = { 0, 0 };

    char           buffer[50];
    char           buffer1[50];
    GBS_strstruct *memfile;
    PT_probematch *ml;

    delete bs.data;
    bs.data = 0;
    memfile = GBS_stropen(50000);

    for (ml = locs->pm; ml;  ml = ml->next)
    {
        GBS_strcat(memfile, virt_name(ml));
        GBS_chrcat(memfile, (char)1);
        sprintf(buffer, "%2i", ml->mismatches);
        GBS_strcat(memfile, buffer);
        GBS_chrcat(memfile, (char)1);
        sprintf(buffer1, "%1.1f", ml->wmismatches);
        GBS_strcat(memfile, buffer1);
        GBS_chrcat(memfile, (char)1);
    }
    bs.data = GBS_strclose(memfile);
    bs.size = strlen(bs.data)+1;
    return &bs;
}


bytestring *MP_all_species_string(const PT_local *) {
    /*! Create list of all species known to PT server
     *
     * Format: ^1name^1name....^0
     *         (where ^0 and ^1 are ASCII 0 and 1)
     *
     * Implements server function 'MP_ALL_SPECIES_STRING'
     */
    static bytestring bs = { 0, 0 };

    GBS_strstruct *memfile;
    int            i;

    delete bs.data;
    bs.data = 0;
    memfile = GBS_stropen(1000);

    for (i = 0; i < psg.data_count; i++)
    {
        GBS_strcat(memfile, psg.data[i].get_name());
        GBS_chrcat(memfile, (char)1);
    }

    bs.data = GBS_strclose(memfile);
    bs.size = strlen(bs.data)+1;
    return &bs;
}

int MP_count_all_species(const PT_local *) {
    return psg.data_count;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

struct EnterStage3 {
    EnterStage3() {
        PT_init_psg();
        psg.ptdata = PT_init(STAGE3);
    }
    ~EnterStage3() {
        PT_exit_psg();
    }
};

#define TEST_SIMPLE_WEIGHTED_MISMATCH(probe,seq,expected) TEST_ASSERT_SIMILAR(get_simple_wmismatch(bonds,probe,seq), expected, EPS)
#define TEST_AMBIG__WEIGHTED_MISMATCH(probe,seq,expected) TEST_ASSERT_SIMILAR(get_weighted_N_mismatch(bonds,probe,seq), expected, EPS)

void TEST_weighted_mismatches() {
    EnterStage3 stage3;
    PT_bond bonds[16] = {
        { 0.0 }, { 0.0 }, { 0.5 }, { 1.1 },
        { 0.0 }, { 0.0 }, { 1.5 }, { 0.0 },
        { 0.5 }, { 1.5 }, { 0.4 }, { 0.9 },
        { 1.1 }, { 0.0 }, { 0.9 }, { 0.0 },
    };

    double EPS = 0.0001;

    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_A, PT_A, 0.0);
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_A, PT_C, 1.1); // (T~A = 1.1) - (T~C = 0)
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_A, PT_G, 0.2); // (T~A = 1.1) - (T~G = 0.9)
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_A, PT_T, 1.1);

    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_C, PT_A, 1.0);
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_C, PT_C, 0.0);
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_C, PT_G, 1.1);
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_C, PT_T, 0.6); // (G~C = 1.5) - (G~T = 0.9)

    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_G, PT_A, 1.5);
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_G, PT_C, 1.5);
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_G, PT_G, 0.0);
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_G, PT_T, 1.5);

    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_T, PT_A, 1.1);
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_T, PT_C, 1.1);
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_T, PT_G, 0.6);
    TEST_SIMPLE_WEIGHTED_MISMATCH(PT_T, PT_T, 0.0);


    TEST_AMBIG__WEIGHTED_MISMATCH(PT_N, PT_A, 0.9);
    TEST_AMBIG__WEIGHTED_MISMATCH(PT_N, PT_C, 0.925);
    TEST_AMBIG__WEIGHTED_MISMATCH(PT_N, PT_G, 0.475);
    TEST_AMBIG__WEIGHTED_MISMATCH(PT_N, PT_T, 0.8);

    TEST_AMBIG__WEIGHTED_MISMATCH(PT_A, PT_N, 0.6);
    TEST_AMBIG__WEIGHTED_MISMATCH(PT_C, PT_N, 0.675);
    TEST_AMBIG__WEIGHTED_MISMATCH(PT_G, PT_N, 1.125);
    TEST_AMBIG__WEIGHTED_MISMATCH(PT_T, PT_N, 0.7);

    TEST_AMBIG__WEIGHTED_MISMATCH(PT_N, PT_N, 0.775);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------


