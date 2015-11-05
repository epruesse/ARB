// =============================================================== //
//                                                                 //
//   File      : fast_aligner.cxx                                  //
//   Purpose   : A fast aligner (not a multiple aligner!)          //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 1998           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#include "fast_aligner.hxx"
#include "ClustalV.hxx"
#include "seq_search.hxx"

#include <island_hopping.h>

#include <awtc_next_neighbours.hxx>
#include <awt_sel_boxes.hxx>

#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>

#include <arbdbt.h>
#include <ad_cb.h>

#include <arb_defs.h>
#include <arb_progress.h>
#include <RangeList.h>

#include <cctype>
#include <cmath>
#include <climits>

#include <list>
#include <awt_config_manager.hxx>
#include <rootAsWin.h>

// --------------------------------------------------------------------------------

#if defined(DEBUG)
// #define TRACE_CLUSTAL_DATA
// #define TRACE_ISLANDHOPPER_DATA
// #define TRACE_COMPRESSED_ALIGNMENT
// #define TRACE_RELATIVES
#endif // DEBUG

// --------------------------------------------------------------------------------

enum FA_report {
    FA_NO_REPORT,               // no report
    FA_TEMP_REPORT,             // report to temporary entries
    FA_REPORT,                  // report to resident entries
};

enum FA_range {
    FA_WHOLE_SEQUENCE,          // align whole sequence
    FA_AROUND_CURSOR,           // align xxx positions around current cursor position
    FA_SELECTED_RANGE,          // align selected range
    FA_SAI_MULTI_RANGE,         // align versus multi range define by SAI
};

enum FA_turn {
    FA_TURN_NEVER,              // never try to turn sequence
    FA_TURN_INTERACTIVE,        // try to turn, but query user
    FA_TURN_ALWAYS,             // turn if score is better
};

enum FA_reference {
    FA_REF_EXPLICIT,            // reference sequence explicitly specified
    FA_REF_CONSENSUS,           // use group consensus as reference
    FA_REF_RELATIVES,           // search next relatives by PT server
};

enum FA_alignTarget {
    FA_CURRENT,                 // align current species
    FA_MARKED,                  // align all marked species
    FA_SELECTED,                // align selected species (= range)
};

enum FA_errorAction {
    FA_NO_ACTION,                // do nothing
    FA_MARK_FAILED,              // mark failed species (unmark rest)
    FA_MARK_ALIGNED,             // mark aligned species (unmark rest)
};

struct AlignParams {
    FA_report report;
    bool      showGapsMessages; // display messages about missing gaps in master?
    PosRange  range;            // range to be aligned
};

struct SearchRelativeParams : virtual Noncopyable {
    FamilyFinder *ff;
    char         *pt_server_alignment; // alignment used in pt_server (may differ from 'alignment')
    int           maxRelatives;        // max # of relatives to use

    SearchRelativeParams(FamilyFinder *ff_, const char *pt_server_alignment_, int maxRelatives_)
        : ff(ff_),
          pt_server_alignment(strdup(pt_server_alignment_)),
          maxRelatives(maxRelatives_)
    {}
    
    ~SearchRelativeParams() {
        free(pt_server_alignment);
        delete(ff);
    }

    FamilyFinder *getFamilyFinder() { return ff; }
};

// --------------------------------------------------------------------------------

#define GAP_CHAR     '-'
#define QUALITY_NAME "ASC_ALIGNER_CLIENT_SCORE"
#define INSERTS_NAME "AMI_ALIGNER_MASTER_INSERTS"

#define FA_AWAR_ROOT                "faligner/"
#define FA_AWAR_TO_ALIGN            FA_AWAR_ROOT "what"
#define FA_AWAR_REFERENCE           FA_AWAR_ROOT "against"
#define FA_AWAR_REFERENCE_NAME      FA_AWAR_ROOT "sagainst"
#define FA_AWAR_RANGE               FA_AWAR_ROOT "range"
#define FA_AWAR_PROTECTION          FA_AWAR_ROOT "protection"
#define FA_AWAR_AROUND              FA_AWAR_ROOT "around"
#define FA_AWAR_MIRROR              FA_AWAR_ROOT "mirror"
#define FA_AWAR_REPORT              FA_AWAR_ROOT "report"
#define FA_AWAR_SHOW_GAPS_MESSAGES  FA_AWAR_ROOT "show_gaps"
#define FA_AWAR_CONTINUE_ON_ERROR   FA_AWAR_ROOT "continue_on_error"
#define FA_AWAR_ACTION_ON_ERROR     FA_AWAR_ROOT "action_on_error"
#define FA_AWAR_USE_SECONDARY       FA_AWAR_ROOT "use_secondary"
#define FA_AWAR_NEXT_RELATIVES      FA_AWAR_ROOT "next_relatives"
#define FA_AWAR_RELATIVE_RANGE      FA_AWAR_ROOT "relrange"
#define FA_AWAR_PT_SERVER_ALIGNMENT "tmp/" FA_AWAR_ROOT "relative_ali"
#define FA_AWAR_SAI_RANGE_NAME      FA_AWAR_ROOT "sai/sainame"
#define FA_AWAR_SAI_RANGE_CHARS     FA_AWAR_ROOT "sai/chars"

#define FA_AWAR_ISLAND_HOPPING_ROOT  "island_hopping/"
#define FA_AWAR_USE_ISLAND_HOPPING   FA_AWAR_ISLAND_HOPPING_ROOT "use"
#define FA_AWAR_ESTIMATE_BASE_FREQ   FA_AWAR_ISLAND_HOPPING_ROOT "estimate_base_freq"
#define FA_AWAR_BASE_FREQ_A          FA_AWAR_ISLAND_HOPPING_ROOT "base_freq_a"
#define FA_AWAR_BASE_FREQ_C          FA_AWAR_ISLAND_HOPPING_ROOT "base_freq_c"
#define FA_AWAR_BASE_FREQ_G          FA_AWAR_ISLAND_HOPPING_ROOT "base_freq_g"
#define FA_AWAR_BASE_FREQ_T          FA_AWAR_ISLAND_HOPPING_ROOT "base_freq_t"
#define FA_AWAR_SUBST_PARA_AC        FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_ac"
#define FA_AWAR_SUBST_PARA_AG        FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_ag"
#define FA_AWAR_SUBST_PARA_AT        FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_at"
#define FA_AWAR_SUBST_PARA_CG        FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_cg"
#define FA_AWAR_SUBST_PARA_CT        FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_ct"
#define FA_AWAR_SUBST_PARA_GT        FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_gt"
#define FA_AWAR_EXPECTED_DISTANCE    FA_AWAR_ISLAND_HOPPING_ROOT "expected_dist"
#define FA_AWAR_STRUCTURE_SUPPLEMENT FA_AWAR_ISLAND_HOPPING_ROOT "struct_suppl"
#define FA_AWAR_THRESHOLD            FA_AWAR_ISLAND_HOPPING_ROOT "threshold"
#define FA_AWAR_GAP_A                FA_AWAR_ISLAND_HOPPING_ROOT "gapa"
#define FA_AWAR_GAP_B                FA_AWAR_ISLAND_HOPPING_ROOT "gapb"
#define FA_AWAR_GAP_C                FA_AWAR_ISLAND_HOPPING_ROOT "gapc"

// --------------------------------------------------------------------------------

static IslandHopping *island_hopper = NULL; // NULL -> use fast aligner; else use island hopper

static GB_alignment_type global_alignmentType = GB_AT_UNKNOWN;  // type of actually aligned sequence

static int currentSequenceNumber;    // used for counter
static int overallSequenceNumber;

// --------------------------------------------------------------------------------

inline ARB_ERROR species_not_found(GB_CSTR species_name) {
    return GBS_global_string("No species '%s' found!", species_name);
}

static ARB_ERROR reverseComplement(GBDATA *gb_species, GB_CSTR ali, int max_protection) {
    GBDATA    *gbd   = GBT_find_sequence(gb_species, ali);
    ARB_ERROR  error = 0;

    if (!gbd) {
        error = GBS_global_string("No 'data' found for species '%s'", GBT_read_name(gb_species));
    }
    else {
        int my_protection = GB_read_security_write(gbd);

        if (my_protection<=max_protection) { // ok
            char              *seq      = GB_read_string(gbd);
            int                length   = GB_read_string_count(gbd);
            GBDATA            *gb_main  = GB_get_root(gb_species);
            GB_alignment_type  ali_type = GBT_get_alignment_type(gb_main, ali);

            char T_or_U;
            error = GBT_determine_T_or_U(ali_type, &T_or_U, "reverse-complement");
            if (!error) {
                GBT_reverseComplementNucSequence(seq, length, T_or_U);
                error = GB_write_string(gbd, seq);
            }
        }
        else { // protection error
            error = GBS_global_string("Cannot reverse-complement species '%s' because of protection level", GBT_read_name(gb_species));
        }

    }

    return error;
}

