// ============================================================== //
//                                                                //
//   File      : arb_msg_fwd.h                                    //
//   Purpose   : easily provide formatted versions of functions   //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2013   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef ARB_MSG_FWD_H
#define ARB_MSG_FWD_H

#ifndef ARB_MSG_H
#include <arb_msg.h>
#endif

#define FORWARD_FORMATTED(receiver,format)                      \
    do {                                                        \
        va_list parg;                                           \
        va_start(parg, format);                                 \
        char *result = GBS_vglobal_string_copy(format, parg);   \
        va_end(parg);                                           \
        receiver(result);                                       \
        free(result);                                           \
    } while(0)

#define FORWARD_FORMATTED_NORETURN(receiver,format)             \
    do {                                                        \
        va_list parg;                                           \
        va_start(parg, format);                                 \
        const char *result = GBS_vglobal_string(format, parg);  \
        va_end(parg);                                           \
        receiver(result);                                       \
    } while(0)

#else
#error arb_msg_fwd.h included twice
#endif // ARB_MSG_FWD_H
