#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_window.hxx>

#include <awt_iupac.hxx>

#include "ed4_class.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_awars.hxx"
#include "ed4_tools.hxx"


// --------------------------------------------------------------------------------
// 		ED4_folding_line::
// --------------------------------------------------------------------------------

ED4_folding_line::ED4_folding_line()
{
    memset((char *)this,0,sizeof(*this));
}
ED4_folding_line::~ED4_folding_line()
{
}

// --------------------------------------------------------------------------------
// 		ED4_bases_table::
// --------------------------------------------------------------------------------

#ifdef DEBUG
//# define COUNT_BASES_TABLE_SIZE
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
            for (i=start; i<=end; i++) {
                set_elem_short(i, get_elem_short(i)+other.get_elem_long(i));
            }
        }
    }
    else {
        if (other.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            for (i=start; i<=end; i++) {
                set_elem_long(i, get_elem_long(i)+other.get_elem_short(i));
            }
        }
        else {
            for (i=start; i<=end; i++) {
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
            for (i=start; i<=end; i++) {
                set_elem_short(i, get_elem_short(i)-other.get_elem_long(i));
            }
        }
    }
    else {
        if (other.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            for (i=start; i<=end; i++) {
                set_elem_long(i, get_elem_long(i)-other.get_elem_short(i));
            }
        }
        else {
            for (i=start; i<=end; i++) {
                set_elem_long(i, get_elem_long(i)-other.get_elem_long(i));
            }
        }
    }
}
void ED4_bases_table::sub_and_add(const ED4_bases_table& Sub, const ED4_bases_table& Add, int start, int end)
{
    e4_assert(no_of_entries==Sub.no_of_entries);
    e4_assert(no_of_entries==Add.no_of_entries);
    e4_assert(start>=0);
    e4_assert(end<no_of_entries);
    e4_assert(start<=end);

    int i;
    if (table_entry_size==SHORT_TABLE_ELEM_SIZE) {
        if (Sub.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            if (Add.table_entry_size==SHORT_TABLE_ELEM_SIZE)  {
                for (i=start; i<=end; i++) {
                    set_elem_short(i, get_elem_short(i)-Sub.get_elem_short(i)+Add.get_elem_short(i));
                }
            }
            else {
                for (i=start; i<=end; i++) {
                    set_elem_short(i, get_elem_short(i)-Sub.get_elem_short(i)+Add.get_elem_long(i));
                }
            }
        }
        else {
            if (Add.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
                for (i=start; i<=end; i++) {
                    set_elem_short(i, get_elem_short(i)-Sub.get_elem_long(i)+Add.get_elem_short(i));
                }
            }
            else {
                for (i=start; i<=end; i++) {
                    set_elem_short(i, get_elem_short(i)-Sub.get_elem_long(i)+Add.get_elem_long(i));
                }
            }
        }
    }
    else {
        if (Sub.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
            if (Add.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
                for (i=start; i<=end; i++) {
                    set_elem_long(i, get_elem_long(i)-Sub.get_elem_short(i)+Add.get_elem_short(i));
                }
            }
            else {
                for (i=start; i<=end; i++) {
                    set_elem_long(i, get_elem_long(i)-Sub.get_elem_short(i)+Add.get_elem_long(i));
                }
            }
        }
        else {
            if (Add.table_entry_size==SHORT_TABLE_ELEM_SIZE) {
                for (i=start; i<=end; i++) {
                    set_elem_long(i, get_elem_long(i)-Sub.get_elem_long(i)+Add.get_elem_short(i));
                }
            }
            else {
                for (i=start; i<=end; i++) {
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
{
    memset((char*)this,0,sizeof(*this));
    table_entry_size = SHORT_TABLE_ELEM_SIZE;
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
                //memset(new_table+no_of_entries, 0, growth*sizeof(*new_table));
            }

            delete [] no_of_bases.shortTable;
            no_of_bases.shortTable = new_table;
        }
        else {
            int *new_table = new int[new_length+1];

            memcpy(new_table, no_of_bases.longTable, min_length*sizeof(*new_table));
            new_table[new_length] = no_of_bases.longTable[no_of_entries];
            if (growth>0) {
                for (int e=no_of_entries; e<new_length; ++e) new_table[e] = default_entry;
                //memset(new_table+no_of_entries, 0, growth*sizeof(*new_table));
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

#if defined(ASSERTION_USED)

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

/* --------------------------------------------------------------------------------
   Build consensus
   -------------------------------------------------------------------------------- */

// we make static copies of the awars to avoid performance breakdown (BK_up_to_date is changed by callback ED4_consensus_definition_changed)

static int BK_up_to_date = 0;
static int BK_countgaps;
static int BK_gapbound;
static int BK_group;
static int BK_considbound;
static int BK_upper;
static int BK_lower;

void ED4_consensus_definition_changed(AW_root*, AW_CL,AW_CL) {
    ED4_terminal *terminal = ED4_ROOT->root_group_man->get_first_terminal();

    e4_assert(terminal);
    while (terminal) {
        if (terminal->parent->parent->flag.is_consensus) {
            terminal->set_refresh();
            terminal->parent->refresh_requested_by_child();
        }
        terminal = terminal->get_next_terminal();
    }

    BK_up_to_date = 0;
    // ED4_ROOT->root_group_man->Show();
    ED4_ROOT->refresh_all_windows(1);
}

static ED4_returncode toggle_consensus_display(void **show, void **, ED4_base *base) {
    if (base->flag.is_consensus) {
        ED4_manager *consensus_man = base->to_manager();
        ED4_spacer_terminal *spacer = consensus_man->parent->get_defined_level(ED4_L_SPACER)->to_spacer_terminal();

        if (show) {
            consensus_man->make_children_visible();
            spacer->extension.size[HEIGHT] = SPACERHEIGHT;
        }
        else {
            consensus_man->hide_children();

            ED4_group_manager *group_man = consensus_man->get_parent(ED4_L_GROUP)->to_group_manager();
            spacer->extension.size[HEIGHT] = (group_man->dynamic_prop&ED4_P_IS_FOLDED) ? SPACERNOCONSENSUSHEIGHT : SPACERHEIGHT;
        }
    }

    return ED4_R_OK;
}

void ED4_consensus_display_changed(AW_root *root, AW_CL, AW_CL) {
    int show = root->awar(ED4_AWAR_CONSENSUS_SHOW)->read_int();
    ED4_ROOT->root_group_man->route_down_hierarchy((void**)show, 0, toggle_consensus_display);
    ED4_ROOT->refresh_all_windows(1);
}

char *ED4_char_table::build_consensus_string(int left_idx, int right_idx, char *fill_id) const
// consensus is only built in intervall
// Note : Always check that consensus behavior is identical to that used in CON_evaluatestatistic()
{
    int i;

    if (!BK_up_to_date) {
        BK_countgaps = ED4_ROOT->aw_root->awar(ED4_AWAR_CONSENSUS_COUNTGAPS)->read_int();
        BK_gapbound = ED4_ROOT->aw_root->awar(ED4_AWAR_CONSENSUS_GAPBOUND)->read_int();
        BK_group = ED4_ROOT->aw_root->awar(ED4_AWAR_CONSENSUS_GROUP)->read_int();
        BK_considbound = ED4_ROOT->aw_root->awar(ED4_AWAR_CONSENSUS_CONSIDBOUND)->read_int();
        BK_upper = ED4_ROOT->aw_root->awar(ED4_AWAR_CONSENSUS_UPPER)->read_int();
        BK_lower = ED4_ROOT->aw_root->awar(ED4_AWAR_CONSENSUS_LOWER)->read_int();
        BK_up_to_date = 1;
    }

    if (!fill_id) {
        long entr = size();

        fill_id = new char[entr+1];
        fill_id[entr] = 0;
    }

    e4_assert(right_idx<size());
    if (right_idx==-1 || right_idx>=size()) right_idx = size()-1;

    char *consensus_string = fill_id;

#define PERCENT(part, all)	((100*(part))/(all))
#define MAX_BASES_TABLES 41     //25

    e4_assert(used_bases_tables<=MAX_BASES_TABLES);	// this is correct for DNA/RNA -> build_consensus_string() must be corrected for AMI/PRO

    if (sequences) {
        for (i=left_idx; i<=right_idx; i++) {
            int bases = 0; // count of all bases together
            int base[MAX_BASES_TABLES]; // count of specific base
            int j;
            int max_base = -1; // maximum count of specific base
            int max_j = -1; // index of this base

            for (j=0; j<used_bases_tables; j++) {
                base[j] = linear_table(j)[i];
                if (!ADPP_IS_ALIGN_CHARACTER(index_to_upperChar(j))) {
                    bases += base[j];
                    if (base[j]>max_base) { // search for most used base
                        max_base = base[j];
                        max_j = j;
                    }
                }
            }

            int gaps = sequences-bases; // count of gaps

            // What to do with gaps?

            if (gaps == sequences) {
                consensus_string[i] = '=';
            }
            else if (BK_countgaps && PERCENT(gaps, sequences)>=BK_gapbound) {
                consensus_string[i] = '-';
            }
            else {
                // Simplify using IUPAC :

                char kchar;     // character for consensus
                int  kcount;    // count this character

                if (BK_group) { // group -> iupac
                    if (IS_NUCLEOTIDE()) {
                        int no_of_bases = 0; // count of different bases used to create iupac
                        char used_bases[MAX_BASES_TABLES+1]; // string containing those bases

                        kcount = 0;
                        for (j=0; j<used_bases_tables; j++) {
                            int bchar = index_to_upperChar(j);

                            if (!ADPP_IS_ALIGN_CHARACTER(bchar)) {
                                if (PERCENT(base[j], sequences)>=BK_considbound) {
                                    used_bases[no_of_bases++] = index_to_upperChar(j);
                                    kcount += base[j];
                                }
                            }
                        }
                        used_bases[no_of_bases] = 0;
                        kchar = ED4_encode_iupac(used_bases, ED4_ROOT->alignment_type);
                    }
                    else {
                        e4_assert(IS_AMINO());

                        int group_count[IUPAC_GROUPS];

                        for (j=0; j<IUPAC_GROUPS; j++) {
                            group_count[j] = 0;
                        }
                        for (j=0; j<used_bases_tables; j++) {
                            unsigned char bchar = index_to_upperChar(j);

                            if (!ADPP_IS_ALIGN_CHARACTER(bchar)) {
                                if (PERCENT(base[j], sequences)>=BK_considbound) {
                                    group_count[AWT_iupac_group[toupper(bchar)-'A']] += base[j];
                                }
                            }
                        }

                        kcount = 0;
                        int bestGroup = 0;
                        for (j=0; j<IUPAC_GROUPS; j++) {
                            if (group_count[j]>kcount) {
                                bestGroup = j;
                                kcount = group_count[j];
                            }
                        }

                        kchar = "?ADHIF"[bestGroup];
                    }
                }
                else {
                    e4_assert(max_base); // expect at least one base to occur  
                    kchar = index_to_upperChar(max_j);
                    kcount = max_base;
                }

                // show as upper or lower case ?

                int percent = PERCENT(kcount, sequences);

                if (percent>=BK_upper) {
                    consensus_string[i] = kchar;
                }
                else if (percent>=BK_lower){
                    consensus_string[i] = tolower(kchar);
                }
                else {
                    consensus_string[i] = '.';
                }
            }
        }
    }
    else {
        for (i=left_idx; i<=right_idx; i++) {
            consensus_string[i] = '?';
        }
    }


#undef PERCENT

    return consensus_string;
}

// --------------------------------------------------------------------------------
//		ED4_char_table::
// --------------------------------------------------------------------------------

// bool ED4_char_table::tables_are_valid = true;
bool ED4_char_table::initialized = false;
unsigned char ED4_char_table::char_to_index_tab[MAXCHARTABLE];
unsigned char *ED4_char_table::upper_index_chars = 0;
unsigned char *ED4_char_table::lower_index_chars = 0;
int ED4_char_table::used_bases_tables = 0;

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

ED4_char_table::ED4_char_table(int maxseqlength)
{
    memset((char*)this,0,sizeof(*this));

    if (!initialized) {
        const char *groups = 0;
        char *align_string = ED4_ROOT->aw_root->awar_string(ED4_AWAR_GAP_CHARS)->read_string();
        used_bases_tables = strlen(align_string);

        if (IS_NUCLEOTIDE()) {
            if (IS_RNA()) {
                groups = "A,C,G,TU,MRWSYKVHDBN,";
            }
            else {
                groups = "A,C,G,UT,MRWSYKVHDBN,";
            }
        }
        else {
            e4_assert(IS_AMINO());
            groups = "P,A,G,S,T,Q,N,E,D,B,Z,H,K,R,L,I,V,M,F,Y,W,";
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

        while (1) {
            char c = *align_string++;
            if (!c) break;
            set_char_to_index(c, idx++);
        }

        e4_assert(idx==used_bases_tables);
        initialized = true;
    }

    bases_table = new ED4_bases_table_ptr[used_bases_tables];

    int i;
    for (i=0; i<used_bases_tables; i++) {
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

int ED4_char_table::changed_range(const ED4_char_table& other, int *startPtr, int *endPtr) const
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

            *startPtr = start;
            *endPtr = end;
            e4_assert(start<=end);
            return 1;
        }
    }
    return 0;
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

void ED4_char_table::sub_and_add(const ED4_char_table& Sub, const ED4_char_table& Add)
{
    int start, end;

    test();

    if (Sub.ignore) {
        e4_assert(Add.ignore);
        return;
    }

    Sub.test();
    Add.test();

    if (Sub.changed_range(Add, &start, &end)) {
        prepare_to_add_elements(Add.added_sequences()-Sub.added_sequences());
        sub_and_add(Sub, Add, start, end);
    }

    test();
}
void ED4_char_table::sub_and_add(const ED4_char_table& Sub, const ED4_char_table& Add, int start, int end)
{
    test();
    Sub.test();
    Add.test();

    e4_assert(!Sub.ignore && !Add.ignore);
    e4_assert(start<=end);

    int i;
    for (i=0; i<used_bases_tables; i++) {
        linear_table(i).sub_and_add(Sub.linear_table(i), Add.linear_table(i), start, end);
    }

    test();
}

int ED4_char_table::changed_range(const char *string1, const char *string2, int min_len, int *start, int *end)
{
    const unsigned long *range1 = (const unsigned long *)string1;
    const unsigned long *range2 = (const unsigned long *)string2;
    const int step = sizeof(*range1);
    int l = min_len/step;
    int r = min_len%step;
    int i;
    int j;
    int k;

    for (i=0; i<l; i++) {	// search wordwise (it's faster)
        if (range1[i] != range2[i]) {
            k = i*step;
            for (j=0; j<3; j++) {
                if (string1[k+j] != string2[k+j]) {
                    *start = *end = k+j;
                    break;
                }
            }

            break;
        }
    }

    k = l*step;
    if (i==l) {		// no difference found in word -> look at rest
        for (j=0; j<r; j++) {
            if (string1[k+j] != string2[k+j]) {
                *start = *end = k+j;
                break;
            }
        }

        if (j==r) {	// the strings are equal
            return 0;
        }
    }

    for (j=r-1; j>=0; j--) {	// search rest backwards
        if (string1[k+j] != string2[k+j]) {
            *end = k+j;
            break;
        }
    }

    if (j==-1) {		// not found in rest -> search words backwards
        int m = *start/step;
        for (i=l-1; i>=m; i--) {
            if (range1[i] != range2[i]) {
                k = i*step;
                for (j=3; j>=0; j--) {
                    if (string1[k+j] != string2[k+j]) {
                        *end = k+j;
                        break;
                    }
                }
                break;
            }
        }
    }

    e4_assert(*start<=*end);
    return 1;
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
        for (; i<sz; i++) {
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
        for (; i<sz; i++) {
            t.dec_long(i);
        }
    }

    sequences--;

    test();
}

void ED4_char_table::sub_and_add(const char *old_string, const char *new_string, int start, int end)
{
    test();
    e4_assert(start<=end);

    int i;
    ED4_bases_table& t = table('.');

    if (get_table_entry_size()==SHORT_TABLE_ELEM_SIZE) {
        for (i=start; i<=end; i++) {
            unsigned char o = old_string[i];
            unsigned char n = new_string[i];

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

void ED4_char_table::change_table_length(int new_length)
{
    int c;

//     int bases1, gaps1;
//     bases_and_gaps_at(0, &bases1, &gaps1);

//     int table_thickness = bases1+gaps1;
//     bool gaps_inserted = false;

    // test(); fails because MAXSEQUENCECHARACTERLENGTH is already incremented

    for (c=0; c<used_bases_tables; c++) {
//         int default_entry = 0;
//         if (!gaps_inserted && ADPP_IS_ALIGN_CHARACTER(upper_index_chars[c])) {
//             default_entry = table_thickness;
//             gaps_inserted = true;
//         }
//         linear_table(c).change_table_length(new_length, default_entry);
        linear_table(c).change_table_length(new_length, 0);
    }

    test();
}

//  --------------
//      tests:
//  --------------

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
            for (j=i+1; j<MAXSEQUENCECHARACTERLENGTH; j++) {	// test if we find any bases till end of table !!!

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

//  ------------------------------------------
//      void ED4_char_table::test() const
//  ------------------------------------------
void ED4_char_table::test() const {
    if (!ok()) {
        fprintf(stderr, "ED4_char_table::test() failed");
        GB_CORE;
    }
}

#endif // TEST_CHAR_TABLE_INTEGRITY







