// ================================================================= //
//                                                                   //
//   File      : chartable.h                                         //
//   Purpose   : count characters of multiple sequences              //
//                                                                   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef CHARTABLE_H
#define CHARTABLE_H

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef POS_RANGE_H
#include <pos_range.h>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef _STDINT_H
#include <stdint.h>
#endif

#define ct_assert(bed) arb_assert(bed)

#ifdef DEBUG
// # define TEST_BASES_TABLE
#endif

#if defined(DEBUG) && !defined(DEVEL_RELEASE)
# define TEST_CHAR_TABLE_INTEGRITY
#endif

#define MAXCHARTABLE          256
#define SHORT_TABLE_ELEM_SIZE 1
#define SHORT_TABLE_MAX_VALUE 0xff
#define LONG_TABLE_ELEM_SIZE  4

#define MAX_INDEX_TABLES    128     // max number of different non-ambigious indices (into BaseFrequencies::bases_table)
#define MAX_TARGET_INDICES  4       // max number of non-ambigious indices affected by one ambigious code (4 for 'N' in RNA/DNA, 2 for amino-acids)
#define MAX_AMBIGUITY_CODES (1+4+6) // for RNA/DNA (3 for amino-acids)

// Note: short tables convert to long tables when adding 22 sequences (for nucleotides)
// * maybe remove short tables completely OR
// * increase values size in short tables to 16 bit

namespace chartable {

    struct Ambiguity {
        /*! define how a specific ambiguity code is counted
         */

        uint8_t indices;                   // number of entries in bases_table affected by this ambiguity
        uint8_t increment;                 // how much each entry gets incremented
        uint8_t index[MAX_TARGET_INDICES]; // into BaseFrequencies::bases_table
    };

    class SepBaseFreq : virtual Noncopyable {
        /*! counts occurances of one specific base (e.g. 'A') or a group of bases,
         *  separately for each alignment position of multiple input sequences.
         */
        int table_entry_size;       // how many bytes are used for each element of 'no_of_bases' (1 or 4 bytes)
        union {
            unsigned char *shortTable;
            int           *longTable;
        } no_of_bases;      // counts bases for each sequence position
        int no_of_entries;      // length of bases table

        int legal(int offset) const { return offset>=0 && offset<no_of_entries; }

        void set_elem_long(int offset, int value) {
#ifdef TEST_BASES_TABLE
            ct_assert(legal(offset));
            ct_assert(table_entry_size==LONG_TABLE_ELEM_SIZE);
#endif
            no_of_bases.longTable[offset] = value;
        }

        void set_elem_short(int offset, int value) {
#ifdef TEST_BASES_TABLE
            ct_assert(legal(offset));
            ct_assert(table_entry_size==SHORT_TABLE_ELEM_SIZE);
            ct_assert(value>=0 && value<=SHORT_TABLE_MAX_VALUE);
#endif
            no_of_bases.shortTable[offset] = value;
        }

        int get_elem_long(int offset) const {
#ifdef TEST_BASES_TABLE
            ct_assert(legal(offset));
            ct_assert(table_entry_size==LONG_TABLE_ELEM_SIZE);
#endif
            return no_of_bases.longTable[offset];
        }

        int get_elem_short(int offset) const {
#ifdef TEST_BASES_TABLE
            ct_assert(legal(offset));
            ct_assert(table_entry_size==SHORT_TABLE_ELEM_SIZE);
#endif
            return no_of_bases.shortTable[offset];
        }

    public:

        SepBaseFreq(int maxseqlength);
        ~SepBaseFreq();

        void init(int length);
        int size() const { return no_of_entries; }

        int get_table_entry_size() const { return table_entry_size; }
        void expand_table_entry_size();
        bool bigger_table_entry_size_needed(int new_max_count) { return table_entry_size==SHORT_TABLE_ELEM_SIZE ? (new_max_count>SHORT_TABLE_MAX_VALUE) : 0; }

        int operator[](int offset) const { return table_entry_size==SHORT_TABLE_ELEM_SIZE ? get_elem_short(offset) : get_elem_long(offset); }

        void inc_short(int offset, int incr)  {
            int old = get_elem_short(offset);
            ct_assert((old+incr)<=255);
            set_elem_short(offset, old+incr);
        }
        void dec_short(int offset, int decr)  {
            int old = get_elem_short(offset);
            ct_assert(old>=decr);
            set_elem_short(offset, old-decr);
        }
        void inc_long(int offset, int incr)   {
            int old = get_elem_long(offset);
            set_elem_long(offset, old+incr);
        }
        void dec_long(int offset, int decr)   {
            int old = get_elem_long(offset);
            ct_assert(old>=decr);
            set_elem_long(offset, old-decr);
        }

        int firstDifference(const SepBaseFreq& other, int start, int end, int *firstDifferentPos) const;
        int lastDifference(const SepBaseFreq& other, int start, int end, int *lastDifferentPos) const;

        void add(const SepBaseFreq& other, int start, int end);
        void sub(const SepBaseFreq& other, int start, int end);
        void sub_and_add(const SepBaseFreq& Sub, const SepBaseFreq& Add, PosRange range);

