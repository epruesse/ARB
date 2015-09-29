// ================================================================= //
//                                                                   //
//   File      : chartable.cxx                                       //
//   Purpose   : count characters of multiple sequences              //
//                                                                   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "chartable.h"
#include <arb_msg.h>
#include <iupac.h>
#include <cctype>
#include <arb_defs.h>

#define CONSENSUS_AWAR_SOURCE CAS_INTERNAL // not awar-constructable here
#include <consensus.h>

using namespace chartable;

// ------------------------
//      SepBaseFreq

#ifdef DEBUG
// # define COUNT_BASES_TABLE_SIZE
#endif

#ifdef COUNT_BASES_TABLE_SIZE
static long bases_table_size = 0L;
#endif

void SepBaseFreq::init(int length)
{
    if (length) {
        if (no_of_bases.shortTable) {
            delete no_of_bases.shortTable;
            no_of_bases.shortTable = 0;
#ifdef COUNT_BASES_TABLE_SIZE
            bases_table_size -= (no_of_entries+1)*table_entry_size;
#endif
        }

        no_of_entries = length;
        int new_size = (no_of_entries+1)*table_entry_size;
        no_of_bases.shortTable = (unsigned char*)new char[new_size];
        memset(no_of_bases.shortTable, 0, new_size);
#ifdef COUNT_BASES_TABLE_SIZE
        bases_table_size += new_size;
        printf("bases_table_size = %li\n", bases_table_size);
#endif
    }
}

void SepBaseFreq::expand_table_entry_size() { // converts short table into long table
    ct_assert(table_entry_size==SHORT_TABLE_ELEM_SIZE);

    int *new_table = new int[no_of_entries+1];
    int i;

    for (i=0; i<=no_of_entries; i++) {
        new_table[i] = no_of_bases.shortTable[i];
    }
    delete [] no_of_bases.shortTable;

#ifdef COUNT_BASES_TABLE_SIZE
    bases_table_size += (no_of_entries+1)*(LONG_TABLE_ELEM_SIZE-SHORT_TABLE_ELEM_SIZE);
    printf("bases_table_size = %li\n", bases_table_size);
#endif

    no_of_bases.longTable = new_table;
    table_entry_size = LONG_TABLE_ELEM_SIZE;
}

#define INVALID_TABLE_OPERATION() GBK_terminatef("SepBaseFreq: invalid operation at %i", __LINE__)

void SepBaseFreq::add(const SepBaseFreq& other, int start, int end)
{
    ct_assert(no_of_entries==other.no_of_entries);
    ct_assert(start>=0);
    ct_assert(end<no_of_entries);
    ct_assert(start<=end);

    int i;
    if (table_entry_size==SHORT_TABLE_ELEM_SIZE) {
        if (other.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            for (i=start; i<=end; i++) {
                set_elem_short(i, get_elem_short(i)+other.get_elem_short(i));
            }
        }
        else {
            INVALID_TABLE_OPERATION(); // cannot add long to short
        }
    }
    else {
        if (other.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            for (i=start; i<=end; i++) { // LOOP_VECTORIZED
                set_elem_long(i, get_elem_long(i)+other.get_elem_short(i));
            }
        }
        else {
            for (i=start; i<=end; i++) { // LOOP_VECTORIZED
                set_elem_long(i, get_elem_long(i)+other.get_elem_long(i));
            }
        }
    }
}
void SepBaseFreq::sub(const SepBaseFreq& other, int start, int end)
{
    ct_assert(no_of_entries==other.no_of_entries);
    ct_assert(start>=0);
    ct_assert(end<no_of_entries);
    ct_assert(start<=end);

    int i;
    if (table_entry_size==SHORT_TABLE_ELEM_SIZE) {
        if (other.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            for (i=start; i<=end; i++) {
                set_elem_short(i, get_elem_short(i)-other.get_elem_short(i));
            }
        }
        else {
            INVALID_TABLE_OPERATION(); // cannot sub long from short
        }
    }
    else {
        if (other.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            for (i=start; i<=end; i++) { // LOOP_VECTORIZED
                set_elem_long(i, get_elem_long(i)-other.get_elem_short(i));
            }
        }
        else {
            for (i=start; i<=end; i++) { // LOOP_VECTORIZED
                set_elem_long(i, get_elem_long(i)-other.get_elem_long(i));
            }
        }
    }
}
void SepBaseFreq::sub_and_add(const SepBaseFreq& Sub, const SepBaseFreq& Add, PosRange range)
{
    ct_assert(no_of_entries==Sub.no_of_entries);
    ct_assert(no_of_entries==Add.no_of_entries);

    int start = range.start();
    int end   = range.end();

    ct_assert(start>=0);
    ct_assert(end<no_of_entries);
    ct_assert(start<=end);

    int i;
    if (table_entry_size==SHORT_TABLE_ELEM_SIZE) {
        if (Sub.table_entry_size==SHORT_TABLE_ELEM_SIZE &&
            Add.table_entry_size==SHORT_TABLE_ELEM_SIZE)
        {
            for (i=start; i<=end; i++) {
                set_elem_short(i, get_elem_short(i)-Sub.get_elem_short(i)+Add.get_elem_short(i));
            }
        }
        else {
            INVALID_TABLE_OPERATION(); // cannot add or sub long to/from short
        }
    }
    else {
        if (Sub.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            if (Add.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
                for (i=start; i<=end; i++) { // LOOP_VECTORIZED
                    set_elem_long(i, get_elem_long(i)-Sub.get_elem_short(i)+Add.get_elem_short(i));
                }
            }
            else {
                for (i=start; i<=end; i++) { // LOOP_VECTORIZED
                    set_elem_long(i, get_elem_long(i)-Sub.get_elem_short(i)+Add.get_elem_long(i));
                }
            }
        }
        else {
            if (Add.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
                for (i=start; i<=end; i++) { // LOOP_VECTORIZED
                    set_elem_long(i, get_elem_long(i)-Sub.get_elem_long(i)+Add.get_elem_short(i));
                }
            }
            else {
                for (i=start; i<=end; i++) { // LOOP_VECTORIZED
                    set_elem_long(i, get_elem_long(i)-Sub.get_elem_long(i)+Add.get_elem_long(i));
                }
            }
        }
    }
}

