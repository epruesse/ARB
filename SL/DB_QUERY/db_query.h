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


// ------------------
//      query box

typedef AW_window *(*awt_create_viewer_window_cb)(AW_root *aw_root, AW_CL cl_gb_main);

struct DbQuery;

class awt_query_struct {
public:
    awt_query_struct();

    GBDATA *gb_main;                                // the main database (in merge tool: source db in left query; dest db in right query)
    GBDATA *gb_ref;                                 // second reference database (only used by merge tool; dest db in left query; source db in right query)
    bool    expect_hit_in_ref_list;                 // merge-tool: when searching dups in fields: match only if hit exists in other DBs hitlist (true for DBII-query)
    AWAR    species_name;                           // AWAR containing current species name
    AWAR    tree_name;                              // AWAR containing current tree name

    const struct ad_item_selector *selector;               // which kind of item do we handle?

    int select_bit;                                 // one of 1 2 4 8 .. 128 (one for each query box)
    int use_menu;                                   // put additional commands in menu

    const char *ere_pos_fig;                        // rebuild enlarge reduce
    const char *where_pos_fig;                      // current, marked or all species (used for sub-items of species)
    const char *by_pos_fig;                         // fit query don't fit, marked

    const char *qbox_pos_fig;                       // key box for queries
    const char *rescan_pos_fig;                     // rescan label
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

    awt_create_viewer_window_cb create_view_window;

    const char *info_box_pos_fig;

    bool is_queried(GBDATA *gb_item) const;
};

void     awt_copy_selection_list_2_queried_species(DbQuery *cbs, AW_selection_list *id, const char *hit_description);
DbQuery *awt_create_query_box(AW_window *aws, awt_query_struct *awtqs, const char *query_id); // create the query box
void     awt_search_equal_entries(AW_window *dummy, DbQuery *cbs, bool tokenize);
long     awt_count_queried_items(DbQuery *cbs, AWT_QUERY_RANGE range);
void     awt_unquery_all(void *dummy, DbQuery *cbs);

const struct ad_item_selector *get_queried_itemtype(DbQuery *query);

inline bool IS_QUERIED(GBDATA *gb_item, const awt_query_struct *aqs) { return aqs->is_queried(gb_item); }
bool IS_QUERIED(GBDATA *gb_item, const DbQuery *dq);

long awt_query_update_list(void *dummy, DbQuery *cbs);

AW_window *awt_create_item_colorizer(AW_root *aw_root, GBDATA *gb_main, const ad_item_selector *sel);

GBDATA *query_get_gb_main(DbQuery *cbs);

#else
#error db_query.h included twice
#endif // DB_QUERY_H
