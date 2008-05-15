// ================================================================= //
//                                                                   //
//   File      : SEC_bonddef.cxx                                     //
//   Purpose   :                                                     //
//   Time-stamp: <Mon Sep/10/2007 10:55 MET Coder@ReallySoft.de>     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2007   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include <cstring>
#include <cctype>
#include <cstdlib>

#include <arbdbt.h>

#include "SEC_bonddef.hxx"
#include "SEC_defs.hxx"

void SEC_bond_def::clear()
{
    int i, j;
    for (i=0; i<SEC_BOND_BASE_CHARS; i++) {
        for (j=0; j<SEC_BOND_BASE_CHARS; j++) {
            bond[i][j] = ' ';
        }
    }
}

int SEC_bond_def::get_index(char base) const
{
    if (base == 0) return -1;

    const char *allowed = SEC_BOND_BASE_CHAR;
    const char *found = strchr(allowed, toupper(base));

    if (!found) return -1;
    
    int idx = int(found-allowed);
    sec_assert(idx>=0 && idx<SEC_BOND_BASE_CHARS);
    return idx;
}

GB_ERROR SEC_bond_def::insert(const char *pairs, char pair_char)
{
    GB_ERROR error = 0;

    if (pair_char==0) pair_char = ' ';

    if (!strchr(SEC_BOND_PAIR_CHAR, pair_char)) {
        error = GBS_global_string("Illegal pair-character '%c' (allowed: '%s')", pair_char, SEC_BOND_PAIR_CHAR);
    }
    else {
        char c1  = 0;
        int  idx = 0;

        while (1) {
            char c2 = pairs[idx++];

            if (!c2) { // end of string
                if (c1) error = "Odd number of characters in pair definition";
                break;
            }
            if (c2 == ' ') continue; // ignore spaces
            if (c1==0) { c1 = c2; continue; } // store first char

            int i1 = get_index(c1);
            int i2 = get_index(c2);

            if (i1==-1 || i2==-1) {
                char ic = i1==-1 ? c1 : c2;
                error   = GBS_global_string("Illegal base-character '%c' (allowed: '%s')", ic, SEC_BOND_BASE_CHAR);
                break;
            }
            else {
                bond[i1][i2] = pair_char;
                bond[i2][i1] = pair_char;
            }
            c1 = 0;
        }
    }
    if (error && pair_char != '@') {
        insert(pairs, '@'); // means error
    }
    return error;
}

char SEC_bond_def::get_bond(char base1, char base2) const
{
    int i1 = get_index(base1);
    int i2 = get_index(base2);

    if (i1==-1 || i2==-1) {
        return ' ';
    }
    return bond[i1][i2];
}

char *SEC_bond_def::get_pair_string(char pair_char) {
    char *str = (char*)malloc(5*5*3+1);
    char *ins = str;

    for (int i = 0; i<SEC_BOND_BASE_CHARS; ++i) {
        for (int j = i; j<SEC_BOND_BASE_CHARS; ++j) {
            if (bond[i][j] == pair_char) {
                if (ins>str) *ins++ = ' ';
                *ins++ = SEC_BOND_BASE_CHAR[i];
                *ins++ = SEC_BOND_BASE_CHAR[j];
            }
        }
    }
    *ins = 0;

    return str;
}


