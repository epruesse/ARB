// ================================================================ //
//                                                                  //
//   File      : a3_basen.h                                         //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef A3_BASEN_H
#define A3_BASEN_H

#define BASEN         6
#define BASENPUR      4
#define CHARS       256

#define INVALID     -1
#define INSERT      -2

typedef enum
{
    ADENIN  =  0,
    CYTOSIN =  1,
    GUANIN  =  2,
    URACIL  =  3,
    ONE     =  4,
    ANY     =  5
}
Base;

extern const int    BCharacter [BASEN];
extern const double BComplement[BASEN][BASEN];
extern const Base   BIndex     [CHARS];

#else
#error a3_basen.h included twice
#endif // A3_BASEN_H
