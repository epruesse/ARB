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

#include "pt_split.h"
#include "probe_tree.h"

#include <arb_strbuf.h>
#include <arb_defs.h>
#include <arb_sort.h>
#include <cctype>
#include <map>

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

    inline void count_weighted(char probe, char seq, int height);
    void        count_versus(const ReadableDataLoc& loc, const char *probe, int height);

    inline bool accepted() const;

    int get_plain() const { return plain; }
    int get_ambig() const { return ambig; }

    double get_weighted() const { return weighted; }

    inline PT_local& get_PT_local() const;
};

class MatchRequest : virtual Noncopyable {
    PT_local& pt_local;

    int  max_ambig; // max. possible ambiguous hits (i.e. max value in Mismatches::ambig)
    int *accepted_N_mismatches;

    MismatchWeights weights;

    void init_accepted_N_mismatches(int ignored_Nmismatches, int when_less_than_Nmismatches);

public:
    explicit MatchRequest(PT_local& locs, int probe_length)
        : pt_local(locs),
          max_ambig(probe_length),
          accepted_N_mismatches(new int[max_ambig+1]),
          weights(locs.bond)
    {
        init_accepted_N_mismatches(pt_local.pm_nmatches_ignored, pt_local.pm_nmatches_limit);
    }
    ~MatchRequest() {
        delete [] accepted_N_mismatches;
    }

    PT_local& get_PT_local() const { return pt_local; }

    bool hit_limit_reached() const {
        bool reached = pt_local.pm_max_hits>0 && pt_local.ppm.cnt >= pt_local.pm_max_hits;
        if (reached) pt_local.matches_truncated = 1;
        return reached;
    }

    int accept_N_mismatches(int ambig) const {
        pt_assert(ambig<=max_ambig);
        return accepted_N_mismatches[ambig];
    }

    bool add_hit(const DataLoc& at, const Mismatches& mismatch);
    bool add_hits_for_children(POS_TREE2 *pt, const Mismatches& mismatch);
    bool collect_hits_for(const char *probe, POS_TREE2 *pt, Mismatches& mismatch, int height);

    int allowed_mismatches() const { return pt_local.pm_max; }
    double get_mismatch_weight(char probe, char seq) const { return weights.get(probe, seq); }
};



void MatchRequest::init_accepted_N_mismatches(int ignored_Nmismatches, int when_less_than_Nmismatches) {
    // calculate table for PT_N mismatches
    //
    // 'ignored_Nmismatches' specifies, how many N-mismatches will be accepted as
    // matches, when overall number of N-mismatches is below 'when_less_than_Nmismatches'.
    //
    // above that limit, every N-mismatch counts as mismatch

    when_less_than_Nmismatches = std::min(when_less_than_Nmismatches, max_ambig+1);
    ignored_Nmismatches        = std::min(ignored_Nmismatches, when_less_than_Nmismatches-1);

    accepted_N_mismatches[0] = 0;
    int mm;
    for (mm = 1; mm<when_less_than_Nmismatches; ++mm) { // LOOP_VECTORIZED
        accepted_N_mismatches[mm] = mm>ignored_Nmismatches ? mm-ignored_Nmismatches : 0;
    }
    pt_assert(mm <= (max_ambig+1));
    for (; mm <= max_ambig; ++mm) {
        accepted_N_mismatches[mm] = mm;
    }
}

inline void Mismatches::count_weighted(char probe, char seq, int height) {
    bool is_ambig = is_ambig_base(probe) || is_ambig_base(seq);
    if (is_ambig || probe != seq) {
        if (is_ambig) ambig++; else plain++;
        weighted += req.get_mismatch_weight(probe, seq) * psg.pos_to_weight[height];
    }
}

inline bool Mismatches::accepted() const {
    if (get_PT_local().sort_by == PT_MATCH_TYPE_INTEGER) {
        return (req.accept_N_mismatches(ambig)+plain) <= req.allowed_mismatches();
    }
    return weighted <= (req.allowed_mismatches()+0.5);
}

inline PT_local& Mismatches::get_PT_local() const {
    return req.get_PT_local();
}

bool MatchRequest::add_hit(const DataLoc& at, const Mismatches& mismatch) {
    PT_probematch *ml = create_PT_probematch();

    ml->name  = at.get_name();
    ml->b_pos = at.get_abs_pos();
    ml->g_pos = -1;
    ml->rpos  = at.get_rel_pos();

    ml->mismatches   = mismatch.get_plain()  + accept_N_mismatches(mismatch.get_ambig());
    ml->wmismatches  = mismatch.get_weighted();
    ml->N_mismatches = mismatch.get_ambig();

    ml->sequence = psg.main_probe;
    ml->reversed = psg.reversed ? 1 : 0;

    aisc_link(&get_PT_local().ppm, ml);

    return hit_limit_reached();
}

