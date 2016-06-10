// ============================================================ //
//                                                              //
//   File      : ValueTuple.h                                   //
//   Purpose   : Abstract value usable for shading items        //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef VALUETUPLE_H
#define VALUETUPLE_H

class ValueTuple {
    /*! Contains a value-tuple (used for shading items).
     *
     * Properties:
     * - variable tuple-size (0-3)
     * - values may be defined or not
     * - values are normalized to [0.0 .. 1.0]
     * - values can be mixed (using weights)
     */



public:
    ValueTuple() {}

};


#else
#error ValueTuple.h included twice
#endif // VALUETUPLE_H
