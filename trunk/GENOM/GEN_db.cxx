#include <malloc.h>

#include <arbdb.h>
#include <arbdbt.h>

#ifndef GEN_LOCAL_HXX
#include "GEN_local.hxx"
#endif

//  --------------
//      genes:
//  --------------

GBDATA* GEN_find_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name) {
    GBDATA *gb_name = GB_find(gb_gene_data, "name", name, down_2_level);

    if (gb_name) return GB_get_father(gb_name); // found existing gene
    return 0;
}

GBDATA* GEN_find_gene(GBDATA *gb_species, const char *name) {
    // find existing gene
    return GEN_find_gene_rel_gene_data(GEN_get_gene_data(gb_species), name);
}

GBDATA* GEN_create_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name) {
    /* Search for a gene, when gene does not exist create it */
    GBDATA *gb_name = GB_find(gb_gene_data, "name", name, down_2_level);

    if (gb_name) return GB_get_father(gb_name); // found existing gene

    GBDATA *gb_gene = GB_create_container(gb_gene_data, "gene");
    gb_name = GB_create(gb_gene, "name", GB_STRING);
    GB_write_string(gb_name, name);

    return gb_gene;
}

GBDATA* GEN_create_gene(GBDATA *gb_species, const char *name) {
    return GEN_create_gene_rel_gene_data(GEN_get_gene_data(gb_species), name);
}

GBDATA* GEN_first_gene(GBDATA *gb_species) {
    GBDATA *gb_gene_data = GEN_get_gene_data(gb_species);
    return GB_find(gb_gene_data, "gene", 0, down_level);
}

GBDATA* GEN_first_gene_rel_gene_data(GBDATA *gb_gene_data) {
    return GB_find(gb_gene_data, "gene", 0, down_level);
}

GBDATA* GEN_next_gene(GBDATA *gb_gene) {
    return GB_find(gb_gene, "gene", 0, this_level|search_next);
}

//  -----------------------------------------
//      test if species is pseudo-species
//  -----------------------------------------

const char *GEN_origin_organism(GBDATA *gb_pseudo) {
    GBDATA *gb_origin = GB_find(gb_pseudo, "ARB_origin_species", 0, down_level);
    return gb_origin ? GB_read_char_pntr(gb_origin) : 0;
}
const char *GEN_origin_gene(GBDATA *gb_pseudo) {
    GBDATA *gb_origin = GB_find(gb_pseudo, "ARB_origin_gene", 0, down_level);
    return gb_origin ? GB_read_char_pntr(gb_origin) : 0;
}

bool GEN_is_pseudo_gene_species(GBDATA *gb_species) {
    return GEN_origin_organism(gb_species) != 0;
}

//  -----------------------------------
//      test if species is organism
//  -----------------------------------

bool GEN_is_organism(GBDATA *gb_species) {
    return
        !GEN_is_pseudo_gene_species(gb_species) &&
        GEN_get_gene_data(gb_species) != 0; // has gene_data
}
//  ------------------------------------------------
//      find organism or gene for pseudo-species
//  ------------------------------------------------

GB_ERROR GEN_organism_not_found(GBDATA *gb_pseudo) {
    gb_assert(GEN_is_pseudo_gene_species(gb_pseudo));
    gb_assert(GEN_find_origin_organism(gb_pseudo) == 0);

    GBDATA *gb_name = GB_find(gb_pseudo, "name", 0, down_level);

    GB_ERROR error = GB_export_error("The gene-species '%s' refers to an unknown organism (%s)\n"
                                     "This occurs if you rename or delete the organism or change the entry 'ARB_origin_species'\n"
                                     "and might cause serious problems.",
                                     GB_read_char_pntr(gb_name),
                                     GEN_origin_organism(gb_pseudo));

    return error;
}

GBDATA *GEN_find_origin_organism(GBDATA *gb_pseudo) {
    const char *origin_species_name = GEN_origin_organism(gb_pseudo);
    return origin_species_name
        ? GBT_find_species_rel_species_data(GB_get_father(gb_pseudo), origin_species_name)
        : 0;
}

GBDATA *GEN_find_origin_gene(GBDATA *gb_pseudo) {
    const char *origin_gene_name = GEN_origin_gene(gb_pseudo);
    if (!origin_gene_name) return 0;

    GBDATA *gb_organism = GEN_find_origin_organism(gb_pseudo);
    gb_assert(gb_organism);
    return GEN_find_gene(gb_organism, origin_gene_name);
}

//  --------------------------------
//      find pseudo-species
//  --------------------------------

GBDATA* GEN_first_pseudo_species(GBDATA *gb_main) {
    GBDATA *gb_species = GBT_first_species(gb_main);

    if (!gb_species || GEN_is_pseudo_gene_species(gb_species)) return gb_species;
    return GEN_next_pseudo_species(gb_species);
}

GBDATA* GEN_first_pseudo_species_rel_species_data(GBDATA *gb_species_data) {
    GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);

    if (!gb_species || GEN_is_pseudo_gene_species(gb_species)) return gb_species;
    return GEN_next_pseudo_species(gb_species);
}

GBDATA* GEN_next_pseudo_species(GBDATA *gb_species) {
    if (gb_species) {
        while (1) {
            gb_species = GBT_next_species(gb_species);
            if (!gb_species || GEN_is_pseudo_gene_species(gb_species)) break;
        }
    }
    return 0;
}

//  ----------------------------------
//      find marked pseudo-species
//  ----------------------------------

GBDATA *GEN_first_marked_pseudo_species(GBDATA *gb_main) {
    GBDATA *gb_species = GBT_first_marked_species(gb_main);

    if (!gb_species || GEN_is_pseudo_gene_species(gb_species)) return gb_species;
    return GEN_next_marked_pseudo_species(gb_species);
}

GBDATA* GEN_next_marked_pseudo_species(GBDATA *gb_species) {
    if (gb_species) {
        while (1) {
            gb_species = GBT_next_marked_species(gb_species);
            if (!gb_species || GEN_is_pseudo_gene_species(gb_species)) break;
        }
    }
    return 0;
}

//  ----------------------
//      find organisms
//  ----------------------

GBDATA *GEN_first_organism(GBDATA *gb_main) {
    GBDATA *gb_organism = GBT_first_species(gb_main);

    if (!gb_organism || GEN_is_organism(gb_organism)) return gb_organism;
    return GEN_next_organism(gb_organism);
}
GBDATA *GEN_next_organism(GBDATA *gb_organism) {
    if (gb_organism) {
        while (1) {
            gb_organism = GBT_next_species(gb_organism);
            if (!gb_organism || GEN_is_organism(gb_organism)) break;
        }
    }
    return 0;

}
//  -----------------------------
//      find marked organisms
//  -----------------------------

GBDATA *GEN_first_marked_organism(GBDATA *gb_main) {
    GBDATA *gb_organism = GBT_first_marked_species(gb_main);

    if (!gb_organism || GEN_is_organism(gb_organism)) return gb_organism;
    return GEN_next_marked_organism(gb_organism);
}
GBDATA *GEN_next_marked_organism(GBDATA *gb_organism) {
    if (gb_organism) {
        while (1) {
            gb_organism = GBT_next_marked_species(gb_organism);
            if (!gb_organism || GEN_is_organism(gb_organism)) break;
        }
    }
    return 0;
}
