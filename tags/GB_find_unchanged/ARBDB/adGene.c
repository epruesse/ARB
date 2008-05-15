/*  ====================================================================  */
/*                                                                        */
/*    File      : adGene.c                                                */
/*    Purpose   : Basic gene access functions                             */
/*    Time-stamp: <Tue Oct/30/2007 11:43 MET Coder@ReallySoft.de>         */
/*                                                                        */
/*                                                                        */
/*  Coded by Ralf Westram (coder@reallysoft.de) in July 2002              */
/*  Copyright Department of Microbiology (Technical University Munich)    */
/*                                                                        */
/*  Visit our web site at: http://www.arb-home.de/                        */
/*                                                                        */
/*                                                                        */
/*  ====================================================================  */

#include <stdio.h>
#include <string.h>

#include "adGene.h"
#include "arbdbt.h"

//  -----------------------------------------------------------------
//      bool GEN_is_genome_db(GBDATA *gb_main, int default_value)
//  -----------------------------------------------------------------
// default_value == 0 -> default to normal database
//               == 1 -> default to GENOM database
//               == -1 -> assume that type is already defined

GB_BOOL GEN_is_genome_db(GBDATA *gb_main, int default_value) {
    GBDATA *gb_genom_db = GB_find(gb_main, GENOM_DB_TYPE, 0, down_level);

    if (!gb_genom_db) {         // no DB-type entry -> create one with default
        if (default_value == -1) {
            GB_CORE;
        }

        gb_genom_db = GB_create(gb_main, GENOM_DB_TYPE, GB_INT);
        GB_write_int(gb_genom_db, default_value);
    }

    return GB_read_int(gb_genom_db) != 0;
}

//  --------------
//      genes:
//  --------------

GBDATA* GEN_findOrCreate_gene_data(GBDATA *gb_species) {
    GBDATA *gb_gene_data = GB_search(gb_species, "gene_data", GB_CREATE_CONTAINER);
    gb_assert(gb_gene_data);
    return gb_gene_data;
}

GBDATA* GEN_find_gene_data(GBDATA *gb_species) {
    return GB_search(gb_species, "gene_data", GB_FIND);
}

GBDATA* GEN_expect_gene_data(GBDATA *gb_species) {
    GBDATA *gb_gene_data = GB_search(gb_species, "gene_data", GB_FIND);
    gb_assert(gb_gene_data);
    return gb_gene_data;
}

GBDATA* GEN_find_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name) {
    GBDATA *gb_name = GB_find(gb_gene_data, "name", name, down_2_level);

    if (gb_name) return GB_get_father(gb_name); // found existing gene
    return 0;
}

GBDATA* GEN_find_gene(GBDATA *gb_species, const char *name) {
    // find existing gene. returns 0 if it does not exist.
    GBDATA *gb_gene_data = GEN_find_gene_data(gb_species);
    return gb_gene_data ? GEN_find_gene_rel_gene_data(gb_gene_data, name) : 0;
}

GBDATA* GEN_create_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name) {
    GBDATA *gb_gene = 0;

    /* Search for a gene, when gene does not exist create it */
    if (!name || !name[0]) {
        GB_export_error("Missing gene name");
    }
    else {
        GBDATA *gb_name = GB_find(gb_gene_data, "name", name, down_2_level);

        if (gb_name) return GB_get_father(gb_name); // found existing gene

        gb_gene = GB_create_container(gb_gene_data, "gene");
        gb_name = GB_create(gb_gene, "name", GB_STRING);
        GB_write_string(gb_name, name);
    }
    return gb_gene;
}

GBDATA* GEN_create_gene(GBDATA *gb_species, const char *name) {
    return GEN_create_gene_rel_gene_data(GEN_findOrCreate_gene_data(gb_species), name);
}

GBDATA* GEN_first_gene(GBDATA *gb_species) {
    return GB_find(GEN_expect_gene_data(gb_species), "gene", 0, down_level);
}

GBDATA* GEN_first_gene_rel_gene_data(GBDATA *gb_gene_data) {
    return GB_find(gb_gene_data, "gene", 0, down_level);
}

GBDATA* GEN_next_gene(GBDATA *gb_gene) {
    return GB_find(gb_gene, "gene", 0, this_level|search_next);
}

