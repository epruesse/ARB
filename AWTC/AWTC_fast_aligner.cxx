/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 1998                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <island_hopping.h>

#include "awtc_next_neighbours.hxx"
#include "awtc_seq_search.hxx"
#include "awtc_ClustalV.hxx"
#include "awtc_fast_aligner.hxx"
#include "awt.hxx"
#include <awt_sel_boxes.hxx>

static IslandHopping *island_hopper = 0;

#define GAP_CHAR '-'
#define QUALITY_NAME "ASC_ALIGNER_CLIENT_SCORE"
#define INSERTS_NAME "AMI_ALIGNER_MASTER_INSERTS"

#define FA_AWAR_ROOT            "faligner/"

#define FA_AWAR_TO_ALIGN           (FA_AWAR_ROOT "what")
#define FA_AWAR_REFERENCE          (FA_AWAR_ROOT "against")
#define FA_AWAR_REFERENCE_NAME     (FA_AWAR_ROOT "sagainst")
#define FA_AWAR_RANGE              (FA_AWAR_ROOT "range")
#define FA_AWAR_PROTECTION         (FA_AWAR_ROOT "protection")
#define FA_AWAR_AROUND             (FA_AWAR_ROOT "around")
//#define FA_AWAR_MARK_PROFILE      (FA_AWAR_ROOT "mark_profile")
#define FA_AWAR_MIRROR             (FA_AWAR_ROOT "mirror")
#define FA_AWAR_INSERT             (FA_AWAR_ROOT "insert")
#define FA_AWAR_SHOW_GAPS_MESSAGES (FA_AWAR_ROOT "show_gaps")
#define FA_AWAR_USE_SECONDARY      (FA_AWAR_ROOT "use_secondary")
#define FA_AWAR_NEXT_RELATIVES     (FA_AWAR_ROOT "next_relatives")

#define FA_AWAR_PT_SERVER_ALIGNMENT ("tmp/" FA_AWAR_ROOT "relative_ali")

#define FA_AWAR_ISLAND_HOPPING_ROOT "island_hopping/"

#define FA_AWAR_USE_ISLAND_HOPPING (FA_AWAR_ISLAND_HOPPING_ROOT "use")
#define FA_AWAR_ESTIMATE_BASE_FREQ (FA_AWAR_ISLAND_HOPPING_ROOT "estimate_base_freq")
#define FA_AWAR_BASE_FREQ_A        (FA_AWAR_ISLAND_HOPPING_ROOT "base_freq_a")
#define FA_AWAR_BASE_FREQ_C        (FA_AWAR_ISLAND_HOPPING_ROOT "base_freq_c")
#define FA_AWAR_BASE_FREQ_G        (FA_AWAR_ISLAND_HOPPING_ROOT "base_freq_g")
#define FA_AWAR_BASE_FREQ_T        (FA_AWAR_ISLAND_HOPPING_ROOT "base_freq_t")

#define FA_AWAR_SUBST_PARA_AC (FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_ac")
#define FA_AWAR_SUBST_PARA_AG (FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_ag")
#define FA_AWAR_SUBST_PARA_AT (FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_at")
#define FA_AWAR_SUBST_PARA_CG (FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_cg")
#define FA_AWAR_SUBST_PARA_CT (FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_ct")
#define FA_AWAR_SUBST_PARA_GT (FA_AWAR_ISLAND_HOPPING_ROOT "subst_para_gt")

#define FA_AWAR_EXPECTED_DISTANCE (FA_AWAR_ISLAND_HOPPING_ROOT "expected_dist")
#define FA_AWAR_STRUCTURE_SUPPLEMENT (FA_AWAR_ISLAND_HOPPING_ROOT "struct_suppl")
#define FA_AWAR_THRESHOLD (FA_AWAR_ISLAND_HOPPING_ROOT "threshold")
#define FA_AWAR_GAP_A (FA_AWAR_ISLAND_HOPPING_ROOT "gapa")
#define FA_AWAR_GAP_B (FA_AWAR_ISLAND_HOPPING_ROOT "gapb")
#define FA_AWAR_GAP_C (FA_AWAR_ISLAND_HOPPING_ROOT "gapc")

// --------------------------------------------------------------------------------

static inline GB_ERROR species_not_found(GB_CSTR species_name) {
    return GB_export_error("No species '%s' found!", species_name);
}

extern GBDATA *gb_main;

static GB_ERROR reverseComplement(GBDATA *gb_species, GB_CSTR ali, int max_protection) {
    GBDATA *gbd = GBT_read_sequence(gb_species, ali);
    GB_ERROR error = 0;

    if (!gbd) {
        char *name = GBT_read_name(gb_species);
        error = GB_export_error("No 'data' found for species '%s'", name ? name : "(unknown)");
    }
    else {
        int my_protection = GB_read_security_write(gbd);

        if (my_protection<=max_protection) { // ok
            char *seq = GB_read_string(gbd);
            int length = GB_read_string_count(gbd);
            GB_alignment_type ali_type = GBT_get_alignment_type(gb_main, ali);

            char T_or_U;
            error = GBT_determine_T_or_U(ali_type, &T_or_U, "reverse-complement");
            if (!error) {
                GBT_reverseComplementNucSequence(seq, length, T_or_U);
                error = GB_write_string(gbd, seq);
            }
        }
        else { // protection error
            char *name = GBT_read_name(gb_species);
            error = GB_export_error("Cannot reverse-complement species '%s' because of protection level", name ? name : "(unknown)");
        }

    }

    return error;
}

void AWTC_build_reverse_complement(AW_window *aw, AW_CL cd2)  {
    GB_push_transaction(gb_main);
    AW_root *root = aw->get_root();
    int revComplWhat = root->awar(FA_AWAR_TO_ALIGN)->read_int();
    char *default_alignment = GBT_get_default_alignment(gb_main);
    GB_CSTR alignment = root->awar_string(AWAR_EDITOR_ALIGNMENT, default_alignment)->read_string();
    GB_ERROR error = 0;
    int max_protection = root->awar(FA_AWAR_PROTECTION)->read_int();

    switch (revComplWhat) {
        case 0: { // current species
            GB_CSTR species_name = root->awar(AWAR_SPECIES_NAME)->read_string();
            GBDATA *gb_species = GBT_find_species(gb_main, species_name);
            if (!gb_species) error = species_not_found(species_name);
            if (!error) error = reverseComplement(gb_species, alignment, max_protection);
            break;
        }
        case 1: { // marked species
            GBDATA *gb_species = GBT_first_marked_species(gb_main);

            if (!gb_species) {
                error = GB_export_error("There is no marked species");
            }
            while (gb_species) {
                error = reverseComplement(gb_species, alignment, max_protection);
                if (error) break;
                gb_species = GBT_next_marked_species(gb_species);
            }
            break;
        }
        case 2: { // selected species
            static struct AWTC_faligner_cd *cd = (struct AWTC_faligner_cd *)cd2;
            AWTC_get_first_selected_species get_first_selected_species = cd->get_first_selected_species;
            AWTC_get_next_selected_species  get_next_selected_species = cd->get_next_selected_species;
            int count = 0;
            GBDATA *gb_species = get_first_selected_species(&count);

            if (!gb_species) {
                error = GB_export_error("There is no selected species!");
            }
            while (gb_species) {
                error = reverseComplement(gb_species, alignment, max_protection);
                if (error) break;
                gb_species = get_next_selected_species();
            }
            break;
        }
        default: {
            awtc_assert(0);
            break;
        }
    }

    if (error) {
        aw_message(error);
        GB_abort_transaction(gb_main);
    }
    else {
        GB_commit_transaction(gb_main);
    }
}

// --------------------------------------------------------------------------------

class UnalignedBasesList;
class UnalignedBases
{
    int start,          // absolute positions
        end;
    UnalignedBases *next;

    friend class UnalignedBasesList;

public:
    UnalignedBases(int pos1, int posn)      { start = pos1; end = posn; next = 0; }
    ~UnalignedBases()               { delete next; }

    int get_start() const { return start; }
    int get_end() const { return end; }
};

class UnalignedBasesList
{
    UnalignedBases *head;
public:
    UnalignedBasesList() { head = 0; }
    ~UnalignedBasesList() { delete head; }

    void clear() { delete head; head = 0; }
    int is_empty() const { return head==0; }

    void memorize(int start, int end) {
#ifdef DEBUG
        printf("memorize %i-%i\n", start, end);
#endif
        awtc_assert(start<=end);
        UnalignedBases *ub = new UnalignedBases(start, end);
        ub->next = head;
        head = ub;
    }
    void recall(int *start, int *end) {
        UnalignedBases *ub = head;

        if (start) *start = ub->start;
        if (end) *end = ub->end;

#ifdef DEBUG
        printf("recall %i-%i\n", ub->start, ub->end);
#endif

        head = head->next;
        ub->next = 0;
        delete ub;
    }

    void add(UnalignedBasesList *ubl);
    void add_and_recalc_positions(UnalignedBasesList *ubl, AWTC_CompactedSubSequence *oldSequence, AWTC_CompactedSubSequence *newSequence);
};

inline void UnalignedBasesList::add(UnalignedBasesList *ubl)
{
    if (!ubl->is_empty()) {
        if (is_empty()) {
            head = ubl->head;
        }
        else {
            UnalignedBases *last = head;

            while (last->next) {
                last = last->next;
            }
            last->next = ubl->head;
        }
        ubl->head = 0;
    }
}

inline void UnalignedBasesList::add_and_recalc_positions(UnalignedBasesList *ubl, AWTC_CompactedSubSequence *oldSequence, AWTC_CompactedSubSequence *newSequence)
{
    if (!ubl->is_empty()) {
        UnalignedBases *toCorrect = ubl->head;

        awtc_assert(oldSequence->length()==newSequence->length());

        add(ubl);
        while (toCorrect) {
            int cs = oldSequence->compPosition(toCorrect->start);   // compressed positions in oldSequence = compressed positions in newSequence
            int ce = oldSequence->compPosition(toCorrect->end);

#ifdef DEBUG
            printf("add_and_recalc_positions %i/%i -> ", toCorrect->start, toCorrect->end);
#endif

            toCorrect->start = newSequence->expdPosition(cs) - newSequence->no_of_gaps_before(cs);
            toCorrect->end = newSequence->expdPosition(ce)   + newSequence->no_of_gaps_after(ce);

            //      if (cs>0) {
            //      toCorrect->start = newSequence->expdPosition(cs-1)+1;   // first position after left neighbour (to get all gaps left of part)
            //      }
            //      else {
            //      toCorrect->start = newSequence->expdStartPosition(); //newSequence->expdPosition(cs);
            //      }

            //      if (ce<newSequence->length()) {
            //      toCorrect->end = newSequence->expdPosition(ce+1)-1;     // last position before right neighbour (to get all gaps right of part)
            //      }
            //      else {
            //      toCorrect->end = newSequence->expdPosition(ce);
            //      }

#ifdef DEBUG
            printf("%i/%i\n", toCorrect->start, toCorrect->end);
#endif

            toCorrect = toCorrect->next;
        }
    }
}


