// =============================================================== //
//                                                                 //
//   File      : sec_abspos.hxx                                    //
//   Purpose   : Encapsulates helix position access                //
//   Time-stamp: <Fri Sep/07/2007 08:56 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SEC_ABSPOS_HXX
#define SEC_ABSPOS_HXX

#ifndef _CPP_CSTDIO
#include <cstdio>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

#ifndef SEC_DEFS_HXX
#include "SEC_defs.hxx"
#endif


class XString : Noncopyable {
    char   *x_string;
    size_t  x_string_len;

    int x_count;                // number of 'x's found in x_string

    size_t *abspos;             // contains absolute positions for all 'x's in 'x_string'
    int    *number_found;       // for each absolute position P, contains number of 'x's from start of sequence till P-1

    bool initialized;           // true if initialize called

    void set_length(size_t len); // dealloc internal array if length grows
    
    void addX(size_t abspos) { // add an X at pos
        initialized   = false;
        sec_assert(abspos<x_string_len);
        x_string[abspos] = 'x';
    }

public:
    XString(size_t ali_length);
    XString(const char *saved_x_string, size_t saved_len, size_t ali_length);
    ~XString();

    void initialize();          // builds all internal data from x_string

    int getXcount() const {
        sec_assert(initialized);
        return x_count;
    }
    size_t getLength() const { return x_string_len; }
    
    size_t getAbsPos(int x) const; // gets absolute position of a 'x'
    int getXleftOf(size_t abspos) const; // gets the number of 'x's left of sequence pos

    void addXpair(size_t start, size_t end) {
#if defined(DEBUG) && 0
        printf("addXpair(%u, %u)\n", start, end);
#endif // DEBUG
        addX(start);
        addX(end);
    }

    const char *get_x_string() const; // version saved to DB
    bool alignment_too_short() const { return x_string[x_string_len-1] == 'x'; }
    size_t get_x_string_length() const { return getLength() - (alignment_too_short() ? 0 : 1); } // version saved to DB
};

#else
#error sec_abspos.hxx included twice
#endif // SEC_ABSPOS_HXX
