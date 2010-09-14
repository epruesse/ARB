// =============================================================== //
//                                                                 //
//   File      : secedit_extern.hxx                                //
//   Purpose   : external interface                                //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SECEDIT_EXTERN_HXX
#define SECEDIT_EXTERN_HXX

#ifndef ED4_PLUGINS_HXX
#include <ed4_plugins.hxx>
#endif

ED4_plugin start_SECEDIT_plugin;

#else
#error secedit_extern.hxx included twice
#endif // SECEDIT_EXTERN_HXX
