// =============================================================== //
//                                                                 //
//   File      : adhashtools.cxx                                   //
//   Purpose   : convenience functions for hashes                  //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_local.h"
#include "arbdbt.h"

static void GBT_add_item_to_hash(GBDATA *gb_item, GB_HASH *item_hash) {
    GBS_write_hash(item_hash, GBT_read_name(gb_item), (long)gb_item);
}

typedef GBDATA *(*item_iterator)(GBDATA *);

static GB_HASH *create_item_hash(long size, GBDATA *gb_start, item_iterator getFirst, item_iterator getNext) {
    GB_HASH *item_hash = GBS_create_hash(size, GB_IGNORE_CASE);
    GBDATA  *gb_item;

    for (gb_item = getFirst(gb_start); gb_item; gb_item = getNext(gb_item)) {
        GBT_add_item_to_hash(gb_item, item_hash);
    }

    return item_hash;
}

GB_HASH *GBT_create_species_hash_sized(GBDATA *gb_main, long species_count) {
    return create_item_hash(species_count, gb_main, GBT_first_species, GBT_next_species);
}

GB_HASH *GBT_create_species_hash(GBDATA *gb_main) {
    return GBT_create_species_hash_sized(gb_main, GBT_get_species_count(gb_main));
}

GB_HASH *GBT_create_marked_species_hash(GBDATA *gb_main) {
    return create_item_hash(GBT_get_species_count(gb_main), gb_main, GBT_first_marked_species, GBT_next_marked_species);
}

GB_HASH *GBT_create_SAI_hash(GBDATA *gb_main) {
    return create_item_hash(GBT_get_SAI_count(gb_main), gb_main, GBT_first_SAI, GBT_next_SAI);
}

GB_HASH *GBT_create_organism_hash(GBDATA *gb_main) {
    return create_item_hash(GEN_get_organism_count(gb_main), gb_main, GEN_first_organism, GEN_next_organism);
}

