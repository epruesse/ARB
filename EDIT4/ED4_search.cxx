#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_keysym.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_global.hxx>
#include <awtc_fast_aligner.hxx>
#include <awt.hxx>
#include <awt_config_manager.hxx>

#include "ed4_awars.hxx"
#include "ed4_class.hxx"
#include "ed4_tools.hxx"

static int result_counter      = 0;
static int max_allowed_results = 10000;
static int ignore_more_results = false;

const char *ED4_SearchPositionTypeId[SEARCH_PATTERNS+1] =
{
    "User1", "User2",
    "Probe",
    "Primer (local)", "Primer (region)", "Primer (global)",
    "Signature (local)", "Signature (region)", "Signature (global)",
    "Any"
};

// --------------------------------------------------------------------------------

typedef struct S_SearchAwarList // contains names of awars
{
    const char *pattern,
        *min_mismatches,
        *max_mismatches,
        *case_sensitive,
        *tu,
        *pat_gaps,
        *seq_gaps,
        *reverse,
        *complement,
        *exact,
        *show,
        *openFolded,
        *autoJump;

} *SearchAwarList;

class SearchSettings
{
    char            *pattern;
    int              min_mismatches;
    int              max_mismatches;
    ED4_SEARCH_CASE  case_sensitive;
    ED4_SEARCH_TU    tu;
    ED4_SEARCH_GAPS  pat_gaps;
    ED4_SEARCH_GAPS  seq_gaps;
    int              reverse;
    int              complement;
    int              exact;
    int              open_folded;
    int              autoJump;

    SearchSettings(const SearchSettings&) { e4_assert(0); } // forbidden

public:

    void update(SearchAwarList awarList)
    {
        AW_root *root = ED4_ROOT->aw_root;

        free(pattern);
        pattern        = root->awar(awarList->pattern)->read_string();
        min_mismatches = root->awar(awarList->min_mismatches)->read_int();
        max_mismatches = root->awar(awarList->max_mismatches)->read_int();
        case_sensitive = ED4_SEARCH_CASE(root->awar(awarList->case_sensitive)->read_int());
        tu             = ED4_SEARCH_TU(root->awar(awarList->tu)->read_int());
        pat_gaps       = ED4_SEARCH_GAPS(root->awar(awarList->pat_gaps)->read_int());
        seq_gaps       = ED4_SEARCH_GAPS(root->awar(awarList->seq_gaps)->read_int());
        open_folded    = root->awar(awarList->openFolded)->read_int();
        autoJump       = root->awar(awarList->autoJump)->read_int();
        reverse        = root->awar(awarList->reverse)->read_int();
        complement     = root->awar(awarList->complement)->read_int();
        exact          = root->awar(awarList->exact)->read_int();

        if (complement) {
            if (IS_AMINO()) {
                complement = 0;
                root->awar(awarList->complement)->write_int(0);
                aw_message(GBS_global_string("Search for complement is not supported for this alignment type"), "Disable");
            }
        }
    }

    SearchSettings(SearchAwarList awarList)     { pattern = 0; update(awarList); }
    ~SearchSettings()                           { delete pattern; }

    GB_CSTR get_pattern() const                 { return pattern; }
    int get_min_mismatches() const              { return min_mismatches; }
    int get_max_mismatches() const              { return max_mismatches; }
    ED4_SEARCH_CASE get_case_sensitive() const  { return case_sensitive; }
    ED4_SEARCH_TU get_tu() const                { return tu; }
    ED4_SEARCH_GAPS get_pat_gaps() const        { return pat_gaps; }
    ED4_SEARCH_GAPS get_seq_gaps() const        { return seq_gaps; }
    int get_open_folded() const                 { return open_folded; }
    int get_autoJump() const                    { return autoJump; }
    int get_reverse() const                     { return reverse; }
    int get_complement() const                  { return complement; }
    int get_exact() const                       { return exact; }
};

// --------------------------------------------------------------------------------

class SearchTree;
typedef void (*reportMatch)(int start, int end, GB_CSTR comment, int mismatches[MAX_MISMATCHES]);

class SearchTreeNode
{
    char            c;          // character
    SearchTreeNode *son;        // son != 0 (exception: FOUND)
    SearchTreeNode *brother;
    char           *comment;    // 0 or comment given in search pattern

    static SearchTreeNode  FOUND;
    static int             start_offset;
    static reportMatch     report;
    static int             min_mismatches;
    static int             max_mismatches;
    static int            *uni2real; // transform unified positions to real sequence positions

    SearchTreeNode(const SearchTreeNode &) { e4_assert(0); } // forbidden

public:

    SearchTreeNode(GB_CSTR pattern, GB_CSTR comment);
    ~SearchTreeNode();

    SearchTreeNode *insert_unified_pattern(GB_CSTR pattern, GB_CSTR pattern_comment);
    void findMatches(int off, GB_CSTR seq, int len, int mismatches, int mismatch_list[MAX_MISMATCHES]);

    // you must call the following functions before calling findMatches():

    static void set_start_offset(int off)               { start_offset = off; }
    static void set_report(reportMatch r, int *u2r)     { report = r; uni2real = u2r; }
    static void set_mismatches(int minMis, int maxMis)  { min_mismatches = minMis; max_mismatches = maxMis; }
};

// --------------------------------------------------------------------------------

SearchTreeNode  SearchTreeNode::FOUND(0, 0);
int             SearchTreeNode::start_offset;
reportMatch     SearchTreeNode::report;
int             SearchTreeNode::min_mismatches;
int             SearchTreeNode::max_mismatches;
int            *SearchTreeNode::uni2real;

SearchTreeNode::SearchTreeNode(GB_CSTR pattern, GB_CSTR pattern_comment)
{
    comment = 0;
    if (pattern) {
        e4_assert(pattern[0]);
        c = pattern[0];
        if (pattern[1]) {
            son = new SearchTreeNode(pattern+1, pattern_comment);
        }
        else {
            son = &FOUND;
            comment = GB_strdup(pattern_comment);
        }
    }
    else {
        e4_assert(this==&FOUND);
        c = 0;
        son = 0;
        comment = GB_strdup(pattern_comment);
    }
    brother = 0;
}

SearchTreeNode::~SearchTreeNode()
{
    if (brother!=&FOUND) delete brother;
    if (son!=&FOUND) delete son;
    delete comment;
}


SearchTreeNode *SearchTreeNode::insert_unified_pattern(GB_CSTR pattern, GB_CSTR pattern_comment)
{
    if (!this) {
        if (pattern[0]) {
            return new SearchTreeNode(pattern, pattern_comment);
        }

        return &FOUND;
    }

    if (this==&FOUND) {
        if (pattern[0]) {
            SearchTreeNode *neu = new SearchTreeNode(pattern, pattern_comment);

            neu->brother = &FOUND;
            return neu;
        }
        return &FOUND;
    }

    e4_assert(c);

    if (pattern[0]) { // pattern contains sth.
        if (c==pattern[0]) {
            e4_assert(son);
            son = son->insert_unified_pattern(pattern+1, pattern_comment);
        }
        else {
            if (brother) {
                brother = brother->insert_unified_pattern(pattern, pattern_comment);
            }
            else {
                brother = new SearchTreeNode(pattern, pattern_comment);
            }
        }
    }
    else { // empty pattern -> insert FOUND
        if (brother) {
            if (brother!=&FOUND) {
                brother = brother->insert_unified_pattern(pattern, pattern_comment);
            }
        }
        else {
            brother = &FOUND;
        }
    }

    return this;
}