static void build_reverse_complement(AW_window *aw, const AlignDataAccess *data_access) {
    GBDATA *gb_main = data_access->gb_main;

    GB_push_transaction(gb_main);

    AW_root        *root              = aw->get_root();
    FA_alignTarget  revComplWhat      = static_cast<FA_alignTarget>(root->awar(FA_AWAR_TO_ALIGN)->read_int());
    char           *default_alignment = GBT_get_default_alignment(gb_main);
    GB_CSTR         alignment         = root->awar_string(AWAR_EDITOR_ALIGNMENT, default_alignment)->read_string();
    ARB_ERROR       error             = 0;
    int             max_protection    = root->awar(FA_AWAR_PROTECTION)->read_int();

    switch (revComplWhat) {
        case FA_CURRENT: { // current species
            GB_CSTR species_name = root->awar(AWAR_SPECIES_NAME)->read_string();
            GBDATA *gb_species = GBT_find_species(gb_main, species_name);
            if (!gb_species) error = species_not_found(species_name);
            if (!error) error = reverseComplement(gb_species, alignment, max_protection);
            break;
        }
        case FA_MARKED: { // marked species
            GBDATA *gb_species = GBT_first_marked_species(gb_main);

            if (!gb_species) {
                error = "There is no marked species";
            }
            while (gb_species) {
                error = reverseComplement(gb_species, alignment, max_protection);
                if (error) break;
                gb_species = GBT_next_marked_species(gb_species);
            }
            break;
        }
        case FA_SELECTED: { // selected species (editor selection!)
            
            Aligner_get_first_selected_species get_first_selected_species = data_access->get_first_selected_species;
            Aligner_get_next_selected_species  get_next_selected_species  = data_access->get_next_selected_species;

            int     count      = 0;
            GBDATA *gb_species = get_first_selected_species(&count);
            
            if (!gb_species) {
                error = "There is no selected species!";
            }
            while (gb_species) {
                error = reverseComplement(gb_species, alignment, max_protection);
                if (error) break;
                gb_species = get_next_selected_species();
            }
            break;
        }
        default: {
            fa_assert(0);
            break;
        }
    }

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

// --------------------------------------------------------------------------------

class AliChange { // describes a local alignment change
    const CompactedSubSequence& Old;
    const CompactedSubSequence& New;

public:
    AliChange(const CompactedSubSequence& old_, const CompactedSubSequence& new_)
        : Old(old_), New(new_)
    {
        fa_assert(Old.may_refer_to_same_part_as(New));
    }

    int follow(ExplicitRange& range) const {
        ExplicitRange compressed(Old.compPosition(range.start()),
                                 Old.compPosition(range.end()));

        int exp_start = New.expdPosition(compressed.start());
        int exp_end   = New.expdPosition(compressed.end());

        int gaps_before = New.no_of_gaps_before(compressed.start());
        int gaps_after = New.no_of_gaps_after(compressed.end());

        fa_assert(gaps_before >= 0);
        fa_assert(gaps_after >= 0);

        range = ExplicitRange(exp_start-gaps_before, exp_end+gaps_after);

        return compressed.size(); // number of bases
    }
};

class LooseBases {
    typedef std::list<ExplicitRange> Ranges;

    Ranges ranges;
    
public:

    bool is_empty() const { return ranges.empty(); }
    void clear() { ranges.clear(); }

    void memorize(ExplicitRange range) {
        ranges.push_front(range);
    }
    ExplicitRange recall() {
        ExplicitRange range = ranges.front();
        ranges.pop_front();
        return range;
    }
    int follow_ali_change(const AliChange& change) {
        // transform positions according to changed alignment (oldSequence -> newSequence) 
        // returns the number of bases contained in 'this'
        int basecount = 0;
        if (!is_empty()) {
            for (Ranges::iterator r = ranges.begin(); r != ranges.end(); ++r) {
                basecount += change.follow(*r);
            }
        }
        return basecount;
    }
    void append(LooseBases& loose) { ranges.splice(ranges.end(), loose.ranges); }
    int follow_ali_change_and_append(LooseBases& loose, const AliChange& change) {
        // returns the number of loose bases (added from 'loose')
        int basecount = loose.follow_ali_change(change);
        append(loose);
        return basecount;
    }
};

static LooseBases unaligned_bases; // if fast_align cannot align (no master bases) -> stores positions here


static const char *read_name(GBDATA *gbd) {
    return gbd ? GBT_read_name(gbd) : "GROUP-CONSENSUS";
}

inline int relatedBases(char base1, char base2) {
    return baseMatch(base1, base2)==1;
}

inline char alignQuality(char slave, char master) {
    fa_assert(slave);
    fa_assert(master);
    char result = '#';
    if (slave==master)              result = '-';   // equal
    else if (slave==GAP_CHAR)           result = '+';   // inserted gap
    else if (master==GAP_CHAR)          result = '+';   // no gap in master
    else if (relatedBases(slave, master))   result = '~';   // mutation (related bases)
    return result;                          // mutation (non-related bases)
}

// -------------------------
//      Debugging stuff

#ifdef DEBUG
static char *lstr(const char *s, int len) {
    static int alloc;
    static char *buffer;

    if (alloc<(len+1)) {
        if (alloc) free(buffer);
        buffer = (char*)malloc(alloc=len+100);
    }

    memcpy(buffer, s, len);
    buffer[len] = 0;

    return buffer;
}

#define BUFLEN 120

inline char compareChar(char base1, char base2) {
    return base1==base2 ? '=' : (relatedBases(base1, base2) ? 'x' : 'X');
}

#if defined(TRACE_COMPRESSED_ALIGNMENT)
    
static void dump_n_compare_one(const char *seq1, const char *seq2, long len, long offset) {
    fa_assert(len<=BUFLEN);
    char compare[BUFLEN+1];

    for (long l=0; l<len; l++) {
        compare[l] = (is_ali_gap(seq1[l]) && is_ali_gap(seq2[l])) ? ' ' : compareChar(seq1[l], seq2[l]);
    }

    compare[len] = 0;

    printf(" %li '%s'\n", offset, lstr(seq1, len));
    printf(" %li '%s'\n", offset, lstr(seq2, len));
    printf(" %li '%s'\n", offset, compare);
}

inline void dump_rest(const char *seq, long len, int idx, long offset) {
    printf(" Rest von Sequenz %i:\n", idx);
    while (len>BUFLEN) {
        printf(" %li '%s'\n", offset, lstr(seq, BUFLEN));
        seq += BUFLEN;
        len -= BUFLEN;
        offset += BUFLEN;
    }

    fa_assert(len>0);
    printf(" '%s'\n", lstr(seq, len));
}

static void dump_n_compare(const char *text, const char *seq1, long len1, const char *seq2, long len2) {
    long offset = 0;

    printf(" Comparing %s:\n", text);

    while (len1>0 && len2>0) {
        long done = 0;

        if (len1>=BUFLEN && len2>=BUFLEN) {
            dump_n_compare_one(seq1, seq2, done=BUFLEN, offset);
        }
        else {
            long min = len1<len2 ? len1 : len2;
            dump_n_compare_one(seq1, seq2, done=min, offset);
        }

        seq1 += done;
        seq2 += done;
        len1 -= done;
        len2 -= done;
        offset += done;
    }

    if (len1>0) dump_rest(seq1, len1, 1, offset);
    if (len2>0) dump_rest(seq2, len2, 2, offset);
    printf(" -------------------\n");
}

static void dump_n_compare(const char *text, const CompactedSubSequence& seq1, const CompactedSubSequence& seq2) {
    dump_n_compare(text, seq1.text(), seq1.length(), seq2.text(), seq2.length());
}
#endif // TRACE_COMPRESSED_ALIGNMENT

#undef BUFLEN

inline void dumpSeq(const char *seq, long len, long pos) {
    printf("'%s' ", lstr(seq, len));
    printf("(Pos=%li,Len=%li)", pos, len);
}

#define dump()                                                          \
    do {                                                                \
        double sig = partSignificance(sequence().length(), slaveSequence.length(), bestLength); \
                                                                        \
        printf(" Score = %li (Significance=%f)\n"                       \
               " Master = ", bestScore, sig);                           \
        dumpSeq(bestMasterLeft.text(), bestLength, bestMasterLeft.leftOf()); \
        printf("\n"                                                     \
               " Slave  = ");                                           \
        dumpSeq(bestSlaveLeft.text(), bestLength, bestSlaveLeft.leftOf()); \
        printf("\n");                                                   \
    } while (0)

#endif //DEBUG


// ------------------------------------------------
//      INLINE-functions used in fast_align():

inline double log3(double d) {
    return log(d)/log(3.0);
}
inline double partSignificance(long seq1len, long seq2len, long partlen) {
    // returns log3 of significance of the part
    // usage: partSignificance(...) < log3(maxAllowedSignificance)
    return log3((seq1len-partlen)*(seq2len-partlen)) - partlen;
}

inline ARB_ERROR bufferTooSmall() {
    return "Cannot align - reserved buffer is to small";
}

inline long insertsToNextBase(AlignBuffer *alignBuffer, const SequencePosition& master) {
    int inserts;
    int nextBase;

    if (master.rightOf()>0) {
        nextBase = master.expdPosition();
    }
    else {
        nextBase = master.sequence().expdPosition(master.sequence().length());
    }
    inserts = nextBase-alignBuffer->offset();

    return inserts;
}

inline void insertBase(AlignBuffer *alignBuffer,
                              SequencePosition& master, SequencePosition& slave,
                              FastAlignReport *report)
{
    char slaveBase = *slave.text();
    char masterBase = *master.text();

    alignBuffer->set(slaveBase, alignQuality(slaveBase, masterBase));
    report->count_aligned_base(slaveBase!=masterBase);
    ++slave;
    ++master;
}

inline void insertSlaveBases(AlignBuffer *alignBuffer,
                                    SequencePosition& slave,
                                    int length,
                                    FastAlignReport *report)
{
    alignBuffer->copy(slave.text(), alignQuality(*slave.text(), GAP_CHAR), length);
    report->count_unaligned_base(length);
    slave += length;
}

inline void insertGap(AlignBuffer *alignBuffer,
                             SequencePosition& master,
                             FastAlignReport *report)
{
    char masterBase = *master.text();

    alignBuffer->set(GAP_CHAR, alignQuality(GAP_CHAR, masterBase));
    report->count_aligned_base(masterBase!=GAP_CHAR);
    ++master;
}

static ARB_ERROR insertClustalValigned(AlignBuffer *alignBuffer,
                                       SequencePosition& master,
                                       SequencePosition& slave,
                                       const char *masterAlignment, const char *slaveAlignment, long alignmentLength,
                                       FastAlignReport *report)
{
    // inserts bases of 'slave' to 'alignBuffer' according to alignment in 'masterAlignment' and 'slaveAlignment'
#define ACID '*'    // contents of 'masterAlignment' and 'slaveAlignment'
#define GAP  '-'

    int pos;
    int baseAtLeft = 0;     // TRUE -> last position in alignBuffer contains a base

    for (pos=0; pos<alignmentLength; pos++) {
        long insert = insertsToNextBase(alignBuffer, master); // in expanded seq

        if (masterAlignment[pos]==ACID) {
            if (insert>0) {
                if (insert>alignBuffer->free()) return bufferTooSmall();
                alignBuffer->set(GAP_CHAR, alignQuality(GAP_CHAR, GAP_CHAR), insert);
                fa_assert(insertsToNextBase(alignBuffer, master)==0);
                insert = 0;
            }

            if (!alignBuffer->free()) return bufferTooSmall();
            if (slaveAlignment[pos]==ACID) {
                insertBase(alignBuffer, master, slave, report);
                baseAtLeft = 1;
            }
            else {
                insertGap(alignBuffer, master, report);
                baseAtLeft = 0;
            }
        }
        else {
            int slave_bases;

            fa_assert(masterAlignment[pos]==GAP);
            for (slave_bases=1; pos+slave_bases<alignmentLength && masterAlignment[pos+slave_bases]==GAP; slave_bases++) {
                ; // count gaps in master (= # of slave bases to insert)
            }
            if (!baseAtLeft && insert>slave_bases) {
                int ins_gaps = insert-slave_bases;

                fa_assert(alignBuffer->free()>=ins_gaps);
                alignBuffer->set(GAP_CHAR, alignQuality(GAP_CHAR, GAP_CHAR), ins_gaps);
                insert -= ins_gaps;
            }

            if (insert<slave_bases) { // master has less gaps than slave bases to insert
                report->memorize_insertion(master.expdPosition(), slave_bases-insert);
            }
            else if (insert>slave_bases) { // master has more gaps than slave bases to insert
                fa_assert(baseAtLeft);
            }

            unaligned_bases.memorize(ExplicitRange(slave.expdPosition(), // memorize base positions without counterpart in master
                                                   slave.expdPosition(slave_bases-1)));

            if (slave_bases>alignBuffer->free()) return bufferTooSmall();
            insertSlaveBases(alignBuffer, slave, slave_bases, report);
            pos += slave_bases-1; // -1 compensates for()-increment
            baseAtLeft = 1;
        }
    }

    return 0;

#undef GAP
#undef ACID
}

static ARB_ERROR insertAligned(AlignBuffer *alignBuffer,
                               SequencePosition& master, SequencePosition& slave, long partLength,
                               FastAlignReport *report)
{
    // insert bases of 'slave' to 'alignBuffer' according to 'master'
    if (partLength) {
        long insert = insertsToNextBase(alignBuffer, master);

        fa_assert(partLength>=0);

        if (insert<0) { // insert gaps into master
            long min_insert = insert;

            report->memorize_insertion(master.expdPosition(), -insert);

            while (insert<0 && partLength) {
                if (insert<min_insert) min_insert = insert;
                if (!alignBuffer->free()) {
                    return bufferTooSmall();
                }
                insertBase(alignBuffer, master, slave, report);
                partLength--;
                insert = insertsToNextBase(alignBuffer, master);
            }

            fa_assert(partLength>=0);
            if (partLength==0) { // all inserted
                return NULL;
            }
        }

        fa_assert(insert>=0);

        if (insert>0) { // insert gaps into slave
            if (insert>alignBuffer->free()) return bufferTooSmall();
            alignBuffer->set(GAP_CHAR, alignQuality(GAP_CHAR, GAP_CHAR), insert);
            fa_assert(insertsToNextBase(alignBuffer, master)==0);
        }

        fa_assert(partLength>=0);

        while (partLength--) {
            insert = insertsToNextBase(alignBuffer, master);

            fa_assert(insert>=0);
            if (insert>0) {
                if (insert>=alignBuffer->free()) return bufferTooSmall();
                alignBuffer->set(GAP_CHAR, alignQuality(GAP_CHAR, GAP_CHAR), insert);
            }
            else {
                if (!alignBuffer->free()) {
                    return bufferTooSmall();
                }
            }

            insertBase(alignBuffer, master, slave, report);
        }
    }

    return NULL;
}

static ARB_ERROR cannot_fast_align(const CompactedSubSequence& master, long moffset, long mlength,
                                   const CompactedSubSequence& slaveSequence, long soffset, long slength,
                                   int max_seq_length,
                                   AlignBuffer *alignBuffer,
                                   FastAlignReport *report)
{
    const char *mtext = master.text(moffset);
    const char *stext = slaveSequence.text(soffset);
    ARB_ERROR   error = 0;

    if (slength) {
        if (mlength) { // if slave- and master-sequences contain bases, we call the slow aligner
#ifdef TRACE_CLUSTAL_DATA
            printf("ClustalV-Align:\n");
            printf(" mseq = '%s'\n", lstr(mtext, mlength));
            printf(" sseq = '%s'\n", lstr(stext, slength));
#endif // TRACE_CLUSTAL_DATA

            int is_dna = -1;

            switch (global_alignmentType) {
                case GB_AT_RNA:
                case GB_AT_DNA: is_dna = 1; break;
                case GB_AT_AA:  is_dna = 0; break;
                default: error = "Unknown alignment type - aligner aborted"; break;
            }

            const char *maligned, *saligned;
            int         len;
            if (!error) {
                int score; // unused
                error = ClustalV_align(is_dna,
                                       1,
                                       mtext, mlength, stext, slength,
                                       master.gapsBefore(moffset), 
                                       max_seq_length,
                                       maligned, saligned, len, score);
            }

            if (!error) {
#ifdef TRACE_CLUSTAL_DATA
                printf("ClustalV returns:\n");
                printf(" maligned = '%s'\n", lstr(maligned, len));
                printf(" saligned = '%s'\n", lstr(saligned, len));
#endif // TRACE_CLUSTAL_DATA

                SequencePosition masterPos(master, moffset);
                SequencePosition slavePos(slaveSequence, soffset);

                error = insertClustalValigned(alignBuffer, masterPos, slavePos, maligned, saligned, len, report);

#if (defined(DEBUG) && 0)

                SequencePosition master2(master->sequence(), moffset);
                SequencePosition slave2(slaveSequence, soffset);
                char *cmp = new char[len];

                for (int l=0; l<len; l++) {
                    int gaps = 0;

                    if (maligned[l]=='*') {
                        maligned[l] = *master2.text();
                        ++master2;
                    }
                    else {
                        gaps++;
                    }

                    if (saligned[l]=='*') {
                        saligned[l] = *slave2.text();
                        ++slave2;
                    }
                    else {
                        gaps++;
                    }

                    cmp[l] = gaps || maligned[l]==saligned[l] ? '=' : 'X';
                }

                printf(" master = '%s'\n", lstr(maligned, len));
                printf(" slave  = '%s'\n", lstr(saligned, len));
                printf("          '%s'\n", lstr(cmp, len));

                delete [] cmp;
#endif
            }
        }
        else { // master is empty here, so we just copy in the slave bases
            if (slength<=alignBuffer->free()) {
                unaligned_bases.memorize(ExplicitRange(slaveSequence.expdPosition(soffset),
                                                       slaveSequence.expdPosition(soffset+slength-1)));
                
                alignBuffer->copy(slaveSequence.text(soffset), '?', slength);   // '?' means not aligned (just inserted)
                // corrected by at alignBuffer->correctUnalignedPositions()
                report->count_unaligned_base(slength);
            }
            else {
                error = bufferTooSmall();
            }
        }
    }

    return error;
}

// ------------------------------------
//      #define's for fast_align()

#define TEST_BETTER_SCORE()                                             \
    do {                                                                \
        if (score>bestScore) {                                          \
            bestScore = score;                                          \
            bestLength = masterRight.text() - masterLeft.text();        \
            bestMasterLeft = masterLeft;                                \
            bestSlaveLeft = slaveLeft;                                  \
        }                                                               \
    } while (0)

#define CAN_SCORE_LEFT()    (masterLeft.leftOf() && slaveLeft.leftOf())
#define CAN_SCORE_RIGHT()   (masterRight.rightOf() && slaveRight.rightOf())

#define SCORE_LEFT()                                                    \
    do {                                                                \
        score += *(--masterLeft).text()==*(--slaveLeft).text() ? match : mismatch; \
        TEST_BETTER_SCORE();                                            \
    } while (0)

#define SCORE_RIGHT()                                                   \
    do {                                                                \
        score += *(++masterRight).text()==*(++slaveRight).text() ? match : mismatch; \
        TEST_BETTER_SCORE();                                            \
    } while (0)


ARB_ERROR FastSearchSequence::fast_align(const CompactedSubSequence& slaveSequence,
                                              AlignBuffer *alignBuffer,
                                              int max_seq_length,
                                         int match, int mismatch,
                                         FastAlignReport *report) const
{
    // aligns 'slaveSequence' to 'this'
    //
    // returns
    // == NULL -> all ok -> 'alignBuffer' contains aligned sequence
    // != NULL -> failure -> no results

    ARB_ERROR error   = NULL;
    int       aligned = 0;

    // set the following #if to zero to test ClustalV-Aligner (not fast_aligner)
#if 1

    static double lowSignificance;
    static int lowSignificanceInitialized;

    if (!lowSignificanceInitialized) {
        lowSignificance = log3(0.01);
        lowSignificanceInitialized = 1;
    }

    SequencePosition slave(slaveSequence);
    long bestScore=0;
    SequencePosition bestMasterLeft(sequence());
    SequencePosition bestSlaveLeft(slaveSequence);
    long bestLength=0;

    while (slave.rightOf()>=3) { // with all triples of slaveSequence
        FastSearchOccurrence occurrence(*this, slave.text()); // "search" first
        SequencePosition rightmostSlave = slave;

        while (occurrence.found()) { // with all occurrences of the triple
            long score = match*3;
            SequencePosition masterLeft(occurrence.sequence(), occurrence.offset());
            SequencePosition masterRight(occurrence.sequence(), occurrence.offset()+3);
            SequencePosition slaveLeft(slave);
            SequencePosition slaveRight(slave, 3);

            while (score>0) {
                if (CAN_SCORE_LEFT()) {
                    SCORE_LEFT();
                }
                else {
                    while (score>0 && CAN_SCORE_RIGHT()) {
                        SCORE_RIGHT();
                    }
                    break;
                }

                if (CAN_SCORE_RIGHT()) {
                    SCORE_RIGHT();
                }
                else {
                    while (score>0 && CAN_SCORE_LEFT()) {
                        SCORE_LEFT();
                    }
                    break;
                }
            }

            occurrence.gotoNext(); // "search" next

            if (rightmostSlave<slaveRight) {
                rightmostSlave = slaveRight;
                rightmostSlave -= 3;
            }
        }

        if (rightmostSlave>slave)   slave = rightmostSlave;
        else                ++slave;
    }

    if (bestLength) {
        double sig = partSignificance(sequence().length(), slaveSequence.length(), bestLength);

        if (sig<lowSignificance) {
            long masterLeftOf = bestMasterLeft.leftOf();
            long masterRightStart = masterLeftOf+bestLength;
            long masterRightOf = bestMasterLeft.rightOf()-bestLength;
            long slaveLeftOf = bestSlaveLeft.leftOf();
            long slaveRightStart = slaveLeftOf+bestLength;
            long slaveRightOf = bestSlaveLeft.rightOf()-bestLength;

#define MIN_ALIGNMENT_RANGE 4

            if (!error) {
                if (masterLeftOf >= MIN_ALIGNMENT_RANGE && slaveLeftOf >= MIN_ALIGNMENT_RANGE) {
                    CompactedSubSequence   leftCompactedMaster(sequence(), 0, masterLeftOf);
                    FastSearchSequence     leftMaster(leftCompactedMaster);

                    error = leftMaster.fast_align(CompactedSubSequence(slaveSequence, 0, slaveLeftOf),
                                                  alignBuffer, max_seq_length, match, mismatch, report);
                }
                else if (slaveLeftOf>0) {
                    error = cannot_fast_align(sequence(), 0, masterLeftOf,
                                              slaveSequence, 0, slaveLeftOf,
                                              max_seq_length, alignBuffer, report);
                }

                aligned = 1;
            }

            // align part of slave sequence according to master sequence:

            if (!error) {
#if (defined(DEBUG) && 0)
                long offset = alignBuffer->offset();
                long used;
#endif
                error = insertAligned(alignBuffer, bestMasterLeft, bestSlaveLeft, bestLength, report);
#if (defined(DEBUG) && 0)
                used = alignBuffer->offset()-offset;
                printf("aligned '%s' (len=%li, address=%li)\n", lstr(alignBuffer->text()+offset, used), used, long(alignBuffer));
#endif
                aligned = 1;
            }

            if (!error) {
                if (masterRightOf >= MIN_ALIGNMENT_RANGE && slaveRightOf >= MIN_ALIGNMENT_RANGE) {
                    CompactedSubSequence rightCompactedMaster(sequence(), masterRightStart, masterRightOf);
                    FastSearchSequence rightMaster(rightCompactedMaster);

                    error = rightMaster.fast_align(CompactedSubSequence(slaveSequence, slaveRightStart, slaveRightOf),
                                                   alignBuffer,
                                                   max_seq_length, match, mismatch, report);
                }
                else if (slaveRightOf>0) {
                    error = cannot_fast_align(sequence(), masterRightStart, masterRightOf,
                                              slaveSequence, slaveRightStart, slaveRightOf,
                                              max_seq_length, alignBuffer, report);
                }

                aligned = 1;
            }

        }
    }

#endif

    if (!aligned && !error) {
        error = cannot_fast_align(sequence(), 0, sequence().length(),
                                  slaveSequence, 0, slaveSequence.length(),
                                  max_seq_length, alignBuffer, report);
    }

    return error;
}

#undef dump
#undef TEST_BETTER_SCORE
#undef CAN_SCORE_LEFT
#undef CAN_SCORE_RIGHT
#undef SCORE_LEFT
#undef SCORE_RIGHT

inline long calcSequenceChecksum(const char *data, long length) {
    return GB_checksum(data, length, 1, GAP_CHARS);
}

#if defined(WARN_TODO)
#warning firstColumn + lastColumn -> PosRange
#endif

static CompactedSubSequence *readCompactedSequence(GBDATA      *gb_species,
                                                   const char  *ali,
                                                   ARB_ERROR   *errorPtr,
                                                   char       **dataPtr,     // if dataPtr != NULL it will be set to the WHOLE uncompacted sequence
                                                   long        *seqChksum,   // may be NULL (of part of sequence)
                                                   PosRange     range)       // if !range.is_whole() -> return only part of the sequence
{
    ARB_ERROR                  error = 0;
    GBDATA                    *gbd;
    CompactedSubSequence *seq   = 0;


    
    gbd = GBT_find_sequence(gb_species, ali);       // get sequence

    if (gbd) {
        long length = GB_read_string_count(gbd);
        char *data = GB_read_string(gbd);
        long partLength;
        char *partData;

        if (dataPtr) {                  // make a copy of the whole uncompacted sequence (returned to caller)
            *dataPtr = data;
        }

        int firstColumn = range.start();
        if (range.is_part()) {     // take only part of sequence
            int lastColumn = range.end();

            fa_assert(firstColumn>=0);
            fa_assert(firstColumn<=lastColumn);

            // include all surrounding gaps (@@@ this might cause problems with partial alignment)
            while (firstColumn>0 && is_ali_gap(data[firstColumn-1])) {
                firstColumn--;
            }
            if (lastColumn!=-1) {
                while (lastColumn<(length-1) && is_ali_gap(data[lastColumn+1])) lastColumn++;
                fa_assert(lastColumn<length);
            }

            partData = data+firstColumn;
            int slen = length-firstColumn;

            fa_assert(slen>=0);
            fa_assert((size_t)slen==strlen(partData));

            if (lastColumn==-1) { // take all till end of sequence
                partLength = slen;
            }
            else {
                partLength = lastColumn-firstColumn+1;
                if (partLength>slen) partLength = slen;     // cut rest, if we have any
            }
        }
        else {
            partLength = length;
            partData = data;
        }

        if (!error) {
            if (seqChksum) {
                *seqChksum = calcSequenceChecksum(partData, partLength);
            }

            seq = new CompactedSubSequence(partData, partLength, GBT_read_name(gb_species), firstColumn);
        }

        if (!dataPtr) {     // free sequence only if user has not requested to get it
            free(data);
        }
    }
    else {
        error = GBS_global_string("No 'data' found for species '%s'", GBT_read_name(gb_species));
        if (dataPtr) *dataPtr = NULL; // (user must not care to free data if we fail)
    }

    fa_assert(errorPtr);
    *errorPtr = error;

    return seq;
}

static ARB_ERROR writeStringToAlignment(GBDATA *gb_species, GB_CSTR alignment, GB_CSTR data_name, GB_CSTR str, bool temporary) {
    GBDATA    *gb_ali  = GB_search(gb_species, alignment, GB_DB);
    ARB_ERROR  error   = NULL;
    GBDATA    *gb_name = GB_search(gb_ali, data_name, GB_STRING);

    if (gb_name) {
        fa_assert(GB_check_father(gb_name, gb_ali));
        error = GB_write_string(gb_name, str);
        if (temporary && !error) error = GB_set_temporary(gb_name);
    }
    else {
        error = GBS_global_string("Cannot create entry '%s' for '%s'", data_name, GBT_read_name(gb_species));
    }

    return error;
}

// --------------------------------------------------------------------------------

static ARB_ERROR alignCompactedTo(CompactedSubSequence     *toAlignSequence,
                                  const FastSearchSequence *alignTo,
                                  int                       max_seq_length,
                                  GB_CSTR                   alignment,
                                  long                      toAlignChksum,
                                  GBDATA                   *gb_toAlign,
                                  GBDATA                   *gb_alignTo, // may be NULL
                                  const AlignParams&        ali_params)
{
    // if only part of the sequence should be aligned, then this functions already gets only the part
    // (i.o.w.: toAlignSequence, alignTo and toAlignChksum refer to the partial sequence)
    AlignBuffer alignBuffer(max_seq_length);
    if (ali_params.range.start()>0) {
        alignBuffer.set(GAP_CHAR, alignQuality(GAP_CHAR, GAP_CHAR), ali_params.range.start());
    }

    const char *master_name = read_name(gb_alignTo);

    FastAlignReport report(master_name, ali_params.showGapsMessages);

    {
        static GBDATA *last_gb_toAlign = 0;
        if (gb_toAlign!=last_gb_toAlign) {
            last_gb_toAlign = gb_toAlign;
            currentSequenceNumber++;
        }
    }

#ifdef TRACE_COMPRESSED_ALIGNMENT
    printf("alignCompactedTo(): master='%s' ", master_name);
    printf("slave='%s'\n", toAlignSequence->name());
#endif // TRACE_COMPRESSED_ALIGNMENT

    ARB_ERROR error = GB_pop_transaction(gb_toAlign);
    if (!error) {
        if (island_hopper) {
            error = island_hopper->do_align();
            if (!error) {
                fa_assert(island_hopper->was_aligned());

#ifdef TRACE_ISLANDHOPPER_DATA
                printf("Island-Hopper returns:\n");
                printf("maligned = '%s'\n", lstr(island_hopper->get_result_ref(), island_hopper->get_result_length()));
                printf("saligned = '%s'\n", lstr(island_hopper->get_result(), island_hopper->get_result_length()));
#endif // TRACE_ISLANDHOPPER_DATA

                SequencePosition masterPos(alignTo->sequence(), 0);
                SequencePosition slavePos(*toAlignSequence, 0);

                error = insertClustalValigned(&alignBuffer, masterPos, slavePos, island_hopper->get_result_ref(), island_hopper->get_result(), island_hopper->get_result_length(), &report);
            }
        }
        else {
            error = alignTo->fast_align(*toAlignSequence, &alignBuffer, max_seq_length, 2, -10, &report); // <- here we align
        }
    }

    if (!error) {
        alignBuffer.correctUnalignedPositions();
        if (alignBuffer.free()) {
            alignBuffer.set('-', alignQuality(GAP_CHAR, GAP_CHAR), alignBuffer.free()); // rest of alignBuffer is set to '-'
        }
        alignBuffer.restoreDots(*toAlignSequence);
    }

#ifdef TRACE_COMPRESSED_ALIGNMENT
    if (!error) {
        CompactedSubSequence alignedSlave(alignBuffer.text(), alignBuffer.length(), toAlignSequence->name());
        dump_n_compare("reference vs. aligned:", alignTo->sequence(), alignedSlave);
    }
#endif // TRACE_COMPRESSED_ALIGNMENT

    {
        GB_ERROR err = GB_push_transaction(gb_toAlign);
        if (!error) error = err;
    }

    if (!error) {
        if (calcSequenceChecksum(alignBuffer.text(), alignBuffer.length())!=toAlignChksum) { // sequence-chksum changed
            error = "Internal aligner error (sequence checksum changed) -- aborted";

#ifdef TRACE_COMPRESSED_ALIGNMENT
            CompactedSubSequence alignedSlave(alignBuffer.text(), alignBuffer.length(), toAlignSequence->name());
            dump_n_compare("Old Slave vs. new Slave", *toAlignSequence, alignedSlave);
#endif // TRACE_COMPRESSED_ALIGNMENT
        }
        else {
            GB_push_my_security(gb_toAlign);
            {
                GBDATA *gbd = GBT_add_data(gb_toAlign, alignment, "data", GB_STRING);

                if (!gbd) {
                    error = "Can't find/create sequence data";
                }
                else {
                    if (ali_params.range.is_part()) { // we aligned just a part of the sequence
                        char *buffer       = GB_read_string(gbd); // so we read old sequence data
                        long  len          = GB_read_string_count(gbd);
                        if (!buffer) error = GB_await_error();
                        else {
                            int  lenToCopy   = ali_params.range.size();
                            long wholeChksum = calcSequenceChecksum(buffer, len);

                            memcpy(buffer+ali_params.range.start(), alignBuffer.text()+ali_params.range.start(), lenToCopy); // copy in the aligned part 
                            // @@@ genau um 1 position zu niedrig
                            
                            if (calcSequenceChecksum(buffer, len) != wholeChksum) {
                                error            = "Internal aligner error (sequence checksum changed) -- aborted";
# ifdef TRACE_COMPRESSED_ALIGNMENT
                                char *buffer_org = GB_read_string(gbd);
                                dump_n_compare("Old seq vs. new seq (slave)", buffer_org, len, buffer, len);
                                free(buffer_org);
# endif // TRACE_COMPRESSED_ALIGNMENT
                            }
                            else {
                                GB_write_string(gbd, "");
                                error = GB_write_string(gbd, buffer);
                            }
                        }

                        free(buffer);
                    }
                    else {
                        alignBuffer.setDotsAtEOSequence();
                        error = GB_write_string(gbd, alignBuffer.text()); // aligned all -> write all
                    }
                }
            }
            GB_pop_my_security(gb_toAlign);

            if (!error && ali_params.report != FA_NO_REPORT) {
                // create temp-entry for slave containing alignment quality:

                error = writeStringToAlignment(gb_toAlign, alignment, QUALITY_NAME, alignBuffer.quality(), ali_params.report==FA_TEMP_REPORT);
                if (!error) {
                    // create temp-entry for master containing insert dots:

                    int   buflen    = max_seq_length*2;
                    char *buffer    = (char*)malloc(buflen+1);
                    char *afterLast = buffer;

                    if (!buffer) {
                        error = "out of memory";
                    }
                    else {
                        memset(buffer, '-', buflen);
                        buffer[buflen] = 0;

                        const FastAlignInsertion *inserts = report.insertion();
                        while (inserts) {
                            memset(buffer+inserts->offset(), '>', inserts->gaps());
                            afterLast = buffer+inserts->offset()+inserts->gaps();
                            inserts = inserts->next();
                        }

                        *afterLast = 0;
                        if (gb_alignTo) {
                            error = writeStringToAlignment(gb_alignTo, alignment, INSERTS_NAME, buffer, ali_params.report==FA_TEMP_REPORT);
                        }
                    }
                }
            }
        }
    }
    return error;
}

ARB_ERROR FastAligner_delete_temp_entries(GBDATA *gb_species, const char *alignment) {
    fa_assert(gb_species);
    fa_assert(alignment);

    GBDATA    *gb_ali = GB_search(gb_species, alignment, GB_FIND);
    ARB_ERROR  error  = NULL;

    if (gb_ali) {
        GBDATA *gb_name = GB_search(gb_ali, QUALITY_NAME, GB_FIND);
        if (gb_name) {
            error = GB_delete(gb_name);
#if defined(DEBUG)
            printf("deleted '%s' of '%s' (gb_name=%p)\n", QUALITY_NAME, GBT_read_name(gb_species), gb_name);
#endif
        }

        if (!error) {
            gb_name = GB_search(gb_ali, INSERTS_NAME, GB_FIND);
            if (gb_name) {
                error = GB_delete(gb_name);
#if defined(DEBUG)
                printf("deleted '%s' of '%s' (gb_name=%p)\n", INSERTS_NAME, GBT_read_name(gb_species), gb_name);
#endif
            }
        }
    }

    return error;
}

static ARB_ERROR align_error(ARB_ERROR olderr, GBDATA *gb_toAlign, GBDATA *gb_alignTo) {
    // used by alignTo() and alignToNextRelative() to transform errors coming from subroutines
    // can be used by higher functions

    const char *name_toAlign = read_name(gb_toAlign);
    const char *name_alignTo = read_name(gb_alignTo);

    fa_assert(olderr);

    return GBS_global_string("Error while aligning '%s' to '%s':\n%s",
                             name_toAlign, name_alignTo, olderr.deliver());
}

static ARB_ERROR alignTo(GBDATA                   *gb_toAlign,
                         GB_CSTR                   alignment,
                         const FastSearchSequence *alignTo,
                         GBDATA                   *gb_alignTo, // may be NULL
                         int                       max_seq_length,
                         const AlignParams&        ali_params)
{
    ARB_ERROR error = NULL;
    long      chksum;

    CompactedSubSequence *toAlignSequence = readCompactedSequence(gb_toAlign, alignment, &error, NULL, &chksum, ali_params.range);

    if (island_hopper) {
        GBDATA *gb_seq = GBT_find_sequence(gb_toAlign, alignment);      // get sequence
        if (gb_seq) {
            long        length = GB_read_string_count(gb_seq);
            const char *data   = GB_read_char_pntr(gb_seq);

            island_hopper->set_toAlign_sequence(data);
            island_hopper->set_alignment_length(length);
        }
    }



    if (!error) {
        error = alignCompactedTo(toAlignSequence, alignTo, max_seq_length, alignment, chksum, gb_toAlign, gb_alignTo, ali_params);
        if (error) error = align_error(error, gb_toAlign, gb_alignTo);
        delete toAlignSequence;
    }

    return error;
}

static ARB_ERROR alignToGroupConsensus(GBDATA                     *gb_toAlign,
                                       GB_CSTR                     alignment,
                                       Aligner_get_consensus_func  get_consensus,
                                       int                         max_seq_length,
                                       const AlignParams&          ali_params)
{
    ARB_ERROR  error     = 0;
    char      *consensus = get_consensus(read_name(gb_toAlign), ali_params.range);
    size_t     cons_len  = strlen(consensus);

    fa_assert(cons_len);

    for (size_t i = 0; i<cons_len; ++i) { // translate consensus to be accepted by aligner
        switch (consensus[i]) {
            case '=': consensus[i] = '-'; break;
            default: break;
        }
    }

    CompactedSubSequence compacted(consensus, cons_len, "group consensus", ali_params.range.start());

    {
        FastSearchSequence fast(compacted);
        error = alignTo(gb_toAlign, alignment, &fast, NULL, max_seq_length, ali_params);
    }

    free(consensus);

    return error;
}

static void appendNameAndUsedBasePositions(char **toString, GBDATA *gb_species, int usedBasePositions) {
    // if usedBasePositions == -1 -> unknown;

    char *currInfo;
    if (usedBasePositions<0) {
        currInfo = strdup(GBT_read_name(gb_species));
    }
    else {
        fa_assert(usedBasePositions>0); // otherwise it should NOT be announced here!
        currInfo = GBS_global_string_copy("%s:%i", GBT_read_name(gb_species), usedBasePositions);
    }

    char *newString = 0;
    if (*toString) {
        newString = GBS_global_string_copy("%s, %s", *toString, currInfo);
    }
    else {
        newString = currInfo;
        currInfo  = 0; // don't free
    }

    freeset(*toString, newString);
    free(currInfo);
}

inline int min(int i, int j) { return i<j ? i : j; }

static ARB_ERROR alignToNextRelative(SearchRelativeParams&  relSearch,
                                     int                    max_seq_length,
                                     FA_turn                turnAllowed,
                                     GB_CSTR                alignment,
                                     GBDATA                *gb_toAlign,
                                     const AlignParams&     ali_params)
{
    CompactedSubSequence *toAlignSequence = NULL;
    bool use_different_pt_server_alignment = 0 != strcmp(relSearch.pt_server_alignment, alignment);

    ARB_ERROR   error;
    long        chksum;
    int         relativesToTest = relSearch.maxRelatives*2; // get more relatives from pt-server (needed when use_different_pt_server_alignment == true)
    char      **nearestRelative = new char*[relativesToTest+1];
    int         next_relatives;
    int         i;
    GBDATA     *gb_main         = GB_get_root(gb_toAlign);

    if (use_different_pt_server_alignment) {
        turnAllowed = FA_TURN_NEVER; // makes no sense if we're using a different alignment for the pt_server
    }

    for (next_relatives=0; next_relatives<relativesToTest; next_relatives++) {
        nearestRelative[next_relatives] = 0;
    }
    next_relatives = 0;

    bool restart = true;
    while (restart) {
        restart = false;

        char *findRelsBySeq = 0;
        if (use_different_pt_server_alignment) {
            toAlignSequence = readCompactedSequence(gb_toAlign, alignment, &error, 0, &chksum, ali_params.range);

            GBDATA *gbd = GBT_find_sequence(gb_toAlign, relSearch.pt_server_alignment); // use a different alignment for next relative search
            if (!gbd) {
                error = GBS_global_string("Species '%s' has no data in alignment '%s'", GBT_read_name(gb_toAlign), relSearch.pt_server_alignment);
            }
            else {
                findRelsBySeq = GB_read_string(gbd);
            }
        }
        else {
            toAlignSequence = readCompactedSequence(gb_toAlign, alignment, &error, &findRelsBySeq, &chksum, ali_params.range);
        }

        if (error) {
            delete toAlignSequence;
            return error; // @@@ leaks ?
        }

        while (next_relatives) {
            next_relatives--;
            freenull(nearestRelative[next_relatives]);
        }

        {
            // find relatives
            FamilyFinder    *familyFinder = relSearch.getFamilyFinder();
            const PosRange&  range        = familyFinder->get_TargetRange();

            if (range.is_part()) {
                range.copy_corresponding_part(findRelsBySeq, findRelsBySeq, strlen(findRelsBySeq));
                turnAllowed = FA_TURN_NEVER; // makes no sense if we're using partial relative search
            }

            error = familyFinder->searchFamily(findRelsBySeq, FF_FORWARD, relativesToTest+1, 0); // @@@ make min_score configurable

            // @@@ case where no relative found (due to min score) handle how ? abort ? warn ?
            
            double bestScore = 0;
            if (!error) {
#if defined(DEBUG)
                double lastScore = -1;
#if defined(TRACE_RELATIVES)
                fprintf(stderr, "List of relatives used for '%s':\n", GBT_read_name(gb_toAlign));
#endif // TRACE_RELATIVES
#endif // DEBUG
                for (const FamilyList *fl = familyFinder->getFamilyList(); fl; fl=fl->next) {
                    if (strcmp(toAlignSequence->name(), fl->name)!=0) {
                        if (GBT_find_species(gb_main, fl->name)) { // @@@
                            double thisScore = familyFinder->uses_rel_matches() ? fl->rel_matches : fl->matches;
#if defined(DEBUG)
                            // check whether family list is sorted correctly
                            fa_assert(lastScore < 0 || lastScore >= thisScore);
                            lastScore        = thisScore;
#if defined(TRACE_RELATIVES)
                            fprintf(stderr, "- %s (%5.2f)\n", fl->name, thisScore);
#endif // TRACE_RELATIVES
#endif // DEBUG
                            if (thisScore>=bestScore) bestScore = thisScore;
                            if (next_relatives<(relativesToTest+1)) {
                                nearestRelative[next_relatives] = strdup(fl->name);
                                next_relatives++;
                            }
                        }
                    }
                }
            }

            if (!error && turnAllowed != FA_TURN_NEVER) { // test if mirrored sequence has better relatives
                char   *mirroredSequence  = strdup(findRelsBySeq);
                long    length            = strlen(mirroredSequence);
                double  bestMirroredScore = 0;

                char T_or_U;
                error = GBT_determine_T_or_U(global_alignmentType, &T_or_U, "reverse-complement");
                if (!error) {
                    GBT_reverseComplementNucSequence(mirroredSequence, length, T_or_U);
                    error = familyFinder->searchFamily(mirroredSequence, FF_FORWARD, relativesToTest+1, 0); // @@@ make min_score configurable
                }
                if (!error) {
#if defined(DEBUG)
                    double lastScore = -1;
#if defined(TRACE_RELATIVES)
                    fprintf(stderr, "List of relatives used for '%s' (turned seq):\n", GBT_read_name(gb_toAlign));
#endif // TRACE_RELATIVES
#endif // DEBUG
                    for (const FamilyList *fl = familyFinder->getFamilyList(); fl; fl = fl->next) {
                        double thisScore = familyFinder->uses_rel_matches() ? fl->rel_matches : fl->matches;
#if defined(DEBUG)
                        // check whether family list is sorted correctly
                        fa_assert(lastScore < 0 || lastScore >= thisScore);
                        lastScore = thisScore;
#if defined(TRACE_RELATIVES)
                        fprintf(stderr, "- %s (%5.2f)\n", fl->name, thisScore);
#endif // TRACE_RELATIVES
#endif // DEBUG
                        if (thisScore >= bestMirroredScore) {
                            if (strcmp(toAlignSequence->name(), fl->name)!=0) {
                                if (GBT_find_species(gb_main, fl->name)) bestMirroredScore = thisScore; // @@@
                            }
                        }
                    }
                }

                int turnIt = 0;
                if (bestMirroredScore>bestScore) {
                    if (turnAllowed==FA_TURN_INTERACTIVE) {
                        const char *message;
                        if (familyFinder->uses_rel_matches()) {
                            message = GBS_global_string("'%s' seems to be the other way round (score: %.1f%%, score if turned: %.1f%%)",
                                                        toAlignSequence->name(), bestScore*100, bestMirroredScore*100);
                        }
                        else {
                            message = GBS_global_string("'%s' seems to be the other way round (score: %li, score if turned: %li)",
                                                        toAlignSequence->name(), long(bestScore+.5), long(bestMirroredScore+.5));
                        }
                        turnIt = aw_question("fastali_turn_sequence", message, "Turn sequence,Leave sequence alone")==0;
                    }
                    else {
                        fa_assert(turnAllowed == FA_TURN_ALWAYS);
                        turnIt = 1;
#if defined(TRACE_RELATIVES)
                        fprintf(stderr, "Using turned sequence!\n");
#endif // TRACE_RELATIVES
                    }
                }

                if (turnIt) { // write mirrored sequence
                    GBDATA *gbd = GBT_find_sequence(gb_toAlign, alignment);
                    GB_push_my_security(gbd);
                    error = GB_write_string(gbd, mirroredSequence);
                    GB_pop_my_security(gbd);
                    if (!error) {
                        delete toAlignSequence;
                        restart = true; // continue while loop
                    }
                }

                free(mirroredSequence);
            }
        }
        free(findRelsBySeq);
    }

    if (!error) {
        if (!next_relatives) {
            char warning[200];
            sprintf(warning, "No relative found for '%s'", toAlignSequence->name());
            aw_message(warning);
        }
        else {
            // assuming relatives are sorted! (nearest to farthest)

            // get data pointers
            typedef GBDATA *GBDATAP;
            GBDATAP *gb_reference = new GBDATAP[relSearch.maxRelatives];

            {
                for (i=0; i<relSearch.maxRelatives && i<next_relatives; i++) {
                    GBDATA *gb_species = GBT_find_species(gb_main, nearestRelative[i]);
                    if (!gb_species) { // pt-server seems not up to date!
                        error = species_not_found(nearestRelative[i]);
                        break;
                    }

                    GBDATA *gb_sequence = GBT_find_sequence(gb_species, alignment);
                    if (gb_sequence) { // if relative has sequence data in the current alignment ..
                        gb_reference[i] = gb_species;
                    }
                    else { // remove this relative
                        free(nearestRelative[i]);
                        for (int j = i+1; j<next_relatives; ++j) {
                            nearestRelative[j-1] = nearestRelative[j];
                        }
                        next_relatives--;
                        nearestRelative[next_relatives] = 0;
                        i--; // redo same index
                    }
                }

                // delete superfluous relatives
                for (; i<next_relatives; ++i) freenull(nearestRelative[i]);

                if (next_relatives>relSearch.maxRelatives) {
                    next_relatives = relSearch.maxRelatives;
                }
            }

            // align

            if (!error) {
                CompactedSubSequence *alignToSequence = readCompactedSequence(gb_reference[0], alignment, &error, NULL, NULL, ali_params.range);
                fa_assert(alignToSequence != 0);

                if (island_hopper) {
                    GBDATA *gb_ref   = GBT_find_sequence(gb_reference[0], alignment); // get reference sequence
                    GBDATA *gb_align = GBT_find_sequence(gb_toAlign, alignment);      // get sequence to align

                    if (gb_ref && gb_align) {
                        long        length_ref   = GB_read_string_count(gb_ref);
                        const char *data;

                        data = GB_read_char_pntr(gb_ref);
                        island_hopper->set_ref_sequence(data);

                        data = GB_read_char_pntr(gb_align);
                        island_hopper->set_toAlign_sequence(data);

                        island_hopper->set_alignment_length(length_ref);
                    }
                }

                {
                    FastSearchSequence referenceFastSeq(*alignToSequence);

                    error = alignCompactedTo(toAlignSequence, &referenceFastSeq,
                                             max_seq_length, alignment, chksum,
                                             gb_toAlign, gb_reference[0], ali_params);
                }

                if (error) {
                    error = align_error(error, gb_toAlign, gb_reference[0]);
                }
                else {
                    char *used_relatives = 0;

                    if (unaligned_bases.is_empty()) {
                        appendNameAndUsedBasePositions(&used_relatives, gb_reference[0], -1);
                    }
                    else {
                        if (island_hopper) {
                            appendNameAndUsedBasePositions(&used_relatives, gb_reference[0], -1);
                            if (next_relatives>1) error = "Island hopping uses only one relative";
                        }
                        else {
                            LooseBases loose;
                            LooseBases loose_for_next_relative;

                            int unaligned_positions;
                            {
                                CompactedSubSequence *alignedSequence = readCompactedSequence(gb_toAlign, alignment, &error, 0, 0, ali_params.range);

                                unaligned_positions = loose.follow_ali_change_and_append(unaligned_bases, AliChange(*toAlignSequence, *alignedSequence));
                                // now loose holds the unaligned (and recalculated) parts from last relative
                                delete alignedSequence;
                            }

                            if (!error) {
                                int toalign_positions = toAlignSequence->length();
                                if (unaligned_positions<toalign_positions) {
                                    appendNameAndUsedBasePositions(&used_relatives, gb_reference[0], toalign_positions-unaligned_positions);
                                }
                            }

                            for (i=1; i<next_relatives && !error; i++) {
                                loose.append(loose_for_next_relative);
                                int unaligned_positions_for_next = 0;

                                while (!loose.is_empty() && !error) {
                                    ExplicitRange         partRange(intersection(loose.recall(), ali_params.range));
                                    CompactedSubSequence *alignToPart = readCompactedSequence(gb_reference[i], alignment, &error, 0, 0, partRange);

                                    if (!error) {
                                        long                  part_chksum;
                                        CompactedSubSequence *toAlignPart = readCompactedSequence(gb_toAlign, alignment, &error, 0, &part_chksum, partRange);

                                        fa_assert(contradicted(error, toAlignPart));

                                        if (!error) {
                                            AlignParams loose_ali_params = { ali_params.report, ali_params.showGapsMessages, partRange };

                                            FastSearchSequence referenceFastSeq(*alignToPart);
                                            error = alignCompactedTo(toAlignPart, &referenceFastSeq,
                                                                     max_seq_length, alignment, part_chksum,
                                                                     gb_toAlign, gb_reference[i], loose_ali_params);
                                            if (!error) {
                                                CompactedSubSequence *alignedPart = readCompactedSequence(gb_toAlign, alignment, &error, 0, 0, partRange);
                                                if (!error) {
                                                    unaligned_positions_for_next += loose_for_next_relative.follow_ali_change_and_append(unaligned_bases, AliChange(*toAlignPart, *alignedPart));
                                                }
                                                delete alignedPart;
                                            }
                                        }
                                        delete toAlignPart;
                                    }
                                    delete alignToPart;
                                }

                                if (!error) {
                                    fa_assert(unaligned_positions_for_next <= unaligned_positions); // means: number of unaligned positions has increased by use of relative
                                    if (unaligned_positions_for_next<unaligned_positions) {
                                        appendNameAndUsedBasePositions(&used_relatives, gb_reference[i], unaligned_positions-unaligned_positions_for_next);
                                        unaligned_positions = unaligned_positions_for_next;
                                    }
                                }
                            }
                        }
                    }

                    if (!error) { // write used relatives to db-field
                        error = GBT_write_string(gb_toAlign, "used_rels", used_relatives);
                    }
                    free(used_relatives);
                }

                delete alignToSequence;
            }

            delete [] gb_reference;
        }
    }

    delete toAlignSequence;

    for (i=0; i<next_relatives; i++) freenull(nearestRelative[i]);
    delete [] nearestRelative;

    return error;
}

// ------------------------
//      AlignmentReference

class AlignmentReference : virtual Noncopyable {
    GB_CSTR            alignment;
    int                max_seq_length;
    const AlignParams& ali_params;

public:
    AlignmentReference(GB_CSTR            alignment_,
                       int                max_seq_length_,
                       const AlignParams& ali_params_)
        : alignment(alignment_),
          max_seq_length(max_seq_length_),
          ali_params(ali_params_)
    {}
    virtual ~AlignmentReference() {}

    virtual ARB_ERROR align_to(GBDATA *gb_toalign) const = 0;

    GB_CSTR get_alignment() const { return alignment; }
    int get_max_seq_length() const { return max_seq_length; }
    const AlignParams& get_ali_params() const { return ali_params; }
};


#if defined(WARN_TODO)
#warning make alignTo a member of ExplicitReference (or of AlignmentReference)
#warning let alignToGroupConsensus and alignToNextRelative use ExplicitReference
#endif

class ExplicitReference: public AlignmentReference { // derived from a Noncopyable
    const FastSearchSequence *targetSequence;
    GBDATA                   *gb_alignTo;

public:
    ExplicitReference(GB_CSTR                   alignment_,
                      const FastSearchSequence *targetSequence_,
                      GBDATA                   *gb_alignTo_, 
                      int                       max_seq_length_,
                      const AlignParams&        ali_params_)
        : AlignmentReference(alignment_, max_seq_length_, ali_params_), 
          targetSequence(targetSequence_), 
          gb_alignTo(gb_alignTo_) 
    {}

    ARB_ERROR align_to(GBDATA *gb_toalign) const OVERRIDE {
        return alignTo(gb_toalign, get_alignment(), targetSequence, gb_alignTo, get_max_seq_length(), get_ali_params());
    }
};

#if defined(WARN_TODO)
#warning make alignToGroupConsensus a member of ConsensusReference
#endif

class ConsensusReference: public AlignmentReference {
    Aligner_get_consensus_func  get_consensus;

public:
    ConsensusReference(GB_CSTR                     alignment_, 
                       Aligner_get_consensus_func  get_consensus_, 
                       int                         max_seq_length_, 
                       const AlignParams&          ali_params_)
        : AlignmentReference(alignment_, max_seq_length_, ali_params_),
          get_consensus(get_consensus_) 
    {}
    
    ARB_ERROR align_to(GBDATA *gb_toalign) const OVERRIDE {
        return alignToGroupConsensus(gb_toalign, get_alignment(), get_consensus, get_max_seq_length(), get_ali_params());
    }
};

#if defined(WARN_TODO)
#warning make alignToNextRelative a member of SearchRelativesReference
#endif

class SearchRelativesReference: public AlignmentReference {
    SearchRelativeParams&  relSearch;
    FA_turn                turnAllowed;

public:
    SearchRelativesReference(SearchRelativeParams&  relSearch_,
                             int                    max_seq_length_,
                             FA_turn                turnAllowed_,
                             GB_CSTR                alignment_,
                             const AlignParams&     ali_params_)
        : AlignmentReference(alignment_, max_seq_length_, ali_params_),
          relSearch(relSearch_),
          turnAllowed(turnAllowed_) 
    {}

    ARB_ERROR align_to(GBDATA *gb_toalign) const OVERRIDE {
        return alignToNextRelative(relSearch, get_max_seq_length(), turnAllowed, get_alignment(), gb_toalign, get_ali_params());
    }
};


// ----------------
//      Aligner

class Aligner : virtual Noncopyable {
    GBDATA *gb_main;

    // define alignment target(s):
    FA_alignTarget                     alignWhat;
    GB_CSTR                            alignment;                  // name of alignment to use (==NULL -> use default)
    GB_CSTR                            toalign;                    // name of species to align (used if alignWhat == FA_CURRENT)
    Aligner_get_first_selected_species get_first_selected_species; // used if alignWhat == FA_SELECTED
    Aligner_get_next_selected_species  get_next_selected_species;  // --- "" ---

    // define reference sequence(s):
    GB_CSTR                    reference;     // name of reference species
    Aligner_get_consensus_func get_consensus; // function to get consensus seq
    SearchRelativeParams&      relSearch;     // params to search for relatives

    // general params:
    FA_turn            turnAllowed;
    const AlignParams& ali_params;
    int                maxProtection;         // protection level

    // -------------------- new members
    int                wasNotAllowedToAlign;  // number of failures caused by wrong protection
    int                err_count;             // count errors
    bool               continue_on_error;         /* true -> run single alignments in separate transactions.
                                               *         If one target fails, continue with rest.
                                               * false -> run all in one transaction
                                               *          One fails -> all fail!
                                               */
    FA_errorAction     error_action;

    typedef std::list<GBDATA*> GBDATAlist;
    GBDATAlist species_to_mark;       // species that will be marked after aligning

    ARB_ERROR alignToReference(GBDATA *gb_toalign, const AlignmentReference& ref);
    ARB_ERROR alignTargetsToReference(const AlignmentReference& ref, GBDATA *gb_species_data);

    ARB_ERROR alignToExplicitReference(GBDATA *gb_species_data, int max_seq_length);
    ARB_ERROR alignToConsensus(GBDATA *gb_species_data, int max_seq_length);
    ARB_ERROR alignToRelatives(GBDATA *gb_species_data, int max_seq_length);

    void triggerAction(GBDATA *gb_species, bool has_been_aligned) {
        bool mark = false;
        switch (error_action) {
            case FA_MARK_FAILED:  mark = !has_been_aligned; break;
            case FA_MARK_ALIGNED: mark = has_been_aligned; break;
            case FA_NO_ACTION:    mark = false; break;
        }
        if (mark) species_to_mark.push_back(gb_species);
    }

public:

#if defined(WARN_TODO)
#warning pass AlignmentReference from caller (replacing reference parameters)
#endif

    Aligner(GBDATA *gb_main_,

            // define alignment target(s):
            FA_alignTarget                     alignWhat_,
            GB_CSTR                            alignment_,                   // name of alignment to use (==NULL -> use default)
            GB_CSTR                            toalign_,                     // name of species to align (used if alignWhat == FA_CURRENT)
            Aligner_get_first_selected_species get_first_selected_species_,  // used if alignWhat == FA_SELECTED
            Aligner_get_next_selected_species  get_next_selected_species_,   // --- "" ---

            // define reference sequence(s):
            GB_CSTR                    reference_,     // name of reference species
            Aligner_get_consensus_func get_consensus_, // function to get consensus seq
            SearchRelativeParams&      relSearch_,     // params to search for relatives

            // general params:
            FA_turn            turnAllowed_,
            const AlignParams& ali_params_,
            int                maxProtection_,      // protection level
            bool               continue_on_error_,
            FA_errorAction     error_action_)
        : gb_main(gb_main_),
          alignWhat(alignWhat_),
          alignment(alignment_),
          toalign(toalign_),
          get_first_selected_species(get_first_selected_species_),
          get_next_selected_species(get_next_selected_species_),
          reference(reference_),
          get_consensus(get_consensus_),
          relSearch(relSearch_),
          turnAllowed(turnAllowed_),
          ali_params(ali_params_),
          maxProtection(maxProtection_),
          wasNotAllowedToAlign(0),
          err_count(0),
          continue_on_error(continue_on_error_),
          error_action(continue_on_error ? error_action_ : FA_NO_ACTION)
    {}

    ARB_ERROR run();
};

ARB_ERROR Aligner::alignToReference(GBDATA *gb_toalign, const AlignmentReference& ref) {
    int       myProtection = GB_read_security_write(GBT_find_sequence(gb_toalign, alignment));
    ARB_ERROR error;

    if (myProtection<=maxProtection) {
        // Depending on 'continue_on_error' we either
        // * stop aligning if an error occurs or
        // * run the alignment of each species in its own transaction, ignore but report errors and
        //   optionally mark aligned or failed species.
        
        if (continue_on_error) {
            fa_assert(GB_get_transaction_level(gb_main) == 1);
            error = GB_end_transaction(gb_main, error); // end global transaction
        }

        if (!error) {
            error             = GB_push_transaction(gb_main);
            if (!error) error = ref.align_to(gb_toalign);
            error             = GB_end_transaction(gb_main, error);

            if (error) err_count++;
            triggerAction(gb_toalign, !error);
        }

        if (continue_on_error) {
            if (error) {
                GB_warning(error.deliver());
                error = NULL;
            }
            error = GB_begin_transaction(gb_main); // re-open global transaction
        }
    }
    else {
        wasNotAllowedToAlign++;
        triggerAction(gb_toalign, false);
    }

    return error;
}

ARB_ERROR Aligner::alignTargetsToReference(const AlignmentReference& ref, GBDATA *gb_species_data) {
    ARB_ERROR error;

    fa_assert(GB_get_transaction_level(gb_main) == 1);
    
    switch (alignWhat) {
        case FA_CURRENT: { // align one sequence
            fa_assert(toalign);

            GBDATA *gb_toalign = GBT_find_species_rel_species_data(gb_species_data, toalign);
            if (!gb_toalign) {
                error = species_not_found(toalign);
            }
            else {
                currentSequenceNumber = overallSequenceNumber = 1;
                error = alignToReference(gb_toalign, ref);
            }
            break;
        }
        case FA_MARKED: { // align all marked sequences
            int     count      = GBT_count_marked_species(gb_main);
            GBDATA *gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);

            arb_progress progress("Aligning marked species", count);
            progress.auto_subtitles("Species");

            currentSequenceNumber = 1;
            overallSequenceNumber = count;

            while (gb_species && !error) {
                error      = alignToReference(gb_species, ref);
                progress.inc_and_check_user_abort(error);
                gb_species = GBT_next_marked_species(gb_species);
            }
            break;
        }
        case FA_SELECTED: { // align all selected species
            int     count;
            GBDATA *gb_species = get_first_selected_species(&count);

            
            currentSequenceNumber = 1;
            overallSequenceNumber = count;

            if (!gb_species) {
                aw_message("There is no selected species!");
            }
            else {
                arb_progress progress("Aligning selected species", count);
                progress.auto_subtitles("Species");

                while (gb_species && !error) {
                    error      = alignToReference(gb_species, ref);
                    progress.inc_and_check_user_abort(error);
                    gb_species = get_next_selected_species();
                }
            }
            break;
        }
    }
    
    fa_assert(GB_get_transaction_level(gb_main) == 1);
    return error;
}

