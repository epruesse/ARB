// =======================================================================================
/*                                                                                       */
//    File       : ed4_visualizeSAI.hxx
//    Purpose    : Visualize the Sequence Associated Information (SAI) in the Editor
//    Author     : Yadhu Kumar (yadhu@mikro.biologie.tu-muenchen.de)
//    web site   : http://www.arb-home.de/
/*                                                                                       */
//        Copyright Department of Microbiology (Technical University Munich)
/*                                                                                       */
// =======================================================================================

#ifndef ED4_VISUALIZESAI_HXX
#define ED4_VISUALIZESAI_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif


void ED4_createVisualizeSAI_Awars(AW_root *aw_root, AW_default aw_def); // create awars
AW_window  *ED4_createVisualizeSAI_window(AW_root *aw_root);

const char *ED4_getSaiColorString(AW_root *awr, int start, int end);

#else
#error ed4_visualizeSAI.hxx included twice
#endif // ED4_VISUALIZESAI_HXX