void SearchTreeNode::findMatches(int off, GB_CSTR seq, int len, int mismatches, int mismatch_list[MAX_MISMATCHES])
{
    if (len) {
        int matches = c=='?' || c==seq[0];
        int use_mismatch = 0;

        if (!matches && mismatches<max_mismatches) {
            int c_is_gap = c=='-' || c=='.';
            int seq_is_gap = seq[0]=='-' || seq[0]=='.';

            if (c_is_gap==seq_is_gap) {
                mismatch_list[mismatches] = uni2real[off];
                mismatches++;
                use_mismatch = 1;
                matches = 1;
            }
        }

        if (matches) {
            if (son==&FOUND) {
                if (mismatches>=min_mismatches) {
                    report(uni2real[start_offset], uni2real[off], comment, mismatch_list);
                }
            }
            else {
                son->findMatches(off+1, seq+1, len-1, mismatches, mismatch_list);
            }

            if (use_mismatch) {
                mismatches--;
                mismatch_list[mismatches] = -1;
            }
        }
    }

    if (brother==&FOUND) {
        if (mismatches>=min_mismatches) {
            report(uni2real[start_offset], uni2real[off-1], comment, mismatch_list);
        }
    }
    else if (brother) {
        brother->findMatches(off, seq, len, mismatches, mismatch_list);
    }
}

// --------------------------------------------------------------------------------

class SearchTree
{
    const SearchSettings *sett;
    SearchTreeNode *root;
    unsigned char unified[256];
    int shortestPattern;

    static char unify_char(char c, int case_sensitive, int T_equal_U);

    char *unify_str(const char *data, int len, ED4_SEARCH_GAPS gaps, int *new_len, int **uni2real);
    char *unify_pattern(const char *pattern, int *new_len);
    char *unify_sequence(const char *sequence, int len, int *new_len, int **uni2real);

    SearchTree(const SearchTree &); // forbidden

public:

    SearchTree(const SearchSettings *s);
    ~SearchTree();

    void findMatches(const char *seq, int len, reportMatch report);
    int get_shortestPattern() const { return shortestPattern; }
};

// --------------------------------------------------------------------------------

static char *shortenString(char *s)
{
    char *end = strchr(s, '\0');

    while (end>s && isspace(end[-1])) {
        *--end = 0;
    }
    while (isspace(s[0])) {
        s++;
    }

    return s;
}

static void splitTokComment(char **tok, char **commentP)
{
    char *num = strchr(*tok, '#');

    if (num) {
        num[0] = 0;
        *commentP = shortenString(num+1);
    }
    else {
        *commentP = 0;
    }

    *tok = shortenString(*tok);
}

static char *appendComment(const char *s1, int l1, const char *s2) {
    if (s1) {
        int l2 = strlen(s2);
        char *s = (char*)malloc(l1+1+l2+1);

        sprintf(s, "%s %s", s1, s2);
        return s;
    }

    return 0;
}

SearchTree::SearchTree(const SearchSettings *s)
{
    sett = s;
    root = 0;
    shortestPattern = INT_MAX;

    {
        int i;
        int case_sensitive = (sett->get_case_sensitive()==ED4_SC_CASE_SENSITIVE);
        int T_equal_U = (sett->get_tu()==ED4_ST_T_EQUAL_U);

        for (i=0; i<256; i++) {
            unified[i] = unify_char(i, case_sensitive, T_equal_U);
        }
    }

#define INSERT_ROOT_PATTERN(tok,com)                        \
    do {                                                    \
        if (root) {                                         \
            root = root->insert_unified_pattern(tok, com);  \
        }                                                   \
        else {                                              \
            root = new SearchTreeNode(tok, com);            \
        }                                                   \
    } while(0)


    {
        char       *pattern           = strdup(sett->get_pattern());
        const char *trenner           = "\n,";
        char       *tok               = strtok(pattern, trenner);
        char       *comment;
        char        T_or_U;
        GB_ERROR    T_or_U_error      = GBT_determine_T_or_U(ED4_ROOT->alignment_type, &T_or_U, "complement");
        bool        show_T_or_U_error = false;

        while (tok) {
            splitTokComment(&tok, &comment);
            int uni_tok_len;
            char *uni_tok = unify_pattern(tok, &uni_tok_len);

            if (uni_tok[0]) {
                int s_exact      = sett->get_exact();
                int s_reverse    = sett->get_reverse();
                int s_complement = sett->get_complement();

                if (uni_tok_len<shortestPattern) {
                    shortestPattern = uni_tok_len;
                }

                if (!s_exact || (!s_reverse && !s_complement)) {
                    // insert original search pattern if all patterns shall be used (!s_exact)
                    // or if neither s_reverse nor s_complement
                    INSERT_ROOT_PATTERN(uni_tok, comment);
                }
                int commentLen = comment ? strlen(comment) : 0;

                if (s_reverse) {
                    char *reverse = GBT_reverseNucSequence(uni_tok, uni_tok_len);

                    if (!s_exact || !s_complement) {
                        char *reverseComment = appendComment(comment, commentLen, "(reverse)");

                        INSERT_ROOT_PATTERN(reverse, reverseComment); // insert reverse pattern
                        free(reverseComment);
                    }
                    if (s_complement) {
                        e4_assert(IS_NUCLEOTIDE());
                        // char T_or_U = ED4_ROOT->alignment_type==GB_AT_DNA ? 'T' : 'U';
                        if (T_or_U) {
                            char *revcomp        = GBT_complementNucSequence(reverse, uni_tok_len, T_or_U);
                            char *revcompComment = appendComment(comment, commentLen, "(reverse complement)");
                            char *uni_revcomp    = unify_pattern(revcomp, 0);

                            INSERT_ROOT_PATTERN(uni_revcomp, revcompComment); // insert reverse complement pattern

                            free(uni_revcomp);
                            free(revcompComment);
                            free(revcomp);
                        }
                        else {
                            show_T_or_U_error = true; // only show error if it matters
                        }
                    }

                    free(reverse);
                }
                
                if (s_complement) {
                    GB_transaction dummy(gb_main);

                    if (T_or_U) {
                        if (!s_exact || !s_reverse) {
                            char *complement        = GBT_complementNucSequence(uni_tok, uni_tok_len, T_or_U);
                            char *complementComment = appendComment(comment, commentLen, "(complement)");
                            char *uni_complement    = unify_pattern(complement, 0);

                            INSERT_ROOT_PATTERN(uni_complement, complementComment); // insert complement pattern

                            free(uni_complement);
                            free(complementComment);
                            free(complement);
                        }
                    }
                    else {
                        show_T_or_U_error = true; // only show error if it matters
                    }
                }

            }

            tok = strtok(0, trenner);
            free(uni_tok);
        }
        free(pattern);

        if (show_T_or_U_error && T_or_U_error) aw_message(T_or_U_error);
    }

#undef INSERT_ROOT_PATTERN
}

