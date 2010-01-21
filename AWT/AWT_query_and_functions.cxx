#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>
#include <ctype.h>

#include <list>
#include <string>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <aw_color_groups.hxx>
#include "awt.hxx"
#include "awtlocal.hxx"
#include "awt_config_manager.hxx"
#include "awt_item_sel_list.hxx"
#include "awt_sel_boxes.hxx"
#include "awt_advice.hxx"

#include "GEN.hxx"

using namespace std;

#define AWT_MAX_QUERY_LIST_LEN  100000
#define AWT_MAX_SHOWN_DATA_SIZE 500

/***************** Create the database query box and functions *************************/

#define AWAR_COLORIZE "tmp/dbquery_all/colorize"

inline void SET_QUERIED(GBDATA *gb_species, adaqbsstruct *cbs, const char *hitInfo, size_t hitInfoLen = 0) {
    awt_assert(hitInfo);

    GB_write_usr_private(gb_species, cbs->select_bit | GB_read_usr_private(gb_species));

    char *name = cbs->selector->generate_item_id(cbs->gb_main, gb_species);
    char *info;

    if (hitInfoLen == 0) hitInfoLen = strlen(hitInfo);
    if (hitInfoLen>AWT_MAX_SHOWN_DATA_SIZE) {
        char *dupInfo = strdup(hitInfo);
        hitInfoLen    = GBS_shorten_repeated_data(dupInfo);
        if (hitInfoLen>AWT_MAX_SHOWN_DATA_SIZE) {
            strcpy(dupInfo+hitInfoLen-5, "[...]");
        }
        info = strdup(dupInfo);
        free(dupInfo);
    }
    else {
        info = strdup(hitInfo);
    }

    long  toFree = GBS_write_hash(cbs->hit_description, name, reinterpret_cast<long>(info));
    if (toFree) free(reinterpret_cast<char*>(toFree)); // free old value (from hash)
    free(name);
}

inline void CLEAR_QUERIED(GBDATA *gb_species, adaqbsstruct *cbs) {
    GB_write_usr_private(gb_species, (~cbs->select_bit) & GB_read_usr_private(gb_species));

    char *name   = cbs->selector->generate_item_id(cbs->gb_main, gb_species);
    long  toFree = GBS_write_hash(cbs->hit_description, name, 0); // delete hit info
    if (toFree) free(reinterpret_cast<char*>(toFree));
    free(name);
}

inline const char *getHitInfo(GBDATA *gb_species, adaqbsstruct *cbs) {
    char *name = cbs->selector->generate_item_id(cbs->gb_main, gb_species);
    long  info = GBS_read_hash(cbs->hit_description, name);
    free(name);
    return reinterpret_cast<const char*>(info);
}

static void awt_query_create_global_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int(AWAR_COLORIZE, 0, aw_def);
}

enum AWT_EXT_QUERY_TYPES {
    AWT_EXT_QUERY_NONE,
    AWT_EXT_QUERY_COMPARE_LINES,
    AWT_EXT_QUERY_COMPARE_WORDS
};

awt_query_struct::awt_query_struct(void) {
    memset((char *)this, 0, sizeof(awt_query_struct));
    select_bit = 1;
}

long awt_count_queried_items(struct adaqbsstruct *cbs, AWT_QUERY_RANGE range) {
    GBDATA                 *gb_main  = cbs->gb_main;
    const ad_item_selector *selector = cbs->selector;

    long count = 0;

    for (GBDATA *gb_item_container = selector->get_first_item_container(gb_main, cbs->aws->get_root(), range);
         gb_item_container;
         gb_item_container = selector->get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = selector->get_first_item(gb_item_container);
             gb_item;
             gb_item = selector->get_next_item(gb_item))
        {
            if (IS_QUERIED(gb_item, cbs)) count++;
        }
    }
    return count;
}

#if defined(DEVEL_RALF)
#warning replace awt_count_items by "method" of selector
#endif // DEVEL_RALF

static int awt_count_items(struct adaqbsstruct *cbs, AWT_QUERY_RANGE range) {
    int                     count    = 0;
    GBDATA                 *gb_main  = cbs->gb_main;
    const ad_item_selector *selector = cbs->selector;
    GB_transaction          ta(gb_main);

    for (GBDATA *gb_item_container = selector->get_first_item_container(gb_main, cbs->aws->get_root(), range);
         gb_item_container;
         gb_item_container = selector->get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = selector->get_first_item(gb_item_container);
             gb_item;
             gb_item = selector->get_next_item(gb_item))
        {
            count++;
        }
    }
    return count;
}

const int MAX_CRITERIA = int(sizeof(unsigned long)*8/AWT_QUERY_SORT_CRITERIA_BITS);

static AWT_QUERY_RESULT_ORDER split_sort_mask(unsigned long sort_mask, AWT_QUERY_RESULT_ORDER *order) {
    // splits the sort order bit mask 'sort_mask' into single sort criteria
    // (order[0] contains the primary sort criteria)
    //
    // Returns the first criteria matching
    // AWT_QUERY_SORT_BY_1STFIELD_CONTENT or AWT_QUERY_SORT_BY_HIT_DESCRIPTION
    // (or AWT_QUERY_SORT_NONE if no sort order defined)

    AWT_QUERY_RESULT_ORDER first = AWT_QUERY_SORT_NONE;

    for (int o = 0; o<MAX_CRITERIA; o++) {
        order[o] = AWT_QUERY_RESULT_ORDER(sort_mask&AWT_QUERY_SORT_CRITERIA_MASK);
        awt_assert(order[o] == (order[o]&AWT_QUERY_SORT_CRITERIA_MASK));

        if (first == AWT_QUERY_SORT_NONE) {
            if (order[o] & (AWT_QUERY_SORT_BY_1STFIELD_CONTENT|AWT_QUERY_SORT_BY_HIT_DESCRIPTION)) {
                first = order[o];
            }
        }

        sort_mask = sort_mask>>AWT_QUERY_SORT_CRITERIA_BITS;
    }
    return first;
}

static void awt_first_searchkey_changed_cb(AW_root *, AW_CL cl_cbs) {
    struct adaqbsstruct *cbs = reinterpret_cast<struct adaqbsstruct*>(cl_cbs);

    AWT_QUERY_RESULT_ORDER order[MAX_CRITERIA];
    AWT_QUERY_RESULT_ORDER first = split_sort_mask(cbs->sort_mask, order);

    if (first != AWT_QUERY_SORT_BY_HIT_DESCRIPTION) { // do we display values? 
        awt_query_update_list(NULL, cbs);
    }
}

inline bool keep_criteria(AWT_QUERY_RESULT_ORDER old_criteria, AWT_QUERY_RESULT_ORDER new_criteria) {
    return
        old_criteria  != AWT_QUERY_SORT_NONE &&     // do not keep 'unsorted' (it is no real criteria)
        (old_criteria != new_criteria        ||     // do not keep new criteria (added later)
         old_criteria == AWT_QUERY_SORT_REVERSE);   // reverse may occur several times -> always keep
}

static void awt_sort_order_changed_cb(AW_root *aw_root, AW_CL cl_cbs) {
    // adds the new selected sort order to the sort order mask
    // (removes itself, if previously existed in sort order mask)
    //
    // if 'unsorted' is selected -> delete sort order

    struct adaqbsstruct    *cbs          = reinterpret_cast<struct adaqbsstruct*>(cl_cbs);
    AWT_QUERY_RESULT_ORDER  new_criteria = (AWT_QUERY_RESULT_ORDER)aw_root->awar(cbs->awar_sort)->read_int();

    if (new_criteria == AWT_QUERY_SORT_NONE) {
        cbs->sort_mask = AWT_QUERY_SORT_NONE; // clear sort_mask
    }
    else {
        AWT_QUERY_RESULT_ORDER order[MAX_CRITERIA];
        split_sort_mask(cbs->sort_mask, order);

        int empty_or_same = 0;
        for (int o = 0; o<MAX_CRITERIA; o++) {
            if (!keep_criteria(order[o], new_criteria)) {
                empty_or_same++; // these criteria will be skipped below
            }
        }

        unsigned long new_sort_mask = 0;
        for (int o = MAX_CRITERIA-(empty_or_same>0 ? 1 : 2); o >= 0; o--) {
            if (keep_criteria(order[o], new_criteria)) {
                new_sort_mask = (new_sort_mask<<AWT_QUERY_SORT_CRITERIA_BITS)|order[o];
            }
        }
        cbs->sort_mask = (new_sort_mask<<AWT_QUERY_SORT_CRITERIA_BITS)|new_criteria; // add new primary key
    }
    awt_query_update_list(NULL, cbs);
}

struct awt_sort_query_hits_param {
    struct adaqbsstruct    *cbs;
    char                   *first_key;
    AWT_QUERY_RESULT_ORDER  order[MAX_CRITERIA];
};

inline int strNULLcmp(const char *str1, const char *str2) {
    return
        str1
        ? (str2 ? strcmp(str1, str2) : -1)
        : (str2 ? 1 : 0);
}

static int awt_sort_query_hits(const void *cl_item1, const void *cl_item2, void *cl_param) {
    awt_sort_query_hits_param *param = static_cast<awt_sort_query_hits_param*>(cl_param);

    GBDATA *gb_item1 = (GBDATA*)cl_item1;
    GBDATA *gb_item2 = (GBDATA*)cl_item2;

    struct adaqbsstruct    *cbs      = param->cbs;
    const ad_item_selector *selector = cbs->selector;

    int cmp = 0;

    for (int o = 0; o<MAX_CRITERIA && cmp == 0; o++) {
        AWT_QUERY_RESULT_ORDER criteria = param->order[o];

        switch (criteria) {
            case AWT_QUERY_SORT_NONE:
                o = MAX_CRITERIA; // don't sort further
                break;

            case AWT_QUERY_SORT_BY_1STFIELD_CONTENT: {
                char *field1 = GBT_read_as_string(gb_item1, param->first_key);
                char *field2 = GBT_read_as_string(gb_item2, param->first_key);

                cmp = strNULLcmp(field1, field2);

                free(field2);
                free(field1);
                break;
            }
            case AWT_QUERY_SORT_BY_NESTED_PID: {
                if (selector->parent_selector) {
                    GBDATA *gb_parent1 = selector->get_parent(gb_item1);
                    GBDATA *gb_parent2 = selector->get_parent(gb_item2);
                    char   *pid1       = AWT_get_item_id(cbs->gb_main, selector, gb_parent1);
                    char   *pid2       = AWT_get_item_id(cbs->gb_main, selector, gb_parent2);

                    cmp = strNULLcmp(pid1, pid2);

                    free(pid2);
                    free(pid1);
                }
                break;
            }
            case AWT_QUERY_SORT_BY_ID: {
                const char *id1 = GBT_read_char_pntr(gb_item1, selector->id_field);
                const char *id2 = GBT_read_char_pntr(gb_item2, selector->id_field);

                cmp = strcmp(id1, id2);
                break;
            }
            case AWT_QUERY_SORT_BY_MARKED:
                cmp = GB_read_flag(gb_item2)-GB_read_flag(gb_item1);
                break;

            case AWT_QUERY_SORT_BY_HIT_DESCRIPTION: {
                const char *id1   = GBT_read_char_pntr(gb_item1, selector->id_field);
                const char *id2   = GBT_read_char_pntr(gb_item2, selector->id_field);
                const char *info1 = reinterpret_cast<const char *>(GBS_read_hash(cbs->hit_description, id1));
                const char *info2 = reinterpret_cast<const char *>(GBS_read_hash(cbs->hit_description, id2));
                cmp = strNULLcmp(info1, info2);
                break;
            }
            case AWT_QUERY_SORT_REVERSE: {
                GBDATA *tmp = gb_item1; // swap items for following compares (this is a prefix revert!)
                gb_item1    = gb_item2;
                gb_item2    = tmp;
                break;
            }
        }
    }

    return cmp;
}

static long detectMaxNameLength(const char *key, long val, void *cl_len) {
    int *len  = (int*)cl_len;
    int  klen = strlen(key);

    if (klen>*len) *len = klen;
    return val;
}

