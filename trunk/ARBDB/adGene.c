/*  ====================================================================  */
/*                                                                        */
/*    File      : adGene.c                                                */
/*    Purpose   : Basic gene access functions                             */
/*    Time-stamp: <Wed Dec/10/2003 14:29 MET Coder@ReallySoft.de>         */
/*                                                                        */
/*                                                                        */
/*  Coded by Ralf Westram (coder@reallysoft.de) in July 2002              */
/*  Copyright Department of Microbiology (Technical University Munich)    */
/*                                                                        */
/*  Visit our web site at: http://www.arb-home.de/                        */
/*                                                                        */
/*                                                                        */
/*  ====================================================================  */

#include "adGene.h"
#include "arbdbt.h"


#ifdef DEVEL_IDP

/* GBDATA *GBT_first_marked_gene_rel_species(GBDATA *gb_species) */
/* { */
/*   GBDATA *gene_data; */
/*   gene_data = GB_find(gb_species,"gene_data",0,down_level); */
/*   return GB_first_marked(gene_data,"gene"); */
/* } */

/* GBDATA *GBT_get_gene_data(GBDATA *gb_main) { */
/*   return GB_search(gb_main,"gene_data",GB_CREATE_CONTAINER); */
/* } */

/* GBDATA *GBT_find_gene_rel_species(GBDATA *gb_species,const char *name) */
/* { */

/*     GBDATA *gb_gene_name; */
/*     GBDATA *gb_gene_data; */
/*     gb_gene_data = GB_find(gb_species,"gene_data",0,down_level); */
/*     gb_gene_name = GB_find(gb_gene_data,"name",name,down_2_level); */
/*     if (!gb_gene_name) return 0; */
/*     return GB_get_father(gb_gene_name); */
/* } */


#endif


//  --------------
//      genes:
//  --------------

GBDATA* GEN_get_gene_data(GBDATA *gb_species) {
    return GB_search(gb_species, "gene_data", GB_CREATE_CONTAINER);
}

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
    GBDATA *gb_gene = 0;

    if (gb_name) return GB_get_father(gb_name); // found existing gene

    gb_gene = GB_create_container(gb_gene_data, "gene");
    gb_name = GB_create(gb_gene, "name", GB_STRING);
    GB_write_string(gb_name, name);

    return gb_gene;
}

GBDATA* GEN_create_gene(GBDATA *gb_species, const char *name) {
    return GEN_create_gene_rel_gene_data(GEN_get_gene_data(gb_species), name);
}

GBDATA* GEN_first_gene(GBDATA *gb_species) {
    return GB_find(GEN_get_gene_data(gb_species), "gene", 0, down_level);
}

GBDATA* GEN_first_gene_rel_gene_data(GBDATA *gb_gene_data) {
    return GB_find(gb_gene_data, "gene", 0, down_level);
}

GBDATA* GEN_next_gene(GBDATA *gb_gene) {
    return GB_find(gb_gene, "gene", 0, this_level|search_next);
}

GBDATA *GEN_first_marked_gene(GBDATA *gb_species) {
    return GB_first_marked(GEN_get_gene_data(gb_species), "gene");
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
    gb_assert(GEN_find_origin_organism(gb_pseudo) == 0);

    gb_name = GB_find(gb_pseudo, "name", 0, down_level);

    error = GB_export_error("The gene-species '%s' refers to an unknown organism (%s)\n"
                            "This occurs if you rename or delete the organism or change the entry 'ARB_origin_species'\n"
                            "and might cause serious problems.",
                            GB_read_char_pntr(gb_name),
                            GEN_origin_organism(gb_pseudo));

    return error;
}



// @@@ FIXME: missing: GEN_gene_not_found (like GEN_organism_not_found)





GBDATA *GEN_find_pseudo_species(GBDATA *gb_main, const char *organism_name, const char *gene_name) {
    GBDATA *gb_pseudo;

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
    return gb_pseudo;
}

GBDATA *GEN_find_origin_organism(GBDATA *gb_pseudo) {
    const char *origin_species_name = GEN_origin_organism(gb_pseudo);
    return origin_species_name
        ? GBT_find_species_rel_species_data(GB_get_father(gb_pseudo), origin_species_name)
        : 0;
}

GBDATA *GEN_find_origin_gene(GBDATA *gb_pseudo) {
    const char *origin_gene_name = GEN_origin_gene(gb_pseudo);
    GBDATA     *gb_organism      = 0;
    if (!origin_gene_name) return 0;

    gb_organism = GEN_find_origin_organism(gb_pseudo);
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
    return gb_species;
}