SearchTree::~SearchTree()
{
    delete root;
}

char SearchTree::unify_char(char c, int case_sensitive, int T_equal_U)
{
    if (!case_sensitive) {
        c = tolower(c);
    }

    if (T_equal_U) {
        if (c=='t') {
            c = 'u';
        }
        else if (c=='T') {
            c = 'U';
        }
    }

    if (c=='.') {
        c = '-';
    }

    return c;
}

char *SearchTree::unify_str(const char *data, int len, ED4_SEARCH_GAPS gaps, int *new_len, int **uni2real)
{
    char *p = (char*)malloc(len+1);

    if (!p) {
        return 0;
    }

    char *pp      = p;
    int   nlen    = 0;
    int   realPos = 0;

    if (uni2real) {
        int *u2r = *uni2real;

        if (gaps==ED4_SG_CONSIDER_GAPS) {
            while(realPos<len) {
                *pp++ = unified[(unsigned char)data[realPos]];
                u2r[nlen++] = realPos++;
            }
        }
        else {
            while(realPos<len) {
                unsigned char c = data[realPos];

                if (c!='-' && c!='.') {
                    *pp++ = unified[c];
                    u2r[nlen++] = realPos;
                }
                realPos++;
            }
        }
    }
    else {
        if (gaps==ED4_SG_CONSIDER_GAPS) {
            while(realPos<len) {
                *pp++ = unified[(unsigned char)data[realPos++]];
                nlen++;
            }
        }
        else {
            while(realPos<len) {
                unsigned char c = data[realPos++];

                if (c!='-' && c!='.') {
                    *pp++ = unified[c];
                    nlen++;
                }
            }
        }
    }

    // ---------------

    pp[0] = 0;
    if (new_len) {
        *new_len = nlen;
    }
    return p;
}

char *SearchTree::unify_pattern(const char *pattern, int *new_len)
{
    int len = strlen(pattern);
    return unify_str(pattern, len, sett->get_pat_gaps(), new_len, 0);
}

char *SearchTree::unify_sequence(const char *sequence, int len, int *new_len, int **uni2real)
{
    return unify_str(sequence, len, sett->get_seq_gaps(), new_len, uni2real);
}

void SearchTree::findMatches(const char *seq, int len, reportMatch report)
{
    if (root) {
        int new_len;
        int *uni2real = (int*)malloc(len*sizeof(int));
        char *uni_seq = uni2real ? unify_sequence(seq, len, &new_len, &uni2real) : 0;

        if (uni_seq) {
            int off;
            char *useq = uni_seq;
            int mismatch_list[MAX_MISMATCHES];

            for (off=0; off<MAX_MISMATCHES; off++) {
                mismatch_list[off] = -1;
            }

            SearchTreeNode::set_report(report, uni2real);
            SearchTreeNode::set_mismatches(sett->get_min_mismatches(), sett->get_max_mismatches());

            for (off=0; off<new_len && !ignore_more_results; off++,useq++) {
                SearchTreeNode::set_start_offset(off);
                root->findMatches(off, useq, new_len-off, 0, mismatch_list);
            }

            free(uni_seq);
            free(uni2real);
        }
        else {
            aw_message("Out of swapspace?", 0);
            if (uni2real) free(uni2real);
        }
    }
}

// --------------------------------------------------------------------------------

#define AWAR_NAME(t,s)  ED4_AWAR_##t##_SEARCH_##s

#define AWAR_LIST(t)                            \
    AWAR_NAME(t,PATTERN),                       \
    AWAR_NAME(t,MIN_MISMATCHES),                \
    AWAR_NAME(t,MAX_MISMATCHES),                \
    AWAR_NAME(t,CASE),                          \
    AWAR_NAME(t,TU),                            \
    AWAR_NAME(t,PAT_GAPS),                      \
    AWAR_NAME(t,SEQ_GAPS),                      \
    AWAR_NAME(t,REVERSE),                       \
    AWAR_NAME(t,COMPLEMENT),                    \
    AWAR_NAME(t,EXACT),                         \
    AWAR_NAME(t,SHOW),                          \
    AWAR_NAME(t,OPEN_FOLDED),                   \
    AWAR_NAME(t,AUTO_JUMP)

static struct S_SearchAwarList awar_list[SEARCH_PATTERNS] = {
    { AWAR_LIST(USER1) },
    { AWAR_LIST(USER2) },
    { AWAR_LIST(PROBE) },
    { AWAR_LIST(PRIMER1) },
    { AWAR_LIST(PRIMER2) },
    { AWAR_LIST(PRIMER3) },
    { AWAR_LIST(SIG1) },
    { AWAR_LIST(SIG2) },
    { AWAR_LIST(SIG3) },
};

static inline int resultsAreShown(ED4_SearchPositionType type) {
    return ED4_ROOT->aw_root->awar(awar_list[type].show)->read_int();
}

enum search_params_changed_action {
    REFRESH_IF_SHOWN   = 1,
    REFRESH_ALWAYS     = 2,
    RECALC_SEARCH_TREE = 4,
    TEST_MIN_MISMATCH  = 8,
    TEST_MAX_MISMATCH  = 16,
    DO_AUTO_JUMP       = 32
};

// --------------------------------------------------------------------------------

static SearchSettings *settings[SEARCH_PATTERNS]; // last searched settings for each type
static SearchTree     *tree[SEARCH_PATTERNS]; // Search trees for each type

// --------------------------------------------------------------------------------

