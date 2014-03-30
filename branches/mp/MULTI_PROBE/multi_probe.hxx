// ================================================================ //
//                                                                  //
//   File      : multi_probe.hxx                                    //
//   Purpose   : External interface of MULTI_PROBE                  //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef MULTI_PROBE_HXX
#define MULTI_PROBE_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

AW_window *create_multiprobe_window(AW_root *root, AW_CL cl_canvas);

#else
#error multi_probe.hxx included twice
#endif // MULTI_PROBE_HXX