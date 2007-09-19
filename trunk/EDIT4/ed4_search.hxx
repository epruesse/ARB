// =============================================================== //
//                                                                 //
//   File      : ed4_search.hxx                                    //
//   Purpose   :                                                   //
//   Time-stamp: <Sat Sep/01/2007 10:00 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de)                   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ED4_SEARCH_HXX
#define ED4_SEARCH_HXX

typedef enum {
    ED4_SC_CASE_SENSITIVE,
    ED4_SC_CASE_INSENSITIVE
} ED4_SEARCH_CASE;

typedef enum {
    ED4_ST_T_NOT_EQUAL_U,
    ED4_ST_T_EQUAL_U
} ED4_SEARCH_TU;

typedef enum {
    ED4_SG_CONSIDER_GAPS,
    ED4_SG_IGNORE_GAPS
} ED4_SEARCH_GAPS;

void       ED4_search(AW_window *aww, AW_CL searchDescriptor);
GB_ERROR   ED4_repeat_last_search(void);
AW_window *ED4_create_search_window(AW_root *root, AW_CL type);
void       ED4_create_search_awars(AW_root *root);

// --------------------------------------------------------------------------------

#define SEARCH_PATTERNS 9
#define MAX_MISMATCHES  5

typedef enum
    {
    ED4_USER1_PATTERN,
    ED4_USER2_PATTERN,
    ED4_PROBE_PATTERN,
    ED4_PRIMER1_PATTERN,
    ED4_PRIMER2_PATTERN,
    ED4_PRIMER3_PATTERN,
    ED4_SIG1_PATTERN,
    ED4_SIG2_PATTERN,
    ED4_SIG3_PATTERN,
    ED4_ANY_PATTERN

} ED4_SearchPositionType;

extern const char *ED4_SearchPositionTypeId[];

inline int ED4_encodeSearchDescriptor(int direction, ED4_SearchPositionType pattern)
{
    e4_assert(direction==-1 || direction==1);
    e4_assert(pattern>=0 && pattern<(SEARCH_PATTERNS+1));
    return (direction==1) + pattern*2;
}

// #define TEST_SEARCH_POSITION



class ED4_SearchPosition // one found position
{
    int start_pos, end_pos;
    int mismatch[MAX_MISMATCHES]; // contains positions of mismatches (or -1)
    ED4_SearchPositionType whatsFound;
    GB_CSTR comment;
    ED4_SearchPosition *next;

    static char *lastShownComment;

    int cmp(const ED4_SearchPosition& sp2) const
    {
        int c = start_pos - sp2.get_start_pos();
        if (!c) c = end_pos - sp2.get_end_pos();
        return c;
    }

public:

    ED4_SearchPosition(int sp, int ep, ED4_SearchPositionType wf, GB_CSTR found_comment, int mismatches[MAX_MISMATCHES]);
    ~ED4_SearchPosition() { delete next; }

    ED4_SearchPosition(const ED4_SearchPosition& other); // copy-ctor ('next' is always zero)

    ED4_SearchPosition *insert(ED4_SearchPosition *toAdd);
    ED4_SearchPosition *remove(ED4_SearchPositionType typeToRemove);

    ED4_SearchPosition *get_next() const          { return next; }
    int get_start_pos() const                     { return start_pos; }
    int get_end_pos() const                       { return end_pos; }
    ED4_SearchPositionType get_whatsFound() const { return whatsFound; }
    GB_CSTR get_comment() const;

    const int *getMismatches() const              { return mismatch; }

    int containsPos(int pos) const { return start_pos<=pos && end_pos>=pos; }

    ED4_SearchPosition *get_next_at(int pos) const;

#ifdef TEST_SEARCH_POSITION
    int ok() const;
#endif
};

class ED4_sequence_terminal;

class ED4_SearchResults // list head
{
    int arraySize;              // ==0 -> 'first' is a list
    // >0 -> 'array' is an array of 'ED4_SearchPosition*' with 'arraySize' elements

    ED4_SearchPosition *first;  // ==0 -> no results, this list is sorted by start_pos, end_pos
    ED4_SearchPosition **array; // ==0 -> no results, same sorting as 'first'
    int timeOf[SEARCH_PATTERNS];

    static int timeOfLastSearch[SEARCH_PATTERNS]; // timestamp used at last search
    static int timeOfNextSearch[SEARCH_PATTERNS]; // timestamp used at next search
    static int shown[SEARCH_PATTERNS]; // copy of ED4_AWAR_xxx_SEARCH_SHOW
    static int bufferSize;
    static char *buffer; // buffer for buildColorString
    static int initialized;

    ED4_SearchResults(const ED4_SearchResults&) { e4_assert(0); }

    int is_list() const { return arraySize==0; }
    int is_array() const { return arraySize>0; }

    void to_array() const;      // ensures that result is in array-format (used to improve search performance)
    void to_list() const;       // ensures that result is in list-format (used to improve insert performance)

public:

    ED4_SearchResults();
    ~ED4_SearchResults();

    void search(const ED4_sequence_terminal *seq_terminal);
    void addSearchPosition(ED4_SearchPosition *pos);

    ED4_SearchPosition *get_first() const { return first; }
    ED4_SearchPosition *get_first_at(ED4_SearchPositionType type, int start, int end) const;
    ED4_SearchPosition *get_first_starting_after(ED4_SearchPositionType type, int pos, int mustBeShown) const;
    ED4_SearchPosition *get_last_starting_before(ED4_SearchPositionType type, int pos, int mustBeShown) const;
    ED4_SearchPosition *get_shown_at(int pos) const;

    static void setNewSearch(ED4_SearchPositionType type);
    void searchAgain();

    char *buildColorString(const ED4_sequence_terminal *seq_terminal, int start, int end);
};


#else
#error ed4_search.hxx included twice
#endif // ED4_SEARCH_HXX

