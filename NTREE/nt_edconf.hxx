
// see also ARBDB/adtools/GBT_find_configuration
void       NT_start_editor_on_tree(AW_window *, GBT_TREE **ptree, int use_species_aside);
GB_ERROR   NT_create_configuration(AW_window *, GBT_TREE **ptree,const char *conf_name, int use_species_aside);
AW_window *NT_start_editor_on_old_configuration(AW_root *root);
AW_window *NT_extract_configuration(AW_root *awr);
void       NT_popup_configuration_admin(AW_window *aww, AW_CL cl_GBT_TREE_ptr, AW_CL);
