#include <arbdb.h>

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

//  --------------------
//      annotations:
//  --------------------

inline GBDATA* get_ann_data(GBDATA *gb_species) {
    return GB_search(gb_species, "ann_data", GB_CREATE_CONTAINER);
}

GBDATA* GEN_find_annotation(GBDATA *gb_gene, const char *name) {
    // find existing annotation
    GBDATA *gb_ann_data = get_ann_data(gb_gene);
    GBDATA *gb_name = GB_find(gb_ann_data, "name", name, down_2_level);

    if (gb_name) return GB_get_father(gb_name); // found existing annotation
    return 0;
}

GBDATA* GEN_create_annotation(GBDATA *gb_gene, const char *name) {
    // create or find existing annotation
    GBDATA *gb_ann_data = get_ann_data(gb_gene);
    GBDATA *gb_name = GB_find(gb_ann_data, "name", name, down_2_level);

    if (gb_name) return GB_get_father(gb_name); // found existing annotation

    GBDATA *gb_ann = GB_create_container(gb_ann_data, "ann");
    gb_name = GB_create(gb_ann, "name", GB_STRING);
    GB_write_string(gb_name, name);

    return gb_ann;
}

GBDATA* GEN_first_annotation(GBDATA *gb_gene) {
    GBDATA *gb_ann_data = get_ann_data(gb_gene);
    return GB_find(gb_ann_data, "ann", 0, down_level);
}
GBDATA* GEN_next_annotation(GBDATA *gb_gene) {
    return GB_find(gb_gene, "ann", 0, this_level|search_next);
}


