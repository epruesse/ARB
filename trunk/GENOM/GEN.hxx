#ifndef GEN_HXX
#define GEN_HXX

#include <arbdb.h>
#include <aw_window.hxx>

#define GENOM_DB_TYPE "genom_db" // main flag (true=genom db, false/missing=normal db)

GB_ERROR GEN_read(GBDATA *gb_main, const char *filename, const char *ali_name);

AW_window *GEN_create_gene_window(AW_root *aw_root);
AW_window *GEN_create_gene_query_window(AW_root *aw_root);
AW_window *create_ad_list_reorder(AW_root *root);
AW_window *create_ad_field_delete(AW_root *root);
AW_window *create_ad_field_create(AW_root *root);
void AD_map_species(AW_root *aw_root, AW_CL scannerid);
#endif // GEN_HXX