bool MatchRequest::add_hits_for_children(POS_TREE2 *pt, const Mismatches& mismatch) {
    //! go down the tree to chains and leafs; copy names, positions and mismatches in locs structure

    pt_assert(pt && mismatch.accepted()); // invalid or superfluous call
    pt_assert(!hit_limit_reached());

    bool enough = false;
    switch (pt->get_type()) {
        case PT2_LEAF:
            enough = add_hit(DataLoc(pt), mismatch);
            break;

        case PT2_CHAIN: {
            ChainIteratorStage2 entry(pt);
            while (entry && !enough) {
                enough = add_hit(DataLoc(entry.at()), mismatch);
                ++entry;
            }
            break;
        }
        case PT2_NODE:
            for (int base = PT_QU; base < PT_BASES && !enough; base++) {
                POS_TREE2 *son  = PT_read_son(pt, (PT_base)base);
                if (son) enough = add_hits_for_children(son, mismatch);
            }
            break;
    }
    return enough;
}

void Mismatches::count_versus(const ReadableDataLoc& loc, const char *probe, int height) {
    int base;
    while ((base = probe[height])) {
        int ref = loc[height];
        if (ref == PT_QU) break;

        count_weighted(base, ref, height);
        height++;
    }

    if (base != PT_QU) { // not end of probe
        pt_assert(loc[height] == PT_QU); // at EOS
        do {
            count_weighted(base, PT_QU, height);
            height++;
        }
        while ((base = probe[height]));
    }
}

bool MatchRequest::collect_hits_for(const char *probe, POS_TREE2 *pt, Mismatches& mismatches, const int height) {
    //! search down the tree to find matching species for the given probe

    pt_assert(pt && mismatches.accepted()); // invalid or superfluous call
    pt_assert(!hit_limit_reached());

    bool enough = false;
    if (probe[height] == PT_QU) {
        enough = add_hits_for_children(pt, mismatches);
    }
    else {
        switch (pt->get_type()) {
            case PT2_LEAF: {
                ReadableDataLoc loc(pt);
                mismatches.count_versus(loc, probe, height);
                if (mismatches.accepted()) {
                    enough = add_hit(loc, mismatches);
                }
                break;
            }
            case PT2_CHAIN: {
                pt_assert(probe);

                ChainIteratorStage2 entry(pt);
                while (entry && !enough) {
                    Mismatches entry_mismatches(mismatches);
                    DataLoc dloc(entry.at()); // @@@ EXPENSIVE_CONVERSION
                    entry_mismatches.count_versus(ReadableDataLoc(dloc), probe, height); // @@@ EXPENSIVE_CONVERSION
                    if (entry_mismatches.accepted()) {
                        enough = add_hit(dloc, entry_mismatches);
                    }
                    ++entry;
                }
                break;
            }
            case PT2_NODE:
                for (int i=PT_QU; i<PT_BASES && !enough; i++) {
                    POS_TREE2 *son = PT_read_son(pt, (PT_base)i);
                    if (son) {
                        Mismatches son_mismatches(mismatches);
                        son_mismatches.count_weighted(probe[height], i, height);
                        if (son_mismatches.accepted()) {
                            if (i == PT_QU) {
                                // @@@ calculation here is constant for a fixed probe (cache results)
                                pt_assert(probe[height] != PT_QU);

                                int son_height = height+1;
                                while (1) {
                                    int base = probe[son_height];
                                    if (base == PT_QU) {
                                        if (son_mismatches.accepted()) {
                                            enough = add_hits_for_children(son, son_mismatches);
                                        }
                                        break;
                                    }

                                    son_mismatches.count_weighted(base, PT_QU, son_height);
                                    if (!son_mismatches.accepted()) break;

                                    ++son_height;
                                }
                            }
                            else {
                                enough = collect_hits_for(probe, son, son_mismatches, height+1);
                            }
                        }
                    }
                }
                break;
        }
    }

    return enough;
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
    for (p=0; p<slen; p++) { // LOOP_VECTORIZED=4 (no idea why this is instantiated 4 times. inline would cause 2)
        if (type == PT_MATCH_TYPE_WEIGHTED_PLUS_POS) {
            psg.pos_to_weight[p] = calc_position_wmis(p, slen, 0.3, 1.0);
        }
        else {
            psg.pos_to_weight[p] = 1.0;
        }
    }
    psg.pos_to_weight[slen] = 0;
}

static std::map<PT_local*,Splits> splits_for_match_overlay; // initialized by probe-match, used by match-retrieval (one entry for each match-request); @@@ leaks.. 1 entry for each request

