//  ==================================================================== //
//                                                                       //
//    File      : inline.h                                               //
//    Purpose   : general purpose inlined funcions                       //
//    Time-stamp: <Fri Jun/14/2002 19:16 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in June 2002             //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef INLINE_H
#define INLINE_H

#include <cctype>

/** Like strcmp but ignoring case */
inline int stricmp(const char *s1, const char *s2) {
    int cmp = 0;
    size_t idx = 0;
    while (!cmp) {
        if (!s1[idx]) return s2[idx] ? -1 : 0;
        if (!s2[idx]) return 1;
        cmp = tolower(s1[idx]) - tolower(s2[idx]);
        ++idx;
    }
    return cmp;
}


#else
#error inline.h included twice
#endif // INLINE_H

