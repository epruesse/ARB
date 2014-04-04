// ============================================================ //
//                                                              //
//   File      : db_query_local.h                               //
//   Purpose   : internal query defs                            //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef DB_QUERY_LOCAL_H
#define DB_QUERY_LOCAL_H

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

#define dbq_assert(cond) arb_assert(cond)


namespace QUERY {

    enum QUERY_MODES {
        QUERY_GENERATE,
        QUERY_ENLARGE,
        QUERY_REDUCE
    };

    enum QUERY_TYPES {
        QUERY_MARKED,
        QUERY_MATCH,
        QUERY_DONT_MATCH
    };

#define QUERY_SEARCHES 3 // no of search-lines in search tool

#define QUERY_SORT_CRITERIA_BITS 6              // number of "real" sort criteria
#define QUERY_SORT_CRITERIA_MASK ((1<<QUERY_SORT_CRITERIA_BITS)-1)

    enum QUERY_RESULT_ORDER {
        QUERY_SORT_NONE = 0,

        // "real" criteria:
        QUERY_SORT_BY_1STFIELD_CONTENT = 1,         // by content of first selected search field
        QUERY_SORT_BY_ID               = 2,         // by item id (not by parent)
        QUERY_SORT_BY_NESTED_PID       = 4,         // by nested parent id
        QUERY_SORT_BY_MARKED           = 8,         // marked items first
        QUERY_SORT_BY_HIT_DESCRIPTION  = 16,        // by hit description
        QUERY_SORT_REVERSE             = 32,        // revert following (may occur multiple times)

    };

    class DbQuery {
        AwarName awar_tree_name;

    public:
        AW_window         *aws;
        GBDATA            *gb_main;                           // the main database (in merge tool: source db in left query; dest db in right query)
        GBDATA            *gb_ref;                            // second reference database (only used by merge tool; dest db in left query; source db in right query)
        bool               expect_hit_in_ref_list;            // merge-tool: when searching dups in fields: match only if hit exists in other DBs hitlist (true for target-DB-query)
        AwarName           species_name;
        AwarName           awar_keys[QUERY_SEARCHES];
        AwarName           awar_setkey;
        AwarName           awar_setprotection;
        AwarName           awar_setvalue;
        AwarName           awar_parskey;
        AwarName           awar_parsvalue;
        AwarName           awar_parspredefined;
        AwarName           awar_queries[QUERY_SEARCHES];
        AwarName           awar_not[QUERY_SEARCHES];          // not flags for queries
        AwarName           awar_operator[QUERY_SEARCHES];     // not flags for queries
        AwarName           awar_ere;
        AwarName           awar_where;
        AwarName           awar_by;
        AwarName           awar_use_tag;
        AwarName           awar_double_pars;
        AwarName           awar_deftag;
        AwarName           awar_tag;
        AwarName           awar_count;
        AwarName           awar_sort;
        unsigned long      sort_mask;                         // contains several cascading sort criteria (QUERY_SORT_CRITERIA_BITS each)
        AW_selection_list *hitlist;
        ItemSelector&      selector;
        int                select_bit;                        // one of 1 2 4 8 .. 128 (one for each query box)
        GB_HASH           *hit_description;                   // key = char* (hit item name), value = char* (description of hit - allocated!)

        DbQuery(ItemSelector& selector_)
            : selector(selector_)
        {
            dbq_assert(&selector);
        }

        bool is_queried(GBDATA *gb_item) const {
            return GB_user_flag(gb_item, select_bit);
        }

        const char *get_tree_name() const;
        void set_tree_awar_name(const char *tree_awar_name) { // @@@ intermediate - should be set by ctor
            awar_tree_name = tree_awar_name;
        }
    };

};
#else
#error db_query_local.h included twice
#endif // DB_QUERY_LOCAL_H