int probe_match(PT_local *locs, aisc_string probestring) {
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

    int  probe_len = strlen(probestring);
    bool failed    = false;
    if (probe_len<MIN_PROBE_LENGTH) {
        pt_export_error(locs, GBS_global_string("Min. probe length is %i", MIN_PROBE_LENGTH));
        failed = true;
    }
    else {
        int max_poss_mismatches = probe_len/2;
        pt_assert(max_poss_mismatches>0);
        if (locs->pm_max > max_poss_mismatches) {
            pt_export_error(locs, GBS_global_string("Max. %i mismatch%s are allowed for probes of length %i",
                                                    max_poss_mismatches,
                                                    max_poss_mismatches == 1 ? "" : "es",
                                                    probe_len));
            failed = true;
        }
    }

    if (!failed) {
        if (locs->pm_complement) {
            complement_probe(probestring, probe_len);
        }
        psg.reversed = 0;

        freedup(locs->pm_sequence, probestring);
        psg.main_probe = locs->pm_sequence;

        pt_build_pos_to_weight((PT_MATCH_TYPE)locs->sort_by, probestring);

        MatchRequest req(*locs, probe_len);

        pt_assert(req.allowed_mismatches() >= 0); // till [8011] value<0 was used to trigger "new match" (feature unused)
        Mismatches mismatch(req);
        req.collect_hits_for(probestring, psg.TREE_ROOT2(), mismatch, 0);

        if (locs->pm_reversed) {
            psg.reversed  = 1;
            char *rev_pro = create_reversed_probe(probestring, probe_len);
            complement_probe(rev_pro, probe_len);
            freeset(locs->pm_csequence, psg.main_probe = strdup(rev_pro));

            Mismatches rev_mismatch(req);
            req.collect_hits_for(rev_pro, psg.TREE_ROOT2(), rev_mismatch, 0);
            free(rev_pro);
        }
        pt_sort_match_list(locs);
        splits_for_match_overlay[locs] = Splits(locs);
    }
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
        if (format.show_ecoli) set_max(GBS_global_string("%li", PT_abs_2_ecoli_rel(ml->b_pos+1)), format.ecoli_width);
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

const char *get_match_overlay(const PT_probematch *ml) {
    int       pr_len = strlen(ml->sequence);
    PT_local *locs   = (PT_local *)ml->mh.parent->parent;

    const int CONTEXT_SIZE = 9;

    char *ref = (char *)calloc(sizeof(char), CONTEXT_SIZE+1+pr_len+1+CONTEXT_SIZE+1);
    memset(ref, '.', CONTEXT_SIZE+1);

    SmartCharPtr  seqPtr = psg.data[ml->name].get_dataPtr();
    const char   *seq    = &*seqPtr;

    const Splits& splits = splits_for_match_overlay[locs];

    for (int pr_pos  = CONTEXT_SIZE-1, al_pos = ml->rpos-1;
         pr_pos     >= 0 && al_pos >= 0;
         pr_pos--, al_pos--)
    {
        if (!seq[al_pos]) break;
        ref[pr_pos] = base_2_readable(seq[al_pos]);
    }
    ref[CONTEXT_SIZE] = '-';

    pt_build_pos_to_weight((PT_MATCH_TYPE)locs->sort_by, ml->sequence);

    bool display_right_context = true;
    {
        char *pref = ref+CONTEXT_SIZE+1;

        for (int pr_pos = 0, al_pos = ml->rpos;
             pr_pos < pr_len && al_pos < psg.data[ml->name].get_size();
             pr_pos++, al_pos++)
        {
            int ps = ml->sequence[pr_pos]; // probe sequence
            int ts = seq[al_pos];          // target sequence (hit)
            if (ps == ts) {
                pref[pr_pos] = '=';
            }
            else {
                if (ts) {
                    int r = base_2_readable(ts);
                    if (is_std_base(ps) && is_std_base(ts)) {
                        double h = splits.check(ml->sequence[pr_pos], ts);
                        if (h>=0.0) r = tolower(r); // if mismatch does not split probe into two domains -> show as lowercase
                    }
                    pref[pr_pos] = r;
                }
                else {
                    // end of sequence or missing data (dots inside sequence) reached
                    // (rest of probe was accepted by N-matches)
                    display_right_context = false;
                    for (; pr_pos < pr_len; pr_pos++) {
                        pref[pr_pos]  = '.';
                    }
                }
            }

        }
    }

    {
        char *cref = ref+CONTEXT_SIZE+1+pr_len+1;
        cref[-1]   = '-';

        int al_size = psg.data[ml->name].get_size();
        int al_pos  = ml->rpos+pr_len;

        if (display_right_context) {
            for (int pr_pos = 0;
                 pr_pos < CONTEXT_SIZE && al_pos < al_size;
                 pr_pos++, al_pos++)
            {
                cref[pr_pos] = base_2_readable(seq[al_pos]);
            }
        }
        else {
            if (al_pos < al_size) strcpy(cref, "<more>");
        }
    }

    static char *result = 0;
    freeset(result, ref);
    return result;
}

const char* get_match_acc(const PT_probematch *ml) {
    return psg.data[ml->name].get_acc();
}
int get_match_start(const PT_probematch *ml) {
    return psg.data[ml->name].get_start();
}
int get_match_stop(const PT_probematch *ml) {
    return psg.data[ml->name].get_stop();
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
        cat_spaced_right(memfile, GBS_global_string("%li", PT_abs_2_ecoli_rel(ml->b_pos+1)), format.ecoli_width);
    }
    cat_spaced_left(memfile, GBS_global_string("%i", ml->reversed), format.rev_width());

    GBS_strcat(memfile, get_match_overlay(ml));

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
        long gene_pos = psg.data[ml->name].get_geneabspos();
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
    /*! Create list of species where probe matches and append number of mismatches and weighted mismatches (used by multiprobe)
     *
     * Format: "header^1name^1#mismatches^1#wmismatches^1name^1#mismatches^1#wmismatches....^0"
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
        GBS_strcat(memfile, psg.data[i].get_shortname());
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

struct EnterStage2 {
    EnterStage2() {
        PT_init_psg();
        psg.enter_stage(STAGE2);
    }
    ~EnterStage2() {
        PT_exit_psg();
    }
};

#define TEST_WEIGHTED_MISMATCH(probe,seq,expected) TEST_EXPECT_SIMILAR(weights.get(probe,seq), expected, EPS)

void TEST_weighted_mismatches() {
    EnterStage2 stage2;
    PT_bond bonds[16] = {
        { 0.0 }, { 0.0 }, { 0.5 }, { 1.1 },
        { 0.0 }, { 0.0 }, { 1.5 }, { 0.0 },
        { 0.5 }, { 1.5 }, { 0.4 }, { 0.9 },
        { 1.1 }, { 0.0 }, { 0.9 }, { 0.0 },
    };

    MismatchWeights weights(bonds);

    double EPS = 0.0001;

    TEST_WEIGHTED_MISMATCH(PT_A, PT_A, 0.0);
    TEST_WEIGHTED_MISMATCH(PT_A, PT_C, 1.1); // (T~A = 1.1) - (T~C = 0)
    TEST_WEIGHTED_MISMATCH(PT_A, PT_G, 0.2); // (T~A = 1.1) - (T~G = 0.9)
    TEST_WEIGHTED_MISMATCH(PT_A, PT_T, 1.1);

    TEST_WEIGHTED_MISMATCH(PT_C, PT_A, 1.0);
    TEST_WEIGHTED_MISMATCH(PT_C, PT_C, 0.0);
    TEST_WEIGHTED_MISMATCH(PT_C, PT_G, 1.1);
    TEST_WEIGHTED_MISMATCH(PT_C, PT_T, 0.6); // (G~C = 1.5) - (G~T = 0.9)

    TEST_WEIGHTED_MISMATCH(PT_G, PT_A, 1.5);
    TEST_WEIGHTED_MISMATCH(PT_G, PT_C, 1.5);
    TEST_WEIGHTED_MISMATCH(PT_G, PT_G, 0.0);
    TEST_WEIGHTED_MISMATCH(PT_G, PT_T, 1.5);

    TEST_WEIGHTED_MISMATCH(PT_T, PT_A, 1.1);
    TEST_WEIGHTED_MISMATCH(PT_T, PT_C, 1.1);
    TEST_WEIGHTED_MISMATCH(PT_T, PT_G, 0.6);
    TEST_WEIGHTED_MISMATCH(PT_T, PT_T, 0.0);


    TEST_WEIGHTED_MISMATCH(PT_N, PT_A, 0.9);
    TEST_WEIGHTED_MISMATCH(PT_N, PT_C, 0.925);
    TEST_WEIGHTED_MISMATCH(PT_N, PT_G, 0.475);
    TEST_WEIGHTED_MISMATCH(PT_N, PT_T, 0.8);

    TEST_WEIGHTED_MISMATCH(PT_A, PT_N, 0.6);
    TEST_WEIGHTED_MISMATCH(PT_C, PT_N, 0.675);
    TEST_WEIGHTED_MISMATCH(PT_G, PT_N, 1.125);
    TEST_WEIGHTED_MISMATCH(PT_T, PT_N, 0.7);

    TEST_WEIGHTED_MISMATCH(PT_N,  PT_N,  0.775);
    TEST_WEIGHTED_MISMATCH(PT_QU, PT_QU, 0.775);
    TEST_WEIGHTED_MISMATCH(PT_QU, PT_N,  0.775);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------



