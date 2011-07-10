// ================================================================= //
//                                                                   //
//   File      : ed4_known_plugins.hxx                               //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef ED4_KNOWN_PLUGINS_HXX
#define ED4_KNOWN_PLUGINS_HXX

void ED4_SECEDIT_start(AW_window *aw, AW_CL cl_gbmain, AW_CL);
void ED4_RNA3D_start(AW_window *aw, AW_CL cl_gb_main, AW_CL);

#else
#error ed4_known_plugins.hxx included twice
#endif // ED4_KNOWN_PLUGINS_HXX
