#ifndef GEN_DB_HXX
#define GEN_DB_HXX
class AW_window;
class AW_root;
// genes:

GBDATA* GEN_find_gene(GBDATA *gb_species, const char *name); // find existing gene
GBDATA* GEN_create_gene(GBDATA *gb_species, const char *name); // create or find existing gene

GBDATA* GEN_first_gene(GBDATA *gb_species);
GBDATA* GEN_next_gene(GBDATA *gb_gene);

// annotations:

GBDATA* GEN_find_annotation(GBDATA *gb_gene, const char *name); // find existing annotation
GBDATA* GEN_create_annotation(GBDATA *gb_gene, const char *name); // create or find existing annotation

GBDATA* GEN_first_annotation(GBDATA *gb_gene);
GBDATA* GEN_next_annotation(GBDATA *gb_gene);

//functions in submenu of gen_info window
void gene_delete_cb(AW_window *aww);
//void GEN_map_gene(AW_root *aw_root, AW_CL scannerid);
void gene_rename_cb(AW_window *aww);
void gene_copy_cb(AW_window *aww);
void gene_create_cb(AW_window *aww);
void GEN_create_field_items(AW_window *aws);
void ad_list_reorder_cb(AW_window *aws);
#else
#error GEN_db.hxx included twice
#endif // GEN_DB_HXX
