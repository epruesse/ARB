// ================================================================ //
//                                                                  //
//   File      : tools.cxx                                          //
//   Purpose   :                                                    //
//   Time-stamp: <Tue Nov/28/2006 17:42 MET Coder@ReallySoft.de>    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#include "tools.h"

using namespace std;


bool parseInfix(const string &str, const string& prefix, const string& postfix, string& foundInfix) {
    bool parsed = false;
    if (beginsWith(str, prefix) && endsWith(str, postfix)) {
        size_t strlen  = str.length();
    size_t prelen  = prefix.length();
    size_t postlen = postfix.length();

        if (strlen >= (prelen+postlen)) { // otherwise str is to short (prefix and postfix overlap)
            foundInfix = str.substr(prelen, strlen-(prelen+postlen));
            parsed     = true;
        }
    }
    return parsed;
}



