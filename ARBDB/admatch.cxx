// =============================================================== //
//                                                                 //
//   File      : admatch.cxx                                       //
//   Purpose   : functions related to string match/replace         //
//                                                                 //
//   ReCoded for POSIX ERE                                         //
//            by Ralf Westram (coder@reallysoft.de) in April 2009  //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_local.h"

#include <cctype>
#include <regex.h>

// ------------------------
//      string matcher

enum string_matcher_type {
    SM_INVALID = -1,
    SM_ANY     = 0,             // matches any string
    SM_WILDCARDED,              // match with wildcards (GBS_string_matches)
    SM_REGEXPR,                 // match using regexpr
};

struct GBS_string_matcher {
    string_matcher_type  type;
    GB_CASE              case_flag;
    char                *wildexpr;
    GBS_regex           *regexpr;
};

GBS_string_matcher *GBS_compile_matcher(const char *search_expr, GB_CASE case_flag) {
    /* returns a valid string matcher (to be used with GBS_string_matches_regexp)
     * or NULL (in which case an error was exported)
     */

    GBS_string_matcher *matcher = (GBS_string_matcher*)malloc(sizeof(*matcher));
    GB_ERROR            error   = 0;

    matcher->type      = SM_INVALID;
    matcher->case_flag = case_flag;
    matcher->wildexpr  = NULL;
    matcher->regexpr   = NULL;

    if (search_expr[0] == '/') {
        const char *end = strchr(search_expr, 0)-1;
        if (end>search_expr && end[0] == '/') {
            GB_CASE     expr_attached_case;
            const char *unwrapped_expr = GBS_unwrap_regexpr(search_expr, &expr_attached_case, &error);

            if (unwrapped_expr) {
                if (expr_attached_case != GB_MIND_CASE) error = "format '/../i' not allowed here";
                else {
                    matcher->regexpr = GBS_compile_regexpr(unwrapped_expr, case_flag, &error);
                    if (matcher->regexpr) {
                        matcher->type = SM_REGEXPR;
                    }
                }
            }
        }
    }

    if (matcher->regexpr == NULL && !error) {
        if (strcmp(search_expr, "*") == 0) {
            matcher->type = SM_ANY;
        }
        else {
            matcher->type     = SM_WILDCARDED;
            matcher->wildexpr = strdup(search_expr);
        }
    }

    if (matcher->type == SM_INVALID) {
        error = GBS_global_string("Failed to create GBS_string_matcher from '%s'", search_expr);
    }

    if (error) {
        GBS_free_matcher(matcher);
        matcher = 0;
        GB_export_error(error);
    }
    return matcher;
}

void GBS_free_matcher(GBS_string_matcher *matcher) {
    free(matcher->wildexpr);
    if (matcher->regexpr) GBS_free_regexpr(matcher->regexpr);
    free(matcher);
}

// ---------------------------------------------
//      Regular Expressions search/replace

struct GBS_regex { regex_t compiled; }; // definition exists twice (see ../SL/REGEXPR/RegExpr.cxx)

