// =============================================================== //
//                                                                 //
//   File      : GEN_nds.hxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 2001           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GEN_NDS_HXX
#define GEN_NDS_HXX

#ifndef ARBDB_H
#include <arbdb.h>
#endif

void       GEN_create_nds_vars(AW_root *aw_root, AW_default awdef, GBDATA *gb_main, const DatabaseCallback& NDS_changed_callback);
AW_window *GEN_open_nds_window(AW_root *aw_root, GBDATA *gb_main);
void       GEN_make_node_text_init(GBDATA *gb_main);
char      *GEN_make_node_text_nds(GBDATA *gb_main, GBDATA * gbd, int mode);

#else
#error GEN_nds.hxx included twice
#endif // GEN_NDS_HXX
