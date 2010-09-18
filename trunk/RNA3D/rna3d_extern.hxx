// ================================================================ //
//                                                                  //
//   File      : rna3d_extern.hxx                                   //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef RNA3D_EXTERN_HXX
#define RNA3D_EXTERN_HXX

#ifndef ED4_PLUGINS_HXX
#include <ed4_plugins.hxx>
#endif

ED4_plugin start_RNA3D_plugin;

#else
#error rna3d_extern.hxx included twice
#endif // RNA3D_EXTERN_HXX