ARB_ERROR Aligner::alignToExplicitReference(GBDATA *gb_species_data, int max_seq_length) {
    ARB_ERROR  error;
    GBDATA    *gb_reference = GBT_find_species_rel_species_data(gb_species_data, reference);

    if (!gb_reference) {
        error = species_not_found(reference);
    }
    else {
        long                  referenceChksum;
        CompactedSubSequence *referenceSeq = readCompactedSequence(gb_reference, alignment, &error, NULL, &referenceChksum, ali_params.range);

        if (island_hopper) {
#if defined(WARN_TODO)
#warning setting island_hopper reference has to be done in called function (seems that it is NOT done for alignToConsensus and alignToRelatives). First get tests in place!
#endif
            GBDATA *gb_seq = GBT_find_sequence(gb_reference, alignment);        // get sequence
            if (gb_seq) {
                long        length = GB_read_string_count(gb_seq);
                const char *data   = GB_read_char_pntr(gb_seq);

                island_hopper->set_ref_sequence(data);
                island_hopper->set_alignment_length(length);
            }
        }


        if (!error) {
#if defined(WARN_TODO)
#warning do not pass FastSearchSequence to ExplicitReference, instead pass sequence and length (ExplicitReference shall create it itself)
#endif

            FastSearchSequence referenceFastSeq(*referenceSeq);
            ExplicitReference  target(alignment, &referenceFastSeq, gb_reference, max_seq_length, ali_params);

            error = alignTargetsToReference(target, gb_species_data);
        }
        delete referenceSeq;
    }
    return error;
}

