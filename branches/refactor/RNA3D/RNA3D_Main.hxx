// ================================================================ //
//                                                                  //
//   File      : RNA3D_Main.hxx                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef RNA3D_MAIN_HXX
#define RNA3D_MAIN_HXX

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif


void RNA3D_StartApplication(AW_root *awr, GBDATA *gb_main);

#else
#error RNA3D_Main.hxx included twice
#endif // RNA3D_MAIN_HXX
