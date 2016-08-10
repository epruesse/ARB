// =============================================================== //
//                                                                 //
//   File      : MG_adapt_ali.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "merge.hxx"
#include "MG_adapt_ali.hxx"

#include <aw_msg.hxx>
#include <arbdbt.h>
#include <arb_strbuf.h>

#include <cmath>
#include <list>

#if defined(DEBUG)
// #define DUMP_MAPPING
// #define DUMP_SOFTMAPPING
#endif // DEBUG

// -----------------
//      MG_remap

const int NO_POSITION  = -1;
const int LEFT_BORDER  = -2;
const int RIGHT_BORDER = -3;

struct softbase {
    char base;
    int  origin;                                    // position in source alignment
    char last_gapchar;                              // last gap seen before base
    int  targetpos;                                 // target position

    softbase(char base_, int origin_, char last_gapchar_)
        : base(base_)
        , origin(origin_)
        , last_gapchar(last_gapchar_)
        , targetpos(NO_POSITION)
    {
        mg_assert(last_gapchar);
    }

    operator char () const { return base; }
};

typedef std::list<softbase>          softbaselist;
typedef softbaselist::iterator       softbaseiter;
typedef softbaselist::const_iterator const_softbaseiter;

class MG_remap : virtual Noncopyable {
    int  in_length;
    int  out_length;
    int *remap_tab;                                 // fixed mapping (targetPosition or NO_POSITION)

    // soft-mapping:
    int *soft_remap_tab;                            // soft-mapping (NO_POSITION, targetPosition, LEFT_BORDER or RIGHT_BORDER)

    char *calc_softmapping(softbaselist& softbases, int start, int end, int &outlen);
    int   softmap_to(softbaselist& softbases, int start, int end, GBS_strstruct *outs);

    bool have_softmapping() const { return soft_remap_tab; }
    void create_softmapping();
    void forget_softmapping() {
        delete [] soft_remap_tab;
        soft_remap_tab = NULL;
    }
    
    static int *build_initial_mapping(const char *iref, int ilen, const char *oref, int olen);
    void merge_mapping(MG_remap &other, int& inconsistent, int& added);

public:

    MG_remap()
        : in_length(0)
        , out_length(0)
        , remap_tab(NULL)
        , soft_remap_tab(NULL)
    {}
    ~MG_remap() {
        forget_softmapping();
        delete [] remap_tab;
    }

    const char *add_reference(const char *in_reference, const char *out_reference); // returns only warnings
    char       *remap(const char *sequence);        // returns 0 on error, else copy of sequence

#if defined(DUMP_MAPPING)
    static void dump(const int *data, int len, const char *comment, int dontShow) {
        fflush(stdout);
        fflush(stderr);
        fputc('>', stdout);
        int digits = calc_signed_digits(len);
        for (int pos = 0; pos<len; ++pos) {
            if (data[pos] == dontShow) {
                fprintf(stdout, "%*s", digits, "_");
            }
            else {
                fprintf(stdout, "%*i", digits, data[pos]);
            }
        }
        fprintf(stdout, "      (%s)\n", comment);
        fflush(stdout);
    }
    void dump_remap(const char *comment) { dump(remap_tab, in_length, comment, NO_POSITION); }
#endif // DUMP_MAPPING
};

int *MG_remap::build_initial_mapping(const char *iref, int ilen, const char *oref, int olen) {
    int *remap = new int[ilen];

    const char *spacers = "-. n";
    
    int ipos = 0;
    int opos = 0;

    while (ipos<ilen && opos<olen) {
        size_t ispaces = strspn(iref+ipos, spacers);
        size_t ospaces = strspn(oref+opos, spacers);

        while (ispaces && ipos<ilen) {
            remap[ipos++] = NO_POSITION;
            ispaces--;
        }
        opos += ospaces;
        if (ipos<ilen && opos<olen) remap[ipos++] = opos++;
    }
    while (ipos<ilen) remap[ipos++] = NO_POSITION;

    return remap;
}

#if defined(DEBUG)
// #define DUMP_INCONSISTENCIES
#endif // DEBUG