ARB_ERROR Aligner::alignToConsensus(GBDATA *gb_species_data, int max_seq_length) {
    return alignTargetsToReference(ConsensusReference(alignment, get_consensus, max_seq_length, ali_params),
                                   gb_species_data);
}

ARB_ERROR Aligner::alignToRelatives(GBDATA *gb_species_data, int max_seq_length) {
    
    return alignTargetsToReference(SearchRelativesReference(relSearch, max_seq_length, turnAllowed, alignment, ali_params),
                                   gb_species_data);
}

ARB_ERROR Aligner::run() {
    // (reference == NULL && get_consensus==NULL -> use next relative for (each) sequence)

    fa_assert(GB_get_transaction_level(gb_main) == 0); // no open transaction allowed here!
    ARB_ERROR error = GB_push_transaction(gb_main);
    bool search_by_pt_server = reference==NULL && get_consensus==NULL;

    err_count            = 0;
    wasNotAllowedToAlign = 0;                       // incremented for every sequence which has higher protection level (and was not aligned)
    species_to_mark.clear();

    fa_assert(reference==NULL || get_consensus==NULL);    // can't do both modes

    if (turnAllowed != FA_TURN_NEVER) {
        if ((ali_params.range.is_part()) || !search_by_pt_server) {
            // if not selected 'Range/Whole sequence' or not selected 'Reference/Auto search..'
            turnAllowed = FA_TURN_NEVER; // then disable mirroring for the current call
        }
    }

    if (!error && !alignment) {
        alignment = (GB_CSTR)GBT_get_default_alignment(gb_main); // get default alignment
        if (!alignment) error = "No default alignment";
    }

    if (!error && alignment) {
        global_alignmentType = GBT_get_alignment_type(gb_main, alignment);
        if (search_by_pt_server) {
            GB_alignment_type pt_server_alignmentType = GBT_get_alignment_type(gb_main, relSearch.pt_server_alignment);

            if (pt_server_alignmentType != GB_AT_RNA &&
                pt_server_alignmentType != GB_AT_DNA) {
                error = "pt_servers only support RNA/DNA sequences.\n"
                    "In the aligner window you may specify a RNA/DNA alignment \n"
                    "and use a pt_server build on that alignment.";
            }
        }
    }

    if (!error) {
        GBDATA *gb_species_data = GBT_get_species_data(gb_main);
        int max_seq_length = GBT_get_alignment_len(gb_main, alignment);

        if (reference) error          = alignToExplicitReference(gb_species_data, max_seq_length);
        else if (get_consensus) error = alignToConsensus(gb_species_data, max_seq_length);
        else error                    = alignToRelatives(gb_species_data, max_seq_length);
    }

    ClustalV_exit();
    unaligned_bases.clear();

    error = GB_end_transaction(gb_main, error); // close global transaction

    if (wasNotAllowedToAlign>0) {
        const char *mess = GBS_global_string("%i species were not aligned (because of protection level)", wasNotAllowedToAlign);
        aw_message(mess);
    }

    if (err_count) {
        aw_message_if(error);
        error = GBS_global_string("Aligner produced %i error%c", err_count, err_count==1 ? '\0' : 's');
    }

    if (error_action != FA_NO_ACTION) {
        fa_assert(continue_on_error);

        GB_transaction ta(gb_main);
        GBT_mark_all(gb_main, 0);
        for (GBDATAlist::iterator sp = species_to_mark.begin(); sp != species_to_mark.end(); ++sp) {
            GB_write_flag(*sp, 1);
        }

        const char *whatsMarked = (error_action == FA_MARK_ALIGNED) ? "aligned" : "failed";
        size_t      markCount   = species_to_mark.size();
        if (markCount>0) {
            const char *msg = GBS_global_string("%zu %s species %s been marked",
                                                markCount,
                                                whatsMarked,
                                                (markCount == 1) ? "has" : "have");
            aw_message(msg);
        }
    }

    return error;
}

