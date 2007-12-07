/* =============================================================== */
/*                                                                 */
/*   File      : adhashtools.c                                     */
/*   Purpose   : convenience functions for hashes                  */
/*   Time-stamp: <Mon Jul/09/2007 14:05 MET Coder@ReallySoft.de>   */
/*                                                                 */
/*   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      */
/*   Institute of Microbiology (Technical University Munich)       */
/*   http://www.arb-home.de/                                       */
/*                                                                 */
/* =============================================================== */

#include <stdio.h>
#include "adlocal.h"
#include "arbdbt.h"

#define ITEMS2HASHSIZE(entries) (2*(entries)) /* hash size = 2 * number of species */

long GBT_get_species_hash_size(GBDATA *gb_main) {
    return ITEMS2HASHSIZE(GBT_get_species_count(gb_main));
}

void GBT_add_item_to_hash(GBDATA *gb_item, GB_HASH *item_hash) {
    GBDATA     *gb_name = GB_find(gb_item, "name", 0, down_level);
    const char *name;

    gb_assert(gb_name);
    name = GB_read_char_pntr(gb_name);
    GBS_write_hash(item_hash, name, (long)gb_item);
}

typedef GBDATA *(*item_iterator)(GBDATA *);

static GB_HASH *create_item_hash(long size, GBDATA *gb_start, item_iterator getFirst, item_iterator getNext) {
    GB_HASH *item_hash = GBS_create_hash(size, 1);
    GBDATA  *gb_item;

    for (gb_item = getFirst(gb_start); gb_item; gb_item = getNext(gb_item)) {
        GBT_add_item_to_hash(gb_item, item_hash);
    }

    return item_hash;
}

GB_HASH *GBT_create_species_hash(GBDATA *gb_main)
{
    return create_item_hash(GBT_get_species_hash_size(gb_main),
                            gb_main, GBT_first_species, GBT_next_species);
}

GB_HASH *GBT_create_species_hash_sized(GBDATA *gb_main, long species_count)
{
    return create_item_hash(ITEMS2HASHSIZE(species_count),
                            gb_main, GBT_first_species, GBT_next_species);
}

GB_HASH *GBT_create_marked_species_hash(GBDATA *gb_main)
{
    return create_item_hash(GBT_get_species_hash_size(gb_main),
                            gb_main, GBT_first_marked_species, GBT_next_marked_species);
}

GB_HASH *GBT_create_SAI_hash(GBDATA *gb_main)
{
    return create_item_hash(ITEMS2HASHSIZE(GBT_get_SAI_count(gb_main)),
                            gb_main, GBT_first_SAI, GBT_next_SAI);
}

GB_HASH *GBT_create_organism_hash(GBDATA *gb_main) {
    return create_item_hash(ITEMS2HASHSIZE(GEN_get_organism_count(gb_main)),
                            gb_main, GEN_first_organism, GEN_next_organism);
}

