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

static void start_of_main() {
    // const char *USER_LOCALE = ""; // -> use user-defined locale
    const char *USER_LOCALE = "de_DE.UTF-8"; // use german locale


    // gtk apparently calls 'setlocale(LC_ALL, "");'
    // @@@ we should already call it here, to make ARB-motif-version behave samesame.

    // --------------------------------------------------------------------------------
    // Fails-counts mentioned below were determined under ubuntu 13.10/64bit.
    // Make sure only one section is enabled!

#if 1
    // enabling this section does not fail any unit test
    setlocale(LC_ALL,  "C");
    // overwritten as soon as gtk-GUI starts (causing wrong behavior throughout ARB)
#endif

#if 0
    // enabling this section does not fail any unit test
    setlocale(LC_ALL,     USER_LOCALE);
    setlocale(LC_NUMERIC, "C");
#endif

#if 0
    // enabling this section does not fail any unit test
    setlocale(LC_ALL,      "C");
    setlocale(LC_COLLATE,  USER_LOCALE);
    setlocale(LC_CTYPE,    USER_LOCALE);
    setlocale(LC_MESSAGES, USER_LOCALE);
    setlocale(LC_MONETARY, USER_LOCALE);
    setlocale(LC_TIME,     USER_LOCALE);
#endif


#if 0
    // enabling this section fails 57 unit tests (for german USER_LOCALE)
    setlocale(LC_ALL, USER_LOCALE);
#endif

#if 0
    // enabling this section fails 57 unit tests (for german USER_LOCALE)
    setlocale(LC_COLLATE,  "C");
    setlocale(LC_CTYPE,    "C");
    setlocale(LC_MESSAGES, "C");
    setlocale(LC_MONETARY, "C");
    setlocale(LC_NUMERIC,  USER_LOCALE);
    setlocale(LC_TIME,     "C");
#endif

#if 0
    // enabling this section fails 57 unit tests (for german USER_LOCALE)
    // (fails 57 unit tests under ubuntu 13.10, centos 5 and 6)
    setlocale(LC_ALL,     "C");
    setlocale(LC_NUMERIC, USER_LOCALE);
#endif
}

#else
#error arb_main.h included twice
#endif // ARB_MAIN_H