void FastAligner_start(AW_window *aw, const AlignDataAccess *data_access) {
    AW_root   *root          = aw->get_root();
    char      *reference     = NULL; // align against next relatives
    char      *toalign       = NULL; // align marked species
    ARB_ERROR  error         = NULL;
    int        get_consensus = 0;
    int        pt_server_id  = -1;

    Aligner_get_first_selected_species get_first_selected_species = 0;
    Aligner_get_next_selected_species  get_next_selected_species  = 0;

    fa_assert(island_hopper == 0);
    if (root->awar(FA_AWAR_USE_ISLAND_HOPPING)->read_int()) {
        island_hopper = new IslandHopping;
        if (root->awar(FA_AWAR_USE_SECONDARY)->read_int()) {
            if (data_access->helix_string) island_hopper->set_helix(data_access->helix_string);
            else error = "Warning: No HELIX found. Can't use secondary structure";
        }

        if (!error) {
            island_hopper->set_parameters(root->awar(FA_AWAR_ESTIMATE_BASE_FREQ)->read_int(),
                                          root->awar(FA_AWAR_BASE_FREQ_T)->read_float(),
                                          root->awar(FA_AWAR_BASE_FREQ_C)->read_float(),
                                          root->awar(FA_AWAR_BASE_FREQ_A)->read_float(),
                                          root->awar(FA_AWAR_BASE_FREQ_C)->read_float(),
                                          root->awar(FA_AWAR_SUBST_PARA_CT)->read_float(),
                                          root->awar(FA_AWAR_SUBST_PARA_AT)->read_float(),
                                          root->awar(FA_AWAR_SUBST_PARA_GT)->read_float(),
                                          root->awar(FA_AWAR_SUBST_PARA_AC)->read_float(),
                                          root->awar(FA_AWAR_SUBST_PARA_CG)->read_float(),
                                          root->awar(FA_AWAR_SUBST_PARA_AG)->read_float(),
                                          root->awar(FA_AWAR_EXPECTED_DISTANCE)->read_float(),
                                          root->awar(FA_AWAR_STRUCTURE_SUPPLEMENT)->read_float(),
                                          root->awar(FA_AWAR_GAP_A)->read_float(),
                                          root->awar(FA_AWAR_GAP_B)->read_float(),
                                          root->awar(FA_AWAR_GAP_C)->read_float(),
                                          root->awar(FA_AWAR_THRESHOLD)->read_float()
                                          );
        }
    }

    FA_alignTarget alignWhat = static_cast<FA_alignTarget>(root->awar(FA_AWAR_TO_ALIGN)->read_int());
    if (!error) {
        switch (alignWhat) {
            case FA_CURRENT: { // align current species
                toalign = root->awar(AWAR_SPECIES_NAME)->read_string();
                break;
            }
            case FA_MARKED: { // align marked species
                break;
            }
            case FA_SELECTED: { // align selected species
                get_first_selected_species = data_access->get_first_selected_species;
                get_next_selected_species  = data_access->get_next_selected_species;
                break;
            }
            default: {
                fa_assert(0);
                break;
            }
        }

        switch (static_cast<FA_reference>(root->awar(FA_AWAR_REFERENCE)->read_int())) {
            case FA_REF_EXPLICIT: // align against specified species
                reference = root->awar(FA_AWAR_REFERENCE_NAME)->read_string();
                break;

            case FA_REF_CONSENSUS: // align against group consensus
                if (data_access->get_group_consensus) {
                    get_consensus = 1;
                }
                else {
                    error = "Can't get group consensus here.";
                }
                break;

            case FA_REF_RELATIVES: // align against species searched via pt_server
                pt_server_id = root->awar(AWAR_PT_SERVER)->read_int();
                if (pt_server_id<0) {
                    error = "No pt_server selected";
                }
                break;

            default: fa_assert(0);
                break;
        }
    }

    RangeList ranges;
    bool      autoRestrictRange4nextRelSearch = true;

    if (!error) {
        switch (static_cast<FA_range>(root->awar(FA_AWAR_RANGE)->read_int())) {
            case FA_WHOLE_SEQUENCE:
                autoRestrictRange4nextRelSearch = false;
                ranges.add(PosRange::whole());
                break;

            case FA_AROUND_CURSOR: {
                int curpos = root->awar(AWAR_CURSOR_POSITION_LOCAL)->read_int();
                int size = root->awar(FA_AWAR_AROUND)->read_int();

                ranges.add(PosRange(curpos-size, curpos+size));
                break;
            }
            case FA_SELECTED_RANGE: {
                PosRange range;
                if (!data_access->get_selected_range(range)) {
                    error = "There is no selected species!";
                }
                else {
                    ranges.add(range);
                }
                break;
            }

            case FA_SAI_MULTI_RANGE: {
                GB_transaction ta(data_access->gb_main);

                char *sai_name = root->awar(FA_AWAR_SAI_RANGE_NAME)->read_string();
                char *aliuse   = GBT_get_default_alignment(data_access->gb_main);

                GBDATA *gb_sai     = GBT_expect_SAI(data_access->gb_main, sai_name);
                if (!gb_sai) error = GB_await_error();
                else {
                    GBDATA *gb_data = GBT_find_sequence(gb_sai, aliuse);
                    if (!gb_data) {
                        error = GB_have_error()
                            ? GB_await_error()
                            : GBS_global_string("SAI '%s' has no data in alignment '%s'", sai_name, aliuse);
                    }
                    else {
                        char *sai_data = GB_read_string(gb_data);
                        char *set_bits = root->awar(FA_AWAR_SAI_RANGE_CHARS)->read_string();

                        ranges = build_RangeList_from_string(sai_data, set_bits, false);

                        free(set_bits);
                        free(sai_data);
                    }
                }
                free(aliuse);
                free(sai_name);
                break;
            }
        }
    }

    if (!error) {
        for (RangeList::iterator r = ranges.begin(); r != ranges.end() && !error; ++r) {
            PosRange range = *r;

            GBDATA *gb_main          = data_access->gb_main;
            char   *editor_alignment = 0;
            long    alignment_length;
            {
                GB_transaction  ta(gb_main);
                char           *default_alignment = GBT_get_default_alignment(gb_main);

                alignment_length = GBT_get_alignment_len(gb_main, default_alignment);

                editor_alignment = root->awar_string(AWAR_EDITOR_ALIGNMENT, default_alignment)->read_string();
                free(default_alignment);

            }

            char     *pt_server_alignment = root->awar(FA_AWAR_PT_SERVER_ALIGNMENT)->read_string();
            PosRange  relRange            = PosRange::whole(); // unrestricted!

            if (autoRestrictRange4nextRelSearch) {
                AW_awar    *awar_relrange = root->awar(FA_AWAR_RELATIVE_RANGE);
                const char *relrange      = awar_relrange->read_char_pntr();
                if (relrange[0]) {
                    int region_plus = atoi(relrange);

                    fa_assert(range.is_part());

                    relRange = PosRange(range.start()-region_plus, range.end()+region_plus); // restricted
                    awar_relrange->write_as_string(GBS_global_string("%i", region_plus)); // set awar to detected value (avoid misbehavior when it contains ' ' or similar)
                }
            }

            if (island_hopper) {
                island_hopper->set_range(ExplicitRange(range, alignment_length));
                range = PosRange::whole();
            }

            SearchRelativeParams relSearch(new PT_FamilyFinder(gb_main,
                                                               pt_server_id,
                                                               root->awar(AWAR_NN_OLIGO_LEN)->read_int(),
                                                               root->awar(AWAR_NN_MISMATCHES)->read_int(),
                                                               root->awar(AWAR_NN_FAST_MODE)->read_int(),
                                                               root->awar(AWAR_NN_REL_MATCHES)->read_int(),
                                                               RSS_BOTH_MIN), // old scaling as b4 [8520] @@@ make configurable
                                           pt_server_alignment,
                                           root->awar(FA_AWAR_NEXT_RELATIVES)->read_int());

            relSearch.getFamilyFinder()->restrict_2_region(relRange);

            struct AlignParams ali_params = {
                static_cast<FA_report>(root->awar(FA_AWAR_REPORT)->read_int()),
                bool(root->awar(FA_AWAR_SHOW_GAPS_MESSAGES)->read_int()),
                range
            };

            {
                arb_progress progress("FastAligner");
                progress.allow_title_reuse();

                int cont_on_error = root->awar(FA_AWAR_CONTINUE_ON_ERROR)->read_int();

                Aligner aligner(gb_main,
                                alignWhat,
                                editor_alignment,
                                toalign,
                                get_first_selected_species,
                                get_next_selected_species,

                                reference,
                                get_consensus ? data_access->get_group_consensus : NULL,
                                relSearch,

                                static_cast<FA_turn>(root->awar(FA_AWAR_MIRROR)->read_int()),
                                ali_params,
                                root->awar(FA_AWAR_PROTECTION)->read_int(),
                                cont_on_error,
                                (FA_errorAction)root->awar(FA_AWAR_ACTION_ON_ERROR)->read_int());
                error = aligner.run();

                if (error && cont_on_error) {
                    aw_message_if(error);
                    error = NULL;
                }
            }

            free(pt_server_alignment);
            free(editor_alignment);
        }
    }

    if (island_hopper) {
        delete island_hopper;
        island_hopper = 0;
    }

    if (toalign) free(toalign);
    aw_message_if(error);
    if (data_access->do_refresh) data_access->refresh_display();
}