        void change_table_length(int new_length, int default_entry);


#if defined(TEST_CHAR_TABLE_INTEGRITY) || defined(ASSERTION_USED)
        int empty() const;
#endif // TEST_CHAR_TABLE_INTEGRITY
    };

};

class ConsensusBuildParams;

class BaseFrequencies : virtual Noncopyable {
    /*! counts occurances of ALL bases of multiple sequences separately
     *  for all alignment positions.
     *
     *  Bases are clustered in groups of bases (alignment type dependent).
     */

    typedef chartable::SepBaseFreq  SepBaseFreq;
    typedef chartable::Ambiguity    Ambiguity;
    typedef SepBaseFreq            *SepBaseFreqPtr;

    SepBaseFreqPtr *bases_table;

    int  sequenceUnits; // # of sequences added to the table (multiplied with 'units')
    bool ignore;        // this table will be ignored when calculating tables higher in hierarchy
                        // (used in EDIT4 to suppress SAIs in tables of ED4_root_group_manager)

    // @@@ move statics into own class:
    static bool               initialized;
    static Ambiguity          ambiguity_table[MAX_AMBIGUITY_CODES];
    static uint8_t            unitsPerSequence;                // multiplier per added sequence (to allow proper distribution of ambiguity codes)
    static unsigned char      char_to_index_tab[MAXCHARTABLE]; // <MAX_INDEX_TABLES = > real index into 'bases_table', else index+MAX_INDEX_TABLES into ambiguity_table
    static bool               is_gap[MAXCHARTABLE];
    static unsigned char     *upper_index_chars;
    static int                used_bases_tables;               // size of 'bases_table'
    static GB_alignment_type  ali_type;

    static inline void set_char_to_index(unsigned char c, int index);

    void add(const BaseFrequencies& other, int start, int end);
    void sub(const BaseFrequencies& other, int start, int end);

    void expand_tables();
    int get_table_entry_size() const {
        return linear_table(0).get_table_entry_size();
    }
    void prepare_to_add_elements(int new_sequences) {
        ct_assert(used_bases_tables);
        if (linear_table(0).bigger_table_entry_size_needed(sequenceUnits+new_sequences*unitsPerSequence)) {
            expand_tables();
        }
    }

    // linear access to all tables
    SepBaseFreq&       linear_table(int c)         { ct_assert(c<used_bases_tables); return *bases_table[c]; }
    const SepBaseFreq& linear_table(int c) const   { ct_assert(c<used_bases_tables); return *bases_table[c]; }

    // access via character
    SepBaseFreq&       table(int c)        { ct_assert(c>0 && c<MAXCHARTABLE); return linear_table(char_to_index_tab[c]); }
    const SepBaseFreq& table(int c) const  { ct_assert(c>0 && c<MAXCHARTABLE); return linear_table(char_to_index_tab[c]); }

    static unsigned char index_to_upperChar(int index) { return upper_index_chars[index]; }

    void incr_short(unsigned char c, int offset);
    void decr_short(unsigned char c, int offset);
    void incr_long(unsigned char c, int offset);
    void decr_long(unsigned char c, int offset);

#if defined(TEST_CHAR_TABLE_INTEGRITY)
    void test() const; // test if table is valid (dumps core if invalid)
#else
    void test() const {}
#endif

public:

#if defined(TEST_CHAR_TABLE_INTEGRITY) || defined(ASSERTION_USED)
    bool ok() const;
    bool empty() const;
#endif

    BaseFrequencies(int maxseqlength=0);
    ~BaseFrequencies();

    static void setup(const char *gap_chars, GB_alignment_type ali_type_);

    void ignore_me() { ignore = 1; }
    int is_ignored() const { return ignore; }

    void init(int maxseqlength);
    int size() const { return bases_table[0]->size(); }
    int added_sequences() const { return sequenceUnits/unitsPerSequence; }

    void bases_and_gaps_at(int column, int *bases, int *gaps) const;
    double max_frequency_at(int column, bool ignore_gaps) const;

    static bool isGap(char c) { return is_gap[safeCharIndex(c)]; }

    const PosRange *changed_range(const BaseFrequencies& other) const;
    static const PosRange *changed_range(const char *string1, const char *string2, int min_len);

    void add(const BaseFrequencies& other);
    void sub(const BaseFrequencies& other);
    void sub_and_add(const BaseFrequencies& Sub, const BaseFrequencies& Add, PosRange range);

    void add(const char *string, int len);
    void sub(const char *string, int len);
    void sub_and_add(const char *old_string, const char *new_string, PosRange range);

    void build_consensus_string_to(char *consensus_string, ExplicitRange range, const ConsensusBuildParams& BK) const;
    char *build_consensus_string(PosRange r, const ConsensusBuildParams& cbp) const;
    char *build_consensus_string(const ConsensusBuildParams& cbp) const { return build_consensus_string(PosRange::whole(), cbp); }

    void change_table_length(int new_length);
};

#else
#error chartable.h included twice
#endif // CHARTABLE_H