GBS_regex *GBS_compile_regexpr(const char *regexpr, GB_CASE case_flag, GB_ERROR *error) {
    GBS_regex *comreg  = (GBS_regex*)malloc(sizeof(*comreg));
    int        cflags  = REG_EXTENDED|(case_flag == GB_IGNORE_CASE ? REG_ICASE : 0)|REG_NEWLINE;
    int        errcode = regcomp(&comreg->compiled, regexpr, cflags);

    if (errcode != 0) {          // error compiling regexpr
        size_t    size = regerror(errcode, &comreg->compiled, NULL, 0);
        GB_BUFFER buf  = GB_give_buffer(size);

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
            gb_assert(*error == NULL);

            static char   *result_buffer = 0;
            static size_t  max_len    = 0;

            size_t len = end-regexpr_in_slashes-2;
            gb_assert(len>0); // don't accept empty expression

            if (len>max_len) {
                max_len = len*3/2;
                freeset(result_buffer, (char*)malloc(max_len+1));
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
            char      *regexpr  = GB_strpartdup(unwrapped_expr, sep-1);
            char      *replexpr = GB_strpartdup(sep+1, NULL);
            GBS_regex *comreg   = GBS_compile_regexpr(regexpr, case_flag, error);

            if (comreg) {
                struct GBS_strstruct *out    = GBS_stropen(1000);
                int                   eflags = 0;

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

// -------------------------
//      wildcard search

GB_CSTR GBS_find_string(GB_CSTR str, GB_CSTR substr, int match_mode) {
    /* search a substring in another string
     * match_mode == 0     -> exact match
     * match_mode == 1     -> a==A
     * match_mode == 2     -> a==a && a==?
     * match_mode == else  -> a==A && a==?
     */
    const char *p1, *p2;
    char        b;

    switch (match_mode) {

        case 0: // exact match
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                }
                else {
                    if (b == *p1) {
                        p1++;
                        p2++;
                    }
                    else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2)   return (char *)str;
            break;

        case 1: // a==A
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                }
                else {
                    if (toupper(*p1) == toupper(b)) {
                        p1++;
                        p2++;
                    }
                    else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;
        case 2: // a==a && a==?
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                }
                else {
                    if (b == *p1 || (b=='?')) {
                        p1++;
                        p2++;
                    }
                    else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;

        default: // a==A && a==?
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                }
                else {
                    if (toupper(*p1) == toupper(b) || (b=='?')) {
                        p1++;
                        p2++;
                    }
                    else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;
    }
    return 0;
}

bool GBS_string_matches(const char *str, const char *search, GB_CASE case_sens)
/* Wildcards in 'search' string:
 *      ?   one character
 *      *   several characters
 *
 * if 'case_sens' == GB_IGNORE_CASE -> change all letters to uppercase
 *
 * returns true if strings are equal, false otherwise
 */
{
    const char *p1, *p2;
    char        a, b, *d;
    long        i;
    char        fsbuf[256];

    p1 = str;
    p2 = search;
    while (1) {
        a = *p1;
        b = *p2;
        if (b == '*') {
            if (!p2[1]) break; // '*' also matches nothing
            i = 0;
            d = fsbuf;
            for (p2++; (b=*p2)&&(b!='*');) {
                *(d++) = b;
                p2++;
                i++;
                if (i > 250) break;
            }
            if (*p2 != '*') {
                p1 += strlen(p1)-i;     // check the end of the string
                if (p1 < str) return false;
                p2 -= i;
            }
            else {
                *d  = 0;
                p1  = GBS_find_string(p1, fsbuf, 2+(case_sens == GB_IGNORE_CASE)); // match with '?' wildcard
                if (!p1) return false;
                p1 += i;
            }
            continue;
        }

        if (!a) return !b;
        if (a != b) {
            if (b != '?') {
                if (!b) return !a;
                if (case_sens == GB_IGNORE_CASE) {
                    a = toupper(a);
                    b = toupper(b);
                    if (a != b) return false;
                }
                else {
                    return false;
                }
            }
        }
        p1++;
        p2++;
    }
    return true;
}

bool GBS_string_matches_regexp(const char *str, const GBS_string_matcher *expr) {
    /* Wildcard or regular expression match
     * Returns true if match
     *
     * Use GBS_compile_matcher() and GBS_free_matcher() to maintain 'expr'
     */
    bool matches = false;

    switch (expr->type) {
        case SM_ANY: {
            matches = true;
            break;
        }
        case SM_WILDCARDED: {
            matches = GBS_string_matches(str, expr->wildexpr, expr->case_flag);
            break;
        }
        case SM_REGEXPR: {
            matches = GBS_regmatch_compiled(str, expr->regexpr, NULL) != NULL;
            break;
        }
        case SM_INVALID: {
            gb_assert(0);
            break;
        }
    }

    return matches;
}

// -----------------------------------
//      Search replace tool (SRT)

#define GBS_SET   ((char)1)
#define GBS_SEP   ((char)2)
#define GBS_MWILD ((char)3)
#define GBS_WILD  ((char)4)

int GBS_reference_not_found;

ATTRIBUTED(__ATTR__USERESULT,
           static GB_ERROR gbs_build_replace_string(GBS_strstruct *strstruct,
                                                    char *bar, char *wildcards, long max_wildcard,
                                                    char **mwildcards, long max_mwildcard, GBDATA *gb_container))
{
    char *p, c, d;
    int   wildcardcnt  = 0;
    int   mwildcardcnt = 0;
    int   wildcard_num;
    char *entry;

    p = bar;

    wildcardcnt  = 0;
    mwildcardcnt = 0;

    p = bar;
    while ((c=*(p++))) {
        switch (c) {
            case GBS_MWILD: case GBS_WILD:
                d = *(p++);
                if (gb_container && (d=='(')) { // if a gbcont then replace till ')'
                    GBDATA *gb_entry;
                    char    *klz;
                    char    *psym;
                    klz = gbs_search_second_bracket(p);
                    if (klz) {          // reference found: $(gbd)
                        int separator = 0;
                        *klz = 0;
                        psym = strpbrk(p, "#|:");
                        if (psym) {
                            separator = *psym;
                            *psym = 0;
                        }
                        if (*p) {
                            gb_entry = GB_search(gb_container, p, GB_FIND);
                        }
                        else {
                            gb_entry = gb_container;
                        }
                        if (psym) *psym = separator;

                        if (!gb_entry || gb_entry == gb_container) {
                            GBS_reference_not_found = 1;
                            entry = strdup("");
                        }
                        else {
                            entry = GB_read_as_string(gb_entry);
                        }
                        if (entry) {
                            char *h;
                            switch (separator) {
                                case ':':
                                    h = GBS_string_eval(entry, psym+1, gb_container);
                                    if (!h) return GB_await_error();

                                    GBS_strcat(strstruct, h);
                                    free(h);
                                    break;

                                case '|':
                                    h = GB_command_interpreter(GB_get_root(gb_container), entry, psym+1, gb_container, 0);
                                    if (!h) return GB_await_error();

                                    GBS_strcat(strstruct, h);
                                    free(h);
                                    break;

                                case '#':
                                    if (!gb_entry) {
                                        GBS_strcat(strstruct, psym+1);
                                        break;
                                    }
                                    // fall-through
                                default:
                                    GBS_strcat(strstruct, entry);
                                    break;
                            }
                            free(entry);
                        }
                        *klz = ')';
                        p = klz+1;
                        break;
                    }
                    c = '*';
                    GBS_chrcat(strstruct, c);
                    GBS_chrcat(strstruct, d);
                }
                else {
                    wildcard_num = d - '1';
                    if (c == GBS_WILD) {
                        c = '?';
                        if ((wildcard_num<0)||(wildcard_num>9)) {
                            p--;            // use this character
                            wildcard_num = wildcardcnt++;
                        }
                        if (wildcard_num>=max_wildcard) {
                            GBS_chrcat(strstruct, c);
                        }
                        else {
                            GBS_chrcat(strstruct, wildcards[wildcard_num]);
                        }
                    }
                    else {
                        c = '*';
                        if ((wildcard_num<0)||(wildcard_num>9)) {
                            p--;            // use this character
                            wildcard_num = mwildcardcnt++;
                        }
                        if (wildcard_num>=max_mwildcard) {
                            GBS_chrcat(strstruct, c);
                        }
                        else {
                            GBS_strcat(strstruct, mwildcards[wildcard_num]);
                        }
                    }
                }
                break;
            default:
                GBS_chrcat(strstruct, c);
                break;
        }
    }
    return 0;
}

static char *gbs_compress_command(const char *com) {
    /* Prepare SRT.
     *
     * Replaces all
     *   '=' by GBS_SET
     *   ':' by GBS_SEP
     *   '?' by GBS_WILD if followed by a number or '?'
     *   '*' by GBS_MWILD  or '('
     * \ is the escape character
     */
    char *result, *s, *d;
    int   ch;

    s = d = result = strdup(com);
    while ((ch = *(s++))) {
        switch (ch) {
            case '=':   *(d++) = GBS_SET; break;
            case ':':   *(d++) = GBS_SEP; break;
            case '?':
                ch = *s;
                *(d++) = GBS_WILD;
                break;
            case '*':
                ch = *s;
                *(d++) = GBS_MWILD;
                break;
            case '\\':
                ch = *(s++); if (!ch) { s--; break; };
                switch (ch) {
                    case 'n':   *(d++) = '\n'; break;
                    case 't':   *(d++) = '\t'; break;
                    case '0':   *(d++) = '\0'; break;
                    default:    *(d++) = ch; break;
                }
                break;
            default:
                *(d++) = ch;
        }
    }
    *d = 0;
    return result;
}


char *GBS_string_eval(const char *insource, const char *icommand, GBDATA *gb_container)
     /* GBS_string_eval replaces substrings in source
      * Syntax: command = "oliver=olli:peter=peti"
      *
      * Returns a heapcopy of result of replacement.
      *
      *         * is a wildcard for any number of character
      *         ? is a wildcard for exactly one character
      *
      * To reference to the wildcards on the left side of the '='
      * use ? and *, to reference in a different order use:
      *         *0 to reference to the first occurrence of *
      *         *1          second
      *         ...
      *         *9
      *
      * if the last and first characters of the search string are no '*' wildcards then
      * the replace is repeated as many times as possible
      * '\' is the escape character: e.g. \n is newline; '\\' is '\'; '\=' is '='; ....
      *
      * eg:
      * print first three characters of first word and the whole second word:
      *
      *         *(arb_key)          is the value of the a database entry arb key
      *         *(arb_key#string)   value of the database entry or 'string' if the entry does not exist
      *         *(arb_key\:SRT)     runs SRT recursively on the value of the database entry
      *         *([arb_key]|ACI)    runs the ACI command interpreter on the value of the database entry (or on an empty string)
      *
      * If an error occurs it returns NULL - in this case the error was exported.
      */
{
    GB_CSTR  source;            // pointer into the current string when parsed
    char    *search;            // pointer into the current command when parsed
    GB_CSTR  p;                 // short live pointer
    char     c;
    GB_CSTR  already_transferred; // point into 'in' string to non parsed position

    char      wildcard[40];
    char     *mwildcard[10];
    GB_ERROR  error;

    long i;
    long max_wildcard;
    long max_mwildcard;

    char *start_of_wildcard;
    char  what_wild_card;

    GB_CSTR start_match;

    char *doppelpunkt;

    char *bar;
    char *in;
    char *nextdp;
    GBS_strstruct *strstruct;
    char *command;

    if (!icommand || !icommand[0]) return strdup(insource);

    command = gbs_compress_command(icommand);
    in = strdup(insource);               // copy insource to allow to destroy it

    for (doppelpunkt = command; doppelpunkt; doppelpunkt = nextdp) {    // loop over command string
        // in is in , strstruct is out
        max_wildcard = 0;
        max_mwildcard = 0;
        nextdp = strchr(doppelpunkt, GBS_SEP);
        if (nextdp) {
            *(nextdp++) = 0;
        }
        if (!doppelpunkt[0]) {                      // empty command -> search next
            continue;
        }

        bar = strchr(doppelpunkt+1, GBS_SET);               // Parse the command string !!!!
        if (bar) {
            *(bar++) = 0;
        }
        else {
            GB_export_errorf("SRT ERROR: no '=' found in command '%s' (position > %zi)", icommand, doppelpunkt-command+1);
            free(command);
            free(in);
            return 0;
        }

        already_transferred = in;
        strstruct = GBS_stropen(1000);          // create output stream

        if ((!*in) && doppelpunkt[0] == GBS_MWILD && doppelpunkt[1] == 0) {     // empty string -> pars myself
            // * matches empty string !!!!
            mwildcard[max_mwildcard++] = strdup("");
            gbs_build_replace_string(strstruct, bar, wildcard, max_wildcard, mwildcard, max_mwildcard, gb_container);
            goto gbs_pars_unsuccessfull;    // successfull search
        }

        for (source = in; *source;) {               // loop over string
            search = doppelpunkt;

            start_match = 0;                // match string for '*'
            while ((c = *(search++))) {             // search matching command
                switch (c) {
                    case GBS_MWILD:
                        if (!start_match) start_match = source;

                        start_of_wildcard = search;
                        if (!(c = *(search++))) {       // last character is a wildcard -> that was it
                            mwildcard[max_mwildcard++] = strdup(source);
                            source += strlen(source);
                            goto gbs_pars_successfull;      // successfull search and end wildcard
                        }
                        while ((c=*(search++)) && c!=GBS_MWILD && c!=GBS_WILD) ;     // search the next wildcardstring
                        search--;                   // back one character
                        *search = 0;
                        what_wild_card = c;
                        p = GBS_find_string(source, start_of_wildcard, 0);
                        if (!p) {                   // string not found -> unsuccessful search
                            goto gbs_pars_unsuccessfull;
                        }
                        c = *p;                     // set wildcard
                        mwildcard[max_mwildcard++] = GB_strpartdup(source, p-1);
                        source = p + strlen(start_of_wildcard);         // we parsed it
                        *search = what_wild_card;
                        break;

                    case GBS_WILD:
                        if (!source[0]) {
                            goto gbs_pars_unsuccessfull;
                        }
                        if (!start_match) start_match = source;
                        wildcard[max_wildcard++] = *(source++);
                        break;
                    default:
                        if (start_match) {
                            if (c != *(source++)) {
                                goto gbs_pars_unsuccessfull;
                            }
                            break;
                        }
                        else {
                            char *buf1;
                            buf1 = search-1;
                            while ((c=*(search++)) && c!=GBS_MWILD && c!=GBS_WILD) ;     // search the next wildcardstring
                            search--;                   // back one character
                            *search = 0;
                            what_wild_card = c;
                            p = GBS_find_string(source, buf1, 0);
                            if (!p) {                   // string not found -> unsuccessful search
                                goto gbs_pars_unsuccessfull;
                            }
                            start_match = p;
                            source = p + strlen(buf1);          // we parsed it
                            *search = what_wild_card;
                        }
                        break;
                }
            }

        gbs_pars_successfull :
            /* now we got
             * source:                  pointer to end of match
             * start_match:             pointer to start of match
             * in:                      pointer to the entire string
             * already_transferred:     pointer to the start of the unparsed string
             * bar:                     the replace string
             */

            // now look for the replace string
            GBS_strncat(strstruct, already_transferred, start_match-already_transferred); // cat old data
            error               = gbs_build_replace_string(strstruct, bar, wildcard, max_wildcard, // do the command
                                             mwildcard, max_mwildcard, gb_container);
            already_transferred = source;

            for (i = 0; i < max_mwildcard; i++) {
                freenull(mwildcard[i]);
            }
            max_wildcard  = 0;
            max_mwildcard = 0;

            if (error) {
                free(GBS_strclose(strstruct));
                free(command);
                free(in);
                GB_export_error(error);
                return 0;
            }
        }
    gbs_pars_unsuccessfull :
        GBS_strcat(strstruct, already_transferred); // cat the rest data

        for (i = 0; i < max_mwildcard; i++) {
            freenull(mwildcard[i]);
        }
        max_wildcard  = 0;
        max_mwildcard = 0;

        freeset(in, GBS_strclose(strstruct));
    }
    free(command);
    return in;
}

