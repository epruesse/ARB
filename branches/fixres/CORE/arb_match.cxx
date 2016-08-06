// ================================================================= //
//                                                                   //
//   File      : arb_match.cxx                                       //
//   Purpose   : POSIX ERE                                           //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2013   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

// AISC_MKPT_PROMOTE:#ifndef ARB_CORE_H
// AISC_MKPT_PROMOTE:#include "arb_core.h"
// AISC_MKPT_PROMOTE:#endif

#include "arb_match.h"
#include "arb_assert.h"
#include "arb_msg.h"
#include "arb_string.h"
#include "arb_strbuf.h"

#include <regex.h>

// ---------------------------------------------
//      Regular Expressions search/replace

struct GBS_regex { regex_t compiled; }; // definition exists twice (see ../SL/REGEXPR/RegExpr.cxx)

inline char *give_buffer(size_t size) {
    static char   *buf     = NULL;
    static size_t  bufsize = 0;

    if (size<1) size = 1;
    if (bufsize<size) {
        bufsize = size;
        freeset(buf, (char*)ARB_alloc(bufsize));
    }
    return buf;
}

GBS_regex *GBS_compile_regexpr(const char *regexpr, GB_CASE case_flag, GB_ERROR *error) {
    GBS_regex *comreg  = (GBS_regex*)ARB_alloc(sizeof(*comreg));
    int        cflags  = REG_EXTENDED|(case_flag == GB_IGNORE_CASE ? REG_ICASE : 0)|REG_NEWLINE;
    int        errcode = regcomp(&comreg->compiled, regexpr, cflags);

    if (errcode != 0) {          // error compiling regexpr
        size_t  size = regerror(errcode, &comreg->compiled, NULL, 0);
        char   *buf  = give_buffer(size);

        regerror(errcode, &comreg->compiled, buf, size);
        *error = buf;

        free(comreg);
        comreg = NULL;
    }
    else {
        *error = NULL;
    }

    return comreg;
}

void GBS_free_regexpr(GBS_regex *toFree) {
    if (toFree) {
        regfree(&toFree->compiled);
        free(toFree);
    }
}

const char *GBS_unwrap_regexpr(const char *regexpr_in_slashes, GB_CASE *case_flag, GB_ERROR *error) {
    /* unwraps 'expr' from '/expr/[i]'
     * if slashes are not present, 'error' is set
     * 'case_flag' is set to GB_MIND_CASE   if format is '/expr/' or
     *                    to GB_IGNORE_CASE if format is '/expr/i'
     *
     * returns a pointer to a static buffer (containing the unwrapped expression)
     * (Note: The content is invalidated by the next call to GBS_unwrap_regexpr)
     */

    const char *result = NULL;
    const char *end    = strchr(regexpr_in_slashes, 0);

    if (end >= (regexpr_in_slashes+3)) {
        *case_flag = GB_MIND_CASE;
        if (end[-1] == 'i') {
            *case_flag = GB_IGNORE_CASE;
            end--;
        }
        if (regexpr_in_slashes[0] == '/' && end[-1] == '/') {
            arb_assert(*error == NULL);

            static char   *result_buffer = 0;
            static size_t  max_len    = 0;

            size_t len = end-regexpr_in_slashes-2;
            arb_assert(len>0); // don't accept empty expression

            if (len>max_len) {
                max_len = len*3/2;
                freeset(result_buffer, (char*)ARB_alloc(max_len+1));
            }

            memcpy(result_buffer, regexpr_in_slashes+1, len);
            result_buffer[len] = 0;

            result = result_buffer;
        }
    }

    if (!result) {
        *error = GBS_global_string("Regular expression format is '/expr/' or '/expr/i', not '%s'",
                                   regexpr_in_slashes);
    }
    return result;
}

const char *GBS_regmatch_compiled(const char *str, GBS_regex *comreg, size_t *matchlen) {
    /* like GBS_regmatch,
     * - but uses a precompiled regular expression
     * - no errors can occur here (beside out of memory, which is not handled)
     */

    regmatch_t  match;
    int         res      = regexec(&comreg->compiled, str, 1, &match, 0);
    const char *matchpos = NULL;

    if (res == 0) { // matched
        matchpos = str+match.rm_so;
        if (matchlen) *matchlen = match.rm_eo-match.rm_so;
    }

    return matchpos;
}

