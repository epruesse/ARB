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

#ifndef PT_COM_H
#include <PT_com.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef SERVER_H
extern "C" {
#include <server.h>
}
#endif
#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef SERVERCNTRL_H
#include <servercntrl.h>
#endif

#else
#error ali_other_stuff.hxx included twice
#endif // ALI_OTHER_STUFF_HXX
