extern GBDATA *gb_main;
#define NT_RESORT_FILTER (1<<GB_STRING)|(1<<GB_INT)|(1<<GB_FLOAT)

AW_window *NT_build_resort_window(AW_root *awr);
void NT_build_resort_awars(AW_root *awr, AW_default aw_def);
void NT_resort_data_by_phylogeny(AW_window *dummy, GBT_TREE **ptree);