static void searchParamsChanged(AW_root *root, AW_CL cl_type, AW_CL cl_action)
{
    ED4_SearchPositionType type = ED4_SearchPositionType(cl_type);
    enum search_params_changed_action action = (enum search_params_changed_action)cl_action;

    result_counter      = 0;
    ignore_more_results = false;

    // check awar values

    if (action & (TEST_MIN_MISMATCH|TEST_MAX_MISMATCH)) {
        int mimi = root->awar(awar_list[type].min_mismatches)->read_int();
        int mami = root->awar(awar_list[type].max_mismatches)->read_int();

        if (mimi>mami) {
            if (action & TEST_MIN_MISMATCH) { // max has changed
                root->awar(awar_list[type].min_mismatches)->write_int(mami);
            }
            if (action & TEST_MAX_MISMATCH) { // min has changed
                root->awar(awar_list[type].max_mismatches)->write_int(mimi);
            }
        }
    }

    // init new search

 recalc:
    ED4_SearchResults::setNewSearch(type);
    if (!settings[type]) return;
    settings[type]->update(&awar_list[type]);

    if (action & RECALC_SEARCH_TREE) {
        delete tree[type];
        tree[type] = new SearchTree(settings[type]);
    }

    if (action & (RECALC_SEARCH_TREE|TEST_MIN_MISMATCH|TEST_MAX_MISMATCH)) {
        int patLen = tree[type]->get_shortestPattern();
        int maxMis = root->awar(awar_list[type].max_mismatches)->read_int();

        if (patLen < 3*maxMis) {
            int maxMaxMis = tree[type]->get_shortestPattern()/3;

            aw_message(GBS_global_string("Too many mismatches (patterns of length=%i allow only %i max. mismatches)", patLen, maxMaxMis));

            root->awar(awar_list[type].max_mismatches)->write_int(maxMaxMis);
            if (root->awar(awar_list[type].min_mismatches)->read_int() > maxMaxMis) {
                root->awar(awar_list[type].min_mismatches)->write_int(maxMaxMis);
            }
            goto recalc;
        }
    }

    if (action & REFRESH_IF_SHOWN) {
        if (resultsAreShown(type)) {
            action = (enum search_params_changed_action)(action|REFRESH_ALWAYS);
        }
    }

    if (settings[type]->get_autoJump() && (action & DO_AUTO_JUMP)) { // auto jump
        ED4_cursor *cursor = &ED4_ROOT->temp_ed4w->cursor;
        int jumped = 0;

        if (cursor->owner_of_cursor && cursor->owner_of_cursor->is_sequence_terminal()) {
            int pos = cursor->get_sequence_pos();
            ED4_sequence_terminal *seq_term = cursor->owner_of_cursor->to_sequence_terminal();
            ED4_SearchResults *result = &seq_term->results();

            result->search(seq_term);
            ED4_SearchPosition *found = result->get_last_starting_before(type, pos+1, 0);
            int bestPos = -1;

            if (found) {
                bestPos = found->get_start_pos();
            }

            if (pos>=1) {
                found = result->get_first_starting_after(type, pos-1, 0);
                if (found) {
                    int next_pos = found->get_start_pos();

                    if (abs(pos-next_pos)<abs(pos-bestPos)) {
                        bestPos = next_pos;
                    }
                }
            }

            if (bestPos!=-1) {
                jumped = 1;
                if (bestPos!=pos) {
                    seq_term->setCursorTo(cursor, bestPos, 1);
                }
            }
        }

        if (!jumped) {
            ED4_search(0, ED4_encodeSearchDescriptor(+1, type));
        }
    }

    if (action & REFRESH_ALWAYS) {
        bool old_update                        = ED4_update_global_cursor_awars_allowed;
        ED4_update_global_cursor_awars_allowed = false;
        ED4_refresh_window(ED4_ROOT->temp_aww, 0, 0);
        ED4_update_global_cursor_awars_allowed = old_update;
    }
}

void ED4_create_search_awars(AW_root *root)
{
#define cb(action) add_callback(searchParamsChanged, AW_CL(i), AW_CL(action))

    int i;
    for (i=0; i<SEARCH_PATTERNS; i++) {
        root->awar_string(awar_list[i].pattern, "", gb_main)                            ->cb(REFRESH_IF_SHOWN | RECALC_SEARCH_TREE | DO_AUTO_JUMP);
        root->awar_int(awar_list[i].case_sensitive, ED4_SC_CASE_INSENSITIVE, gb_main)   ->cb(REFRESH_IF_SHOWN | RECALC_SEARCH_TREE | DO_AUTO_JUMP);
        root->awar_int(awar_list[i].tu, ED4_ST_T_EQUAL_U, gb_main)                      ->cb(REFRESH_IF_SHOWN | RECALC_SEARCH_TREE | DO_AUTO_JUMP);
        root->awar_int(awar_list[i].pat_gaps, ED4_SG_IGNORE_GAPS, gb_main)              ->cb(REFRESH_IF_SHOWN | RECALC_SEARCH_TREE | DO_AUTO_JUMP);
        root->awar_int(awar_list[i].reverse, 0, gb_main)                                ->cb(REFRESH_IF_SHOWN | RECALC_SEARCH_TREE | DO_AUTO_JUMP);
        root->awar_int(awar_list[i].complement, 0, gb_main)                             ->cb(REFRESH_IF_SHOWN | RECALC_SEARCH_TREE | DO_AUTO_JUMP);
        root->awar_int(awar_list[i].exact, 0, gb_main)                                  ->cb(REFRESH_IF_SHOWN | RECALC_SEARCH_TREE | DO_AUTO_JUMP);
        root->awar_int(awar_list[i].min_mismatches, 0, gb_main)                         ->cb(REFRESH_IF_SHOWN | TEST_MAX_MISMATCH | DO_AUTO_JUMP);
        root->awar_int(awar_list[i].max_mismatches, 0, gb_main)                         ->cb(REFRESH_IF_SHOWN | TEST_MIN_MISMATCH | DO_AUTO_JUMP);
        root->awar_int(awar_list[i].seq_gaps, ED4_SG_IGNORE_GAPS, gb_main)              ->cb(REFRESH_IF_SHOWN | DO_AUTO_JUMP);
        root->awar_int(awar_list[i].show, 1, gb_main)                                   ->cb(REFRESH_ALWAYS);
        root->awar_int(awar_list[i].openFolded, 1, gb_main)                             ->cb(0);
        root->awar_int(awar_list[i].autoJump, 1, gb_main)                               ->cb(DO_AUTO_JUMP);

        settings[i] = new SearchSettings(&awar_list[i]);
        tree[i] = new SearchTree(settings[i]);
    }

#undef cb

    // awars to save/load search parameters:
    {
        char *dir = GBS_global_string_copy("%s/lib/search_settings", GB_getenvARBHOME());
        aw_create_selection_box_awars(root, ED4_SEARCH_SAVE_BASE, dir, ".asp", "noname.asp");
        root->awar(ED4_SEARCH_SAVE_BASE"/directory")->write_string(dir);
        free(dir);
    }
}

// --------------------------------------------------------------------------------

char *ED4_SearchPosition::lastShownComment = 0;

ED4_SearchPosition::ED4_SearchPosition(int sp, int ep, ED4_SearchPositionType wf, GB_CSTR found_comment, int mismatches[MAX_MISMATCHES])
{
    e4_assert(sp<=ep && sp>=0); // && ep<20000);
    start_pos = sp;
    end_pos = ep;
    whatsFound = wf;
    next = 0;
    comment = found_comment;
    memcpy(mismatch, mismatches, sizeof(*mismatch)*MAX_MISMATCHES);
}
ED4_SearchPosition::ED4_SearchPosition(const ED4_SearchPosition& other) {
    start_pos = other.start_pos;
    end_pos = other.end_pos;
    whatsFound = other.whatsFound;
    next = 0;
    comment = strdup(other.comment);
    memcpy(mismatch, other.mismatch, sizeof(mismatch[0])*MAX_MISMATCHES);
}

ED4_SearchPosition *ED4_SearchPosition::insert(ED4_SearchPosition *toAdd)
{
    ED4_SearchPosition *head = this;
    ED4_SearchPosition *self = this;
    ED4_SearchPosition *last = 0;

    while (1) {
        if (toAdd->cmp(*self)<=0) { // add before self
            toAdd->next          = self;
            if (last) last->next = toAdd;
            else head            = toAdd;
            break;
        }
        if (!self->next) { // add to end of list
            self->next = toAdd;
            break;
        }
        last = self;
        self = self->next;
    }

    return head;
}
ED4_SearchPosition *ED4_SearchPosition::remove(ED4_SearchPositionType typeToRemove)
{
    ED4_SearchPosition *head = this;
    ED4_SearchPosition *self = this;
    ED4_SearchPosition *last = 0;

    while (self) {
        if (self->whatsFound == typeToRemove) { // remove self
            ED4_SearchPosition **ptrToMe = last ? &(last->next) : &head;

            *ptrToMe   = self->next;
            self->next = 0;
            delete self;
            self       = *ptrToMe;
        }
        else { // do not remove
            last = self;
            self = self->next;
        }
    }

    return head;
}