static UnalignedBasesList unaligned_bases; // if fast_align cannot align (no master bases) -> stores positions here

static GB_alignment_type global_alignmentType = GB_AT_UNKNOWN; // type of actually aligned sequence

static inline GB_ERROR AWTC_memerr()
{
    return GB_export_error("Out of memory/swap space! -- Maybe your disk is full?");
}

static char *AWTC_read_name(GBDATA *gbd)
{
    if (gbd) return GBT_read_name(gbd);
    return strdup("GROUP-CONSENSUS");
}

static inline int relatedBases(char base1, char base2)
{
    return AWTC_baseMatch(base1, base2)==1;
}

static inline char alignQuality(char slave, char master)
{
    awtc_assert(slave);
    awtc_assert(master);
    char result = '#';
    if (slave==master)              result = '-';   // equal
    else if (slave==GAP_CHAR)           result = '+';   // inserted gap
    else if (master==GAP_CHAR)          result = '+';   // no gap in master
    else if (relatedBases(slave,master))    result = '~';   // mutation (related bases)
    return result;                          // mutation (non-related bases)
}

// --------------------------------------------------------------------------------
//      Debugging stuff
// --------------------------------------------------------------------------------

#ifdef DEBUG
static char *lstr(const char *s, int len)
{
    static int alloc;
    static char *buffer;

    if (alloc<(len+1))
    {
        if (alloc) free(buffer);
        buffer = (char*)malloc(alloc=len+100);
    }

    memcpy(buffer, s, len);
    buffer[len] = 0;

    return buffer;
}

#define BUFLEN 120

static inline char compareChar(char base1, char base2)
{
    return base1==base2 ? '=' : (relatedBases(base1,base2) ? 'x' : 'X');
}
static void dump_n_compare_one(const char *seq1, const char *seq2, long len, long offset)
{
    awtc_assert(len<=BUFLEN);
    char compare[BUFLEN+1];

    for (long l=0; l<len; l++)
        compare[l] = AWTC_is_gap(seq1[l]) || AWTC_is_gap(seq2[l]) ? ' ' : compareChar(seq1[l],seq2[l]);

    compare[len] = 0;

    printf(" %li '%s'\n", offset, lstr(seq1,len));
    printf(" %li '%s'\n", offset, lstr(seq2,len));
    printf(" %li '%s'\n", offset, compare);
}

static inline void dump_rest(const char *seq, long len, int idx, long offset)
{
    printf(" Rest von Sequenz %i:\n", idx);
    while (len>BUFLEN)
    {
        printf(" %li '%s'\n", offset, lstr(seq, BUFLEN));
        seq += BUFLEN;
        len -= BUFLEN;
        offset += BUFLEN;
    }

    awtc_assert(len>0);
    printf(" '%s'\n", lstr(seq, len));
}