GBDATA *GEN_first_marked_gene(GBDATA *gb_species) {
    return GB_first_marked(GEN_expect_gene_data(gb_species), "gene");
}
GBDATA *GEN_next_marked_gene(GBDATA *gb_gene) {
    return GB_next_marked(gb_gene,"gene");
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

GB_BOOL GEN_is_pseudo_gene_species(GBDATA *gb_species) {
    return GEN_origin_organism(gb_species) != 0;
}

//  ------------------------------------------------
//      find organism or gene for pseudo-species
//  ------------------------------------------------

GB_ERROR GEN_organism_not_found(GBDATA *gb_pseudo) {
    GBDATA   *gb_name = 0;
    GB_ERROR  error   = 0;

    gb_assert(GEN_is_pseudo_gene_species(gb_pseudo));
    gb_assert(GEN_find_origin_organism(gb_pseudo, 0) == 0);

    gb_name = GB_find(gb_pseudo, "name", 0, down_level);

    error = GB_export_error("The gene-species '%s' refers to an unknown organism (%s)\n"
                            "This occurs if you rename or delete the organism or change the entry\n"
                            "'ARB_origin_species' and will most likely cause serious problems.",
                            GB_read_char_pntr(gb_name),
                            GEN_origin_organism(gb_pseudo));

    return error;
}

// @@@ FIXME: missing: GEN_gene_not_found (like GEN_organism_not_found)

/* ---------------------------------- */
/*      searching pseudo species      */
/* ---------------------------------- */

static const char *pseudo_species_hash_key(const char *organism_name, const char *gene_name) {
    return GBS_global_string("%s*%s", organism_name, gene_name);
}

GBDATA *GEN_read_pseudo_species_from_hash(GB_HASH *pseudo_hash, const char *organism_name, const char *gene_name) {
    return (GBDATA*)GBS_read_hash(pseudo_hash, pseudo_species_hash_key(organism_name, gene_name));
}

void GEN_add_pseudo_species_to_hash(GBDATA *gb_pseudo, GB_HASH *pseudo_hash) {
    const char *organism_name = GEN_origin_organism(gb_pseudo);
    const char *gene_name     = GEN_origin_gene(gb_pseudo);

    gb_assert(organism_name);
    gb_assert(gene_name);

    GBS_write_hash(pseudo_hash, pseudo_species_hash_key(organism_name, gene_name), (long)gb_pseudo);
}

GB_HASH *GEN_create_pseudo_species_hash(GBDATA *gb_main, int additionalSize) {
    GB_HASH *pseudo_hash = GBS_create_hash(GBT_get_species_hash_size(gb_main)+2*additionalSize, 1);
    GBDATA  *gb_pseudo;

    for (gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        GEN_add_pseudo_species_to_hash(gb_pseudo, pseudo_hash);
    }

    return pseudo_hash;
}

GBDATA *GEN_find_pseudo_species(GBDATA *gb_main, const char *organism_name, const char *gene_name, GB_HASH *pseudo_hash) {
    // parameter pseudo_hash :
    // 0 -> use slow direct search [if you only search one]
    // otherwise it shall be a hash generated by GEN_create_pseudo_species_hash() [if you search several times]
    // Note : use GEN_add_pseudo_species_to_hash to keep hash up-to-date
    GBDATA *gb_pseudo;

    if (pseudo_hash) {
        gb_pseudo = GEN_read_pseudo_species_from_hash(pseudo_hash, organism_name, gene_name);
    }
    else {
        for (gb_pseudo = GEN_first_pseudo_species(gb_main);
             gb_pseudo;
             gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
        {
            const char *origin_gene_name = GEN_origin_gene(gb_pseudo);
            if (strcmp(gene_name, origin_gene_name) == 0) {
                const char *origin_species_name = GEN_origin_organism(gb_pseudo);
                if (strcmp(organism_name, origin_species_name) == 0) {
                    break; // found pseudo species
                }
            }
        }
    }
    return gb_pseudo;
}

/* --------------------------- */
/*      searching origins      */
/* --------------------------- */

GBDATA *GEN_find_origin_organism(GBDATA *gb_pseudo, GB_HASH *organism_hash) {
    // parameter organism_hash:
    // 0 -> use slow direct search [if you only search one or two]
    // otherwise it shall be a hash generated by GBT_create_organism_hash() [if you search several times]
    // Note : use GBT_add_item_to_hash() to keep hash up-to-date
    
    const char *origin_species_name;
    GBDATA     *gb_organism = 0;
    gb_assert(GEN_is_pseudo_gene_species(gb_pseudo));

    origin_species_name = GEN_origin_organism(gb_pseudo);
    if (origin_species_name) {
        if (organism_hash) {
            gb_organism = (GBDATA*)GBS_read_hash(organism_hash, origin_species_name);
        }
        else {
            gb_organism = GBT_find_species_rel_species_data(GB_get_father(gb_pseudo), origin_species_name);
        }
    }

    return gb_organism;
}

GBDATA *GEN_find_origin_gene(GBDATA *gb_pseudo, GB_HASH *organism_hash) {
    const char *origin_gene_name;

    gb_assert(GEN_is_pseudo_gene_species(gb_pseudo));

    origin_gene_name = GEN_origin_gene(gb_pseudo);
    if (origin_gene_name) {
        GBDATA *gb_organism = GEN_find_origin_organism(gb_pseudo, organism_hash);
        gb_assert(gb_organism);

        return GEN_find_gene(gb_organism, origin_gene_name);
    }
    return 0;
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
    return gb_species;
}

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
    return gb_species;
}



/* ------------------------ */
/*        organisms         */
/* ------------------------ */

GB_BOOL GEN_is_organism(GBDATA *gb_species) {
    gb_assert(GEN_is_genome_db(GB_get_root(gb_species), -1)); /* assert this is a genome db */
    /* otherwise it is an error to use GEN_is_organism (or its callers)!!!! */
    
    return GB_find(gb_species, GENOM_ALIGNMENT, 0, down_level) != 0;
}

GBDATA *GEN_find_organism(GBDATA *gb_main, const char *name) {
    GBDATA *gb_orga = GBT_find_species(gb_main, name);
    if (gb_orga) {
        if (!GEN_is_organism(gb_orga)) {
            fprintf(stderr, "ARBDB-warning: found unspecific species named '%s', but expected an 'organism' with that name\n", name);
            gb_orga = 0;
        }
    }
    return gb_orga;
}

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
    return gb_organism;

}

long GEN_get_organism_count(GBDATA *gb_main) {
    long    count       = 0;
    GBDATA *gb_organism = GEN_first_organism(gb_main);
    while (gb_organism) {
        count++;
        gb_organism = GEN_next_organism(gb_organism);
    }
    return count;
}


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
    return gb_organism;
}