int SepBaseFreq::firstDifference(const SepBaseFreq& other, int start, int end, int *firstDifferentPos) const
{
    int i;
    int result = 0;

    if (table_entry_size==SHORT_TABLE_ELEM_SIZE) {
        if (other.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            for (i=start; i<=end; i++) {
                if (get_elem_short(i)!=other.get_elem_short(i)) {
                    *firstDifferentPos = i;
                    result = 1;
                    break;
                }
            }
        }
        else {
            for (i=start; i<=end; i++) {
                if (get_elem_short(i)!=other.get_elem_long(i)) {
                    *firstDifferentPos = i;
                    result = 1;
                    break;
                }
            }
        }
    }
    else {
        if (other.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            for (i=start; i<=end; i++) {
                if (get_elem_long(i)!=other.get_elem_short(i)) {
                    *firstDifferentPos = i;
                    result = 1;
                    break;
                }
            }
        }
        else {
            for (i=start; i<=end; i++) {
                if (get_elem_long(i)!=other.get_elem_long(i)) {
                    *firstDifferentPos = i;
                    result = 1;
                    break;
                }
            }
        }
    }

    return result;
}
int SepBaseFreq::lastDifference(const SepBaseFreq& other, int start, int end, int *lastDifferentPos) const
{
    int i;
    int result = 0;

    if (table_entry_size==SHORT_TABLE_ELEM_SIZE) {
        if (other.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            for (i=end; i>=start; i--) {
                if (get_elem_short(i)!=other.get_elem_short(i)) {
                    *lastDifferentPos = i;
                    result = 1;
                    break;
                }
            }
        }
        else {
            for (i=end; i>=start; i--) {
                if (get_elem_short(i)!=other.get_elem_long(i)) {
                    *lastDifferentPos = i;
                    result = 1;
                    break;
                }
            }
        }
    }
    else {
        if (other.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            for (i=end; i>=start; i--) {
                if (get_elem_long(i)!=other.get_elem_short(i)) {
                    *lastDifferentPos = i;
                    result = 1;
                    break;
                }
            }
        }
        else {
            for (i=end; i>=start; i--) {
                if (get_elem_long(i)!=other.get_elem_long(i)) {
                    *lastDifferentPos = i;
                    result = 1;
                    break;
                }
            }
        }
    }

    return result;
}


SepBaseFreq::SepBaseFreq(int maxseqlength)
    : table_entry_size(SHORT_TABLE_ELEM_SIZE),
      no_of_entries(0)
{
    no_of_bases.shortTable = NULL;
    if (maxseqlength) init(maxseqlength);
}

SepBaseFreq::~SepBaseFreq()
{
    delete [] no_of_bases.shortTable;
#ifdef COUNT_BASES_TABLE_SIZE
    bases_table_size -= (no_of_entries+1)*table_entry_size;
    printf("bases_table_size = %li\n", bases_table_size);
#endif
}

void SepBaseFreq::change_table_length(int new_length, int default_entry)
{
    if (new_length!=no_of_entries) {
        int min_length = new_length<no_of_entries ? new_length : no_of_entries;
        int growth = new_length-no_of_entries;

        if (table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            unsigned char *new_table = new unsigned char[new_length+1];

            ct_assert(default_entry>=0 && default_entry<256);

            memcpy(new_table, no_of_bases.shortTable, min_length*sizeof(*new_table));
            new_table[new_length] = no_of_bases.shortTable[no_of_entries];
            if (growth>0) {
                for (int e=no_of_entries; e<new_length; ++e) new_table[e] = default_entry;
            }

            delete [] no_of_bases.shortTable;
            no_of_bases.shortTable = new_table;
        }
        else {
            int *new_table = new int[new_length+1];

            memcpy(new_table, no_of_bases.longTable, min_length*sizeof(*new_table));
            new_table[new_length] = no_of_bases.longTable[no_of_entries];
            if (growth>0) {
                for (int e=no_of_entries; e<new_length; ++e) { // LOOP_VECTORIZED
                    new_table[e] = default_entry;
                }
            }

            delete [] no_of_bases.longTable;
            no_of_bases.longTable = new_table;
        }

#ifdef COUNT_BASES_TABLE_SIZE
        bases_table_size += growth*table_entry_size;
        printf("bases_table_size = %li\n", bases_table_size);
#endif
        no_of_entries = new_length;
    }
}

#if defined(TEST_CHAR_TABLE_INTEGRITY) || defined(ASSERTION_USED)

int SepBaseFreq::empty() const
{
    int i;

    if (table_entry_size==SHORT_TABLE_ELEM_SIZE) {
        for (i=0; i<no_of_entries; i++) {
            if (get_elem_short(i)) {
                return 0;
            }
        }
    }
    else {
        for (i=0; i<no_of_entries; i++) {
            if (get_elem_long(i)) {
                return 0;
            }
        }
    }

    return 1;
}
#endif // ASSERTION_USED

char *BaseFrequencies::build_consensus_string(PosRange r, const ConsensusBuildParams& cbp) const {
    ExplicitRange  range(r, size());
    long           entries = range.size();
    char          *new_buf = (char*)malloc(entries+1);

    build_consensus_string_to(new_buf, range, cbp);
    new_buf[entries] = 0;

    return new_buf;
}

#if defined(DEBUG)
 #if defined(DEVEL_RALF)
  #define DEBUG_CONSENSUS
 #endif
#endif

