// =============================================================== //
//                                                                 //
//   File      : ali_other_stuff.hxx                               //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ALI_OTHER_STUFF_HXX
#define ALI_OTHER_STUFF_HXX

#include <PT_com.h>

extern "C" {
#include <server.h>
}

#include <arbdbt.h>
#include <servercntrl.h>

#else
#error ali_other_stuff.hxx included twice
#endif // ALI_OTHER_STUFF_HXX
