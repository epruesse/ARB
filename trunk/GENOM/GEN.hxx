#ifndef GEN_HXX
#define GEN_HXX

#include <arbdb.h>
#include <aw_window.hxx>

#define GENOM_DB_TYPE "genom_db" // main flag (true=genom db, false/missing=normal db)
#define AD_F_ALL (AW_active)(-1)
#define AWAR_GENE_DEST "tmp/edit/gene_name_dest"


GB_ERROR GEN_read(GBDATA *gb_main, const char *filename, const char *ali_name);

AW_window *GEN_create_gene_window(AW_root *aw_root);
AW_window *GEN_create_gene_query_window(AW_root *aw_root);
AW_window *create_ad_list_reorder(AW_root *root);
AW_window *create_ad_field_delete(AW_root *root);
AW_window *create_ad_field_create(AW_root *root);


//functions in submenus of gen_info window
void gene_delete_cb(AW_window *aww);
void GEN_map_gene(AW_root *aw_root, AW_CL scannerid);
void gene_rename_cb(AW_window *aww);
void gene_copy_cb(AW_window *aww);
void gene_create_cb(AW_window *aww);
void GEN_create_field_items(AW_window *aws);
void ad_list_reorder_cb(AW_window *aws);
void AD_map_species(AW_root *aw_root, AW_CL scannerid);




#endif // GEN_HXX