long awt_query_update_list(void *dummy, struct adaqbsstruct *cbs) {
    AWUSE(dummy);
    GB_push_transaction(cbs->gb_main);

    // clear
    cbs->aws->clear_selection_list(cbs->result_id);

    AW_root         *aw_root = cbs->aws->get_root();
    AWT_QUERY_RANGE  range   = (AWT_QUERY_RANGE)aw_root->awar(cbs->awar_where)->read_int();

    // create array of hits
    long     count  = awt_count_queried_items(cbs, range);
    GBDATA **sorted = static_cast<GBDATA**>(malloc(count*sizeof(*sorted)));
    {
        long s = 0;

        for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
             gb_item_container;
             gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 gb_item;
                 gb_item = cbs->selector->get_next_item(gb_item))
            {
                if (IS_QUERIED(gb_item, cbs)) sorted[s++] = gb_item;
            }
        }
    }

    // sort hits

    bool show_value = true; // default
    awt_sort_query_hits_param param = { cbs, NULL, {} };
    param.first_key = cbs->aws->get_root()->awar(cbs->awar_keys[0])->read_string();

    if (cbs->sort_mask != AWT_QUERY_SORT_NONE) {    // unsorted -> don't sort
        AWT_QUERY_RESULT_ORDER main_criteria = split_sort_mask(cbs->sort_mask, param.order);

        if (main_criteria == AWT_QUERY_SORT_BY_HIT_DESCRIPTION) {
            show_value = false;
        }
        GB_sort((void**)sorted, 0, count, awt_sort_query_hits, &param);
    }

    // display hits

    int name_len = cbs->selector->item_name_length;
    if (name_len == -1) { // if name_len is unknown -> detect
        GBS_hash_do_loop(cbs->hit_description, detectMaxNameLength, &name_len);
    }

    long i;
    for (i = 0; i<count && i<AWT_MAX_QUERY_LIST_LEN; i++) {
        char *name = cbs->selector->generate_item_id(cbs->gb_main, sorted[i]);
        if (name) {
            char       *toFree = 0;
            const char *info;

            if (show_value) {
                toFree = GBT_read_as_string(sorted[i], param.first_key);
                if (toFree) {
                    if (strlen(toFree)>AWT_MAX_SHOWN_DATA_SIZE) {
                        size_t shortened_len = GBS_shorten_repeated_data(toFree);
                        if (shortened_len>AWT_MAX_SHOWN_DATA_SIZE) {
                            strcpy(toFree+AWT_MAX_SHOWN_DATA_SIZE-5, "[...]");
                        }
                    }
                }
                else {
                    toFree = GBS_global_string_copy("<%s has no data>", param.first_key);
                }
                info = toFree;
            }
            else {
                info = reinterpret_cast<const char *>(GBS_read_hash(cbs->hit_description, name));
                if (!info) info = "<no hit info>";
            }

            awt_assert(info);
            const char *line = GBS_global_string("%c %-*s :%s",
                                                 GB_read_flag(sorted[i]) ? '*' : ' ',
                                                 name_len, name,
                                                 info);

            cbs->aws->insert_selection(cbs->result_id, line, name);
            free(toFree);
            free(name);
        }
    }

    if (count>AWT_MAX_QUERY_LIST_LEN) {
        cbs->aws->insert_selection(cbs->result_id, "*****  List truncated  *****", "");
    }

    free(sorted);
    
    cbs->aws->insert_default_selection(cbs->result_id, "End of list", "");
    cbs->aws->update_selection_list(cbs->result_id);
    cbs->aws->get_root()->awar(cbs->awar_count)->write_int((long)count);
    GB_pop_transaction(cbs->gb_main);

    return count;
}
// Mark listed species
// mark = 1 -> mark listed
// mark | 8 -> don't change rest
void awt_do_mark_list(void *dummy, struct adaqbsstruct *cbs, long mark)
{
    AWUSE(dummy);
    GB_push_transaction(cbs->gb_main);

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, cbs->aws->get_root(), AWT_QUERY_ALL_SPECIES);
         gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, AWT_QUERY_ALL_SPECIES))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 gb_item;
                 gb_item = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item, cbs)) {
                        GB_write_flag(gb_item, mark&1);
                    }
                    else if ((mark&8) == 0) {
                        GB_write_flag(gb_item, 1-(mark&1));
                    }
                }
        }

    awt_query_update_list(0, cbs);
    GB_pop_transaction(cbs->gb_main);
}

void awt_unquery_all(void *dummy, struct adaqbsstruct *cbs) {
    AWUSE(dummy);
    GB_push_transaction(cbs->gb_main);
    GBDATA *gb_species;
    for (gb_species = GBT_first_species(cbs->gb_main);
                gb_species;
                gb_species = GBT_next_species(gb_species)) {
        CLEAR_QUERIED(gb_species, cbs);
    }
    awt_query_update_list(0, cbs);
    GB_pop_transaction(cbs->gb_main);
}

void awt_delete_species_in_list(void *dummy, struct adaqbsstruct *cbs)
{
    GB_begin_transaction(cbs->gb_main);
    long cnt = 0;

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, cbs->aws->get_root(), AWT_QUERY_ALL_SPECIES);
         gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, AWT_QUERY_ALL_SPECIES))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 gb_item;
                 gb_item = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item, cbs)) cnt++;
                }
        }

    sprintf(AW_ERROR_BUFFER, "Are you sure to delete %li %s", cnt, cbs->selector->items_name);
    if (aw_question(AW_ERROR_BUFFER, "OK,CANCEL")) {
        GB_abort_transaction(cbs->gb_main);
        return;
    }

    GB_ERROR error = 0;

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, cbs->aws->get_root(), AWT_QUERY_ALL_SPECIES);
         !error && gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, AWT_QUERY_ALL_SPECIES))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item, cbs)) {
                        error = GB_delete(gb_item);
                    }
                }
        }

    if (error) {
        GB_abort_transaction(cbs->gb_main);
        aw_message(error);
    }
    else {
        awt_query_update_list(dummy, cbs);
        GB_commit_transaction(cbs->gb_main);
    }
}

static GB_HASH *awt_create_ref_hash(const struct adaqbsstruct *cbs, const char *key, bool split_words) {
    GBDATA  *gb_ref       = cbs->gb_ref;
    bool     queried_only = cbs->expect_hit_in_ref_list;
    GB_HASH *hash         = GBS_create_hash(GBT_get_species_hash_size(gb_ref), GB_IGNORE_CASE);

    for (GBDATA  *gb_species = GBT_first_species(gb_ref);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        if (!queried_only || IS_QUERIED(gb_species, cbs)) {
            GBDATA *gb_name = GB_search(gb_species, key, GB_FIND);
            if (gb_name) {
                char *keyas = GB_read_as_string(gb_name);
                if (keyas && strlen(keyas)) {
                    if (split_words) {
                        char *t;
                        for (t = strtok(keyas, " "); t; t = strtok(0, " ")) {
                            if (t[0]) GBS_write_hash(hash, t, (long)gb_species);
                        }
                    }
                    else {
                        GBS_write_hash(hash, keyas, (long)gb_species);
                    }
                }
                free(keyas);
            }
        }
    }
    return hash;
}

//  ------------------------
//      class awt_query
//  ------------------------
typedef enum { ILLEGAL, AND, OR } awt_query_operator;

typedef enum {
    AQT_INVALID,
    AQT_EMPTY,
    AQT_NON_EMPTY,
    AQT_LOWER,
    AQT_GREATER,
    AQT_EXACT_MATCH,
    AQT_OCCURS,
    AQT_STARTS_WITH,
    AQT_ENDS_WITH,
    AQT_WILDCARD,
    AQT_REGEXPR,
    AQT_ACI,
} awt_query_type;

typedef enum {
    AQFT_EXPLICIT,                                  // query should match one explicit field
    AQFT_ANY_FIELD,                                 // query should match one field (no matter which)
    AQFT_ALL_FIELDS,                                // query should match all fields
} awt_query_field_type;

class awt_query {
    awt_query_operator  op;                         // operator (AND or OR)
    char               *key;                        // search key
    bool                Not;                        // true means "don't match"
    char               *query;                      // search expression

    GBDATA     *gb_main;
    const char *tree;                               // name of current default tree (needed for ACI)

    bool                 rek;                       // is 'key' hierarchical ?
    awt_query_field_type match_field;               // type of search key
    GBQUARK              keyquark;                  // valid only if match_field == AQFT_EXPLICIT
    awt_query_type       type;                      // type of 'query'
    struct {                                        // used for some values of 'type'
        string     str;
        GBS_regex *regexp;
        float      number;
    } xquery;

    mutable char *error;                            // set by matches(), once set all future matches fail
    mutable char *lastACIresult;                    // result of last ACI query

    awt_query *next;
    int        index;                               // number of query (0 = first query, 1 = second query); not always consecutive!

    // --------------------

    void initFields(struct adaqbsstruct *cbs, int idx, awt_query_operator aqo, AW_root *aw_root);
    awt_query() {}

    void       detect_query_type();
    awt_query *remove_tail();
    void       append(awt_query *tail);

public:

    awt_query(struct adaqbsstruct *cbs);

    ~awt_query() {
        free(key);
        free(query);
        free(lastACIresult);
        if (xquery.regexp) GBS_free_regexpr(xquery.regexp);
        free(error);
        delete next;
    }

    awt_query_operator getOperator() const { return op; }
    const char *getKey() const { return key; }
    bool applyNot(bool matched) const { return Not ? !matched : matched; }
    bool shallMatch() const { return !Not; }
    awt_query_type getType() const { return type; }
    int getIndex() const { return index; }

    bool matches(const char *data, GBDATA *gb_item) const;
    GB_ERROR getError(int count = 0) const;
    const char *get_last_ACI_result() const { return type == AQT_ACI ? lastACIresult : 0; }

    awt_query *getNext() { return next; }

    bool is_rek() const { return rek; }
    awt_query_field_type get_match_field() const { return match_field; }
    GBQUARK getKeyquark() const { return keyquark; }

    GBDATA *get_first_key(GBDATA *gb_item) const {
        GBDATA *gb_key = 0;

        if (is_rek()) {
            gb_key = GB_search(gb_item, getKey(), GB_FIND);
        }
        else if (match_field != AQFT_EXPLICIT) {
            gb_key = GB_child(gb_item);
            while (gb_key && GB_read_type(gb_key) == GB_DB) {
                gb_key = GB_nextChild(gb_key);
            }
        }
        else {
            gb_key = GB_find_sub_by_quark(gb_item, getKeyquark(), 0);
        }

        return gb_key;
    }

    void negate();

    bool prefers_sort_by_hit() {
        return
            match_field == AQFT_ALL_FIELDS ||
            match_field == AQFT_ANY_FIELD ||
            (next && next->prefers_sort_by_hit());
    }

#if defined(DEBUG)
    string dump_str() const { return string(key)+(Not ? "!=" : "==")+query; }
    void dump(string *prev = 0) const {
        string mine = dump_str();
        if (prev) {
            string both = *prev+' ';
            switch (op) {
                case AND: both += "&&"; break;
                case OR: both  += "||"; break;
                default: awt_assert(0); break;
            }
            both += ' '+mine;
            mine  = both;

            if (next) mine = '('+mine+')';
        }

        if (next) next->dump(&mine);
        else fputs(mine.c_str(), stdout);
    }
#endif // DEBUG
};

void awt_query::initFields(struct adaqbsstruct *cbs, int idx, awt_query_operator aqo, AW_root *aw_root) {
    awt_assert(aqo == OR || aqo == AND);

    error         = 0;
    lastACIresult = 0;
    next          = 0;
    index         = idx;
    xquery.regexp = 0;

    op    = aqo;
    key   = aw_root->awar(cbs->awar_keys[idx])->read_string();
    Not   = aw_root->awar(cbs->awar_not[idx])->read_int() != 0;
    query = aw_root->awar(cbs->awar_queries[idx])->read_string();

    gb_main = cbs->gb_main;
    tree    = cbs->tree_name;

    rek         = false;
    match_field = AQFT_EXPLICIT;
    keyquark    = -1;    
    if (GB_first_non_key_char(key)) {
        if (strcmp(key, PSEUDO_FIELD_ANY_FIELD) == 0) match_field = AQFT_ANY_FIELD;
        else if (strcmp(key, PSEUDO_FIELD_ALL_FIELDS) == 0) match_field = AQFT_ALL_FIELDS;
        else rek = true;
    }
    else {
        keyquark = GB_key_2_quark(gb_main, key);
    }

    detect_query_type();
}

awt_query::awt_query(struct adaqbsstruct *cbs) {
    AW_root *aw_root = cbs->aws->get_root();

    initFields(cbs, 0, OR, aw_root);                // initial value is false, (false OR QUERY1) == QUERY1

    awt_query *tail = this;
    for (size_t keyidx = 1; keyidx<AWT_QUERY_SEARCHES; ++keyidx) {
        char *opstr = aw_root->awar(cbs->awar_operator[keyidx])->read_string();

        if (strcmp(opstr, "ign") != 0) {            // not ignore
            awt_query_operator next_op = ILLEGAL;

            if (strcmp(opstr, "and")     == 0) next_op = AND;
            else if (strcmp(opstr, "or") == 0) next_op = OR;
#if defined(ASSERTION_USED)
            else aw_assert(0);
#endif // ASSERTION_USED

            if (next_op != ILLEGAL) {
                awt_query *next_query = new awt_query;

                next_query->initFields(cbs, keyidx, next_op, aw_root);

                tail->next = next_query;
                tail       = next_query;
            }
        }
        free(opstr);
    }
}

awt_query *awt_query::remove_tail() {
    awt_query *tail = 0;
    if (next) {
        awt_query *body_last = this;
        while (body_last->next && body_last->next->next) {
            body_last = body_last->next;
        }
        awt_assert(body_last->next);
        awt_assert(body_last->next->next == 0);
        
        tail            = body_last->next;
        body_last->next = 0;
    }
    return tail;
}

void awt_query::append(awt_query *tail) {
    awt_assert(this != tail);
    if (next) next->append(tail);
    else next = tail;
}

void awt_query::negate() {
    if (next) {
        awt_query *tail = remove_tail();

        negate();
        tail->negate();

        switch (tail->op) {
            case AND: tail->op = OR; break;
            case OR: tail->op  = AND; break;
            default: awt_assert(0); break;
        }

        append(tail);
    }
    else {
        Not = !Not;
        switch (match_field) {
            case AQFT_EXPLICIT: break;
            case AQFT_ALL_FIELDS: match_field = AQFT_ANY_FIELD; break; // not match allFields <=> mismatch anyField
            case AQFT_ANY_FIELD: match_field  = AQFT_ALL_FIELDS; break; // not match anyField <=> mismatch allFields
        }
    }
}

GB_ERROR awt_query::getError(int count) const {
    GB_ERROR err = error;
    error        = NULL;

    if (err) {
        err = GBS_global_string("%s (in query #%i)", err, count+1);
    }

    if (next) {
        if (err) {
            char *dup = strdup(err);

            err = next->getError(count+1);
            if (err) err = GBS_global_string("%s\n%s", dup, err);
            else err = GBS_global_string("%s", dup);
            free(dup);
        }
        else {
            err = next->getError(count+1);
        }
    }

    return err;
}


inline bool containsWildcards(const char *str) { return strpbrk(str, "*?") != 0; }
inline bool containsWildcards(const string& str) { return str.find_first_of("*?") != string::npos; }

