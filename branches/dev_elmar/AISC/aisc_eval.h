//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //

#ifndef AISC_EVAL_H
#define AISC_EVAL_H

#ifndef AISC_DEF_H
#include "aisc_def.h"
#endif
#ifndef AISC_LOCATION_H
#include "aisc_location.h"
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

class Expression : virtual Noncopyable {
    const Data&     data;
    const Location& loc;

    int   used; // currently used size
    int   bufsize; // including zero-byte
    char *ebuffer;

    bool allow_missing_ref;

    bool was_evaluated;
    int  errors_before;

    char *eval_math(char *expr, char op_char);
    char *evalPart(int start, int end, int& result_len);

    bool evalRest(int offset);

    char *evalBehind(int offset) {
        aisc_assert(!was_evaluated);
        char *toEval = strstr(ebuffer+offset, "$(");
        return (!toEval || evalRest(toEval-ebuffer)) ? ebuffer : NULL;
    }


    char *report_result(char *s, bool& failed) {
        int errors_after = Location::get_error_count();
        failed           = errors_after != errors_before;

        aisc_assert(!(failed && s));
        if (s) {
            aisc_assert(s == ebuffer);
            ebuffer = NULL; // now owned by caller
        }

        was_evaluated = true;
        return s;
    }

public:
    Expression(const Data& data_, const Location& loc_, const char *str, bool allow_missing_ref_)
        : data(data_),
          loc(loc_),
          allow_missing_ref(allow_missing_ref_), 
          was_evaluated(false),
          errors_before(Location::get_error_count())
    {
        used       = strlen(str);
        bufsize    = used*2+1;
        ebuffer = (char*)malloc(bufsize);
        aisc_assert(used<bufsize);
        memcpy(ebuffer, str, used+1);
    }
    ~Expression() {
        aisc_assert(was_evaluated); // useless Expression
        free(ebuffer);
    }

    char *evaluate(bool& failed) {
        return report_result(evalBehind(0), failed);
    }

    char *evalVarDecl(bool& failed) {
        // dont eval first '$('
    
        char *res = NULL;
        char *varDecl  = strstr(ebuffer, "$(");
        if (varDecl) {
            res = evalBehind((varDecl+2)-ebuffer);
        }
        else {
            print_error(&loc, "Expected identifier, i.e. '$(ident)'");
        }
        return report_result(res, failed);
    }
};



#else
#error aisc_eval.h included twice
#endif // AISC_EVAL_H
