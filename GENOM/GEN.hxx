// =============================================================== //
//                                                                 //
//   File      : GEN.hxx                                           //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2001   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GEN_HXX
#define GEN_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ITEMS_H
#include <items.h>
#endif

// --------------------------------------------------------------------------------
// this header is visible ARB-wide (so here are only things needed somewhere else)
// see GEN_local.hxx for local stuff
// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// awars:

#define AWAR_GENE_NAME  "tmp/gene/name"

void GEN_create_awars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main);

// --------------------------------------------------------------------------------
// windows/menus:

void GEN_popup_gene_infowindow(AW_root *aw_root, GBDATA *gb_main);
AW_window *GEN_create_gene_query_window(AW_root *aw_root, AW_CL cl_gb_main);
AW_window *GEN_create_first_map(AW_root *aw_root, GBDATA *gb_main);

class AW_window_menu_modes;
void GEN_create_genes_submenu(AW_window_menu_modes *awm, GBDATA *gb_main, bool for_ARB_NTREE);

// --------------------------------------------------------------------------------
// genes:

GBDATA* GEN_get_current_organism(GBDATA *gb_main, AW_root *aw_root); // uses AWAR_ORGANISM_NAME
GBDATA* GEN_get_current_gene_data(GBDATA *gb_main, AW_root *aw_root); // uses AWAR_ORGANISM_NAME

// --------------------------------------------------------------------------------
// toolkit:

class AW_repeated_question;
GB_ERROR GEN_testAndRemoveTranslations(GBDATA *gb_gene_data, void (*warn)(AW_CL cd, const char *msg), AW_CL cd, AW_repeated_question *ok_to_ignore_wrong_start_codon);

ItemSelector& GEN_get_selector();

#else
#error GEN.hxx included twice
#endif // GEN_HXX
