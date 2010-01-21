/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef GEN_NDS_HXX
#define GEN_NDS_HXX

void       GEN_create_nds_vars(AW_root *aw_root, AW_default awdef, GBDATA *gb_main, GB_CB NDS_changed_callback);
AW_window *GEN_open_nds_window(AW_root *aw_root, AW_CL cgb_main);
void       GEN_make_node_text_init(GBDATA *gb_main);
char      *GEN_make_node_text_nds(GBDATA *gb_main, GBDATA * gbd, int mode);

#else
#error GEN_nds.hxx included twice
#endif // GEN_NDS_HXX
