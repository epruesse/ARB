// =============================================================== //
//                                                                 //
//   File      : di_clusters.hxx                                   //
//   Purpose   : Detect clusters of homologous sequences in tree   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef DI_CLUSTERS_HXX
#define DI_CLUSTERS_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

void DI_create_cluster_awars(AW_root *aw_root, AW_default def, AW_default db);
AW_window *DI_create_cluster_detection_window(AW_root *aw_root, class WeightedFilter *weightedFilter);

#else
#error di_clusters.hxx included twice
#endif // DI_CLUSTERS_HXX
