/*  ====================================================================  */
/*                                                                        */
/*    File      : adGene.h                                                */
/*    Purpose   : Basic gene access functions                             */
/*    Time-stamp: <Fri Oct/01/2004 19:50 MET Coder@ReallySoft.de>         */
/*                                                                        */
/*                                                                        */
/*  Coded by Ralf Westram (coder@reallysoft.de) in July 2002              */
/*  Copyright Department of Microbiology (Technical University Munich)    */
/*                                                                        */
/*  Visit our web site at: http://www.arb-home.de/                        */
/*                                                                        */
/*  ====================================================================  */

#ifndef ADGENE_H
#define ADGENE_H

#ifndef ARBDB_H
#include "arbdb.h"
#endif 

#ifdef __cplusplus
extern "C" {
#endif

    /* -------------------------------------------------------------------------------- */
    /* genes : */

    GBDATA *GEN_get_gene_data(GBDATA *gb_species);

    GBDATA* GEN_find_gene(GBDATA *gb_species, const char *name); /* find existing gene */
    GBDATA* GEN_create_gene(GBDATA *gb_species, const char *name); /* create or find existing gene */
    GBDATA* GEN_find_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name); /* find existing gene */
    GBDATA* GEN_create_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name); /* create or find existing gene */

    GBDATA* GEN_first_gene(GBDATA *gb_species);
    GBDATA* GEN_first_gene_rel_gene_data(GBDATA *gb_gene_data);
    GBDATA* GEN_next_gene(GBDATA *gb_gene);

    GBDATA *GEN_first_marked_gene(GBDATA *gb_species);
    GBDATA *GEN_next_marked_gene(GBDATA *gb_gene);

    /* -------------------------------------------------------------------------------- */
    /* pseudo gene-species: */

    GB_BOOL GEN_is_pseudo_gene_species(GBDATA *gb_species);
    GB_BOOL GEN_is_organism(GBDATA *gb_species);

    const char *GEN_origin_organism(GBDATA *gb_pseudo);
    const char *GEN_origin_gene(GBDATA *gb_pseudo);
    GBDATA     *GEN_find_origin_organism(GBDATA *gb_pseudo);
    GBDATA     *GEN_find_origin_gene(GBDATA *gb_pseudo);
    GBDATA     *GEN_find_pseudo_species(GBDATA *gb_main, const char *organism_name, const char *gene_name);

    GB_ERROR GEN_organism_not_found(GBDATA *gb_pseudo);


    GBDATA *GEN_find_pseudo(GBDATA *gb_organism, GBDATA *gb_gene);

    GBDATA *GEN_first_pseudo_species(GBDATA *gb_main);
    GBDATA *GEN_first_pseudo_species_rel_species_data(GBDATA *gb_species_data);
    GBDATA *GEN_next_pseudo_species(GBDATA *gb_species);

    GBDATA *GEN_first_marked_pseudo_species(GBDATA *gb_main);
    GBDATA *GEN_next_marked_pseudo_species(GBDATA *gb_species);

    /* -------------------------------------------------------------------------------- */
    /* organisms: */

    GBDATA *GEN_find_organism(GBDATA *gb_main, const char *name);

    GBDATA *GEN_first_organism(GBDATA *gb_main);
    GBDATA *GEN_next_organism(GBDATA *gb_organism);

    GBDATA *GEN_first_marked_organism(GBDATA *gb_main);
    GBDATA *GEN_next_marked_organism(GBDATA *gb_organism);



#ifdef __cplusplus
}
#endif


#else
#error adGene.h included twice
#endif /* ADGENE_H */