void FastAligner_create_variables(AW_root *root, AW_default db1) {
    root->awar_string(FA_AWAR_REFERENCE_NAME, "", db1);

    root->awar_int(FA_AWAR_TO_ALIGN,  FA_CURRENT,        db1);
    root->awar_int(FA_AWAR_REFERENCE, FA_REF_EXPLICIT,   db1);
    root->awar_int(FA_AWAR_RANGE,     FA_WHOLE_SEQUENCE, db1);

    AW_awar *ali_protect = root->awar_int(FA_AWAR_PROTECTION, 0, db1);
    if (ARB_in_novice_mode(root)) {
        ali_protect->write_int(0); // reset protection for noobs
    }

    root->awar_int(FA_AWAR_AROUND,             25,                  db1);
    root->awar_int(FA_AWAR_MIRROR,             FA_TURN_INTERACTIVE, db1);
    root->awar_int(FA_AWAR_REPORT,             FA_NO_REPORT,        db1);
    root->awar_int(FA_AWAR_SHOW_GAPS_MESSAGES, 1,                   db1);
    root->awar_int(FA_AWAR_CONTINUE_ON_ERROR,  1,                   db1);
    root->awar_int(FA_AWAR_ACTION_ON_ERROR,    FA_NO_ACTION,        db1);
    root->awar_int(FA_AWAR_USE_SECONDARY,      0,                   db1);
    root->awar_int(AWAR_PT_SERVER,             0,                   db1);
    root->awar_int(FA_AWAR_NEXT_RELATIVES,     1,                   db1)->set_minmax(1, 100);

    root->awar_string(FA_AWAR_RELATIVE_RANGE, "", db1);
    root->awar_string(FA_AWAR_PT_SERVER_ALIGNMENT, root->awar(AWAR_DEFAULT_ALIGNMENT)->read_char_pntr(), db1);

    root->awar_string(FA_AWAR_SAI_RANGE_NAME,  "",   db1);
    root->awar_string(FA_AWAR_SAI_RANGE_CHARS, "x1", db1);

    // island hopping:

    root->awar_int(FA_AWAR_USE_ISLAND_HOPPING, 0, db1);

    root->awar_int(FA_AWAR_ESTIMATE_BASE_FREQ, 1, db1);

    root->awar_float(FA_AWAR_BASE_FREQ_A, 0.25, db1);
    root->awar_float(FA_AWAR_BASE_FREQ_C, 0.25, db1);
    root->awar_float(FA_AWAR_BASE_FREQ_G, 0.25, db1);
    root->awar_float(FA_AWAR_BASE_FREQ_T, 0.25, db1);

    root->awar_float(FA_AWAR_SUBST_PARA_AC, 1.0, db1);
    root->awar_float(FA_AWAR_SUBST_PARA_AG, 4.0, db1);
    root->awar_float(FA_AWAR_SUBST_PARA_AT, 1.0, db1);
    root->awar_float(FA_AWAR_SUBST_PARA_CG, 1.0, db1);
    root->awar_float(FA_AWAR_SUBST_PARA_CT, 4.0, db1);
    root->awar_float(FA_AWAR_SUBST_PARA_GT, 1.0, db1);

    root->awar_float(FA_AWAR_EXPECTED_DISTANCE,    0.3,   db1);
    root->awar_float(FA_AWAR_STRUCTURE_SUPPLEMENT, 0.5,   db1);
    root->awar_float(FA_AWAR_THRESHOLD,            0.005, db1);

    root->awar_float(FA_AWAR_GAP_A, 8.0, db1);
    root->awar_float(FA_AWAR_GAP_B, 4.0, db1);
    root->awar_float(FA_AWAR_GAP_C, 7.0, db1);

    AWTC_create_common_next_neighbour_vars(root);
}

void FastAligner_set_align_current(AW_root *root, AW_default db1) {
    root->awar_int(FA_AWAR_TO_ALIGN, FA_CURRENT, db1);
}

void FastAligner_set_reference_species(AW_root *root) {
    // sets the aligner reference species to current species
    char *specName = root->awar(AWAR_SPECIES_NAME)->read_string();
    root->awar(FA_AWAR_REFERENCE_NAME)->write_string(specName);
    free(specName);
}

static AW_window *create_island_hopping_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "ISLAND_HOPPING_PARA", "Parameters for Island Hopping");
    aws->load_xfig("faligner/islandhopping.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "O");

    aws->at("help");
    aws->callback(makeHelpCallback("islandhopping.hlp"));
    aws->create_button("HELP", "HELP");

    aws->at("use_secondary");
    aws->label("Use secondary structure (only for re-align)");
    aws->create_toggle(FA_AWAR_USE_SECONDARY);

    aws->at("freq");
    aws->create_toggle_field(FA_AWAR_ESTIMATE_BASE_FREQ, "Base freq.", "B");
    aws->insert_default_toggle("Estimate", "E", 1);
    aws->insert_toggle("Define here: ", "D", 0);
    aws->update_toggle_field();

    aws->at("freq_a"); aws->label("A:"); aws->create_input_field(FA_AWAR_BASE_FREQ_A, 4);
    aws->at("freq_c"); aws->label("C:"); aws->create_input_field(FA_AWAR_BASE_FREQ_C, 4);
    aws->at("freq_g"); aws->label("G:"); aws->create_input_field(FA_AWAR_BASE_FREQ_G, 4);
    aws->at("freq_t"); aws->label("T:"); aws->create_input_field(FA_AWAR_BASE_FREQ_T, 4);

    int xpos[4], ypos[4];
    {
        aws->button_length(1);

        int dummy;
        aws->at("h_a"); aws->get_at_position(&xpos[0], &dummy); aws->create_button(NULL, "A");
        aws->at("h_c"); aws->get_at_position(&xpos[1], &dummy); aws->create_button(NULL, "C");
        aws->at("h_g"); aws->get_at_position(&xpos[2], &dummy); aws->create_button(NULL, "G");
        aws->at("h_t"); aws->get_at_position(&xpos[3], &dummy); aws->create_button(NULL, "T");

        aws->at("v_a"); aws->get_at_position(&dummy, &ypos[0]); aws->create_button(NULL, "A");
        aws->at("v_c"); aws->get_at_position(&dummy, &ypos[1]); aws->create_button(NULL, "C");
        aws->at("v_g"); aws->get_at_position(&dummy, &ypos[2]); aws->create_button(NULL, "G");
        aws->at("v_t"); aws->get_at_position(&dummy, &ypos[3]); aws->create_button(NULL, "T");
    }

    aws->at("subst"); aws->create_button(NULL, "Substitution rate parameters:");

#define XOFF -25
#define YOFF 0

    aws->at(xpos[1]+XOFF, ypos[0]+YOFF); aws->create_input_field(FA_AWAR_SUBST_PARA_AC, 4);
    aws->at(xpos[2]+XOFF, ypos[0]+YOFF); aws->create_input_field(FA_AWAR_SUBST_PARA_AG, 4);
    aws->at(xpos[3]+XOFF, ypos[0]+YOFF); aws->create_input_field(FA_AWAR_SUBST_PARA_AT, 4);
    aws->at(xpos[2]+XOFF, ypos[1]+YOFF); aws->create_input_field(FA_AWAR_SUBST_PARA_CG, 4);
    aws->at(xpos[3]+XOFF, ypos[1]+YOFF); aws->create_input_field(FA_AWAR_SUBST_PARA_CT, 4);
    aws->at(xpos[3]+XOFF, ypos[2]+YOFF); aws->create_input_field(FA_AWAR_SUBST_PARA_GT, 4);

#undef XOFF
#undef YOFF

    aws->label_length(22);

    aws->at("dist");
    aws->label("Expected distance");
    aws->create_input_field(FA_AWAR_EXPECTED_DISTANCE, 5);

    aws->at("supp");
    aws->label("Structure supplement");
    aws->create_input_field(FA_AWAR_STRUCTURE_SUPPLEMENT, 5);

    aws->at("thres");
    aws->label("Threshold");
    aws->create_input_field(FA_AWAR_THRESHOLD, 5);

    aws->label_length(10);

    aws->at("gapA");
    aws->label("Gap A");
    aws->create_input_field(FA_AWAR_GAP_A, 5);

    aws->at("gapB");
    aws->label("Gap B");
    aws->create_input_field(FA_AWAR_GAP_B, 5);

    aws->at("gapC");
    aws->label("Gap C");
    aws->create_input_field(FA_AWAR_GAP_C, 5);

    return aws;
}

static AW_window *create_family_settings_window(AW_root *root) {
    static AW_window_simple *aws = 0;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(root, "FAMILY_PARAMS", "Family search parameters");
        aws->load_xfig("faligner/family_settings.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "O");

        aws->at("help");
        aws->callback(makeHelpCallback("next_neighbours_common.hlp"));
        aws->create_button("HELP", "HELP");

        AWTC_create_common_next_neighbour_fields(aws);
    }

    return aws;
}

static AWT_config_mapping_def aligner_config_mapping[] = {
    { FA_AWAR_USE_ISLAND_HOPPING,  "island" },
    { FA_AWAR_TO_ALIGN,            "target" },
    { FA_AWAR_REFERENCE,           "ref" },
    { FA_AWAR_REFERENCE_NAME,      "refname" },
    { FA_AWAR_RELATIVE_RANGE,      "relrange" },
    { FA_AWAR_NEXT_RELATIVES,      "relatives" },
    { FA_AWAR_PT_SERVER_ALIGNMENT, "ptali" },

    // relative-search specific parameters from subwindow (create_family_settings_window)
    // same as ../DB_UI/ui_species.cxx@RELATIVES_CONFIG
    { AWAR_NN_OLIGO_LEN,   "oligolen" },
    { AWAR_NN_MISMATCHES,  "mismatches" },
    { AWAR_NN_FAST_MODE,   "fastmode" },
    { AWAR_NN_REL_MATCHES, "relmatches" },
    { AWAR_NN_REL_SCALING, "relscaling" },

    // island-hopping parameters (create_island_hopping_window)
    { FA_AWAR_USE_SECONDARY,        "use2nd" },
    { FA_AWAR_ESTIMATE_BASE_FREQ,   "estbasefreq" },
    { FA_AWAR_BASE_FREQ_A,          "freq_a" },
    { FA_AWAR_BASE_FREQ_C,          "freq_c" },
    { FA_AWAR_BASE_FREQ_G,          "freq_g" },
    { FA_AWAR_BASE_FREQ_T,          "freq_t" },
    { FA_AWAR_SUBST_PARA_AC,        "subst_ac" },
    { FA_AWAR_SUBST_PARA_AG,        "subst_ag" },
    { FA_AWAR_SUBST_PARA_AT,        "subst_at" },
    { FA_AWAR_SUBST_PARA_CG,        "subst_cg" },
    { FA_AWAR_SUBST_PARA_CT,        "subst_ct" },
    { FA_AWAR_SUBST_PARA_GT,        "subst_gt" },
    { FA_AWAR_EXPECTED_DISTANCE,    "distance" },
    { FA_AWAR_STRUCTURE_SUPPLEMENT, "supplement" },
    { FA_AWAR_THRESHOLD,            "threshold" },
    { FA_AWAR_GAP_A,                "gap_a" },
    { FA_AWAR_GAP_B,                "gap_b" },
    { FA_AWAR_GAP_C,                "gap_c" },

    { 0, 0 }
};