#ifdef TEST_SEARCH_POSITION
int ED4_SearchPosition::ok() const
{
#ifndef NDEBUG
    if (next) {
        int c = cmp(*next);

        if (c>0) {
            printf("ED4_SearchPosition: list not sorted\n");
            return 0;
        }

        return next->ok();
    }
#endif
    return 1;
}
#endif

GB_CSTR ED4_SearchPosition::get_comment() const
{
    if (!comment) return 0;
    if (lastShownComment && strcmp(lastShownComment, comment)==0) return 0; // do not show comment twice

    delete lastShownComment;
    lastShownComment = strdup(comment);
    return lastShownComment;
}

ED4_SearchPosition *ED4_SearchPosition::get_next_at(int pos) const
{
    ED4_SearchPosition *self = this->next;

    while (self) {
        if (self->start_pos > pos) break; // none found
        if (self->containsPos(pos)) return self;
        self = self->next;
    }
    return 0;
}

// --------------------------------------------------------------------------------

int   ED4_SearchResults::initialized = 0;
int   ED4_SearchResults::timeOfLastSearch[SEARCH_PATTERNS];
int   ED4_SearchResults::timeOfNextSearch[SEARCH_PATTERNS];
int   ED4_SearchResults::shown[SEARCH_PATTERNS];
int   ED4_SearchResults::bufferSize;
char *ED4_SearchResults::buffer;

ED4_SearchResults::ED4_SearchResults()
{
    if (!initialized) {
        int i;

        for (i=0; i<SEARCH_PATTERNS; i++) {
            timeOfLastSearch[i] = 0;
            timeOfNextSearch[i] = 1;
            shown[i] = resultsAreShown(ED4_SearchPositionType(i));
        }

        bufferSize = 100;
        buffer = (char*)GB_calloc(bufferSize, sizeof(char));

        initialized = 1;
    }

    arraySize = 0;              // list-format
    array     = 0;
    first     = 0;
    int i;
    for (i=0; i<SEARCH_PATTERNS; i++) {
        timeOf[i] = 0;
    }
}

ED4_SearchResults::~ED4_SearchResults() {
    delete first;
    free(array);
}

// --------------------------------------------------------------------------------

static ED4_SearchResults      *reportToResult = 0;
static ED4_SearchPositionType  reportType;

static void reportSearchPosition(int start, int end, GB_CSTR comment, int mismatches[MAX_MISMATCHES])
{
    ED4_SearchPosition *pos = new ED4_SearchPosition(start, end, reportType, comment, mismatches);
    reportToResult->addSearchPosition(pos);
}

// --------------------------------------------------------------------------------

void ED4_SearchResults::addSearchPosition(ED4_SearchPosition *pos)
{
    if (ignore_more_results) return;

    if (is_array()) {
        to_list();
    }

    ++result_counter;
    if (result_counter >= max_allowed_results) {
        if (aw_message(GBS_global_string("More than %i results found!", result_counter), "Allow more,That's enough") == 0) {
            max_allowed_results = max_allowed_results*2;
        }
        else {
            ignore_more_results = true;
            return;
        }
    }

    if (first) {
        first = first->insert(pos);
#ifdef TEST_SEARCH_POSITION
        e4_assert(first->ok());
#endif
    }
    else {
        first = pos;
    }
}

void ED4_SearchResults::search(ED4_sequence_terminal *seq_terminal)
{
    int i;
    int needed[SEARCH_PATTERNS];
    int st_needed = 0;

    for (i=0; i<SEARCH_PATTERNS; i++) {
        if (timeOf[i]<timeOfNextSearch[i]) {
            timeOf[i] = timeOfNextSearch[i];
            timeOfLastSearch[i] = timeOfNextSearch[i];
            needed[i] = 1;
            st_needed = 1;
            if (first) {
                if (is_array()) {
                    to_list();
                }
                first = first->remove(ED4_SearchPositionType(i));
#if defined TEST_SEARCH_POSITION
                e4_assert(!first || first->ok());
#endif
            }
        }
        else {
            needed[i] = 0;
        }
    }

    if (st_needed) {
        int len;
        char *seq = seq_terminal->resolve_pointer_to_string_copy(&len);

        if (seq) {
            reportToResult = this;
            for (i=0; i<SEARCH_PATTERNS; i++) {
                reportType = ED4_SearchPositionType(i);
                if (needed[i]) {
                    if (is_array()) {
                        to_list();
                    }
                    tree[i]->findMatches(seq, len, reportSearchPosition);
                }
            }
            reportToResult = 0;
        }
        free(seq);
    }
}

ED4_SearchPosition *ED4_SearchResults::get_first_at(ED4_SearchPositionType type, int start, int end) const {
    if (is_list()) {
        to_array();
    }

    int l = 0;
    int h = arraySize-1;

    ED4_SearchPosition *pos = 0;
    int m = 0;

    while (l<=h) {
        m = (l+h)/2;
        pos = array[m];
        if (pos->get_end_pos()<start) {
            l = m+1;
        }
        else if (pos->get_start_pos()>end) {
            h = m-1;
        }
        else {
            e4_assert(pos->get_end_pos()>=start && pos->get_start_pos()<=end);
            break;
        }
    }

    if (l>h) {
        return 0;
    }

    if (pos) {
        int best_m = m;

        while (m>0) {
            m--;
            pos = array[m];
            if (pos->get_end_pos()>=start && pos->get_start_pos()<=end) {
                best_m = m;
            }
        }
        pos = array[best_m];

        while (pos) {
            if (type==ED4_ANY_PATTERN || pos->get_whatsFound()==type) {
                break;
            }
            pos = pos->get_next();
        }

        return pos;
    }

    return 0;
}

ED4_SearchPosition *ED4_SearchResults::get_first_starting_after(ED4_SearchPositionType type, int pos, int mustBeShown) const
{
    ED4_SearchPosition *sp = first;

    while (sp) {
        if (type==ED4_ANY_PATTERN || sp->get_whatsFound()==type) {
            if (sp->get_start_pos()>pos && (!mustBeShown || resultsAreShown(sp->get_whatsFound()))) {
                break;
            }
        }
        sp = sp->get_next();
    }

    return sp;
}

ED4_SearchPosition *ED4_SearchResults::get_last_starting_before(ED4_SearchPositionType type, int pos, int mustBeShown) const
{
    ED4_SearchPosition *sp = first,
        *best = 0;

    while (sp) {
        if (type==ED4_ANY_PATTERN || sp->get_whatsFound()==type) {
            if (sp->get_start_pos()<pos && (!mustBeShown || resultsAreShown(sp->get_whatsFound()))) {
                best = sp;
            }
            else {
                break;
            }
        }
        sp = sp->get_next();
    }

    return best;
}

void ED4_SearchResults::setNewSearch(ED4_SearchPositionType type)
{
    if (type==ED4_ANY_PATTERN) {
        int i;
        for (i=0; i<SEARCH_PATTERNS; i++) {
            setNewSearch(ED4_SearchPositionType(i));
        }
    }
    else {
        int next_unused_stamp = timeOfLastSearch[type] + 1;

        shown[type] = resultsAreShown(type);
        if (timeOfNextSearch[type]!=next_unused_stamp) {
            timeOfNextSearch[type] = next_unused_stamp;
        }
    }
}

