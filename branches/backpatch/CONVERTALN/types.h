// ================================================================= //
//                                                                   //
//   File      : types.h                                             //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef TYPES_H
#define TYPES_H

struct Convaln_exception {
    int         error_code;
    const char *error;

    Convaln_exception(int error_code_, const char *error_)
        : error_code(error_code_),
          error(error_)
    {}
};

#else
#error types.h included twice
#endif // TYPES_H
