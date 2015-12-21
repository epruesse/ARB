// ============================================================= //
//                                                               //
//   File      : adperl.h                                        //
//   Purpose   : special shell for perl                          //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef ADPERL_H
#define ADPERL_H

#ifndef ARBDB_H
#include "arbdb.h"
#endif

class GB_shell4perl : virtual Noncopyable {
    GB_shell shell;

public:
    GB_shell4perl() {}
    ~GB_shell4perl();
};


#else
#error adperl.h included twice
#endif // ADPERL_H
