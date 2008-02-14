#ifndef MG_MERGE_HXX
#define MG_MERGE_HXX

extern GBDATA *gb_merge;
extern GBDATA *gb_dest;
extern GBDATA *gb_main;

AW_window *create_MG_main_window(AW_root *aw_root);
void       MG_create_all_awars(AW_root *awr, AW_default aw_def, const char *fname_one = "db1.arb", const char *fname_two = "db2.arb");
void       MG_start_cb(AW_window *aww); // load the databases
void       MG_start_cb2(AW_window *aww, AW_root *aw_root, bool save_enabled, bool dest_is_new); // uses loaded databases
GB_ERROR   MG_simple_merge(AW_root *awr); // simple merge of species

#else
#error mg_merge.hxx included twice
#endif
