// ================================================================ //
//                                                                  //
//   File      : ed4_naligner.hxx                                   //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ED4_NALIGNER_HXX
#define ED4_NALIGNER_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

void create_naligner_variables(AW_root *root, AW_default db1);
AW_window *create_naligner_window(AW_root *root);

#else
#error ed4_naligner.hxx included twice
#endif // ED4_NALIGNER_HXX
