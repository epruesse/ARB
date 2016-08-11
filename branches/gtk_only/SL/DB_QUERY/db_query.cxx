// ============================================================ //
//                                                              //
//   File      : db_query.cxx                                   //
//   Purpose   : Database queries                               //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "db_query.h"
#include "db_query_local.h"

#include <item_sel_list.h>
#include <awt_config_manager.hxx>

#include <aw_advice.hxx>
#include <aw_color_groups.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_awar.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <aw_question.hxx>

#include <arb_strbuf.h>
#include <arb_sort.h>
#include <arb_str.h>
#include <arb_match.h>

#include <list>
#include <string>
#include <awt_sel_boxes.hxx>
#include <rootAsWin.h>
#include <ad_cb.h>
#include <Keeper.h>
#include <dbui.h>
#include <ad_colorset.h>
#include <arb_global_defs.h>

using namespace std;
using namespace QUERY;

#define MAX_QUERY_LIST_LEN  100000
#define MAX_SHOWN_DATA_SIZE 500

#define AWAR_COLORIZE "tmp/dbquery_all/colorize"

static void free_hit_description(long info) {
    free(reinterpret_cast<char*>(info));
}

inline void SET_QUERIED(GBDATA *gb_species, DbQuery *query, const char *hitInfo, size_t hitInfoLen = 0) {
    dbq_assert(hitInfo);

    GB_raise_user_flag(gb_species, query->select_bit);

    char *name = query->selector.generate_item_id(query->gb_main, gb_species);
    char *info;

    if (hitInfoLen == 0) hitInfoLen = strlen(hitInfo);
    if (hitInfoLen>MAX_SHOWN_DATA_SIZE) {
        char *dupInfo = strdup(hitInfo);
        hitInfoLen    = GBS_shorten_repeated_data(dupInfo);
        if (hitInfoLen>MAX_SHOWN_DATA_SIZE) {
            strcpy(dupInfo+hitInfoLen-5, "[...]");
        }
        info = strdup(dupInfo);
        free(dupInfo);
    }
    else {
        info = strdup(hitInfo);
    }

    GBS_write_hash(query->hit_description, name, reinterpret_cast<long>(info)); // overwrite hit info (also deallocates) 
    free(name);
}

inline void CLEAR_QUERIED(GBDATA *gb_species, DbQuery *query) {
    GB_clear_user_flag(gb_species, query->select_bit);

    char *name = query->selector.generate_item_id(query->gb_main, gb_species);
    GBS_write_hash(query->hit_description, name, 0); // delete hit info (also deallocates)
    free(name);
}

inline const char *getHitInfo(const char *item_id, DbQuery *query) {
    long info = GBS_read_hash(query->hit_description, item_id);
    return reinterpret_cast<const char*>(info);
}
inline const char *getHitInfo(GBDATA *gb_species, DbQuery *query) {
    char       *name   = query->selector.generate_item_id(query->gb_main, gb_species);
    const char *result = getHitInfo(name, query);
    free(name);
    return result;
}
inline string keptHitReason(const string& currentHitReason, GBDATA *gb_item, DbQuery *query) {
    string      reason  = string("kept because ")+currentHitReason;
    const char *hitinfo = getHitInfo(gb_item, query);
    if (hitinfo) reason = string(hitinfo)+" ("+reason+')';
    return reason;
}

static void create_query_independent_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int(AWAR_COLORIZE, 0, aw_def);
}

GBDATA *QUERY::query_get_gb_main(DbQuery *query) {
    return query->gb_main;
}

const ItemSelector& QUERY::get_queried_itemtype(DbQuery *query) {
    return query->selector;
}

enum EXT_QUERY_TYPES {
    EXT_QUERY_NONE,
    EXT_QUERY_COMPARE_LINES,
    EXT_QUERY_COMPARE_WORDS
};

query_spec::query_spec(ItemSelector& selector_)
    : selector(selector_),
      gb_main(0),
      gb_ref(0),
      expect_hit_in_ref_list(0),
      species_name(0),
      tree_name(0),
      select_bit(GB_USERFLAG_QUERY), // always == GB_USERFLAG_QUERY atm (nevertheless DO NOT hardcode)
      use_menu(0),
      ere_pos_fig(0),
      where_pos_fig(0),
      by_pos_fig(0),
      qbox_pos_fig(0),
      key_pos_fig(0),
      query_pos_fig(0),
      result_pos_fig(0),
      count_pos_fig(0),
      do_query_pos_fig(0),
      config_pos_fig(0),
      do_mark_pos_fig(0),
      do_unmark_pos_fig(0),
      do_delete_pos_fig(0),
      do_set_pos_fig(0),
      open_parser_pos_fig(0),
      do_refresh_pos_fig(0),
      popup_info_window(0),
      info_box_pos_fig(0)
{
    dbq_assert(&selector);
}

bool query_spec::is_queried(GBDATA *gb_item) const {
    return GB_user_flag(gb_item, select_bit);
}

bool QUERY::IS_QUERIED(GBDATA *gb_item, const DbQuery *query) {
    return query->is_queried(gb_item);
}

long QUERY::count_queried_items(DbQuery *query, QUERY_RANGE range) {
    GBDATA       *gb_main  = query->gb_main;
    ItemSelector& selector = query->selector;

    long count = 0;

    for (GBDATA *gb_item_container = selector.get_first_item_container(gb_main, query->aws->get_root(), range);
         gb_item_container;
         gb_item_container = selector.get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
             gb_item;
             gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
        {
            if (IS_QUERIED(gb_item, query)) count++;
        }
    }
    return count;
}

#if defined(WARN_TODO)
#warning replace query_count_items by "method" of selector
#endif

static int query_count_items(DbQuery *query, QUERY_RANGE range, QUERY_MODES mode) {
    int             count    = 0;
    GBDATA         *gb_main  = query->gb_main;
    ItemSelector&   selector = query->selector;
    GB_transaction  ta(gb_main);

    for (GBDATA *gb_item_container = selector.get_first_item_container(gb_main, query->aws->get_root(), range);
         gb_item_container;
         gb_item_container = selector.get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
             gb_item;
             gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
        {
            switch (mode) {
                case QUERY_GENERATE: ++count; break;
                case QUERY_ENLARGE:  count += !IS_QUERIED(gb_item, query); break;
                case QUERY_REDUCE:   count +=  IS_QUERIED(gb_item, query); break;
            }
        }
    }
    return count;
}

const int MAX_CRITERIA = int(sizeof(unsigned long)*8/QUERY_SORT_CRITERIA_BITS);

static void split_sort_mask(unsigned long sort_mask, QUERY_RESULT_ORDER *order) {
    // splits the sort order bit mask 'sort_mask' into single sort criteria and write these into 'order'
    // (order[0] will contain the primary sort criteria, order[1] the secondary, ...)

    for (int o = 0; o<MAX_CRITERIA; ++o) {
        order[o] = QUERY_RESULT_ORDER(sort_mask&QUERY_SORT_CRITERIA_MASK);
        dbq_assert(order[o] == (order[o]&QUERY_SORT_CRITERIA_MASK));
        sort_mask = sort_mask>>QUERY_SORT_CRITERIA_BITS;
    }
}

static QUERY_RESULT_ORDER find_display_determining_sort_order(QUERY_RESULT_ORDER *order) {
    // Returns the first criteria in 'order' (which has to have MAX_CRITERIA elements)
    // that matches
    // - QUERY_SORT_BY_1STFIELD_CONTENT or
    // - QUERY_SORT_BY_HIT_DESCRIPTION
    // (or QUERY_SORT_NONE if none of the above is used)

    QUERY_RESULT_ORDER first = QUERY_SORT_NONE;
    for (int o = 0; o<MAX_CRITERIA && first == QUERY_SORT_NONE; ++o) {
        if (order[o] & (QUERY_SORT_BY_1STFIELD_CONTENT|QUERY_SORT_BY_HIT_DESCRIPTION)) {
            first = order[o];
        }
    }
    return first;
}

static void remove_keydependent_sort_criteria(QUERY_RESULT_ORDER *order) {
    // removes all sort-criteria from order which would use the order of the currently selected primary key

    int n = 0;
    for (int o = 0; o<MAX_CRITERIA; ++o) {
        if (order[o] != QUERY_SORT_BY_1STFIELD_CONTENT) {
            order[n++] = order[o];
        }
    }
    for (; n<MAX_CRITERIA; ++n) {
        order[n] = QUERY_SORT_NONE;
    }
}

static void first_searchkey_changed_cb(AW_root *, DbQuery *query) {
    QUERY_RESULT_ORDER order[MAX_CRITERIA];
    split_sort_mask(query->sort_mask, order);

    QUERY_RESULT_ORDER usedOrder = find_display_determining_sort_order(order);
    if (usedOrder != QUERY_SORT_BY_HIT_DESCRIPTION && usedOrder != QUERY_SORT_NONE) { // do we display values?
        DbQuery_update_list(query);
    }
}

inline bool keep_criteria(QUERY_RESULT_ORDER old_criteria, QUERY_RESULT_ORDER new_criteria) {
    return
        old_criteria  != QUERY_SORT_NONE &&     // do not keep 'unsorted' (it is no real criteria)
        (old_criteria != new_criteria        ||     // do not keep new criteria (added later)
         old_criteria == QUERY_SORT_REVERSE);   // reverse may occur several times -> always keep
}

static void result_sort_order_changed_cb(AW_root *aw_root, DbQuery *query) {
    // adds the new selected sort order to the sort order mask
    // (added order removes itself, if it previously existed in sort order mask)
    //
    // if 'unsorted' is selected -> delete sort order

    QUERY_RESULT_ORDER new_criteria = (QUERY_RESULT_ORDER)aw_root->awar(query->awar_sort)->read_int();
    if (new_criteria == QUERY_SORT_NONE) {
        query->sort_mask = QUERY_SORT_NONE; // clear sort_mask
    }
    else {
        QUERY_RESULT_ORDER order[MAX_CRITERIA];
        split_sort_mask(query->sort_mask, order);

        int empty_or_same = 0;
        for (int o = 0; o<MAX_CRITERIA; o++) {
            if (!keep_criteria(order[o], new_criteria)) {
                empty_or_same++; // these criteria will be skipped below
            }
        }

        unsigned long new_sort_mask = 0;
        for (int o = MAX_CRITERIA-(empty_or_same>0 ? 1 : 2); o >= 0; o--) {
            if (keep_criteria(order[o], new_criteria)) {
                new_sort_mask = (new_sort_mask<<QUERY_SORT_CRITERIA_BITS)|order[o];
            }
        }
        query->sort_mask = (new_sort_mask<<QUERY_SORT_CRITERIA_BITS)|new_criteria; // add new primary key
    }
    DbQuery_update_list(query);
}

struct hits_sort_params : virtual Noncopyable {
    DbQuery            *query;
    char               *first_key;
    QUERY_RESULT_ORDER  order[MAX_CRITERIA];

    hits_sort_params(DbQuery *q, const char *fk)
        : query(q),
          first_key(strdup(fk))
    {}

    ~hits_sort_params() {
        free(first_key);
    }
};