void ED4_SearchResults::searchAgain()
{
    int i;

    for (i=0; i<SEARCH_PATTERNS; i++) {
        timeOf[i] = 0;
    }
}

char *ED4_SearchResults::buildColorString(ED4_sequence_terminal *seq_terminal, int start, int end)
    // builds a char buffer (access is only valid from result[start] till result[end])
{
    int i;
    int st_shown = 0;

    e4_assert(start<=end);                  //confirming the condition
    for (i=0; i<SEARCH_PATTERNS; i++) {
        if (shown[i]) {
            st_shown = 1;
            break;
        }
    }
    if (!st_shown) {
        return 0; // nothing shown
    }

    search(seq_terminal);
    if (!get_first()) {
        return 0; // nothing found
    }

    int needed_size = end-start+1;
    if (needed_size>bufferSize) {
        free(buffer);
        bufferSize = needed_size;
        buffer = (char*)GB_calloc(bufferSize, sizeof(char));
    }
    else {
        memset(buffer, 0, sizeof(char)*needed_size);
    }

    // search first search-result that goes in between start-end

    ED4_SearchPosition *pos = get_first_at(ED4_ANY_PATTERN, start, end);
    e4_assert(!pos || (pos->get_start_pos()<=end && pos->get_end_pos()>=start));

    int correct_neg_values = 0;

    while (pos && pos->get_start_pos()<=end) {
        int what = int(pos->get_whatsFound());

        if (shown[what]) {
            int color = ED4_G_SBACK_0 + what;
            int s = max(pos->get_start_pos(), start)-start;
            int e = min(pos->get_end_pos(), end)-start;

            for (i=s; i<=e; i++) {
                if (buffer[i]==0 || abs(buffer[i])>abs(color)) {
                    buffer[i] = color;
                }
            }

            const int *mismatches = pos->getMismatches();

            for (i=0; i<5 && mismatches[i]>=0; i++) {
                int mpos = mismatches[i];

                if (mpos>=start && mpos<=end) {
                    int rpos = mpos-start;
                    if (buffer[rpos]==color) {
                        buffer[rpos] = -buffer[rpos];
                        correct_neg_values = 1;
                    }
                }
            }
        }
        pos = pos->get_next();
    }

    if (correct_neg_values) {
        for (i=end-start; i>=0; i--) {
            if (buffer[i]<0) {
                buffer[i] = ED4_G_MBACK;
            }
        }
    }

    return buffer-start;
}

ED4_SearchPosition *ED4_SearchResults::get_shown_at(int pos) const
    // used by ED4_cursor::updateAwars to get search-comment
{
    ED4_SearchPosition *p = get_last_starting_before(ED4_ANY_PATTERN, pos, 0); // @@@ tofix: should find last_ending before
    ED4_SearchPosition *best = 0;

    if (!p) {
        p = get_first();
    }

    if (p && !p->containsPos(pos)) {
        p = p->get_next_at(pos);
    }

    while (p) {
        e4_assert(p->containsPos(pos));
        ED4_SearchPositionType type = p->get_whatsFound();
        if (shown[type]) {
            if (!best || type<best->get_whatsFound()) {
                best = p;
            }
        }
        p = p->get_next_at(pos);
    }

    return best;
}

void ED4_SearchResults::to_array() const {
    e4_assert(arraySize==0); // assert list-format

    ED4_SearchPosition *pos = first;

    {
        int a_arraySize = 0;

        while (pos) {
            a_arraySize++;
            pos = pos->get_next();
        }
        *((int*)&arraySize) = a_arraySize;
    }

    ED4_SearchPosition **a_array = (ED4_SearchPosition**)malloc(sizeof(ED4_SearchPosition*)*arraySize);

    int e;
    pos = first;
    for (e=0; e<arraySize; e++) {
        e4_assert(pos);
        a_array[e] = pos;
        pos = pos->get_next();
    }

    *((ED4_SearchPosition***)&array) = a_array;
}
void ED4_SearchResults::to_list() const {
    e4_assert(arraySize>0); // assert array-format
    free(array);

    *((int*)&arraySize) = 0;
    *((ED4_SearchPosition***)&array) = 0;
}


// --------------------------------------------------------------------------------

inline void decodeSearchDescriptor(int descriptor, int *direction, ED4_SearchPositionType *pattern)
{
    *direction = descriptor&1 ? 1 : -1;
    *pattern = ED4_SearchPositionType(descriptor/2);
}

static AW_CL last_searchDescriptor = -1;

GB_ERROR ED4_repeat_last_search(void) {
    if (int(last_searchDescriptor)==-1) {
        return GBS_global_string("You have to search first, before you can repeat a search.");
    }

    ED4_search(0, last_searchDescriptor);
    return 0;
}

void ED4_search(AW_window */*aww*/, AW_CL searchDescriptor)
{
    int direction;
    ED4_SearchPositionType pattern;
    int searchOnlyForShownPatterns;

    last_searchDescriptor = searchDescriptor;
    decodeSearchDescriptor(searchDescriptor, &direction, &pattern);
    searchOnlyForShownPatterns = pattern==ED4_ANY_PATTERN;

    // detect position where to start searching

    ED4_terminal *terminal = 0; // start search at this terminal
    int pos; // ... and at this position

    ED4_cursor *cursor = &ED4_ROOT->temp_ed4w->cursor;
    if (cursor->owner_of_cursor) { // if cursor is shown -> use cursor position
        terminal = cursor->owner_of_cursor->to_terminal();
        pos = cursor->get_sequence_pos();
    }
    else { // start at end or beginning
        if (direction==1) {
            terminal = ED4_ROOT->root_group_man->get_first_terminal();
            pos = INT_MIN;
        }
        else {
            terminal = ED4_ROOT->root_group_man->get_last_terminal();
            pos = INT_MAX;
        }
    }

    ED4_terminal *start_terminal = terminal;
    int start_pos = pos;
    int last_loop = 0;

    while (terminal) {
        if (terminal->is_sequence_terminal()) {
            ED4_sequence_terminal *seq_terminal = terminal->to_sequence_terminal();
            ED4_SearchResults&     results      = seq_terminal->results();
            ED4_SearchPosition    *found        = 0;

            if (pattern==ED4_ANY_PATTERN || settings[pattern]->get_open_folded() || !terminal->is_in_folded_group()) {
                results.search(seq_terminal);

                if (direction==1) {
                    found = results.get_first_starting_after(pattern, pos, searchOnlyForShownPatterns);
                }
                else {
                    found = results.get_last_starting_before(pattern, pos, searchOnlyForShownPatterns);
                }

                if (found) {
                    pos = found->get_start_pos();
                    if (terminal==start_terminal && pos==start_pos) {
                        if (searchOnlyForShownPatterns) {
                            aw_message("There are no other shown patterns");
                        }
                        else {
                            aw_message("This is the only occurance of the search pattern");
                        }
                    }
                    else {
                        seq_terminal->setCursorTo(&ED4_ROOT->temp_ed4w->cursor, pos, 1);
                    }
                    break;
                }
                else if (last_loop) {
                    if (searchOnlyForShownPatterns) {
                        aw_message("There are no shown patterns");
                    }
                    else {
                        aw_message("Search pattern was not found in any sequence");
                    }
                    break;
                }
            }
        }

        if (direction==1) {
            terminal = terminal->get_next_terminal();
            if (!terminal) terminal = ED4_ROOT->root_group_man->get_first_terminal();
            pos = INT_MIN;
        }
        else {
            terminal = terminal->get_prev_terminal();
            if (!terminal) terminal = ED4_ROOT->root_group_man->get_last_terminal();
            pos = INT_MAX;
        }

        if (terminal==start_terminal) {
            last_loop = 1;
        }
    }
}

