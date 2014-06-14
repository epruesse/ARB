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
#ifndef _GLIBCXX_CCTYPE
#include <cctype>
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif

inline void appendSpaced(std::string& str, const std::string& toAppend) {
    if (!toAppend.empty()) {
        if (!str.empty()) str.append(1, ' ');
        str.append(toAppend);
    }
}

// --------------------------------------------------------------------------------

#define CURRENT_REST std::string(pos, end).c_str()

struct StringParser {
    stringCIter pos, end;

    StringParser(const std::string& str) : pos(str.begin()), end(str.end()) {}

    bool atEnd() const { return pos == end; }

    unsigned char at() const { gi_assert(pos != end); return *pos; }

    stringCIter getPosition() const { return pos; }
    void setPosition(const stringCIter& position) { pos = position; }
    void advance(size_t offset) { std::advance(pos, offset); }

    std::string rest() const { return std::string(pos, end); }

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
            throw GBS_global_string("Expected %zu%s spaces, found %zu (before '%s')",
                                    count, allowMore ? " or more" : "",
                                    spaces, CURRENT_REST);
        }
        return spaces;
    }

    size_t lookingAt(const char *content) {
        // returns 0 if different content is seen (or if content is "")
        // otherwise it returns the string length of content

        size_t      p;
        stringCIter look = pos;
        for (p = 0; content[p]; ++p, ++look) {
            if (content[p] != *look) {
                return 0;
            }
        }
        return p;
    }

    void expectContent(const char *content) {
        size_t len = lookingAt(content);
        if (!len) throw GBS_global_string("Expected to see '%s' (found='%s')", content, CURRENT_REST);
        std::advance(pos, len); // eat the found content
    }

    std::string extractWord(const char *delimiter = " ") {
        if (atEnd() || strchr(delimiter, *pos) != 0) {
            throw GBS_global_string("Expected non-delimiter at '%s'", CURRENT_REST);
        }

        stringCIter start = pos++;

        while (!atEnd() && strchr(delimiter, *pos) == 0) ++pos;
        return std::string(start, pos);
    }

    long eatNumber(bool &eaten) {
        long lnum = 0;
        char c;

        eaten = false;
        for (; isdigit(c = *pos); ++pos) {
            lnum  = lnum*10+(c-'0');
            eaten = true;
        }

        return lnum;
    }

    long extractNumber() {
        bool seen_digits;
        long lnum = eatNumber(seen_digits);

        if (!seen_digits) throw GBS_global_string("Expected number, found '%s'", CURRENT_REST);
        return lnum;
    }
};

#else
#error tools.h included twice
#endif // TOOLS_H

