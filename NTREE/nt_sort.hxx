#error "bad header"

extern GBDATA *GLOBAL_gb_main;

AW_window *NT_build_resort_window(AW_root *awr);
void       NT_build_resort_awars(AW_root *awr, AW_default aw_def);
void       NT_resort_data_by_phylogeny(AW_window *dummy, GBT_TREE **ptree);
GB_ERROR   NT_resort_data_base(GBT_TREE *tree, const char *key1, const char *key2, const char *key3) __ATTR__USERESULT;
