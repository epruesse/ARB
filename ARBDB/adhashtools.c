/* =============================================================== */
/*                                                                 */
/*   File      : adhashtools.c                                     */
/*   Purpose   : convenience functions for hashes                  */
/*   Time-stamp: <Fri Jul/06/2007 12:08 MET Coder@ReallySoft.de>   */
/*                                                                 */
/*   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      */
/*   Institute of Microbiology (Technical University Munich)       */
/*   http://www.arb-home.de/                                       */
/*                                                                 */
/* =============================================================== */

#include <stdio.h>
#include "adlocal.h"
#include "arbdbt.h"

static long items2hashsize(long estimated_entries) {
    return 2*estimated_entries; // hash size = 2 * number of species
}

long GBT_get_species_hash_size(GBDATA *gb_main) {
    return items2hashsize(GBT_get_species_count(gb_main));
}

void GBT_add_item_to_hash(GBDATA *gb_item, GB_HASH *item_hash) {
    GBDATA     *gb_name = GB_find(gb_item, "name", 0, down_level);
    const char *name;

    gb_assert(gb_name);
    name = GB_read_char_pntr(gb_name);
    GBS_write_hash(item_hash, name, (long)gb_item);
}

static GB_HASH *create_species_hash(GBDATA *gb_main, long size) {
    GB_HASH *species_hash = GBS_create_hash(size, 1);
    GBDATA  *gb_species;

    for (gb_species = GBT_first_species(gb_main);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        GBT_add_item_to_hash(gb_species, species_hash);
    }
    
    return species_hash;
}

GB_HASH *GBT_create_species_hash(GBDATA *gb_main) { /* = GBT_generate_species_hash(gb_main, 1) */
    long size = GBT_get_species_hash_size(gb_main);
    return create_species_hash(gb_main, size);
}

GB_HASH *GBT_create_species_hash_with_size(GBDATA *gb_main, long estimated_species) {
    long hash_size = items2hashsize(estimated_species);
    return create_species_hash(gb_main, hash_size);
}

#if defined(DEVEL_RALF)
#warning parameter ignore_case is misleading. now all calls use 1. remove if no problems arise. 
#endif /* DEVEL_RALF */

GB_HASH *GBT_generate_species_hash(GBDATA *gb_main,int ignore_case)
{
    GB_HASH *hash = GBS_create_hash(GBT_get_species_hash_size(gb_main),ignore_case);
    GBDATA  *gb_species;
    
    for (gb_species = GBT_first_species(gb_main);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        GBT_add_item_to_hash(gb_species, hash);
    }

    return hash;
}

GB_HASH *GBT_generate_species_hash_sized(GBDATA *gb_main, long species_count)
{
    GB_HASH *hash = GBS_create_hash(items2hashsize(species_count), 1);
    GBDATA  *gb_species;
    
    for (gb_species = GBT_first_species(gb_main);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        GBT_add_item_to_hash(gb_species, hash);
    }

    return hash;
}

GB_HASH *GBT_generate_marked_species_hash(GBDATA *gb_main)
{
    GB_HASH *hash = GBS_create_hash(GBT_get_species_hash_size(gb_main),0);
    GBDATA  *gb_species;
    
    for (gb_species = GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        GBT_add_item_to_hash(gb_species, hash);
    }
    
    return hash;
}

GB_HASH *GBT_generate_SAI_hash(GBDATA *gb_main)
{
    GB_HASH *hash = GBS_create_hash(1000, 0);
    GBDATA  *gb_ext;

    for (gb_ext = GBT_first_SAI(gb_main);
         gb_ext;
         gb_ext = GBT_next_SAI(gb_ext))
    {
        GBDATA *gb_name = GB_find(gb_ext, "name", 0, down_level);
        if (gb_name) {
            GBS_write_hash(hash, GB_read_char_pntr(gb_name), (long)gb_ext);
        }
    }
    return hash;
}

