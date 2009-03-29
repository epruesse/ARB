// ================================================================ //
//                                                                  //
//   File      : tools.h                                            //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef TOOLS_H
#define TOOLS_H

#ifndef TYPES_H
#include "types.h"
#endif
#ifndef _CPP_CCTYPE
#include <cctype>
#endif


inline bool beginsWith(const string& str, const string& start) {
    return str.find(start) == 0;
}

inline bool endsWith(const string& str, const string& postfix) {
    size_t slen = str.length();
    size_t plen = postfix.length();

    if (plen>slen) { return false; }
    return str.substr(slen-plen) == postfix;
}

inline void appendSpaced(string& str, const string& toAppend) {
    if (!toAppend.empty()) {
        if (!str.empty()) str.append(1, ' ');
        str.append(toAppend);
    }
}

bool parseInfix(const string &str, const string& prefix, const string& postfix, string& foundInfix);

// --------------------------------------------------------------------------------

#define CURRENT_REST string(pos, end).c_str() 

struct StringParser {
    stringCIter pos, end;

    StringParser(const string& str) : pos(str.begin()), end(str.end()) {}

    bool atEnd() const { return pos == end; }

    unsigned char at() const { gi_assert(pos != end); return *pos; }

    stringCIter getPosition() const { return pos; }
    void setPosition(const stringCIter& position) { pos = position; }

    string rest() const { return string(pos, end); }

    stringCIter find(char c) {
        while (pos != end && *pos != c) {
            ++pos;
        }
        return pos;
    }


    size_t eatSpaces() {
        int spaces = 0;
        while (pos != end && *pos == ' ') {
            ++pos;
            ++spaces;
        }
        return spaces;
    }

    size_t expectSpaces(size_t count = 1, bool allowMore = true) {
        size_t spaces      = eatSpaces();
        bool   validNumber = allowMore ? spaces >= count : spaces == count;

        if (!validNumber) {
            throw GBS_global_string("Expected %u%s spaces, found %u (before '%s')",
                                    count, allowMore ? " or more" : "",
                                    spaces, CURRENT_REST);
        }
        return spaces;
    }

    void expectWord(const char *word) {
        for (size_t p = 0; word[p]; ++p, ++pos) {
            if (word[p] != *pos) {
                throw GBS_global_string("Expected to see '%s'", word);
            }
        }
    }

    string extractWord(const char *delimiter = " ") {
        if (atEnd() || strchr(delimiter, *pos) != 0) {
            throw GBS_global_string("Expected non-delimiter at '%s'", CURRENT_REST);
        }

        stringCIter start = pos++;

        while (!atEnd() && strchr(delimiter, *pos) == 0) ++pos;
        return string(start, pos);
    }

    long extractNumber() {
        long l           = 0;
        bool seen_digits = false;
        char c;
    
        for (; isdigit(c = *pos); ++pos) {
            l           = l*10+(c-'0');
            seen_digits = true;
        }

        if (!seen_digits) {
            throw GBS_global_string("Expected number, found '%s'", CURRENT_REST);
        }

        return l;
    }

};

#else
#error tools.h included twice
#endif // TOOLS_H