void BaseFrequencies::build_consensus_string_to(char *consensus_string, ExplicitRange range, const ConsensusBuildParams& BK) const {
    // 'consensus_string' has to be a buffer of size 'range.size()+1'
    // Note : Always check that consensus behavior is identical to that used in CON_evaluatestatistic()

    ct_assert(consensus_string);
    ct_assert(range.end()<size());

#define PERCENT(part, all)      ((100*(part))/(all))
#define MAX_BASES_TABLES 41     // 25

    ct_assert(used_bases_tables<=MAX_BASES_TABLES);     // this is correct for DNA/RNA -> build_consensus_string() must be corrected for AMI/PRO

    const int left_idx  = range.start();
    const int right_idx = range.end();

#if defined(DEBUG_CONSENSUS)
 #define DUMPINT(var) do { if (dumpcol) fprintf(stderr, "%s=%i ", #var, var); } while(0)
#else
 #define DUMPINT(var)
#endif

    if (sequenceUnits) {
        for (int i=left_idx; i<=right_idx; i++) {
#if defined(DEBUG_CONSENSUS)
            int  column  = i+1;
            bool dumpcol = (column == 21);
            DUMPINT(column);
#endif

            const int o = i-left_idx; // output offset

            int bases        = 0;       // count of all bases together
            int base[MAX_BASES_TABLES]; // count of specific base
            int max_base     = -1;      // maximum count of specific base
            int max_base_idx = -1;      // index of this base

            for (int j=0; j<used_bases_tables; j++) {
                base[j] = linear_table(j)[i];
                if (!isGap(index_to_upperChar(j))) {
                    bases += base[j];
                    if (base[j]>max_base) { // search for most used base
                        max_base     = base[j];
                        max_base_idx = j;
                    }
                }
            }

            int gaps = sequenceUnits-bases; // count of gaps

            DUMPINT(sequenceUnits);
            DUMPINT(gaps);
            DUMPINT(bases);

            // What to do with gaps?

            if (gaps == sequenceUnits) {
                consensus_string[o] = '=';
            }
            else if (BK.countgaps && PERCENT(gaps, sequenceUnits) >= BK.gapbound) {
                DUMPINT(PERCENT(gaps,sequenceUnits));
                DUMPINT(BK.gapbound);
                consensus_string[o] = '-';
            }
            else {
                char kchar  = 0; // character for consensus
                int  kcount = 0; // count this character

                if (BK.group) { // simplify using IUPAC
                    if (ali_type == GB_AT_RNA || ali_type == GB_AT_DNA) {
                        int no_of_bases = 0; // count of different bases used to create iupac
                        char used_bases[MAX_BASES_TABLES+1]; // string containing those bases

                        for (int j=0; j<used_bases_tables; j++) {
                            int bchar = index_to_upperChar(j);

                            if (!isGap(bchar)) {
                                if (PERCENT(base[j],bases) >= BK.considbound) {
#if defined(DEBUG_CONSENSUS)
                                    if (!kcount) DUMPINT(BK.considbound);
#endif
                                    used_bases[no_of_bases++]  = index_to_upperChar(j);
                                    kcount                    += base[j];

                                    DUMPINT(base[j]);
                                    DUMPINT(PERCENT(base[j],bases));
                                    DUMPINT(kcount);
                                }
                            }
                        }
                        used_bases[no_of_bases] = 0;
                        kchar = iupac::encode(used_bases, ali_type);
                    }
                    else {
                        ct_assert(ali_type == GB_AT_AA);

                        const int amino_groups = iupac::AA_GROUP_COUNT;
                        int       group_count[amino_groups];

                        for (int j=0; j<amino_groups; j++) {
                            group_count[j] = 0;
                        }
                        for (int j=0; j<used_bases_tables; j++) {
                            unsigned char bchar = index_to_upperChar(j);

                            if (!isGap(bchar)) {
                                if (PERCENT(base[j], bases) >= BK.considbound) {
                                    group_count[iupac::get_amino_group_for(bchar)] += base[j];
                                }
                            }
                        }

                        int bestGroup = 0;
                        for (int j=0; j<amino_groups; j++) {
                            if (group_count[j]>kcount) {
                                bestGroup = j;
                                kcount = group_count[j];
                            }
                        }

                        kchar = iupac::get_amino_consensus_char(iupac::Amino_Group(bestGroup));
                    }
                }
                if (!kcount) {           // IUPAC grouping is either off OR didnt consider any bases
                    ct_assert(max_base); // expect at least one base to occur
                    ct_assert(max_base_idx >= 0);

                    kchar  = index_to_upperChar(max_base_idx);
                    kcount = max_base;
                }

                ct_assert(kchar);
                ct_assert(kcount);
                ct_assert(kcount<=bases);

                // show as upper or lower case ?
                int percent = PERCENT(kcount, sequenceUnits); // @@@ if gaps==off -> calc percent of non-gaps
                DUMPINT(percent);
                DUMPINT(BK.upper);
                DUMPINT(BK.lower);
                if (percent>=BK.upper) {
                    consensus_string[o] = kchar;
                }
                else if (percent>=BK.lower) {
                    consensus_string[o] = tolower(kchar);
                }
                else {
                    consensus_string[o] = '.';
                }
            }
            ct_assert(consensus_string[o]);

#if defined(DEBUG_CONSENSUS)
            if (dumpcol) fprintf(stderr, "result='%c'\n", consensus_string[o]);
#endif
        }
    }
    else {
        memset(consensus_string, '?', right_idx-left_idx+1);
    }
#undef PERCENT
}

// -----------------------
//      BaseFrequencies

bool               BaseFrequencies::initialized       = false;
uint8_t            BaseFrequencies::unitsPerSequence  = 12;
unsigned char      BaseFrequencies::char_to_index_tab[MAXCHARTABLE];
bool               BaseFrequencies::is_gap[MAXCHARTABLE];
unsigned char     *BaseFrequencies::upper_index_chars = 0;
int                BaseFrequencies::used_bases_tables = 0;
GB_alignment_type  BaseFrequencies::ali_type          = GB_AT_RNA;

