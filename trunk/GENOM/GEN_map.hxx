/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef GEN_MAP_HXX
#define GEN_MAP_HXX

AW_window *GEN_map_create_main_window(AW_root *awr);
void       GEN_gene_container_changed_cb(GBDATA *gb_gene_data, int *cl_ntw, GB_CB_TYPE gb_type);

#else
#error GEN_map.hxx included twice
#endif // GEN_MAP_HXX