void awt_query::detect_query_type() {
    char    first = query[0];
    string& str   = xquery.str;
    str           = query;

    type = AQT_INVALID;

    if (!first)            type = AQT_EMPTY;
    else if (first == '/') {
        GB_CASE     case_flag;
        GB_ERROR    err       = 0;
        const char *unwrapped = GBS_unwrap_regexpr(query, &case_flag, &err);
        if (unwrapped) {
            xquery.regexp = GBS_compile_regexpr(unwrapped, case_flag, &err);
            if (xquery.regexp) type = AQT_REGEXPR;
        }
        if (err) freedup(error, err);
    }
    else if (first == '|') type = AQT_ACI;
    else if (first == '<' || first == '>') {
        const char *rest = query+1;
        const char *end;
        float       f    = strtof(rest, const_cast<char**>(&end));

        if (end != rest) { // did convert part or all of rest to float
            if (end[0] == 0) { // all of rest has been converted
                type          = query[0] == '<' ? AQT_LOWER : AQT_GREATER;
                xquery.number = f;
            }
            else {
                freeset(error, GBS_global_string_copy("Could not convert '%s' to number (unexpected content '%s')", rest, end));
            }
        }
        // otherwise handle as non-special search string
    }

    if (type == AQT_INVALID && !error) {            // no type detected above
        if (containsWildcards(query)) {
            size_t qlen = strlen(query);
            char   last = query[qlen-1];

            if (first == '*') {
                if (last == '*') {
                    str  = string(str, 1, str.length()-2); // cut off first and last
                    type = str.length() ? AQT_OCCURS : AQT_NON_EMPTY;
                }
                else {
                    str  = string(str, 1);          // cut of first
                    type = AQT_ENDS_WITH;
                }
            }
            else {
                if (last == '*') {
                    str  = string(str, 0, str.length()-1); // cut of last
                    type = AQT_STARTS_WITH;
                }
                else type = AQT_WILDCARD;
            }

            if (type != AQT_WILDCARD && containsWildcards(str)) { // still contains wildcards -> fallback
                str  = query; 
                type = AQT_WILDCARD;
            }
        }
        else type = AQT_EXACT_MATCH;
    }

    awt_assert(type != AQT_INVALID || error);
}

bool awt_query::matches(const char *data, GBDATA *gb_item) const {
    // 'data' is the content of the searched field (read as string)
    // 'gb_item' is the DB item (e.g. species, gene). Used in ACI-search only.

    bool hit = false;

    awt_assert(data);
    awt_assert(gb_item);

    if (error) hit = false;
    else switch (type) {
        case AQT_EMPTY: {
            hit = (data[0] == 0);
            break;
        }
        case AQT_NON_EMPTY: { 
            hit = (data[0] != 0);
            break;
        }
        case AQT_EXACT_MATCH: {                     // exact match (but ignoring case)
            hit = strcasecmp(data, query) == 0;
            break;
        }
        case AQT_OCCURS: {                          // query expression occurs in data (equiv to '*expr*')
            hit = GBS_find_string(data, xquery.str.c_str(), 1) != 0;
            break;
        }
        case AQT_STARTS_WITH: {                     // data starts with query expression (equiv to 'expr*')
            hit = strncasecmp(data, xquery.str.c_str(), xquery.str.length()) == 0;
            break;
        }
        case AQT_ENDS_WITH: {                       // data ends with query expression (equiv to '*expr')
            int dlen = strlen(data);
            hit = strcasecmp(data+dlen-xquery.str.length(), xquery.str.c_str()) == 0;
            break;
        }
        case AQT_WILDCARD: {                        // expr contains wildcards (use GBS_string_matches for compare)
            hit = GBS_string_matches(data, query, GB_IGNORE_CASE);
            break;
        }
        case AQT_GREATER:                           // data is greater than query
        case AQT_LOWER: {                           // data is lower than query
            const char *start = data;
            while (start[0] == ' ') ++start;

            const char *end;
            float       f = strtof(start, const_cast<char**>(&end));

            if (end == start) { // nothing was converted
                hit = false;
            }
            else {
                bool is_numeric = (end[0] == 0);

                if (!is_numeric) {
                    while (end[0] == ' ') ++end;
                    is_numeric = (end[0] == 0);
                }
                if (is_numeric) {
                    hit = (type == AQT_GREATER)
                        ? f > xquery.number
                        : f < xquery.number;
                }
                else {
                    hit = false;
                }
            }
            break;
        }
        case AQT_REGEXPR: {                         // expr is a regexpr ('/.../')
            hit = GBS_regmatch_compiled(data, xquery.regexp, NULL) != 0;
            break;
        }
        case AQT_ACI: {                             // expr is a ACI ('|...'); result = "0" -> no hit; otherwise hit
            char *aci_result = GB_command_interpreter(gb_main, data, query, gb_item, tree);
            if (!aci_result) {
                freedup(error, GB_await_error());
                hit   = false;
            }
            else {
                hit = strcmp(aci_result, "0") != 0;
            }
            freeset(lastACIresult, aci_result);
            break;
        }
        case AQT_INVALID: {                     // invalid
            awt_assert(0);
            freedup(error, "Invalid search expression");
            hit   = false;
            break;
        }
    }
    return applyNot(hit);
}

static void awt_do_query(void *dummy, struct adaqbsstruct *cbs, AW_CL cl_ext_query) {
    AWUSE(dummy);

    GB_push_transaction(cbs->gb_main);

    AWT_EXT_QUERY_TYPES  ext_query = (AWT_EXT_QUERY_TYPES)cl_ext_query;
    AW_root             *aw_root   = cbs->aws->get_root();
    awt_query            query(cbs);

    AWT_QUERY_MODES mode  = (AWT_QUERY_MODES)aw_root->awar(cbs->awar_ere)->read_int();
    AWT_QUERY_RANGE range = (AWT_QUERY_RANGE)aw_root->awar(cbs->awar_where)->read_int();
    AWT_QUERY_TYPES type  = (AWT_QUERY_TYPES)aw_root->awar(cbs->awar_by)->read_int();

    if (cbs->gb_ref && type != AWT_QUERY_MARKED) {  // special for merge tool!
        char *first_query = aw_root->awar(cbs->awar_queries[0])->read_string();
        if (strlen(first_query) == 0) {
            if (!ext_query) ext_query  = AWT_EXT_QUERY_COMPARE_LINES;
        }
        free(first_query);
    }

    GB_ERROR error = query.getError();

    if (!error) {
        size_t   item_count     = awt_count_items(cbs, range);
        size_t   searched_count = 0;

        aw_openstatus("Searching");
        aw_status(0.0);

        if (cbs->gb_ref && // merge tool only
            (ext_query == AWT_EXT_QUERY_COMPARE_LINES || ext_query == AWT_EXT_QUERY_COMPARE_WORDS))
        {
            GB_push_transaction(cbs->gb_ref);
            const char *first_key = query.getKey();
            GB_HASH    *ref_hash  = awt_create_ref_hash(cbs, first_key, ext_query == AWT_EXT_QUERY_COMPARE_WORDS);

#if defined(DEBUG)
            printf("query: search identical %s in field %s%s\n",
                   (ext_query == AWT_EXT_QUERY_COMPARE_WORDS ? "words" : "values"),
                   first_key,
                   cbs->expect_hit_in_ref_list ? " of species listed in other hitlist" : "");
#endif // DEBUG

            for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
                 gb_item_container && !error;
                 gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
            {
                for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                     gb_item && !error;
                     gb_item = cbs->selector->get_next_item(gb_item))
                {
                    switch (mode) {
                        case AWT_QUERY_GENERATE: CLEAR_QUERIED(gb_item, cbs); break;
                        case AWT_QUERY_ENLARGE:  if (IS_QUERIED(gb_item, cbs)) continue; break;
                        case AWT_QUERY_REDUCE:   if (!IS_QUERIED(gb_item, cbs)) continue; break;
                    }
                    
                    GBDATA *gb_key = query.get_first_key(gb_item);

                    if (gb_key) {
                        char   *data        = GB_read_as_string(gb_key);
                        GBDATA *gb_ref_pntr = 0;

                        if (data && data[0]) {
                            string hit_reason;
                            bool   this_hit = false;

                            if (ext_query == AWT_EXT_QUERY_COMPARE_WORDS) {
                                for (char *t = strtok(data, " "); t; t = strtok(0, " ")) {
                                    gb_ref_pntr = (GBDATA *)GBS_read_hash(ref_hash, t);
                                    if (gb_ref_pntr) { // found item in other DB, with 'first_key' containing word from 'gb_key'
                                        this_hit   = true;
                                        hit_reason = GBS_global_string("%s%s has word '%s' in %s",
                                                                       cbs->expect_hit_in_ref_list ? "Hit " : "",
                                                                       cbs->selector->generate_item_id(cbs->gb_ref, gb_ref_pntr),
                                                                       t, first_key);
                                    }
                                }
                            }
                            else {
                                gb_ref_pntr = (GBDATA *)GBS_read_hash(ref_hash, data);
                                if (gb_ref_pntr) { // found item in other DB, with identical 'first_key' 
                                    this_hit   = true;
                                    hit_reason = GBS_global_string("%s%s matches %s",
                                                                   cbs->expect_hit_in_ref_list ? "Hit " : "",
                                                                   cbs->selector->generate_item_id(cbs->gb_ref, gb_ref_pntr),
                                                                   first_key);
                                }
                            }

                            if (type == AWT_QUERY_DONT_MATCH) {
                                this_hit = !this_hit;
                                if (this_hit) hit_reason = "<no matching entry>";
                            }

                            if (this_hit) {
                                awt_assert(!hit_reason.empty());
                                SET_QUERIED(gb_item, cbs, hit_reason.c_str(), hit_reason.length());
                            }
                            else CLEAR_QUERIED(gb_item, cbs);
                        }
                        free(data);
                    }

                    if (aw_status(searched_count++/double(item_count))) error = "aborted";
                }
            }

            GBS_free_hash(ref_hash);
            GB_pop_transaction(cbs->gb_ref);
        }
        else {                                          // "normal" query
            if (type == AWT_QUERY_DONT_MATCH) {
#if defined(DEBUG)
                fputs("query: !(", stdout); query.dump();
#endif // DEBUG

                query.negate();
                type = AWT_QUERY_MATCH;

#if defined(DEBUG)
                fputs(") => query: ", stdout); query.dump(); fputc('\n', stdout);
#endif // DEBUG
            }
#if defined(DEBUG)
            else { fputs("query: ", stdout); query.dump(); fputc('\n', stdout); }
#endif // DEBUG

            if (query.prefers_sort_by_hit()) {
                // automatically switch to sorting 'by hit' if 'any field' or 'all fields' is selected
                aw_root->awar(cbs->awar_sort)->write_int(AWT_QUERY_SORT_BY_HIT_DESCRIPTION);
            }

            for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
                 gb_item_container && !error;
                 gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
            {
                for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                     gb_item && !error;
                     gb_item = cbs->selector->get_next_item(gb_item))
                {
                    string hit_reason;

                    switch (mode) {
                        case AWT_QUERY_GENERATE: CLEAR_QUERIED(gb_item, cbs); break;
                        case AWT_QUERY_ENLARGE:  if (IS_QUERIED(gb_item, cbs)) continue; break;
                        case AWT_QUERY_REDUCE:   if (!IS_QUERIED(gb_item, cbs)) continue; break;
                    }

                    bool hit = false;

                    switch (type) {
                        case AWT_QUERY_MARKED: {
                            hit        = GB_read_flag(gb_item);
                            hit_reason = "<marked>";
                            break;
                        }
                        case AWT_QUERY_MATCH: {
                            for (awt_query *this_query = &query; this_query;  this_query = this_query ? this_query->getNext() : 0) { // iterate over all single queries
                                awt_query_operator this_op = this_query->getOperator();

                                if ((this_op == OR) == hit) {
                                    continue; // skip query Q for '1 OR Q' and for '0 AND Q' (result can't change)
                                }

                                string this_hit_reason;

                                GBDATA               *gb_key      = this_query->get_first_key(gb_item);
                                awt_query_field_type  match_field = this_query->get_match_field();
                                bool                  this_hit    = (match_field == AQFT_ALL_FIELDS);

                                char *data = gb_key ? GB_read_as_string(gb_key) : strdup(""); // assume "" for explicit fields that don't exist
                                awt_assert(data);

                                while (data) {
                                    awt_assert(ext_query == AWT_EXT_QUERY_NONE);
                                    bool        matched    = this_query->matches(data, gb_item); // includes not-op
                                    const char *reason_key = 0;

                                    switch (match_field) {
                                        case AQFT_EXPLICIT:
                                            this_hit   = matched;
                                            reason_key = this_query->getKey();
                                            break;

                                        case AQFT_ANY_FIELD:
                                            if (matched) {
                                                this_hit   = true;
                                                reason_key = GB_read_key_pntr(gb_key);
                                            }
                                            break;

                                        case AQFT_ALL_FIELDS:
                                            if (!matched) {
                                                this_hit   = false;
                                                reason_key = GB_read_key_pntr(gb_key);
                                            }
                                            break;
                                    }

                                    if (reason_key) {
                                        if (strlen(data)>AWT_MAX_SHOWN_DATA_SIZE) {
                                            size_t shortened_len = GBS_shorten_repeated_data(data);
                                            if (shortened_len>AWT_MAX_SHOWN_DATA_SIZE) {
                                                strcpy(data+AWT_MAX_SHOWN_DATA_SIZE-5, "[...]");
                                            }
                                        }
                                        this_hit_reason       = string(reason_key)+"="+data;
                                        const char *ACIresult = this_query->get_last_ACI_result();
                                        if (ACIresult) {
                                            this_hit_reason = string("[ACI=")+ACIresult+"] "+this_hit_reason;
                                        }
                                        gb_key = NULL; // stop!
                                    }

                                    freenull(data);
                                    if (gb_key) {
                                        do { gb_key = GB_nextChild(gb_key); }
                                        while (gb_key && GB_read_type(gb_key) == GB_DB);

                                        if (gb_key) {
                                            data = GB_read_as_string(gb_key);
                                            awt_assert(data);
                                        }
                                    }
                                }

                                if (this_hit && match_field == AQFT_ALL_FIELDS) {
                                    awt_assert(this_hit_reason.empty());
                                    this_hit_reason = this_query->shallMatch() ? "<matched all fields>" : "<matched no field>";
                                }

                            
                                if (this_hit) {
                                    awt_assert(!this_hit_reason.empty()); // if we got a hit, we also need a reason
                                    const char *prefix = GBS_global_string("%c%c", '1'+this_query->getIndex(), this_query->shallMatch() ? ' ' : '!');
                                    this_hit_reason    = string(prefix)+this_hit_reason;
                                }

                                // calculate result
                                // (Note: the operator of the 1st query is always OR)
                                switch (this_op) {
                                    case AND: {
                                        awt_assert(hit); // otherwise there was no need to run this sub-query
                                        hit        = this_hit;
                                        hit_reason = hit_reason.empty() ? this_hit_reason : hit_reason+" & "+this_hit_reason;
                                        break;
                                    }
                                    case OR: {
                                        awt_assert(!hit); // otherwise there was no need to run this sub-query
                                        hit        = this_hit;
                                        hit_reason = this_hit_reason;
                                        break;
                                    }
                                    default:
                                        awt_assert(0);
                                        break;
                                }
                                awt_assert(!hit || !hit_reason.empty()); // if we got a hit, we also need a reason
                            }
                            break;
                        }
                        default: awt_assert(0); break;
                    }

                    if (hit) {
                        awt_assert(!hit_reason.empty());

                        if (mode == AWT_QUERY_REDUCE) {
                            string prev_info = getHitInfo(gb_item, cbs);
                            hit_reason = prev_info+" (kept cause "+hit_reason+")";
                        }

                        SET_QUERIED(gb_item, cbs, hit_reason.c_str(), hit_reason.length());
                    }
                    else CLEAR_QUERIED(gb_item, cbs);

                    if (aw_status(searched_count++/double(item_count))) error = "aborted";
                }
            }
        }

        aw_closestatus();
    }

    if (!error) error = query.getError(); // check for query error

    if (error) aw_message(error);
    else awt_query_update_list(0, cbs);

    GB_pop_transaction(cbs->gb_main);
}