AW_window *FastAligner_create_window(AW_root *root, const AlignDataAccess *data_access) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "INTEGRATED_ALIGNERS", INTEGRATED_ALIGNERS_TITLE);
    aws->load_xfig("faligner/faligner.fig");

    aws->label_length(10);
    aws->button_length(10);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "O");

    aws->at("help");
    aws->callback(makeHelpCallback("faligner.hlp"));
    aws->create_button("HELP", "HELP");

    aws->at("aligner");
    aws->create_toggle_field(FA_AWAR_USE_ISLAND_HOPPING, "Aligner", "A");
    aws->insert_default_toggle("Fast aligner",   "F", 0);
    aws->sens_mask(AWM_EXP);
    aws->insert_toggle        ("Island Hopping", "I", 1);
    aws->sens_mask(AWM_ALL);
    aws->update_toggle_field();

    aws->button_length(12);
    aws->at("island_para");
    aws->callback(create_island_hopping_window);
    aws->sens_mask(AWM_EXP);
    aws->create_button("island_para", "Parameters", "");
    aws->sens_mask(AWM_ALL);

    aws->button_length(10);

    aws->at("rev_compl");
    aws->callback(makeWindowCallback(build_reverse_complement, data_access));
    aws->create_button("reverse_complement", "Turn now!", "");

    aws->at("what");
    aws->create_toggle_field(FA_AWAR_TO_ALIGN, "Align what?", "A");
    aws->insert_toggle        ("Current Species:", "A", FA_CURRENT);
    aws->insert_default_toggle("Marked Species",   "M", FA_MARKED);
    aws->insert_toggle        ("Selected Species", "S", FA_SELECTED);
    aws->update_toggle_field();

    aws->at("swhat");
    aws->create_input_field(AWAR_SPECIES_NAME, 2);

    aws->at("against");
    aws->create_toggle_field(FA_AWAR_REFERENCE, "Reference", "");
    aws->insert_toggle        ("Species by name:",          "S", FA_REF_EXPLICIT);
    aws->insert_toggle        ("Group consensus",           "K", FA_REF_CONSENSUS);
    aws->insert_default_toggle("Auto search by pt_server:", "A", FA_REF_RELATIVES);
    aws->update_toggle_field();

    aws->at("sagainst");
    aws->create_input_field(FA_AWAR_REFERENCE_NAME, 2);

    aws->at("copy");
    aws->callback(RootAsWindowCallback::simple(FastAligner_set_reference_species));
    aws->create_button("Copy", "Copy", "");

    aws->label_length(0);
    aws->at("pt_server");
    awt_create_PTSERVER_selection_button(aws, AWAR_PT_SERVER);

    aws->label_length(23);
    aws->at("relrange");
    aws->label("Data from range only, plus");
    aws->create_input_field(FA_AWAR_RELATIVE_RANGE, 3);
    
    aws->at("relatives");
    aws->label("Number of relatives to use");
    aws->create_input_field(FA_AWAR_NEXT_RELATIVES, 3);

    aws->label_length(9);
    aws->at("use_ali");
    aws->label("Alignment");
    aws->create_input_field(FA_AWAR_PT_SERVER_ALIGNMENT, 12);

    aws->at("relSett");
    aws->callback(create_family_settings_window);
    aws->create_autosize_button("Settings", "More settings", "");

    // Range

    aws->label_length(10);
    aws->at("range");
    aws->create_toggle_field(FA_AWAR_RANGE, "Range", "");
    aws->insert_default_toggle("Whole sequence",            "", FA_WHOLE_SEQUENCE);
    aws->insert_toggle        ("Positions around cursor: ", "", FA_AROUND_CURSOR);
    aws->insert_toggle        ("Selected range",            "", FA_SELECTED_RANGE);
    aws->insert_toggle        ("Multi-Range by SAI",        "", FA_SAI_MULTI_RANGE);
    aws->update_toggle_field();

    aws->at("around");
    aws->create_input_field(FA_AWAR_AROUND, 2);

    aws->at("sai");
    awt_create_SAI_selection_button(data_access->gb_main, aws, FA_AWAR_SAI_RANGE_NAME);
    
    aws->at("rchars");
    aws->create_input_field(FA_AWAR_SAI_RANGE_CHARS, 2);

    // Protection

    aws->at("protection");
    aws->label("Protection");
    aws->create_option_menu(FA_AWAR_PROTECTION, true);
    aws->insert_default_option("0", 0, 0);
    aws->insert_option("1", 0, 1);
    aws->insert_option("2", 0, 2);
    aws->insert_option("3", 0, 3);
    aws->insert_option("4", 0, 4);
    aws->insert_option("5", 0, 5);
    aws->insert_option("6", 0, 6);
    aws->update_option_menu();

    // MirrorCheck

    aws->at("mirror");
    aws->label("Turn check");
    aws->create_option_menu(FA_AWAR_MIRROR, true);
    aws->insert_option        ("Never turn sequence",         "", FA_TURN_NEVER);
    aws->insert_default_option("User acknowledgment ",        "", FA_TURN_INTERACTIVE);
    aws->insert_option        ("Automatically turn sequence", "", FA_TURN_ALWAYS);
    aws->update_option_menu();

    // Report

    aws->at("insert");
    aws->label("Report");
    aws->create_option_menu(FA_AWAR_REPORT, true);
    aws->insert_option        ("No report",                   "", FA_NO_REPORT);
    aws->sens_mask(AWM_EXP);
    aws->insert_default_option("Report to temporary entries", "", FA_TEMP_REPORT);
    aws->insert_option        ("Report to resident entries",  "", FA_REPORT);
    aws->sens_mask(AWM_ALL);
    aws->update_option_menu();

    aws->at("gaps");
    aws->create_toggle(FA_AWAR_SHOW_GAPS_MESSAGES);

    aws->at("continue");
    aws->create_toggle(FA_AWAR_CONTINUE_ON_ERROR);

    aws->at("on_failure");
    aws->label("On failure");
    aws->create_option_menu(FA_AWAR_ACTION_ON_ERROR, true);
    aws->insert_default_option("do nothing",   "", FA_NO_ACTION);
    aws->insert_option        ("mark failed",  "", FA_MARK_FAILED);
    aws->insert_option        ("mark aligned", "", FA_MARK_ALIGNED);
    aws->update_option_menu();

    aws->at("align");
    aws->callback(makeWindowCallback(FastAligner_start, data_access));
    aws->highlight();
    aws->create_button("GO", "GO", "G");

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "aligner", aligner_config_mapping);

    return aws;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

// ---------------------
//      OligoCounter

#include <map>
#include <string>

using std::map;
using std::string;

typedef map<string, size_t> OligoCount;

class OligoCounter {
    size_t oligo_len;
    size_t datasize;
    
    mutable OligoCount occurance;

    static string removeGaps(const char *seq) {
        size_t len = strlen(seq);
        string nogaps;
        nogaps.reserve(len);

        for (size_t p = 0; p<len; ++p) {
            char c = seq[p];
            if (c != '-' && c != '.') nogaps.append(1, c);
        }
        return nogaps;
    }

    void count_oligos(const string& seq) {
        occurance.clear();
        size_t max_pos = seq.length()-oligo_len;
        for (size_t p = 0; p <= max_pos; ++p) {
            string oligo(seq, p, oligo_len);
            occurance[oligo]++;
        }
    }

public:
    OligoCounter()
        : oligo_len(0), 
          datasize(0)
    {}
    OligoCounter(const char *seq, size_t oligo_len_)
        : oligo_len(oligo_len_)
    {
        string seq_nogaps = removeGaps(seq);
        datasize          = seq_nogaps.length();
        count_oligos(seq_nogaps);
    }

    OligoCounter(const OligoCounter& other)
        : oligo_len(other.oligo_len),
          datasize(other.datasize),
          occurance(other.occurance)
    {}

    size_t oligo_count(const char *oligo) {
        fa_assert(strlen(oligo) == oligo_len);
        return occurance[oligo];
    }

    size_t similarity_score(const OligoCounter& other) const {
        size_t score = 0;
        if (oligo_len == other.oligo_len) {
            for (OligoCount::const_iterator o = occurance.begin(); o != occurance.end(); ++o) {
                const string& oligo = o->first;
                size_t        count = o->second;

                score += min(count, other.occurance[oligo]);
            }
        }
        return score;
    }

    size_t getDataSize() const { return datasize; }
};

void TEST_OligoCounter() {
    OligoCounter oc1("CCAGGT", 3);
    OligoCounter oc2("GGTCCA", 3);
    OligoCounter oc2_gaps("..GGT--CCA..", 3);
    OligoCounter oc3("AGGTCC", 3);
    OligoCounter oc4("AGGTCCAGG", 3);

    TEST_EXPECT_EQUAL(oc1.oligo_count("CCA"), 1);
    TEST_EXPECT_ZERO(oc1.oligo_count("CCG"));
    TEST_EXPECT_EQUAL(oc4.oligo_count("AGG"), 2);

    int sc1_2 = oc1.similarity_score(oc2);
    int sc2_1 = oc2.similarity_score(oc1);
    TEST_EXPECT_EQUAL(sc1_2, sc2_1);

    int sc1_2gaps = oc1.similarity_score(oc2_gaps);
    TEST_EXPECT_EQUAL(sc1_2, sc1_2gaps);
    
    int sc1_3 = oc1.similarity_score(oc3);
    int sc2_3 = oc2.similarity_score(oc3);
    int sc3_4 = oc3.similarity_score(oc4);

    TEST_EXPECT_EQUAL(sc1_2, 2); // common oligos (CCA GGT)
    TEST_EXPECT_EQUAL(sc1_3, 2); // common oligos (AGG GGT)
    TEST_EXPECT_EQUAL(sc2_3, 3); // common oligos (GGT GTC TCC)

    TEST_EXPECT_EQUAL(sc3_4, 4);
}

// -------------------------
//      FakeFamilyFinder

class FakeFamilyFinder: public FamilyFinder { // derived from a Noncopyable
    // used by unit tests to detect next relatives instead of asking the pt-server

    GBDATA                    *gb_main;
    string                     ali_name;
    map<string, OligoCounter>  oligos_counted;      // key = species name
    PosRange                   counted_for_range;
    size_t                     oligo_len;

public:
    FakeFamilyFinder(GBDATA *gb_main_, string ali_name_, bool rel_matches_, size_t oligo_len_)
        : FamilyFinder(rel_matches_, RSS_BOTH_MIN),
          gb_main(gb_main_),
          ali_name(ali_name_),
          counted_for_range(PosRange::whole()), 
          oligo_len(oligo_len_)
    {}

    GB_ERROR searchFamily(const char *sequence, FF_complement compl_mode, int max_results, double min_score) OVERRIDE { // @@@ use min_score
        // 'sequence' has to contain full sequence or part corresponding to 'range'

        TEST_EXPECT_EQUAL(compl_mode, FF_FORWARD); // not fit for other modes

        delete_family_list();
        
        OligoCounter seq_oligo_count(sequence, oligo_len);

        if (range != counted_for_range) {
            oligos_counted.clear(); // forget results for different range
            counted_for_range = range;
        }

        char *buffer     = 0;
        int   buffersize = 0;

        bool partial_match = range.is_part();

        GB_transaction ta(gb_main);
        int            results = 0;

        for (GBDATA *gb_species = GBT_first_species(gb_main);
             gb_species && results<max_results;
             gb_species = GBT_next_species(gb_species))
        {
            string name = GBT_get_name(gb_species);
            if (oligos_counted.find(name) == oligos_counted.end()) {
                GBDATA     *gb_data  = GBT_find_sequence(gb_species, ali_name.c_str());
                const char *spec_seq = GB_read_char_pntr(gb_data);

                if (partial_match) {
                    int spec_seq_len = GB_read_count(gb_data);
                    int range_len    = ExplicitRange(range, spec_seq_len).size();

                    if (buffersize<range_len) {
                        delete [] buffer;
                        buffersize = range_len;
                        buffer     = new char[buffersize+1];
                    }

                    range.copy_corresponding_part(buffer, spec_seq, spec_seq_len);
                    oligos_counted[name] = OligoCounter(buffer, oligo_len);
                }
                else {
                    oligos_counted[name] = OligoCounter(spec_seq, oligo_len);
                }
            }

            const OligoCounter& spec_oligo_count = oligos_counted[name];
            size_t              score            = seq_oligo_count.similarity_score(spec_oligo_count);

            FamilyList *newMember = new FamilyList;

            newMember->name        = strdup(name.c_str());
            newMember->matches     = score;
            newMember->rel_matches = score/spec_oligo_count.getDataSize();
            newMember->next        = NULL;

            family_list = newMember->insertSortedBy_matches(family_list);
            results++;
        }

        delete [] buffer;

        return NULL;
    }
};

// ----------------------------
//      test_alignment_data

#include <arb_unit_test.h>

static const char *test_aliname = "ali_test";

static const char *get_aligned_data_of(GBDATA *gb_main, const char *species_name) {
    GB_transaction  ta(gb_main);
    ARB_ERROR       error;
    const char     *data = NULL;

    GBDATA *gb_species     = GBT_find_species(gb_main, species_name);
    if (!gb_species) error = GB_await_error();
    else {
        GBDATA *gb_data     = GBT_find_sequence(gb_species, test_aliname);
        if (!gb_data) error = GB_await_error();
        else {
            data             = GB_read_char_pntr(gb_data);
            if (!data) error = GB_await_error();
        }
    }

    TEST_EXPECT_NULL(error.deliver());
    
    return data;
}

static const char *get_used_rels_for(GBDATA *gb_main, const char *species_name) {
    GB_transaction  ta(gb_main);
    const char     *result     = NULL;
    GBDATA         *gb_species = GBT_find_species(gb_main, species_name);
    if (!gb_species) result    = GBS_global_string("<No such species '%s'>", species_name);
    else {
        GBDATA *gb_used_rels      = GB_search(gb_species, "used_rels", GB_FIND);
        if (!gb_used_rels) result = "<No such field 'used_rels'>";
        else    result            = GB_read_char_pntr(gb_used_rels);
    }
    return result;
}

static GB_ERROR forget_used_relatives(GBDATA *gb_main) {
    GB_ERROR       error = NULL;
    GB_transaction ta(gb_main);
    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species && !error;
         gb_species = GBT_next_species(gb_species))
    {
        GBDATA *gb_used_rels = GB_search(gb_species, "used_rels", GB_FIND);
        if (gb_used_rels) {
            error = GB_delete(gb_used_rels);
        }
    }
    return error;
}


#define ALIGNED_DATA_OF(name) get_aligned_data_of(gb_main, name)
#define USED_RELS_FOR(name)   get_used_rels_for(gb_main, name)

// ----------------------------------------

static GBDATA *selection_fake_gb_main = NULL;
static GBDATA *selection_fake_gb_last = NULL;

static GBDATA *fake_first_selected(int *count) {
    selection_fake_gb_last = NULL;
    *count                 = 2; // we fake two species as selected ("c1" and "c2")
    return GBT_find_species(selection_fake_gb_main, "c1");
}
static GBDATA *fake_next_selected() {
    if (!selection_fake_gb_last) {
        selection_fake_gb_last = GBT_find_species(selection_fake_gb_main, "c2");
    }
    else {
        selection_fake_gb_last = NULL;
    }
    return selection_fake_gb_last;
}

static char *fake_get_consensus(const char*, PosRange range) {
    const char *data = get_aligned_data_of(selection_fake_gb_main, "s1");
    if (range.is_whole()) return strdup(data);
    return GB_strpartdup(data+range.start(), data+range.end());
}

static void test_install_fakes(GBDATA *gb_main) {
    selection_fake_gb_main = gb_main;
}


// ----------------------------------------

static AlignParams test_ali_params = {
    FA_NO_REPORT,
    false, // showGapsMessages
    PosRange::whole()
};

static AlignParams test_ali_params_partial = {
    FA_NO_REPORT,
    false, // showGapsMessages
    PosRange(25, 60)
};

// ----------------------------------------

static struct arb_unit_test::test_alignment_data TestAlignmentData_TargetAndReferenceHandling[] = {
    { 0, "s1", ".........A--UCU-C------C-U-AAACC-CA-A-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-GUAA-CU-C..........." }, // reference
    { 0, "s2", "AUCUCCUAAACCCAACCGUAGUUCGAAUUGAGGACUGUAACUC......................................................" }, // align single sequence (same data as reference)
    { 1, "m1", "UAGAGGAUUUGGGUUGGCAUCAAGCUUAACUCCUGACAUUGAG......................................................" }, // align marked sequences.. (complement of reference)
    { 1, "m2", "...UCCUAAACCAACCCGUAGUUCGAAUUGAGGACUGUAA........................................................." },
    { 1, "m3", "AUC---UAAACCAACCCGUAGUUCGAAUUGAGGACUG---CUC......................................................" },
    { 0, "c1", "AUCUCCUAAACCCAACC--------AAUUGAGGACUGUAACUC......................................................" },  // align selected sequences..
    { 0, "c2", "AUCUCCU------AACCGUAGUUCCCCGAA------ACUGUAACUC..................................................." },
    { 0, "r1", "GAGUUACAGUCCUCAAUUCGGGGAACUACGGUUGGGUUUAGGAGAU..................................................." },  // align by faked pt_server
};