static int compare_hits(const void *cl_item1, const void *cl_item2, void *cl_param) {
    const hits_sort_params *param = static_cast<const hits_sort_params*>(cl_param);

    GBDATA *gb_item1 = (GBDATA*)cl_item1;
    GBDATA *gb_item2 = (GBDATA*)cl_item2;

    DbQuery      *query    = param->query;
    ItemSelector& selector = query->selector;

    int cmp = 0;

    for (int o = 0; o<MAX_CRITERIA && cmp == 0; o++) {
        QUERY_RESULT_ORDER criteria = param->order[o];

        switch (criteria) {
            case QUERY_SORT_NONE:
                o = MAX_CRITERIA; // don't sort further
                break;

            case QUERY_SORT_BY_1STFIELD_CONTENT: {
                char *field1 = GBT_read_as_string(gb_item1, param->first_key);
                char *field2 = GBT_read_as_string(gb_item2, param->first_key);

                cmp = ARB_strNULLcmp(field1, field2);

                free(field2);
                free(field1);
                break;
            }
            case QUERY_SORT_BY_NESTED_PID: {
                if (selector.parent_selector) {
                    GBDATA *gb_parent1 = selector.get_parent(gb_item1);
                    GBDATA *gb_parent2 = selector.get_parent(gb_item2);
                    
                    char *pid1 = selector.generate_item_id(query->gb_main, gb_parent1);
                    char *pid2 = selector.generate_item_id(query->gb_main, gb_parent2);

                    cmp = ARB_strNULLcmp(pid1, pid2);

                    free(pid2);
                    free(pid1);
                }
                break;
            }
            case QUERY_SORT_BY_ID: {
                const char *id1 = GBT_read_char_pntr(gb_item1, selector.id_field);
                const char *id2 = GBT_read_char_pntr(gb_item2, selector.id_field);

                cmp = strcmp(id1, id2);
                break;
            }
            case QUERY_SORT_BY_MARKED:
                cmp = GB_read_flag(gb_item2)-GB_read_flag(gb_item1);
                break;

            case QUERY_SORT_BY_HIT_DESCRIPTION: {
                const char *id1   = GBT_read_char_pntr(gb_item1, selector.id_field);
                const char *id2   = GBT_read_char_pntr(gb_item2, selector.id_field);
                const char *info1 = reinterpret_cast<const char *>(GBS_read_hash(query->hit_description, id1));
                const char *info2 = reinterpret_cast<const char *>(GBS_read_hash(query->hit_description, id2));
                cmp = ARB_strNULLcmp(info1, info2);
                break;
            }
            case QUERY_SORT_REVERSE: {
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

#if defined(ASSERTION_USED)
inline bool SLOW_is_pseudo_key(const char *key) {
    return
        strcmp(key, PSEUDO_FIELD_ANY_FIELD) == 0 ||
        strcmp(key, PSEUDO_FIELD_ALL_FIELDS) == 0;
}
#endif
inline bool is_pseudo_key(const char *key) {
    // returns true, if 'key' is a pseudo-key
    bool is_pseudo = key[0] == '[';
    dbq_assert(is_pseudo == SLOW_is_pseudo_key(key));
    return is_pseudo;
}

void QUERY::DbQuery_update_list(DbQuery *query) {
    GB_push_transaction(query->gb_main);

    dbq_assert(query->hitlist);
    query->hitlist->clear();

    AW_window     *aww      = query->aws;
    AW_root       *aw_root  = aww->get_root();
    QUERY_RANGE    range    = (QUERY_RANGE)aw_root->awar(query->awar_where)->read_int();
    ItemSelector&  selector = query->selector;

    // create array of hits
    long     count  = count_queried_items(query, range);
    GBDATA **sorted = ARB_alloc<GBDATA*>(count);
    {
        long s = 0;

        for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, aw_root, range);
             gb_item_container;
             gb_item_container = selector.get_next_item_container(gb_item_container, range))
        {
            for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
                 gb_item;
                 gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
            {
                if (IS_QUERIED(gb_item, query)) sorted[s++] = gb_item;
            }
        }
    }

    // sort hits

    hits_sort_params param(query, aww->get_root()->awar(query->awar_keys[0])->read_char_pntr());

    bool is_pseudo  = is_pseudo_key(param.first_key);
    bool show_value = !is_pseudo; // cannot refer to key-value of pseudo key

    if (query->sort_mask != QUERY_SORT_NONE) {    // unsorted -> don't sort
        split_sort_mask(query->sort_mask, param.order);
        if (is_pseudo) {
            remove_keydependent_sort_criteria(param.order);
        }
        if (show_value && find_display_determining_sort_order(param.order) == QUERY_SORT_BY_HIT_DESCRIPTION) {
            show_value = false;
        }
        GB_sort((void**)sorted, 0, count, compare_hits, &param);
    }

    // display hits

    int name_len = selector.item_name_length;
    if (name_len == -1) { // if name_len is unknown -> detect
        GBS_hash_do_loop(query->hit_description, detectMaxNameLength, &name_len);
    }

    long i;
    for (i = 0; i<count && i<MAX_QUERY_LIST_LEN; i++) {
        char *name = selector.generate_item_id(query->gb_main, sorted[i]);
        if (name) {
            char       *toFree = 0;
            const char *info;

            if (show_value) {
                toFree = GBT_read_as_string(sorted[i], param.first_key);
                if (toFree) {
                    if (strlen(toFree)>MAX_SHOWN_DATA_SIZE) {
                        size_t shortened_len = GBS_shorten_repeated_data(toFree);
                        if (shortened_len>MAX_SHOWN_DATA_SIZE) {
                            strcpy(toFree+MAX_SHOWN_DATA_SIZE-5, "[...]");
                        }
                    }
                }
                else {
                    toFree = GBS_global_string_copy("<%s has no data>", param.first_key);
                }
                info = toFree;
            }
            else {
                info = getHitInfo(name, query);
                if (!info) info = "<no hit info>";
            }

            dbq_assert(info);
            const char *line = GBS_global_string("%c %-*s :%s",
                                                 GB_read_flag(sorted[i]) ? '*' : ' ',
                                                 name_len, name,
                                                 info);

            query->hitlist->insert(line, name);
            free(toFree);
            free(name);
        }
    }

    if (count>MAX_QUERY_LIST_LEN) {
        query->hitlist->insert("*****  List truncated  *****", "");
    }

    free(sorted);

    query->hitlist->insert_default("End of list", "");
    query->hitlist->update();
    aww->get_root()->awar(query->awar_count)->write_int((long)count);
    GB_pop_transaction(query->gb_main);
}

static void mark_queried_cb(AW_window*, DbQuery *query, int mark) {
    // Mark listed species
    // mark = 1 -> mark listed
    // mark | 8 -> don't change rest

    ItemSelector& selector = query->selector;
    GB_push_transaction(query->gb_main);

    for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, query->aws->get_root(), QUERY_ALL_ITEMS);
         gb_item_container;
         gb_item_container = selector.get_next_item_container(gb_item_container, QUERY_ALL_ITEMS))
        {
            for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
                 gb_item;
                 gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
                {
                    if (IS_QUERIED(gb_item, query)) {
                        GB_write_flag(gb_item, mark&1);
                    }
                    else if ((mark&8) == 0) {
                        GB_write_flag(gb_item, 1-(mark&1));
                    }
                }
        }

    DbQuery_update_list(query);
    GB_pop_transaction(query->gb_main);
}

void QUERY::unquery_all(void *, DbQuery *query) {
    GB_push_transaction(query->gb_main);
    GBDATA *gb_species;
    for (gb_species = GBT_first_species(query->gb_main);
                gb_species;
                gb_species = GBT_next_species(gb_species)) {
        CLEAR_QUERIED(gb_species, query);
    }
    DbQuery_update_list(query);
    GB_pop_transaction(query->gb_main);
}

static void delete_queried_species_cb(AW_window*, DbQuery *query) {
    ItemSelector& selector = query->selector;
    GB_begin_transaction(query->gb_main);

    long cnt = 0;
    for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, query->aws->get_root(), QUERY_ALL_ITEMS);
         gb_item_container;
         gb_item_container = selector.get_next_item_container(gb_item_container, QUERY_ALL_ITEMS))
    {
        for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
             gb_item;
             gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
        {
            if (IS_QUERIED(gb_item, query)) cnt++;
        }
    }

    if (!cnt || !aw_ask_sure("delete_queried_species", GBS_global_string("Are you sure to delete %li %s", cnt, selector.items_name))) {
        GB_abort_transaction(query->gb_main);
        return;
    }

    GB_ERROR error = 0;

    for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, query->aws->get_root(), QUERY_ALL_ITEMS);
         !error && gb_item_container;
         gb_item_container = selector.get_next_item_container(gb_item_container, QUERY_ALL_ITEMS))
        {
            for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
                 !error && gb_item;
                 gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
                {
                    if (IS_QUERIED(gb_item, query)) {
                        error = GB_delete(gb_item);
                    }
                }
        }

    if (error) {
        GB_abort_transaction(query->gb_main);
        aw_message(error);
    }
    else {
        DbQuery_update_list(query);
        GB_commit_transaction(query->gb_main);
    }
}

