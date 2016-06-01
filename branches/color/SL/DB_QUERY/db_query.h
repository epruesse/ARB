// ============================================================ //
//                                                              //
//   File      : db_query.h                                     //
//   Purpose   : Database queries                               //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef DB_QUERY_H
#define DB_QUERY_H

#ifndef ITEMS_H
#include <items.h>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif


class AW_selection_list;

namespace QUERY {

    typedef const char *AwarName;
    typedef void (*popup_info_window_cb)(AW_root *aw_root, GBDATA *gb_main);

    class DbQuery;

    class query_spec {
        ItemSelector& selector;                         // which kind of item do we handle?

    public:
        query_spec(ItemSelector& selector_);

        ItemSelector& get_queried_itemtype() const { return selector; }

        GBDATA   *gb_main;                                  // the main database (in merge tool: source db in left query; dest db in right query)
        GBDATA   *gb_ref;                                   // second reference database (only used by merge tool; dest db in left query; source db in right query)
        bool      expect_hit_in_ref_list;                   // merge-tool: when searching dups in fields: match only if hit exists in other DBs hitlist (true for target-DB-query)
        AwarName  species_name;                             // awar containing current species name
        AwarName  tree_name;                                // awar containing current tree name

        int select_bit;                                 // one of 1 2 4 8 .. 128 (one for each query box)
        int use_menu;                                   // put additional commands in menu

        const char *ere_pos_fig;                        // rebuild enlarge reduce
        const char *where_pos_fig;                      // current, marked or all species (used for sub-items of species)
        const char *by_pos_fig;                         // fit query don't fit, marked

        const char *qbox_pos_fig;                       // key box for queries
        const char *key_pos_fig;                        // the key
        const char *query_pos_fig;                      // the query


        const char *result_pos_fig;                     // the result box
        const char *count_pos_fig;

        const char *do_query_pos_fig;
        const char *config_pos_fig;
        const char *do_mark_pos_fig;
        const char *do_unmark_pos_fig;
        const char *do_delete_pos_fig;
        const char *do_set_pos_fig;                     // multi set a key
        const char *open_parser_pos_fig;
        const char *do_refresh_pos_fig;

        popup_info_window_cb popup_info_window;

        const char *info_box_pos_fig;

        bool is_queried(GBDATA *gb_item) const;
    };

    void     copy_selection_list_2_query_box(DbQuery *query, AW_selection_list *srclist, const char *hit_description);
    DbQuery *create_query_box(AW_window *aws, query_spec *awtqs, const char *query_id); // create the query box
    void     search_duplicated_field_content(AW_window *dummy, DbQuery *query, bool tokenize);
    long     count_queried_items(DbQuery *query, QUERY_RANGE range);
    void     unquery_all(void *dummy, DbQuery *query);

    ItemSelector& get_queried_itemtype(DbQuery *query);

    inline bool IS_QUERIED(GBDATA *gb_item, const query_spec *aqs) { return aqs->is_queried(gb_item); }
    bool IS_QUERIED(GBDATA *gb_item, const DbQuery *query);

    void DbQuery_update_list(DbQuery *query);

    AW_window *create_colorize_items_window(AW_root *aw_root, GBDATA *gb_main, ItemSelector& sel);

    GBDATA *query_get_gb_main(DbQuery *query);
    
};

#else
#error db_query.h included twice
#endif // DB_QUERY_H
