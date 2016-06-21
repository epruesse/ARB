// ============================================================ //
//                                                              //
//   File      : items.h                                        //
//   Purpose   :                                                //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef ITEMS_H
#define ITEMS_H

#ifndef CB_H
#include <cb.h>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif


#define it_assert(cond) arb_assert(cond)

enum QUERY_ITEM_TYPE {
    QUERY_ITEM_SPECIES,
    QUERY_ITEM_GENES,
    QUERY_ITEM_EXPERIMENTS,

    QUERY_ITEM_TYPES // how many different types do we have
};

enum QUERY_RANGE {
    QUERY_CURRENT_ITEM,
    QUERY_MARKED_ITEMS,
    QUERY_ALL_ITEMS
};

struct                            MutableItemSelector;
typedef const MutableItemSelector ItemSelector;

struct MutableItemSelector { // @@@ remove AW_root arguments!
    QUERY_ITEM_TYPE type;

    // if user selects an item in the result list,
    // this callback sets the appropriate AWARs
    // - for species: AWAR_SPECIES_NAME is changed (item_name = 'species_name')
    // - for genes: AWAR_GENE_NAME and AWAR_SPECIES_NAME are changed (item_name = 'species_name/gene_name')

    void (*update_item_awars)(GBDATA* gb_main, AW_root *aw_root, const char *item_name);
    char *(*generate_item_id)(GBDATA *gb_main, GBDATA *gb_item); // @@@ remove parameter 'gb_main'
    GBDATA *(*find_item_by_id)(GBDATA *gb_main, const char *id);
    void (*selection_list_rescan_cb)(AW_window*, GBDATA *gb_main);

    int item_name_length; // -1 means "unknown" (might be long)

    const char *change_key_path;
    const char *item_name;                          // "species" or "gene" or "experiment" or "organism"
    const char *items_name;                         // "species" or "genes" or "experiments" or "organisms"
    const char *id_field;                           // e.g. "name" for species, genes

    GBDATA *(*get_first_item_container)(GBDATA *, AW_root *, QUERY_RANGE); // AW_root may be NULL for QUERY_ALL_ITEMS and QUERY_MARKED_ITEMS
    GBDATA *(*get_next_item_container)(GBDATA *, QUERY_RANGE);             // use same QUERY_RANGE as in get_first_item_container()

    GBDATA *(*get_first_item)(GBDATA *, QUERY_RANGE);
    GBDATA *(*get_next_item)(GBDATA *, QUERY_RANGE);

    GBDATA *(*get_selected_item)(GBDATA *gb_main, AW_root *aw_root); // searches the currently selected item
    void (*add_selection_changed_cb)(AW_root *aw_root, const RootCallback& cb); // gets called when selected item changes

    ItemSelector *parent_selector;              // selector of parent item (or NULL if item has no parents)
    GBDATA *(*get_parent)(GBDATA *gb_item);     // if 'parent_selector' is defined, this function returns the parent of the item

    void (*trigger_display_refresh)(); // shall be called when displays shall be refreshed (e.g. tree-display for species)
};

struct MutableBoundItemSel {
    GBDATA        *gb_main;
    ItemSelector&  selector;

    MutableBoundItemSel(GBDATA *gb_main_, ItemSelector& selector_)
        : gb_main(gb_main_),
          selector(selector_)
    {
        it_assert(gb_main);
        it_assert(&selector);
    }

    GBDATA *get_any_item() const;
};

typedef const MutableBoundItemSel BoundItemSel;

void init_itemType_specific_window(AW_root *aw_root, class AW_window_simple *aws, const ItemSelector& itemType, const char *id, const char *title_format, bool plural = false);

ItemSelector& SPECIES_get_selector();
ItemSelector& ORGANISM_get_selector();

#else
#error items.h included twice
#endif // ITEMS_H