static GB_HASH *create_ref_hash(const DbQuery *query, const char *key, bool split_words) {
    GBDATA  *gb_ref       = query->gb_ref;
    bool     queried_only = query->expect_hit_in_ref_list;
    GB_HASH *hash         = GBS_create_hash(GBT_get_species_count(gb_ref), GB_IGNORE_CASE);

    for (GBDATA  *gb_species = GBT_first_species(gb_ref);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        if (!queried_only || IS_QUERIED(gb_species, query)) {
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
//      class query_info

enum query_operator { ILLEGAL, AND, OR };

enum query_type {
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
};

enum query_field_type {
    AQFT_EXPLICIT,                                  // query should match one explicit field
    AQFT_ANY_FIELD,                                 // query should match one field (no matter which)
    AQFT_ALL_FIELDS,                                // query should match all fields
};

class query_info : virtual Noncopyable {
    query_operator op;                              // operator (AND or OR)

    char *key;                                      // search key
    bool  Not;                                      // true means "don't match"
    char *expr;                                     // search expression

    GBDATA     *gb_main;
    const char *tree;                               // name of current default tree (needed for ACI)

    bool             rek;                           // is 'key' hierarchical ?
    query_field_type match_field;                   // type of search key
    GBQUARK          keyquark;                      // valid only if match_field == AQFT_EXPLICIT
    query_type       type;                          // type of 'query'

    struct {                                        // used for some values of 'type'
        string     str;
        GBS_regex *regexp;
        float      number;
    } xquery;

    mutable char *error;                            // set by matches(), once set all future matches fail
    mutable char *lastACIresult;                    // result of last ACI query

    query_info *next;
    int        index;                               // number of query (0 = first query, 1 = second query); not always consecutive!

    // --------------------

    void initFields(DbQuery *query, int idx, query_operator aqo, AW_root *aw_root);
    query_info() {}

    void       detect_query_type();
    query_info *remove_tail();
    void       append(query_info *tail);

public:

    query_info(DbQuery *query);

    ~query_info() {
        free(key);
        free(expr);
        free(lastACIresult);
        if (xquery.regexp) GBS_free_regexpr(xquery.regexp);
        free(error);
        delete next;
    }

    query_operator getOperator() const { return op; }
    const char *getKey() const { return key; }
    bool applyNot(bool matched) const { return Not ? !matched : matched; }
    bool shallMatch() const { return !Not; }
    query_type getType() const { return type; }
    int getIndex() const { return index; }

    bool matches(const char *data, GBDATA *gb_item) const;
    GB_ERROR getError(int count = 0) const;
    const char *get_last_ACI_result() const { return type == AQT_ACI ? lastACIresult : 0; }

    query_info *getNext() { return next; }

    bool is_rek() const { return rek; }
    query_field_type get_match_field() const { return match_field; }
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
            gb_key = GB_find_sub_by_quark(gb_item, getKeyquark(), 0, 0);
        }

        return gb_key;
    }

    void negate();

#if defined(DEBUG)
    string dump_str() const { return string(key)+(Not ? "!=" : "==")+expr; }
    void dump(string *prev = 0) const {
        string mine = dump_str();
        if (prev) {
            string both = *prev+' ';
            switch (op) {
                case AND: both += "&&"; break;
                case OR: both  += "||"; break;
                default: dbq_assert(0); break;
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

void query_info::initFields(DbQuery *query, int idx, query_operator aqo, AW_root *aw_root) {
    dbq_assert(aqo == OR || aqo == AND);

    error         = 0;
    lastACIresult = 0;
    next          = 0;
    index         = idx;
    xquery.regexp = 0;

    op   = aqo;
    key  = aw_root->awar(query->awar_keys[idx])->read_string();
    Not  = aw_root->awar(query->awar_not[idx])->read_int() != 0;
    expr = aw_root->awar(query->awar_queries[idx])->read_string();

    gb_main = query->gb_main;
    tree    = query->get_tree_name();

    rek         = false;
    match_field = AQFT_EXPLICIT;
    keyquark    = -1;
    if (GB_first_non_key_char(key)) {
        if (strcmp(key, PSEUDO_FIELD_ANY_FIELD) == 0) match_field = AQFT_ANY_FIELD;
        else if (strcmp(key, PSEUDO_FIELD_ALL_FIELDS) == 0) match_field = AQFT_ALL_FIELDS;
        else rek = true;
    }
    else {
        keyquark = GB_find_or_create_quark(gb_main, key);
    }

    detect_query_type();

    if (!error && (!key[0] || strcmp(key, NO_FIELD_SELECTED) == 0)) {
        error = strdup("Please select a field");
    }
}

query_info::query_info(DbQuery *query) {
    AW_root *aw_root = query->aws->get_root();

    initFields(query, 0, OR, aw_root);                // initial value is false, (false OR QUERY1) == QUERY1

    query_info *tail = this;
    for (size_t keyidx = 1; keyidx<QUERY_SEARCHES; ++keyidx) {
        char *opstr = aw_root->awar(query->awar_operator[keyidx])->read_string();

        if (strcmp(opstr, "ign") != 0) {            // not ignore
            query_operator next_op = ILLEGAL;

            if (strcmp(opstr, "and")     == 0) next_op = AND;
            else if (strcmp(opstr, "or") == 0) next_op = OR;
#if defined(ASSERTION_USED)
            else aw_assert(0);
#endif // ASSERTION_USED

            if (next_op != ILLEGAL) {
                query_info *next_query = new query_info;

                next_query->initFields(query, keyidx, next_op, aw_root);

                tail->next = next_query;
                tail       = next_query;
            }
        }
        free(opstr);
    }
}

query_info *query_info::remove_tail() {
    query_info *tail = 0;
    if (next) {
        query_info *body_last = this;
        while (body_last->next && body_last->next->next) {
            body_last = body_last->next;
        }
        dbq_assert(body_last->next);
        dbq_assert(body_last->next->next == 0);

        tail            = body_last->next;
        body_last->next = 0;
    }
    return tail;
}

void query_info::append(query_info *tail) {
    dbq_assert(this != tail);
    if (next) next->append(tail);
    else next = tail;
}

void query_info::negate() {
    if (next) {
        query_info *tail = remove_tail();

        negate();
        tail->negate();

        switch (tail->op) {
            case AND: tail->op = OR; break;
            case OR: tail->op  = AND; break;
            default: dbq_assert(0); break;
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

GB_ERROR query_info::getError(int count) const {
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
            else err = GBS_static_string(dup);
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

void query_info::detect_query_type() {
    char    first = expr[0];
    string& str   = xquery.str;
    str           = expr;

    type = AQT_INVALID;

    if (!first)            type = AQT_EMPTY;
    else if (first == '/') {
        GB_CASE     case_flag;
        GB_ERROR    err       = 0;
        const char *unwrapped = GBS_unwrap_regexpr(expr, &case_flag, &err);
        if (unwrapped) {
            xquery.regexp = GBS_compile_regexpr(unwrapped, case_flag, &err);
            if (xquery.regexp) type = AQT_REGEXPR;
        }
        if (err) freedup(error, err);
    }
    else if (first == '|') type = AQT_ACI;
    else if (first == '<' || first == '>') {
        const char *rest = expr+1;
        const char *end;
        float       f    = strtof(rest, const_cast<char**>(&end));

        if (end != rest) { // did convert part or all of rest to float
            if (end[0] == 0) { // all of rest has been converted
                type          = expr[0] == '<' ? AQT_LOWER : AQT_GREATER;
                xquery.number = f;
            }
            else {
                freeset(error, GBS_global_string_copy("Could not convert '%s' to number (unexpected content '%s')", rest, end));
            }
        }
        // otherwise handle as non-special search string
    }

    if (type == AQT_INVALID && !error) {            // no type detected above
        if (containsWildcards(expr)) {
            size_t qlen = strlen(expr);
            char   last = expr[qlen-1];

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
                str  = expr;
                type = AQT_WILDCARD;
            }
        }
        else type = AQT_EXACT_MATCH;
    }

    dbq_assert(type != AQT_INVALID || error);
}

bool query_info::matches(const char *data, GBDATA *gb_item) const {
    // 'data' is the content of the searched field (read as string)
    // 'gb_item' is the DB item (e.g. species, gene). Used in ACI-search only.

    bool hit = false;

    dbq_assert(data);
    dbq_assert(gb_item);

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
            hit = strcasecmp(data, expr) == 0;
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
            hit = GBS_string_matches(data, expr, GB_IGNORE_CASE);
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
            char *aci_result = GB_command_interpreter(gb_main, data, expr, gb_item, tree);
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
            dbq_assert(0);
            freedup(error, "Invalid search expression");
            hit   = false;
            break;
        }
    }
    return applyNot(hit);
}

static void perform_query_cb(AW_window*, DbQuery *query, EXT_QUERY_TYPES ext_query) {
    ItemSelector& selector = query->selector;

    GB_push_transaction(query->gb_main);

    AW_root    *aw_root = query->aws->get_root();
    query_info  qinfo(query);

    QUERY_MODES mode  = (QUERY_MODES)aw_root->awar(query->awar_ere)->read_int();
    QUERY_RANGE range = (QUERY_RANGE)aw_root->awar(query->awar_where)->read_int();
    QUERY_TYPES type  = (QUERY_TYPES)aw_root->awar(query->awar_by)->read_int();

    if (query->gb_ref && type != QUERY_MARKED) {  // special for merge tool!
        char *first_query = aw_root->awar(query->awar_queries[0])->read_string();
        if (strlen(first_query) == 0) {
            if (!ext_query) ext_query  = EXT_QUERY_COMPARE_LINES;
        }
        free(first_query);
    }

    GB_ERROR error = qinfo.getError();

    if (!error) {
        size_t item_count = query_count_items(query, range, mode);
        if (item_count) {
            arb_progress progress("Searching", item_count);

            if (query->gb_ref && // merge tool only
                (ext_query == EXT_QUERY_COMPARE_LINES || ext_query == EXT_QUERY_COMPARE_WORDS))
            {
                GB_push_transaction(query->gb_ref);
                const char *first_key = qinfo.getKey();
                GB_HASH    *ref_hash  = create_ref_hash(query, first_key, ext_query == EXT_QUERY_COMPARE_WORDS);

#if defined(DEBUG)
                printf("query: search identical %s in field %s%s\n",
                       (ext_query == EXT_QUERY_COMPARE_WORDS ? "words" : "values"),
                       first_key,
                       query->expect_hit_in_ref_list ? " of species listed in other hitlist" : "");
#endif // DEBUG

                for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, aw_root, range);
                     gb_item_container && !error;
                     gb_item_container = selector.get_next_item_container(gb_item_container, range))
                {
                    for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
                         gb_item && !error;
                         gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
                    {
                        switch (mode) {
                            case QUERY_GENERATE: CLEAR_QUERIED(gb_item, query); break;
                            case QUERY_ENLARGE:  if (IS_QUERIED(gb_item, query)) continue; break;
                            case QUERY_REDUCE:   if (!IS_QUERIED(gb_item, query)) continue; break;
                        }

                        GBDATA *gb_key = qinfo.get_first_key(gb_item);

                        if (gb_key) {
                            char *data = GB_read_as_string(gb_key);

                            if (data && data[0]) {
                                string hit_reason;
                                bool   this_hit = false;

                                if (ext_query == EXT_QUERY_COMPARE_WORDS) {
                                    for (char *t = strtok(data, " "); t; t = strtok(0, " ")) {
                                        GBDATA *gb_ref_pntr = (GBDATA *)GBS_read_hash(ref_hash, t);
                                        if (gb_ref_pntr) { // found item in other DB, with 'first_key' containing word from 'gb_key'
                                            this_hit   = true;
                                            hit_reason = GBS_global_string("%s%s has word '%s' in %s",
                                                                           query->expect_hit_in_ref_list ? "Hit " : "",
                                                                           selector.generate_item_id(query->gb_ref, gb_ref_pntr),
                                                                           t, first_key);
                                        }
                                    }
                                }
                                else {
                                    GBDATA *gb_ref_pntr = (GBDATA *)GBS_read_hash(ref_hash, data);
                                    if (gb_ref_pntr) { // found item in other DB, with identical 'first_key'
                                        this_hit   = true;
                                        hit_reason = GBS_global_string("%s%s matches %s",
                                                                       query->expect_hit_in_ref_list ? "Hit " : "",
                                                                       selector.generate_item_id(query->gb_ref, gb_ref_pntr),
                                                                       first_key);
                                    }
                                }

                                if (type == QUERY_DONT_MATCH) {
                                    this_hit = !this_hit;
                                    if (this_hit) hit_reason = "<no matching entry>";
                                }

                                if (this_hit) {
                                    dbq_assert(!hit_reason.empty());
                                    SET_QUERIED(gb_item, query, hit_reason.c_str(), hit_reason.length());
                                }
                                else CLEAR_QUERIED(gb_item, query);
                            }
                            free(data);
                        }

                        progress.inc_and_check_user_abort(error);
                    }
                }

                GBS_free_hash(ref_hash);
                GB_pop_transaction(query->gb_ref);
            }
            else {                                          // "normal" query
                if (type == QUERY_DONT_MATCH) {
#if defined(DEBUG)
                    fputs("query: !(", stdout); qinfo.dump();
#endif // DEBUG

                    qinfo.negate();
                    type = QUERY_MATCH;

#if defined(DEBUG)
                    fputs(") => query: ", stdout); qinfo.dump(); fputc('\n', stdout);
#endif // DEBUG
                }
#if defined(DEBUG)
                else { fputs("query: ", stdout); qinfo.dump(); fputc('\n', stdout); }
#endif // DEBUG

                for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, aw_root, range);
                     gb_item_container && !error;
                     gb_item_container = selector.get_next_item_container(gb_item_container, range))
                {
                    for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
                         gb_item && !error;
                         gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
                    {
                        string hit_reason;

                        switch (mode) {
                            case QUERY_GENERATE: CLEAR_QUERIED(gb_item, query); break;
                            case QUERY_ENLARGE:  if (IS_QUERIED(gb_item, query)) continue; break;
                            case QUERY_REDUCE:   if (!IS_QUERIED(gb_item, query)) continue; break;
                        }

                        bool hit = false;

                        switch (type) {
                            case QUERY_MARKED: {
                                hit        = GB_read_flag(gb_item);
                                hit_reason = "<marked>";
                                break;
                            }
                            case QUERY_MATCH: {
                                for (query_info *this_qinfo = &qinfo; this_qinfo;  this_qinfo = this_qinfo ? this_qinfo->getNext() : 0) { // iterate over all single queries
                                    query_operator this_op = this_qinfo->getOperator();

                                    if ((this_op == OR) == hit) {
                                        continue; // skip query Q for '1 OR Q' and for '0 AND Q' (result can't change)
                                    }

                                    string this_hit_reason;

                                    GBDATA           *gb_key      = this_qinfo->get_first_key(gb_item);
                                    query_field_type  match_field = this_qinfo->get_match_field();
                                    bool              this_hit    = (match_field == AQFT_ALL_FIELDS);


                                    char *data = NULL;
                                    if (!gb_key) {
                                        if (!GB_have_error()) {
                                            // non-existing field -> assume "" as default value
                                            // (needed to search for missing field using '!=*' ?)
                                            data = strdup("");
                                        }
                                    }
                                    else {
                                        data = GB_read_as_string(gb_key);
                                    }
                                    if (!data) error = GB_await_error();

                                    while (data) {
                                        dbq_assert(ext_query == EXT_QUERY_NONE);
                                        bool        matched    = this_qinfo->matches(data, gb_item); // includes not-op
                                        const char *reason_key = 0;

                                        switch (match_field) {
                                            case AQFT_EXPLICIT:
                                                this_hit   = matched;
                                                reason_key = this_qinfo->getKey();
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
                                            if (strlen(data)>MAX_SHOWN_DATA_SIZE) {
                                                size_t shortened_len = GBS_shorten_repeated_data(data);
                                                if (shortened_len>MAX_SHOWN_DATA_SIZE) {
                                                    strcpy(data+MAX_SHOWN_DATA_SIZE-5, "[...]");
                                                }
                                            }
                                            this_hit_reason       = string(reason_key)+"="+data;
                                            const char *ACIresult = this_qinfo->get_last_ACI_result();
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
                                                dbq_assert(data);
                                            }
                                        }
                                    }

                                    if (this_hit && match_field == AQFT_ALL_FIELDS) {
                                        dbq_assert(this_hit_reason.empty());
                                        this_hit_reason = this_qinfo->shallMatch() ? "<matched all fields>" : "<matched no field>";
                                    }


                                    if (this_hit) {
                                        dbq_assert(!this_hit_reason.empty()); // if we got a hit, we also need a reason
                                        const char *prefix = GBS_global_string("%c%c", '1'+this_qinfo->getIndex(), this_qinfo->shallMatch() ? ' ' : '!');
                                        this_hit_reason    = string(prefix)+this_hit_reason;
                                    }

                                    // calculate result
                                    // (Note: the operator of the 1st query is always OR)
                                    switch (this_op) {
                                        case AND: {
                                            dbq_assert(hit); // otherwise there was no need to run this sub-query
                                            hit        = this_hit;
                                            hit_reason = hit_reason.empty() ? this_hit_reason : hit_reason+" & "+this_hit_reason;
                                            break;
                                        }
                                        case OR: {
                                            dbq_assert(!hit); // otherwise there was no need to run this sub-query
                                            hit        = this_hit;
                                            hit_reason = this_hit_reason;
                                            break;
                                        }
                                        default:
                                            dbq_assert(0);
                                            break;
                                    }
                                    dbq_assert(!hit || !hit_reason.empty()); // if we got a hit, we also need a reason
                                }
                                break;
                            }
                            default: dbq_assert(0); break;
                        }

                        if (hit) {
                            dbq_assert(!hit_reason.empty());

                            if (mode == QUERY_REDUCE) hit_reason = keptHitReason(hit_reason, gb_item, query);
                            SET_QUERIED(gb_item, query, hit_reason.c_str(), hit_reason.length());
                        }
                        else CLEAR_QUERIED(gb_item, query);

                        if (error) {
                            error = GB_failedTo_error("query", GBT_get_name(gb_item), error);
                        }
                        else {
                            progress.inc_and_check_user_abort(error);
                        }
                    }
                }
            }

            if (error) progress.done();
        }
    }

    if (!error) error = qinfo.getError(); // check for query error

    if (error) aw_message(error);
    else DbQuery_update_list(query);

    GB_pop_transaction(query->gb_main);
}

void QUERY::copy_selection_list_2_query_box(DbQuery *query, AW_selection_list *srclist, const char *hit_description) {
    GB_transaction ta(query->gb_main);

    dbq_assert(strstr(hit_description, "%s")); // hit_description needs '%s' (which is replaced by visible content of 'id')

    GB_ERROR         error     = 0;
    AW_window       *aww       = query->aws;
    AW_root         *aw_root   = aww->get_root();
    GB_HASH         *list_hash = srclist->to_hash(false);
    QUERY_MODES  mode      = (QUERY_MODES)aw_root->awar(query->awar_ere)->read_int();
    QUERY_TYPES  type      = (QUERY_TYPES)aw_root->awar(query->awar_by)->read_int();

    if (type == QUERY_MARKED) {
        error = "Query mode 'that are marked' does not apply here.\nEither select 'that match the query' or 'that don't match the q.'";
    }

    if (type != QUERY_MATCH || mode != QUERY_GENERATE) { // different behavior as in the past -> advice
        AW_advice("'Move to hitlist' now depends on the values selected for\n"
                  " * 'Search/Add/Keep species' and\n"
                  " * 'that match/don't match the query'\n"
                  "in the search tool.",
                  AW_ADVICE_TOGGLE_AND_HELP,
                  "Behavior changed",
                  "next_neighbours.hlp");
    }

    long inHitlist = GBS_hash_elements(list_hash);
    long seenInDB  = 0;

    for (GBDATA *gb_species = GBT_first_species(query->gb_main);
         gb_species && !error;
         gb_species = GBT_next_species(gb_species))
    {
        switch (mode) {
            case QUERY_GENERATE: CLEAR_QUERIED(gb_species, query); break;
            case QUERY_ENLARGE:  if (IS_QUERIED(gb_species, query)) continue; break;
            case QUERY_REDUCE:   if (!IS_QUERIED(gb_species, query)) continue; break;
        }

        const char *name      = GBT_get_name(gb_species);
        const char *displayed = reinterpret_cast<const char*>(GBS_read_hash(list_hash, name));

        if (displayed) seenInDB++;

        if ((displayed == 0) == (type == QUERY_DONT_MATCH)) {
            string hit_reason = GBS_global_string(hit_description, displayed ? displayed : "<no near neighbour>");

            if (mode == QUERY_REDUCE) hit_reason = keptHitReason(hit_reason, gb_species, query);
            SET_QUERIED(gb_species, query, hit_reason.c_str(), hit_reason.length());
        }
        else {
            CLEAR_QUERIED(gb_species, query);
        }
    }

    if (seenInDB < inHitlist) {
        aw_message(GBS_global_string("%li of %li hits were found in database", seenInDB, inHitlist));
    }

    GBS_free_hash(list_hash);
    if (error) aw_message(error);
    DbQuery_update_list(query);
}


void QUERY::search_duplicated_field_content(AW_window *, DbQuery *query, bool tokenize) {
    AW_root  *aw_root = query->aws->get_root();
    char     *key     = aw_root->awar(query->awar_keys[0])->read_string();
    GB_ERROR  error   = 0;

    if (strlen(key) == 0) {
        error = "Please select a key (in the first query expression)";
    }
    else {
        GB_transaction dumy(query->gb_main);
        ItemSelector&  selector = query->selector;

        GBDATA      *gb_species_data = GBT_get_species_data(query->gb_main);
        long         hashsize;
        QUERY_RANGE  range           = QUERY_ALL_ITEMS;
        QUERY_TYPES  type            = (QUERY_TYPES)aw_root->awar(query->awar_by)->read_int();

        switch (selector.type) {
            case QUERY_ITEM_SPECIES: {
                hashsize = GB_number_of_subentries(gb_species_data);
                break;
            }
            case QUERY_ITEM_EXPERIMENTS:
            case QUERY_ITEM_GENES: {
                // handle species sub-items
                hashsize = 0;

                for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, aw_root, range);
                     gb_item_container;
                     gb_item_container = selector.get_next_item_container(gb_item_container, range))
                {
                    hashsize += GB_number_of_subentries(gb_item_container);
                }

                break;
            }
            default: {
                dbq_assert(0);
                hashsize = 0;
                break;
            }
        }

        if (!hashsize) {
            error = "No items exist";
        }
        else if (type == QUERY_MARKED) {
            error = "'that are marked' is not applicable here";
        }

        if (!error) {
            GB_HASH *hash = GBS_create_hash(hashsize, GB_IGNORE_CASE);

            for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, aw_root, range);
                 gb_item_container;
                 gb_item_container = selector.get_next_item_container(gb_item_container, range))
            {
                for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
                     gb_item;
                     gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
                {
                    CLEAR_QUERIED(gb_item, query);
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

                                if (IS_QUERIED(gb_old, query)) {
                                    const char *prevInfo = getHitInfo(gb_old, query);
                                    if (!prevInfo) {
                                        oldInfo = firstInfo;
                                    }
                                    else if (strstr(prevInfo, firstInfo) == 0) { // not already have 1st-entry here
                                        oldInfo = GBS_global_string("%s %s", prevInfo, firstInfo);
                                    }
                                }
                                else {
                                    oldInfo = firstInfo;
                                }

                                if (oldInfo) SET_QUERIED(gb_old, query, oldInfo);
                                SET_QUERIED(gb_item, query, GBS_global_string("dup=%s", s));
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
                            if (!IS_QUERIED(gb_old, query)) {
                                SET_QUERIED(gb_old, query, GBS_global_string("%s (1st)", data));
                            }
                            SET_QUERIED(gb_item, query, GBS_global_string("%s (duplicate)", data));
                            GB_write_flag(gb_item, 1);
                        }
                        else {
                            GBS_write_hash(hash, data, (long)gb_item);
                        }
                    }

                    free(data);
                }

                if (type == QUERY_DONT_MATCH) {
                    for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
                         gb_item;
                         gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
                    {
                        if (IS_QUERIED(gb_item, query)) {
                            CLEAR_QUERIED(gb_item, query);
                            GB_write_flag(gb_item, 0); // unmark
                        }
                        else {
                            SET_QUERIED(gb_item, query, tokenize ? "<entry with unique words>" : "<unique entry>");
                        }
                    }
                }
            }

            GBS_free_hash(hash);
        }

        if (type != QUERY_MATCH) {
            AW_advice("'Find equal entries' now depends on the values selected for\n"
                      " * 'that match/don't match the query'\n"
                      "in the search tool.",
                      AW_ADVICE_TOGGLE_AND_HELP,
                      "Behavior changed",
                      "search_duplicates.hlp");
        }
    }

    free(key);

    if (error) aw_message(error);
    DbQuery_update_list(query);
}