const char *GBS_regmatch(const char *str, const char *regExpr, size_t *matchlen, GB_ERROR *error) {
    /* searches 'str' for first occurrence of 'regExpr'
     * 'regExpr' has to be in format "/expr/[i]", where 'expr' is a POSIX extended regular expression
     *
     * for regexpression format see http://help.arb-home.de/reg.html#Syntax_of_POSIX_extended_regular_expressions_as_used_in_ARB
     *
     * returns
     * - pointer to start of first match in 'str' and
     *   length of match in 'matchlen' ('matchlen' may be NULL, then no len is reported)
     *                                            or
     * - NULL if nothing matched (in this case 'matchlen' is undefined)
     *
     * 'error' will be set if sth is wrong
     *
     * Note: Only use this function if you do exactly ONE match.
     *       Use GBS_regmatch_compiled if you use the regexpr twice or more!
     */
    const char *firstMatch     = NULL;
    GB_CASE     case_flag;
    const char *unwrapped_expr = GBS_unwrap_regexpr(regExpr, &case_flag, error);

    if (unwrapped_expr) {
        GBS_regex *comreg = GBS_compile_regexpr(unwrapped_expr, case_flag, error);
        if (comreg) {
            firstMatch = GBS_regmatch_compiled(str, comreg, matchlen);
            GBS_free_regexpr(comreg);
        }
    }

    return firstMatch;
}

char *GBS_regreplace(const char *str, const char *regReplExpr, GB_ERROR *error) {
    /* search and replace all matches in 'str' using POSIX extended regular expression
     * 'regReplExpr' has to be in format '/regexpr/replace/[i]'
     *
     * returns
     * - a heap copy of the modified string or
     * - NULL if something went wrong (in this case 'error' contains the reason)
     *
     * 'replace' may contain several special substrings:
     *
     *     "\n" gets replaced by '\n'
     *     "\t" -------''------- '\t'
     *     "\\" -------''------- '\\'
     *     "\0" -------''------- the complete match to regexpr
     *     "\1" -------''------- the match to the first subexpression
     *     "\2" -------''------- the match to the second subexpression
     *     ...
     *     "\9" -------''------- the match to the ninth subexpression
     */

    GB_CASE     case_flag;
    const char *unwrapped_expr = GBS_unwrap_regexpr(regReplExpr, &case_flag, error);
    char       *result         = NULL;

    if (unwrapped_expr) {
        const char *sep = unwrapped_expr;
        while (sep) {
            sep = strchr(sep, '/');
            if (!sep) break;
            if (sep>unwrapped_expr && sep[-1] != '\\') break;
        }

        if (!sep) {
            // Warning: GB_command_interpreter() tests for this error message - don't change
            *error = "Missing '/' between search and replace string";
        }
        else {
            char      *regexpr  = ARB_strpartdup(unwrapped_expr, sep-1);
            char      *replexpr = ARB_strpartdup(sep+1, NULL);
            GBS_regex *comreg   = GBS_compile_regexpr(regexpr, case_flag, error);

            if (comreg) {
                GBS_strstruct *out    = GBS_stropen(1000);
                int            eflags = 0;

                while (str) {
                    regmatch_t match[10];
                    int        res = regexec(&comreg->compiled, str, 10, match, eflags);

                    if (res == REG_NOMATCH) {
                        GBS_strcat(out, str);
                        str = 0;
                    }
                    else {      // found match
                        size_t p;
                        char   c;

                        GBS_strncat(out, str, match[0].rm_so);

                        for (p = 0; (c = replexpr[p]); ++p) {
                            if (c == '\\') {
                                c = replexpr[++p];
                                if (!c) break;
                                if (c >= '0' && c <= '9') {
                                    regoff_t start = match[c-'0'].rm_so;
                                    GBS_strncat(out, str+start, match[c-'0'].rm_eo-start);
                                }
                                else {
                                    switch (c) {
                                        case 'n': c = '\n'; break;
                                        case 't': c = '\t'; break;
                                        default: break;
                                    }
                                    GBS_chrcat(out, c);
                                }
                            }
                            else {
                                GBS_chrcat(out, c);
                            }
                        }

                        str    = str+match[0].rm_eo; // continue behind match
                        eflags = REG_NOTBOL; // for futher matches, do not regard 'str' as "beginning of line"
                    }
                }

                GBS_free_regexpr(comreg);
                result = GBS_strclose(out);
            }
            free(replexpr);
            free(regexpr);
        }
    }

    return result;
}

