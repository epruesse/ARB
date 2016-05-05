// =============================================================== //
//                                                                 //
//   File      : arb_version.h                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2007     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //
#ifndef ARB_VERSION_H
#define ARB_VERSION_H

// exports the following variables:
//
// ARB_BUILD_DATE           contains the date on which arb was build
// ARB_BUILD_YEAR           contains the year in which arb was build
// ARB_BUILD_HOST           contains the host on which arb was build
// ARB_BUILD_USER           contains the user by whom  arb was build
//
// ARB_VERSION              arb version (no revision number for rc and stable)
// ARB_VERSION_DETAILED     arb version (always including revision number)

#include <arb_build.h>
#include <svn_revision.h>

#ifndef ARB_VERSION
#error No version defined
#endif

#else
#error arb_version.h included twice
#endif // ARB_VERSION_H