static void modify_fields_of_queried_cb(AW_window*, DbQuery *query) {
    ItemSelector&  selector = query->selector;
    AW_root       *aw_root  = query->aws->get_root();
    GB_ERROR       error    = GB_begin_transaction(query->gb_main);

    const char *key;
    if (!error) {
        key = prepare_and_get_selected_itemfield(aw_root, query->awar_parskey, query->gb_main, selector);
        if (!key) error = GB_await_error();
    }

    if (!error) {
        GB_TYPES  key_type        = GBT_get_type_of_changekey(query->gb_main, key, selector.change_key_path);
        bool      safe_conversion = (key_type != GB_STRING) && !aw_root->awar(query->awar_acceptConvError)->read_int();
        char     *command         = aw_root->awar(query->awar_parsvalue)->read_string();

        if (!strlen(command)) error = "Please enter a command";

        if (!error) {
            long  ncount = aw_root->awar(query->awar_count)->read_int();
            char *deftag = aw_root->awar(query->awar_deftag)->read_string();
            char *tag    = aw_root->awar(query->awar_tag)->read_string();

            {
                long use_tag = aw_root->awar(query->awar_use_tag)->read_int();
                if (!use_tag || !strlen(tag)) {
                    freenull(tag);
                }
            }
            int double_pars = aw_root->awar(query->awar_double_pars)->read_int();

            arb_progress progress("Parse fields", ncount);
            QUERY_RANGE  range = (QUERY_RANGE)aw_root->awar(query->awar_where)->read_int();

            for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, aw_root, range);
                 !error && gb_item_container;
                 gb_item_container = selector.get_next_item_container(gb_item_container, range))
            {
                for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
                     !error && gb_item;
                     gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
                {
                    if (IS_QUERIED(gb_item, query)) {
                        GBDATA *gb_new = GB_search(gb_item, key, GB_FIND);

                        if (!gb_new && GB_have_error()) {
                            error = GB_await_error();
                        }
                        else {
                            char *str    = gb_new ? GB_read_as_string(gb_new) : strdup("");
                            char *parsed = 0;

                            if (double_pars) {
                                char *com2 = GB_command_interpreter(query->gb_main, str, command, gb_item, query->get_tree_name());
                                if (com2) {
                                    if (tag) parsed = GBS_string_eval_tagged_string(query->gb_main, "", deftag, tag, 0, com2, gb_item);
                                    else parsed     = GB_command_interpreter       (query->gb_main, "", com2, gb_item, query->get_tree_name());
                                }
                                free(com2);
                            }
                            else {
                                if (tag) parsed = GBS_string_eval_tagged_string(query->gb_main, str, deftag, tag, 0, command, gb_item);
                                else parsed     = GB_command_interpreter       (query->gb_main, str, command, gb_item, query->get_tree_name());
                            }

                            if (!parsed) error = GB_await_error();
                            else {
                                if (strcmp(parsed, str) != 0) { // any change?
                                    if (gb_new && parsed[0] == 0) { // empty result -> delete field
                                        error = GB_delete(gb_new);
                                    }
                                    else {
                                        if (!gb_new) {
                                            gb_new = GB_search(gb_item, key, key_type);
                                            if (!gb_new) error = GB_await_error();
                                        }
                                        if (!error) {
                                            error = GB_write_autoconv_string(gb_new, parsed); // <- field is actually written HERE
                                            if (!error && safe_conversion) {
                                                dbq_assert(key_type != GB_STRING);
                                                char *resulting = GB_read_as_string(gb_new);
                                                if (!resulting) {
                                                    error = GB_await_error();
                                                }
                                                else {
                                                    if (strcmp(resulting, parsed) != 0) {
                                                        error = GBS_global_string("Conversion error: writing '%s'\n"
                                                                                  "resulted in '%s'\n"
                                                                                  "(mark checkbox to accept conversion errors)",
                                                                                  parsed, resulting);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                free(parsed);
                            }
                            free(str);
                            progress.inc_and_check_user_abort(error);
                        }
                    }
                }
            }

            delete tag;
            free(deftag);

            if (error) progress.done();
        }

        free(command);
    }

    error = GB_end_transaction(query->gb_main, error);
    aw_message_if(error);
}

static void predef_prg(AW_root *aw_root, DbQuery *query) {
    char *str = aw_root->awar(query->awar_parspredefined)->read_string();
    char *brk = strchr(str, '#');
    if (brk) {
        *(brk++) = 0;
        char *kv = str;
        if (!strcmp(str, "ali_*/data")) {
            GB_transaction ta(query->gb_main);
            char *use = GBT_get_default_alignment(query->gb_main);
            kv = GBS_global_string_copy("%s/data", use);
            free(use);
        }
        aw_root->awar(query->awar_parskey)->write_string(kv);
        if (kv != str) free(kv);
        aw_root->awar(query->awar_parsvalue)->write_string(brk);
    }
    else {
        aw_root->awar(query->awar_parsvalue)->write_string(str);
    }
    free(str);
}

static void colorize_queried_cb(AW_window *, DbQuery *query) {
    ItemSelector&   selector    = query->selector;
    GB_transaction  ta(query->gb_main);
    GB_ERROR        error       = 0;
    AW_root        *aw_root     = query->aws->get_root();
    int             color_group = aw_root->awar(AWAR_COLORIZE)->read_int();
    QUERY_RANGE     range       = (QUERY_RANGE)aw_root->awar(query->awar_where)->read_int();
    bool            changed     = false;

    for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = selector.get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
             !error && gb_item;
             gb_item       = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
        {
            if (IS_QUERIED(gb_item, query)) {
                error               = GBT_set_color_group(gb_item, color_group);
                if (!error) changed = true;
            }
        }
    }

    if (error) GB_export_error(error);
    else if (changed) selector.trigger_display_refresh();
}

static void colorize_marked_cb(AW_window *aww, BoundItemSel *cmd) {
    ItemSelector&   sel         = cmd->selector;
    GB_transaction  ta(cmd->gb_main);
    GB_ERROR        error       = 0;
    AW_root        *aw_root     = aww->get_root();
    int             color_group = aw_root->awar(AWAR_COLORIZE)->read_int();
    QUERY_RANGE     range       = QUERY_ALL_ITEMS;         // @@@ FIXME: make customizable

    for (GBDATA *gb_item_container = sel.get_first_item_container(cmd->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = sel.get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = sel.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
             !error && gb_item;
             gb_item       = sel.get_next_item(gb_item, QUERY_ALL_ITEMS))
        {
            if (GB_read_flag(gb_item)) {
                error = GBT_set_color_group(gb_item, color_group);
            }
        }
    }

    if (error) GB_export_error(error);
}

enum mark_mode {
    UNMARK,
    MARK,
    INVERT,
};

static void mark_colored_cb(AW_window *aww, BoundItemSel *cmd, mark_mode mode) {
    // @@@ mark_colored_cb is obsolete! (will be replaced by dynamic coloring in the future)
    ItemSelector&  sel         = cmd->selector;
    AW_root       *aw_root     = aww->get_root();
    int            color_group = aw_root->awar(AWAR_COLORIZE)->read_int();
    QUERY_RANGE    range       = QUERY_ALL_ITEMS;          // @@@ FIXME: make customizable

    GB_transaction ta(cmd->gb_main);

    for (GBDATA *gb_item_container = sel.get_first_item_container(cmd->gb_main, aw_root, range);
         gb_item_container;
         gb_item_container = sel.get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = sel.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
             gb_item;
             gb_item = sel.get_next_item(gb_item, QUERY_ALL_ITEMS))
        {
            long my_color = GBT_get_color_group(gb_item);
            if (my_color == color_group) {
                bool marked = GB_read_flag(gb_item);

                switch (mode) {
                    case UNMARK: marked = 0;       break;
                    case MARK:   marked = 1;       break;
                    case INVERT: marked = !marked; break;

                    default: dbq_assert(0); break;
                }

                GB_write_flag(gb_item, marked);
            }
        }
    }

}

// --------------------------------------------------------------------------------
// color sets

struct color_save_data {
    BoundItemSel      *bsel;
    AW_selection_list *colorsets;

    const char *get_items_name() const {
        return bsel->selector.items_name;
    }
};

#define AWAR_COLOR_LOADSAVE_NAME "tmp/colorset/name"

inline GBDATA *get_colorset_root(BoundItemSel *bsel) {
    return GBT_colorset_root(bsel->gb_main, bsel->selector.items_name);
}

static void update_colorset_selection_list(const color_save_data *csd) {
    ConstStrArray foundSets;
    {
        GB_transaction ta(csd->bsel->gb_main);
        GBT_get_colorset_names(foundSets, get_colorset_root(csd->bsel));
    }

    AW_selection_list *sel = csd->colorsets;
    sel->clear();
    sel->insert_default("<new colorset>", "");
    for (size_t i = 0; i<foundSets.size(); ++i) {
        sel->insert(foundSets[i], foundSets[i]);
    }
    sel->update();
}

static void colorset_changed_cb(GBDATA*, const color_save_data *csd, GB_CB_TYPE cbt) {
    if (cbt&GB_CB_CHANGED) {
        update_colorset_selection_list(csd);
    }
}

static void create_colorset_representation(BoundItemSel *bsel, AW_root *aw_root, StrArray& colordefs, GB_ERROR& error) {
    ItemSelector&  sel     = bsel->selector;
    GBDATA        *gb_main = bsel->gb_main;

    dbq_assert(!error);

    for (GBDATA *gb_item_container = sel.get_first_item_container(gb_main, aw_root, QUERY_ALL_ITEMS);
         gb_item_container;
         gb_item_container = sel.get_next_item_container(gb_item_container, QUERY_ALL_ITEMS))
    {
        for (GBDATA *gb_item = sel.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
             gb_item;
             gb_item = sel.get_next_item(gb_item, QUERY_ALL_ITEMS))
        {
            long color = GBT_get_color_group(gb_item);
            if (color>0) {
                char *id        = sel.generate_item_id(gb_main, gb_item);
                char *color_def = GBS_global_string_copy("%s=%li", id, color);

                colordefs.put(color_def);
                free(id);
            }
        }
    }

    if (colordefs.empty()) {
        error = GBS_global_string("Could not find any colored %s", sel.items_name);
    }
}

static GB_ERROR clear_all_colors(BoundItemSel *bsel, AW_root *aw_root) {
    ItemSelector& sel     = bsel->selector;
    GB_ERROR      error   = 0;
    bool          changed = false;

    for (GBDATA *gb_item_container = sel.get_first_item_container(bsel->gb_main, aw_root, QUERY_ALL_ITEMS);
         !error && gb_item_container;
         gb_item_container = sel.get_next_item_container(gb_item_container, QUERY_ALL_ITEMS))
    {
        for (GBDATA *gb_item = sel.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
             !error && gb_item;
             gb_item = sel.get_next_item(gb_item, QUERY_ALL_ITEMS))
        {
            error               = GBT_set_color_group(gb_item, 0); // clear colors
            if (!error) changed = true;
        }
    }

    if (changed && !error) sel.trigger_display_refresh();
    return error;
}

static void clear_all_colors_cb(AW_window *aww, BoundItemSel *bsel) {
    GB_transaction ta(bsel->gb_main);
    GB_ERROR       error = clear_all_colors(bsel, aww->get_root());

    if (error) {
        error = ta.close(error);
        aw_message(error);
    }
}

static GB_ERROR restore_colorset_representation(BoundItemSel *bsel, CharPtrArray& colordefs) {
    ItemSelector&  sel     = bsel->selector;
    GBDATA        *gb_main = bsel->gb_main;
    bool           changed = false;
    GB_ERROR       error   = NULL;
    int            ignores = 0;

    for (size_t d = 0; d<colordefs.size() && !error; ++d) {
        const char *def   = colordefs[d];
        const char *equal = strchr(def, '=');

        if (equal) {
            LocallyModify<char> tempSplit(const_cast<char*>(equal)[0], 0);

            const char *id      = def;
            GBDATA     *gb_item = sel.find_item_by_id(gb_main, id);
            if (!gb_item) {
                aw_message(GBS_global_string("No such %s: '%s' (ignored)", sel.item_name, id)); // only warn
                ++ignores;
            }
            else {
                int color_group = atoi(equal+1);
                if (color_group>0) { // bugfix: saved color groups contained zero (which means "no color") by mistake; ignore
                    error = GBT_set_color_group(gb_item, color_group);
                    if (!error) changed = true;
                }
            }
        }
        else {
            aw_message(GBS_global_string("Invalid colordef '%s' (ignored)", def));
            ++ignores;
        }
    }
    if (changed && !error) sel.trigger_display_refresh();
    if (ignores>0 && !error) {
        aw_message(GBS_global_string("Warning: failed to restore color assignment for %i %s", ignores, sel.items_name));
    }

    return error;
}

enum loadsave_mode {
    SAVE,
    LOAD,
    OVERLAY,
    DELETE,
};

static void loadsave_colorset_cb(AW_window *aws, BoundItemSel *bsel, loadsave_mode mode) {
    AW_root  *aw_root = aws->get_root();
    char     *name    = aw_root->awar(AWAR_COLOR_LOADSAVE_NAME)->read_string();
    GB_ERROR  error   = 0;

    if (name[0] == 0) error = "Please enter a name for the colorset.";
    else {
        GB_transaction ta(bsel->gb_main);

        GBDATA *gb_colorset_root = get_colorset_root(bsel);
        GBDATA *gb_colorset      = gb_colorset_root ? GBT_find_colorset(gb_colorset_root, name) : NULL;

        if (GB_have_error()) error = GB_await_error();
        else {
            if (mode == SAVE) {
                if (!gb_colorset) { // create new colorset
                    gb_colorset             = GBT_find_or_create_colorset(gb_colorset_root, name);
                    if (!gb_colorset) error = GB_await_error();
                }
                dbq_assert(gb_colorset || error);

                if (!error) {
                    StrArray colordefs;
                    create_colorset_representation(bsel, aw_root, colordefs, error);
                    if (!error) error = GBT_save_colorset(gb_colorset, colordefs);
                }
            }
            else {
                if (!gb_colorset) error = GBS_global_string("Colorset '%s' not found", name);
                else {
                    if (mode == LOAD || mode == OVERLAY) {
                        ConstStrArray colordefs;
                        error = GBT_load_colorset(gb_colorset, colordefs);

                        if (!error && colordefs.empty()) error = "oops.. empty colorset";
                        if (!error && mode == LOAD)      error = clear_all_colors(bsel, aw_root);
                        if (!error)                      error = restore_colorset_representation(bsel, colordefs);
                    }
                    else {
                        dbq_assert(mode == DELETE);
                        error = GB_delete(gb_colorset);
                    }
                }
            }
        }
        error = ta.close(error);
    }
    free(name);

    if (error) aw_message(error);
}

static AW_window *create_loadsave_colored_window(AW_root *aw_root, color_save_data *csd) {
    static AW_window **aw_loadsave = 0;
    if (!aw_loadsave) {
        // init data
        aw_root->awar_string(AWAR_COLOR_LOADSAVE_NAME, "", AW_ROOT_DEFAULT);
        ARB_calloc(aw_loadsave, QUERY_ITEM_TYPES); // contains loadsave windows for each item type
    }

    QUERY_ITEM_TYPE type = csd->bsel->selector.type;

    dbq_assert(type<QUERY_ITEM_TYPES);

    if (!aw_loadsave[type]) {
        // init window
        AW_window_simple *aws = new AW_window_simple;
        {
            char *window_id = GBS_global_string_copy("colorset_loadsave_%s", csd->get_items_name());
            aws->init(aw_root, window_id, GBS_global_string("Load/Save %s colorset", csd->get_items_name()));
            free(window_id);
        }

        aws->load_xfig("query/color_loadsave.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("color_loadsave.hlp"));
        aws->create_button("HELP", "HELP", "H");

        dbq_assert(csd->colorsets == 0);
        csd->colorsets = awt_create_selection_list_with_input_field(aws, AWAR_COLOR_LOADSAVE_NAME, "list", "name");

        update_colorset_selection_list(csd);

        aws->at("save");    aws->callback(makeWindowCallback(loadsave_colorset_cb, csd->bsel, SAVE));    aws->create_button("save",    "Save",    "S");
        aws->at("load");    aws->callback(makeWindowCallback(loadsave_colorset_cb, csd->bsel, LOAD));    aws->create_button("load",    "Load",    "L");
        aws->at("overlay"); aws->callback(makeWindowCallback(loadsave_colorset_cb, csd->bsel, OVERLAY)); aws->create_button("overlay", "Overlay", "O");
        aws->at("delete");  aws->callback(makeWindowCallback(loadsave_colorset_cb, csd->bsel, DELETE));  aws->create_button("delete",  "Delete",  "D");

        aws->at("reset");
        aws->callback(makeWindowCallback(clear_all_colors_cb, csd->bsel));
        aws->create_button("reset", "Reset", "R");

        // callbacks
        {
            GB_transaction  ta(csd->bsel->gb_main);
            GBDATA         *gb_colorset = get_colorset_root(csd->bsel);
            GB_add_callback(gb_colorset, GB_CB_CHANGED, makeDatabaseCallback(colorset_changed_cb, csd));
        }

        aw_loadsave[type] = aws;
    }

    return aw_loadsave[type];
}

static AW_window *create_colorize_window(AW_root *aw_root, GBDATA *gb_main, DbQuery *query, ItemSelector *sel) {
    // invoked by   'colorize listed'                   (sel   != 0)
    // and          'colorize marked/mark colored'      (query != 0)

    enum { COLORIZE_INVALID, COLORIZE_LISTED, COLORIZE_MARKED } mode = COLORIZE_INVALID;

    create_query_independent_awars(aw_root, AW_ROOT_DEFAULT);

    AW_window_simple *aws = new AW_window_simple;

    dbq_assert(contradicted(query, sel));

    if (query) {
        dbq_assert(mode == COLORIZE_INVALID);
        mode = COLORIZE_LISTED;
    }
    if (sel) {
        dbq_assert(mode == COLORIZE_INVALID);
        mode = COLORIZE_MARKED;
    }
    dbq_assert(!(mode == COLORIZE_INVALID));

    ItemSelector& Sel  = mode == COLORIZE_LISTED ? query->selector : *sel;
    const char   *what = mode == COLORIZE_LISTED ? "listed" : "marked";

    {
        char *macro_name  = GBS_global_string_copy("COLORIZE_%s_%s", what, Sel.items_name);
        char *window_name = GBS_global_string_copy("Colorize %s %s", what, Sel.items_name);

        aws->init(aw_root, macro_name, window_name);

        free(window_name);
        free(macro_name);
    }

    aws->load_xfig("query/colorize.fig");

    aws->auto_space(10, 10);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback(mode == COLORIZE_LISTED ? "set_color_of_listed.hlp" : "colorize.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("colorize");

    BoundItemSel *bsel = new BoundItemSel(gb_main, ((mode == COLORIZE_MARKED) ? *sel : query->selector)); // do not free, bound to CB

    if (mode == COLORIZE_LISTED) aws->callback(makeWindowCallback(colorize_queried_cb, query));
    else                         aws->callback(makeWindowCallback(colorize_marked_cb, bsel));

    aws->create_autosize_button("COLORIZE", GBS_global_string_copy("Set color of %s %s to ...", what, Sel.items_name), "S", 2);

    {
        int color_group;

        aws->create_option_menu(AWAR_COLORIZE, true);
        aws->insert_default_option("No color group", "none", 0);
        for (color_group = 1; color_group <= AW_COLOR_GROUPS; ++color_group) {
            char *name = AW_get_color_group_name(aw_root, color_group);
            aws->insert_option(name, "", color_group);
            free(name);
        }
        aws->update_option_menu();
    }

    color_save_data *csd = new color_save_data; // do not free, bound to CB
    csd->bsel            = bsel;
    csd->colorsets       = 0;

    aws->at("loadsave");
    aws->callback(makeCreateWindowCallback(create_loadsave_colored_window, csd));
    aws->create_autosize_button("LOADSAVE_COLORED", "Load/Save", "L");

    if (mode == COLORIZE_MARKED) {
        aws->at("mark");
        aws->callback(makeWindowCallback(mark_colored_cb, bsel, MARK));
        aws->create_autosize_button("MARK_COLORED", GBS_global_string_copy("Mark all %s of ...", Sel.items_name), "M", 2);

        aws->at("unmark");
        aws->callback(makeWindowCallback(mark_colored_cb, bsel, UNMARK));
        aws->create_autosize_button("UNMARK_COLORED", GBS_global_string_copy("Unmark all %s of ...", Sel.items_name), "U", 2);

        aws->at("invert");
        aws->callback(makeWindowCallback(mark_colored_cb, bsel, INVERT));
        aws->create_autosize_button("INVERT_COLORED", GBS_global_string_copy("Invert all %s of ...", Sel.items_name), "I", 2);
    }

    aws->at_newline();
    aws->window_fit();

    return aws;
}

static AW_window *create_colorize_queried_window(AW_root *aw_root, DbQuery *query) {
    return create_colorize_window(aw_root, query->gb_main, query, NULL);
}

AW_window *QUERY::create_colorize_items_window(AW_root *aw_root, GBDATA *gb_main, ItemSelector& sel) {
    return create_colorize_window(aw_root, gb_main, NULL, &sel);
}

static void setup_modify_fields_config(AWT_config_definition& cdef, const DbQuery *query) {
    const char *typeawar = get_itemfield_type_awarname(query->awar_parskey);
    dbq_assert(typeawar);

    cdef.add(query->awar_parskey,         "key");
    cdef.add(typeawar,                    "type");
    cdef.add(query->awar_use_tag,         "usetag");
    cdef.add(query->awar_deftag,          "deftag");
    cdef.add(query->awar_tag,             "modtag");
    cdef.add(query->awar_double_pars,     "doubleparse");
    cdef.add(query->awar_acceptConvError, "acceptconv");
    cdef.add(query->awar_parsvalue,       "aci");
}

static AW_window *create_modify_fields_window(AW_root *aw_root, DbQuery *query) {
    AW_window_simple *aws = new AW_window_simple;

    {
        char *macro_name = GBS_global_string_copy("MODIFY_DATABASE_FIELD_%s", query->selector.items_name);
        char *window_name = GBS_global_string_copy("MODIFY DATABASE FIELD of listed %s", query->selector.items_name);

        aws->init(aw_root, macro_name, window_name);

        free(window_name);
        free(macro_name);
    }

    aws->load_xfig("query/modify_fields.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("mod_field_list.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("helptags");
    aws->callback(makeHelpCallback("tags.hlp"));
    aws->create_button("HELP_TAGS", "Help tags", "H");

    create_itemfield_selection_button(aws, FieldSelDef(query->awar_parskey, query->gb_main, query->selector, FIELD_FILTER_STRING_READABLE, "target field", SF_ALLOW_NEW), "field");
    aws->at("accept");  aws->create_toggle(query->awar_acceptConvError);

    // ------------------------
    //      expert section
    aws->at("usetag");  aws->create_toggle(query->awar_use_tag);
    aws->at("deftag");  aws->create_input_field(query->awar_deftag);
    aws->at("tag");     aws->create_input_field(query->awar_tag);
    aws->at("double");  aws->create_toggle(query->awar_double_pars);

    aws->at("parser");
    aws->create_text_field(query->awar_parsvalue);

    aws->at("go");
    aws->callback(makeWindowCallback(modify_fields_of_queried_cb, query));
    aws->create_button("GO", "GO", "G");

    aws->at("pre");
    AW_selection_list *programs = aws->create_selection_list(query->awar_parspredefined, true);

    const char *sellst = NULL;
    switch (query->selector.type) {
        case QUERY_ITEM_SPECIES:     sellst = "mod_fields*.sellst"; break;
        case QUERY_ITEM_GENES:       sellst = "mod_gene_fields*.sellst"; break;
        case QUERY_ITEM_EXPERIMENTS: sellst = "mod_experiment_fields*.sellst"; break;
        default: dbq_assert(0); break;
    }

    GB_ERROR error;
    if (sellst) {
        StorableSelectionList storable_sellist(TypedSelectionList("sellst", programs, "field modification scripts", "mod_fields"));
        error = storable_sellist.load(GB_path_in_ARBLIB("sellists", sellst), false);
    }
    else {
        error = "No default selection list for query-type";
    }

    if (error) {
        aw_message(error);
    }
    else {
        aws->get_root()->awar(query->awar_parspredefined)->add_callback(makeRootCallback(predef_prg, query));
    }

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT,
                              GBS_global_string("mod_%s_fields", query->selector.item_name),
                              makeConfigSetupCallback(setup_modify_fields_config, query));

    return aws;
}

static void set_field_of_queried_cb(AW_window*, DbQuery *query, bool append) {
    // set fields of listed items
    ItemSelector&  selector   = query->selector;
    GB_ERROR       error      = NULL;
    AW_root       *aw_root    = query->aws->get_root();
    bool           allow_loss = aw_root->awar(query->awar_writelossy)->read_int();

    char *value = aw_root->awar(query->awar_setvalue)->read_string();
    if (value[0] == 0) freenull(value);

    size_t value_len = value ? strlen(value) : 0;

    GB_begin_transaction(query->gb_main);

    const char *key = prepare_and_get_selected_itemfield(aw_root, query->awar_writekey, query->gb_main, selector);
    if (!key) error = GB_await_error();

    for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, aw_root, QUERY_ALL_ITEMS);
         !error && gb_item_container;
         gb_item_container = selector.get_next_item_container(gb_item_container, QUERY_ALL_ITEMS))
    {
        for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
             !error && gb_item;
             gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
        {
            if (IS_QUERIED(gb_item, query)) {
                if (!value) { // empty value -> delete field
                    if (!append) {
                        GBDATA *gb_field    = GB_search(gb_item, key, GB_FIND);
                        if (gb_field) error = GB_delete(gb_field);
                    }
                    // otherwise "nothing" is appended (=noop)
                }
                else {
                    GBDATA *gb_field     = GBT_searchOrCreate_itemfield_according_to_changekey(gb_item, key, selector.change_key_path);
                    if (!gb_field) error = GB_await_error();
                    else {
                        const char   *new_content = value;
                        SmartCharPtr  content;

                        if (append) {
                            char *old       = GB_read_as_string(gb_field);
                            if (!old) error = "field has incompatible type";
                            else {
                                size_t         old_len = strlen(old);
                                GBS_strstruct *strstr  = GBS_stropen(old_len+value_len+2);

                                GBS_strncat(strstr, old, old_len);
                                GBS_strncat(strstr, value, value_len);

                                content     = GBS_strclose(strstr);
                                new_content = &*content;
                            }
                        }

                        error = GB_write_autoconv_string(gb_field, new_content);

                        if (!allow_loss && !error) {
                            char *result = GB_read_as_string(gb_field);
                            dbq_assert(result);

                            if (strcmp(new_content, result) != 0) {
                                error = GBS_global_string("value modified by type conversion\n('%s' -> '%s')", new_content, result);
                            }
                            free(result);
                        }
                    }
                }

                if (error) { // add name of current item to error
                    const char *name = GBT_read_char_pntr(gb_item, "name");
                    error            = GBS_global_string("Error while writing to %s '%s':\n%s", selector.item_name, name, error);
                }
            }
        }
    }

    GB_end_transaction_show_error(query->gb_main, error, aw_message);

    free(value);
}

static AW_window *create_writeFieldOfListed_window(AW_root *aw_root, DbQuery *query) {
    AW_window_simple *aws = new AW_window_simple;
    init_itemType_specific_window(aw_root, aws, query->selector, "WRITE_DB_FIELD_OF_LISTED", "Write field of listed %s", true);
    aws->load_xfig("query/write_fields.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("write_field_list.hlp"));
    aws->create_button("HELP", "HELP", "H");

    create_itemfield_selection_button(aws, FieldSelDef(query->awar_writekey, query->gb_main, query->selector, FIELD_FILTER_STRING_READABLE, "target field", SF_ALLOW_NEW), "field");

    aws->at("val");
    aws->create_text_field(query->awar_setvalue, 2, 2);

    aws->at("lossy");
    aws->label("Allow lossy conversion?");
    aws->create_toggle(query->awar_writelossy);

    aws->at("create");
    aws->callback(makeWindowCallback(set_field_of_queried_cb, query, false));
    aws->create_button("SET_SINGLE_FIELD_OF_LISTED", "WRITE");

    aws->at("do");
    aws->callback(makeWindowCallback(set_field_of_queried_cb, query, true));
    aws->create_button("APPEND_SINGLE_FIELD_OF_LISTED", "APPEND");

    return aws;
}

static void set_protection_of_queried_cb(AW_window*, DbQuery *query) {
    // set protection of listed items
    ItemSelector&  selector = query->selector;
    AW_root       *aw_root  = query->aws->get_root();
    char          *key      = aw_root->awar(query->awar_protectkey)->read_string();

    if (strcmp(key, NO_FIELD_SELECTED) == 0) {
        aw_message("Please select a field for which you want to set the protection");
    }
    else {
        GB_begin_transaction(query->gb_main);

        GB_ERROR  error       = 0;
        GBDATA   *gb_key_data = GB_search(query->gb_main, selector.change_key_path, GB_CREATE_CONTAINER);
        GBDATA   *gb_key_name = GB_find_string(gb_key_data, CHANGEKEY_NAME, key, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

        if (!gb_key_name) {
            error = GBS_global_string("The destination field '%s' does not exists", key);
        }
        else {
            int         level = aw_root->awar(query->awar_setprotection)->read_int();
            QUERY_RANGE range = (QUERY_RANGE)aw_root->awar(query->awar_where)->read_int();

            for (GBDATA *gb_item_container = selector.get_first_item_container(query->gb_main, aw_root, range);
                 !error && gb_item_container;
                 gb_item_container = selector.get_next_item_container(gb_item_container, range))
            {
                for (GBDATA *gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
                     !error && gb_item;
                     gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
                {
                    if (IS_QUERIED(gb_item, query)) {
                        GBDATA *gb_new = GB_search(gb_item, key, GB_FIND);
                        if (gb_new) {
                            error             = GB_write_security_delete(gb_new, level);
                            if (!error) error = GB_write_security_write(gb_new, level);
                        }
                    }
                }
            }
        }
        GB_end_transaction_show_error(query->gb_main, error, aw_message);
    }

    free(key);
}

static AW_window *create_set_protection_window(AW_root *aw_root, DbQuery *query) {
    AW_window_simple *aws = new AW_window_simple;
    init_itemType_specific_window(aw_root, aws, query->selector, "SET_PROTECTION_OF_FIELD_OF_LISTED", "Protect field of listed %s", true);
    aws->load_xfig("query/set_protection.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("set_protection.hlp"));
    aws->create_button("HELP", "HELP", "H");


    aws->at("prot");
    aws->create_toggle_field(query->awar_setprotection, 0);
    aws->insert_toggle("0 temporary", "0", 0);
    aws->insert_toggle("1 checked",   "1", 1);
    aws->insert_toggle("2",           "2", 2);
    aws->insert_toggle("3",           "3", 3);
    aws->insert_toggle("4 normal",    "4", 4);
    aws->insert_toggle("5 ",          "5", 5);
    aws->insert_toggle("6 the truth", "5", 6);
    aws->update_toggle_field();

    create_itemfield_selection_list(aws, FieldSelDef(query->awar_protectkey, query->gb_main, query->selector, FIELD_UNFILTERED, "field to protect"), "list");

    aws->at("go");
    aws->callback(makeWindowCallback(set_protection_of_queried_cb, query));
    aws->create_autosize_button("SET_PROTECTION_OF_FIELD_OF_LISTED", "Assign\nprotection\nto field\nof listed");

    return aws;
}

static void toggle_flag_cb(AW_window *aww, DbQuery *query) {
    GB_transaction  ta(query->gb_main);
    GBDATA         *gb_item = query->selector.get_selected_item(query->gb_main, aww->get_root());
    if (gb_item) {
        long flag = GB_read_flag(gb_item);
        GB_write_flag(gb_item, 1-flag);
    }
    DbQuery_update_list(query);
}

static void new_selection_made_cb(AW_root *aw_root, const char *awar_selection, DbQuery *query) {
    char *item_name = aw_root->awar(awar_selection)->read_as_string();
    query->selector.update_item_awars(query->gb_main, aw_root, item_name);
    free(item_name);
}

static void query_box_setup_config(AWT_config_definition& cdef, DbQuery *query) {
    // this defines what is saved/restored to/from configs
    for (int key_id = 0; key_id<QUERY_SEARCHES; ++key_id) {
        cdef.add(query->awar_keys[key_id], "key", key_id);
        cdef.add(query->awar_queries[key_id], "query", key_id);
        cdef.add(query->awar_not[key_id], "not", key_id);
        cdef.add(query->awar_operator[key_id], "operator", key_id);
    }
}

template<typename CB>
static void query_rel_menu_entry(AW_window *aws, const char *id, const char *query_id, const char* label, const char *mnemonic, const char *helpText, AW_active Mask, const CB& cb) {
    char *rel_id = GBS_global_string_copy("%s_%s", query_id, id);
    aws->insert_menu_topic(rel_id, label, mnemonic, helpText, Mask, cb);
    free(rel_id);
}

const char *DbQuery::get_tree_name() const {
    if (!awar_tree_name) 
        return NULL;
    else
        return aws->get_root()->awar(awar_tree_name)->read_char_pntr();
}

DbQuery::~DbQuery() {
    for (int s = 0; s<QUERY_SEARCHES; ++s) {
        free(awar_keys[s]);
        free(awar_queries[s]);
        free(awar_not[s]);
        free(awar_operator[s]);
    }

    free(species_name);

    free(awar_writekey);
    free(awar_writelossy);
    free(awar_protectkey);
    free(awar_setprotection);
    free(awar_setvalue);

    free(awar_parskey);
    free(awar_parsvalue);
    free(awar_parspredefined);
    free(awar_acceptConvError);

    free(awar_ere);
    free(awar_where);
    free(awar_by);
    free(awar_use_tag);
    free(awar_double_pars);
    free(awar_deftag);
    free(awar_tag);
    free(awar_count);
    free(awar_sort);

    GBS_free_hash(hit_description);
}

// ----------------------------------------
// store query data until DB close

typedef Keeper<DbQuery*> QueryKeeper;
template<> void Keeper<DbQuery*>::destroy(DbQuery *q) { delete q; }
static SmartPtr<QueryKeeper> queryKeeper;
static void destroyKeptQueries(GBDATA*, void*) {
    queryKeeper.SetNull();
}
static void keepQuery(GBDATA *gbmain, DbQuery *q) {
    if (queryKeeper.isNull()) {
        queryKeeper = new QueryKeeper;
        GB_atclose(gbmain, destroyKeptQueries, NULL);
    }
    queryKeeper->keep(q);
}

// ----------------------------------------

DbQuery *QUERY::create_query_box(AW_window *aws, query_spec *awtqs, const char *query_id) {
    char     buffer[256];
    AW_root *aw_root = aws->get_root();
    GBDATA  *gb_main = awtqs->gb_main;
    DbQuery *query   = new DbQuery(awtqs->get_queried_itemtype());

    keepQuery(gb_main, query); // transfers ownership of query

    // @@@ set all the things below via ctor!
    // @@@ create a copyable object containing everything query_spec and DbQuery have in common -> pass that object to DbQuery-ctor

    query->gb_main                = awtqs->gb_main;
    query->aws                    = aws;
    query->gb_ref                 = awtqs->gb_ref;
    query->expect_hit_in_ref_list = awtqs->expect_hit_in_ref_list;
    query->select_bit             = awtqs->select_bit;
    query->species_name           = strdup(awtqs->species_name);

    query->set_tree_awar_name(awtqs->tree_name);
    query->hit_description = GBS_create_dynaval_hash(query_count_items(query, QUERY_ALL_ITEMS, QUERY_GENERATE), GB_IGNORE_CASE, free_hit_description);

    GB_push_transaction(gb_main);

    // Create query box AWARS
    create_query_independent_awars(aw_root, AW_ROOT_DEFAULT);

    {
        const char *default_key[QUERY_SEARCHES+1] = { "name", "name", "name", 0 };

        for (int key_id = 0; key_id<QUERY_SEARCHES; ++key_id) {
            sprintf(buffer, "tmp/dbquery_%s/key_%i", query_id, key_id);
            query->awar_keys[key_id] = strdup(buffer);
            dbq_assert(default_key[key_id] != 0);
            aw_root->awar_string(query->awar_keys[key_id], default_key[key_id], AW_ROOT_DEFAULT);

            sprintf(buffer, "tmp/dbquery_%s/query_%i", query_id, key_id);
            query->awar_queries[key_id] = strdup(buffer);
            aw_root->awar_string(query->awar_queries[key_id], "*", AW_ROOT_DEFAULT);

            sprintf(buffer, "tmp/dbquery_%s/not_%i", query_id, key_id);
            query->awar_not[key_id] = strdup(buffer);
            aw_root->awar_int(query->awar_not[key_id], 0, AW_ROOT_DEFAULT);

            sprintf(buffer, "tmp/dbquery_%s/operator_%i", query_id, key_id);
            query->awar_operator[key_id] = strdup(buffer);
            aw_root->awar_string(query->awar_operator[key_id], "ign", AW_ROOT_DEFAULT);
        }
        aw_root->awar(query->awar_keys[0])->add_callback(makeRootCallback(first_searchkey_changed_cb, query));
    }

    sprintf(buffer, "tmp/dbquery_%s/ere", query_id);
    query->awar_ere = strdup(buffer);
    aw_root->awar_int(query->awar_ere, QUERY_GENERATE);

    sprintf(buffer, "tmp/dbquery_%s/where", query_id);
    query->awar_where = strdup(buffer);
    aw_root->awar_int(query->awar_where, QUERY_ALL_ITEMS);

    sprintf(buffer, "tmp/dbquery_%s/count", query_id);
    query->awar_count = strdup(buffer);
    aw_root->awar_int(query->awar_count, 0);

    sprintf(buffer, "tmp/dbquery_%s/sort", query_id);
    query->awar_sort = strdup(buffer);
    aw_root->awar_int(query->awar_sort, QUERY_SORT_NONE)->add_callback(makeRootCallback(result_sort_order_changed_cb, query));
    query->sort_mask = QUERY_SORT_NONE; // default to unsorted

    sprintf(buffer, "tmp/dbquery_%s/by", query_id);
    query->awar_by = strdup(buffer);
    aw_root->awar_int(query->awar_by, QUERY_MATCH);

    if (awtqs->ere_pos_fig) {
        aws->at(awtqs->ere_pos_fig);
        aws->create_toggle_field(query->awar_ere, "", "");

        aws->insert_toggle(GBS_global_string("Search %s", query->selector.items_name), "G", (int)QUERY_GENERATE);
        aws->insert_toggle(GBS_global_string("Add %s", query->selector.items_name), "E", (int)QUERY_ENLARGE);
        aws->insert_toggle(GBS_global_string("Keep %s", query->selector.items_name), "R", (int)QUERY_REDUCE);

        aws->update_toggle_field();
    }
    if (awtqs->where_pos_fig) {
        aws->at(awtqs->where_pos_fig);
        aws->create_toggle_field(query->awar_where, "", "");
        aws->insert_toggle("of current organism", "C", (int)QUERY_CURRENT_ITEM);
        aws->insert_toggle("of marked organisms", "M", (int)QUERY_MARKED_ITEMS);
        aws->insert_toggle("of all organisms", "A", (int)QUERY_ALL_ITEMS);
        aws->update_toggle_field();

    }
    if (awtqs->by_pos_fig) {
        aws->at(awtqs->by_pos_fig);
        aws->create_toggle_field(query->awar_by, "", "");
        aws->insert_toggle("that match the query", "M", (int)QUERY_MATCH);
        aws->insert_toggle("that don't match the q.", "D", (int)QUERY_DONT_MATCH);
        aws->insert_toggle("that are marked", "A", (int)QUERY_MARKED);
        aws->update_toggle_field();
    }

    // distances for multiple queries :

#define KEY_Y_OFFSET 32

    int xpos_calc[3] = { -1, -1, -1 }; // X-positions for elements in search expressions

    if (awtqs->qbox_pos_fig) {
        aws->auto_space(1, 1);
        aws->at(awtqs->qbox_pos_fig);

        SmartPtr<AW_at_storage> at_size(AW_at_storage::make(aws, AW_AT_SIZE_AND_ATTACH));

        int xpos, ypos;
        aws->get_at_position(&xpos, &ypos);

        int ypos_dummy;

        for (int key = QUERY_SEARCHES-1;  key >= 0; --key) {
            if (key) {
                aws->at(xpos, ypos+key*KEY_Y_OFFSET);
                aws->create_option_menu(query->awar_operator[key], true);
                aws->insert_option("and", "", "and");
                aws->insert_option("or", "", "or");
                aws->insert_option("ign", "", "ign");
                aws->update_option_menu();

                if (xpos_calc[0] == -1) aws->get_at_position(&xpos_calc[0], &ypos_dummy);
            }

            aws->at(xpos_calc[0], ypos+key*KEY_Y_OFFSET);
            aws->restore_at_from(*at_size);

            create_itemfield_selection_button(aws, FieldSelDef(query->awar_keys[key], gb_main, awtqs->get_queried_itemtype(), FIELD_FILTER_STRING_READABLE, "source field", SF_PSEUDO), NULL);

            if (xpos_calc[1] == -1) aws->get_at_position(&xpos_calc[1], &ypos_dummy);

            aws->at(xpos_calc[1], ypos+key*KEY_Y_OFFSET);
            aws->create_toggle(query->awar_not[key], "#equal.xpm", "#notEqual.xpm");

            if (xpos_calc[2] == -1) aws->get_at_position(&xpos_calc[2], &ypos_dummy);
        }
    }
    if (awtqs->key_pos_fig) {
        aws->at(awtqs->key_pos_fig);
        aws->create_input_field(query->awar_keys[0], 12);
    }

    if (awtqs->query_pos_fig) {
        aws->at(awtqs->query_pos_fig);

        SmartPtr<AW_at_storage> at_size(AW_at_storage::make(aws, AW_AT_SIZE_AND_ATTACH));

        int xpos, ypos;
        aws->get_at_position(&xpos, &ypos);

        for (int key = 0; key<QUERY_SEARCHES; ++key) {
            aws->at(xpos_calc[2], ypos+key*KEY_Y_OFFSET);
            aws->restore_at_from(*at_size);
#if defined(ARB_MOTIF)
            aws->d_callback(makeWindowCallback(perform_query_cb, query, EXT_QUERY_NONE)); // enable ENTER in searchfield to start search
#endif
            aws->create_input_field(query->awar_queries[key], 12);
        }
    }

    if (awtqs->result_pos_fig) {
        aws->at(awtqs->result_pos_fig);
        aws->d_callback(makeWindowCallback(toggle_flag_cb, query));

        {
            const char *this_awar_name = ARB_keep_string(GBS_global_string_copy("tmp/dbquery_%s/select", query_id)); // do not free this cause it's passed to new_selection_made_cb
            AW_awar    *awar           = aw_root->awar_string(this_awar_name, "", AW_ROOT_DEFAULT);

            query->hitlist = aws->create_selection_list(this_awar_name, 5, 5, true);
            awar->add_callback(makeRootCallback(new_selection_made_cb, this_awar_name, query));
        }
        query->hitlist->insert_default("end of list", "");
        query->hitlist->update();
    }

    if (awtqs->count_pos_fig) {
        aws->button_length(13);
        aws->at(awtqs->count_pos_fig);
        aws->label("Hits:");
        aws->create_button(0, query->awar_count, 0, "+");

        if (awtqs->popup_info_window) {
            aws->callback(RootAsWindowCallback::simple(awtqs->popup_info_window, query->gb_main));
            aws->create_button("popinfo", "Info");
        }
    }
    else {
        dbq_assert(!awtqs->popup_info_window); // dont know where to display
    }

    if (awtqs->config_pos_fig) {
        aws->button_length(0);
        aws->at(awtqs->config_pos_fig);
        char *macro_id = GBS_global_string_copy("SAVELOAD_CONFIG_%s", query_id);
        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "query_box", makeConfigSetupCallback(query_box_setup_config, query), macro_id);
        free(macro_id);
    }

    aws->button_length(18);

    if (awtqs->do_query_pos_fig) {
        aws->at(awtqs->do_query_pos_fig);
        aws->callback(makeWindowCallback(perform_query_cb, query, EXT_QUERY_NONE));
        char *macro_id = GBS_global_string_copy("SEARCH_%s", query_id);
#if defined(ARB_GTK)
        aws->highlight();
#endif
        aws->create_button(macro_id, "Search");
        free(macro_id);
    }
    if (awtqs->do_refresh_pos_fig) {
        aws->at(awtqs->do_refresh_pos_fig);
        aws->create_option_menu(query->awar_sort, true);
        aws->insert_default_option("unsorted",  "u", QUERY_SORT_NONE);
        aws->insert_option        ("by value",  "v", QUERY_SORT_BY_1STFIELD_CONTENT);
        aws->insert_option        ("by id",     "n", QUERY_SORT_BY_ID);
        if (query->selector.parent_selector) {
            aws->insert_option    ("by parent", "p", QUERY_SORT_BY_NESTED_PID);
        }
        aws->insert_option        ("by marked", "m", QUERY_SORT_BY_MARKED);
        aws->insert_option        ("by hit",    "h", QUERY_SORT_BY_HIT_DESCRIPTION);
        aws->insert_option        ("reverse",   "r", QUERY_SORT_REVERSE);
        aws->update_option_menu();
    }
    else {
        dbq_assert(0); // hmm - no sort button here. -> tell ralf where this happens
    }

    if (awtqs->do_mark_pos_fig) {
        aws->at(awtqs->do_mark_pos_fig);
        aws->help_text("mark_list.hlp");
        aws->callback(makeWindowCallback(mark_queried_cb, query, 1));
        aws->create_button("MARK_LISTED_UNMARK_REST", "Mark Listed\nUnmark Rest", "M");
    }
    if (awtqs->do_unmark_pos_fig) {
        aws->at(awtqs->do_unmark_pos_fig);
        aws->help_text("unmark_list.hlp");
        aws->callback(makeWindowCallback(mark_queried_cb, query, 0));
        aws->create_button("UNMARK_LISTED_MARK_REST", "Unmark Listed\nMark Rest", "U");
    }
    if (awtqs->do_delete_pos_fig) {
        aws->at(awtqs->do_delete_pos_fig);
        aws->help_text("del_list.hlp");
        aws->callback(makeWindowCallback(delete_queried_species_cb, query));
        char *macro_id = GBS_global_string_copy("DELETE_LISTED_%s", query_id);
        aws->create_button(macro_id, "Delete Listed", "D");
        free(macro_id);
    }
    if (awtqs->do_set_pos_fig) {
        query->awar_writekey      = GBS_global_string_copy("tmp/dbquery_%s/write_key",      query_id); aw_root->awar_string(query->awar_writekey);
        query->awar_writelossy    = GBS_global_string_copy("tmp/dbquery_%s/write_lossy",    query_id); aw_root->awar_int   (query->awar_writelossy);
        query->awar_protectkey    = GBS_global_string_copy("tmp/dbquery_%s/protect_key",    query_id); aw_root->awar_string(query->awar_protectkey);
        query->awar_setprotection = GBS_global_string_copy("tmp/dbquery_%s/set_protection", query_id); aw_root->awar_int   (query->awar_setprotection, 4);
        query->awar_setvalue      = GBS_global_string_copy("tmp/dbquery_%s/set_value",      query_id); aw_root->awar_string(query->awar_setvalue);

        aws->at(awtqs->do_set_pos_fig);
        aws->help_text("mod_field_list.hlp");
        aws->callback(makeCreateWindowCallback(create_writeFieldOfListed_window, query));
        char *macro_id = GBS_global_string_copy("WRITE_TO_FIELDS_OF_LISTED_%s", query_id);
        aws->create_button(macro_id, "Write to Fields\nof Listed", "S");
        free(macro_id);
    }

    char *Items = strdup(query->selector.items_name);
    Items[0]    = toupper(Items[0]);

    if ((awtqs->use_menu || awtqs->open_parser_pos_fig)) {
        sprintf(buffer, "tmp/dbquery_%s/tag",                 query_id); query->awar_tag             = strdup(buffer); aw_root->awar_string(query->awar_tag);
        sprintf(buffer, "tmp/dbquery_%s/use_tag",             query_id); query->awar_use_tag         = strdup(buffer); aw_root->awar_int   (query->awar_use_tag);
        sprintf(buffer, "tmp/dbquery_%s/deftag",              query_id); query->awar_deftag          = strdup(buffer); aw_root->awar_string(query->awar_deftag);
        sprintf(buffer, "tmp/dbquery_%s/double_pars",         query_id); query->awar_double_pars     = strdup(buffer); aw_root->awar_int   (query->awar_double_pars);
        sprintf(buffer, "tmp/dbquery_%s/parskey",             query_id); query->awar_parskey         = strdup(buffer); aw_root->awar_string(query->awar_parskey);
        sprintf(buffer, "tmp/dbquery_%s/parsvalue",           query_id); query->awar_parsvalue       = strdup(buffer); aw_root->awar_string(query->awar_parsvalue);
        sprintf(buffer, "tmp/dbquery_%s/awar_parspredefined", query_id); query->awar_parspredefined  = strdup(buffer); aw_root->awar_string(query->awar_parspredefined);
        sprintf(buffer, "tmp/dbquery_%s/acceptConvError",     query_id); query->awar_acceptConvError = strdup(buffer); aw_root->awar_int   (query->awar_acceptConvError);

        if (awtqs->use_menu) {
            sprintf(buffer, "Modify Fields of Listed %s", Items); query_rel_menu_entry(aws, "mod_fields_of_listed", query_id, buffer, "F", "mod_field_list.hlp", AWM_ALL, makeCreateWindowCallback(create_modify_fields_window, query));
        }
        else {
            aws->at(awtqs->open_parser_pos_fig);
            aws->callback(makeCreateWindowCallback(create_modify_fields_window, query));
            aws->create_button("MODIFY_FIELDS_OF_LISTED", "MODIFY FIELDS\nOF LISTED", "F");
        }
    }
    if (awtqs->use_menu) {
        sprintf(buffer, "Set Protection of Fields of Listed %s", Items); query_rel_menu_entry(aws, "s_prot_of_listed", query_id, buffer, "P", "set_protection.hlp", AWM_ALL, makeCreateWindowCallback(create_set_protection_window, query));
        aws->sep______________();
        sprintf(buffer, "Mark Listed %s, don't Change Rest",   Items); query_rel_menu_entry(aws, "mark_listed",             query_id, buffer, "M", "mark.hlp", AWM_ALL, makeWindowCallback(mark_queried_cb, query, 1|8));
        sprintf(buffer, "Mark Listed %s, Unmark Rest",         Items); query_rel_menu_entry(aws, "mark_listed_unmark_rest", query_id, buffer, "L", "mark.hlp", AWM_ALL, makeWindowCallback(mark_queried_cb, query, 1  ));
        sprintf(buffer, "Unmark Listed %s, don't Change Rest", Items); query_rel_menu_entry(aws, "unmark_listed",           query_id, buffer, "U", "mark.hlp", AWM_ALL, makeWindowCallback(mark_queried_cb, query, 0|8));
        sprintf(buffer, "Unmark Listed %s, Mark Rest",         Items); query_rel_menu_entry(aws, "unmark_listed_mark_rest", query_id, buffer, "R", "mark.hlp", AWM_ALL, makeWindowCallback(mark_queried_cb, query, 0  ));
        aws->sep______________();


        sprintf(buffer, "Set Color of Listed %s", Items);    query_rel_menu_entry(aws, "set_color_of_listed", query_id, buffer, "C", "set_color_of_listed.hlp", AWM_ALL, makeCreateWindowCallback(create_colorize_queried_window, query));

        if (query->gb_ref) {
            dbq_assert(query->selector.type == QUERY_ITEM_SPECIES); // stuff below works only for species
            aws->sep______________();
            if (query->expect_hit_in_ref_list) {
                aws->insert_menu_topic("search_equal_fields_and_listed_in_I", "Search entries existing in both DBs and listed in the source DB hitlist", "S",
                                       "search_equal_fields.hlp", AWM_ALL, makeWindowCallback(perform_query_cb, query, EXT_QUERY_COMPARE_LINES));
                aws->insert_menu_topic("search_equal_words_and_listed_in_I",  "Search words existing in entries of both DBs and listed in the source DB hitlist", "w",
                                       "search_equal_fields.hlp", AWM_ALL, makeWindowCallback(perform_query_cb, query, EXT_QUERY_COMPARE_WORDS));
            }
            else {
                aws->insert_menu_topic("search_equal_field_in_both_db", "Search entries existing in both DBs", "S",
                                       "search_equal_fields.hlp", AWM_ALL, makeWindowCallback(perform_query_cb, query, EXT_QUERY_COMPARE_LINES));
                aws->insert_menu_topic("search_equal_word_in_both_db", "Search words existing in entries of both DBs", "w",
                                       "search_equal_fields.hlp", AWM_ALL, makeWindowCallback(perform_query_cb, query, EXT_QUERY_COMPARE_WORDS));
            }
        }
    }

    free(Items);

    GB_pop_transaction(gb_main);
    return query;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_nullcmp() {
    const char *whatever = "bla";
    TEST_EXPECT_EQUAL(ARB_strNULLcmp(NULL, NULL), 0);
    TEST_EXPECT_EQUAL(ARB_strNULLcmp(whatever, whatever), 0);
    TEST_EXPECT_EQUAL(ARB_strNULLcmp("a", "b"), -1);

    // document uncommon behavior: NULL is bigger than any other text!
    TEST_EXPECT_EQUAL(ARB_strNULLcmp(whatever, NULL), -1);
    TEST_EXPECT_EQUAL(ARB_strNULLcmp(NULL, whatever), 1);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