void ED4_mark_matching_species(AW_window */*aww*/, AW_CL cl_pattern) {
    ED4_SearchPositionType  pattern  = ED4_SearchPositionType(cl_pattern);
    ED4_terminal           *terminal = ED4_ROOT->root_group_man->get_first_terminal();
    GB_ERROR                error    = 0;
    GB_transaction          ta(gb_main);

    while (terminal && !error) {
        if (terminal->is_sequence_terminal()) {
            ED4_sequence_terminal *seq_terminal = terminal->to_sequence_terminal();
            ED4_SearchResults&     results      = seq_terminal->results();

            results.search(seq_terminal);
            ED4_SearchPosition *found = results.get_first_starting_after(pattern, INT_MIN, false);

            if (found) {
                ED4_species_manager *species_man = seq_terminal->get_parent(ED4_L_SPECIES)->to_species_manager();;
                if (!species_man->flag.is_consensus) {
                    GBDATA *gbd = species_man->get_species_pointer();
                    e4_assert(gbd);
                    error = GB_write_flag(gbd, 1);
                }            
            }
        }

        terminal = terminal->get_next_terminal();
    }

    if (error) {
        aw_message(error);
        ta.abort();
    }
}

#define ESC '\\'

static char *pattern2str(GB_CSTR p) {
    char *s = GB_give_buffer(strlen(p)*2);
    char *s1 = s;
    GB_CSTR p1 = p;

    while (1) {
        char c = *p1++;

        if (!c) {
            break;
        }
        else if (c==ESC) {
            *s1++ = ESC;
            *s1++ = ESC;
        }
        else if (c=='\n') {
            *s1++ = ESC;
            *s1++ = 'n';
        }
        else {
            *s1++ = c;
        }
    }

    *s1 = 0;
    return strdup(s);
}

static void str2pattern(char *s) { // works on string
    char *a = s;
    char *n = s;

    while (1) {
        char c = *a++;

        if (c==ESC) {
            c = *a++;
            if (c==ESC) {
                *n++ = ESC;
            }
            else if (c=='n') {
                *n++ = '\n';
            }
            else { // should not occur
                *n++ = ESC;
                *n++ = c;
            }
        }
        else {
            *n++ = c;
            if (!c) {
                break;
            }
        }
    }
}

#undef ESC

static void save_search_paras_to_file(AW_window *aw, AW_CL cl_type) {
    AW_root *root = ED4_ROOT->aw_root;
    ED4_SearchPositionType type = ED4_SearchPositionType(cl_type);
    char *filename = root->awar(ED4_SEARCH_SAVE_BASE"/file_name")->read_string();
    FILE *in = fopen(filename, "rt");

    if (in) {
        fclose(in);
        GB_CSTR error = GBS_global_string("'%s' already exists", filename);
        int answer = aw_message(error, "Overwrite,Cancel");
        if (answer!=0) {
            return;
        }
    }

    FILE *out = fopen(filename, "wt");

    if (!out) {
        GB_CSTR error = GBS_global_string("Can't write file '%s' (%s)", filename, strerror(errno));
        aw_message(error, "OK");
    }
    else {
        SearchSettings *s = settings[type];

        char *fpat = pattern2str(s->get_pattern());

        fprintf(out,
                "pattern=%s\n"
                "minmis=%i\n"
                "maxmis=%i\n"
                "case=%i\n"
                "tu=%i\n"
                "patgaps=%i\n"
                "seqgaps=%i\n"
                "openfolded=%i\n"
                "autojump=%i\n"
                "reverse=%i\n"
                "complement=%i\n"
                "exact=%i\n",
                fpat,
                s->get_min_mismatches(),
                s->get_max_mismatches(),
                s->get_case_sensitive(),
                s->get_tu(),
                s->get_pat_gaps(),
                s->get_seq_gaps(),
                s->get_open_folded(),
                s->get_autoJump(),
                s->get_reverse(),
                s->get_complement(),
                s->get_exact());

        free(fpat);
        fclose(out);
    }
    AW_POPDOWN(aw);
}

static void load_search_paras_from_file(AW_window *aw, AW_CL cl_type) {
    AW_root *root = ED4_ROOT->aw_root;
    ED4_SearchPositionType type = ED4_SearchPositionType(cl_type);
    char *filename = root->awar(ED4_SEARCH_SAVE_BASE"/file_name")->read_string();
    FILE *in = fopen(filename, "rt");

    if (!in) {
        GB_CSTR error = GBS_global_string("File '%s' not found", filename);
        aw_message(error, "Oops.. sorry!");
        return;
    }

#define BUFFERSIZE 10000

    SearchAwarList al = &awar_list[type];
    while(1) {
        char buffer[BUFFERSIZE];

        buffer[0] = 0;
        fgets(buffer, BUFFERSIZE, in);
        if (!buffer[0]) {
            break;
        }
        char *content = strchr(buffer, '=');
        if (!content) {
            aw_message(GBS_global_string("Format error in '%s' - ignored!", filename));
            continue;
        }
        *content++ = 0;

        if (strcmp(buffer, "pattern")==0) {
            str2pattern(content);
            root->awar(al->pattern)->write_string(content);
        }
        else {
            int value = atoi(content);

            if (strcmp(buffer, "minmis")==0)            root->awar(al->min_mismatches)->write_int(value);
            else if (strcmp(buffer, "maxmis")==0)       root->awar(al->max_mismatches)->write_int(value);
            else if (strcmp(buffer, "case")==0)         root->awar(al->case_sensitive)->write_int(value);
            else if (strcmp(buffer, "tu")==0)           root->awar(al->tu)->write_int(value);
            else if (strcmp(buffer, "patgaps")==0)      root->awar(al->pat_gaps)->write_int(value);
            else if (strcmp(buffer, "seqgaps")==0)      root->awar(al->seq_gaps)->write_int(value);
            else if (strcmp(buffer, "openfolded")==0)   root->awar(al->openFolded)->write_int(value);
            else if (strcmp(buffer, "autojump")==0)     root->awar(al->autoJump)->write_int(value);
            else if (strcmp(buffer, "reverse")==0)      root->awar(al->reverse)->write_int(value);
            else if (strcmp(buffer, "complement")==0)   root->awar(al->complement)->write_int(value);
            else if (strcmp(buffer, "exact")==0)        root->awar(al->exact)->write_int(value);
            else {
                aw_message(GBS_global_string("Unknown tag '%s' in '%s' - ignored!", buffer, filename));
            }
        }
    }

#undef BUFFERSIZE

    fclose(in);
    AW_POPDOWN(aw);
}