void MG_remap::merge_mapping(MG_remap &other, int& inconsistent, int& added) {
    const int *primary       = remap_tab;
    int       *secondary     = other.remap_tab;
    bool       increased_len = false;

    if (other.in_length>in_length) {
        // re-use memory of bigger map
        std::swap(other.in_length, in_length);
        std::swap(other.remap_tab, remap_tab);

        increased_len = true;
    }
    out_length = std::max(out_length, other.out_length); // remember biggest output length

    int mixlen = std::min(in_length, other.in_length);

    // eliminate inconsistant positions from secondary mapping (forward)
    inconsistent = 0;
    {
        int max_target_pos = 0;
        for (int pos = 0; pos<mixlen; ++pos) {
            max_target_pos = std::max(max_target_pos, primary[pos]);
            if (secondary[pos]<max_target_pos) {
                if (secondary[pos] != NO_POSITION) {
#if defined(DUMP_INCONSISTENCIES) 
                    fprintf(stderr, "merge-inconsistency: pos=%i primary[]=%i secondary[]=%i max_target_pos=%i\n",
                            pos, primary[pos], secondary[pos], max_target_pos);
#endif // DUMP_INCONSISTENCIES
                    inconsistent++;
                }
                secondary[pos] = NO_POSITION;       // consistency error -> ignore position
            }
        }
    }
    // (backward)
    {
        int min_target_pos = out_length-1;
        for (int pos = mixlen-1; pos >= 0; --pos) {
            if (primary[pos] >= 0 && primary[pos]<min_target_pos) min_target_pos = primary[pos];
            if (secondary[pos] > min_target_pos) {
#if defined(DUMP_INCONSISTENCIES)
                fprintf(stderr, "merge-inconsistency: pos=%i primary[]=%i secondary[]=%i min_target_pos=%i\n",
                        pos, primary[pos], secondary[pos], min_target_pos);
#endif // DUMP_INCONSISTENCIES
                inconsistent++;
                secondary[pos] = NO_POSITION;       // consistency error -> ignore position
            }
        }
    }

    // merge mappings
    added = 0;
    for (int pos = 0; pos < mixlen; ++pos) {
        if (primary[pos] == NO_POSITION) {
            remap_tab[pos]  = secondary[pos];
            added          += (remap_tab[pos] != NO_POSITION);
        }
        else {
            remap_tab[pos] = primary[pos];
        }
        // remap_tab[pos] = primary[pos] == NO_POSITION ? secondary[pos] : primary[pos];
        mg_assert(remap_tab[pos]<out_length);
    }

    if (increased_len) { // count positions appended at end of sequence
        for (int pos = other.in_length; pos<in_length; ++pos) {
            added += (remap_tab[pos] != NO_POSITION);
        }
    }

    // Note: copying the rest from larger mapping is not necessary
    // (it's already there, because of memory-reuse)
}

const char *MG_remap::add_reference(const char *in_reference, const char *out_reference) {
    // returns warning about inconsistencies/useless reference
    const char *warning = NULL;

    if (have_softmapping()) forget_softmapping();

    if (!remap_tab) {
        in_length  = strlen(in_reference);
        out_length = strlen(out_reference);
        remap_tab  = build_initial_mapping(in_reference, in_length, out_reference, out_length);
#if defined(DUMP_MAPPING)
        dump_remap("initial");
#endif // DUMP_MAPPING
    }
    else {
        MG_remap tmp;
        tmp.add_reference(in_reference, out_reference);

        int inconsistent, added;
        merge_mapping(tmp, inconsistent, added);

        if (inconsistent>0) warning = GBS_global_string("contains %i inconsistent positions", inconsistent);
        if (added<1) {
            const char *useless_warning = "doesn't add useful information";
            if (warning) warning        = GBS_global_string("%s and %s", warning, useless_warning);
            else        warning         = useless_warning;
        }

#if defined(DUMP_MAPPING)
        dump_remap("merged");
#endif // DUMP_MAPPING
    }

    return warning;
}

void MG_remap::create_softmapping() {
    soft_remap_tab = new int[in_length];

    int last_fixed_position = NO_POSITION;
    int pos;
    for (pos = 0; pos<in_length && last_fixed_position == NO_POSITION; ++pos) {
        if (remap_tab[pos] == NO_POSITION) {
            soft_remap_tab[pos] = LEFT_BORDER;
        }
        else {
            soft_remap_tab[pos] = NO_POSITION;
            last_fixed_position = pos;
        }
    }
    if (last_fixed_position != NO_POSITION) {
        for ( ; pos<in_length; ++pos) {
            if (remap_tab[pos] != NO_POSITION) {
                int softstart = last_fixed_position+1;
                int softsize  = pos-softstart;

                if (softsize>0) {
                    int target_softstart = remap_tab[last_fixed_position]+1;
                    int target_softsize  = remap_tab[pos]-target_softstart;

                    double target_step;
                    if (softsize>1 && target_softsize>1) {
                        target_step = double(target_softsize-1)/(softsize-1);
                    }
                    else {
                        target_step = 0.0;
                    }

                    if (target_step >= 1.0 && target_softsize>softsize) {
                        // target range > source range -> split softmapping in the middle
                        int halfsoftsize   = softsize/2;
                        int target_softpos = softstart;
                        int off;
                        for (off = 0; off<halfsoftsize; ++off) {
                            soft_remap_tab[softstart+off] = target_softpos++;
                        }
                        target_softpos += target_softsize-softsize;
                        for (; off<softsize; ++off) {
                            soft_remap_tab[softstart+off] = target_softpos++;
                        }
                    }
                    else {
                        double target_softpos = target_softstart;
                        for (int off = 0; off<softsize; ++off) {
                            soft_remap_tab[softstart+off]  = int(target_softpos+0.5);
                            target_softpos                += target_step;
                        }
                    }
                }
                last_fixed_position = pos;
                soft_remap_tab[pos] = NO_POSITION;
            }
        }

        for (--pos; pos>last_fixed_position; --pos) {
            soft_remap_tab[pos] = RIGHT_BORDER;
        }
    }

#if defined(DUMP_MAPPING)
    dump(soft_remap_tab, in_length, "softmap", -1);
#endif // DUMP_MAPPING
}

