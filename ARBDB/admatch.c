/* ============================================================= */
/*                                                               */
/*   File      : admatch.c                                       */
/*   Purpose   : functions related to string match/replace       */
/*                                                               */
/*   Institute of Microbiology (Technical University Munich)     */
/*   http://www.arb-home.de/                                     */
/*                                                               */
/* ============================================================= */

#include "adlocal.h"

#include <ctype.h>
#include <stdlib.h>


void gbs_regerror(int en);

#ifdef NO_REGEXPR
int regerrno;
#   define INIT      char *sp = instring;
#   define GETC()    (*sp++)
#   define PEEKC()   (*sp)
#   define UNGETC(c) (--sp)
#   define RETURN(c) return c;
#   define ERROR(c)  gbs_regerror(c)
#   include "adregexp.h"
#else
#   include <regexpr.h>
#endif

#define GBS_SET   ((char)1)
#define GBS_SEP   ((char)2)
#define GBS_MWILD ((char)3)
#define GBS_WILD  ((char)4)

/* search a substring in another string
    match_mode == 0     -> exact match
    match_mode == 1     -> a==A 
    match_mode == 2     -> a==a && a==?
    match_mode == else  -> a==A && a==?
*/

GB_CSTR GBS_find_string(GB_CSTR str, GB_CSTR substr, int match_mode) {
    const char *p1, *p2;
    char        b;
    
    switch (match_mode) {

        case 0: /* exact match */
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (b == *p1) {
                        p1++;
                        p2++;
                    } else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2)   return (char *)str;
            break;

        case 1: /* a==A */
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (toupper(*p1) == toupper(b)) {
                        p1++;
                        p2++;
                    } else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;
        case 2: /* a==a && a==? */
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (b == *p1 || (b=='?')) {
                        p1++;
                        p2++;
                    } else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;

        default: /* a==A && a==? */
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (toupper(*p1) == toupper(b) || (b=='?') ) {
                        p1++;
                        p2++;
                    } else {
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

GB_BOOL GBS_string_matches_regexp(const char *str,const char *search,GB_CASE case_sens) {
    /* same as GBS_string_matches except '/regexpr/' matches regexpr  */
    if (search[0] == '/') {
        if (search[strlen(search)-1] == '/'){
#if defined(DEVEL_RALF)
#warning check case-handling in GBS_regsearch
#endif /* DEVEL_RALF */
            return GBS_regsearch(str,search) ? GB_TRUE : GB_FALSE;
        }
    }
    return GBS_string_matches(str,search,case_sens);
}

GB_BOOL GBS_string_matches(const char *str, const char *search, GB_CASE case_sens)
/* Wildcards in 'search' string:
 *      ?   one character
 *      *   serveral characters
 *
 * if 'case_sens' == GB_IGNORE_CASE -> change all letters to uppercase
 * 
 * returns GB_TRUE if strings are equal, GB_FALSE otherwise
 */
{
    const char *p1,*p2;
    char        a,b,*d;
    long        i;
    char        fsbuf[256];

    p1 = str;
    p2 = search;
    while (1) {
        a = *p1;
        b = *p2;
        if (b == '*')   {
            if (!p2[1]) break; /* '*' also matches nothing */
            i = 0;
            d = fsbuf;
            for (p2++; (b=*p2)&&(b!='*'); ) {
                *(d++) = b;
                p2++;
                i++;
                if (i > 250) break;
            }
            if (*p2 != '*' ) {
                p1 += strlen(p1)-i;     /* check the end of the string */
                if (p1 < str) return GB_FALSE;
                p2 -= i;
            }
            else {
                *d  = 0;
                p1  = GBS_find_string(p1,fsbuf,2+(case_sens == GB_IGNORE_CASE)); // match with '?' wildcard
                if (!p1) return GB_FALSE;
                p1 += i;
            }
            continue;
        }

        if (!a) return b ? GB_FALSE : GB_TRUE;
        if (a != b) {
            if (b != '?') {
                if (!b) return a ? GB_FALSE : GB_TRUE;
                if (case_sens == GB_IGNORE_CASE) {
                    a = toupper(a);
                    b = toupper(b);
                    if (a != b) return GB_FALSE;
                }
                else {
                    return GB_FALSE;
                }
            }
        }
        p1++;
        p2++;
    }
    return GB_TRUE;
}

/********************************************************************************************
                    String Replace
********************************************************************************************/

char *gbs_compress_command(const char *com) /* replaces all '=' by GBS_SET
                                               ':' by GBS_SEP
                                               '?' by GBS_WILD if followed by a number or '?'
                                               '*' by GBS_MWILD  or '('
                                               \ is the escape charakter
                                            */
{
    char *result,*s,*d;
    int   ch;

    s = d = result = strdup(com);
    while ( (ch = *(s++)) ){
        switch (ch) {
            case '=':   *(d++) = GBS_SET;break;
            case ':':   *(d++) = GBS_SEP;break;
            case '?':
                ch = *s;
                /*if ( (ch>='0' && ch <='9') || ch=='?'){   *(d++) = GBS_WILD;break;}*/
                *(d++) = GBS_WILD; break;
            case '*':
                ch = *s;
                /* if ( (ch>='0' && ch <='9') || ch=='('){  *(d++) = GBS_MWILD;break;}*/
                *(d++) = GBS_MWILD; break;
            case '\\':
                ch = *(s++); if (!ch) { s--; break; };
                switch (ch) {
                    case 'n':   *(d++) = '\n';break;
                    case 't':   *(d++) = '\t';break;
                    case '0':   *(d++) = '\0';break;
                    default:    *(d++) = ch;break;
                }
                break;
            default:
                *(d++) = ch;
        }
    }
    *d = 0;
    return result;
}

/********************************************************************************************
                    String Replace
********************************************************************************************/
int GBS_reference_not_found;

ATTRIBUTED(__ATTR__USERESULT,
           static GB_ERROR gbs_build_replace_string(void *strstruct,
                                                    char *bar,char *wildcards, long max_wildcard,
                                                    char **mwildcards, long max_mwildcard, GBDATA *gb_container))
{
    char *p,c,d;
    int   wildcardcnt  = 0;
    int   mwildcardcnt = 0;
    int   wildcard_num;
    char *entry;

    p = bar;

    wildcardcnt  = 0;
    mwildcardcnt = 0;

    p = bar;
    while ( (c=*(p++)) ) {
        switch (c){
            case GBS_MWILD: case GBS_WILD:
                d = *(p++);
                if (gb_container && (d=='(')) { /* if a gbcont then replace till ')' */
                    GBDATA *gb_entry;
                    char    *klz;
                    char    *psym;
                    klz = gbs_search_second_bracket(p);
                    if (klz) {          /* reference found: $(gbd) */
                        int seperator = 0;
                        *klz = 0;
                        psym = strpbrk(p,"#|:");
                        if (psym) {
                            seperator = *psym;
                            *psym =0;
                        }
                        if (*p){
                            gb_entry = GB_search(gb_container,p,GB_FIND);
                        }else{
                            gb_entry = gb_container;
                        }
                        if (psym) *psym = seperator;

                        if (!gb_entry || gb_entry == gb_container) {
                            GBS_reference_not_found = 1;
                            entry = strdup("");
                        }else{
                            entry = GB_read_as_string(gb_entry);
                        }
                        if (entry) {
                            char *h;
                            switch(seperator) {
                                case ':':
                                    h = GBS_string_eval(entry,psym+1,gb_container);
                                    if (h){
                                        GBS_strcat(strstruct,h);
                                        free(h);
                                    }else{
                                        return GB_get_error();
                                    }
                                    break;
                                case '|':
                                    h = GB_command_interpreter(GB_get_root(gb_container), entry,psym+1,gb_container, 0);
                                    if (h) {
                                        GBS_strcat(strstruct,h);
                                        free(h);
                                    }
                                    else {
                                        return GB_get_error();
                                    }
                                    break;
                                case '#':
                                    if (!gb_entry){
                                        GBS_strcat(strstruct,psym+1);
                                        break;
                                    }
                                default:
                                    GBS_strcat(strstruct,entry);
                                    break;
                            }
                            free(entry);
                        }
                        *klz = ')';
                        p = klz+1;
                        break;
                    }
                    c = '*';
                    GBS_chrcat(strstruct,c);
                    GBS_chrcat(strstruct,d);
                }
                else {
                    wildcard_num = d - '1';
                    if (c == GBS_WILD) {
                        c = '?';
                        if ( (wildcard_num<0)||(wildcard_num>9) ) {
                            p--;            /* use this character */
                            wildcard_num = wildcardcnt++;
                        }
                        if (wildcard_num>=max_wildcard) {
                            GBS_chrcat(strstruct,c);
                        }
                        else {
                            GBS_chrcat(strstruct,wildcards[wildcard_num]);
                        }
                    }
                    else {
                        c = '*';
                        if ( (wildcard_num<0)||(wildcard_num>9) ) {
                            p--;            /* use this character */
                            wildcard_num = mwildcardcnt++;
                        }
                        if (wildcard_num>=max_mwildcard) {
                            GBS_chrcat(strstruct,c);
                        }else{
                            GBS_strcat(strstruct,mwildcards[wildcard_num]);
                        }
                    }
                }
                break;
            default:
                GBS_chrcat(strstruct,c);
                break;
        }       /* switch c */
    }
    return 0;
}

char *GBS_string_eval(const char *insource, const char *icommand, GBDATA *gb_container)
     /* GBS_string_eval replaces substrings in source
        Syntax: command = "oliver=olli:peter=peti"
        The string is GB_STRDUPped !
        *   is a wildcard for any number of character
        ?   is a wildcard for exactly one character

        To reference to the wildcards on the left side of the '='
        use ? and *, to reference in a different order use:
            *0 to reference to the first occurrence of *
            *1          second
            ...
            *9

        if the last and first characters of the search string are no '*' wildcards then
        the replace is repeated as many times as possible
        '\' is the escape character: e.g. \n is newline; '\\' is '\'; '\=' is '='; ....

        eg:
        print first three characters of first word and the whole second word:

        *(arb_key)          is the value of the a database entry arb key
        *(arb_key#string)   value of the database entry or 'string' if the entry does not exist
        *(arb_key\:SRT)     runs SRT recursively on the value of the database entry
        *([arb_key]|ACI)    runs the ACI command interpreter on the value of the database entry (or on an empty string)

     */

{
    GB_CSTR  source;            /* pointer into the current string when parsed */
    char    *search;            /* pointer into the current command when parsed */
    GB_CSTR  p;                 /* short live pointer */
    char     c;
    GB_CSTR  already_transferred; /* point into 'in' string to non parsed position */

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
    void *strstruct;
    char *command;

    if (!icommand || !icommand[0]) return strdup(insource);

    command = gbs_compress_command(icommand);
    in = strdup(insource);               /* copy insource to allow to destroy it */

    for (doppelpunkt = command; doppelpunkt; doppelpunkt = nextdp) {    /* loop over command string */
        /* in is in , strstruct is out */
        max_wildcard = 0;
        max_mwildcard = 0;
        nextdp = strchr(doppelpunkt, GBS_SEP);
        if (nextdp){
            *(nextdp++) = 0;
        }
        if (!doppelpunkt[0]) {                      /* empty command -> search next */
            continue;
        }

        bar = strchr(doppelpunkt+1, GBS_SET);               /* Parse the command string !!!! */
        if (bar) {
            *(bar++) = 0;
        } else {
            GB_export_error("SRT ERROR: no '=' found in command '%s' (position > %zi)", icommand, doppelpunkt-command+1);
            free(command);
            free(in);
            return 0;
        }

        already_transferred = in;
        strstruct = GBS_stropen(1000);          /* create output stream */

        if ( (!*in) && doppelpunkt[0] == GBS_MWILD && doppelpunkt[1] == 0) {    /* empty string -> pars myself */
            /* * matches empty string !!!!  */
            mwildcard[max_mwildcard++] = strdup("");
            gbs_build_replace_string(strstruct,bar,wildcard, max_wildcard, mwildcard, max_mwildcard,gb_container);
            goto gbs_pars_unsuccessfull;    /* successfull search*/
        }

        for (source = in;*source; ) {               /* loop over string */
            search = doppelpunkt;

            start_match = 0;                /* match string for '*' */
            while ( (c = *(search++))  ) {          /* search matching command */
                switch (c) {
                    case GBS_MWILD:
                        if (!start_match) start_match = source;

                        start_of_wildcard = search;
                        if ( !(c = *(search++) ) ) {    /* last character is a wildcard -> that was it */
                            mwildcard[max_mwildcard++] = strdup(source);
                            source += strlen(source);
                            goto gbs_pars_successfull;      /* successfull search and end wildcard*/
                        }
                        while ( (c=*(search++)) && c!=GBS_MWILD && c!=GBS_WILD );   /* search the next wildcardstring */
                        search--;                   /* back one character */
                        *search = 0;
                        what_wild_card = c;
                        p = GBS_find_string(source,start_of_wildcard,0);
                        if (!p){                    /* string not found -> unsuccessful search */
                            goto gbs_pars_unsuccessfull;
                        }
                        c = *p;                     /* set wildcard */
                        mwildcard[max_mwildcard++] = GB_strpartdup(source, p-1);
                        source = p + strlen(start_of_wildcard);         /* we parsed it */
                        *search = what_wild_card;
                        break;

                    case GBS_WILD:
                        if ( !source[0] ) {
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
                        }else{
                            char *buf1;
                            buf1 = search-1;
                            while ( (c=*(search++)) && c!=GBS_MWILD && c!=GBS_WILD );   /* search the next wildcardstring */
                            search--;                   /* back one character */
                            *search = 0;
                            what_wild_card = c;
                            p = GBS_find_string(source,buf1,0);
                            if (!p){                    /* string not found -> unsuccessful search */
                                goto gbs_pars_unsuccessfull;
                            }
                            start_match = p;
                            source = p + strlen(buf1);          /* we parsed it */
                            *search = what_wild_card;
                        }
                        break;
                } /* switch */
            } /* while */           /* search matching command */

        gbs_pars_successfull:   /* now we got   source: pointer to end of match
                                   start_match: pointer to start of match
                                   in:  pointer to the entire string
                                   already_transferred: pointer to the start of the unparsed string
                                   bar: the replace string
                                */
            /* now look for the replace string */
            GBS_strncat(strstruct,already_transferred,start_match-already_transferred); /* cat old data */
            error = gbs_build_replace_string(strstruct,bar,wildcard, max_wildcard,      /* do the command */
                                             mwildcard, max_mwildcard,gb_container);
            already_transferred = source;

            for (i = 0; i < max_mwildcard; i++) {
                freeset(mwildcard[i], 0);
            }
            max_wildcard  = 0;
            max_mwildcard = 0;

            if (error) {
                free(GBS_strclose(strstruct));
                free(command);
                free(in);
                GB_export_error("%s",error);
                return 0;
            }
        }       /* while parsing */
    gbs_pars_unsuccessfull:
        GBS_strcat(strstruct,already_transferred);  /* cat the rest data */

        for (i = 0; i < max_mwildcard; i++){
            freeset(mwildcard[i], 0);
        }
        max_wildcard  = 0;
        max_mwildcard = 0;

        freeset(in, GBS_strclose(strstruct));
    }
    free(command);
    return in;
}

/************************************************************
 *      Regular Expressions string/substition
 ************************************************************/

void gbs_regerror(int en){
    regerrno = en;
    switch(regerrno){
        case 11:    GB_export_error("Range endpoint too large.");break;
        case 16:    GB_export_error("Bad number.");break;
        case 25:    GB_export_error("``\\digit'' out of range.");break;
        case 36:    GB_export_error("Illegal or missing delimiter");break;
        case 41:    GB_export_error("No remembered search string");break;
        case 42:    GB_export_error("(~) imbalance.");break;
        case 43:    GB_export_error("Too many (");break;
        case 44:    GB_export_error("More than 2 numbers given in {~}");break;
        case 45:    GB_export_error("} expected after \\");break;
        case 46:    GB_export_error("First number exceeds second in {~}");break;
        case 49:    GB_export_error("[ ] imbalance");break;
    }
}


/****** expects '/term/by/' *******/

#if defined(DEVEL_RALF)
#warning rewrite regexpr code using GNU regexpressions
/*
 * see http://www.gnu.org/software/hello/manual/libc/Regular-Expressions.html
 * use better error handling as well (separate error and result)
 */
#endif /* DEVEL_RALF */


#ifdef NO_REGEXPR
/** regexpr = '/regexpr/' */


GB_CSTR GBS_regsearch(GB_CSTR in, const char *regexprin){
    /* search the beginning first match
     * returns the position or NULL
     */
    static char  expbuf[8000];
    static char *regexpr  = 0;
    static int   lastRlen = -1;
    char        *res;
    int          rlen     = strlen(regexprin)-2;

    if (regexprin[0] != '/' || regexprin[rlen+1] != '/') {
        GB_export_error("RegExprSyntax: '/searchterm/'");
        GB_print_error();
        return 0;
    }

    if (!regexpr || lastRlen != rlen || strncmp(regexpr, regexprin+1, rlen) != 0) { // first or new regexpr
        freedup(regexpr, regexprin+1);
        regexpr[rlen] = 0;
        regerrno      = 0;
        lastRlen      = rlen;

        res = compile(regexpr, &expbuf[0], &expbuf[8000], 0);
        if (!res|| regerrno) {
            gbs_regerror(regerrno);
            return 0;
        }
    }

    if (step((char *)in,expbuf)) return loc1;
    return 0;
}

char *GBS_regreplace(const char *in, const char *regexprin, GBDATA *gb_species){
    static char  expbuf[8000];
    char        *regexpr;
    char        *subs;
    void        *out;
    char        *res;
    const char  *loc;
    int          rlen = strlen(regexprin)-2;
    GBUSE(gb_species);
    if (regexprin[0] != '/' || regexprin[rlen+1] != '/') {
        GB_export_error("RegExprSyntax: '/searchterm/replace/'");
        return 0;
    }
    /* Copy regexpr and remove leading + trailing '/' */
    regexpr = strdup(regexprin+1);
    regexpr[rlen] = 0;

    /* Search seperating '/' */
    subs                                                    = strrchr(regexpr,'/');
    while (subs && subs > regexpr && subs[-1] == '\\') subs = strrchr(subs,'/');
    if (!subs || subs == regexpr){
        free(regexpr);

        /* dont change this error message (or change it in adquery/GB_command_interpreter too) : */
        GB_export_error("no '/' found in regexpr");
        return 0;
    }
    *(subs++) = 0;
    regerrno = 0;
    res = compile(regexpr,expbuf,&expbuf[8000],0);
    if (!res || regerrno){
        gbs_regerror(regerrno);
        free(regexpr);
        return 0;
    }
    loc = in;
    out = GBS_stropen(10000);
    while ( step((char *)loc,expbuf)){      /* found a match */
        char *s;
        int c;
        GBS_strncat(out,loc,loc1-loc);  /* prefix */
        s = subs;
        while ( (c = *(s++)) ){
            if (c== '\\'){
                c = *(s++);
                if (!c) { s--; break; }
                switch (c){
                    case 'n': c = '\n'; break;
                    case 't': c = '\t'; break;
                    default: break;
                }
            }
            GBS_chrcat(out,c);
        }
        loc = loc2;
    }
    GBS_strcat(out,loc);        /* copy rest */
    free ( regexpr);
    return GBS_strclose(out);
}

#else

#error sorry.. got no system with regexp.h to fix code below -- ralf May 2008

/** regexpr = '/regexpr/' */
static GB_CSTR gb_compile_regexpr(GB_CSTR regexprin,char **subsout){
    static char *expbuf = 0;
    static char *old_reg_expr = 0;
    char *regexpr;
    char *subs;

    int rlen = strlen(regexprin)-2;
    if (regexprin[0] != '/' || regexprin[rlen+1] != '/') {
        GB_export_error("RegExprSyntax: '/searchterm/replace/'");
        return 0;
    }

    /* Copy regexpr and remove leading + trailing '/' */
    regexpr = GB_STRDUP(regexprin+1);
    regexpr[rlen] = 0;
    if (subsout){
        /* Search seperating '/' */
        subs = strrchr(regexpr,'/');
        *subsout = 0;
        while (subs && subs > regexpr && subs[-1] == '\\') subs = strrchr(subs,'/');
        if (!subs || subs == regexpr){
            free(regexpr);
            GB_export_error("no '/' found in regexpr");
            return 0;
        }
        *(subs++) = 0;
        *subsout = subs;
    }

    regerrno = 0;
    if (!old_reg_expr || strcmp(old_reg_expr ,regexpr) != 0) {
        freeset(expbuf, compile(regexpr,0,0));
    }

    reassign(old_reg_expr, regexpr);

    if (regerrno){
        gbs_regerror(regerrno);
        return 0;
    }
    return expbuf;
}

GB_CSTR GBS_regsearch(GB_CSTR in, GB_CSTR regexprin){
    char *expbuf;

    expbuf = gb_compile_regexpr(regexprin,0);
    if (!expbuf) return 0;
    if (step((char *)in,expbuf)) return loc1;
    return 0;
}

char *GBS_regreplace(const char *in, const char *regexprin, GBDATA *gb_species){
    char *expbuf = 0;
    char *subs;
    void *out;
    const char *loc;
    GBUSE(gb_species);
    expbuf = gb_compile_regexpr(regexprin,&subs);
    if (!expbuf) return 0;

    loc = in;
    out = GBS_stropen(10000);
    while ( step((char *)loc,expbuf)){      /* found a match */
        char *s;
        int c;
        GBS_strncat(out,loc,loc1-loc);  /* prefix */
        s = subs;
        while (c = *(s++)){
            if (c== '\\'){
                c = *(s++);
                if (!c) { s--; break; }
                switch (c){
                    case 'n': c = '\n'; break;
                    case 't': c = '\t'; break;
                    default: break;
                }
            }else if (c=='$'){
                c = *(s++);
                if (!c) { s--; break; }
                if (c>='0' && c <'0' + nbra){
                    c -= '0';
                    GBS_strncat(out,braslist[c], braelist[c] - braslist[c]);
                    continue;
                }

            }
            GBS_chrcat(out,c);
        }
        loc = loc2;
    }
    GBS_strcat(out,loc);        /* copy rest */
    return GBS_strclose(out);
}

#endif

char *GBS_regmatch(const char *in, const char *regexprin) {
    /* returns the first match */
    GB_CSTR found = GBS_regsearch(in, regexprin);
    if (found) {
        int   length   = loc2-loc1;
        char *result   = (char*)malloc(length+1);
        memcpy(result, loc1, length);
        result[length] = 0;

        return result;
    }
    return 0;
}

