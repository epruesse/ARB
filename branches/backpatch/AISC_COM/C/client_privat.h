// =============================================================== //
//                                                                 //
//   File      : client_privat.h                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef CLIENT_PRIVAT_H
#define CLIENT_PRIVAT_H

#ifndef AISC_GLOBAL_H
#include "aisc_global.h"
#endif


#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef SIGHANDLER_H
#include <SigHandler.h>
#endif

#define AISC_MAX_ATTR           4095
#define MAX_AISC_SET_GET        16
#define AISC_MAX_STRING_LEN     1024
#define AISC_MESSAGE_BUFFER_LEN ((AISC_MAX_STRING_LEN/4+3)*(16+2))

struct aisc_bytes_list {
    char            *data;
    int              size;
    aisc_bytes_list *next;
};

struct aisc_com {
    int         socket;
    int         message_type;
    char       *message;
    int        *message_queue;
    long        magic;
    const char *error;

    long             aisc_mes_buffer[AISC_MESSAGE_BUFFER_LEN];
    aisc_bytes_list *aisc_client_bytes_first;
    aisc_bytes_list *aisc_client_bytes_last;
    SigHandler       old_sigpipe_handler;
};

#define AISC_MAGIC_NUMBER 0

enum aisc_command_list {
    AISC_GET         = AISC_MAGIC_NUMBER + 0,
    AISC_SET         = AISC_MAGIC_NUMBER + 1,
    AISC_NSET        = AISC_MAGIC_NUMBER + 2,
    AISC_CREATE      = AISC_MAGIC_NUMBER + 3,
    AISC_FIND        = AISC_MAGIC_NUMBER + 4,
    AISC_COPY        = AISC_MAGIC_NUMBER + 5,
    AISC_DELETE      = AISC_MAGIC_NUMBER + 6,
    AISC_INIT        = AISC_MAGIC_NUMBER + 7,
    AISC_DEBUG_INFO  = AISC_MAGIC_NUMBER + 8,
    AISC_FORK_SERVER = AISC_MAGIC_NUMBER + 9
};

enum aisc_client_command_list {
    AISC_CCOM_OK      = AISC_MAGIC_NUMBER + 0,
    AISC_CCOM_ERROR   = AISC_MAGIC_NUMBER + 1,
    AISC_CCOM_MESSAGE = AISC_MAGIC_NUMBER + 2
};

#else
#error client_privat.h included twice
#endif // CLIENT_PRIVAT_H
