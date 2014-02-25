// ================================================================ //
//                                                                  //
//   File      : arb_main.h                                         //
//   Purpose   : code executed at start of main()                   //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2014   //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARB_MAIN_H
#define ARB_MAIN_H

#include <locale.h>

inline void start_of_main() {
#if 0
    setlocale(LC_ALL, ""); // this is apparently done by gtk as well.
                           // already call it here to make motif-version behave samesame.
    setlocale(LC_NUMERIC, "C");
#endif
}

#else
#error arb_main.h included twice
#endif // ARB_MAIN_H
