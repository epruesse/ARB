// ================================================================= //
//                                                                   //
//   File      : aw_msg.hxx                                          //
//   Purpose   : Provide aw_message and related functions            //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef AW_MSG_HXX
#define AW_MSG_HXX

#ifndef ARB_ERROR_H
#include <arb_error.h>
#endif


void aw_message(const char *msg);
inline void aw_message_if(GB_ERROR error) { if (error) aw_message(error); }
inline void aw_message_if(ARB_ERROR& error) { aw_message_if(error.deliver()); }

#else
#error aw_msg.hxx included twice
#endif // AW_MSG_HXX
