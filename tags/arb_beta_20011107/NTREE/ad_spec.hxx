#define AWAR_SPECIES_DEST "tmp/adspec/dest"
#define AWAR_SPECIES_INFO "tmp/adspec/info"
#define AWAR_SPECIES_KEY  "tmp/adspec/key"

#define AWAR_FIELD_CREATE_NAME "tmp/adfield/name"
#define AWAR_FIELD_CREATE_TYPE "tmp/adfield/type"
#define AWAR_FIELD_DELETE	   "tmp/adfield/source"


#define AWAR_FIELD_REORDER_SOURCE "tmp/ad_reorder/source"
#define AWAR_FIELD_REORDER_DEST   "tmp/ad_reorder/dest"

void       create_species_var(AW_root *aw_root, AW_default aw_def);
AW_window *NT_create_species_window(AW_root *aw_root);
AW_window *NT_create_organism_window(AW_root *aw_root);
AW_window *ad_create_query_window(AW_root *aw_root);
void       ad_unquery_all();
void       ad_query_update_list();
void       ad_spec_create_field_items(AW_window *aws);

AW_window *NT_create_ad_list_reorder(AW_root *root, AW_CL cl_item_selector);
AW_window *NT_create_ad_field_delete(AW_root *root, AW_CL cl_item_selector);
AW_window *NT_create_ad_field_create(AW_root *root, AW_CL cl_item_selector);
