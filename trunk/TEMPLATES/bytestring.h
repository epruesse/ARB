// ============================================================== //
//                                                                //
//   File      : bytestring.h                                     //
//   Purpose   :                                                  //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2010   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef BYTESTRING_H
#define BYTESTRING_H

struct bytestring {
    char *data;
    int   size;
};

#else
#error bytestring.h included twice
#endif // BYTESTRING_H