static AW_window *loadsave_search_parameters(AW_root *root, ED4_SearchPositionType type, int load) {
    AW_window_simple *aws = new AW_window_simple;

    if (load) {
        ED4_aws_init(root, aws, "load_%s_search_para", "Load %s Search Parameters", ED4_SearchPositionTypeId[type]);
    }
    else {
        ED4_aws_init(root, aws, "save_%s_search_para", "Save %s Search Parameters", ED4_SearchPositionTypeId[type]);
    }

    aws->load_xfig("edit4/save_search.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP, (AW_CL)"search_parameters.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box((AW_window *)aws, ED4_SEARCH_SAVE_BASE);

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("cancel");
    aws->create_button("CANCEL","CANCEL","C");

    aws->at("save");
    if (load) {
        aws->callback(load_search_paras_from_file, (AW_CL)type);
        aws->create_button("LOAD","LOAD","L");
    }
    else {
        aws->callback(save_search_paras_to_file, (AW_CL)type);
        aws->create_button("SAVE","SAVE","S");
    }

    return aws;
}

static AW_window *load_search_parameters(AW_root *root, AW_CL cl_type) {
    return loadsave_search_parameters(root, ED4_SearchPositionType(cl_type), 1);
}

static AW_window *save_search_parameters(AW_root *root, AW_CL cl_type) {
    return loadsave_search_parameters(root, ED4_SearchPositionType(cl_type), 0);
}


static void search_init_config(AWT_config_definition& cdef, int search_type) {
    SearchAwarList awarList = &awar_list[search_type];

    cdef.add(awarList->show, "show");
    cdef.add(awarList->openFolded, "openFolded");
    cdef.add(awarList->autoJump, "autoJump");
    cdef.add(awarList->pattern, "pattern");
    cdef.add(awarList->min_mismatches, "min_mismatches");
    cdef.add(awarList->max_mismatches, "max_mismatches");
    cdef.add(awarList->seq_gaps, "seq_gaps");
    cdef.add(awarList->pat_gaps, "pat_gaps");
    cdef.add(awarList->tu, "tu");
    cdef.add(awarList->case_sensitive, "case_sensitive");
    cdef.add(awarList->reverse, "reverse");
    cdef.add(awarList->complement, "complement");
    cdef.add(awarList->exact, "exact");
}

//  -------------------------------------------------------------------------
//      static char *search_store_config(AW_window *aww, AW_CL , AW_CL )
//  -------------------------------------------------------------------------
static char *search_store_config(AW_window *aww, AW_CL cl_search_type, AW_CL ) {
    AWT_config_definition cdef(aww->get_root());
    search_init_config(cdef, int(cl_search_type));
    return cdef.read();
}
//  -----------------------------------------------------------------------------------------------------
//      static void search_restore_config(AW_window *aww, const char *stored_string, AW_CL , AW_CL )
//  -----------------------------------------------------------------------------------------------------
static void search_restore_config(AW_window *aww, const char *stored_string, AW_CL cl_search_type, AW_CL ) {
    AWT_config_definition cdef(aww->get_root());
    search_init_config(cdef, int(cl_search_type));
    cdef.write(stored_string);
}


AW_window *ED4_create_search_window(AW_root *root, AW_CL cl) {
    ED4_SearchPositionType type = ED4_SearchPositionType(cl);
    SearchAwarList awarList = &awar_list[type];
    AW_window_simple *aws = new AW_window_simple;

    ED4_aws_init(root, aws, "%s_search", "%s Search", ED4_SearchPositionTypeId[type]);
    aws->load_xfig("edit4/search.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");
    aws->callback( AW_POPUP_HELP,(AW_CL)"e4_search.hlp");
    aws->create_button("HELP", "HELP","H");

    aws->at("load");
    aws->callback(AW_POPUP, (AW_CL)load_search_parameters, (AW_CL)type);
    aws->create_button("LOAD", "LOAD", "L");

    aws->at("save");
    aws->callback(AW_POPUP, (AW_CL)save_search_parameters, (AW_CL)type);
    aws->create_button("SAVE", "SAVE", "S");

    aws->at("next");
    aws->callback(ED4_search, (AW_CL)ED4_encodeSearchDescriptor(+1, type));
    aws->create_button("SEARCH_NEXT", "#edit/next.bitmap", "N");

    aws->at("previous");
    aws->callback(ED4_search, (AW_CL)ED4_encodeSearchDescriptor(-1, type));
    aws->create_button("SEARCH_LAST", "#edit/last.bitmap", "L");

    aws->at("mark");
    aws->callback(ED4_mark_matching_species, (AW_CL)type);
    aws->create_autosize_button("MARK_SPECIES", "Mark species with matches", "M");

    aws->at("show");
    aws->create_toggle(awarList->show);

    aws->at("open");
    aws->create_toggle(awarList->openFolded);

    aws->at("jump");
    aws->create_toggle(awarList->autoJump);

    aws->at("pattern");
    aws->create_text_field(awarList->pattern, 28, 17);

    aws->at("minmis");
    aws->create_toggle_field(awarList->min_mismatches, 1);
    aws->insert_default_toggle("0", "0", 0);
    aws->insert_toggle("1", "1", 1);
    aws->insert_toggle("2", "2", 2);
    aws->update_toggle_field();

    aws->at("maxmis");
    aws->create_toggle_field(awarList->max_mismatches, 1);
    aws->insert_default_toggle("0", "0", 0);
    aws->insert_toggle("1", "1", 1);
    aws->insert_toggle("2", "2", 2);
    aws->insert_toggle("3", "3", 3);
    aws->insert_toggle("4", "4", 4);
    aws->insert_toggle("5", "5", 5);
    aws->update_toggle_field();

    aws->at("seq_gaps");
    aws->create_toggle(awarList->seq_gaps);
    aws->at("pat_gaps");
    aws->create_toggle(awarList->pat_gaps);
    aws->at("tu");
    aws->create_toggle(awarList->tu);
    aws->at("case");
    aws->create_toggle(awarList->case_sensitive);
    aws->at("reverse");
    aws->create_toggle(awarList->reverse);
    aws->at("complement");
    aws->create_toggle(awarList->complement);
    aws->at("exact");
    aws->create_toggle(awarList->exact);

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "search", search_store_config, search_restore_config, (AW_CL)type, 0);

    return (AW_window *)aws;
}

static int has_species_name(ED4_base *base, AW_CL cl_species_name) {
    if (base->is_sequence_terminal()) {
        ED4_sequence_terminal *seq_term = base->to_sequence_terminal();
        const char *species_name = (const char *)cl_species_name;
        return species_name && seq_term && seq_term->species_name && strcmp(species_name, seq_term->species_name)==0;
    }
    return 0;
}

ED4_sequence_terminal *ED4_find_seq_terminal(const char *species_name) {
    ED4_base *base = ED4_ROOT->main_manager->find_first_that(ED4_L_SEQUENCE_STRING, has_species_name, (AW_CL)species_name);
    ED4_sequence_terminal *seq_term = base->to_sequence_terminal();

    return seq_term;
}