void awt_copy_selection_list_2_queried_species(struct adaqbsstruct *cbs, AW_selection_list *id, const char *hit_description) {
    GB_transaction ta(cbs->gb_main);

    awt_assert(strstr(hit_description, "%s")); // hit_description needs '%s' (which is replaced by visible content of 'id')

    GB_ERROR         error     = 0;
    AW_window       *aww       = cbs->aws;
    AW_root         *aw_root   = aww->get_root();
    GB_HASH         *list_hash = aww->selection_list_to_hash(id, false);
    AWT_QUERY_MODES  mode      = (AWT_QUERY_MODES)aw_root->awar(cbs->awar_ere)->read_int();
    AWT_QUERY_TYPES  type      = (AWT_QUERY_TYPES)aw_root->awar(cbs->awar_by)->read_int();

    if (type == AWT_QUERY_MARKED) {
        error = "Query mode 'that are marked' does not apply here.\nEither select 'that match the query' or 'that don't match the q.'";
    }

    if (type != AWT_QUERY_MATCH || mode != AWT_QUERY_GENERATE) { // different behavior as in the past -> advice
        AWT_advice("'Move to hitlist' now depends on the values selected for\n"
                   " * 'Search/Add/Keep species' and\n"
                   " * 'that match/don't match the query'\n"
                   "in the search tool.",
                   AWT_ADVICE_TOGGLE|AWT_ADVICE_HELP,
                   "Behavior changed",
                   "next_neighbours.hlp");
    }

    long inHitlist = GBS_hash_count_elems(list_hash);
    long seenInDB  = 0;

    for (GBDATA *gb_species = GBT_first_species(cbs->gb_main);
         gb_species && !error;
         gb_species = GBT_next_species(gb_species))
    {
        switch (mode) {
            case AWT_QUERY_GENERATE: CLEAR_QUERIED(gb_species, cbs); break;
            case AWT_QUERY_ENLARGE:  if (IS_QUERIED(gb_species, cbs)) continue; break;
            case AWT_QUERY_REDUCE:   if (!IS_QUERIED(gb_species, cbs)) continue; break;
        }

        const char *name      = GBT_get_name(gb_species);
        const char *displayed = reinterpret_cast<const char*>(GBS_read_hash(list_hash, name));

        if (displayed) seenInDB++;

        if ((displayed == 0) == (type == AWT_QUERY_DONT_MATCH)) {
            string hit_reason = GBS_global_string(hit_description, displayed ? displayed : "<no near neighbour>");
            
            if (mode == AWT_QUERY_REDUCE) {
                string prev_info = getHitInfo(gb_species, cbs);
                hit_reason = prev_info+" (kept cause "+hit_reason+")";
            }
            SET_QUERIED(gb_species, cbs, hit_reason.c_str(), hit_reason.length());
        }
        else {
            CLEAR_QUERIED(gb_species, cbs);
        }
    }

    if (seenInDB < inHitlist) {
        aw_message(GBS_global_string("%li of %li hits were found in database", seenInDB, inHitlist));
    }

    GBS_free_hash(list_hash);
    if (error) aw_message(error);
    awt_query_update_list(0, cbs);
}


void awt_search_equal_entries(AW_window *, struct adaqbsstruct *cbs, bool tokenize) {
    char     *key   = cbs->aws->get_root()->awar(cbs->awar_keys[0])->read_string();
    GB_ERROR  error = 0;

    if (strlen(key) == 0) {
        error = "Please select a key (in the first query expression)";
    }
    else {
        GB_transaction dumy(cbs->gb_main);

        GBDATA          *gb_species_data = GB_search(cbs->gb_main, "species_data", GB_CREATE_CONTAINER);
        AW_root         *aw_root         = cbs->aws->get_root();
        long             hashsize;
        AWT_QUERY_RANGE  range           = AWT_QUERY_ALL_SPECIES;
        AWT_QUERY_TYPES  type            = (AWT_QUERY_TYPES)aw_root->awar(cbs->awar_by)->read_int();

        switch (cbs->selector->type) {
            case AWT_QUERY_ITEM_SPECIES: {
                hashsize = GB_number_of_subentries(gb_species_data);
                break;
            }
            case AWT_QUERY_ITEM_EXPERIMENTS:
            case AWT_QUERY_ITEM_GENES: {
                // handle species sub-items
                hashsize = 0;

                for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, cbs->aws->get_root(), range);
                     gb_item_container;
                     gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
                {
                    hashsize += GB_number_of_subentries(gb_item_container);
                }

                break;
            }
            default: {
                awt_assert(0);
                hashsize = 0;
                break;
            }
        }

        if (!hashsize) {
            error = "No items exist";
        }
        else if (type == AWT_QUERY_MARKED) {
            error = "'that are marked' is not applicable here";
        }

        if (!error) {
            GB_HASH *hash = GBS_create_hash(hashsize, GB_IGNORE_CASE);

            for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
                 gb_item_container;
                 gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
            {
                for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                     gb_item;
                     gb_item = cbs->selector->get_next_item(gb_item))
                {
                    CLEAR_QUERIED(gb_item, cbs);
                    GB_write_flag(gb_item, 0);

                    GBDATA *gb_key = GB_search(gb_item, key, GB_FIND);  if (!gb_key) continue;
                    char   *data   = GB_read_as_string(gb_key);         if (!data) continue;

                    if (tokenize) {
                        char *s;
                        for (s=strtok(data, ",; \t."); s; s = strtok(0, ",; \t.")) {
                            GBDATA *gb_old = (GBDATA *)GBS_read_hash(hash, s);
                            if (gb_old) {
                                const char *oldInfo   = 0;
                                char       *firstInfo = GBS_global_string_copy("1st=%s", s);

                                if (IS_QUERIED(gb_old, cbs)) {
                                    const char *prevInfo = getHitInfo(gb_old, cbs);

                                    if (strstr(prevInfo, firstInfo) == 0) { // not already have 1st-entry here
                                        oldInfo = GBS_global_string("%s %s", prevInfo, firstInfo);
                                    }
                                }
                                else {
                                    oldInfo = firstInfo;
                                }
                                
                                if (oldInfo) SET_QUERIED(gb_old, cbs, oldInfo);
                                SET_QUERIED(gb_item, cbs, GBS_global_string("dup=%s", s));
                                GB_write_flag(gb_item, 1);

                                free(firstInfo);
                            }
                            else {
                                GBS_write_hash(hash, s, (long)gb_item);
                            }
                        }
                    }
                    else {
                        GBDATA *gb_old = (GBDATA *)GBS_read_hash(hash, data);
                        if (gb_old) {
                            if (!IS_QUERIED(gb_old, cbs)) {
                                SET_QUERIED(gb_old, cbs, GBS_global_string("%s (1st)", data));
                            }
                            SET_QUERIED(gb_item, cbs, GBS_global_string("%s (duplicate)", data));
                            GB_write_flag(gb_item, 1);
                        }
                        else {
                            GBS_write_hash(hash, data, (long)gb_item);
                        }
                    }

                    free(data);
                }

                if (type == AWT_QUERY_DONT_MATCH) {
                    for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                         gb_item;
                         gb_item = cbs->selector->get_next_item(gb_item))
                    {
                        if (IS_QUERIED(gb_item, cbs)) {
                            CLEAR_QUERIED(gb_item, cbs);
                            GB_write_flag(gb_item, 0); // unmark
                        }
                        else {
                            SET_QUERIED(gb_item, cbs, tokenize ? "<entry with unique words>" : "<unique entry>");
                        }
                    }
                }
            }

            GBS_free_hash(hash);
        }

        if (type != AWT_QUERY_MATCH) {
            AWT_advice("'Find equal entries' now depends on the values selected for\n"
                       " * 'that match/don't match the query'\n"
                       "in the search tool.",
                       AWT_ADVICE_TOGGLE|AWT_ADVICE_HELP,
                       "Behavior changed",
                       "search_duplicates.hlp");
        }
    }

    free(key);

    if (error) aw_message(error);
    awt_query_update_list(0, cbs);
}

/***************** Pars fields *************************/

void awt_do_pars_list(void *dummy, struct adaqbsstruct *cbs)
{
    AWUSE(dummy);
    GB_ERROR  error = 0;
    char     *key   = cbs->aws->get_root()->awar(cbs->awar_parskey)->read_string();

    if (!strcmp(key, "name")) {
        switch (cbs->selector->type) {
            case AWT_QUERY_ITEM_SPECIES: {
                if (aw_question("WARNING WARNING WARNING!!! You now try to rename the species\n"
                                "    The name is used to link database entries and trees\n"
                                "    ->  ALL TREES WILL BE LOST\n"
                                "    ->  The new name MUST be UNIQUE"
                                "        if not you will corrupt the database!",
                                "Let's Go,Cancel")) return;
                break;
            }
            case AWT_QUERY_ITEM_GENES: {
                if (aw_question("WARNING! You now try to rename the gene\n"
                                "    ->  Pseudo-species will loose their link to the gene"
                                "    ->  The new name MUST be UNIQUE"
                                "        if not you will corrupt the database!",
                                "Let's Go,Cancel")) return;
                break;
            }
            case AWT_QUERY_ITEM_EXPERIMENTS: {
                if (aw_question("WARNING! You now try to rename the experiment\n"
                                "    ->  The new name MUST be UNIQUE"
                                "        if not you will corrupt the database!",
                                "Let's Go,Cancel")) return;
                break;
            }
            default: {
                awt_assert(0);
                return;
            }
        }
    }


    if (!strlen(key)) error = "Please select a valid key";

    char *command = cbs->aws->get_root()->awar(cbs->awar_parsvalue)->read_string();
    if (!error && !strlen(command)) {
        error = "Please enter your command";
    }
    if (!error) {
        GB_begin_transaction(cbs->gb_main);
        
        GBDATA *gb_key_name;
        {
            GBDATA *gb_key_data = GB_search(cbs->gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);
            while (!error && !(gb_key_name = GB_find_string(gb_key_data, CHANGEKEY_NAME, key, GB_IGNORE_CASE, SEARCH_GRANDCHILD))) {
                const char *question = GBS_global_string("The destination field '%s' does not exists", key);
                if (aw_question(question, "Create Field (Type STRING),Cancel")) {
                    error = "Aborted by user";
                }
                else {
                    error = GBT_add_new_changekey_to_keypath(cbs->gb_main, key, GB_STRING, cbs->selector->change_key_path);
                }
            }
        }

        GBDATA *gb_key_type;
        if (!error) {
            gb_key_type = GB_brother(gb_key_name, CHANGEKEY_TYPE);
            
            if (!gb_key_type) error = GB_await_error();
            else {
                if (GB_read_int(gb_key_type)!=GB_STRING &&
                    aw_question("Writing to a non-STRING database field may lead to conversion problems.", "Abort,Continue")==0)
                {
                    error = "Aborted by user";
                }
            }
        }

        if (!error) {
            long  count  = 0;
            long  ncount = cbs->aws->get_root()->awar(cbs->awar_count)->read_int();
            char *deftag = cbs->aws->get_root()->awar(cbs->awar_deftag)->read_string();
            char *tag    = cbs->aws->get_root()->awar(cbs->awar_tag)->read_string();

            {
                long use_tag = cbs->aws->get_root()->awar(cbs->awar_use_tag)->read_int();
                if (!use_tag || !strlen(tag)) {
                    freenull(tag);
                }
            }
            int double_pars = cbs->aws->get_root()->awar(cbs->awar_double_pars)->read_int();

            aw_openstatus("Pars Fields");

            AW_root         *aw_root = cbs->aws->get_root();
            AWT_QUERY_RANGE  range   = (AWT_QUERY_RANGE)aw_root->awar(cbs->awar_where)->read_int();

            for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
                 !error && gb_item_container;
                 gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
            {
                for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                     !error && gb_item;
                     gb_item = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item, cbs)) {
                        if (aw_status((count++)/(double)ncount)) {
                            error = "Aborted by user";
                            break;
                        }
                        GBDATA *gb_new = GB_search(gb_item, key, GB_FIND);
                        char   *str    = gb_new ? GB_read_as_string(gb_new) : strdup("");
                        char   *parsed = 0;

                        if (double_pars) {
                            char *com2 = GB_command_interpreter(cbs->gb_main, str, command, gb_item, cbs->tree_name);
                            if (com2) {
                                if (tag) parsed = GBS_string_eval_tagged_string(cbs->gb_main, "", deftag, tag, 0, com2, gb_item);
                                else parsed     = GB_command_interpreter       (cbs->gb_main, "", com2, gb_item, cbs->tree_name);
                            }
                            free(com2);
                        }
                        else {
                            if (tag) parsed = GBS_string_eval_tagged_string(cbs->gb_main, str, deftag, tag, 0, command, gb_item);
                            else parsed     = GB_command_interpreter       (cbs->gb_main, str, command, gb_item, cbs->tree_name);
                        }

                        if (!parsed) error = GB_await_error();
                        else {
                            if (strcmp(parsed, str) != 0) { // any change?
                                if (gb_new && parsed[0] == 0) { // empty result -> delete field
                                    error = GB_delete(gb_new);
                                }
                                else {
                                    if (!gb_new) {
                                        gb_new = GB_search(gb_item, key, (GB_TYPES)GB_read_int(gb_key_type));
                                        if (!gb_new) error = GB_await_error();
                                    }
                                    if (!error) error = GB_write_as_string(gb_new, parsed);
                                }
                            }
                            free(parsed);
                        }
                        free(str);
                    }
                }
            }


            aw_closestatus();
            delete tag;
            free(deftag);
        }

        error = GB_end_transaction(cbs->gb_main, error);
    }
    
    if (error) aw_message(error);

    free(key);
    free(command);
}

