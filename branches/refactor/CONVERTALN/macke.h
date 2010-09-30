// ================================================================= //
//                                                                   //
//   File      : macke.h                                             //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef MACKE_H
#define MACKE_H

class MackeReader {
    bool firstRead;

public:
    MackeReader() : firstRead(true) {}

    char in(FILE_BUFFER ifp1, FILE_BUFFER ifp2, FILE_BUFFER ifp3);
};


#else
#error macke.h included twice
#endif // MACKE_H