static void drop_dots(softbaselist& softbases, int excessive_positions) {
    // drop consecutive dots
    bool justseendot = false;
    
    softbaseiter next = softbases.begin();
    while (excessive_positions && next != softbases.end()) {
        bool isdot = (next->base == '.');
        if (isdot && justseendot) {
            excessive_positions--;
            next = softbases.erase(next);
        }
        else {
            justseendot = isdot;
            ++next;
        }
    }

    if (excessive_positions) {
        // drop single dots
        next = softbases.begin();
        while (excessive_positions && next != softbases.end()) {
            if (next->base == '.') {
                next = softbases.erase(next);
            }
            else {
                ++next;
            }
        }
    }
}

#if defined(DUMP_SOFTMAPPING)
static char *softbaselist_2_string(const softbaselist& softbases) {
    GBS_strstruct *out = GBS_stropen(softbases.size()*100);

    for (const_softbaseiter base = softbases.begin(); base != softbases.end(); ++base) {
        const char *info = GBS_global_string(" %c'%c' %i->",
                                             base->last_gapchar ? base->last_gapchar :  ' ',
                                             base->base,
                                             base->origin);
        GBS_strcat(out, info);
        if (base->targetpos == NO_POSITION) {
            GBS_strcat(out, "NP");
        }
        else {
            GBS_intcat(out, base->targetpos);
        }
    }

    return GBS_strclose(out);
}
#endif // DUMP_SOFTMAPPING

char *MG_remap::calc_softmapping(softbaselist& softbases, int start, int end, int& outlen) {
    //! tries to map all bases(+dots) in 'softbases' into range [start, end]
    //! @return heap-copy of range-content (may be oversized, if too many bases in list)

    int wanted_size = end-start+1;
    int listsize    = softbases.size();

#if defined(DUMP_SOFTMAPPING)
    char *sbl_initial = softbaselist_2_string(softbases);
    char *sbl_exdots  = NULL;
    char *sbl_target  = NULL;
    char *sbl_exclash = NULL;
#endif // DUMP_SOFTMAPPING

    if (listsize>wanted_size) {
        int excessive_positions = listsize-wanted_size;
        drop_dots(softbases, excessive_positions);
        listsize                = softbases.size();

#if defined(DUMP_SOFTMAPPING)
        sbl_exdots = softbaselist_2_string(softbases);
#endif // DUMP_SOFTMAPPING
    }

    char *result = NULL;
    if (listsize >= wanted_size) {                  // not or just enough space -> plain copy
        result = (char*)ARB_alloc(listsize+1);
        *std::copy(softbases.begin(), softbases.end(), result) = 0;
        outlen = listsize;
    }
    else {                                          // otherwise do soft-mapping
        result = (char*)ARB_alloc(wanted_size+1);
        mg_assert(listsize < wanted_size);

        // calculate target positions and detect mapping conflicts
        bool conflicts = false;
        {
            int lasttargetpos = NO_POSITION;
            for (softbaseiter base = softbases.begin(); base != softbases.end(); ++base) {
                // // int targetpos = calc_softpos(base->origin);
                int targetpos = soft_remap_tab[base->origin];
                if (targetpos == lasttargetpos) {
                    mg_assert(targetpos != NO_POSITION);
                    conflicts = true;
                }
                base->targetpos = lasttargetpos = targetpos;
            }
        }

#if defined(DUMP_SOFTMAPPING)
        sbl_target = softbaselist_2_string(softbases);
#endif // // DUMP_SOFTMAPPING
      
        if (conflicts) {
            int nextpos = end+1;
            for (softbaselist::reverse_iterator base = softbases.rbegin(); base != softbases.rend(); ++base) {
                if (base->targetpos >= nextpos) {
                    base->targetpos = nextpos-1;
                }
                nextpos = base->targetpos;
                mg_assert(base->targetpos >= start);
                mg_assert(base->targetpos <= end);
            }
            mg_assert(nextpos >= start);
          
#if defined(DUMP_SOFTMAPPING)
            sbl_exclash = softbaselist_2_string(softbases);
#endif // // DUMP_SOFTMAPPING
         }

        int idx = 0;
        for (softbaseiter base = softbases.begin(); base != softbases.end(); ++base) {
            int pos = base->targetpos - start;

            if (idx<pos) {
                char gapchar = base->last_gapchar;
                while (idx<pos) result[idx++] = gapchar;
            }
            result[idx++] = base->base;
        }
        result[idx] = 0;
        outlen      = idx;
        mg_assert(idx <= wanted_size);
    }

#if defined(DUMP_SOFTMAPPING)
    fflush(stdout);
    fflush(stderr);
    printf("initial:%s\n", sbl_initial);
    if (sbl_exdots) printf("exdots :%s\n", sbl_exdots);
    if (sbl_target) printf("target :%s\n", sbl_target);
    if (sbl_exclash) printf("exclash:%s\n", sbl_exclash);
    printf("calc_softmapping(%i, %i) -> \"%s\"\n", start, end, result);
    fflush(stdout);
    free(sbl_exclash);
    free(sbl_target);
    free(sbl_exdots);
    free(sbl_initial);
#endif // DUMP_SOFTMAPPING

    return result;
}

