// =============================================================== //
//                                                                 //
//   File      : mg_merge.hxx                                      //
//   Purpose   : Merge tool external interface                     //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2008     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef MG_MERGE_HXX
#define MG_MERGE_HXX

#ifndef ARB_CORE_H
#include <arb_core.h>
#endif


extern GBDATA *GLOBAL_gb_src;
extern GBDATA *GLOBAL_gb_dst;

AW_window *create_MG_main_window(AW_root *aw_root);
void       MG_create_all_awars(AW_root *awr, AW_default aw_def, const char *fname_one = "db1.arb", const char *fname_two = "db2.arb");
void       MG_start_cb2(AW_window *aww, AW_root *aw_root, bool save_enabled, bool dest_is_new); // uses loaded databases
GB_ERROR   MG_simple_merge(AW_root *awr); // simple merge of species

#else
#error mg_merge.hxx included twice
#endif
