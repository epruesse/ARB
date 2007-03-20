// =============================================================== //
//                                                                 //
//   File      : arb_version.h                                     //
//   Purpose   :                                                   //
//   Time-stamp: <Wed Mar/14/2007 12:19 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2007     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //
#ifndef ARB_VERSION_H
#define ARB_VERSION_H

#include <arb_build.h>

#define BUILD_BY BUILD_USER##__##BUILD_HOST

#ifdef DEVEL_RELEASE // we build a release
# if BUILD_BY == westram__waltz_mikro_biologie_tu_muenchen_de
#  define VERSION_ID "org" // original build by Institute of Microbiology (Technical University Munich) 
# else
#  if BUILD_BY == user__machine_mpi_bremen_de
#   define VERSION_ID "mpi" // build by MPI Bremen 
#  endif
# endif
#endif

#ifndef VERSION_ID
# define VERSION_ID "prv"              // private build
# define SHOW_WHERE_BUILD
#endif

#ifdef DEBUG
# define ARB_VERSION DATE VERSION_ID "D"
#else
# define ARB_VERSION DATE VERSION_ID
#endif

#else
#error arb_version.h included twice
#endif // ARB_VERSION_H




