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
// ARB_BUILD_DATE            contains the date on which arb was build
// ARB_BUILD_YEAR            contains the year in which arb was build
// ARB_BUILD_HOST           contains the host on which arb was build
// ARB_BUILD_USER           contains the user by whom  arb was build
//
// ARB_VERSION          arb version
//
// may define the following variable:
//
// SHOW_WHERE_BUILD     show build location/user (at arb_ntree command line and in mails sent from ARB)
//                      has to be set for non-official builds!

#include <arb_build.h>
#include <svn_revision.h>

#ifndef ARB_VERSION_MAJOR
#error No version defined
#endif

# define ARB_VERSION ARB_VERSION_MAJOR "." ARB_VERSION_MINOR "-" ARB_VERSION_TAG "-" ARB_SVN_REVISION

#else
#error arb_version.h included twice
#endif // ARB_VERSION_H