inline void BaseFrequencies::set_char_to_index(unsigned char c, int index)
{
    ct_assert(index>=0 && index<used_bases_tables);
    char_to_index_tab[upper_index_chars[index] = toupper(c)] = index;
    char_to_index_tab[tolower(c)] = index;
}

void BaseFrequencies::expand_tables() {
    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).expand_table_entry_size();
    }
}

void BaseFrequencies::setup(const char *gap_chars, GB_alignment_type ali_type_) {
    const char *groups       = 0;

    ali_type = ali_type_;

    switch (ali_type) {
        case GB_AT_RNA:
            groups = "A,C,G,TU,MRWSYKVHDBN,";
            break;
        case GB_AT_DNA:
            groups = "A,C,G,UT,MRWSYKVHDBN,";
            break;
        case GB_AT_AA:
            groups = "P,A,G,S,T,Q,N,E,D,B,Z,H,K,R,L,I,V,M,F,Y,W,C,X"; // @@@ DRY (create 'groups' from AA_GROUP_...)
            break;
        default:
            ct_assert(0);
            break;
    }

    ct_assert(groups);

    for (int i = 0; i<MAXCHARTABLE; ++i) is_gap[i] = false;

    int   gapcharcount     = 0;
    char *unique_gap_chars = strdup(gap_chars);
    for (int i = 0; gap_chars[i]; ++i) {
        if (!is_gap[safeCharIndex(gap_chars[i])]) { // ignore duplicate occurrences in 'gap_chars'
            is_gap[safeCharIndex(gap_chars[i])] = true;
            unique_gap_chars[gapcharcount++]    = gap_chars[i];
        }
    }

    used_bases_tables = gapcharcount;

    for (int i=0; groups[i]; i++) {
        if (groups[i]==',') {
            used_bases_tables++;
        }
    }

    upper_index_chars = new unsigned char[used_bases_tables];

    int idx = 0;
    unsigned char init_val = used_bases_tables-1; // map all unknown stuff to last table
    memset(char_to_index_tab, init_val, MAXCHARTABLE*sizeof(*char_to_index_tab));

    for (int i=0; groups[i]; i++) {
        if (groups[i]==',') {
            idx++;
        }
        else {
            ct_assert(isupper(groups[i]));
            set_char_to_index(groups[i], idx);
        }
    }

    const char *align_string_ptr = unique_gap_chars;
    while (1) {
        char c = *align_string_ptr++;
        if (!c) break;
        set_char_to_index(c, idx++);
    }

    free(unique_gap_chars);
    ct_assert(idx==used_bases_tables);
    initialized = true;
}

BaseFrequencies::BaseFrequencies(int maxseqlength)
    : ignore(0)
{
    ct_assert(initialized);

    bases_table = new SepBaseFreqPtr[used_bases_tables];

    for (int i=0; i<used_bases_tables; i++) {
        bases_table[i] = new SepBaseFreq(maxseqlength);
    }

    sequenceUnits = 0;
}

void BaseFrequencies::init(int maxseqlength)
{
    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).init(maxseqlength);
    }

    sequenceUnits = 0;
}

void BaseFrequencies::bases_and_gaps_at(int column, int *bases, int *gaps) const
{
    int i,
        b = 0,
        g = 0;

    for (i=0; i<used_bases_tables; i++) {
        char c = upper_index_chars[i];

        if (isGap(c)) {
            g += table(c)[column];
        }
        else {
            b += table(c)[column];
        }
    }

    if (bases) {
        *bases = b/unitsPerSequence;
        ct_assert((b%unitsPerSequence) == 0);
    }
    if (gaps)  {
        *gaps  = g/unitsPerSequence;
        ct_assert((g%unitsPerSequence) == 0);
    }
}

BaseFrequencies::~BaseFrequencies()
{
    int i;
    for (i=0; i<used_bases_tables; i++) {
        delete bases_table[i];
    }

    delete [] bases_table;
}

const PosRange *BaseFrequencies::changed_range(const BaseFrequencies& other) const
{
    int i;
    int Size = size();
    int start = Size-1;
    int end = 0;

    ct_assert(Size==other.size());
    for (i=0; i<used_bases_tables; i++) {
        if (linear_table(i).firstDifference(other.linear_table(i), 0, start, &start)) {
            if (end<start) {
                end = start;
            }
            linear_table(i).lastDifference(other.linear_table(i), end+1, Size-1, &end);

            for (; i<used_bases_tables; i++) {
                linear_table(i).firstDifference(other.linear_table(i), 0, start-1, &start);
                if (end<start) {
                    end = start;
                }
                linear_table(i).lastDifference(other.linear_table(i), end+1, Size-1, &end);
            }

            ct_assert(start<=end);

            static PosRange range;
            range = PosRange(start, end);

            return &range;
        }
    }
    return NULL;
}
void BaseFrequencies::add(const BaseFrequencies& other)
{
    if (other.ignore) return;
    if (other.size()==0) return;
    prepare_to_add_elements(other.added_sequences());
    add(other, 0, other.size()-1);
}
void BaseFrequencies::add(const BaseFrequencies& other, int start, int end)
{
    if (other.ignore) return;

    test();
    other.test();

    ct_assert(start<=end);

    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).add(other.linear_table(i), start, end);
    }

    sequenceUnits += other.sequenceUnits;

    test();
}

void BaseFrequencies::sub(const BaseFrequencies& other)
{
    if (other.ignore) return;
    sub(other, 0, other.size()-1);
}

void BaseFrequencies::sub(const BaseFrequencies& other, int start, int end)
{
    if (other.ignore) return;

    test();
    other.test();
    ct_assert(start<=end);

    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).sub(other.linear_table(i), start, end);
    }

    sequenceUnits -= other.sequenceUnits;

    test();
}