int MG_remap::softmap_to(softbaselist& softbases, int start, int end, GBS_strstruct *outs) {
    int   mappedlen;
    char *softmapped = calc_softmapping(softbases, start, end, mappedlen);

    GBS_strcat(outs, softmapped);
    free(softmapped);
    softbases.clear();

    return mappedlen;
}

char *MG_remap::remap(const char *sequence) {
    int            slen    = strlen(sequence);
    int            minlen  = std::min(slen, in_length);
    GBS_strstruct *outs    = GBS_stropen(out_length+1);
    int            written = 0;
    softbaselist   softbases;
    int            pos;

    if (!have_softmapping()) create_softmapping();

    // remap left border
    for (pos = 0; pos<in_length && soft_remap_tab[pos] == LEFT_BORDER; ++pos) {
        char c = pos<slen ? sequence[pos] : '-';
        if (c != '-' && c != '.') {
            softbases.push_back(softbase(c, pos, '.'));
        }
    }
    char last_gapchar = 0;
    {
        int bases = softbases.size();
        if (bases) {
            int fixed = remap_tab[pos];
            mg_assert(fixed != NO_POSITION);

            int bases_start = fixed-bases;
            if (written<bases_start) {
                GBS_chrncat(outs, '.', bases_start-written);
                written = bases_start;
            }

            written += softmap_to(softbases, written, fixed-1, outs);
        }
        else {
            last_gapchar = '.';
        }
    }


    // remap center (fixed mapping and softmapping inbetween)
    for (; pos<in_length; ++pos) {
        char c          = pos<slen ? sequence[pos] : '-';
        int  target_pos = remap_tab[pos];

        if (target_pos == NO_POSITION) {            // softmap
            if (c == '-') {
                if (!last_gapchar) last_gapchar = '-';
            }
            else {
                if (!last_gapchar) last_gapchar = c == '.' ? '.' : '-';

                softbases.push_back(softbase(c, pos, last_gapchar)); // remember bases for softmapping
                if (c != '.') last_gapchar = 0;
            }
        }
        else {                                      // fixed mapping
            if (!softbases.empty()) {
                written += softmap_to(softbases, written, target_pos-1, outs);
            }
            if (written<target_pos) {
                if (!last_gapchar) last_gapchar = c == '.' ? '.' : '-';

                GBS_chrncat(outs, last_gapchar, target_pos-written);
                written = target_pos;
            }

            if (c == '-') {
                if (!last_gapchar) last_gapchar = '-';
                GBS_chrcat(outs, last_gapchar); written++;
            }
            else {
                GBS_chrcat(outs, c); written++;
                if (c != '.') last_gapchar = 0;
            }
        }
    }

    // Spool leftover softbases.
    // Happens when
    //  - sequence ends before last fixed position
    //  - sequence starts after last fixed position

    if (!softbases.empty()) {
        // softmap_to(softbases, written, written+softbases.size()-1, outs);
        softmap_to(softbases, written, written, outs);
    }
    mg_assert(softbases.empty());

    // copy overlength rest of sequence:
    const char *gap_chars = "- .";
    for (int i = minlen; i < slen; i++) {
        int c = sequence[i];
        if (!strchr(gap_chars, c)) GBS_chrcat(outs, c); // don't fill with gaps
    }

    // convert end of seq ('-' -> '.')
    {
        char *out = GBS_mempntr(outs);
        int  end = GBS_memoffset(outs);

        for (int off = end-1; off >= 0; --off) {
            if (out[off] == '-') out[off] = '.';
            else if (out[off] != '.') break;
        }
    }

    return GBS_strclose(outs);
}

