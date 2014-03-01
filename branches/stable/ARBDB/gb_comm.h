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

struct gbcmc_comm {
    int   socket;
    char *unix_name;
    char *error;
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