void BaseFrequencies::sub_and_add(const BaseFrequencies& Sub, const BaseFrequencies& Add, PosRange range) {
    test();
    Sub.test();
    Add.test();

    ct_assert(!Sub.ignore && !Add.ignore);
    ct_assert(range.is_part());

    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).sub_and_add(Sub.linear_table(i), Add.linear_table(i), range);
    }

    test();
}

const PosRange *BaseFrequencies::changed_range(const char *string1, const char *string2, int min_len)
{
    const unsigned long *range1 = (const unsigned long *)string1;
    const unsigned long *range2 = (const unsigned long *)string2;
    const int            step   = sizeof(*range1);

    int l = min_len/step;
    int r = min_len%step;
    int i;
    int j;
    int k;

    int start = -1, end = -1;

    for (i=0; i<l; i++) {       // search wordwise (it's faster)
        if (range1[i] != range2[i]) {
            k = i*step;
            for (j=0; j<step; j++) {
                if (string1[k+j] != string2[k+j]) {
                    start = end = k+j;
                    break;
                }
            }

            break;
        }
    }

    k = l*step;
    if (i==l) {         // no difference found in word -> look at rest
        for (j=0; j<r; j++) {
            if (string1[k+j] != string2[k+j]) {
                start = end = k+j;
                break;
            }
        }

        if (j==r) {     // the strings are equal
            return 0;
        }
    }

    ct_assert(start != -1 && end != -1);

    for (j=r-1; j>=0; j--) {    // search rest backwards
        if (string1[k+j] != string2[k+j]) {
            end = k+j;
            break;
        }
    }

    if (j==-1) {                // not found in rest -> search words backwards
        int m = start/step;
        for (i=l-1; i>=m; i--) {
            if (range1[i] != range2[i]) {
                k = i*step;
                for (j=step-1; j>=0; j--) {
                    if (string1[k+j] != string2[k+j]) {
                        end = k+j;
                        break;
                    }
                }
                break;
            }
        }
    }

    ct_assert(start<=end);

    static PosRange range;
    range = PosRange(start, end);

    return &range;
}

void BaseFrequencies::add(const char *scan_string, int len)
{
    test();

    prepare_to_add_elements(1);

    int i;
    int sz = size();

    if (get_table_entry_size()==SHORT_TABLE_ELEM_SIZE) {
        for (i=0; i<len; i++) {
            unsigned char c = scan_string[i];
            if (!c) break;
            table(c).inc_short(i, unitsPerSequence);
        }
        SepBaseFreq& t = table('.');
        for (; i<sz; i++) {
            t.inc_short(i, unitsPerSequence);
        }
    }
    else {
        for (i=0; i<len; i++) {
            unsigned char c = scan_string[i];
            if (!c) break;
            table(c).inc_long(i, unitsPerSequence);
        }
        SepBaseFreq& t = table('.');
        for (; i<sz; i++) { // LOOP_VECTORIZED
            t.inc_long(i, unitsPerSequence);
        }
    }

    sequenceUnits += unitsPerSequence;

    test();
}

void BaseFrequencies::sub(const char *scan_string, int len)
{
    test();

    int i;
    int sz = size();

    if (get_table_entry_size()==SHORT_TABLE_ELEM_SIZE) {
        for (i=0; i<len; i++) {
            unsigned char c = scan_string[i];
            if (!c) break;
            table(c).dec_short(i, unitsPerSequence);
        }
        SepBaseFreq& t = table('.');
        for (; i<sz; i++) {
            t.dec_short(i, unitsPerSequence);
        }
    }
    else {
        for (i=0; i<len; i++) {
            unsigned char c = scan_string[i];
            if (!c) break;
            table(c).dec_long(i, unitsPerSequence);
        }
        SepBaseFreq& t = table('.');
        for (; i<sz; i++) { // LOOP_VECTORIZED
            t.dec_long(i, unitsPerSequence);
        }
    }

    sequenceUnits -= unitsPerSequence;

    test();
}

void BaseFrequencies::sub_and_add(const char *old_string, const char *new_string, PosRange range) {
    test();
    int start = range.start();
    int end   = range.end();
    ct_assert(start<=end);

    if (get_table_entry_size()==SHORT_TABLE_ELEM_SIZE) {
        for (int i=start; i<=end; i++) {
            unsigned char o = old_string[i];
            unsigned char n = new_string[i];

            ct_assert(o && n); // passed string have to be defined in range

            if (o!=n) {
                table(o).dec_short(i, unitsPerSequence);
                table(n).inc_short(i, unitsPerSequence);
            }
        }
    }
    else {
        for (int i=start; i<=end; i++) {
            unsigned char o = old_string[i];
            unsigned char n = new_string[i];

            ct_assert(o && n); // passed string have to be defined in range

            if (o!=n) {
                table(o).dec_long(i, unitsPerSequence);
                table(n).inc_long(i, unitsPerSequence);
            }
        }
    }

    test();
}

void BaseFrequencies::change_table_length(int new_length) {
    for (int c=0; c<used_bases_tables; c++) {
        linear_table(c).change_table_length(new_length, 0);
    }
    test();
}

//  --------------
//      tests:

#if defined(TEST_CHAR_TABLE_INTEGRITY) || defined(ASSERTION_USED)

bool BaseFrequencies::empty() const
{
    int i;
    for (i=0; i<used_bases_tables; i++) {
        if (!linear_table(i).empty()) {
            return false;
        }
    }
    return true;
}