void awt_predef_prg(AW_root *aw_root, struct adaqbsstruct *cbs) {
    char *str = aw_root->awar(cbs->awar_parspredefined)->read_string();
    char *brk = strchr(str, '#');
    if (brk) {
        *(brk++) = 0;
        char *kv = str;
        if (!strcmp(str, "ali_*/data")) {
            GB_transaction valid_transaction(cbs->gb_main);
            char *use = GBT_get_default_alignment(cbs->gb_main);
            kv = GBS_global_string_copy("%s/data", use);
            free(use);
        }
        aw_root->awar(cbs->awar_parskey)->write_string(kv);
        if (kv != str) free(kv);
        aw_root->awar(cbs->awar_parsvalue)->write_string(brk);
    }
    else {
        aw_root->awar(cbs->awar_parsvalue)->write_string(str);
    }
    free(str);
}

char *AWT_get_item_id(GBDATA *gb_main, const ad_item_selector *sel, GBDATA *gb_item) {
    return sel->generate_item_id(gb_main, gb_item);
}

GBDATA *AWT_get_item_with_id(GBDATA *gb_main, const ad_item_selector *sel, const char *id) {
    return sel->find_item_by_id(gb_main, id);
}

static void awt_colorize_listed(AW_window * /* dummy */, AW_CL cl_cbs) {
    struct adaqbsstruct *cbs         = (struct adaqbsstruct *)cl_cbs;
    GB_transaction       trans_dummy(cbs->gb_main);
    GB_ERROR             error       = 0;
    AW_root             *aw_root     = cbs->aws->get_root();
    int                  color_group = aw_root->awar(AWAR_COLORIZE)->read_int();
    AWT_QUERY_RANGE      range       = (AWT_QUERY_RANGE)aw_root->awar(cbs->awar_where)->read_int();

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item       = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item, cbs)) {
                        error = AW_set_color_group(gb_item, color_group);
                    }
                }
        }

    if (error) GB_export_error(error);
}

struct color_mark_data {
    const ad_item_selector *sel;
    GBDATA                 *gb_main;
};

static void awt_colorize_marked(AW_window *aww, AW_CL cl_cmd) {
    const color_mark_data  *cmd         = (struct color_mark_data *)cl_cmd;
    const ad_item_selector *sel         = cmd->sel;
    GB_transaction          trans_dummy(cmd->gb_main);
    GB_ERROR                error       = 0;
    AW_root                *aw_root     = aww->get_root();
    int                     color_group = aw_root->awar(AWAR_COLORIZE)->read_int();
    AWT_QUERY_RANGE         range       = AWT_QUERY_ALL_SPECIES; // @@@ FIXME: make customizable

    for (GBDATA *gb_item_container = sel->get_first_item_container(cmd->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = sel->get_next_item_container(gb_item_container, range))
        {
            for (GBDATA *gb_item = sel->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item       = sel->get_next_item(gb_item))
                {
                    if (GB_read_flag(gb_item)) {
                        error = AW_set_color_group(gb_item, color_group);
                    }
                }
        }

    if (error) GB_export_error(error);
}

// @@@ awt_mark_colored is obsolete! (will be replaced by dynamic coloring in the future)
static void awt_mark_colored(AW_window *aww, AW_CL cl_cmd, AW_CL cl_mode) {
    const color_mark_data  *cmd         = (struct color_mark_data *)cl_cmd;
    const ad_item_selector *sel         = cmd->sel;
    int                     mode        = int(cl_mode); // 0 = unmark 1 = mark 2 = invert
    AW_root                *aw_root     = aww->get_root();
    int                     color_group = aw_root->awar(AWAR_COLORIZE)->read_int();
    AWT_QUERY_RANGE         range       = AWT_QUERY_ALL_SPECIES; // @@@ FIXME: make customizable

    GB_transaction trans_dummy(cmd->gb_main);

    for (GBDATA *gb_item_container = sel->get_first_item_container(cmd->gb_main, aw_root, range);
         gb_item_container;
         gb_item_container = sel->get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = sel->get_first_item(gb_item_container);
             gb_item;
             gb_item = sel->get_next_item(gb_item))
        {
            long my_color = AW_find_color_group(gb_item, true);
            if (my_color == color_group) {
                bool marked = GB_read_flag(gb_item);

                switch (mode) {
                    case 0: marked = 0; break;
                    case 1: marked = 1; break;
                    case 2: marked = !marked; break;
                    default: awt_assert(0); break;
                }

                GB_write_flag(gb_item, marked);
            }
        }
    }

}

// --------------------------------------------------------------------------------
// color sets

struct color_save_data {
    color_mark_data   *cmd;
    const char        *items_name;
    AW_window         *aww; // window of selection list
    AW_selection_list *sel_id;
};

#define AWT_COLOR_LOADSAVE_NAME "tmp/colorset/name"

static GBDATA *get_colorset_root(const color_save_data *csd) {
    GBDATA *gb_main      = csd->cmd->gb_main;
    GBDATA *gb_colorsets = GB_search(gb_main, "colorsets", GB_CREATE_CONTAINER);
    GBDATA *gb_item_root = GB_search(gb_colorsets, csd->items_name, GB_CREATE_CONTAINER);

    awt_assert(gb_item_root);
    return gb_item_root;
}

static void update_colorset_selection_list(const color_save_data *csd) {
    GB_transaction  ta(csd->cmd->gb_main);
    AW_window      *aww = csd->aww;

    aww->clear_selection_list(csd->sel_id);
    GBDATA *gb_item_root = get_colorset_root(csd);

    for (GBDATA *gb_colorset = GB_entry(gb_item_root, "colorset");
         gb_colorset;
         gb_colorset = GB_nextEntry(gb_colorset))
    {
        const char *name = GBT_read_name(gb_colorset);
        aww->insert_selection(csd->sel_id, name, name);
    }
    aww->insert_default_selection(csd->sel_id, "<new colorset>", "");

    aww->update_selection_list(csd->sel_id);
}

static void colorset_changed_cb(GBDATA*, int *cl_csd, GB_CB_TYPE cbt) {
    if (cbt&GB_CB_CHANGED) {
        const color_save_data *csd = (const color_save_data*)cl_csd;
        update_colorset_selection_list(csd);
    }
}

static char *create_colorset_representation(const color_save_data *csd, GB_ERROR& error) {
    const color_mark_data  *cmd     = csd->cmd;
    const ad_item_selector *sel     = cmd->sel;
    AWT_QUERY_RANGE         range   = AWT_QUERY_ALL_SPECIES;
    AW_root                *aw_root = csd->aww->get_root();
    GBDATA                 *gb_main = cmd->gb_main;

    typedef list<string> ColorList;
    ColorList             cl;

    for (GBDATA *gb_item_container = sel->get_first_item_container(cmd->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = sel->get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = sel->get_first_item(gb_item_container);
             !error && gb_item;
             gb_item = sel->get_next_item(gb_item))
        {
            long        color     = AW_find_color_group(gb_item, true);
            char       *id        = sel->generate_item_id(gb_main, gb_item);
            const char *color_def = GBS_global_string("%s=%li", id, color);
            cl.push_front(color_def);
            free(id);
        }
    }

    char *result = 0;
    if (cl.empty()) {
        error = GBS_global_string("Could not find any %s", sel->items_name);
    }
    else {
        string res;
        for (ColorList::iterator ci = cl.begin(); ci != cl.end(); ++ci) {
            res += (*ci) + ';';
        }

        result               = (char*)malloc(res.length());
        memcpy(result, res.c_str(), res.length()-1); // skip trailing ';'
        result[res.length()] = 0;
    }
    return result;
}

static GB_ERROR clear_all_colors(const color_save_data *csd) {
    const color_mark_data  *cmd     = csd->cmd;
    const ad_item_selector *sel     = cmd->sel;
    AWT_QUERY_RANGE         range   = AWT_QUERY_ALL_SPECIES;
    AW_root                *aw_root = csd->aww->get_root();
    GB_ERROR                error   = 0;

    for (GBDATA *gb_item_container = sel->get_first_item_container(cmd->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = sel->get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = sel->get_first_item(gb_item_container);
             !error && gb_item;
             gb_item = sel->get_next_item(gb_item))
        {
            error = AW_set_color_group(gb_item, 0); // clear colors
        }
    }

    return error;
}

static void awt_clear_all_colors(AW_window *, AW_CL cl_csd) {
    const color_save_data *csd   = (const color_save_data*)cl_csd;
    GB_transaction         ta(csd->cmd->gb_main);
    GB_ERROR               error = clear_all_colors(csd);

    if (error) {
        error = ta.close(error);
        aw_message(error);
    }
}

static GB_ERROR restore_colorset_representation(const color_save_data *csd, const char *colorset) {
    const color_mark_data  *cmd     = csd->cmd;
    const ad_item_selector *sel     = cmd->sel;
    GBDATA                 *gb_main = cmd->gb_main;

    int   buffersize = 200;
    char *buffer     = (char*)malloc(buffersize);

    while (colorset) {
        const char *equal = strchr(colorset, '=');
        awt_assert(equal);
        const char *semi  = strchr(equal, ';');

        int size = equal-colorset;
        if (size >= buffersize) {
            buffersize = int(size*1.5);
            freeset(buffer, (char*)malloc(buffersize));
        }

        awt_assert(buffer && buffersize>size);
        memcpy(buffer, colorset, size);
        buffer[size] = 0;       // now buffer contains the item id

        GBDATA *gb_item = AWT_get_item_with_id(gb_main, sel, buffer);
        if (!gb_item) {
            aw_message(GBS_global_string("No such %s: '%s'", sel->item_name, buffer));
        }
        else {
            int color_group = atoi(equal+1);
            AW_set_color_group(gb_item, color_group);
        }
        colorset = semi ? semi+1 : 0;
    }

    free(buffer);
    return 0;
}