static void dump_n_compare(const char *text, const char *seq1, long len1, const char *seq2, long len2)
{
    long offset = 0;

    printf(" Comparing %s:\n", text);

    while (len1>0 && len2>0)
    {
        long done = 0;

        if (len1>=BUFLEN && len2>=BUFLEN)
        {
            dump_n_compare_one(seq1, seq2, done=BUFLEN, offset);
        }
        else
        {
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

static void dump_n_compare(const char *text, const AWTC_CompactedSubSequence& seq1, const AWTC_CompactedSubSequence& seq2)
{
    dump_n_compare(text, seq1.text(), seq1.length(), seq2.text(), seq2.length());
}

#undef BUFLEN

static inline void dumpSeq(const char *seq, long len, long pos)
{
    printf("'%s' ", lstr(seq, len));
    printf("(Pos=%li,Len=%li)", pos, len);
}

#define dump()                                          \
do                                              \
{                                               \
    double sig = partSignificance(sequence().length(), slaveSequence.length(), bestLength); \
                                                \
    printf(" Score = %li (Significance=%f)\n"                           \
       " Master = ", bestScore, sig);                           \
       dumpSeq(bestMasterLeft.text(), bestLength, bestMasterLeft.leftOf());         \
       printf("\n"                                      \
          " Slave  = ");                                \
          dumpSeq(bestSlaveLeft.text(), bestLength, bestSlaveLeft.leftOf());        \
          printf("\n");                                 \
}                                               \
while(0)

#endif /*DEBUG*/


// --------------------------------------------------------------------------------
//  INLINE-functions used in fast_align():
// --------------------------------------------------------------------------------

static inline double log3(double d)
{
    return log(d)/log(3.0);
}
static inline double partSignificance(long seq1len, long seq2len, long partlen)
    // returns log3 of significance of the part
    // usage: partSignificance(...) < log3(maxAllowedSignificance)
{
    return log3((seq1len-partlen)*(seq2len-partlen)) - partlen;
}

#if 0
static inline void recalcUsedBuffer(char **bufPtr, long *lenPtr, long *usedPtr, long used)
{
    *bufPtr += used;
    *lenPtr -= used;
    *usedPtr += used;
}
#endif

static inline GB_ERROR bufferTooSmall()
{
    return GB_export_error("Cannot align - reserved buffer is to small");
}

static inline long insertsToNextBase(AWTC_alignBuffer *alignBuffer, const AWTC_SequencePosition& master)
{
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

static inline void insertBase(AWTC_alignBuffer *alignBuffer,
                              AWTC_SequencePosition& master, AWTC_SequencePosition& slave,
                              AWTC_fast_align_report *report)
{
    char slaveBase = *slave.text();
    char masterBase = *master.text();

    alignBuffer->set(slaveBase, alignQuality(slaveBase, masterBase));
    report->count_aligned_base(slaveBase!=masterBase);
    ++slave;
    ++master;
}

static inline void insertSlaveBases(AWTC_alignBuffer *alignBuffer,
                                    AWTC_SequencePosition& slave,
                                    int length,
                                    AWTC_fast_align_report *report)
{
    alignBuffer->copy(slave.text(), alignQuality(*slave.text(),GAP_CHAR), length);
    report->count_unaligned_base(length);
    slave += length;
}

static inline void insertGap(AWTC_alignBuffer *alignBuffer,
                             AWTC_SequencePosition& master,
                             AWTC_fast_align_report *report)
{
    char masterBase = *master.text();

    alignBuffer->set(GAP_CHAR, alignQuality(GAP_CHAR, masterBase));
    report->count_aligned_base(masterBase!=GAP_CHAR);
    ++master;
}

static inline GB_ERROR insertClustalValigned(AWTC_alignBuffer *alignBuffer,
                                             AWTC_SequencePosition& master,
                                             AWTC_SequencePosition& slave,
                                             const char *masterAlignment, const char *slaveAlignment, long alignmentLength,
                                             AWTC_fast_align_report *report)
    // inserts bases of 'slave' to 'alignBuffer' according to alignment in 'masterAlignment' and 'slaveAlignment'
{
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
                awtc_assert(insertsToNextBase(alignBuffer,master)==0);
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
            //int memo = 0;

            awtc_assert(masterAlignment[pos]==GAP);
            for (slave_bases=1; pos+slave_bases<alignmentLength && masterAlignment[pos+slave_bases]==GAP; slave_bases++) {
                ; // count gaps in master (= # of slave bases to insert)
            }
            if (!baseAtLeft && insert>slave_bases) {
                int ins_gaps = insert-slave_bases;

                awtc_assert(alignBuffer->free()>=ins_gaps);
                alignBuffer->set(GAP_CHAR, alignQuality(GAP_CHAR, GAP_CHAR), ins_gaps);
                insert -= ins_gaps;
            }

            if (insert<slave_bases) { // master has less gaps than slave bases to insert
                report->memorize_insertion(master.expdPosition(), slave_bases-insert);
            }
            else if (insert>slave_bases) { // master has more gaps than slave bases to insert
                awtc_assert(baseAtLeft);
            }

            int start = slave.expdPosition(); // memorize base positions without counterpart in master
            int end = slave.expdPosition(slave_bases-1);
            unaligned_bases.memorize(start, end);

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

static inline GB_ERROR insertAligned(AWTC_alignBuffer *alignBuffer,
                                     AWTC_SequencePosition& master, AWTC_SequencePosition& slave, long partLength,
                                     AWTC_fast_align_report *report)
    // insert bases of 'slave' to 'alignBuffer' according to 'master'
{
    if (partLength) {
        long insert = insertsToNextBase(alignBuffer, master);

        awtc_assert(partLength>=0);

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
                insert = insertsToNextBase(alignBuffer,master);
            }

            awtc_assert(partLength>=0);
            if (partLength==0) { // all inserted
                return NULL;
            }
        }

        awtc_assert(insert>=0);

        if (insert>0) { // insert gaps into slave
            if (insert>alignBuffer->free()) return bufferTooSmall();
            alignBuffer->set(GAP_CHAR, alignQuality(GAP_CHAR, GAP_CHAR), insert);
            awtc_assert(insertsToNextBase(alignBuffer,master)==0);
        }

        awtc_assert(partLength>=0);

        while (partLength--) {
            insert = insertsToNextBase(alignBuffer,master);

            awtc_assert(insert>=0);
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

static inline GB_ERROR cannot_fast_align(const AWTC_CompactedSubSequence& master, long moffset, long mlength,
                                         const AWTC_CompactedSubSequence& slaveSequence, long soffset, long slength,
                                         int max_seq_length,
                                         AWTC_alignBuffer *alignBuffer,
                                         AWTC_fast_align_report *report)
{
    const char *mtext = master.text(moffset);
    const char *stext = slaveSequence.text(soffset);
    char *maligned, *saligned;
    int len;
    GB_ERROR error = 0;

    if (slength) {
        if (mlength) { // if slave- and master-sequences contain bases, we call the slow aligner
            int score;

#if defined(DEBUG) && 1
            printf("ClustalV-Align:\n");
            printf(" mseq = '%s'\n", lstr(mtext, mlength));
            printf(" sseq = '%s'\n", lstr(stext, slength));
#endif

            int is_dna = -1;

            switch (global_alignmentType) {
                case GB_AT_RNA:
                case GB_AT_DNA: is_dna = 1; break;
                case GB_AT_AA:  is_dna = 0; break;
                default: error = GB_export_error("Unknown alignment type - aligner aborted"); break;
            }

            if (!error) {
                error = AWTC_ClustalV_align(is_dna,
                                            1,
                                            mtext, mlength, stext, slength,
                                            master.gapsBefore(moffset),
                                            max_seq_length,
                                            &maligned, &saligned, &len, &score);
            }

            if (!error) {
#if defined(DEBUG) && 1
                printf("ClustalV returns:\n");
                printf(" maligned = '%s'\n", lstr(maligned, len));
                printf(" saligned = '%s'\n", lstr(saligned, len));
#endif

                AWTC_SequencePosition masterPos(master, moffset);
                AWTC_SequencePosition slavePos(slaveSequence, soffset);

                error = insertClustalValigned(alignBuffer, masterPos, slavePos, maligned, saligned, len, report);

#if (defined(DEBUG) && 0)

                AWTC_SequencePosition master2(master->sequence(), moffset);
                AWTC_SequencePosition slave2(slaveSequence, soffset);
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

                printf(" master = '%s'\n", lstr(maligned,len));
                printf(" slave  = '%s'\n", lstr(saligned,len));
                printf("          '%s'\n", lstr(cmp,len));

                delete [] cmp;
#endif
            }
        }
        else { // master is empty here, so we just copy in the slave bases
            if (slength<=alignBuffer->free()) {
                int start = slaveSequence.expdPosition(soffset);
                int end = slaveSequence.expdPosition(soffset+slength-1);

                unaligned_bases.memorize(start, end);
                //      int fill = start-alignBuffer->offset();
                //      if (fill>0) {
                //          alignBuffer->set(GAP_CHAR, alignQuality(GAP_CHAR, GAP_CHAR), fill);
                //      }
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

// --------------------------------------------------------------------------------
//      #define's for fast_align()
// --------------------------------------------------------------------------------

#define TEST_BETTER_SCORE()                                 \
do                                                          \
{                                                           \
    if (score>bestScore)                                    \
    {                                                       \
    bestScore = score;                                      \
    bestLength = masterRight.text() - masterLeft.text();    \
    bestMasterLeft = masterLeft;                            \
    bestSlaveLeft = slaveLeft;                              \
    /*dump();*/                                             \
    }                                                       \
}                                                           \
while (0)

#define CAN_SCORE_LEFT()    (masterLeft.leftOf() && slaveLeft.leftOf())
#define CAN_SCORE_RIGHT()   (masterRight.rightOf() && slaveRight.rightOf())

#define SCORE_LEFT()                                                            \
do                                                                              \
{                                                                               \
    score += *(--masterLeft).text()==*(--slaveLeft).text() ? match : mismatch;  \
    TEST_BETTER_SCORE();                                                        \
}                                                                               \
while (0)

#define SCORE_RIGHT()                                                               \
do                                                                                  \
{                                                                                   \
    score += *(++masterRight).text()==*(++slaveRight).text() ? match : mismatch;    \
    TEST_BETTER_SCORE();                                                            \
}                                                                                   \
while (0)

// --------------------------------------------------------------------------------
//  AWTC_FastSearchSequence::fast_align()
// --------------------------------------------------------------------------------

GB_ERROR AWTC_FastSearchSequence::fast_align(const AWTC_CompactedSubSequence& slaveSequence,
                                             AWTC_alignBuffer *alignBuffer,
                                             int max_seq_length,
                                             int match, int mismatch,
                                             AWTC_fast_align_report *report) const
    //
    // aligns 'slaveSequence' to 'this'
    //
    // returns ==NULL -> all ok -> 'alignBuffer' contains aligned sequence
    //     !=NULL -> failure -> no results
    //
{
    GB_ERROR error = NULL;
    int aligned = 0;

    // set the following #if to zero to test ClustalV-Aligner (not fast_aligner)
#if 1

    static double lowSignificance;
    static int lowSignificanceInitialized;

    if (!lowSignificanceInitialized) {
        lowSignificance = log3(0.01);
        //printf("lowSignificance=%f\n", lowSignificance);
        lowSignificanceInitialized = 1;
    }

    AWTC_SequencePosition slave(slaveSequence);
    long bestScore=0;
    AWTC_SequencePosition bestMasterLeft(sequence());
    AWTC_SequencePosition bestSlaveLeft(slaveSequence);
    long bestLength=0;

    while (slave.rightOf()>=3) { // with all triples of slaveSequence
        AWTC_FastSearchOccurance occurance(*this, slave.text()); // "search" first
        AWTC_SequencePosition rightmostSlave = slave;

        while (occurance.found()) { // with all occurances of the triple
            long score = match*3;
            AWTC_SequencePosition masterLeft(occurance.sequence(), occurance.offset());
            AWTC_SequencePosition masterRight(occurance.sequence(), occurance.offset()+3);
            AWTC_SequencePosition slaveLeft(slave);
            AWTC_SequencePosition slaveRight(slave,3);

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

            occurance.gotoNext(); // "search" next

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

        //dump();

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
                    AWTC_CompactedSubSequence   leftCompactedMaster(sequence(), 0, masterLeftOf);
                    AWTC_FastSearchSequence     leftMaster(leftCompactedMaster);

                    error = leftMaster.fast_align(AWTC_CompactedSubSequence(slaveSequence, 0, slaveLeftOf),
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
                    AWTC_CompactedSubSequence rightCompactedMaster(sequence(), masterRightStart, masterRightOf);
                    AWTC_FastSearchSequence rightMaster(rightCompactedMaster);

                    error = rightMaster.fast_align(AWTC_CompactedSubSequence(slaveSequence, slaveRightStart, slaveRightOf),
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
        else {
            //printf("Not significant!\n");
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

// --------------------------------------------------------------------------------
//  Tools for AWTC_aligner()
// --------------------------------------------------------------------------------

static long calcSequenceChecksum(const char *data, int length)
{
    static char *gaps;

    if (!gaps)
    {
        gaps = (char*)malloc(257);

        int c = 1;
        int cnt = 0;

        while (c<256)
        {
            if (AWTC_is_gap(toupper(c)))
                gaps[cnt++] = c;
            c++;
        }

        gaps[cnt] = 0;
    }

    long sum = GB_checksum(data, length, 1, gaps);
    return sum;
}

#if 0
static char *readExpandedSequence(GBDATA *gb_species, const char *ali, GB_ERROR *errorPtr, int *readLength)
{
    GBDATA *gbd;
    char *name = GBT_read_name(gb_species);
    char *data = 0;

    *errorPtr = 0;

    gbd = GBT_read_sequence(gb_species, ali);
    if (gbd) {
        data = GB_read_string(gbd);
        *readLength = GB_read_string_count(gbd);
    }
    else {
        *errorPtr = GB_export_error("No 'data' found for species '%s'", name ? name : "(unknown)");
    }

    return data;
}
#endif

static AWTC_CompactedSubSequence *readCompactedSequence(GBDATA *gb_species, const char *ali,
                                                        GB_ERROR *errorPtr,
                                                        char **dataPtr,     // if dataPtr!=NULL it will be set to the WHOLE uncompacted sequence
                                                        long *seqChksum,    // may be NULL (of part of sequence)
                                                        int firstColumn,    // return only part of the sequence
                                                        int lastColumn) // (-1=till end of sequence)
{
    GB_ERROR                   error = 0;
    GBDATA                    *gbd;
    AWTC_CompactedSubSequence *seq   = 0;
    char                      *name  = GBT_read_name(gb_species);

    gbd = GBT_read_sequence(gb_species, ali);       // get sequence

    if (gbd)
    {
        long length = GB_read_string_count(gbd);
        char *data = GB_read_string(gbd);
        long partLength;
        char *partData;

        if (dataPtr) {                  // make a copy of the whole uncompacted sequence (returned to caller)
            *dataPtr = data;
        }

        if (firstColumn!=0 || lastColumn!=-1) {     // take only part of sequence
            awtc_assert(firstColumn>=0);
            awtc_assert(firstColumn<=lastColumn);

            // include all surrounding gaps
            while (firstColumn>0 && AWTC_is_gap(data[firstColumn-1])) {
                firstColumn--;
            }
            if (lastColumn!=-1) {
                while (lastColumn<(length+1) && AWTC_is_gap(data[lastColumn+1])) {
                    lastColumn++;
                }
            }

            partData = data+firstColumn;
            int slen = length-firstColumn;

            awtc_assert(slen>=0);
            awtc_assert((size_t)slen==strlen(partData));

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

            seq = new AWTC_CompactedSubSequence(partData, partLength, name, firstColumn);
        }

        if (!dataPtr) {     // free sequence only if user has not requested to get it
            free(data);
        }
    }
    else {
        error = GB_export_error("No 'data' found for species '%s'", name ? name : "(unknown)");
        if (dataPtr) *dataPtr = NULL; // (user must not care to free data if we fail)
    }

    awtc_assert(errorPtr);
    *errorPtr = error;

    free(name);

    return seq;
}

static GB_ERROR writeStringToAlignment(GBDATA *gb_species, GB_CSTR alignment, GB_CSTR data_name, GB_CSTR string, int temporary)
{
    GBDATA *gb_ali = GB_search(gb_species, alignment, GB_DB);
    GB_ERROR error = NULL;
    GBDATA *gb_name = GB_search(gb_ali, data_name, GB_STRING);

    if (gb_name)
    {
        awtc_assert(GB_check_father(gb_name, gb_ali));
        error = GB_write_string(gb_name, string);
        if (temporary && !error)
            error = GB_set_temporary(gb_name);
    }
    else
    {
        GBDATA *gb_species_name = GB_find(gb_species, "name", 0, down_level);

        error = GB_export_error("Cannot create entry '%s' for '%s'",
                                data_name,
                                gb_species_name ? GB_read_char_pntr(gb_species_name) : "(noname)");
    }

    return error;
}

// --------------------------------------------------------------------------------


static int actualSequenceNumber,    // only used for counter in
    overallSequenceNumber;

static GB_ERROR alignCompactedTo(AWTC_CompactedSubSequence *toAlignSequence,
                                 const AWTC_FastSearchSequence *alignTo,
                                 int max_seq_length, GB_CSTR alignment, long toAlignChksum, int temporary,
                                 GBDATA *gb_toAlign,
                                 GBDATA *gb_alignTo, // may be NULL
                                 int firstColumn, int lastColumn,
                                 int showGapsMessages)
    // if only part of the sequence should be aligned, then this functions already gets only the part
    // (i.o.w.: toAlignSequence, alignTo and toAlignChksum refer to the partial sequence)
{
    AWTC_alignBuffer  alignBuffer(max_seq_length);
    char             *master_name = AWTC_read_name(gb_alignTo);

    AWTC_fast_align_report report(master_name, showGapsMessages);

    {
        char stat_buf[200];
        char *to_align_name = AWTC_read_name(gb_toAlign);
        char *align_to_name = AWTC_read_name(gb_alignTo);

        sprintf(stat_buf, "Aligning #%i:%i %s (to %s)",
                actualSequenceNumber, overallSequenceNumber,
                to_align_name, align_to_name);

        static GBDATA *last_gb_toAlign;
        if (gb_toAlign!=last_gb_toAlign) {
            last_gb_toAlign = gb_toAlign;
            actualSequenceNumber++;
        }

        aw_status(stat_buf);

        free(to_align_name);
        free(align_to_name);
    }

#if (defined(DEBUG) && 1)
    printf("alignCompactedTo(): master='%s' ", master_name);
    printf("slave='%s'\n", toAlignSequence->name());
#endif

    GB_ERROR error = GB_pop_transaction(gb_toAlign);
    GB_ERROR err;

    if (error) goto ende;

    if (island_hopper) {
        error = island_hopper->do_align();
        if (!error) {
            awtc_assert(island_hopper->was_aligned());

#if defined(DEBUG)
            printf("Island-Hopper returns:\n");
            printf("maligned = '%s'\n", lstr(island_hopper->get_result_ref(), island_hopper->get_result_length()));
            printf("saligned = '%s'\n", lstr(island_hopper->get_result(), island_hopper->get_result_length()));
#endif // DEBUG

            AWTC_SequencePosition masterPos(alignTo->sequence(), 0);
            AWTC_SequencePosition slavePos(*toAlignSequence, 0);

            error = insertClustalValigned(&alignBuffer, masterPos, slavePos, island_hopper->get_result_ref(), island_hopper->get_result(), island_hopper->get_result_length(), &report);
        }
    }
    else {
        error = alignTo->fast_align(*toAlignSequence, &alignBuffer, max_seq_length, 2, -10, &report); // <- here we align
    }

    if (!error) {
        alignBuffer.correctUnalignedPositions();
        if (alignBuffer.free()) {
            alignBuffer.set('-', alignQuality(GAP_CHAR,GAP_CHAR), alignBuffer.free());  // rest of alignBuffer is set to '-'
        }
        alignBuffer.expandPoints(*toAlignSequence);
    }

#if (defined(DEBUG) && 1)
    {
        AWTC_CompactedSubSequence alignedSlave(alignBuffer.text(), alignBuffer.length(), toAlignSequence->name());
        dump_n_compare("reference vs. aligned:", alignTo->sequence(), alignedSlave);
    }
#endif

    err = GB_push_transaction(gb_toAlign);
    if (!error) error = err;
    if (error) goto ende;

    if (calcSequenceChecksum(alignBuffer.text(), alignBuffer.length())!=toAlignChksum) { // sequence-chksum changed
        error = GB_export_error("Internal aligner error (sequence checksum changed) -- aborted");

# ifdef DEBUG
        AWTC_CompactedSubSequence alignedSlave(alignBuffer.text(), alignBuffer.length(), toAlignSequence->name());
        dump_n_compare("Old Slave vs. new Slave", *toAlignSequence, alignedSlave);
# endif
    }
    else
    {
        error = GB_push_my_security(gb_toAlign);
        if (!error) {
            GBDATA *gbd = GBT_add_data(gb_toAlign, alignment, "data", GB_STRING);

            if (!gbd) {
                error = GB_export_error("Can't find/create sequence data");
            }
            else {
                if (firstColumn!=0 || lastColumn!=-1) {                         // we aligned just a part of the sequence
                    char *buffer = GB_read_string(gbd);                         // so we read old sequence data
                    long len = GB_read_string_count(gbd);

                    if (!buffer) {
                        error = GB_get_error();
                    }
                    else {
                        int lenToCopy = lastColumn-firstColumn+1;
                        long wholeChksum = calcSequenceChecksum(buffer,len);
                        //long partChksum = calcSequenceChecksum(buffer+firstColumn, lenToCopy);

                        memcpy(buffer+firstColumn, alignBuffer.text()+firstColumn, lenToCopy);  // copy in the aligned part

                        if (calcSequenceChecksum(buffer, len)!=wholeChksum) {
                            error = GB_export_error("Internal aligner error (sequence checksum changed) -- aborted");
                        }
                        else {
                            GB_write_string(gbd,"");
                            error = GB_write_string(gbd, buffer);
                        }
                    }

                    free(buffer);
                }
                else {
                    alignBuffer.point_ends_of();
                    error = GBT_write_sequence(gbd, alignment, max_seq_length, alignBuffer.text()); // aligned all -> write all
                }
            }
        }
        err = GB_pop_my_security(gb_toAlign);
        if (!error) error = err;
        if (error) goto ende;

        if (temporary)
        {
            // create temp-entry for slave containing alignment quality:

            error = writeStringToAlignment(gb_toAlign, alignment, QUALITY_NAME, alignBuffer.quality(), temporary==1);
            if (error) goto ende;

            // create temp-entry for master containing insert points:

            int buflen = max_seq_length*2;
            char *buffer = (char*)malloc(buflen+1);
            char *afterLast = buffer;

            if (!buffer)
            {
                AWTC_memerr();
                goto ende;
            }
            memset(buffer, '-', buflen);
            buffer[buflen] = 0;

            const AWTC_fast_align_insertion *inserts = report.insertion();
            while (inserts)
            {
                memset(buffer+inserts->offset(), '>', inserts->gaps());
                afterLast = buffer+inserts->offset()+inserts->gaps();
                inserts = inserts->next();
            }

            *afterLast = 0;
            if (gb_alignTo) {
                error = writeStringToAlignment(gb_alignTo, alignment, INSERTS_NAME, buffer,temporary==1);
            }
        }
    }
 ende:
    free(master_name);
    return error;
}

GB_ERROR AWTC_delete_temp_entries(GBDATA *gb_species, GB_CSTR alignment)
{
    awtc_assert(gb_species);
    awtc_assert(alignment);

    GBDATA *gb_ali = GB_search(gb_species, alignment, GB_FIND);
    GB_ERROR error = NULL;

    if (gb_ali)
    {
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

static GB_ERROR GB_align_error(GB_ERROR olderr, GBDATA *gb_toAlign, GBDATA *gb_alignTo)
    // used by alignTo() and alignToNextRelative() to transform errors coming from subroutines
    // can be used by higher functions
{
    aw_message(olderr);

    char *name_toAlign = AWTC_read_name(gb_toAlign);
    char *name_alignTo = AWTC_read_name(gb_alignTo);
    GB_ERROR neu = GB_export_error("..while aligning '%s' to '%s'", name_toAlign, name_alignTo);

    free(name_alignTo);
    free(name_toAlign);

    return neu;
}

static GB_ERROR alignTo(GBDATA *gb_toAlign, GB_CSTR alignment,
                        const AWTC_FastSearchSequence *alignTo,
                        GBDATA *gb_alignTo, // may be NULL
                        int max_seq_length, int temporary,
                        int firstColumn, int lastColumn, int showGapsMessages)
{
    GB_ERROR                   error           = NULL;
    long                       chksum;
    AWTC_CompactedSubSequence *toAlignSequence = readCompactedSequence(gb_toAlign, alignment, &error, NULL, &chksum, firstColumn, lastColumn);

    if (island_hopper) {
        GBDATA *gb_seq = GBT_read_sequence(gb_toAlign, alignment);      // get sequence
        if (gb_seq) {
            long        length = GB_read_string_count(gb_seq);
            const char *data   = GB_read_char_pntr(gb_seq);

            island_hopper->set_toAlign_sequence(data);
            island_hopper->set_alignment_length(length);
        }
    }



    if (!error)
    {
        error = alignCompactedTo(toAlignSequence, alignTo,
                                 max_seq_length, alignment, chksum, temporary,
                                 gb_toAlign, gb_alignTo,
                                 firstColumn, lastColumn, showGapsMessages);
        if (error) error = GB_align_error(error, gb_toAlign, gb_alignTo);
        delete toAlignSequence;
    }

    return error;
}

static GB_ERROR alignToGroupConsensus(GBDATA *gb_toAlign, GB_CSTR alignment,
                                      AWTC_get_consensus_func get_consensus,
                                      int max_seq_length, int temporary,
                                      int firstColumn, int lastColumn, int showGapsMessages)
{
    GB_ERROR  error        = 0;
    char     *species_name = AWTC_read_name(gb_toAlign);
    char     *consensus    = get_consensus(species_name, firstColumn, lastColumn);
    size_t    cons_len     = strlen(consensus);

    for (size_t i = 0; i<cons_len; ++i) { // translate consensus to be accepted by aligner
        switch (consensus[i]) {
            case '=': consensus[i] = '-'; break;
            default : break;
        }
    }

    AWTC_CompactedSubSequence compacted(consensus, cons_len, "group consensus");

    {
        AWTC_FastSearchSequence fast(compacted);
        error = alignTo(gb_toAlign, alignment, &fast, NULL, max_seq_length, temporary, firstColumn, lastColumn, showGapsMessages);
    }

    free(consensus);
    free(species_name);

    return error;
}

inline long middle(long l1, long l2, long l3)
{
    long result;
    if (l1<l2) {
        if (l2<l3) {
            result =  l2;
        }else{
            // l2>=l3
            result = l1<l3 ? l3 : l1;
        }
    }else{
        // l1>=l2
        if (l3<l2) {
            result =  l2;
        }else{
            // l3>=l2
            result =  l1<l3 ? l1 : l3;
        }
    }
    return result;
}

inline void swap(long &l1, long &l2) { long l = l1; l1 = l2; l2 = l; }
inline void swap(char* &s1, char* &s2) { char* s = s1; s1 = s2; s2 = s; }

inline void bubbleSort(long *score, char **relative, int anz)
{
    if (anz>1) {
        int l = 0;
        int l2;
        int h = anz-1;
        int h2;
        int i;

        while (1) {
            int changes;

            h2 = h+1;
            changes = 0;
            for (i=l; i<h; i++) {
                if (score[i]<score[i+1]) { // sort big scores first
                    swap(score[i], score[i+1]);
                    swap(relative[i], relative[i+1]);
                    h2 = i+1;
                    changes = 1;
                }
            }
            h = h2-1;
            if (!changes) break;

            l2 = l-1;
            changes = 0;
            for (i=h; i>l; i--) {
                if (score[i-1]<score[i]) { // sort big scores first
                    swap(score[i-1], score[i]);
                    swap(relative[i-1], relative[i]);
                    l2 = i-1;
                    changes = 1;
                }
            }
            l = l2+1;
            if (!changes) break;
        }
    }
}

inline void mixBelow(const char *toMix, char *mixTo, int firstPos, int lastPos)
{
    int i;

    awtc_assert(firstPos<=lastPos);

    for (i=firstPos; i<=lastPos; i++) {
        if (AWTC_is_gap(mixTo[i])) {
            mixTo[i] = toMix[i];
        }
    }
}

static void appendName(char **toString, GBDATA *gb_species) {
    char *name      = GBT_read_name(gb_species);
    if (!name) name = strdup("-unnamed-");

    fprintf(stderr, "appendName(%s, %s)\n", *toString, name);

    char *newString = 0;
    if (*toString) {
        newString = GBS_global_string_copy("%s, %s", *toString, name);
        free(name);
    }
    else {
        newString = name;
    }

    free(*toString);
    *toString = newString;
}

inline int min(int i, int j) { return i<j ? i : j; }

static GB_ERROR alignToNextRelative(int      pt_server_id,
                                    GB_CSTR  pt_server_alignment,
                                    int      max_seq_length,
                                    int      temporary,
                                    int      turnAllowed,
                                    GB_CSTR  alignment,
                                    GBDATA  *gb_toAlign,
                                    int      firstColumn,
                                    int      lastColumn,
                                    int      maxNextRelatives,
                                    int      showGapsMessages)
{
    AWTC_CompactedSubSequence *toAlignSequence = NULL;
    bool use_different_pt_server_alignment = 0 != strcmp(pt_server_alignment, alignment);

    GB_ERROR   error;
    long       chksum;
    int        restart         = 1;
    int        relativesToTest = maxNextRelatives*2; // get more relatives from pt-server (needed when use_different_pt_server_alignment == true)
    char     **nearestRelative = new char*[relativesToTest+1];
    long      *relativeScore   = new long[relativesToTest+1];
    int        next_relatives;
    int        i;

    if (use_different_pt_server_alignment) {
        turnAllowed = 0; // makes no sense if we're using a different alignment for the pt_server
    }

    for (next_relatives=0; next_relatives<relativesToTest; next_relatives++) {
        nearestRelative[next_relatives] = 0;
    }
    next_relatives = 0;

    while (restart) {
        char *toAlignExpSequence = 0;

        restart = 0;

        if (use_different_pt_server_alignment) {
            toAlignSequence = readCompactedSequence(gb_toAlign, alignment, &error, 0, &chksum, firstColumn, lastColumn);

            GBDATA *gbd = GBT_read_sequence(gb_toAlign, pt_server_alignment); // use a different alignment for next relative search
            if (!gbd) {
                char *name = GBT_read_name(gb_toAlign);
                error      = GB_export_error("Species '%s' has no data in alignment '%s'",
                                             name ? name : "unknown", pt_server_alignment);
                free(name);
            }
            else {
                toAlignExpSequence = GB_read_string(gbd);
            }
        }
        else {
            toAlignSequence = readCompactedSequence(gb_toAlign, alignment, &error, &toAlignExpSequence, &chksum, firstColumn, lastColumn);
        }

        if (error) {
            return error;
        }

        while (next_relatives) {
            next_relatives--;
            free(nearestRelative[next_relatives]);
            nearestRelative[next_relatives] = 0;
        }

        {                       // find relatives
            AWTC_FIND_FAMILY         family(gb_main);
            AWTC_FIND_FAMILY_MEMBER *fl;
            long                     bestScore = -1;

            aw_status("Starting PT_SERVER");
            error = family.go(pt_server_id,toAlignExpSequence,0,relativesToTest+1);
            if (!error) {
                for (fl = family.family_list; fl; fl=fl->next) {
                    if (strcmp(toAlignSequence->name(), fl->name)!=0) {
                        if (GBT_find_species(gb_main,fl->name)) {
                            if (fl->matches>=bestScore) {
                                bestScore = fl->matches;
                            }

                            if (next_relatives<(relativesToTest+1)) {
                                nearestRelative[next_relatives] = strdup(fl->name);
                                relativeScore[next_relatives] = fl->matches;
                                next_relatives++;
                            }
                        }
                    }
                }
            }

            if (!error && turnAllowed) { // test if mirrored sequence has better relatives
                char *mirroredSequence = strdup(toAlignExpSequence);
                long length = strlen(mirroredSequence);
                long bestMirroredScore = -1;

                char T_or_U;
                error = GBT_determine_T_or_U(global_alignmentType, &T_or_U, "reverse-complement");
                GBT_reverseComplementNucSequence(mirroredSequence, length, T_or_U);

                error = family.go(pt_server_id, mirroredSequence,0,relativesToTest+1);
                if (!error) {
                    for (fl=family.family_list; fl; fl=fl->next) {
                        if (fl->matches>=bestMirroredScore && strcmp(toAlignSequence->name(), fl->name)!=0) {
                            if (GBT_find_species(gb_main, fl->name)) {
                                bestMirroredScore = fl->matches;
                            }
                        }
                    }
                }

                int turnIt = 0;

                if (bestMirroredScore>bestScore) {
                    if (turnAllowed==1) {
                        char message[200];
                        sprintf(message, "'%s' seems to be the other way round (matches=%li, matches if turned=%li)",
                                toAlignSequence->name(), bestScore, bestMirroredScore);

                        turnIt = aw_message(message, "Turn sequence,Leave sequence alone")==0;
                    }
                    else {
                        turnIt = 1;
                    }
                }

                if (turnIt) { // write mirrored sequence
                    GBDATA *gbd = GBT_read_sequence(gb_toAlign, alignment);
                    error = GB_push_my_security(gbd);
                    if (!error) error = GB_write_string(gbd, mirroredSequence);
                    GB_pop_my_security(gbd);
                    if (!error) {
                        delete toAlignSequence;
                        restart = 1; // continue while loop
                    }
                }

                free(mirroredSequence);
            }
        }
        free(toAlignExpSequence);
    }

    if (!error) {
        if (!next_relatives) {
            char warning[200];
            sprintf(warning, "No relative found for '%s'", toAlignSequence->name());
            aw_message(warning);
        }
        else {
            // sort relatives

            if (next_relatives>1) {
                bubbleSort(relativeScore, nearestRelative, next_relatives);
#ifdef DEBUG
                for (i=1; i<next_relatives; i++) {
                    awtc_assert(relativeScore[i-1]>=relativeScore[i]);
                }
#endif
            }

            // get data pointers

            typedef GBDATA *GBDATAP;
            GBDATAP *gb_reference = new GBDATAP[maxNextRelatives];

            {
                for (i=0; i<maxNextRelatives && i<next_relatives; i++) {
                    GBDATA *gb_species = GBT_find_species(gb_main, nearestRelative[i]);
                    if (!gb_species) { // pt-server seems not up to date!
                        error = species_not_found(nearestRelative[i]);
                        break;
                    }

                    GBDATA *gb_sequence = GBT_read_sequence(gb_species, alignment);
                    if (gb_sequence) { // if relative has sequence data in the current alignment ..
                        gb_reference[i] = gb_species;
                    }
                    else { // remove this relative
                        free(nearestRelative[i]);

                        for (int j = i+1; j<next_relatives; ++j) {
                            nearestRelative[j-1] = nearestRelative[j];
                            relativeScore[j-1]   = relativeScore[j];
                        }

                        next_relatives--;

                        nearestRelative[next_relatives] = 0;
                        relativeScore[next_relatives]   = 0;

                        i--; // redo same index
                    }
                }

                for (; i<next_relatives; ++i) { // delete superfluous relatives
                    free(nearestRelative[i]);
                    nearestRelative[i] = 0;
                    relativeScore[i]   = 0;
                }

                if (next_relatives>maxNextRelatives) {
                    next_relatives = maxNextRelatives;
                }
            }

            // align

            if (!error) {
                AWTC_CompactedSubSequence *alignToSequence = readCompactedSequence(gb_reference[0], alignment, &error, NULL, NULL, firstColumn, lastColumn);
                awtc_assert(alignToSequence != 0);

                if (island_hopper) {
                    GBDATA *gb_ref   = GBT_read_sequence(gb_reference[0], alignment); // get reference sequence
                    GBDATA *gb_align = GBT_read_sequence(gb_toAlign, alignment); // get sequence to align

                    if (gb_ref && gb_align) {
                        long        length_ref   = GB_read_string_count(gb_ref);
                        //                         long        length_align = GB_read_string_count(gb_align);
                        const char *data;

                        data = GB_read_char_pntr(gb_ref);
                        island_hopper->set_ref_sequence(data);

                        data = GB_read_char_pntr(gb_align);
                        island_hopper->set_toAlign_sequence(data);

                        island_hopper->set_alignment_length(length_ref);
                    }
                }

                {
                    AWTC_FastSearchSequence referenceFastSeq(*alignToSequence);

                    error = alignCompactedTo(toAlignSequence, &referenceFastSeq,
                                             max_seq_length, alignment, chksum, temporary,
                                             gb_toAlign, gb_reference[0],
                                             firstColumn, lastColumn, showGapsMessages);
                }

                if (error) {
                    error = GB_align_error(error, gb_toAlign, gb_reference[0]);
                }
                else {
                    char *used_relatives = 0;
                    appendName(&used_relatives, gb_reference[0]);

                    if (/*0 && */!unaligned_bases.is_empty()) {
                        if (island_hopper) {
                            if (next_relatives>1) error = "Island hopping uses only one relative";
                        }
                        else {
                            UnalignedBasesList ubl;
                            UnalignedBasesList ubl_for_next_relative;

                            {
                                AWTC_CompactedSubSequence *alignedSequence = readCompactedSequence(gb_toAlign, alignment, &error, 0, 0, firstColumn, lastColumn);

                                ubl.add_and_recalc_positions(&unaligned_bases, toAlignSequence, alignedSequence);
                                // now ubl holds the unaligned (and recalculated) parts from last relative
                                delete alignedSequence;
                            }

                            for (i=1; i<next_relatives && !error; i++) {
                                ubl.add(&ubl_for_next_relative);

                                bool relative_was_used = false;
                                while (!ubl.is_empty() && !error) {
                                    int start, end;
                                    ubl.recall(&start, &end);

                                    awtc_assert(firstColumn<=start && start<=end && (end<=lastColumn || lastColumn==-1));

                                    AWTC_CompactedSubSequence *alignToPart = readCompactedSequence(gb_reference[i], alignment, &error, 0, 0, start, end);
                                    if (error) break;

                                    long part_chksum;
                                    AWTC_CompactedSubSequence *toAlignPart = readCompactedSequence(gb_toAlign, alignment, &error, 0, &part_chksum, start, end);
                                    if (error) break;

                                    AWTC_FastSearchSequence referenceFastSeq(*alignToPart);
                                    error             = alignCompactedTo(toAlignPart, &referenceFastSeq,
                                                                         max_seq_length, alignment, part_chksum, temporary,
                                                                         gb_toAlign, gb_reference[i], start, end, showGapsMessages);
                                    relative_was_used = true;

                                    AWTC_CompactedSubSequence *alignedPart = readCompactedSequence(gb_toAlign, alignment, &error, 0, 0, start, end);
                                    ubl_for_next_relative.add_and_recalc_positions(&unaligned_bases, toAlignPart, alignedPart);

                                    delete alignedPart;
                                    delete alignToPart;
                                    delete toAlignPart;
                                }

                                if (relative_was_used || 1) {
                                    appendName(&used_relatives, gb_reference[i]);
                                }
                            }
                        }
                    }

                    if (!error) { // write used relatives to db-field
                        GBDATA *gb_used_relatives = GB_search(gb_toAlign, "used_rels", GB_STRING);
                        GB_write_string(gb_used_relatives, used_relatives);
                    }
                }

                delete alignToSequence;
            }

            delete [] gb_reference;
        }
    }

    delete toAlignSequence;

    for (i=0; i<next_relatives; i++) {
        free(nearestRelative[i]);
        nearestRelative[i] = 0;
    }
    delete [] nearestRelative;
    delete [] relativeScore;

    return error;
}

// --------------------------------------------------------------------------------
//  AWTC_aligner()
// --------------------------------------------------------------------------------

static GB_ERROR AWTC_aligner(GB_CSTR                         reference, // name of reference species
                             AWTC_get_consensus_func         get_consensus, // function to get consensus seq
                             int                             pt_server_id, // pt_server to search for next relative
                             GB_CSTR                         pt_server_alignment, // alignment used in pt_server (may differ from 'alignment')
                             GB_CSTR                         alignment, // name of alignment to use (==NULL -> use default)
                             GB_CSTR                         toalign, // name of species to align
                             int                             alignWhat, // 0 -> align current, 1 -> align marked, 2 -> align selected
                             AWTC_get_first_selected_species get_first_selected_species, // used if alignWhat == 2
                             AWTC_get_next_selected_species  get_next_selected_species,
                             int                             turnAllowed, // 0 -> don't mirror; 1 -> ask user; 2 -> mirror and don't ask
                             int                             temporary, // ==1 -> create only temporary aligment report into alignment (2=resident,0=none)
                             int                             showGapsMessages, // ==1 -> display messages about missing gaps in master
                             int                             firstColumn, // first column of range to be aligned (0..len-1)
                             int                             lastColumn, // last column of range to be aligned (0..len-1, -1  = (len-1))
                             int                             maxRelatives, // max # of relatives to use
                             int maxProtection) // protection level
{
    // (reference==NULL && get_consensus==NULL -> use next relative for (each) sequence)

    GB_ERROR error                = GB_push_transaction(gb_main);
    int      search_by_pt_server  = reference==NULL && get_consensus==NULL;
    int      err_count            = 0;
    int      wasNotAllowedToAlign = 0; // incremented for every sequence which has higher protection level (and was not aligned)

    awtc_assert(reference==NULL || get_consensus==NULL);    // can't do both modes

    if (turnAllowed) {
        if ((firstColumn!=0 || lastColumn!=-1) || !search_by_pt_server) {
            // if not selected 'Range/Whole sequence' or not selected 'Reference/Auto search..'
            turnAllowed = 0; // then disable mirroring for the actual call
        }
    }

    if (!error && !alignment) {
        alignment = (GB_CSTR)GBT_get_default_alignment(gb_main); // get default alignment
        if (!alignment) {
            error = GB_export_error("No default alignment");
        }
    }

    if (!error && alignment) {
        global_alignmentType = GBT_get_alignment_type(gb_main, alignment);
        if (search_by_pt_server) {
            GB_alignment_type pt_server_alignmentType = GBT_get_alignment_type(gb_main, pt_server_alignment);

            if (pt_server_alignmentType != GB_AT_RNA &&
                pt_server_alignmentType != GB_AT_DNA) {
                error = GB_export_error("pt_servers only support RNA/DNA sequences.\n"
                                        "In the aligner window you may specify a RNA/DNA alignment \n"
                                        "and use a pt_server build on that alignment.");
            }
        }
    }

    if (!error) {
        GBDATA *gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
        int max_seq_length = GBT_get_alignment_len(gb_main, alignment);

        if (reference) { // align to reference sequence
            GBDATA *gb_reference = GBT_find_species_rel_species_data(gb_species_data, reference);

            if (!gb_reference) {
                error = species_not_found(reference);
            }
            else {
                long                       referenceChksum;
                AWTC_CompactedSubSequence *referenceSeq = readCompactedSequence(gb_reference, alignment, &error, NULL, &referenceChksum, firstColumn, lastColumn);
                if (island_hopper) {
                    GBDATA *gb_seq = GBT_read_sequence(gb_reference, alignment);        // get sequence
                    if (gb_seq) {
                        long        length = GB_read_string_count(gb_seq);
                        const char *data   = GB_read_char_pntr(gb_seq);

                        island_hopper->set_ref_sequence(data);
                        island_hopper->set_alignment_length(length);
                    }
                }


                if (!error) {
                    AWTC_FastSearchSequence referenceFastSeq(*referenceSeq);

                    switch (alignWhat) {
                        case 0: { // align one sequence
                            awtc_assert(toalign);
                            GBDATA *gb_toalign = GBT_find_species_rel_species_data(gb_species_data, toalign);
                            if (!gb_toalign) {
                                error = species_not_found(toalign);
                            }
                            else {
                                actualSequenceNumber = overallSequenceNumber = 1;
                                int myProtection = GB_read_security_write(GBT_read_sequence(gb_toalign, alignment));
                                error = 0;
                                if (myProtection<=maxProtection) {
                                    error = alignTo(gb_toalign, alignment, &referenceFastSeq, gb_reference,
                                                    max_seq_length, temporary,
                                                    firstColumn, lastColumn, showGapsMessages);
                                }
                                else {
                                    wasNotAllowedToAlign++;
                                }
                            }
                            break;
                        }
                        case 1: { // align all marked sequences
                            int count = GBT_count_marked_species(gb_main);
                            int done = 0;
                            GBDATA *gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);

                            actualSequenceNumber = 1;
                            overallSequenceNumber = count;

                            while (gb_species) {
                                int myProtection = GB_read_security_write(GBT_read_sequence(gb_species, alignment));
                                error = 0;
                                if (myProtection<=maxProtection) {
                                    error = alignTo(gb_species, alignment, &referenceFastSeq, gb_reference,
                                                    max_seq_length, temporary,
                                                    firstColumn, lastColumn, showGapsMessages);
                                }
                                else {
                                    wasNotAllowedToAlign++;
                                }

                                if (error) {
                                    aw_message(error);
                                    error = 0;
                                    err_count++;
                                    GB_abort_transaction(gb_main);
                                }
                                else {
                                    GB_pop_transaction(gb_main);
                                }

                                done++;
                                GB_status(double(done)/double(count));

                                GB_push_transaction(gb_main);
                                gb_species = GBT_next_marked_species(gb_species);
                            }
                            break;
                        }
                        case 2: { // align all selected species
                            int count;
                            int done = 0;
                            GBDATA *gb_species = get_first_selected_species(&count);

                            actualSequenceNumber = 1;
                            overallSequenceNumber = count;

                            if (!gb_species) {
                                aw_message("There is no selected species!");
                            }

                            while (gb_species) {
                                int myProtection = GB_read_security_write(GBT_read_sequence(gb_species, alignment));

                                error = 0;
                                if (myProtection<=maxProtection) {
                                    error = alignTo(gb_species, alignment, &referenceFastSeq, gb_reference,
                                                    max_seq_length, temporary,
                                                    firstColumn, lastColumn, showGapsMessages);
                                }
                                else {
                                    wasNotAllowedToAlign++;
                                }

                                if (error) {
                                    aw_message(error);
                                    error = 0;
                                    err_count++;
                                    GB_abort_transaction(gb_main);
                                }
                                else {
                                    GB_pop_transaction(gb_main);
                                }

                                done++;
                                GB_status(double(done)/double(count));

                                GB_push_transaction(gb_main);
                                gb_species = get_next_selected_species();
                            }

                            break;
                        }
                    }

                    delete referenceSeq;
                }
            }
        }
        else if (get_consensus) { // align to group consensi
            switch (alignWhat) {
                case 0: { // align one sequence
                    awtc_assert(toalign);
                    GBDATA *gb_toalign = GBT_find_species_rel_species_data(gb_species_data, toalign);

                    actualSequenceNumber = overallSequenceNumber = 1;

                    if (!gb_toalign) {
                        error = species_not_found(toalign);
                    }
                    else {
                        int myProtection = GB_read_security_write(GBT_read_sequence(gb_toalign, alignment));

                        error = 0;
                        if (myProtection<=maxProtection) {
                            error = alignToGroupConsensus(gb_toalign, alignment,
                                                          get_consensus,
                                                          max_seq_length, temporary,
                                                          firstColumn, lastColumn, showGapsMessages);
                        }
                        else {
                            wasNotAllowedToAlign++;
                        }
                    }
                    break;
                }
                case 1: { // align all marked sequences
                    int count = GBT_count_marked_species(gb_main);
                    int done = 0;
                    GBDATA *gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);

                    actualSequenceNumber = 1;
                    overallSequenceNumber = count;

                    while (gb_species) {
                        int myProtection = GB_read_security_write(GBT_read_sequence(gb_species, alignment));

                        error = 0;
                        if (myProtection<=maxProtection) {
                            error = alignToGroupConsensus(gb_species, alignment,
                                                          get_consensus,
                                                          max_seq_length, temporary,
                                                          firstColumn, lastColumn, showGapsMessages);
                        }
                        else {
                            wasNotAllowedToAlign++;
                        }

                        if (error) {
                            aw_message(error);
                            error = 0;
                            err_count++;
                            GB_abort_transaction(gb_main);
                        }
                        else {
                            GB_pop_transaction(gb_main);
                        }

                        done++;
                        GB_status(double(done)/double(count));

                        GB_push_transaction(gb_main);
                        gb_species = GBT_next_marked_species(gb_species);
                    }
                    break;
                }
                case 2: { // align all selected species
                    int count;
                    int done = 0;
                    GBDATA *gb_species = get_first_selected_species(&count);

                    actualSequenceNumber = 1;
                    overallSequenceNumber = count;

                    if (!gb_species) {
                        aw_message("There is no selected species!");
                    }

                    while (gb_species) {
                        int myProtection = GB_read_security_write(GBT_read_sequence(gb_species, alignment));

                        if (myProtection<=maxProtection) {
                            error = alignToGroupConsensus(gb_species, alignment,
                                                          get_consensus,
                                                          max_seq_length, temporary,
                                                          firstColumn, lastColumn, showGapsMessages);
                        }
                        else {
                            wasNotAllowedToAlign++;
                        }

                        if (error) {
                            aw_message(error);
                            error = 0;
                            err_count++;
                            GB_abort_transaction(gb_main);
                        }
                        else {
                            GB_pop_transaction(gb_main);
                        }

                        done++;
                        GB_status(double(done)/double(count));

                        GB_push_transaction(gb_main);
                        gb_species = get_next_selected_species();
                    }
                    break;
                }
            }
        }
        else { // align to next relative
            switch (alignWhat) {
                case 0: { // align one sequence
                    awtc_assert(toalign);
                    GBDATA *gb_toalign = GBT_find_species_rel_species_data(gb_species_data, toalign);

                    actualSequenceNumber = overallSequenceNumber = 1;

                    if (!gb_toalign) {
                        error = species_not_found(toalign);
                    }
                    else {
                        int myProtection = GB_read_security_write(GBT_read_sequence(gb_toalign, alignment));

                        error = 0;
                        if (myProtection<=maxProtection) {
                            error = alignToNextRelative(pt_server_id, pt_server_alignment, max_seq_length,
                                                        temporary, turnAllowed,
                                                        alignment, gb_toalign,
                                                        firstColumn, lastColumn, maxRelatives, showGapsMessages);
                        }
                        else {
                            wasNotAllowedToAlign++;
                        }
                    }
                    break;
                }
                case 1: { // align all marked sequences
                    int count = GBT_count_marked_species(gb_main);
                    int done = 0;
                    GBDATA *gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);

                    actualSequenceNumber = 1;
                    overallSequenceNumber = count;

                    while (gb_species) {
                        int myProtection = GB_read_security_write(GBT_read_sequence(gb_species, alignment));

                        error = 0;
                        if (myProtection<=maxProtection) {
                            error = alignToNextRelative(pt_server_id, pt_server_alignment, max_seq_length,
                                                        temporary, turnAllowed,
                                                        alignment, gb_species,
                                                        firstColumn, lastColumn, maxRelatives, showGapsMessages);
                        }
                        else {
                            wasNotAllowedToAlign++;
                        }

                        if (error) {
                            aw_message(error);
                            error = 0;
                            err_count++;
                            GB_abort_transaction(gb_main);
                        }
                        else {
                            GB_pop_transaction(gb_main);
                        }

                        done++;
                        GB_status(double(done)/double(count));

                        GB_push_transaction(gb_main);
                        gb_species = GBT_next_marked_species(gb_species);
                    }
                    break;
                }
                case 2: { // align all selected species
                    int count;
                    int done = 0;
                    GBDATA *gb_species = get_first_selected_species(&count);

                    actualSequenceNumber = 1;
                    overallSequenceNumber = count;

                    if (!gb_species) {
                        aw_message("There is no selected species!");
                    }

                    while (gb_species) {
                        int myProtection = GB_read_security_write(GBT_read_sequence(gb_species, alignment));

                        if (myProtection<=maxProtection) {
                            error = alignToNextRelative(pt_server_id, pt_server_alignment, max_seq_length,
                                                        temporary, turnAllowed,
                                                        alignment, gb_species,
                                                        firstColumn, lastColumn, maxRelatives, showGapsMessages);
                        }
                        else {
                            wasNotAllowedToAlign++;
                        }

                        if (error) {
                            aw_message(error);
                            error = 0;
                            err_count++;
                            GB_abort_transaction(gb_main);
                        }
                        else {
                            GB_pop_transaction(gb_main);
                        }

                        done++;
                        GB_status(double(done)/double(count));

                        GB_push_transaction(gb_main);
                        gb_species = get_next_selected_species();
                    }
                    break;
                }
            }
        }
    }

    AWTC_ClustalV_align_exit();
    unaligned_bases.clear();

    if (error) {
        GB_abort_transaction(gb_main);
    }
    else {
        GB_pop_transaction(gb_main);
    }

    if (wasNotAllowedToAlign>0) {
        const char *mess = GB_export_error("%i species were not aligned (because of protection level)", wasNotAllowedToAlign);
        aw_message(mess, "OK");
    }

    if (err_count) {
        if (error) {
            aw_message(error);
        }
        error = GB_export_error("Aligner produced %i error%c", err_count, err_count==1 ? '\0' : 's');
    }

    return error;
}

// --------------------------------------------------------------------------------
//  Parameter-Window for Fast-Aligner
// --------------------------------------------------------------------------------

void AWTC_start_faligning(AW_window *aw, AW_CL cd2)
{
    AW_root                        *root          = aw->get_root();
    char                           *reference     = NULL; // align against next relatives
    char                           *toalign       = NULL; // align marked species
    GB_ERROR                        error         = NULL;
    static struct AWTC_faligner_cd *cd            = (struct AWTC_faligner_cd *)cd2;
    int                             get_consensus = 0;
    int                             pt_server_id  = -1;

    //  awtc_assert(cd->helix_string != 0);

    AWTC_get_first_selected_species get_first_selected_species = 0;
    AWTC_get_next_selected_species  get_next_selected_species  = 0;
    int                             alignWhat;

    awtc_assert(island_hopper == 0);
    if (root->awar(FA_AWAR_USE_ISLAND_HOPPING)->read_int()) {
        island_hopper = new IslandHopping();
        if (root->awar(FA_AWAR_USE_SECONDARY)->read_int()) island_hopper->set_helix(cd->helix_string);

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

    switch (alignWhat=root->awar(FA_AWAR_TO_ALIGN)->read_int()) {
        case 0: { // align current species
            toalign = root->awar(AWAR_SPECIES_NAME)->read_string();
            break;
        }
        case 1: { // align marked species
            break;
        }
        case 2: { // align selected species
            get_first_selected_species = cd->get_first_selected_species;
            get_next_selected_species = cd->get_next_selected_species;
            break;
        }
        default: {
            awtc_assert(0);
            break;
        }
    }

    switch (root->awar(FA_AWAR_REFERENCE)->read_int())
    {
        case 0: // align against specified species

            reference = root->awar(FA_AWAR_REFERENCE_NAME)->read_string();
            break;

        case 1: // align against group consensus

            if (cd->get_group_consensus) {
                get_consensus = 1;
            }
            else {
                error = "Can't get group consensus here.";
            }
            break;

        case 2: // align against species searched via pt_server

            pt_server_id = root->awar(AWAR_PT_SERVER)->read_int();
            if (pt_server_id<0) {
                error = "No pt_server selected";
            }
            break;

        default: awtc_assert(0);
            break;
    }

    int firstColumn = 0;
    int lastColumn = -1;

    switch (root->awar(FA_AWAR_RANGE)->read_int())
    {
        case 0:
            break;
        case 1: {
            int curpos = root->awar(AWAR_CURSOR_POSITION_LOCAL)->read_int();
            int size = root->awar(FA_AWAR_AROUND)->read_int();

            if ((curpos-size)>0) firstColumn = curpos-size;
            lastColumn = curpos+size;
            break;
        }
        case 2: {
            if (!cd->get_selected_range(&firstColumn, &lastColumn)) {
                error = "There is no selected species!";
            }
#ifdef DEBUG
            else {
                printf("Selected range: %i .. %i\n", firstColumn, lastColumn);
            }
#endif
            break;
        }
        default: { awtc_assert(0); break; }
    }

    if (!error) {
        char *editor_alignment = 0;
        {
            GB_transaction  dummy(gb_main);
            char           *default_alignment = GBT_get_default_alignment(gb_main);

            editor_alignment = root->awar_string(AWAR_EDITOR_ALIGNMENT, default_alignment)->read_string();
            free(default_alignment);
        }
        char *pt_server_alignment = root->awar(FA_AWAR_PT_SERVER_ALIGNMENT)->read_string();

        if (island_hopper) {
            island_hopper->set_range(firstColumn, lastColumn);
            firstColumn = 0;
            lastColumn  = -1;
        }

        aw_openstatus("FastAligner");
        error = AWTC_aligner(reference, get_consensus ? cd->get_group_consensus : NULL,
                             pt_server_id, pt_server_alignment,
                             editor_alignment,
                             toalign, alignWhat, get_first_selected_species, get_next_selected_species,
                             root->awar(FA_AWAR_MIRROR)->read_int(),
                             root->awar(FA_AWAR_INSERT)->read_int(),
                             root->awar(FA_AWAR_SHOW_GAPS_MESSAGES)->read_int(),
                             firstColumn, lastColumn,
                             root->awar(FA_AWAR_NEXT_RELATIVES)->read_int(),
                             root->awar(FA_AWAR_PROTECTION)->read_int());
        aw_closestatus();

        free(pt_server_alignment);
        free(editor_alignment);
    }

    if (island_hopper) {
        delete island_hopper;
        island_hopper = 0;
    }

    if (toalign) free(toalign);
    if (error) aw_message(error);
    if (cd->do_refresh) cd->refresh_display();
}



void AWTC_create_faligner_variables(AW_root *root,AW_default db1)
{
    root->awar_int(     FA_AWAR_TO_ALIGN,           0,      db1);
    root->awar_int(     FA_AWAR_REFERENCE,          1,      db1);
    root->awar_string(  FA_AWAR_REFERENCE_NAME,         "My name is nobody!",       db1);
    root->awar_int( FA_AWAR_RANGE,              0,  db1);
#if defined(DEVEL_RALF)
    root->awar_int(     FA_AWAR_PROTECTION,         0,  db1)->write_int(6);
#else
    root->awar_int(     FA_AWAR_PROTECTION,         0,  db1)->write_int(0);
#endif
    root->awar_int( FA_AWAR_AROUND,             25,     db1);
    root->awar_int( FA_AWAR_MIRROR,             1,  db1);
    root->awar_int( FA_AWAR_INSERT,             0,  db1);
    root->awar_int( FA_AWAR_SHOW_GAPS_MESSAGES,     1,  db1);
    root->awar_int( FA_AWAR_USE_SECONDARY,      0,  db1);
    root->awar_int( AWAR_PT_SERVER,             0,  db1);
    root->awar_int( FA_AWAR_NEXT_RELATIVES,         1,  db1)->set_minmax(1,100);

    root->awar_string( FA_AWAR_PT_SERVER_ALIGNMENT, root->awar(AWAR_DEFAULT_ALIGNMENT)->read_string(),  db1);

    // island hopping:

    root->awar_int( FA_AWAR_USE_ISLAND_HOPPING,     0,  db1);

    root->awar_int( FA_AWAR_ESTIMATE_BASE_FREQ,     1,      db1);

    root->awar_float( FA_AWAR_BASE_FREQ_A,      0.25,   db1);
    root->awar_float( FA_AWAR_BASE_FREQ_C,      0.25,   db1);
    root->awar_float( FA_AWAR_BASE_FREQ_G,      0.25,   db1);
    root->awar_float( FA_AWAR_BASE_FREQ_T,      0.25,   db1);

    root->awar_float(FA_AWAR_SUBST_PARA_AC,     1.0,    db1);
    root->awar_float(FA_AWAR_SUBST_PARA_AG,     4.0,    db1);
    root->awar_float(FA_AWAR_SUBST_PARA_AT,     1.0,    db1);
    root->awar_float(FA_AWAR_SUBST_PARA_CG,     1.0,    db1);
    root->awar_float(FA_AWAR_SUBST_PARA_CT,     4.0,    db1);
    root->awar_float(FA_AWAR_SUBST_PARA_GT,     1.0,    db1);

    root->awar_float(FA_AWAR_EXPECTED_DISTANCE,     0.3,    db1);
    root->awar_float(FA_AWAR_STRUCTURE_SUPPLEMENT,  0.5,    db1);
    root->awar_float(FA_AWAR_THRESHOLD,             0.005,      db1);

    root->awar_float(FA_AWAR_GAP_A,     8.0,    db1);
    root->awar_float(FA_AWAR_GAP_B,     4.0,    db1);
    root->awar_float(FA_AWAR_GAP_C,     7.0,    db1);
}

void AWTC_awar_set_current_sequence(AW_root *root, AW_default db1)
{
    root->awar_int(FA_AWAR_TO_ALIGN, 0, db1);
}

// sets the aligner reference species to current species
void AWTC_set_reference_species_name(AW_window */*aww*/, AW_CL cl_AW_root)
{
    AW_root *root     = (AW_root*)cl_AW_root;
    char    *specName = root->awar(AWAR_SPECIES_NAME)->read_string();

    root->awar(FA_AWAR_REFERENCE_NAME)->write_string(specName);
    free(specName);
}

AW_window *AWTC_create_island_hopping_window(AW_root *root, AW_CL ) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init( root, "ISLAND_HOPPING_PARA", "Parameters for Island Hopping");
    aws->load_xfig("awtc/islandhopping.fig");

    aws->at( "close" );
    aws->callback     ( (AW_CB0)AW_POPDOWN  );
    aws->create_button( "CLOSE", "CLOSE", "O" );

    aws->at( "help" );
    aws->callback     ( AW_POPUP_HELP, (AW_CL) "islandhopping.hlp"  );
    aws->create_button( "HELP", "HELP" );

    aws->at("use_secondary");
    aws->label("Use secondary structure (only for re-align)");
    aws->create_toggle(FA_AWAR_USE_SECONDARY);

    aws->at("freq");
    aws->create_toggle_field(FA_AWAR_ESTIMATE_BASE_FREQ,"Base freq.","B");
    aws->insert_default_toggle("Estimate","E",1);
    aws->insert_toggle("Define here: ","D",0);
    aws->update_toggle_field();

    aws->at("freq_a"); aws->label("A:"); aws->create_input_field(FA_AWAR_BASE_FREQ_A, 4);
    aws->at("freq_c"); aws->label("C:"); aws->create_input_field(FA_AWAR_BASE_FREQ_C, 4);
    aws->at("freq_g"); aws->label("G:"); aws->create_input_field(FA_AWAR_BASE_FREQ_G, 4);
    aws->at("freq_t"); aws->label("T:"); aws->create_input_field(FA_AWAR_BASE_FREQ_T, 4);

    int xpos[4], ypos[4];
    {
        aws->button_length(1);

        int dummy;
        aws->at("h_a"); aws->get_at_position(&xpos[0], &dummy); aws->create_button("A","A");
        aws->at("h_c"); aws->get_at_position(&xpos[1], &dummy); aws->create_button("C","C");
        aws->at("h_g"); aws->get_at_position(&xpos[2], &dummy); aws->create_button("G","G");
        aws->at("h_t"); aws->get_at_position(&xpos[3], &dummy); aws->create_button("T","T");

        aws->at("v_a"); aws->get_at_position(&dummy, &ypos[0] ); aws->create_button("A","A");
        aws->at("v_c"); aws->get_at_position(&dummy, &ypos[1] ); aws->create_button("C","C");
        aws->at("v_g"); aws->get_at_position(&dummy, &ypos[2] ); aws->create_button("G","G");
        aws->at("v_t"); aws->get_at_position(&dummy, &ypos[3] ); aws->create_button("T","T");
    }

    aws->at("subst"); aws->create_button("subst_para", "Substitution rate parameters:");

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

    return (AW_window *)aws;
}

AW_window *AWTC_create_faligner_window(AW_root *root, AW_CL cd2)
{
    AW_window_simple *aws = new AW_window_simple;

    aws->init( root, "INTEGRATED_ALIGNERS", INTEGRATED_ALIGNERS_TITLE);
    aws->load_xfig("awtc/faligner.fig");

    aws->label_length( 10 );
    aws->button_length( 10 );

    aws->at( "close" );
    aws->callback     ( (AW_CB0)AW_POPDOWN  );
    aws->create_button( "CLOSE", "CLOSE", "O" );

    aws->at( "help" );
    aws->callback     ( AW_POPUP_HELP, (AW_CL) "faligner.hlp"  );
    aws->create_button( "HELP", "HELP" );

    aws->at("aligner");
    aws->create_toggle_field(FA_AWAR_USE_ISLAND_HOPPING,"Aligner","A");
    aws->insert_default_toggle("Fast aligner","F",0);
    aws->insert_toggle("Island Hopping","I",1);
    aws->update_toggle_field();

    aws->button_length(12);
    aws->at("island_para");
    aws->callback(AW_POPUP, (AW_CL)AWTC_create_island_hopping_window, (AW_CL)0);
    aws->create_button("island_para", "Parameters", "");

    aws->button_length(10);

    aws->at( "rev_compl" );
    aws->callback(AWTC_build_reverse_complement, cd2);
    aws->create_button( "reverse_complement", "Turn now!", "");

    aws->at("what");
    aws->create_toggle_field(FA_AWAR_TO_ALIGN,"Align what?","A");
    aws->insert_toggle("Current Species:","A",0);
    aws->insert_default_toggle("Marked Species", "M",1);
    aws->insert_toggle("Selected Species","S",2);
    aws->update_toggle_field();

    aws->at( "swhat" );
    aws->create_input_field( AWAR_SPECIES_NAME, 2);

    aws->at("against");
    aws->create_toggle_field(FA_AWAR_REFERENCE,"Reference","");
    aws->insert_toggle("Species by name:","S",0);
    aws->insert_toggle("Group consensus","K",1);
    aws->insert_default_toggle("Auto search by pt_server:","A",2);
    aws->update_toggle_field();

    aws->at( "sagainst" );
    aws->create_input_field(FA_AWAR_REFERENCE_NAME, 2);

    aws->at("copy");
    aws->callback(AWTC_set_reference_species_name, (AW_CL)root);
    aws->create_button("Copy", "Copy", "");

    aws->label_length(0);
    aws->at( "pt_server" );
    awt_create_selection_list_on_pt_servers(aws, AWAR_PT_SERVER, AW_TRUE);

    aws->at("use_ali");
    aws->create_input_field(FA_AWAR_PT_SERVER_ALIGNMENT, 15);

    aws->at("relatives");
    aws->label("Number of relatives to use:");
    aws->create_input_field(FA_AWAR_NEXT_RELATIVES, 3);

    // Range

    aws->label_length( 10 );
    aws->at("range");
    aws->create_toggle_field(FA_AWAR_RANGE, "Range", "");
    aws->insert_default_toggle("Whole sequence", "", 0);
    aws->insert_toggle("Positions around cursor: ", "", 1);
    aws->insert_toggle("Selected Range", "", 2);
    aws->update_toggle_field();

    aws->at("around");
    aws->create_input_field(FA_AWAR_AROUND, 2);

    aws->at("protection");
    aws->create_option_menu(FA_AWAR_PROTECTION, "Protection", "");
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
    aws->create_option_menu(FA_AWAR_MIRROR, "Turn check", "");
    aws->insert_option("Never turn sequence", "", 0);
    aws->insert_default_option("User acknowledgement", "", 1);
    aws->insert_option("Automatically turn sequence", "", 2);
    aws->update_option_menu();

    // Report

    aws->at("insert");
    aws->create_option_menu(FA_AWAR_INSERT, "Report", "");
    aws->insert_option("No report", "", 0);
    aws->insert_default_option("Report to temporary entries", "", 1);
    aws->insert_option("Report to resident entries", "", 2);
    aws->update_option_menu();

    aws->at("gaps");
    aws->create_toggle(FA_AWAR_SHOW_GAPS_MESSAGES);

    aws->at( "align" );
    aws->callback     ( AWTC_start_faligning, cd2);
    aws->highlight();
    aws->create_button( "GO", "GO", "G");

    return (AW_window *)aws;
}