bool BaseFrequencies::ok() const
{
    if (empty()) return true;
    if (is_ignored()) return true;

    if (sequenceUnits<0) {
        fprintf(stderr, "Negative # of sequences (%i) in BaseFrequencies\n", added_sequences());
        return false;
    }

    const int table_size = size();
    for (int i=0; i<table_size; i++) {
        int bases, gaps;
        bases_and_gaps_at(i, &bases, &gaps);

        if (!bases && !gaps) {                      // this occurs after insert column
            for (int j=i+1; j<table_size; j++) {    // test if we find any bases till end of table !!!

                bases_and_gaps_at(j, &bases, 0);
                if (bases) { // bases found -> empty position was illegal
                    fprintf(stderr, "Zero in BaseFrequencies at column %i\n", i);
                    return false;
                }
            }
            break;
        }
        else {
            if ((bases+gaps)!=added_sequences()) {
                fprintf(stderr, "bases+gaps should be equal to # of sequences at column %i\n", i);
                return false;
            }
        }
    }

    return true;
}

#endif // TEST_CHAR_TABLE_INTEGRITY || ASSERTION_USED

#if defined(TEST_CHAR_TABLE_INTEGRITY)

void BaseFrequencies::test() const {

    if (!ok()) {
        GBK_terminate("BaseFrequencies::test() failed");
    }
}

#endif // TEST_CHAR_TABLE_INTEGRITY


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#define SETUP(gapChars,alitype) BaseFrequencies::setup(gapChars,alitype)

void TEST_char_table() {
    const char alphabeth[]   = "ACGTN-";
    const int alphabeth_size = strlen(alphabeth);

    const int seqlen = 30;
    char      seq[seqlen+1];

    const char *gapChars = ".-"; // @@@ doesnt make any difference which gaps are used - why?
    // const char *gapChars = "-";
    // const char *gapChars = ".";
    // const char *gapChars = "A"; // makes me fail
    SETUP(gapChars, GB_AT_RNA);

    ConsensusBuildParams BK;
    // BK.lower = 70; BK.upper = 95; BK.gapbound = 60; // defaults from awars
    BK.lower    = 40; BK.upper = 70; BK.gapbound = 40;

    srand(100);
    for (int loop = 0; loop<5; ++loop) {
        unsigned seed = rand();

        srand(seed);

        BaseFrequencies tab(seqlen);

        // build some seq
        for (int c = 0; c<seqlen; ++c) seq[c] = alphabeth[rand()%alphabeth_size];
        seq[seqlen]                           = 0;

        TEST_EXPECT_EQUAL(strlen(seq), size_t(seqlen));

        const int  add_count = 300;
        char      *added_seqs[add_count];

        const char *s1 = "ACGTACGTAcgtacgtaCGTACGTACGTAC";
        const char *s2 = "MRWSYKVHDBNmrwsykvhdbnSYKVHDBN"; // use ambiguities

        // add seq multiple times
        for (int a = 0; a<add_count; ++a) {
            tab.add(seq, seqlen);
            added_seqs[a] = strdup(seq);

            // modify 1 character in seq:
            int sidx  = rand()%seqlen;
            int aidx  = rand()%alphabeth_size;
            seq[sidx] = alphabeth[aidx];

            if (a == 16) { // with 15 sequences in tab
                // test sub_and_add (short table version)
                char *consensus = tab.build_consensus_string(BK);

                tab.add(s1, seqlen);
                PosRange r(0, seqlen-1);
                tab.sub_and_add(s1, s2, r);
                tab.sub_and_add(s2, consensus, r);
                tab.sub(consensus, seqlen);

                char *consensus2 = tab.build_consensus_string(BK);
                TEST_EXPECT_EQUAL(consensus, consensus2);

                free(consensus2);
                free(consensus);
            }
        }

        // build consensi (just check regression)
        {
            char *consensus = tab.build_consensus_string(BK);
            switch (seed) {
                case 677741240: TEST_EXPECT_EQUAL(consensus, "k-s-NW..aWu.NnWYa.R.mKcNK.c.rn"); break;
                case 721151648: TEST_EXPECT_EQUAL(consensus, "aNnn..K..-gU.RW-SNcau.WNNacn.u"); break;
                case 345295160: TEST_EXPECT_EQUAL(consensus, "yy-g..kMSn...NucyNny.Rnn.-Ng.k"); break;
                case 346389111: TEST_EXPECT_EQUAL(consensus, ".unAn.y.nN.kc-cS.RauNm..Sa-kY."); break;
                case 367171911: TEST_EXPECT_EQUAL(consensus, "na.NanNunc-.-NU.aYgn-nng-nWanM"); break;
                default: TEST_EXPECT_EQUAL(consensus, "undef");
            }

            // test sub_and_add()

            tab.add(s1, seqlen);
            PosRange r(0, seqlen-1);
            tab.sub_and_add(s1, s2, r);
            tab.sub_and_add(s2, consensus, r);
            tab.sub(consensus, seqlen);

            char *consensus2 = tab.build_consensus_string(BK);
            TEST_EXPECT_EQUAL(consensus, consensus2);

            free(consensus2);
            free(consensus);
        }

        for (int a = 0; a<add_count; a++) {
            tab.sub(added_seqs[a], seqlen);
            freenull(added_seqs[a]);
        }
        {
            char *consensus = tab.build_consensus_string(BK);
            TEST_EXPECT_EQUAL(consensus, "??????????????????????????????"); // check tab is empty
            free(consensus);
        }
    }
}

