// =============================================================== //
//                                                                 //
//   File      : ED4_mini_classes.cxx                              //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#include "ed4_class.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_awars.hxx"
#include "ed4_tools.hxx"
#include "ed4_seq_colors.hxx"

#include <aw_awar.hxx>
#include <aw_root.hxx>
#include <iupac.h>

#include <cctype>
#include <arb_msg.h>
#include <arb_defs.h>

#define CONSENSUS_AWAR_SOURCE CAS_EDIT4
#include <consensus.h>

// ------------------------
//      ED4_bases_table

#ifdef DEBUG
// # define COUNT_BASES_TABLE_SIZE
#endif

#ifdef COUNT_BASES_TABLE_SIZE
static long bases_table_size = 0L;
#endif

void ED4_bases_table::init(int length)
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

void ED4_bases_table::expand_table_entry_size() { // converts short table into long table
    e4_assert(table_entry_size==SHORT_TABLE_ELEM_SIZE);

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

#define INVALID_TABLE_OPERATION() GBK_terminatef("ED4_bases_table: invalid operation at %i", __LINE__)

void ED4_bases_table::add(const ED4_bases_table& other, int start, int end)
{
    e4_assert(no_of_entries==other.no_of_entries);
    e4_assert(start>=0);
    e4_assert(end<no_of_entries);
    e4_assert(start<=end);

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
void ED4_bases_table::sub(const ED4_bases_table& other, int start, int end)
{
    e4_assert(no_of_entries==other.no_of_entries);
    e4_assert(start>=0);
    e4_assert(end<no_of_entries);
    e4_assert(start<=end);

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
void ED4_bases_table::sub_and_add(const ED4_bases_table& Sub, const ED4_bases_table& Add, PosRange range)
{
    e4_assert(no_of_entries==Sub.no_of_entries);
    e4_assert(no_of_entries==Add.no_of_entries);

    int start = range.start();
    int end   = range.end();

    e4_assert(start>=0);
    e4_assert(end<no_of_entries);
    e4_assert(start<=end);

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

int ED4_bases_table::firstDifference(const ED4_bases_table& other, int start, int end, int *firstDifferentPos) const
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
int ED4_bases_table::lastDifference(const ED4_bases_table& other, int start, int end, int *lastDifferentPos) const
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


ED4_bases_table::ED4_bases_table(int maxseqlength)
    : table_entry_size(SHORT_TABLE_ELEM_SIZE),
      no_of_entries(0)
{
    no_of_bases.shortTable = NULL;
    if (maxseqlength) init(maxseqlength);
}

ED4_bases_table::~ED4_bases_table()
{
    delete [] no_of_bases.shortTable;
#ifdef COUNT_BASES_TABLE_SIZE
    bases_table_size -= (no_of_entries+1)*table_entry_size;
    printf("bases_table_size = %li\n", bases_table_size);
#endif
}

void ED4_bases_table::change_table_length(int new_length, int default_entry)
{
    if (new_length!=no_of_entries) {
        int min_length = new_length<no_of_entries ? new_length : no_of_entries;
        int growth = new_length-no_of_entries;

        if (table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            unsigned char *new_table = new unsigned char[new_length+1];

            e4_assert(default_entry>=0 && default_entry<256);

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

int ED4_bases_table::empty() const
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

// ------------------------
//      Build consensus

static ConsensusBuildParams *BK = NULL; // @@@ make member of ED4_char_table ?

void ED4_consensus_definition_changed(AW_root*) {
    delete BK; BK = 0; // invalidate

    ED4_reference *ref = ED4_ROOT->reference;
    if (ref->reference_is_a_consensus()) {
        ref->data_changed_cb(NULL);
        ED4_ROOT->request_refresh_for_specific_terminals(ED4_L_SEQUENCE_STRING); // refresh all sequences
    }
    else {
        ED4_ROOT->request_refresh_for_consensus_terminals();
    }
}

static ARB_ERROR toggle_consensus_display(ED4_base *base, AW_CL show) {
    if (base->is_consensus_manager()) {
        ED4_manager *consensus_man = base->to_manager();
        ED4_spacer_terminal *spacer = consensus_man->parent->get_defined_level(ED4_L_SPACER)->to_spacer_terminal();

        if (show) {
            consensus_man->unhide_children();
            spacer->extension.size[HEIGHT] = SPACERHEIGHT;
        }
        else {
            consensus_man->hide_children();

            ED4_group_manager *group_man = consensus_man->get_parent(ED4_L_GROUP)->to_group_manager();
            spacer->extension.size[HEIGHT] = (group_man->dynamic_prop&ED4_P_IS_FOLDED) ? SPACERNOCONSENSUSHEIGHT : SPACERHEIGHT;
        }
    }

    return NULL;
}

void ED4_consensus_display_changed(AW_root *root) {
    int show = root->awar(ED4_AWAR_CONSENSUS_SHOW)->read_int();
    ED4_ROOT->root_group_man->route_down_hierarchy(toggle_consensus_display, show).expect_no_error();
}

char *ED4_char_table::build_consensus_string(PosRange r) const {
    ExplicitRange  range(r, size());
    long           entries = range.size();
    char          *new_buf = (char*)malloc(entries+1);

    build_consensus_string_to(new_buf, range);
    new_buf[entries] = 0;

    return new_buf;
}

#if defined(DEBUG)
 #if defined(DEVEL_RALF)
  #define DEBUG_CONSENSUS
 #endif
#endif

void ED4_char_table::build_consensus_string_to(char *consensus_string, ExplicitRange range) const {
    // 'consensus_string' has to be a buffer of size 'range.size()+1'
    // Note : Always check that consensus behavior is identical to that used in CON_evaluatestatistic()

    if (!BK) BK = new ConsensusBuildParams(ED4_ROOT->aw_root);

    e4_assert(consensus_string);
    e4_assert(range.end()<size());

#define PERCENT(part, all)      ((100*(part))/(all))
#define MAX_BASES_TABLES 41     // 25

    e4_assert(used_bases_tables<=MAX_BASES_TABLES);     // this is correct for DNA/RNA -> build_consensus_string() must be corrected for AMI/PRO

    const int left_idx  = range.start();
    const int right_idx = range.end();

#if defined(DEBUG_CONSENSUS)
 #define DUMPINT(var) do { if (dumpcol) fprintf(stderr, "%s=%i ", #var, var); } while(0)
#else
 #define DUMPINT(var)
#endif

    if (sequences) {
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
                if (!ADPP_IS_ALIGN_CHARACTER(index_to_upperChar(j))) {
                    bases += base[j];
                    if (base[j]>max_base) { // search for most used base
                        max_base     = base[j];
                        max_base_idx = j;
                    }
                }
            }

            int gaps = sequences-bases; // count of gaps

            DUMPINT(sequences);
            DUMPINT(gaps);
            DUMPINT(bases);

            // What to do with gaps?

            if (gaps == sequences) {
                consensus_string[o] = '=';
            }
            else if (BK->countgaps && PERCENT(gaps, sequences) >= BK->gapbound) {
                DUMPINT(PERCENT(gaps,sequences));
                DUMPINT(BK->gapbound);
                consensus_string[o] = '-';
            }
            else {
                char kchar  = 0; // character for consensus
                int  kcount = 0; // count this character

                if (BK->group) { // simplify using IUPAC
                    if (ali_type == GB_AT_RNA || ali_type == GB_AT_DNA) {
                        int no_of_bases = 0; // count of different bases used to create iupac
                        char used_bases[MAX_BASES_TABLES+1]; // string containing those bases

                        for (int j=0; j<used_bases_tables; j++) {
                            int bchar = index_to_upperChar(j);

                            if (!ADPP_IS_ALIGN_CHARACTER(bchar)) {
                                if (PERCENT(base[j],bases) >= BK->considbound) {
#if defined(DEBUG_CONSENSUS)
                                    if (!kcount) DUMPINT(BK->considbound);
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
                        kchar = ED4_encode_iupac(used_bases, ali_type);
                    }
                    else {
                        e4_assert(ali_type == GB_AT_AA);

                        const int amino_groups = iupac::AA_GROUP_COUNT;
                        int       group_count[amino_groups];

                        for (int j=0; j<amino_groups; j++) {
                            group_count[j] = 0;
                        }
                        for (int j=0; j<used_bases_tables; j++) {
                            unsigned char bchar = index_to_upperChar(j);

                            if (!ADPP_IS_ALIGN_CHARACTER(bchar)) {
                                if (PERCENT(base[j], bases) >= BK->considbound) {
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
                    e4_assert(max_base); // expect at least one base to occur
                    e4_assert(max_base_idx >= 0);

                    kchar  = index_to_upperChar(max_base_idx);
                    kcount = max_base;
                }

                e4_assert(kchar);
                e4_assert(kcount);
                e4_assert(kcount<=bases);

                // show as upper or lower case ?
                int percent = PERCENT(kcount, sequences); // @@@ if gaps==off -> calc percent of non-gaps
                DUMPINT(percent);
                DUMPINT(BK->upper);
                DUMPINT(BK->lower);
                if (percent>=BK->upper) {
                    consensus_string[o] = kchar;
                }
                else if (percent>=BK->lower) {
                    consensus_string[o] = tolower(kchar);
                }
                else {
                    consensus_string[o] = '.';
                }
            }
            e4_assert(consensus_string[o]);

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
//      ED4_char_table

bool               ED4_char_table::initialized       = false;
unsigned char      ED4_char_table::char_to_index_tab[MAXCHARTABLE];
unsigned char     *ED4_char_table::upper_index_chars = 0;
unsigned char     *ED4_char_table::lower_index_chars = 0;
int                ED4_char_table::used_bases_tables = 0;
GB_alignment_type  ED4_char_table::ali_type          = GB_AT_RNA;

inline void ED4_char_table::set_char_to_index(unsigned char c, int index)
{
    e4_assert(index>=0 && index<used_bases_tables);
    char_to_index_tab[upper_index_chars[index] = toupper(c)] = index;
    char_to_index_tab[lower_index_chars[index] = tolower(c)] = index;
}
unsigned char ED4_char_table::index_to_upperChar(int index) const
{
    return upper_index_chars[index];
}
unsigned char ED4_char_table::index_to_lowerChar(int index) const
{
    return lower_index_chars[index];
}

void ED4_char_table::expand_tables() {
    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).expand_table_entry_size();
    }
}

void ED4_char_table::initial_setup(const char *gap_chars, GB_alignment_type ali_type_) {
    const char *groups = 0;
    used_bases_tables  = strlen(gap_chars);
    ali_type           = ali_type_;

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
            e4_assert(0);
            break;
    }

    e4_assert(groups);

    int i;

    for (i=0; groups[i]; i++) {
        if (groups[i]==',') {
            used_bases_tables++;
        }
    }

    lower_index_chars = new unsigned char[used_bases_tables];
    upper_index_chars = new unsigned char[used_bases_tables];

    int idx = 0;
    unsigned char init_val = used_bases_tables-1; // map all unknown stuff to last table
    memset(char_to_index_tab, init_val, MAXCHARTABLE*sizeof(*char_to_index_tab));

    for (i=0; groups[i]; i++) {
        if (groups[i]==',') {
            idx++;
        }
        else {
            e4_assert(isupper(groups[i]));
            set_char_to_index(groups[i], idx);
        }
    }

    const char *align_string_ptr = gap_chars;
    while (1) {
        char c = *align_string_ptr++;
        if (!c) break;
        set_char_to_index(c, idx++);
    }

    e4_assert(idx==used_bases_tables);
    initialized = true;
}

ED4_char_table::ED4_char_table(int maxseqlength)
    : ignore(0)
{
    if (!initialized) {
        char *align_string = ED4_ROOT->aw_root->awar_string(ED4_AWAR_GAP_CHARS)->read_string();
        initial_setup(align_string, ED4_ROOT->alignment_type);
        free(align_string);
    }

    bases_table = new ED4_bases_table_ptr[used_bases_tables];

    for (int i=0; i<used_bases_tables; i++) {
        bases_table[i] = new ED4_bases_table(maxseqlength);
    }

    sequences = 0;
}

void ED4_char_table::init(int maxseqlength)
{
    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).init(maxseqlength);
    }

    sequences = 0;
}

void ED4_char_table::bases_and_gaps_at(int column, int *bases, int *gaps) const
{
    int i,
        b = 0,
        g = 0;

    for (i=0; i<used_bases_tables; i++) {
        char c = upper_index_chars[i];

        if (ADPP_IS_ALIGN_CHARACTER(c)) {
            g += table(c)[column];
        }
        else {
            b += table(c)[column];
        }
    }

    if (bases) *bases = b;
    if (gaps)  *gaps  = g;
}

ED4_char_table::~ED4_char_table()
{
    int i;
    for (i=0; i<used_bases_tables; i++) {
        delete bases_table[i];
    }

    delete [] bases_table;
}

const PosRange *ED4_char_table::changed_range(const ED4_char_table& other) const
{
    int i;
    int Size = size();
    int start = Size-1;
    int end = 0;

    e4_assert(Size==other.size());
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

            e4_assert(start<=end);

            static PosRange range;
            range = PosRange(start, end);

            return &range;
        }
    }
    return NULL;
}
void ED4_char_table::add(const ED4_char_table& other)
{
    if (other.ignore) return;
    if (other.size()==0) return;
    prepare_to_add_elements(other.sequences);
    add(other, 0, other.size()-1);
}
void ED4_char_table::add(const ED4_char_table& other, int start, int end)
{
    if (other.ignore) return;

    test();
    other.test();

    e4_assert(start<=end);

    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).add(other.linear_table(i), start, end);
    }

    sequences += other.sequences;

    test();
}

void ED4_char_table::sub(const ED4_char_table& other)
{
    if (other.ignore) return;
    sub(other, 0, other.size()-1);
}

void ED4_char_table::sub(const ED4_char_table& other, int start, int end)
{
    if (other.ignore) return;

    test();
    other.test();
    e4_assert(start<=end);

    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).sub(other.linear_table(i), start, end);
    }

    sequences -= other.sequences;

    test();
}

void ED4_char_table::sub_and_add(const ED4_char_table& Sub, const ED4_char_table& Add) {
    test();

    if (Sub.ignore) {
        e4_assert(Add.ignore);
        return;
    }

    Sub.test();
    Add.test();

    const PosRange *range = Sub.changed_range(Add);
    if (range) {
        prepare_to_add_elements(Add.added_sequences()-Sub.added_sequences());
        sub_and_add(Sub, Add, *range);
    }

    test();
}

void ED4_char_table::sub_and_add(const ED4_char_table& Sub, const ED4_char_table& Add, PosRange range) {
    test();
    Sub.test();
    Add.test();

    e4_assert(!Sub.ignore && !Add.ignore);
    e4_assert(range.is_part());

    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).sub_and_add(Sub.linear_table(i), Add.linear_table(i), range);
    }

    test();
}

const PosRange *ED4_char_table::changed_range(const char *string1, const char *string2, int min_len)
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

    e4_assert(start != -1 && end != -1);

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

    e4_assert(start<=end);

    static PosRange range;
    range = PosRange(start, end);

    return &range;
}

void ED4_char_table::add(const char *scan_string, int len)
{
    test();

    prepare_to_add_elements(1);

    int i;
    int sz = size();

    if (get_table_entry_size()==SHORT_TABLE_ELEM_SIZE) {
        for (i=0; i<len; i++) {
            unsigned char c = scan_string[i];
            if (!c) break;
            table(c).inc_short(i);
        }
        ED4_bases_table& t = table('.');
        for (; i<sz; i++) {
            t.inc_short(i);
        }
    }
    else {
        for (i=0; i<len; i++) {
            unsigned char c = scan_string[i];
            if (!c) break;
            table(c).inc_long(i);
        }
        ED4_bases_table& t = table('.');
        for (; i<sz; i++) { // LOOP_VECTORIZED
            t.inc_long(i);
        }
    }

    sequences++;

    test();
}

void ED4_char_table::sub(const char *scan_string, int len)
{
    test();

    int i;
    int sz = size();

    if (get_table_entry_size()==SHORT_TABLE_ELEM_SIZE) {
        for (i=0; i<len; i++) {
            unsigned char c = scan_string[i];
            if (!c) break;
            table(c).dec_short(i);
        }
        ED4_bases_table& t = table('.');
        for (; i<sz; i++) {
            t.dec_short(i);
        }
    }
    else {
        for (i=0; i<len; i++) {
            unsigned char c = scan_string[i];
            if (!c) break;
            table(c).dec_long(i);
        }
        ED4_bases_table& t = table('.');
        for (; i<sz; i++) { // LOOP_VECTORIZED
            t.dec_long(i);
        }
    }

    sequences--;

    test();
}

void ED4_char_table::sub_and_add(const char *old_string, const char *new_string, PosRange range) {
    test();
    int start = range.start();
    int end   = range.end();
    e4_assert(start<=end);

    int i;
    ED4_bases_table& t = table('.');

    if (get_table_entry_size()==SHORT_TABLE_ELEM_SIZE) {
        for (i=start; i<=end; i++) {
            unsigned char o = old_string[i];
            unsigned char n = new_string[i];

            e4_assert(o); // @@@ if these never fail, some code below is superfluous
            e4_assert(n);
            
            if (!o) {
                for (; n && i<=end; i++) {
                    n = new_string[i];
                    table(n).inc_short(i);
                    t.dec_short(i);
                }
            }
            else if (!n) {
                for (; o && i<=end; i++) {
                    o = old_string[i];
                    table(o).dec_short(i);
                    t.inc_short(i);
                }

            }
            else if (o!=n) {
                table(o).dec_short(i);
                table(n).inc_short(i);

            }
        }
    }
    else {
        for (i=start; i<=end; i++) {
            unsigned char o = old_string[i];
            unsigned char n = new_string[i];

            e4_assert(o); // @@@ if these never fail, some code below is superfluous
            e4_assert(n);
            
            if (!o) {
                for (; n && i<=end; i++) {
                    n = new_string[i];
                    table(n).inc_long(i);
                    t.dec_long(i);
                }
            }
            else if (!n) {
                for (; o && i<=end; i++) {
                    o = old_string[i];
                    table(o).dec_long(i);
                    t.inc_long(i);
                }

            }
            else if (o!=n) {
                table(o).dec_long(i);
                table(n).inc_long(i);
            }
        }
    }

    test();
}

void ED4_char_table::change_table_length(int new_length) {
    for (int c=0; c<used_bases_tables; c++) {
        linear_table(c).change_table_length(new_length, 0);
    }
    test();
}

//  --------------
//      tests:

#if defined(TEST_CHAR_TABLE_INTEGRITY) || defined(ASSERTION_USED)

bool ED4_char_table::empty() const
{
    int i;
    for (i=0; i<used_bases_tables; i++) {
        if (!linear_table(i).empty()) {
            return false;
        }
    }
    return true;
}

bool ED4_char_table::ok() const
{
    if (empty()) return true;
    if (is_ignored()) return true;

    if (sequences<0) {
        fprintf(stderr, "Negative # of sequences (%i) in ED4_char_table\n", sequences);
        return false;
    }

    int i;
    for (i=0; i<MAXSEQUENCECHARACTERLENGTH; i++) {
        int bases;
        int gaps;

        bases_and_gaps_at(i, &bases, &gaps);

        if (!bases && !gaps) { // this occurs after insert column
            int j;
            for (j=i+1; j<MAXSEQUENCECHARACTERLENGTH; j++) {    // test if we find any bases till end of table !!!

                bases_and_gaps_at(j, &bases, 0);
                if (bases) { // bases found -> empty position was illegal
                    fprintf(stderr, "Zero in ED4_char_table at column %i\n", i);
                    return false;
                }
            }
            break;
        }
        else {
            if ((bases+gaps)!=sequences) {
                fprintf(stderr, "bases+gaps should be equal to # of sequences at column %i\n", i);
                return false;
            }
        }
    }

    return true;
}

#endif // TEST_CHAR_TABLE_INTEGRITY || ASSERTION_USED

#if defined(TEST_CHAR_TABLE_INTEGRITY)

void ED4_char_table::test() const {

    if (!ok()) {
        GBK_terminate("ED4_char_table::test() failed");
    }
}

#endif // TEST_CHAR_TABLE_INTEGRITY


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_char_table() {
    const char alphabeth[]   = "ACGTN-";
    const int alphabeth_size = strlen(alphabeth);

    const int seqlen = 30;
    char      seq[seqlen+1];

    const char *gapChars = ".-"; // @@@ doesnt make any difference which gaps are used - why?
    // const char *gapChars = "-";
    // const char *gapChars = ".";
    // const char *gapChars = "A"; // makes me fail
    ED4_char_table::initial_setup(gapChars, GB_AT_RNA);
    ED4_init_is_align_character(gapChars);

    TEST_REJECT(BK); BK = new ConsensusBuildParams;

    // BK->lower = 70; BK->upper = 95; BK->gapbound = 60; // defaults from awars
    BK->lower    = 40; BK->upper = 70; BK->gapbound = 40;

    srand(100);
    for (int loop = 0; loop<5; ++loop) {
        unsigned seed = rand();

        srand(seed);

        ED4_char_table tab(seqlen);

        // build some seq
        for (int c = 0; c<seqlen; ++c) seq[c] = alphabeth[rand()%alphabeth_size];
        seq[seqlen]                           = 0;

        TEST_EXPECT_EQUAL(strlen(seq), size_t(seqlen));

        const int  add_count = 300;
        char      *added_seqs[add_count];

        // add seq multiple times
        for (int a = 0; a<add_count; ++a) {
            tab.add(seq, seqlen);
            added_seqs[a] = strdup(seq);

            // modify 1 character in seq:
            int sidx  = rand()%seqlen;
            int aidx  = rand()%alphabeth_size;
            seq[sidx] = alphabeth[aidx];
        }

        // build consensi (just check regression)
        {
            char *consensus = tab.build_consensus_string();
            switch (seed) {
                case 677741240: TEST_EXPECT_EQUAL(consensus, "k-s-NW..aWu.NnWYa.R.mKcNK.c.rn"); break;
                case 721151648: TEST_EXPECT_EQUAL(consensus, "aNnn..K..-gU.RW-SNcau.WNNacn.u"); break;
                case 345295160: TEST_EXPECT_EQUAL(consensus, "yy-g..kMSn...NucyNny.Rnn.-Ng.k"); break;
                case 346389111: TEST_EXPECT_EQUAL(consensus, ".unAn.y.nN.kc-cS.RauNm..Sa-kY."); break;
                case 367171911: TEST_EXPECT_EQUAL(consensus, "na.NanNunc-.-NU.aYgn-nng-nWanM"); break;
                default: TEST_EXPECT_EQUAL(consensus, "undef");
            }
            free(consensus);
        }

        for (int a = 0; a<add_count; a++) {
            tab.sub(added_seqs[a], seqlen);
            freenull(added_seqs[a]);
        }
        {
            char *consensus = tab.build_consensus_string();
            TEST_EXPECT_EQUAL(consensus, "??????????????????????????????"); // check tab is empty
            free(consensus);
        }
    }

    delete BK; BK = 0;
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
    ED4_char_table::initial_setup(gapChars, GB_AT_RNA);
    ED4_init_is_align_character(gapChars);

    TEST_REJECT(BK); BK = new ConsensusBuildParams;

    ED4_char_table tab(seqlen);
    for (int s = 0; s<sequenceCount; ++s) {
        e4_assert(strlen(sequence[s]) == seqlen);
        tab.add(sequence[s], seqlen);
    }

    for (int c = 0; c<consensusCount; ++c) {
        TEST_ANNOTATE(GBS_global_string("c=%i", c));
        switch (c) {
            case 0: break;                                                      // use default settings
            case 1: BK->countgaps   = false; break;                             // dont count gaps
            case 2: BK->considbound = 26; BK->lower = 0; BK->upper = 75; break; // settings from #663
            case 3: BK->countgaps   = true; BK->gapbound = 70; break;
            case 4: BK->considbound = 20; break;
            default: e4_assert(0); break;                                       // missing
        }

        char *consensus = tab.build_consensus_string();
        TEST_EXPECT_EQUAL(consensus, expected_consensus[c]);
        free(consensus);
    }

    delete BK; BK = 0;
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
    ED4_char_table::initial_setup(gapChars, GB_AT_AA);
    ED4_init_is_align_character(gapChars);

    TEST_REJECT(BK); BK = new ConsensusBuildParams;

    ED4_char_table tab(seqlen);
    for (int s = 0; s<sequenceCount; ++s) {
        e4_assert(strlen(sequence[s]) == seqlen);
        tab.add(sequence[s], seqlen);
    }

    for (int c = 0; c<consensusCount; ++c) {
        TEST_ANNOTATE(GBS_global_string("c=%i", c));
        switch (c) {
            case 0: break;                                                      // use default settings
            case 1: BK->countgaps   = false; break;                             // dont count gaps
            case 2: BK->considbound = 26; BK->lower = 0; BK->upper = 75; break; // settings from #663
            case 3: BK->countgaps   = true; BK->gapbound = 70; break;
            case 4: BK->considbound = 20; break;
            default: e4_assert(0); break;                                       // missing
        }

        char *consensus = tab.build_consensus_string();
        TEST_EXPECT_EQUAL(consensus, expected_consensus[c]);
        free(consensus);
    }

    delete BK; BK = 0;
}

void TEST_bases_table() {
    const int LEN  = 20;
    const int OFF1 = 6;
    const int OFF2 = 7; // adjacent to OFF1

    ED4_bases_table short1(LEN), short2(LEN);
    for (int i = 0; i<100; ++i) short1.inc_short(OFF1);
    for (int i = 0; i<70;  ++i) short1.inc_short(OFF2);
    for (int i = 0; i<150; ++i) short2.inc_short(OFF1);
    for (int i = 0; i<80;  ++i) short2.inc_short(OFF2);

    ED4_bases_table long1(LEN), long2(LEN);
    long1.expand_table_entry_size();
    long2.expand_table_entry_size();
    for (int i = 0; i<2000; ++i) long1.inc_long(OFF1);
    for (int i = 0; i<2500; ++i) long2.inc_long(OFF1);

    ED4_bases_table shortsum(LEN);
    shortsum.add(short1, 0, LEN-1); TEST_EXPECT_EQUAL(shortsum[OFF1], 100); TEST_EXPECT_EQUAL(shortsum[OFF2],  70);
    shortsum.add(short2, 0, LEN-1); TEST_EXPECT_EQUAL(shortsum[OFF1], 250); TEST_EXPECT_EQUAL(shortsum[OFF2], 150);
    shortsum.sub(short1, 0, LEN-1); TEST_EXPECT_EQUAL(shortsum[OFF1], 150); TEST_EXPECT_EQUAL(shortsum[OFF2],  80);

    shortsum.sub_and_add(short1, short2, PosRange(0, LEN-1)); TEST_EXPECT_EQUAL(shortsum[OFF1], 200); TEST_EXPECT_EQUAL(shortsum[OFF2], 90);

    // shortsum.add(long1, 0, LEN-1); // invalid operation -> terminate

    ED4_bases_table longsum(LEN);
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
