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

// @@@ convert the following constants into an enum
// and change result types where used
#define GBCM_SERVER_OK_WAIT 3
#define GBCM_SERVER_ABORTED 2
#define GBCM_SERVER_FAULT   1
#define GBCM_SERVER_OK      0

struct gbcmc_comm {
    int   socket;
    char *unix_name;
    char *error;
};


struct gb_user_struct {
    char *username;
    int   userid;
    int   userbit;
    int   nusers;                                   // number of clients of this user
};

#else
#error gb_comm.h included twice
#endif // GB_COMM_H