void TEST_nucleotide_consensus() {
    // keep similar to ../NTREE/AP_consensus.cxx@TEST_nucleotide_consensus
    const char *sequence[] = {
        "-.AAAAAAAAAAcAAAAAAAAATTTTTTTTTTTTTTTTTAAAAAAAAgggggAAAAgAA---",
        "-.-AAAAAAAAAccAAAAAAAAggTTgTTTTgTTTTTTTcccAAAAAgggggAAAAgAA---",
        "-.--AAAAAAAAcccAAAAAAA-ggTggTTTggTTTTTTccccAAAAgggCCtAAAgAC---",
        "-.---AAAAAAAccccAAAAAA-ggggggTTgggTTTTTcccccAAAggCCC-tAACtC---",
        "-.----AAAAAAcccccAAAAA----ggggTggggTTTTGGGcccAAgCCCt-ttACtG---",
        "-.-----AAAAAccccccAAAA----ggggggggggTTgGGGGcccAcCCtt--tttCG---",
        "-.------AAAAcccccccAAA---------ggggggTgGGGGGccccCt----tt-gT---",
        "-.-------AAAccccccccAA---------ggggggggttGGGGccct------t--Tyyk",
        "-.--------AAcccccccccA----------------gttGGGGGct-------t---ymm",
        "-.---------Acccccccccc----------------gtAGGGGGG---------------",
    };
    const char *expected_consensus[] = {
        "==----..aaaACccMMMMMaa----.....g.kkk.uKb.ssVVmmss...-.ww...---", // default settings (see ConsensusBuildParams-ctor), gapbound=60, considbound=30, lower/upper=70/95
        "==......aaaACccMMMMMaa.........g.kkk.uKb.ssVVmmss.....ww......", // countgaps=0
        "==aaaaaaaAAACCCMMMMMAAkgkugkkkuggKKKuuKBsSSVVMMSsssbwwwWswannn", // countgaps=0,              considbound=26, lower=0, upper=75 (as described in #663)
        "==---aaaaAAACCCMMMMMAA-gkugkkkuggKKKuuKBsSSVVMMSsssb-wwWswa---", // countgaps=1, gapbound=70, considbound=26, lower=0, upper=75
        "==---aaaaAAACCMMMMMMMA-kkkgkkkugKKKKKuKBNSVVVVMSsssb-wwWswN---", // countgaps=1, gapbound=70, considbound=20, lower=0, upper=75
    };
    const size_t seqlen         = strlen(sequence[0]);
    const int    sequenceCount  = ARRAY_ELEMS(sequence);
    const int    consensusCount = ARRAY_ELEMS(expected_consensus);

    const char *gapChars = ".-";
    SETUP(gapChars, GB_AT_RNA);

    BaseFrequencies tab(seqlen);
    for (int s = 0; s<sequenceCount; ++s) {
        ct_assert(strlen(sequence[s]) == seqlen);
        tab.add(sequence[s], seqlen);
    }

    ConsensusBuildParams BK;
    for (int c = 0; c<consensusCount; ++c) {
        TEST_ANNOTATE(GBS_global_string("c=%i", c));
        switch (c) {
            case 0: break;                                                     // use default settings
            case 1: BK.countgaps   = false; break;                             // dont count gaps
            case 2: BK.considbound = 26; BK.lower = 0; BK.upper = 75; break;   // settings from #663
            case 3: BK.countgaps   = true; BK.gapbound = 70; break;
            case 4: BK.considbound = 20; break;
            default: ct_assert(0); break;                                      // missing
        }

        char *consensus = tab.build_consensus_string(BK);
        TEST_EXPECT_EQUAL(consensus, expected_consensus[c]);
        free(consensus);
    }
}

void TEST_amino_consensus() {
    // keep similar to ../NTREE/AP_consensus.cxx@TEST_amino_consensus
    const char *sequence[] = {
        "-.ppppppppppQQQQQQQQQDDDDDELLLLLwwwwwwwwwwwwwwwwgggggggggggSSSe-PPP-DEL-",
        "-.-pppppppppkQQQQQQQQnDDDDELLLLLVVwwVwwwwVwwwwwwSgggggggggSSSee-QPP-DEL-",
        "-.--ppppppppkkQQQQQQQnnDDDELLLLL-VVwVVwwwVVwwwwwSSgggggggSSSeee-KQP-DEI-",
        "-.---pppppppkkkQQQQQQnnnDDELLLLL-VVVVVVwwVVVwwwwSSSgggggSSSeee--LQQ-DQI-",
        "-.----ppppppkkkkQQQQQnnnnDELLLLL----VVVVwVVVVwwweSSSgggSSSeee---WKQ-NQJ-",
        "-.-----pppppkkkkkQQQQnnnnnqiLLLL----VVVVVVVVVVwweeSSSggSSeee-----KQ-NQJ-",
        "-.------ppppkkkkkkQQQnnnnnqiiLLL---------VVVVVVweeeSSSgSeee------LK-NZJ-",
        "-.-------pppkkkkkkkQQnnnnnqiiiLL---------VVVVVVVeeeeSSSeee-------LK-NZJ-",
        "-.--------ppkkkkkkkkQnnnnnqiiiiL----------------eeeeeSSee--------WK-BZJ-",
        "-.---------pkkkkkkkkknnnnnqiiiii----------------eeeeeeSe---------WK-BZJ-",
    };
    const char *expected_consensus[] = {
        "==----..aaaAhhh...bbbbbBBBBIIIii----.....i.....f...aaaAa.....--=...=bB-=", // default settings (see ConsensusBuildParams-ctor), gapbound=60, considbound=30, lower/upper=70/95
        "==......aaaAhhh...bbbbbBBBBIIIii.........i.....f...aaaAa.......=...=bB.=", // countgaps=0
        "==aaaaaaaAAAHHhhbbbBBBBBBBBIIIIIiiifiiiffiiiifffbbaaAAAaaaaabbb=pph=BBi=", // countgaps=0,              considbound=26, lower=0, upper=75
        "==---aaaaAAAHHhhbbbBBBBBBBBIIIII-iifiiiffiiiifffbbaaAAAaaaaabb-=pph=BBi=", // countgaps=1, gapbound=70, considbound=26, lower=0, upper=75
        "==---aaaaAAAHHhhbbbBBBBBBBBIIIII-iifiiiffiiiifffbaaaAAAaaaaabb-=aah=BBi=", // countgaps=1, gapbound=70, considbound=20, lower=0, upper=75
    };
    const size_t seqlen         = strlen(sequence[0]);
    const int    sequenceCount  = ARRAY_ELEMS(sequence);
    const int    consensusCount = ARRAY_ELEMS(expected_consensus);

    const char *gapChars = ".-";
    SETUP(gapChars, GB_AT_AA);

    BaseFrequencies tab(seqlen);
    for (int s = 0; s<sequenceCount; ++s) {
        ct_assert(strlen(sequence[s]) == seqlen);
        tab.add(sequence[s], seqlen);
    }

    ConsensusBuildParams BK;
    for (int c = 0; c<consensusCount; ++c) {
        TEST_ANNOTATE(GBS_global_string("c=%i", c));
        switch (c) {
            case 0: break;                                                      // use default settings
            case 1: BK.countgaps   = false; break;                              // dont count gaps
            case 2: BK.considbound = 26; BK.lower = 0; BK.upper = 75; break;    // settings from #663
            case 3: BK.countgaps   = true; BK.gapbound = 70; break;
            case 4: BK.considbound = 20; break;
            default: ct_assert(0); break;                                       // missing
        }

        char *consensus = tab.build_consensus_string(BK);
        TEST_EXPECT_EQUAL(consensus, expected_consensus[c]);
        free(consensus);
    }
}

