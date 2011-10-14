// =============================================================== //
//                                                                 //
//   File      : gb_comm.h                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_COMM_H
#define GB_COMM_H

#ifndef ARB_ASSERT_H
#include "arb_assert.h"
#endif
#ifndef SIGHANDLER_H
#include <SigHandler.h>
#endif

struct gbcmc_comm {
    int         socket;
    char       *unix_name;
    char       *error;
    SigHandler  old_SIGPIPE_handler;
};


struct gb_user {
    char *username;
    int   userid;
    int   userbit;
    int   nusers;                                   // number of clients of this user
};

#else
#error gb_comm.h included twice
#endif // GB_COMM_H
