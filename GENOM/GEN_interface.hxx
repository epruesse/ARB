/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef GEN_INTERFACE_HXX
#define GEN_INTERFACE_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif


extern struct ad_item_selector GEN_item_selector;

// internal helpers :
extern "C" GB_ERROR GEN_mark_organism_or_corresponding_organism(GBDATA *gb_species, int *client_data);

#else
#error GEN_interface.hxx included twice
#endif // GEN_INTERFACE_HXX