// --------------------------------------------------------------------------------

static MG_remap *MG_create_remap(GBDATA *gb_left, GBDATA *gb_right, const char *reference_species_names, const char *alignment_name) {
    MG_remap *rem      = new MG_remap;
    char     *ref_list = ARB_strdup(reference_species_names);

    for (char *tok = strtok(ref_list, " \n,;"); tok; tok = strtok(NULL, " \n,;")) {
        bool    is_SAI           = strncmp(tok, "SAI:", 4) == 0;
        GBDATA *gb_species_left  = 0;
        GBDATA *gb_species_right = 0;

        if (is_SAI) {
            gb_species_left  = GBT_find_SAI(gb_left, tok+4);
            gb_species_right = GBT_find_SAI(gb_right, tok+4);
        }
        else {
            gb_species_left  = GBT_find_species(gb_left, tok);
            gb_species_right = GBT_find_species(gb_right, tok);
        }

        if (!gb_species_left || !gb_species_right) {
            aw_message(GBS_global_string("Warning: Couldn't find %s'%s' in %s DB.\nPlease read ADAPT ALIGNMENT section in help file!",
                                         is_SAI ? "" : "species ",
                                         tok,
                                         gb_species_left ? "right" : "left"));
        }
        else {
            // look for sequence/SAI "data"
            GBDATA *gb_seq_left  = GBT_find_sequence(gb_species_left, alignment_name);
            GBDATA *gb_seq_right = GBT_find_sequence(gb_species_right, alignment_name);

            if (gb_seq_left && gb_seq_right) {
                GB_TYPES type_left  = GB_read_type(gb_seq_left);
                GB_TYPES type_right = GB_read_type(gb_seq_right);

                if (type_left != type_right) {
                    aw_message(GBS_global_string("Warning: data type of '%s' differs in both databases", tok));
                }
                else {
                    const char *warning;
                    if (type_right == GB_STRING) {
                        warning = rem->add_reference(GB_read_char_pntr(gb_seq_left), GB_read_char_pntr(gb_seq_right));
                    }
                    else {
                        char *sleft  = GB_read_as_string(gb_seq_left);
                        char *sright = GB_read_as_string(gb_seq_right);

                        warning = rem->add_reference(sleft, sright);

                        free(sleft);
                        free(sright);
                    }

                    if (warning) aw_message(GBS_global_string("Warning: '%s' %s", tok, warning));
                }
            }
        }
    }
    free(ref_list);

    return rem;
}

// --------------------------------------------------------------------------------

MG_remaps::MG_remaps(GBDATA *gb_left, GBDATA *gb_right, bool enable, const char *reference_species_names)
    : n_remaps(0), 
      remaps(NULL)
{
    if (enable) { // otherwise nothing will be remapped!
        GBT_get_alignment_names(alignment_names, gb_left);
        for (n_remaps = 0; alignment_names[n_remaps]; n_remaps++) {} // count alignments

        ARB_calloc(remaps, n_remaps);
        for (int i = 0; i<n_remaps; ++i) {
            remaps[i] = MG_create_remap(gb_left, gb_right, reference_species_names, alignment_names[i]);
        }
    }
}

MG_remaps::~MG_remaps() {
    for (int i=0; i<n_remaps; i++) delete remaps[i];
    free(remaps);
}

static GB_ERROR MG_transfer_sequence(MG_remap *remap, GBDATA *source_species, GBDATA *destination_species, const char *alignment_name) {
    // align sequence after copy
    GB_ERROR error = NULL;

    if (remap) {                                    // shall remap?
        GBDATA *gb_seq_left  = GBT_find_sequence(source_species,      alignment_name);
        GBDATA *gb_seq_right = GBT_find_sequence(destination_species, alignment_name);

        if (gb_seq_left && gb_seq_right) {          // if one DB hasn't sequence -> sequence was not copied
            char *ls = GB_read_string(gb_seq_left);
            char *rs = GB_read_string(gb_seq_right);

            if (strcmp(ls, rs) == 0) {              // if sequences are not identical -> sequence was not copied
                free(rs);
                rs = remap->remap(ls);

                if (!rs) error = GB_await_error();
                else {
                    long old_check = GBS_checksum(ls, 0, ".- ");
                    long new_check = GBS_checksum(rs, 0, ".- ");

                    if (old_check == new_check) {
                        error = GB_write_string(gb_seq_right, rs);
                    }
                    else {
                        error = GBS_global_string("Failed to adapt alignment of '%s' (checksum changed)", GBT_read_name(source_species));
                    }
                }
            }
            free(rs);
            free(ls);
        }
    }
    return error;
}