void TEST_SepBaseFreq() {
    const int LEN  = 20;
    const int OFF1 = 6;
    const int OFF2 = 7; // adjacent to OFF1

    SepBaseFreq short1(LEN), short2(LEN);
    for (int i = 0; i<100; ++i) short1.inc_short(OFF1, 1);
    for (int i = 0; i<70;  ++i) short1.inc_short(OFF2, 1);
    for (int i = 0; i<150; ++i) short2.inc_short(OFF1, 1);
    for (int i = 0; i<80;  ++i) short2.inc_short(OFF2, 1);

    SepBaseFreq long1(LEN), long2(LEN);
    long1.expand_table_entry_size();
    long2.expand_table_entry_size();
    for (int i = 0; i<2000; ++i) long1.inc_long(OFF1, 1);
    for (int i = 0; i<2500; ++i) long2.inc_long(OFF1, 1);

    SepBaseFreq shortsum(LEN);
    shortsum.add(short1, 0, LEN-1); TEST_EXPECT_EQUAL(shortsum[OFF1], 100); TEST_EXPECT_EQUAL(shortsum[OFF2],  70);
    shortsum.add(short2, 0, LEN-1); TEST_EXPECT_EQUAL(shortsum[OFF1], 250); TEST_EXPECT_EQUAL(shortsum[OFF2], 150);
    shortsum.sub(short1, 0, LEN-1); TEST_EXPECT_EQUAL(shortsum[OFF1], 150); TEST_EXPECT_EQUAL(shortsum[OFF2],  80);

    shortsum.sub_and_add(short1, short2, PosRange(0, LEN-1)); TEST_EXPECT_EQUAL(shortsum[OFF1], 200); TEST_EXPECT_EQUAL(shortsum[OFF2], 90);

    // shortsum.add(long1, 0, LEN-1); // invalid operation -> terminate

    SepBaseFreq longsum(LEN);
    longsum.expand_table_entry_size();
    longsum.add(long1,  0, LEN-1); TEST_EXPECT_EQUAL(longsum[OFF1], 2000);
    longsum.add(long2,  0, LEN-1); TEST_EXPECT_EQUAL(longsum[OFF1], 4500);
    longsum.sub(long1,  0, LEN-1); TEST_EXPECT_EQUAL(longsum[OFF1], 2500);
    longsum.add(short1, 0, LEN-1); TEST_EXPECT_EQUAL(longsum[OFF1], 2600);
    longsum.sub(short2, 0, LEN-1); TEST_EXPECT_EQUAL(longsum[OFF1], 2450);

    longsum.sub_and_add(long1,  long2,  PosRange(0, LEN-1)); TEST_EXPECT_EQUAL(longsum[OFF1], 2950);
    longsum.sub_and_add(short1, short2, PosRange(0, LEN-1)); TEST_EXPECT_EQUAL(longsum[OFF1], 3000);
    longsum.sub_and_add(long1,  short2, PosRange(0, LEN-1)); TEST_EXPECT_EQUAL(longsum[OFF1], 1150);
    longsum.sub_and_add(short1, long2,  PosRange(0, LEN-1)); TEST_EXPECT_EQUAL(longsum[OFF1], 3550);

    int pos = -1;
    TEST_EXPECT_EQUAL(short1.firstDifference(short2, 0, LEN-1, &pos), 1); TEST_EXPECT_EQUAL(pos, OFF1);
    TEST_EXPECT_EQUAL(short1.lastDifference (short2, 0, LEN-1, &pos), 1); TEST_EXPECT_EQUAL(pos, OFF2);
    TEST_EXPECT_EQUAL(short1.firstDifference(long1,  0, LEN-1, &pos), 1); TEST_EXPECT_EQUAL(pos, OFF1);
    TEST_EXPECT_EQUAL(short1.lastDifference (long1,  0, LEN-1, &pos), 1); TEST_EXPECT_EQUAL(pos, OFF2);
    TEST_EXPECT_EQUAL(long2.firstDifference (short2, 0, LEN-1, &pos), 1); TEST_EXPECT_EQUAL(pos, OFF1);
    TEST_EXPECT_EQUAL(long2.lastDifference  (short2, 0, LEN-1, &pos), 1); TEST_EXPECT_EQUAL(pos, OFF2);
    TEST_EXPECT_EQUAL(long2.firstDifference (long1,  0, LEN-1, &pos), 1); TEST_EXPECT_EQUAL(pos, OFF1);
    TEST_EXPECT_EQUAL(long2.lastDifference  (long1,  0, LEN-1, &pos), 1); TEST_EXPECT_EQUAL(pos, OFF1);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------



