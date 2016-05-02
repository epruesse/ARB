#ifndef PAUP_H
#define PAUP_H

struct Paup {
    int         ntax;           // number of sequences
    int         nchar;          // max number of chars per sequence
    const char *equate;         // equal meaning char
    char        gap;            // char of gap, default is '-'

    Paup() {
        ntax  = 0;
        nchar = 0;
        equate = "~=.|><";
        gap    = '-';
    }
};

#else
#error paup.h included twice
#endif // PAUP_H