GB_ERROR MG_transfer_all_alignments(MG_remaps *remaps, GBDATA *source_species, GBDATA *destination_species) {
    GB_ERROR error = NULL;
    
    if (remaps->remaps) {
        for (int i=0; i<remaps->n_remaps && !error; i++) {
            error = MG_transfer_sequence(remaps->remaps[i], source_species, destination_species, remaps->alignment_names[i]);
        }
    }
    
    return error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

// #define VERBOOSE_REMAP_TESTS

#if defined(VERBOOSE_REMAP_TESTS)
#define DUMP_REMAP(id, comment, from, to) fprintf(stderr, "%s %s '%s' -> '%s'\n", id, comment, from, to)
#define DUMP_REMAP_STR(str)               fputs(str, stderr)
#else
#define DUMP_REMAP(id, comment, from, to)
#define DUMP_REMAP_STR(str)
#endif

#define TEST_REMAP(id, remapper, ASS_EQ, seqold, expected, got) \
    char *calculated = remapper.remap(seqold);                  \
    DUMP_REMAP(id, "want", seqold, expected);                   \
    DUMP_REMAP(id, "calc", seqold, calculated);                 \
    ASS_EQ(calculated, expected, got);                          \
    DUMP_REMAP_STR("\n");                                       \
    free(calculated);

#define TEST_REMAP1REF_INT(id, ASS_EQ, refold, refnew, seqold, expected, got)   \
    do {                                                                        \
        MG_remap  remapper;                                                     \
        DUMP_REMAP(id, "ref ", refold, refnew);                                 \
        remapper.add_reference(refold, refnew);                                 \
        TEST_REMAP(id, remapper, ASS_EQ, seqold, expected, got);                \
    } while(0)

#define TEST_REMAP2REFS_INT(id, ASS_EQ, refold1, refnew1, refold2, refnew2, seqold, expected, got)      \
    do {                                                                                                \
        MG_remap remapper;                                                                              \
        DUMP_REMAP(id, "ref1", refold1, refnew1);                                                       \
        DUMP_REMAP(id, "ref2", refold2, refnew2);                                                       \
        remapper.add_reference(refold1, refnew1);                                                       \
        remapper.add_reference(refold2, refnew2);                                                       \
        TEST_REMAP(id, remapper, ASS_EQ, seqold, expected, got);                                        \
    } while(0)

#define TEST_REMAP1REF(id,ro,rn,seqold,expected)             TEST_REMAP1REF_INT(id, TEST_EXPECT_EQUAL__IGNARG, ro, rn, seqold, expected, NULL)
#define TEST_REMAP1REF__BROKEN(id,ro,rn,seqold,expected,got) TEST_REMAP1REF_INT(id, TEST_EXPECT_EQUAL__BROKEN, ro, rn, seqold, expected, got)

#define TEST_REMAP2REFS(id,ro1,rn1,ro2,rn2,seqold,expected) TEST_REMAP2REFS_INT(id, TEST_EXPECT_EQUAL__IGNARG, ro1, rn1, ro2, rn2, seqold, expected, NULL)

#define TEST_REMAP1REF_FWDREV(id, ro, rn, so, sn)       \
    TEST_REMAP1REF(id "(fwd)", ro, rn, so, sn);         \
    TEST_REMAP1REF(id "(rev)", rn, ro, sn, so)
    
#define TEST_REMAP1REF_ALLDIR(id, ro, rn, so, sn)               \
    TEST_REMAP1REF_FWDREV(id "/ref=ref", ro, rn, so, sn);       \
    TEST_REMAP1REF_FWDREV(id "/ref=src", so, sn, ro, rn)

#define TEST_REMAP2REFS_FWDREV(id, ro1, rn1, ro2, rn2, so, sn)  \
    TEST_REMAP2REFS(id "(fwd)", ro1, rn1, ro2, rn2, so, sn);    \
    TEST_REMAP2REFS(id "(rev)", rn1, ro1, rn2, ro2, sn, so)

#define TEST_REMAP2REFS_ALLDIR(id, ro1, rn1, ro2, rn2, so, sn)          \
    TEST_REMAP2REFS_FWDREV(id "/ref=r1+r2 ", ro1, rn1, ro2, rn2, so, sn); \
    TEST_REMAP2REFS_FWDREV(id "/ref=r1+src", ro1, rn1, so, sn, ro2, rn2); \
    TEST_REMAP2REFS_FWDREV(id "/ref=r2+src", ro2, rn2, so, sn, ro1, rn1)

void TEST_remapping() {
    // ----------------------------------------
    // remap with 1 reference

    TEST_REMAP1REF_ALLDIR("simple gap",
                          "ACGT", "AC--GT",
                          "TGCA", "TG--CA");

    TEST_REMAP1REF_FWDREV("dotgap",
                          "ACGT", "AC..GT",         // dots in reference do not get propagated
                          "TGCA", "TG--CA");

    TEST_REMAP1REF("unused position (1)",
                   "A-C-G-T", "A--C--G--T",
                   "A---G-T", "A-----G--T");
    TEST_REMAP1REF("unused position (2)",
                   "A-C-G-T", "A--C--G--T",
                   "Az-z-zT", "Az--z--z-T");
                          
    TEST_REMAP1REF("empty (1)",
                   "A-C-G", "A--C--G",
                   "",      ".......");
    TEST_REMAP1REF("empty (2)",
                   "...A-C-G...", "...A--C--G...",
                   "",            "..........");

    TEST_REMAP1REF("leadonly",
                   "...A-C-G...", "...A--C--G...",
                   "XX",          ".XX.......");
    TEST_REMAP1REF("trailonly",
                   "...A-C-G...", "...A--C--G...",
                   ".........XX", "..........XX");
    TEST_REMAP1REF__BROKEN("lead+trail",
                           "...A-C-G...", "...A--C--G...",
                           "XX.......XX", ".XX-------XX",
                                          ".XX.......XX");

    TEST_REMAP1REF("enforce leading dots (1)",
                   "--A-T", "------A--T",
                   "--T-A", "......T--A");          // leading gaps shall always be dots
    TEST_REMAP1REF("enforce leading dots (2)",
                   "---A-T", "------A--T",
                   "-.-T-A", "......T--A");         // leading gaps shall always be dots
    TEST_REMAP1REF("enforce leading dots (3)",
                   "---A-T", "------A--T",
                   "...T-A", "......T--A");         // leading gaps shall always be dots
    TEST_REMAP1REF("enforce leading dots (4)",
                   "-..--A-T", "--.---A--T",
                   "-.-.-T-A", "......T--A");       // leading gaps shall always be dots
    TEST_REMAP1REF("enforce leading dots (5)",
                   "---A-T-", "---A----T",
                   "-----A-", "........A");       // leading gaps shall always be dots
    TEST_REMAP1REF("enforce leading dots (6)",
                   "---A-T-", "---A----T",
                   ".....A-", "........A");       // leading gaps shall always be dots

    TEST_REMAP1REF("no trailing gaps",
                   "A-T", "A--T---",
                   "T-A", "T--A");

    TEST_REMAP1REF("should expand full-dot-gaps (1)",
                   "AC-GT", "AC--GT",
                   "TG.CA", "TG..CA");              // if gap was present and only contained dots -> new gap should only contain dots as well
    TEST_REMAP1REF("should expand full-dot-gaps (2)",
                   "AC--GT", "AC----GT",
                   "TG..CA", "TG....CA");           // if gap was present and only contained dots -> new gap should only contain dots as well

    TEST_REMAP1REF("keep 'missing bases' (1)",
                   "AC---GT", "AC---GT",
                   "TG-.-CA", "TG-.-CA"); // missing bases should not be deleted

    TEST_REMAP1REF("keep 'missing bases' (2)",
                   "AC-D-GT", "AC-D-GT",
                   "TG-.-CA", "TG-.-CA"); // missing bases should not be deleted

    // ----------------------------------------
    // remap with 2 references
    TEST_REMAP2REFS_ALLDIR("simple 3-way",
                           "A-CA-C-T", "AC-A---CT",
                           "A---GC-T", "A----G-CT",
                           "T-GAC--A", "TG-A-C--A");

    TEST_REMAP2REFS("undefpos-nogap(2U)",
                    "---A", "------A",
                    "C---", "C------",
                    "GUUG", "GU---UG");
    TEST_REMAP2REFS("undefpos-nogap(3U)",
                    "C----", "C------",
                    "----A", "------A",
                    "GUUUG", "GU--UUG");
    TEST_REMAP2REFS("undefpos-nogap(4U)",
                    "-----A", "------A",
                    "C-----", "C------",
                    "GUUUUG", "GUU-UUG");
    TEST_REMAP2REFS("undefpos-nogap(4U2)",
                    "-----A", "-------A",
                    "C-----", "C-------",
                    "GUUUUG", "GUU--UUG");

    TEST_REMAP2REFS("undefpos1-gapleft",
                    "----A", "------A",
                    "C----", "C------",
                    "G-UUG", "G---UUG");
    TEST_REMAP2REFS("undefpos1-gapright",
                    "----A", "------A",
                    "C----", "C------",
                    "GUU-G", "GU--U-G");            // behavior changed (prior: "GUU---G")

    // test non-full sequences
    TEST_REMAP2REFS("missing ali-pos (ref1-source)",
                    "C",  "C--",                    // in-seq missing 1 ali pos
                    "-A", "--A",
                    "GG", "G-G");

    TEST_REMAP2REFS("missing ali-pos (ref2-source)",
                    "-A", "--A",
                    "C",  "C--",                    // in-seq missing 1 ali pos
                    "GG", "G-G");

    TEST_REMAP2REFS("missing ali-pos (ref1-target)",
                    "C-", "C",                      // out-seq missing 2 ali pos
                    "-A", "--A",
                    "GG", "G-G");
    TEST_REMAP2REFS("missing ali-pos (ref2-target)",
                    "-A", "--A",
                    "C-", "C",                      // out-seq missing 2 ali pos
                    "GG", "G-G");

    TEST_REMAP2REFS("missing ali-pos (seq-source)",
                    "C--", "C--",
                    "-A-", "--A",
                    "GG",  "G-G");                  // in-seq missing 1 ali pos

    // --------------------
    // test (too) small gaps

    TEST_REMAP1REF("gap gets too small (1)",
                   "A---T---A", "A--T--A",
                   "AGGGT---A", "AGGGT-A");

    TEST_REMAP1REF("gap gets too small (2)",
                   "A---T---A", "A--T--A",
                   "AGGGT--GA", "AGGGTGA");

    TEST_REMAP1REF("gap gets too small (3)",
                   "A---T---A", "A--T--A",
                   "AGGGT-G-A", "AGGGTGA");

    TEST_REMAP1REF("gap gets too small (4)",
                   "A---T---A", "A--T--A",
                   "AGGGTG--A", "AGGGTGA");
                   
    TEST_REMAP1REF("gap tightens to fit (1)",
                   "A---T---A", "A--T--A",
                   "AGG-T---A", "AGGT--A");

    TEST_REMAP1REF("gap tightens to fit (2)",
                   "A---T---A", "A--T--A",
                   "AGC-T---A", "AGCT--A");
    TEST_REMAP1REF("gap tightens to fit (2)",
                   "A---T---A", "A--T--A",
                   "A-CGT---A", "ACGT--A");

    TEST_REMAP1REF("gap with softmapping conflict (1)",
                   "A-------A", "A----A",
                   "A-CGT---A", "ACGT-A");
    TEST_REMAP1REF("gap with softmapping conflict (2)",
                   "A-------A", "A----A",
                   "A--CGT--A", "ACGT-A");
    TEST_REMAP1REF("gap with softmapping conflict (3)",
                   "A-------A", "A----A",
                   "A---CGT-A", "A-CGTA");
    TEST_REMAP1REF("gap with softmapping conflict (4)",
                   "A-------A", "A----A",
                   "A----CGTA", "A-CGTA");
    TEST_REMAP1REF("gap with softmapping conflict (5)",
                   "A-------A", "A----A",
                   "AC----GTA", "AC-GTA");
                   
    TEST_REMAP1REF("drop missing bases to avoid misalignment (1)",
                   "A---T", "A--T",
                   "AG.GT", "AGGT");
    TEST_REMAP1REF("drop missing bases to avoid misalignment (2)",
                   "A----T", "A--T",
                   "AG..GT", "AGGT");
    TEST_REMAP1REF("dont drop missing bases if fixed map",
                   "A-C-T", "A-CT",
                   "AG.GT", "AG.GT");
    TEST_REMAP1REF("dont drop gaps if fixed map",
                   "A-C-T", "A-CT",
                   "AG-GT", "AG-GT");

    // --------------------

    TEST_REMAP1REF("append rest of seq (1)",
                   "AT"            , "A--T",
                   "AGG---T...A...", "A--GGTA");
    TEST_REMAP1REF("append rest of seq (2)",
                   "AT."           , "A--T--.",
                   "AGG---T...A...", "A--GGTA");
    TEST_REMAP1REF("append rest of seq (3)",
                   "AT--"          , "A--T---",
                   "AGG---T...A...", "A--GGTA");
    TEST_REMAP1REF("append rest of seq (4)",
                   "ATC"           , "A--T--C",
                   "AGG---T...A...", "A--G--GTA");
    TEST_REMAP1REF("append rest of seq (4)",
                   "AT-C"          , "A--T--C",
                   "AGG---T...A...", "A--GG--TA");

    // --------------------

    TEST_REMAP2REFS("impossible references",
                    "AC-TA", "A---CT--A",           // impossible references
                    "A-GTA", "AG---T--A",
                    "TGCAT", "T---GCA-T");          // solve by insertion (partial misalignment)

    TEST_REMAP2REFS("impossible references",
                    "AC-TA", "A--C-T--A",           // impossible references
                    "A-GTA", "AG---T--A",
                    "TGCAT", "T--GCA--T");          // solve by insertion (partial misalignment)
}

#endif // UNIT_TESTS
