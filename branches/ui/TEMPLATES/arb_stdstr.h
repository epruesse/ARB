// =============================================================== //
//                                                                 //
//   File      : arb_stdstr.h                                      //
//   Purpose   : inlined string functions using std::string        //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2002      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ARB_STDSTR_H
#define ARB_STDSTR_H

inline bool beginsWith(const std::string& str, const std::string& start) {
    return str.find(start) == 0;
}

inline bool endsWith(const std::string& str, const std::string& postfix) {
    size_t slen = str.length();
    size_t plen = postfix.length();

    if (plen>slen) { return false; }
    return str.substr(slen-plen) == postfix;
}

#else
#error arb_stdstr.h included twice
#endif // ARB_STDSTR_H