void TEST_Aligner_TargetAndReferenceHandling() {
    // performs some alignments to check logic of target and reference handling
    
    GB_shell   shell;
    ARB_ERROR  error;
    GBDATA    *gb_main = TEST_CREATE_DB(error, test_aliname, TestAlignmentData_TargetAndReferenceHandling, false);

    TEST_EXPECT_NULL(error.deliver());

    SearchRelativeParams search_relative_params(new FakeFamilyFinder(gb_main, test_aliname, false, 8),
                                                test_aliname,
                                                2);

    test_install_fakes(gb_main);
    arb_suppress_progress silence;

    // bool cont_on_err    = true;
    bool cont_on_err = false;

    TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 3); // we got 3 marked species
    {
        Aligner aligner(gb_main,
                        FA_CURRENT,
                        test_aliname,
                        "s2",                       // toalign
                        NULL,                       // get_first_selected_species
                        NULL,                       // get_next_selected_species
                        "s1",                       // reference species
                        NULL,                       // get_consensus
                        search_relative_params,     // relative search
                        FA_TURN_ALWAYS,
                        test_ali_params,
                        0,
                        cont_on_err,
                        FA_NO_ACTION);
        error = aligner.run();
        TEST_EXPECT_NULL(error.deliver());
    }
    TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 3); // we still got 3 marked species
    {
        Aligner aligner(gb_main,
                        FA_MARKED,
                        test_aliname,
                        NULL,                       // toalign
                        NULL,                       // get_first_selected_species
                        NULL,                       // get_next_selected_species
                        "s1",                       // reference species
                        NULL,                       // get_consensus
                        search_relative_params,     // relative search
                        FA_TURN_ALWAYS,
                        test_ali_params,
                        0,
                        cont_on_err,
                        FA_MARK_FAILED);
        error = aligner.run();
        TEST_EXPECT_NULL(error.deliver());

        TEST_EXPECT(!cont_on_err || GBT_count_marked_species(gb_main) == 0); // FA_MARK_FAILED (none failed -> none marked)
    }
    {
        Aligner aligner(gb_main,
                        FA_SELECTED,
                        test_aliname,
                        NULL,                       // toalign
                        fake_first_selected,        // get_first_selected_species
                        fake_next_selected,         // get_next_selected_species
                        NULL,                       // reference species
                        fake_get_consensus,         // get_consensus
                        search_relative_params,     // relative search
                        FA_TURN_ALWAYS,
                        test_ali_params,
                        0,
                        cont_on_err,
                        FA_MARK_ALIGNED);
        error = aligner.run();
        TEST_EXPECT_NULL(error.deliver());

        TEST_EXPECT(!cont_on_err || GBT_count_marked_species(gb_main) == 2); // FA_MARK_ALIGNED (2 selected were aligned)
    }
    {
        Aligner aligner(gb_main,
                        FA_CURRENT,
                        test_aliname,
                        "r1",                       // toalign
                        NULL,                       // get_first_selected_species
                        NULL,                       // get_next_selected_species
                        NULL,                       // reference species
                        NULL,                       // get_consensus
                        search_relative_params,     // relative search
                        FA_TURN_ALWAYS,
                        test_ali_params,
                        0,
                        cont_on_err,
                        FA_MARK_ALIGNED);

        error = aligner.run();
        TEST_EXPECT_NULL(error.deliver());

        TEST_EXPECT(!cont_on_err || GBT_count_marked_species(gb_main) == 1);
    }
    
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("s2"), ".........A--UCU-C------C-U-AAACC-CA-A-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-GUAA-CU-C...........");

    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("m1"), ".......UAG--AGG-A------U-U-UGGGU-UG-G-C-A-U-CAA-GCU--------UAA-C-UCCUG-AC--A-UUGAG...............");
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("m2"), "..............U-C------C-U-AAACC-AA-C-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-GUAA................");
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("m3"), ".........A--U----------C-U-AAACC-AA-C-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-G----CU-C...........");

    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("c1"), ".........A--UCU-C------C-U-AAACC-CA-A-C-C-------------------AA-U-UGAGG-AC--U-GUAA-CU-C...........");
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("c2"), ".........A--UCU-C------C-U-AA---------C-C-G-UAG-UUC------------C-CCGAA-AC--U-GUAA-CU-C...........");

    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("r1"), ".........A--UCU-C------C-U-AAACC-CA-A-C-C-G-UAG-UUCCCC-----GAA-U-UGAGG-AC--U-GUAA-CU-C..........."); // here sequence shall be turned!


    TEST_EXPECT_EQUAL(USED_RELS_FOR("r1"), "s2:43, s1:1");

    // ----------------------------------------------
    //      now align all others vs next relative

    search_relative_params.maxRelatives = 5;
    TEST_EXPECT_NO_ERROR(forget_used_relatives(gb_main));

    int species_count = ARRAY_ELEMS(TestAlignmentData_TargetAndReferenceHandling);
    for (int sp = 0; sp<species_count; ++sp) {
        const char *name = TestAlignmentData_TargetAndReferenceHandling[sp].name;
        if (strcmp(name, "r1") != 0) { // skip "r1" (already done above)
            Aligner aligner(gb_main,
                            FA_CURRENT,
                            test_aliname,
                            name,                       // toalign
                            NULL,                       // get_first_selected_species
                            NULL,                       // get_next_selected_species
                            NULL,                       // reference species
                            NULL,                       // get_consensus
                            search_relative_params,     // relative search
                            FA_TURN_ALWAYS,
                            test_ali_params,
                            0,
                            cont_on_err,
                            FA_MARK_ALIGNED);

            error = aligner.run();
            TEST_EXPECT_NULL(error.deliver());

            TEST_EXPECT(!cont_on_err || GBT_count_marked_species(gb_main) == 1);
        }
    }

    TEST_EXPECT_EQUAL(USED_RELS_FOR("s1"), "s2");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("s2"), "s1"); // same as done manually
    TEST_EXPECT_EQUAL(USED_RELS_FOR("m1"), "r1:42");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("m2"), "m3");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("m3"), "m2");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("c1"), "r1");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("c2"), "r1");

    //                                       range aligned below (see test_ali_params_partial)
    //                                       "-------------------------XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX------------------------------------"
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("s1"), ".........A--UCU-C------C-U-AAACC-CA-A-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-GUAA-CU-C..........."); // 1st aligning of 's1'
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("s2"), ".........A--UCU-C------C-U-AAACC-CA-A-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-GUAA-CU-C..........."); // same_as_above (again aligned vs 's1')

    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("m1"), ".........U--AGA-G------G---AUUUG-GG-U-U-G-G-CAU-CAAGCU-----UAA-C-UCCUG-AC--A-UUGAG---------------"); // changed; @@@ bug: no dots at end
 
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("m2"), ".........U--C----------C-U-AAACC-AA-C-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-G----UA-A..........."); // changed (1st align vs 's1', this align vs 'm3')
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("m3"), ".........A--U----------C-U-AAACC-AA-C-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-G----CU-C..........."); // same_as_above (1st align vs 's1', this align vs 'm2')
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("c1"), ".........A--UCU-C------C-U-AAACC-CA-A-C-C-------------------AA-U-UGAGG-AC--U-GUAA-CU-C..........."); // same_as_above
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("c2"), ".........A--UCU-C------C-U--------A-A-C-C-G-UAG-UUCCCC-----GA--------A-AC--U-GUAA-CU-C..........."); // changed

    // --------------------------------------
    //      test partial relative search

    search_relative_params.getFamilyFinder()->restrict_2_region(test_ali_params_partial.range);
    TEST_EXPECT_NO_ERROR(forget_used_relatives(gb_main));

    for (int sp = 0; sp<species_count; ++sp) {
        const char *name = TestAlignmentData_TargetAndReferenceHandling[sp].name;
        Aligner aligner(gb_main,
                        FA_CURRENT,
                        test_aliname,
                        name,                       // toalign
                        NULL,                       // get_first_selected_species
                        NULL,                       // get_next_selected_species
                        NULL,                       // reference species
                        NULL,                       // get_consensus
                        search_relative_params,     // relative search
                        FA_TURN_NEVER,
                        test_ali_params_partial,
                        0,
                        cont_on_err,
                        FA_MARK_ALIGNED);

        error = aligner.run();
        TEST_EXPECT_NULL(error.deliver());

        TEST_EXPECT(!cont_on_err || GBT_count_marked_species(gb_main) == 1);
    }

    TEST_EXPECT_EQUAL(USED_RELS_FOR("s1"), "s2");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("s2"), "s1");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("m1"), "r1"); // (not really differs)
    TEST_EXPECT_EQUAL(USED_RELS_FOR("m2"), "m3");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("m3"), "m2");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("c1"), "r1");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("c2"), "r1");
    TEST_EXPECT_EQUAL(USED_RELS_FOR("r1"), "s2:20, c2:3");

    //                                       aligned range (see test_ali_params_partial)
    //                                       "-------------------------XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX------------------------------------"
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("s1"), ".........A--UCU-C------C-U-AAACC-CA-A-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-GUAA-CU-C..........."); // same_as_above
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("s2"), ".........A--UCU-C------C-U-AAACC-CA-A-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-GUAA-CU-C..........."); // same_as_above

    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("m1"), ".........U--AGA-G------G-A-UU-UG-GG-U-U-G-G-CAU-CAAGCU-----UAA-C-UCCUG-AC--A-UUGAG---------------"); // changed
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("m2"), ".........U--C----------C-U-AAACC-AA-C-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-G----UA-A..........."); // same_as_above
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("m3"), ".........A--U----------C-U-AAACC-AA-C-C-C-G-UAG-UUC--------GAA-U-UGAGG-AC--U-G----CU-C..........."); // same_as_above

    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("c1"), ".........A--UCU-C------C-U-AAACC-CA-A-C-C-------------------AA-U-UGAGG-AC--U-GUAA-CU-C..........."); // same_as_above
    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("c2"), ".........A--UCU-C------C---------UA-A-C-C-G-UAG-UUCCCC-----GA--------A-AC--U-GUAA-CU-C..........."); // changed

    TEST_EXPECT_EQUAL(ALIGNED_DATA_OF("r1"), ".........A--UCU-C------C-U-AAACC-CA-A-C-C-G-UAG-UUCCCC-----GAA-U-UGAGG-AC--U-GUAA-CU-C..........."); // same_as_above

    GB_close(gb_main);
}

// ----------------------------------------

static struct arb_unit_test::test_alignment_data TestAlignmentData_checksumError[] = {
    { 0, "MtnK1722", "...G-GGC-C-G............CCC-GG--------CAAUGGGGGCGGCCCGGCGGAC----GG--C-UCAGU-A---AAG-UCGUAACAA-GG-UAG-CCGU-AGGGGAA-CCUG-CGGC-UGGAUCACCUCC....." }, // gets aligned
    { 0, "MhnFormi", "...A-CGA-U-C------------CUUCGG--------GGUCG-U-GG-C-GU-A--C------GG--C-UCAGU-A---AAG-UCGUAACAA-GG-UAG-CCGU-AGGGGAA-CCUG-CGGC-UGGAUCACCUCCU...." }, // 1st relative
    { 0, "MhnT1916", "...A-CGA-A-C------------CUU-GU--------GUUCG-U-GG-C-GA-A--C------GG--C-UCAGU-A---AAG-UCGUAACAA-GG-UAG-CCGU-AGGGGAA-CCUG-CGGC-UGGAUCACCUCCU...." }, // next relative
    { 0, "MthVanni", "...U-GGU-U-U------------C-------------GGCCA-U-GG-C-GG-A--C------GG--C-UCAUU-A---AAG-UCGUAACAA-GG-UAG-CCGU-AGGGGAA-CCUG-CGGC-UGGAUCACCUCC....." }, // next relative
    { 0, "ThcCeler", "...G-GGG-C-G...CC-U---U--------GC--G--CGCAC-C-GG-C-GG-A--C------GG--C-UCAGU-A---AAG-UCGUAACAA-GG-UAG-CCGU-AGGGGAA-CCUA-CGGC-UCGAUCACCUCCU...." }, // next relative
};

void TEST_SLOW_Aligner_checksumError() {
    // @@@ SLOW cause this often gets terminated in nightly builds
    //     no idea why. normally it runs a few ms
    
    // this produces an internal aligner error

    GB_shell   shell;
    ARB_ERROR  error;
    GBDATA    *gb_main = TEST_CREATE_DB(error, test_aliname, TestAlignmentData_checksumError, false);

    SearchRelativeParams search_relative_params(new FakeFamilyFinder(gb_main, test_aliname, false, 8),
                                                test_aliname,
                                                10); // use up to 10 relatives

    test_install_fakes(gb_main);
    arb_suppress_progress silence;

    bool cont_on_err = true;
    if (!error) {
        Aligner aligner(gb_main,
                        FA_CURRENT,
                        test_aliname,
                        "MtnK1722",                 // toalign
                        NULL,                       // get_first_selected_species
                        NULL,                       // get_next_selected_species
                        NULL,                       // reference species
                        NULL,                       // get_consensus
                        search_relative_params,     // relative search
                        FA_TURN_ALWAYS,
                        test_ali_params,
                        0,
                        cont_on_err,
                        FA_MARK_ALIGNED);

        error = aligner.run();
    }
    {
        GB_ERROR err = error.deliver();
        TEST_EXPECT_NULL__BROKEN(err, "Aligner produced 1 error");
    }
    TEST_EXPECT_EQUAL__BROKEN(USED_RELS_FOR("MtnK1722"), "???", "<No such field 'used_rels'>"); // subsequent failure

    GB_close(gb_main);
}

static const char *asstr(LooseBases& ub) {
    LooseBases tmp;
    while (!ub.is_empty()) tmp.memorize(ub.recall());
    
    const char *result = "";
    while (!tmp.is_empty()) {
        ExplicitRange r = tmp.recall();
        result          = GBS_global_string("%s %i/%i", result, r.start(), r.end());
        ub.memorize(r);
    }
    return result;
}

void TEST_BASIC_UnalignedBases() {
    LooseBases ub;
    TEST_EXPECT(ub.is_empty());
    TEST_EXPECT_EQUAL(asstr(ub), "");

    // test add+remove
    ub.memorize(ExplicitRange(5, 7));
    TEST_REJECT(ub.is_empty());
    TEST_EXPECT_EQUAL(asstr(ub), " 5/7");
    
    TEST_EXPECT(ub.recall() == ExplicitRange(5, 7));
    TEST_EXPECT(ub.is_empty());

    ub.memorize(ExplicitRange(2, 4));
    TEST_EXPECT_EQUAL(asstr(ub), " 2/4");

    ub.memorize(ExplicitRange(4, 9));
    TEST_EXPECT_EQUAL(asstr(ub), " 2/4 4/9");
    
    ub.memorize(ExplicitRange(8, 10));
    ub.memorize(ExplicitRange(11, 14));
    ub.memorize(ExplicitRange(12, 17));
    TEST_EXPECT_EQUAL(asstr(ub), " 2/4 4/9 8/10 11/14 12/17");
    TEST_EXPECT_EQUAL(asstr(ub), " 2/4 4/9 8/10 11/14 12/17"); // check asstr has no side-effect

    {
        LooseBases toaddNrecalc;

        CompactedSubSequence Old("ACGTACGT", 8, "name1");
        CompactedSubSequence New("--A-C--G-T--A-C-G-T", 19, "name2");
        // ---------------------- 0123456789012345678

        toaddNrecalc.memorize(ExplicitRange(1, 7));
        toaddNrecalc.memorize(ExplicitRange(3, 5));
        TEST_EXPECT_EQUAL(asstr(toaddNrecalc), " 1/7 3/5");

        ub.follow_ali_change_and_append(toaddNrecalc, AliChange(Old, New));

        TEST_EXPECT_EQUAL(asstr(ub), " 3/18 8/15 2/4 4/9 8/10 11/14 12/17");
        TEST_EXPECT(toaddNrecalc.is_empty());

        LooseBases selfRecalc;
        selfRecalc.follow_ali_change_and_append(ub, AliChange(New, New));
        TEST_EXPECT_EQUAL__BROKEN(asstr(selfRecalc),
                                  " 3/18 8/15 0/6 3/11 8/11 10/15 10/17",  // wanted behavior?
                                  " 3/18 8/17 0/6 3/11 8/13 10/15 10/18"); // doc wrong behavior @@@ "8/17", "8/13", "10/18" are wrong

        ub.follow_ali_change_and_append(selfRecalc, AliChange(New, Old));
        TEST_EXPECT_EQUAL__BROKEN(asstr(ub),
                                  " 1/7 3/5 0/1 1/3 3/3 4/5 4/6",  // wanted behavior? (from wanted behavior above)
                                  " 1/7 3/7 0/2 1/4 3/5 4/6 4/7"); // document wrong behavior
        TEST_EXPECT_EQUAL__BROKEN(asstr(ub),
                                  " 1/7 3/6 0/1 1/3 3/4 4/5 4/7",  // wanted behavior? (from wrong result above)
                                  " 1/7 3/7 0/2 1/4 3/5 4/6 4/7"); // document wrong behavior
    }
}


#endif // UNIT_TESTS


