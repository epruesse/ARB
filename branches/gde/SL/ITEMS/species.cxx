// ================================================================ //
//                                                                  //
//   File      : species.cxx                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "item_sel_list.h"

#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_awars.hxx>


static GBDATA *get_first_species_data(GBDATA *gb_main, AW_root *, QUERY_RANGE) {
    return GBT_get_species_data(gb_main);
}
static GBDATA *get_next_species_data(GBDATA *, QUERY_RANGE) {
    return 0; // there is only ONE species_data
}

static void select_species(GBDATA*,  AW_root *aw_root, const char *item_name) {
    aw_root->awar(AWAR_SPECIES_NAME)->write_string(item_name);
}

static GBDATA* get_selected_species(GBDATA *gb_main, AW_root *aw_root) {
    char   *species_name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species   = 0;
    if (species_name[0]) {
        gb_species = GBT_find_species(gb_main, species_name);
    }
    free(species_name);
    return gb_species;
}

static void add_selected_species_changed_cb(AW_root *aw_root, const RootCallback& cb) {
    aw_root->awar(AWAR_SPECIES_NAME)->add_callback(cb);
}

static char* get_species_id(GBDATA *, GBDATA *gb_species) {
    GBDATA *gb_name = GB_entry(gb_species, "name");
    if (!gb_name) return 0;     // species w/o name -> skip
    return GB_read_as_string(gb_name);
}

static GBDATA *find_species_by_id(GBDATA *gb_main, const char *id) {
    return GBT_find_species(gb_main, id); // id is 'name' field
}

static GBDATA *get_first_species(GBDATA *gb_species_data, QUERY_RANGE range) {
    GBDATA *gb_first = NULL;
    switch (range) {
        case QUERY_ALL_ITEMS:    gb_first = GBT_first_species_rel_species_data(gb_species_data); break;
        case QUERY_MARKED_ITEMS: gb_first = GBT_first_marked_species_rel_species_data(gb_species_data); break;
        case QUERY_CURRENT_ITEM: gb_first = get_selected_species(GB_get_root(gb_species_data), AW_root::SINGLETON); break;
    }
    return gb_first;
}
static GBDATA *get_next_species(GBDATA *gb_prev, QUERY_RANGE range) {
    GBDATA *gb_next = NULL;
    switch (range) {
        case QUERY_ALL_ITEMS:    gb_next = GBT_next_species(gb_prev); break;
        case QUERY_MARKED_ITEMS: gb_next = GBT_next_marked_species(gb_prev); break;
        case QUERY_CURRENT_ITEM: gb_next = NULL; break;
    }
    return gb_next;
}

static void refresh_displayed_species() {
    AW_root::SINGLETON->awar(AWAR_TREE_REFRESH)->touch();
}

static struct MutableItemSelector ITEM_species = {
    QUERY_ITEM_SPECIES,
    select_species,
    get_species_id,
    find_species_by_id,
    species_field_selection_list_update_cb,
    12,
    CHANGE_KEY_PATH,
    "species",
    "species",
    "name",
    get_first_species_data,
    get_next_species_data,
    get_first_species,
    get_next_species,
    get_selected_species,
    add_selected_species_changed_cb,
    0, 0,
    refresh_displayed_species,
};

static struct MutableItemSelector ITEM_organism = {
    QUERY_ITEM_SPECIES,
    select_species,
    get_species_id,
    find_species_by_id,
    species_field_selection_list_update_cb,
    12,
    CHANGE_KEY_PATH,
    "organism",
    "organism",
    "name",
    get_first_species_data,
    get_next_species_data,
    get_first_species,
    get_next_species,
    get_selected_species,
    add_selected_species_changed_cb,
    0, 0,
    refresh_displayed_species,
};

ItemSelector& SPECIES_get_selector() { return ITEM_species; }
ItemSelector& ORGANISM_get_selector() { return ITEM_organism; }

