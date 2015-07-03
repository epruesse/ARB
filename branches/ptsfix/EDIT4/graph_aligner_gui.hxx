// =============================================================== //
//                                                                 //
//   File      : graph_aligner_gui.hxx                             //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Elmar Pruesse in October 2008                        //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GRAPH_ALIGNER_GUI_HXX
#define GRAPH_ALIGNER_GUI_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

void      show_sina_window(UNFIXED, const struct AlignDataAccess *alignData);
void      create_sina_variables(AW_root*, AW_default);
AW_active sina_mask(AW_root*);

#else
#error graph_aligner_gui.hxx included twice
#endif // GRAPH_ALIGNER_GUI_HXX