static void awt_loadsave_colorset(AW_window *aws, AW_CL cl_csd, AW_CL cl_mode) {
    const color_save_data *csd     = (const color_save_data*)cl_csd;
    int                    mode    = (int)cl_mode;  // 0 = save; 1 = load; 2 = overlay; 3 = delete;
    AW_root               *aw_root = aws->get_root();
    char                  *name    = aw_root->awar(AWT_COLOR_LOADSAVE_NAME)->read_string();
    GB_ERROR               error   = 0;

    if (name[0] == 0) {
        error = "Please enter a name for the colorset.";
    }
    else {
        GB_transaction ta(csd->cmd->gb_main);
        GBDATA *gb_colorset_root = get_colorset_root(csd);
        awt_assert(gb_colorset_root);

        GBDATA *gb_colorset_name = GB_find_string(gb_colorset_root, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
        GBDATA *gb_colorset      = gb_colorset_name ? GB_get_father(gb_colorset_name) : 0;
        if (mode == 0) {        // save-mode
            if (!gb_colorset) { // create new (otherwise overwrite w/o comment)
                gb_colorset             = GB_create_container(gb_colorset_root, "colorset");
                if (!gb_colorset) error = GB_await_error();
                else error              = GBT_write_string(gb_colorset, "name", name);
            }
            if (!error) {
                char *colorset = create_colorset_representation(csd, error);
                if (colorset) {
                    error = GBT_write_string(gb_colorset, "color_set", colorset);
                    free(colorset);
                }
            }
        }
        else { // load/delete
            if (!gb_colorset) {
                error = GBS_global_string("Colorset '%s' not found", name);
            }
            else {
                if (mode == 1 || mode == 2) { // load- and overlay-mode
                    GBDATA *gb_set = GB_entry(gb_colorset, "color_set");
                    if (!gb_set) {
                        error = "colorset is empty (oops)";
                    }
                    else {
                        const char *colorset = GB_read_char_pntr(gb_set);
                        if (!colorset) error = GB_await_error();
                        else {
                            if (mode == 1) { // load => clear all colors
                                error = clear_all_colors(csd);
                            }
                            if (!error) {
                                error = restore_colorset_representation(csd, colorset);
                            }
                        }
                    }
                }
                else {
                    awt_assert(mode == 3); // delete-mode
                    error = GB_delete(gb_colorset);
                }
            }
        }

        error = ta.close(error);
    }
    free(name);

    if (error) aw_message(error);
}

static AW_window *awt_create_loadsave_colored_window(AW_root *aw_root, AW_CL cl_csd) {
    static AW_window **aw_loadsave = 0;
    if (!aw_loadsave) {
        // init data
        aw_root->awar_string(AWT_COLOR_LOADSAVE_NAME, "", AW_ROOT_DEFAULT);
        aw_loadsave = (AW_window**)GB_calloc(AWT_QUERY_ITEM_TYPES, sizeof(*aw_loadsave)); // contains loadsave windows for each item type
    }

    color_save_data     *csd  = (color_save_data*)cl_csd;
    AWT_QUERY_ITEM_TYPE  type = csd->cmd->sel->type;

    awt_assert(type<AWT_QUERY_ITEM_TYPES);

    if (!aw_loadsave[type]) {
        // init window
        AW_window_simple *aws = new AW_window_simple;
        {
            char *window_id = GBS_global_string_copy("colorset_loadsave_%s", csd->items_name);
            aws->init(aw_root, window_id, GBS_global_string("Load/Save %s colorset", csd->items_name));
            free(window_id);
        }

        aws->load_xfig("color_loadsave.fig");

        aws->at("close");
        aws->callback((AW_CB0) AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(AW_POPUP_HELP, (AW_CL)"color_loadsave.hlp");
        aws->create_button("HELP", "HELP", "H");

        aws->at("name");
        aws->create_input_field(AWT_COLOR_LOADSAVE_NAME, 60);

        awt_assert(csd->sel_id == 0);
        aws->at("list");
        csd->sel_id = aws->create_selection_list(AWT_COLOR_LOADSAVE_NAME, 0, 0, 0, 0);
        csd->aww    = aws;

        update_colorset_selection_list(csd);

        aws->at("save");
        aws->callback(awt_loadsave_colorset, (AW_CL)csd, (AW_CL)0); // 0 = save
        aws->create_button("save", "Save", "S");

        aws->at("load");
        aws->callback(awt_loadsave_colorset, (AW_CL)csd, (AW_CL)1); // 1 = load
        aws->create_button("load", "Load", "L");

        aws->at("overlay");
        aws->callback(awt_loadsave_colorset, (AW_CL)csd, (AW_CL)2); // 2 = overlay
        aws->create_button("overlay", "Overlay", "O");

        aws->at("delete");
        aws->callback(awt_loadsave_colorset, (AW_CL)csd, (AW_CL)3); // 3 = delete
        aws->create_button("delete", "Delete", "D");

        aws->at("reset");
        aws->callback(awt_clear_all_colors, (AW_CL)csd);
        aws->create_button("reset", "Reset", "R");

        // callbacks
        {
            GB_transaction  ta(csd->cmd->gb_main);
            GBDATA         *gb_colorset = get_colorset_root(csd);
            GB_add_callback(gb_colorset, GB_CB_CHANGED, colorset_changed_cb, (int*)csd);
        }

        aw_loadsave[type] = aws;
    }

    return aw_loadsave[type];
}

static const char *color_group_name(int color_group) {
    static char buf[30];

    if (color_group) {
        sprintf(buf, "color group %i", color_group);
    }
    else {
        strcpy(buf, "no color group");
    }

    return buf;
}

static AW_window *create_awt_colorizer_window(AW_root *aw_root, GBDATA *gb_main, struct adaqbsstruct *cbs, const ad_item_selector *sel) {
    // invoked by   'colorize listed'                   (sel != 0)
    // and          'colorize marked/mark colored'      (cbs != 0)

    enum { AWT_COL_INVALID, AWT_COL_COLORIZE_LISTED, AWT_COL_COLORIZE_MARKED } mode = AWT_COL_INVALID;

    awt_query_create_global_awars(aw_root, AW_ROOT_DEFAULT);

    AW_window_simple *aws = new AW_window_simple;

    if (cbs) {
        awt_assert(mode == AWT_COL_INVALID);
        mode = AWT_COL_COLORIZE_LISTED;
    }
    if (sel) {
        awt_assert(mode == AWT_COL_INVALID);
        mode = AWT_COL_COLORIZE_MARKED;
    }
    awt_assert(!(mode == AWT_COL_INVALID));

    const ad_item_selector *Sel  = mode == AWT_COL_COLORIZE_LISTED ? cbs->selector : sel;
    const char             *what = mode == AWT_COL_COLORIZE_LISTED ? "listed" : "marked";

    {
        char *macro_name  = GBS_global_string_copy("COLORIZE_%s_%s", what, Sel->items_name);
        char *window_name = GBS_global_string_copy("Colorize %s %s", what, Sel->items_name);

        aws->init(aw_root, macro_name, window_name);

        free(window_name);
        free(macro_name);
    }

    aws->load_xfig("colorize.fig");

    aws->auto_space(10, 10);

    aws->at("close");
    aws->callback((AW_CB0) AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    if (mode == AWT_COL_COLORIZE_LISTED) aws->callback(AW_POPUP_HELP, (AW_CL)"set_color_of_listed.hlp");
    else                                 aws->callback(AW_POPUP_HELP, (AW_CL)"colorize.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at("colorize");

    color_mark_data *cmd = new color_mark_data; // do not free!
    cmd->sel             = (mode == AWT_COL_COLORIZE_MARKED) ? sel : cbs->selector;
    cmd->gb_main = gb_main;

    if (mode == AWT_COL_COLORIZE_LISTED)    aws->callback(awt_colorize_listed, (AW_CL)cbs);
    else                                    aws->callback(awt_colorize_marked, (AW_CL)cmd);

    aws->create_autosize_button("COLORIZE", GBS_global_string_copy("Set color of %s %s to ...", what, Sel->items_name), "S", 2);

    {
        int color_group;

        aws->create_option_menu(AWAR_COLORIZE);
        aws->insert_default_option(color_group_name(0), "none", 0);
        for (color_group = 1; color_group <= AW_COLOR_GROUPS; ++color_group) {
            aws->insert_option(color_group_name(color_group), "", color_group);
        }
        aws->update_option_menu();
    }

    color_save_data *csd = new color_save_data; // do not free!
    csd->cmd             = cmd;
    csd->items_name      = Sel->items_name;
    csd->aww             = 0;
    csd->sel_id          = 0;

    aws->at("loadsave");
    aws->callback(AW_POPUP, (AW_CL)awt_create_loadsave_colored_window, (AW_CL)csd);
    aws->create_autosize_button("LOADSAVE_COLORED", "Load/Save", "L");

    if (mode == AWT_COL_COLORIZE_MARKED) {
        aws->at("mark");
        aws->callback(awt_mark_colored, (AW_CL)cmd, (AW_CL)1);
        aws->create_autosize_button("MARK_COLORED", GBS_global_string_copy("Mark all %s of ...", Sel->items_name), "M", 2);

        aws->at("unmark");
        aws->callback(awt_mark_colored, (AW_CL)cmd, (AW_CL)0);
        aws->create_autosize_button("UNMARK_COLORED", GBS_global_string_copy("Unmark all %s of ...", Sel->items_name), "U", 2);

        aws->at("invert");
        aws->callback(awt_mark_colored, (AW_CL)cmd, (AW_CL)2);
        aws->create_autosize_button("INVERT_COLORED", GBS_global_string_copy("Invert all %s of ...", Sel->items_name), "I", 2);
    }

    aws->at_newline();
    aws->window_fit();

    return aws;
}

AW_window *create_awt_listed_items_colorizer(AW_root *aw_root, struct adaqbsstruct *cbs) {
    return create_awt_colorizer_window(aw_root, cbs->gb_main, cbs, 0);
}

AW_window *awt_create_item_colorizer(AW_root *aw_root, GBDATA *gb_main, const ad_item_selector *sel) {
    return create_awt_colorizer_window(aw_root, gb_main, 0, sel);
}

AW_window *create_awt_open_parser(AW_root *aw_root, struct adaqbsstruct *cbs)
{
    AW_window_simple *aws = 0;
    aws                   = new AW_window_simple;

    {
        char *macro_name = GBS_global_string_copy("MODIFY_DATABASE_FIELD_%s", cbs->selector->items_name);
        char *window_name = GBS_global_string_copy("MODIFY DATABASE FIELD of listed %s", cbs->selector->items_name);

        aws->init(aw_root, macro_name, window_name);

        free(window_name);
        free(macro_name);
    }

    aws->load_xfig("awt/parser.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mod_field_list.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at("helptags");
    aws->callback(AW_POPUP_HELP, (AW_CL)"tags.hlp");
    aws->create_button("HELP_TAGS", "HELP TAGS", "H");

    aws->at("usetag");  aws->create_toggle(cbs->awar_use_tag);
    aws->at("deftag");  aws->create_input_field(cbs->awar_deftag);
    aws->at("tag");     aws->create_input_field(cbs->awar_tag);

    aws->at("double");  aws->create_toggle(cbs->awar_double_pars);

    awt_create_selection_list_on_scandb(cbs->gb_main, aws, cbs->awar_parskey, AWT_PARS_FILTER, "field", 0, cbs->selector, 20, 10);

    aws->at("go");
    aws->callback((AW_CB1)awt_do_pars_list, (AW_CL)cbs);
    aws->create_button("GO", "GO", "G");

    aws->at("parser");
    aws->create_text_field(cbs->awar_parsvalue);

    aws->at("pre");
    AW_selection_list *id = aws->create_selection_list(cbs->awar_parspredefined);

    char *filename = 0;
    switch (cbs->selector->type) {
    case AWT_QUERY_ITEM_SPECIES:
        filename = AWT_unfold_path("lib/sellists/mod_fields*.sellst", "ARBHOME");
        break;
    case AWT_QUERY_ITEM_GENES:
        filename = AWT_unfold_path("lib/sellists/mod_gene_fields*.sellst", "ARBHOME");
        break;
    case AWT_QUERY_ITEM_EXPERIMENTS:
        filename = AWT_unfold_path("lib/sellists/mod_experiment_fields*.sellst", "ARBHOME");
        break;
    default:
        awt_assert(0);
        break;
    }

    GB_ERROR error = filename ? aws->load_selection_list(id, filename) : "No default selection list for query-type";
    free(filename);
    if (error) {
        aw_message(error);
    }
    else {
        aws->get_root()->awar(cbs->awar_parspredefined)->add_callback((AW_RCB1)awt_predef_prg, (AW_CL)cbs);
    }
    return (AW_window *)aws;
}


/***************** Multi set fields *************************/

void awt_do_set_list(void *, struct adaqbsstruct *cbs, long append) {
    GB_ERROR  error = 0;
    char     *key   = cbs->aws->get_root()->awar(cbs->awar_setkey)->read_string();
    if (strcmp(key, "name") == 0) {
        error = "You cannot set the name field";
    }

    char *value = cbs->aws->get_root()->awar(cbs->awar_setvalue)->read_string();
    if (value[0] == 0) freenull(value);

    GB_begin_transaction(cbs->gb_main);

    GBDATA *gb_key_type = 0;
    {
        GBDATA *gb_key_data     = GB_search(cbs->gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);
        if (!gb_key_data) error = GB_await_error();
        else {
            GBDATA *gb_key_name = GB_find_string(gb_key_data, CHANGEKEY_NAME, key, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
            if (!gb_key_name) {
                error = GBS_global_string("The destination field '%s' does not exists", key);
            }
            else {
                gb_key_type = GB_brother(gb_key_name, CHANGEKEY_TYPE);
                if (!gb_key_type) error = GB_await_error();
            }
        }
    }

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, cbs->aws->get_root(), AWT_QUERY_ALL_SPECIES);
         !error && gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, AWT_QUERY_ALL_SPECIES))
    {
        for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
             !error && gb_item;
             gb_item = cbs->selector->get_next_item(gb_item))
        {
            if (IS_QUERIED(gb_item, cbs)) {
                GBDATA *gb_new = GB_search(gb_item, key, GB_FIND);
                if (gb_new) {
                    if (value) {
                        if (append) {
                            char *old = GB_read_as_string(gb_new);
                            if (old) {
                                GBS_strstruct *strstr = GBS_stropen(strlen(old)+strlen(value)+2);
                                GBS_strcat(strstr, old);
                                GBS_strcat(strstr, value);
                                char *v = GBS_strclose(strstr);
                                error = GB_write_as_string(gb_new, v);
                                free(v);
                            }
                            else {
                                char *name = GBT_read_string(gb_item, "name");
                                error = GB_export_errorf("Field '%s' of %s '%s' has incompatible type", key, cbs->selector->item_name, name);
                                free(name);
                            }
                        }
                        else { 
                            error = GB_write_as_string(gb_new, value);
                        }
                    }
                    else {
                        if (!append) error = GB_delete(gb_new);
                    }
                }
                else {
                    gb_new = GB_search(gb_item, key, (GB_TYPES)GB_read_int(gb_key_type));

                    if (!gb_new) error = GB_await_error();
                    else error         = GB_write_as_string(gb_new, value);
                }
            }
        }
    }

    GB_end_transaction_show_error(cbs->gb_main, error, aw_message);
    
    free(key);
    free(value);
}

AW_window *create_awt_do_set_list(AW_root *aw_root, struct adaqbsstruct *cbs)
{
    AW_window_simple *aws = 0;
    aws = new AW_window_simple;
    aws->init(aw_root, "SET_DATABASE_FIELD_OF_LISTED", "SET MANY FIELDS");
    aws->load_xfig("ad_mset.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"write_field_list.hlp");
    aws->create_button("HELP", "HELP", "H");

    awt_create_selection_list_on_scandb(cbs->gb_main, aws, cbs->awar_setkey, AWT_NDS_FILTER, "box", 0, cbs->selector, 20, 10);
    aws->at("create");
    aws->callback((AW_CB)awt_do_set_list, (AW_CL)cbs, 0);
    aws->create_button("SET_SINGLE_FIELD_OF_LISTED", "WRITE");
    aws->at("do");
    aws->callback((AW_CB)awt_do_set_list, (AW_CL)cbs, 1);
    aws->create_button("APPEND_SINGLE_FIELD_OF_LISTED", "APPEND");

    aws->at("val");
    aws->create_text_field(cbs->awar_setvalue, 2, 2);
    return (AW_window *)aws;

}

/***************** Multi set fields *************************/

void awt_do_set_protection(void *, struct adaqbsstruct *cbs) {
    GB_ERROR  error = 0;
    char     *key   = cbs->aws->get_root()->awar(cbs->awar_setkey)->read_string();

    GB_begin_transaction(cbs->gb_main);
    GBDATA *gb_key_data = GB_search(cbs->gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);
    GBDATA *gb_key_name = GB_find_string(gb_key_data, CHANGEKEY_NAME, key, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
    if (!gb_key_name) {
        error = GBS_global_string("The destination field '%s' does not exists", key);
    }
    else {
        int              level   = cbs->aws->get_root()->awar(cbs->awar_setprotection)->read_int();
        AW_root         *aw_root = cbs->aws->get_root();
        AWT_QUERY_RANGE  range   = (AWT_QUERY_RANGE)aw_root->awar(cbs->awar_where)->read_int();

        for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
             !error && gb_item_container;
             gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item = cbs->selector->get_next_item(gb_item))
            {
                if (IS_QUERIED(gb_item, cbs)) {
                    GBDATA *gb_new = GB_search(gb_item, key, GB_FIND);
                    if (!gb_new) continue;
                    error = GB_write_security_delete(gb_new, level);
                    error = GB_write_security_write(gb_new, level);
                }
            }
        }
    }
    GB_end_transaction_show_error(cbs->gb_main, error, aw_message);
    
    free(key);
}

AW_window *create_awt_set_protection(AW_root *aw_root, struct adaqbsstruct *cbs)
{
    AW_window_simple *aws = 0;
    aws = new AW_window_simple;
    aws->init(aw_root, "SET_PROTECTION_OF_FIELD_OF_LISTED", "SET PROTECTIONS OF FIELDS");
    aws->load_xfig("awt/set_protection.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"set_protection.hlp");
    aws->create_button("HELP", "HELP", "H");


    aws->at("prot");
    aws->create_toggle_field(cbs->awar_setprotection, 0);
    aws->insert_toggle("0 Temporary", "0", 0);
    aws->insert_toggle("1 Checked", "1", 1);
    aws->insert_toggle("2", "2", 2);
    aws->insert_toggle("3", "3", 3);
    aws->insert_toggle("4 normal", "4", 4);
    aws->insert_toggle("5 ", "5", 5);
    aws->insert_toggle("6 the truth", "5", 6);

    awt_create_selection_list_on_scandb(cbs->gb_main, aws, cbs->awar_setkey, AWT_NDS_FILTER, "list", 0, cbs->selector, 20, 10);

    aws->at("go");
    aws->callback((AW_CB1)awt_do_set_protection, (AW_CL)cbs);
    aws->create_button("SET_PROTECTION_OF_FIELD_OF_LISTED", "SET PROTECTION");

    return (AW_window *)aws;
}

void awt_toggle_flag(AW_window *aww, struct adaqbsstruct *cbs) {
    GB_transaction dummy(cbs->gb_main);
    GBDATA *gb_item = cbs->selector->get_selected_item(cbs->gb_main, aww->get_root());
    if (gb_item) {
        long flag = GB_read_flag(gb_item);
        GB_write_flag(gb_item, 1-flag);
    }
    awt_query_update_list(aww, cbs);
}

//  -----------------------------------------------------
//      struct ad_item_selector AWT_species_selector
//  -----------------------------------------------------

static GBDATA *awt_get_first_species_data(GBDATA *gb_main, AW_root *, AWT_QUERY_RANGE) {
    return GBT_get_species_data(gb_main);
}
static GBDATA *awt_get_next_species_data(GBDATA *, AWT_QUERY_RANGE) {
    return 0; // there is only ONE species_data
}

static void awt_select_species(GBDATA*,  AW_root *aw_root, const char *item_name) {
    aw_root->awar(AWAR_SPECIES_NAME)->write_string(item_name);
}

static GBDATA* awt_get_selected_species(GBDATA *gb_main, AW_root *aw_root) {
    char   *species_name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species   = 0;
    if (species_name[0]) {
        gb_species = GBT_find_species(gb_main, species_name);
    }
    free(species_name);
    return gb_species;
}

static char* awt_get_species_id(GBDATA *, GBDATA *gb_species) {
    // awt_get_species_id creates the label that occurs in the search and query result list
    GBDATA *gb_name = GB_entry(gb_species, "name");
    if (!gb_name) return 0;     // species w/o name -> skip
    return GB_read_as_string(gb_name);
}

static GBDATA *awt_find_species_by_id(GBDATA *gb_main, const char *id) {
    return GBT_find_species(gb_main, id); // id is 'name' field
}

struct ad_item_selector AWT_species_selector = {
    AWT_QUERY_ITEM_SPECIES,
    awt_select_species,
    awt_get_species_id,
    awt_find_species_by_id,
    (AW_CB)awt_selection_list_update_cb,
    12,
    CHANGE_KEY_PATH,
    "species",
    "species",
    "name",
    awt_get_first_species_data,
    awt_get_next_species_data,
    GBT_first_species_rel_species_data,
    GBT_next_species,
    awt_get_selected_species,
    0, 0, 
};

struct ad_item_selector AWT_organism_selector = {
    AWT_QUERY_ITEM_SPECIES,
    awt_select_species,
    awt_get_species_id,
    awt_find_species_by_id,
    (AW_CB)awt_selection_list_update_cb,
    12,
    CHANGE_KEY_PATH,
    "organism",
    "organism",
    "name",
    awt_get_first_species_data,
    awt_get_next_species_data,
    GBT_first_species_rel_species_data,
    GBT_next_species,
    awt_get_selected_species,
    0, 0, 
};

static void awt_new_selection_made(AW_root *aw_root, AW_CL cl_awar_selection, AW_CL cl_cbs) {
    const char          *awar_selection = (const char *)cl_awar_selection;
    struct adaqbsstruct *cbs            = (struct adaqbsstruct *)cl_cbs;

    char *item_name = aw_root->awar(awar_selection)->read_as_string();
    cbs->selector->update_item_awars(cbs->gb_main, aw_root, item_name);
    free(item_name);
}

static void query_box_init_config(AWT_config_definition& cdef, struct adaqbsstruct *cbs) {
    // this defines what is saved/restored to/from configs
    for (int key_id = 0; key_id<AWT_QUERY_SEARCHES; ++key_id) {
        cdef.add(cbs->awar_keys[key_id], "key", key_id);
        cdef.add(cbs->awar_queries[key_id], "query", key_id);
        cdef.add(cbs->awar_not[key_id], "not", key_id);
        cdef.add(cbs->awar_operator[key_id], "operator", key_id);
    }
}
static char *query_box_store_config(AW_window *aww, AW_CL cl_cbs, AW_CL) {
    AWT_config_definition cdef(aww->get_root());
    query_box_init_config(cdef, (struct adaqbsstruct *)cl_cbs);
    return cdef.read();
}
static void query_box_restore_config(AW_window *aww, const char *stored, AW_CL cl_cbs, AW_CL) {
    AWT_config_definition cdef(aww->get_root());
    query_box_init_config(cdef, (struct adaqbsstruct *)cl_cbs);
    cdef.write(stored);
}


typedef AW_window *(*window_generator)(AW_root *);

static void query_box_popup_view_window(AW_window *aww, AW_CL cl_create_window, AW_CL) {
    window_generator  create_window = (window_generator)cl_create_window;
    AW_window        *aw_viewer     = create_window(aww->get_root());
    aw_viewer->show();
}

/***************** Create the database query box and functions *************************/

static void query_rel_menu_entry(AW_window *aws, const char *id, const char *query_id, AW_label label, const char *mnemonic, const char *helpText, AW_active Mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    char *rel_id = GBS_global_string_copy("%s_%s", query_id, id);
    aws->insert_menu_topic(rel_id, label, mnemonic, helpText, Mask, f, cd1, cd2);
    free(rel_id);
}

struct adaqbsstruct *awt_create_query_box(AW_window *aws, awt_query_struct *awtqs, const char *query_id) // create the query box
{
    char                 buffer[256];
    AW_root             *aw_root = aws->get_root();
    GBDATA              *gb_main = awtqs->gb_main;
    struct adaqbsstruct *cbs     = (struct adaqbsstruct *)calloc(1, sizeof(struct adaqbsstruct));

    cbs->gb_main                = awtqs->gb_main;
    cbs->aws                    = aws;
    cbs->gb_ref                 = awtqs->gb_ref;
    cbs->expect_hit_in_ref_list = awtqs->expect_hit_in_ref_list;
    cbs->select_bit             = awtqs->select_bit;
    cbs->species_name           = strdup(awtqs->species_name);
    cbs->tree_name              = awtqs->tree_name ? aw_root->awar(awtqs->tree_name)->read_string() : 0;
    cbs->selector               = awtqs->selector;
    cbs->hit_description        = GBS_create_hash(2*awt_count_items(cbs, AWT_QUERY_ALL_SPECIES), GB_IGNORE_CASE);

    GB_push_transaction(gb_main);
    /*************** Create local AWARS *******************/

    awt_query_create_global_awars(aw_root, AW_ROOT_DEFAULT);

    {
        const char *default_key[AWT_QUERY_SEARCHES+1] = { "name", "name", "name", 0 };

        for (int key_id = 0; key_id<AWT_QUERY_SEARCHES; ++key_id) {
            sprintf(buffer, "tmp/dbquery_%s/key_%i", query_id, key_id);
            cbs->awar_keys[key_id] = strdup(buffer);
            awt_assert(default_key[key_id] != 0);
            aw_root->awar_string(cbs->awar_keys[key_id], default_key[key_id], AW_ROOT_DEFAULT);

            sprintf(buffer, "tmp/dbquery_%s/query_%i", query_id, key_id);
            cbs->awar_queries[key_id] = strdup(buffer);
            aw_root->awar_string(cbs->awar_queries[key_id], "*", AW_ROOT_DEFAULT);

            sprintf(buffer, "tmp/dbquery_%s/not_%i", query_id, key_id);
            cbs->awar_not[key_id] = strdup(buffer);
            aw_root->awar_int(cbs->awar_not[key_id], 0, AW_ROOT_DEFAULT);

            sprintf(buffer, "tmp/dbquery_%s/operator_%i", query_id, key_id);
            cbs->awar_operator[key_id] = strdup(buffer);
            aw_root->awar_string(cbs->awar_operator[key_id], "ign", AW_ROOT_DEFAULT);
        }
        aw_root->awar(cbs->awar_keys[0])->add_callback(awt_first_searchkey_changed_cb, (AW_CL)cbs);
    }

    sprintf(buffer, "tmp/dbquery_%s/ere", query_id);
    cbs->awar_ere = strdup(buffer);
    aw_root->awar_int(cbs->awar_ere, AWT_QUERY_GENERATE);

    sprintf(buffer, "tmp/dbquery_%s/where", query_id);
    cbs->awar_where = strdup(buffer);
    aw_root->awar_int(cbs->awar_where, AWT_QUERY_ALL_SPECIES);

    sprintf(buffer, "tmp/dbquery_%s/count", query_id);
    cbs->awar_count = strdup(buffer);
    aw_root->awar_int(cbs->awar_count, 0);

    sprintf(buffer, "tmp/dbquery_%s/sort", query_id);
    cbs->awar_sort = strdup(buffer);
    aw_root->awar_int(cbs->awar_sort, AWT_QUERY_SORT_NONE)->add_callback(awt_sort_order_changed_cb, (AW_CL)cbs);
    cbs->sort_mask = AWT_QUERY_SORT_NONE; // default to unsorted

    sprintf(buffer, "tmp/dbquery_%s/by", query_id);
    cbs->awar_by = strdup(buffer);
    aw_root->awar_int(cbs->awar_by, AWT_QUERY_MATCH);

    if (awtqs->ere_pos_fig) {
        aws->at(awtqs->ere_pos_fig);
        aws->create_toggle_field(cbs->awar_ere, "", "");

        aws->insert_toggle(GBS_global_string("Search %s", cbs->selector->items_name), "G", (int)AWT_QUERY_GENERATE);
        aws->insert_toggle(GBS_global_string("Add %s", cbs->selector->items_name), "E", (int)AWT_QUERY_ENLARGE);
        aws->insert_toggle(GBS_global_string("Keep %s", cbs->selector->items_name), "R", (int)AWT_QUERY_REDUCE);

        aws->update_toggle_field();
    }
    if (awtqs->where_pos_fig) {
        aws->at(awtqs->where_pos_fig);
        aws->create_toggle_field(cbs->awar_where, "", "");
        aws->insert_toggle("of current organism", "C", (int)AWT_QUERY_CURRENT_SPECIES);
        aws->insert_toggle("of marked organisms", "M", (int)AWT_QUERY_MARKED_SPECIES);
        aws->insert_toggle("of all organisms", "A", (int)AWT_QUERY_ALL_SPECIES);
        aws->update_toggle_field();

    }
    if (awtqs->by_pos_fig) {
        aws->at(awtqs->by_pos_fig);
        aws->create_toggle_field(cbs->awar_by, "", "");
        aws->insert_toggle("that match the query", "M", (int)AWT_QUERY_MATCH);
        aws->insert_toggle("that don't match the q.", "D", (int)AWT_QUERY_DONT_MATCH);
        aws->insert_toggle("that are marked", "A", (int)AWT_QUERY_MARKED);
        aws->update_toggle_field();
    }

    // distances for multiple queries :

#define KEY_Y_OFFSET 32

    int xpos_calc[3] = { -1, -1, -1 }; // X-positions for elements in search expressions

    if (awtqs->qbox_pos_fig) {
        AW_at_size at_size;
        int        xpos, ypos;

        aws->auto_space(1, 1);

        aws->at(awtqs->qbox_pos_fig);
        aws->store_at_size_and_attach(&at_size);
        aws->get_at_position(&xpos, &ypos);

        int ypos_dummy;

        for (int key = AWT_QUERY_SEARCHES-1;  key >= 0; --key) {
            if (key) {
                aws->at(xpos, ypos+key*KEY_Y_OFFSET);
                aws->create_option_menu(cbs->awar_operator[key], 0, "");
                aws->insert_option("and", "", "and");
                aws->insert_option("or", "", "or");
                aws->insert_option("ign", "", "ign");
                aws->update_option_menu();

                if (xpos_calc[0] == -1) aws->get_at_position(&xpos_calc[0], &ypos_dummy);
            }

            aws->at(xpos_calc[0], ypos+key*KEY_Y_OFFSET);
            aws->restore_at_size_and_attach(&at_size);

            {
                char *button_id = GBS_global_string_copy("field_sel_%s_%i", query_id, key);
                awt_create_selection_list_on_scandb(gb_main, aws, cbs->awar_keys[key], AWT_NDS_FILTER,
                                                    0, awtqs->rescan_pos_fig,
                                                    awtqs->selector, 22, 20, AWT_SF_PSEUDO, button_id);
                free(button_id);
            }

            if (xpos_calc[1] == -1) aws->get_at_position(&xpos_calc[1], &ypos_dummy);

            aws->at(xpos_calc[1], ypos+key*KEY_Y_OFFSET);
            aws->create_toggle(cbs->awar_not[key], "#equal.xpm", "#notEqual.xpm");

            if (xpos_calc[2] == -1) aws->get_at_position(&xpos_calc[2], &ypos_dummy);
        }
    }
    if (awtqs->key_pos_fig) {
        aws->at(awtqs->key_pos_fig);
        aws->create_input_field(cbs->awar_keys[0], 12);
    }

    if (awtqs->query_pos_fig) {
        aws->at(awtqs->query_pos_fig);

        AW_at_size at_size;
        int        xpos, ypos;
        aws->store_at_size_and_attach(&at_size);
        aws->get_at_position(&xpos, &ypos);

        for (int key = 0; key<AWT_QUERY_SEARCHES; ++key) {
            aws->at(xpos_calc[2], ypos+key*KEY_Y_OFFSET);
            aws->restore_at_size_and_attach(&at_size);
            aws->d_callback((AW_CB)awt_do_query, (AW_CL)cbs, AWT_EXT_QUERY_NONE); // enable ENTER in searchfield to start search
            aws->create_input_field(cbs->awar_queries[key], 12);
        }
    }

    if (awtqs->result_pos_fig) {
        aws->at(awtqs->result_pos_fig);
        if (awtqs->create_view_window) {
            aws->callback(query_box_popup_view_window, (AW_CL)awtqs->create_view_window, 0);
        }
        aws->d_callback((AW_CB1)awt_toggle_flag, (AW_CL)cbs);

        {
            char    *this_awar_name = GBS_global_string_copy("tmp/dbquery_%s/select", query_id); // do not free this, cause it's passed to awt_new_selection_made
            AW_awar *awar           = aw_root->awar_string(this_awar_name, "", AW_ROOT_DEFAULT);

            cbs->result_id = aws->create_selection_list(this_awar_name, "", "", 5, 5);
            awar->add_callback(awt_new_selection_made, (AW_CL)this_awar_name, (AW_CL)cbs);
        }
        aws->insert_default_selection(cbs->result_id, "end of list", "");
        aws->update_selection_list(cbs->result_id);
    }
    if (awtqs->count_pos_fig) {
        aws->at(awtqs->count_pos_fig);
        aws->label("Hits:");
        aws->create_button(0, cbs->awar_count, 0, "+");
    }

    if (awtqs->config_pos_fig) {
        aws->button_length(0);
        aws->at(awtqs->config_pos_fig);
        char *macro_id = GBS_global_string_copy("SAVELOAD_CONFIG_%s", query_id);
        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "query_box",
                                  query_box_store_config, query_box_restore_config, (AW_CL)cbs, 0,
                                  macro_id);
        free(macro_id);
    }

    aws->button_length(18);
    if (awtqs->do_query_pos_fig) {
        aws->at(awtqs->do_query_pos_fig);
        aws->callback((AW_CB)awt_do_query, (AW_CL)cbs, AWT_EXT_QUERY_NONE);
        char *macro_id = GBS_global_string_copy("SEARCH_%s", query_id);
        aws->create_button(macro_id, "Search");
        free(macro_id);
    }
    if (awtqs->do_refresh_pos_fig) {
        aws->at(awtqs->do_refresh_pos_fig);
        aws->create_option_menu(cbs->awar_sort);
        aws->insert_default_option("unsorted",  "u", AWT_QUERY_SORT_NONE);
        aws->insert_option        ("by value",  "v", AWT_QUERY_SORT_BY_1STFIELD_CONTENT);
        aws->insert_option        ("by id",     "n", AWT_QUERY_SORT_BY_ID);
        if (cbs->selector->parent_selector) {
            aws->insert_option    ("by parent", "p", AWT_QUERY_SORT_BY_NESTED_PID);
        }
        aws->insert_option        ("by marked", "m", AWT_QUERY_SORT_BY_MARKED);
        aws->insert_option        ("by hit",    "h", AWT_QUERY_SORT_BY_HIT_DESCRIPTION);
        aws->insert_option        ("reverse",   "r", AWT_QUERY_SORT_REVERSE);
        aws->update_option_menu();
    }
    else {
        awt_assert(0); // hmm - no sort button here. -> tell ralf where this happens
    }

    if (awtqs->do_mark_pos_fig) {
        aws->at(awtqs->do_mark_pos_fig);
        aws->help_text("mark_list.hlp");
        aws->callback((AW_CB)awt_do_mark_list, (AW_CL)cbs, 1);
        aws->create_button("MARK_LISTED_UNMARK_REST", "Mark Listed\nUnmark Rest", "M");
    }
    if (awtqs->do_unmark_pos_fig) {
        aws->at(awtqs->do_unmark_pos_fig);
        aws->help_text("unmark_list.hlp");
        aws->callback((AW_CB)awt_do_mark_list, (AW_CL)cbs, 0);
        aws->create_button("UNMARK_LISTED_MARK_REST", "Unmark Listed\nMark Rest", "U");
    }
    if (awtqs->do_delete_pos_fig) {
        aws->at(awtqs->do_delete_pos_fig);
        aws->help_text("del_list.hlp");
        aws->callback((AW_CB)awt_delete_species_in_list, (AW_CL)cbs, 0);
        char *macro_id = GBS_global_string_copy("DELETE_LISTED_%s", query_id);
        aws->create_button(macro_id, "Delete Listed", "D");
        free(macro_id);
    }
    if (awtqs->do_set_pos_fig) {
        sprintf(buffer, "tmp/dbquery_%s/set_key", query_id);
        cbs->awar_setkey = strdup(buffer);
        aw_root->awar_string(cbs->awar_setkey);

        sprintf(buffer, "tmp/dbquery_%s/set_protection", query_id);
        cbs->awar_setprotection = strdup(buffer);
        aw_root->awar_int(cbs->awar_setprotection, 4);

        sprintf(buffer, "tmp/dbquery_%s/set_value", query_id);
        cbs->awar_setvalue = strdup(buffer);
        aw_root->awar_string(cbs->awar_setvalue);

        aws->at(awtqs->do_set_pos_fig);
        aws->help_text("mod_field_list.hlp");
        aws->callback(AW_POPUP, (AW_CL)create_awt_do_set_list, (AW_CL)cbs);
        char *macro_id = GBS_global_string_copy("WRITE_TO_FIELDS_OF_LISTED_%s", query_id);
        aws->create_button(macro_id, "Write to Fields\nof Listed", "S");
        free(macro_id);
    }

    char *Items = strdup(cbs->selector->items_name);
    Items[0]    = toupper(Items[0]);

    if ((awtqs->use_menu || awtqs->open_parser_pos_fig)) {
        sprintf(buffer, "tmp/dbquery_%s/tag",                 query_id); cbs->awar_tag            = strdup(buffer); aw_root->awar_string(cbs->awar_tag);
        sprintf(buffer, "tmp/dbquery_%s/use_tag",             query_id); cbs->awar_use_tag        = strdup(buffer); aw_root->awar_int   (cbs->awar_use_tag);
        sprintf(buffer, "tmp/dbquery_%s/deftag",              query_id); cbs->awar_deftag         = strdup(buffer); aw_root->awar_string(cbs->awar_deftag);
        sprintf(buffer, "tmp/dbquery_%s/double_pars",         query_id); cbs->awar_double_pars    = strdup(buffer); aw_root->awar_int   (cbs->awar_double_pars);
        sprintf(buffer, "tmp/dbquery_%s/parskey",             query_id); cbs->awar_parskey        = strdup(buffer); aw_root->awar_string(cbs->awar_parskey);
        sprintf(buffer, "tmp/dbquery_%s/parsvalue",           query_id); cbs->awar_parsvalue      = strdup(buffer); aw_root->awar_string(cbs->awar_parsvalue);
        sprintf(buffer, "tmp/dbquery_%s/awar_parspredefined", query_id); cbs->awar_parspredefined = strdup(buffer); aw_root->awar_string(cbs->awar_parspredefined);

        if (awtqs->use_menu) {
            sprintf(buffer, "Modify Fields of Listed %s", Items); query_rel_menu_entry(aws, "mod_fields_of_listed", query_id, buffer, "F", "mod_field_list.hlp", AWM_ALL, AW_POPUP, (AW_CL)create_awt_open_parser, (AW_CL)cbs);
        }
        else {
            aws->at(awtqs->open_parser_pos_fig);
            aws->callback(AW_POPUP, (AW_CL)create_awt_open_parser, (AW_CL)cbs);
            aws->create_button("MODIFY_FIELDS_OF_LISTED", "MODIFY FIELDS\nOF LISTED", "F");
        }
    }
    if (awtqs->use_menu) {
        sprintf(buffer, "Set Protection of Fields of Listed %s", Items); query_rel_menu_entry(aws, "s_prot_of_listed", query_id, buffer, "P", "set_protection.hlp", AWM_ALL, AW_POPUP, (AW_CL)create_awt_set_protection, (AW_CL)cbs);
        aws->insert_separator();
        sprintf(buffer, "Mark Listed %s, don't Change Rest",   Items); query_rel_menu_entry(aws, "mark_listed",             query_id, buffer, "M", "mark.hlp", AWM_ALL, (AW_CB)awt_do_mark_list, (AW_CL)cbs, (AW_CL)1 | 8);
        sprintf(buffer, "Mark Listed %s, Unmark Rest",         Items); query_rel_menu_entry(aws, "mark_listed_unmark_rest", query_id, buffer, "L", "mark.hlp", AWM_ALL, (AW_CB)awt_do_mark_list, (AW_CL)cbs, (AW_CL)1);
        sprintf(buffer, "Unmark Listed %s, don't Change Rest", Items); query_rel_menu_entry(aws, "unmark_listed",           query_id, buffer, "U", "mark.hlp", AWM_ALL, (AW_CB)awt_do_mark_list, (AW_CL)cbs, (AW_CL)0 | 8);
        sprintf(buffer, "Unmark Listed %s, Mark Rest",         Items); query_rel_menu_entry(aws, "unmark_listed_mark_rest", query_id, buffer, "R", "mark.hlp", AWM_ALL, (AW_CB)awt_do_mark_list, (AW_CL)cbs, (AW_CL)0);
        aws->insert_separator();


        sprintf(buffer, "Set Color of Listed %s", Items);    query_rel_menu_entry(aws, "set_color_of_listed", query_id, buffer, "C", "set_color_of_listed.hlp", AWM_ALL, AW_POPUP, (AW_CL)create_awt_listed_items_colorizer, (AW_CL)cbs);

        if (cbs->gb_ref) {
            awt_assert(cbs->selector->type == AWT_QUERY_ITEM_SPECIES); // stuff below works only for species
            aws->insert_separator();
            if (cbs->expect_hit_in_ref_list) {
                aws->insert_menu_topic("search_equal_fields_and_listed_in_I", "Search entries existing in both DBs and listed in the DB I hitlist", "S",
                                       "search_equal_fields.hlp", AWM_ALL, (AW_CB)awt_do_query, (AW_CL)cbs, AWT_EXT_QUERY_COMPARE_LINES);
                aws->insert_menu_topic("search_equal_words_and_listed_in_I",  "Search words existing in entries of both DBs and listed in the DB I hitlist", "W",
                                       "search_equal_fields.hlp", AWM_ALL, (AW_CB)awt_do_query, (AW_CL)cbs, AWT_EXT_QUERY_COMPARE_WORDS);
            }
            else {
                aws->insert_menu_topic("search_equal_field_in_both_db", "Search entries existing in both DBs", "S",
                                       "search_equal_fields.hlp", AWM_ALL, (AW_CB)awt_do_query, (AW_CL)cbs, AWT_EXT_QUERY_COMPARE_LINES);
                aws->insert_menu_topic("search_equal_word_in_both_db", "Search words existing in entries of both DBs", "W",
                                       "search_equal_fields.hlp", AWM_ALL, (AW_CB)awt_do_query, (AW_CL)cbs, AWT_EXT_QUERY_COMPARE_WORDS);
            }
        }
    }

    free(Items);

    GB_pop_transaction(gb_main);
    return cbs;
}
