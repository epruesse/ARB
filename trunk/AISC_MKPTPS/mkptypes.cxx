/* Program to extract function declarations from C source code
 * Written by Eric R. Smith and placed in the public domain
 * Thanks to:
 * Jwahar R. Bammi, for catching several bugs and providing the Unix makefiles
 * Byron T. Jenings Jr. for cleaning up the space code, providing a Unix
 *   manual page, and some ANSI and C++ improvements.
 * Skip Gilbrech for code to skip parameter names in prototypes.
 * ... and many others for helpful comments and suggestions.
 */

/* many extension were made for use in ARB build process
 * by Ralf Westram <ralf@arb-home.de>
*/

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS  0
#define EXIT_FAILURE  1
#endif

static void Version(void);


#define check_heap_sanity() do { char *x = malloc(10); free(x); } while (0)

#if defined(DEBUG)
/* #define DEBUG_PRINTS */
#endif /* DEBUG */

#ifdef DEBUG_PRINTS 
#define DEBUG_PRINT(s) fputs((s), stderr)
#else
#define DEBUG_PRINT(s)
#endif

#define PRINT(s) fputs((s), stdout)

#define ISCSYM(x) ((x) > 0 && (isalnum(x) || (x) == '_'))
#define ABORTED ((Word *) -1)
#define MAXPARAM 20         /* max. number of parameters to a function */
#define NEWBUFSIZ (20480*sizeof(char)) /* new buffer size */


static int dostatic            = 0;                 /* include static functions? */
static int doinline            = 0;                 /* include inline functions? */
static int donum               = 0;                 /* print line numbers? */
static int define_macro        = 1;                 /* define macro for prototypes? */
static int use_macro           = 1;                 /* use a macro for prototypes? */
static int use_main            = 0;                 /* add prototype for main? */
static int no_parm_names       = 0;                 /* no parm names - only types */
static int print_extern        = 0;                 /* use "extern" before function declarations */
static int dont_promote        = 0;                 /* don't promote prototypes */
static int promote_lines       = 0;                 /* promote 'AISC_MKPT_PROMOTE'-lines */
static int aisc                = 0;                 /* aisc compatible output */
static int cansibycplus        = 0;                 /* produce extern "C" */
static int promote_extern_c    = 0;                 /* produce extern "C" into prototype */
static int extern_c_seen       = 0;                 /* true if extern "C" was parsed */
static int search__attribute__ = 0;                 /* search for gnu-extension __attribute__(()) ? */
static int search__ATTR__      = 0;                 /* search for ARB-__attribute__-macros (__ATTR__) ? */

static const char *include_wrapper = NULL;          /* add include wrapper (contains name of header or NULL) */

static int inquote      = 0;                        /* in a quote?? */
static int newline_seen = 1;                        /* are we at the start of a line */
static int glastc       = ' ';                      /* last char. seen by getsym() */

static char *current_file   = 0;  /* name of current file */
static char *current_dir    = 0;  /* name of current directory */
static char *header_comment = 0;  /* comment written into header */
static long  linenum        = 1L; /* line number in current file */

static char const *macro_name = "P_";  /*   macro to use for prototypes */
static char const *ourname;            /* our name, from argv[] array */

/* ------------------------------------------------------------ */

static void errorAt(long line, const char *msg) {
    printf("\n"
           "#line %li \"%s/%s\"\n"
           "#error in aisc_mkpt: %s\n",
           line, 
           current_dir,
           current_file,
           msg);
}

static void error(const char *msg) {
    errorAt(linenum, msg);
}

struct SymPart {
    char *part;
    int   len;                                      // len of part
    bool  atStart;                                  // part has to match at start of name
    
    SymPart *next;
};

static SymPart *symParts = 0; /* create only prototypes for parts in this list */

static void addSymParts(const char *parts) {
    char *p = strdup(parts);
    const char *sep = ",";
    char *s = strtok(p, sep);

    while (s) {
        SymPart *sp = (SymPart*)malloc(sizeof(*sp));

        sp->atStart = s[0] == '^';
        sp->part    = strdup(sp->atStart ? s+1 : s);
        sp->len     = strlen(sp->part);
        sp->next    = symParts;

        symParts = sp;

        s = strtok(0, sep);
    }

    free(p);
}

static bool matchesSymPart(const char *name) {
    SymPart *sp       = symParts;
    bool     matches = 0;

    while (sp && !matches) {
        if (sp->atStart) {
            matches = strncmp(name, sp->part, sp->len) == 0;
        }
        else {
            matches = strstr(name, sp->part) != NULL;
        }
        sp = sp->next;
    }

    return matches;
}

static void freeSymParts() {
    SymPart *next = symParts;

    while (next) {
        SymPart *del = next;
        next = del->next;

        free(del->part);
        free(del);
    }
}

typedef struct word {
    struct word *next;
    char   string[1];
} Word;

// Routines for manipulating lists of words.

static Word *word_alloc(const char *s)
{
    Word *w;

    /* note that sizeof(Word) already contains space for a terminating null
     * however, we add 1 so that typefixhack can promote "float" to "double"
     * by just doing a strcpy.
     */
    w = (Word *) malloc(sizeof(Word) + strlen(s) + 1);
    strcpy(w->string, s);
    w->next = NULL;
    return w;
}

static void word_free(Word *w)
{
    Word *oldw;
    while (w) {
        oldw = w;
        w = w->next;
        free((char *)oldw);
    }
}

static int List_len(Word *w)
{
    // return the length of a list
    // empty words are not counted 
    int count = 0;

    while (w) {
        if (*w->string) count++;
        w = w->next;
    }
    return count;
}

/* Append two lists, and return the result */

static Word *word_append(Word *w1, Word *w2) {
    Word *r, *w;

    r = w = word_alloc("");

    while (w1) {
        w->next = word_alloc(w1->string);
        w = w->next;
        w1 = w1->next;
    }
    while (w2) {
        w->next = word_alloc(w2->string);
        w = w->next;
        w2 = w2->next;
    }

    return r;
}

/* see if the last entry in w2 is in w1 */

static int foundin(Word *w1, Word *w2) {
    while (w2->next)
        w2 = w2->next;

    while (w1) {
        if (strcmp(w1->string, w2->string)==0)
            return 1;
        w1 = w1->next;
    }
    return 0;
}

/* add the string s to the given list of words */

static void addword(Word *w, const char *s) {
    while (w->next) w = w->next;
    w->next = word_alloc(s);

    DEBUG_PRINT("addword: '");
    DEBUG_PRINT(s);
    DEBUG_PRINT("'\n");
}

/* typefixhack: promote formal parameters of type "char", "unsigned char",
   "short", or "unsigned short" to "int".
*/

static void typefixhack(Word *w) {
    Word *oldw = 0;

    if (dont_promote)
        return;

    while (w) {
        if (*w->string) {
            if ((strcmp(w->string, "char")==0 || strcmp(w->string, "short")==0) && (List_len(w->next) < 2)) {
                /* delete any "unsigned" specifier present -- yes, it's supposed to do this */
                if (oldw && strcmp(oldw->string, "unsigned")==0) {
                    oldw->next = w->next;
                    free((char *)w);
                    w = oldw;
                }
                strcpy(w->string, "int");
            }
            else if (strcmp(w->string, "float")==0 && List_len(w->next) < 2) {
                strcpy(w->string, "double");
            }
        }
        w = w->next;
    }
}

/* read a character: if it's a newline, increment the line count */

static int ngetc(FILE *f) {
    int c;

    c = getc(f);
    if (c == '\n') linenum++;

    return c;
}

#define MAX_COMMENT_SIZE 10000

static char  last_comment[MAX_COMMENT_SIZE];
static int   lc_size            = 0;
static char *found__attribute__ = 0;
static char *found__ATTR__      = 0;

static void clear_found_attribute() {
    free(found__attribute__); found__attribute__ = 0;
    free(found__ATTR__); found__ATTR__      = 0;
}

static void search_comment_for_attribute() {
    char *att;

    if (found__attribute__ || found__ATTR__) return; // only take first __attribute__

    last_comment[lc_size] = 0;  // close string
    
    att = strstr(last_comment, "__attribute__");
    if (att != 0) {
        char *a  = att+13;
        int   parens = 1;

        while (*a && *a != '(') ++a; // search '('
        if (*a++ == '(') {    // if '(' found
            while (parens && *a) {
                switch (*a++) {
                    case '(': parens++; break;
                    case ')': parens--; break;
                }
            }
            *a                 = 0;
            DEBUG_PRINT("__attribute__ found!\n");
            found__attribute__ = strdup(att);
            if (search__ATTR__) { error("found '__attribute__' but expected '__ATTR__..'"); }
        }
    }

    att = strstr(last_comment, "__ATTR__");
    if (att != 0) {
        char *a  = att+8;

        while (*a && (isalnum(*a) || *a == '_')) ++a; // goto end of name

        if (*a == '(') {
            int parens = 1;
            a++;
            
            while (parens && *a) {
                switch (*a++) {
                    case '(': parens++; break;
                    case ')': parens--; break;
                }
            }
            *a = 0;
            DEBUG_PRINT("__ATTR__ with parameters found!\n");
            found__ATTR__ = strdup(att);
        }
        else {
            *a = 0;
            DEBUG_PRINT("__ATTR__ w/o parameters found!\n");
            found__ATTR__ = strdup(att);
        }
        if (search__attribute__) { error("found '__ATTR__..' but expected '__attribute__'"); }
    }

    if (found__attribute__ && found__ATTR__) {
        error("Either specify __attribute__ or __ATTR__... - not both\n");
    }
}

struct promotion {
    char      *to_promote;                          // text to promote to header
    promotion *next;
};

static promotion *promotions = 0;

static void add_promotion(char *to_promote) {
    promotion *new_promotion = (promotion*)malloc(sizeof(promotion));
    new_promotion->to_promote       = to_promote;
    new_promotion->next             = 0;

    if (!promotions) {
        promotions = new_promotion;
    }
    else { // append
        promotion *last = promotions;
        while (last->next) last = last->next;

        last->next = new_promotion;
    }
}

static void print_promotions() {
    promotion *p = promotions;

    if (promotions) fputc('\n', stdout);

    while (p) {
        promotion *next = p->next;

        printf("%s\n", p->to_promote);
        free(p->to_promote);
        free(p);

        p = next;
    }

    if (promotions) fputc('\n', stdout);
    promotions = 0;
}

static const char *promotion_tag     = "AISC_MKPT_PROMOTE:";
static int         promotion_tag_len = 18;

static void search_comment_for_promotion() {
    char *promotion_found;
    last_comment[lc_size] = 0;  // close string

    promotion_found = strstr(last_comment, promotion_tag);
    while (promotion_found) {
        char *behind_promo = promotion_found+promotion_tag_len;
        char *eol, *eoc;
        assert(behind_promo[-1] == ':'); // wrong promotion_tag_len

        eol = strchr(behind_promo, '\n');
        eoc = strstr(behind_promo, "*/");

        if (eoc && eol) {
            if (eoc<eol) eol = eoc;
        }
        else if (!eol) {
            eol = eoc;
        }

        if (!eol) eol = strchr(behind_promo, 0);

        if (eol) {
            while (eol>behind_promo && eol[-1] == ' ') --eol; // trim spaces at eol
        }

        assert(eol);
        if (!eol) {
            promotion_found = 0;
        }
        else {
            int   promo_length = eol-behind_promo;
            char *to_promote   = (char*)malloc(promo_length+1);

            memcpy(to_promote, behind_promo, promo_length);
            to_promote[promo_length] = 0;

            DEBUG_PRINT("promotion found!\n");

            add_promotion(to_promote);
            promotion_found = strstr(eol, promotion_tag);
        }
    }
}

/* read the next character from the file. If the character is '\' then
 * read and skip the next character. Any comment sequence is converted
 * to a blank.
 *
 * if a comment contains __attribute__ and search__attribute__ != 0
 * the attribute string is stored in found__attribute__
 *
 * if a comment contains __ATTR__ and search__ATTR__ != 0
 * the attribute string is stored in found__ATTR__
 */


static int fnextch(FILE *f) {
    int c, lastc, incomment;

    c = ngetc(f);
    while (c == '\\') {
        DEBUG_PRINT("fnextch: in backslash loop\n");
        c = ngetc(f);   /* skip a character */
        c = ngetc(f);
    }
    if (c == '/' && !inquote) {
        c = ngetc(f);
        if (c == '*') {
            long commentStartLine = linenum;

            incomment = 1;
            c         = ' ';
            DEBUG_PRINT("fnextch: comment seen\n");
            lc_size   = 0;

            while (incomment) {
                lastc                   = c;
                c                       = ngetc(f);
                last_comment[lc_size++] = c;
                assert(lc_size<MAX_COMMENT_SIZE);

                if (lastc == '*' && c == '/') incomment = 0;
                else if (c < 0) {
                    error("EOF reached in comment");
                    errorAt(commentStartLine, "comment started here");
                    return c;
                }
            }
            if (search__attribute__ || search__ATTR__) search_comment_for_attribute();
            if (promote_lines) search_comment_for_promotion();
            return fnextch(f);
        }
        else if (c == '/') {    /* C++ style comment */
            incomment = 1;
            c         = ' ';
            DEBUG_PRINT("fnextch: C++ comment seen\n");
            lc_size   = 0;

            while (incomment) {
                lastc                   = c;
                c                       = ngetc(f);
                last_comment[lc_size++] = c;
                assert(lc_size<MAX_COMMENT_SIZE);

                if (lastc != '\\' && c == '\n') incomment = 0;
                else if (c < 0) break;
            }
            if (search__attribute__ || search__ATTR__) search_comment_for_attribute();
            if (promote_lines) search_comment_for_promotion();

            if (c == '\n') return c;
            return fnextch(f);
        }
        else {
            /* if we pre-fetched a linefeed, remember to adjust the line number */
            if (c == '\n') linenum--;
            ungetc(c, f);
            return '/';
        }
    }
    return c;
}


/* Get the next "interesting" character. Comments are skipped, and strings
 * are converted to "0". Also, if a line starts with "#" it is skipped.
 */

static int nextch(FILE *f) {
    int c, n;
    char *p, numbuf[10];

    c = fnextch(f);

    /* skip preprocessor directives */
    /* EXCEPTION: #line nnn or #nnn lines are interpreted */

    if (newline_seen && c == '#') {
        /* skip blanks */
        do {
            c = fnextch(f);
        } while (c >= 0 && (c == '\t' || c == ' '));
        /* check for #line */
        if (c == 'l') {
            c = fnextch(f);
            if (c != 'i')   /* not a #line directive */
                goto skip_rest_of_line;
            do {
                c = fnextch(f);
            } while (c >= 0 && c != '\n' && !isdigit(c));
        }

        /* if we have a digit it's a line number, from the preprocessor */
        if (c >= 0 && isdigit(c)) {
            p = numbuf;
            for (n = 8; n >= 0; --n) {
                *p++ = c;
                c = fnextch(f);
                if (c <= 0 || !isdigit(c))
                    break;
            }
            *p = 0;
            linenum = atol(numbuf) - 1;
        }

        /* skip the rest of the line */
    skip_rest_of_line :
        while (c >= 0 && c != '\n')
            c = fnextch(f);
        if (c < 0)
            return c;
    }
    newline_seen = (c == '\n');

    if (c == '\'' || c == '\"') {
        char buffer[11];
        int  index          = 0;
        long quoteStartLine = linenum;

        DEBUG_PRINT("nextch: in a quote\n");
        inquote = c;
        while ((c = fnextch(f)) >= 0) {
            if (c == inquote) {
                buffer[index] = 0;
                DEBUG_PRINT("quoted content='");
                DEBUG_PRINT(buffer);
                DEBUG_PRINT("'\n");

                DEBUG_PRINT("nextch: out of quote\n");

                if (linenum != quoteStartLine) {
                    error("multiline quotes");
                    errorAt(quoteStartLine, "quotes opened here");
                }

                if (inquote=='\"' && strcmp(buffer, "C")==0) {
                    inquote = 0;
                    return '$'; /* found "C" (needed for 'extern "C"') */
                }
                inquote = 0;
                return '0';
            }
            else {
                if (index<10) buffer[index++] = c;
            }
        }
        error("EOF in a quote");
        errorAt(quoteStartLine, "quote started here");
        DEBUG_PRINT("nextch: EOF in a quote\n");
    }
    return c;
}

static int getsym(char *buf, FILE *f) {
    /* Get the next symbol from the file, skipping blanks.
     * Return 0 if OK, -1 for EOF.
     * Also collapses everything between { and }
     */

    int c;
    int inbrack = 0;
    
#if defined(DEBUG_PRINTS)
    char *bufStart = buf;
#endif /* DEBUG_PRINTS */

    c = glastc;
    while ((c > 0) && isspace(c)) {
        c = nextch(f);
    }

    if (c < 0) {
        DEBUG_PRINT("EOF read in getsym\n");
        return -1;
    }

    if (c == '{') {
        long bracketStartLine = linenum;

        inbrack = 1;
        DEBUG_PRINT("getsym: in '{'\n");
        while (inbrack) {
            c = nextch(f);
            if (c < 0) {
                error("EOF seen in bracket loop (unbalanced brackets?)");
                errorAt(bracketStartLine, "bracket opened here");
                DEBUG_PRINT("getsym: EOF seen in bracket loop\n");
                glastc = c;
                return c;
            }
            if (c == '{') {
                inbrack++;
#if defined(DEBUG_PRINTS)
                fprintf(stderr, "inbrack=%i (line=%li)\n", inbrack, linenum);
#endif /* DEBUG_PRINTS */
            }
            else if (c == '}') {
                inbrack--;
#if defined(DEBUG_PRINTS)
                fprintf(stderr, "inbrack=%i (line=%li)\n", inbrack, linenum);
#endif /* DEBUG_PRINTS */
            }
        }
        strcpy(buf, "{}");
        glastc = nextch(f);
        DEBUG_PRINT("getsym: returning brackets '");
    }
    else if (!ISCSYM(c)) {
        *buf++ = c;
        *buf   = 0;
        glastc = nextch(f);

        DEBUG_PRINT("getsym: returning special symbol '");
    }
    else {
        while (ISCSYM(c)) {
            *buf++ = c;
            c = nextch(f);
        }
        *buf   = 0;
        glastc = c;
        DEBUG_PRINT("getsym: returning word '");
    }

    DEBUG_PRINT(bufStart);
    DEBUG_PRINT("'\n");
    
    return 0;
}

static int skipit(char *buf, FILE *f) {
    // skip until a ";" or the end of a function declaration is seen
    
    int i;
    do {
        DEBUG_PRINT("in skipit loop\n");

        i = getsym(buf, f);
        if (i < 0) return i;

        DEBUG_PRINT("skipit: '");
        DEBUG_PRINT(buf);
        DEBUG_PRINT("'\n");

    } while (*buf != ';' && *buf != '{');

    return 0;
}

static int is_type_word(char *s) {
    /* find most common type specifiers for purpose of ruling them out as
     * parm names
     */

    static const char *typewords[] = {
        "char",     "const",    "double",   "enum",
        "float",    "int",      "long",     "short",
        "signed",   "struct",   "union",    "unsigned",
        "void",     "volatile", (char *)0
    };

    const char **ss;

    for (ss = typewords; *ss; ++ss)
        if (strcmp(s, *ss) == 0)
            return 1;

    return 0;
}


/* Ad-hoc macro to recognize parameter name for purposes of removal.
 * Idea is to remove the bulk of easily recognized parm names without
 * losing too many type specifiers. (sg)
 */
#define IS_PARM_NAME(w) \
    (ISCSYM(*(w)->string) && !is_type_word((w)->string) && \
    (!(w)->next || *(w)->next->string == ',' || \
     *(w)->next->string == '['))


static Word *typelist(Word *p) {
    /* given a list representing a type and a variable name, extract just
     * the base type, e.g. "struct word *x" would yield "struct word"
     */

    Word *w, *r;

    r = w = word_alloc("");
    while (p && p->next) {
        /* handle int *x --> int */
        if (p->string[0] && !ISCSYM(p->string[0]))
            break;
        /* handle int x[] --> int */
        if (p->next->string[0] == '[')
            break;
        w->next = word_alloc(p->string);
        w = w->next;
        p = p->next;
    }
    return r;
}

static Word *getparamlist(FILE *f) {
    /* Get a parameter list; when this is called the next symbol in line
     * should be the first thing in the list.
     */

    static Word *pname[MAXPARAM]; /* parameter names */
    Word    *tlist,       /* type name */
        *plist;       /* temporary */
    int     np = 0;       /* number of parameters */
    int     typed[MAXPARAM];  /* parameter has been given a type */
    int tlistdone;    /* finished finding the type name */
    int sawsomething;
    int     i;
    int inparen = 0;
    char buf[80];

    DEBUG_PRINT("in getparamlist\n");
    for (i = 0; i < MAXPARAM; i++)
        typed[i] = 0;

    plist = word_alloc("");

    /* first, get the stuff inside brackets (if anything) */

    sawsomething = 0;   /* gets set nonzero when we see an arg */
    for (;;) {
        if (getsym(buf, f) < 0) return NULL;
        if (*buf == ')' && (--inparen < 0)) {
            if (sawsomething) { /* if we've seen an arg */
                pname[np] = plist;
                plist = word_alloc("");
                np++;
            }
            break;
        }
        if (*buf == ';') {  /* something weird */
            return ABORTED;
        }
        sawsomething = 1;   /* there's something in the arg. list */
        if (*buf == ',' && inparen == 0) {
            pname[np] = plist;
            plist = word_alloc("");
            np++;
        }
        else {
            addword(plist, buf);
            if (*buf == '(') inparen++;
        }
    }

    /* next, get the declarations after the function header */

    inparen = 0;

    tlist = word_alloc("");
    plist = word_alloc("");
    tlistdone = 0;
    sawsomething = 0;
    for (;;) {
        if (getsym(buf, f) < 0) return NULL;

        /* handle a list like "int x,y,z" */
        if (*buf == ',' && !inparen) {
            if (!sawsomething)
                return NULL;
            for (i = 0; i < np; i++) {
                if (!typed[i] && foundin(plist, pname[i])) {
                    typed[i] = 1;
                    word_free(pname[i]);
                    pname[i] = word_append(tlist, plist);
                    /* promote types */
                    typefixhack(pname[i]);
                    break;
                }
            }
            if (!tlistdone) {
                tlist = typelist(plist);
                tlistdone = 1;
            }
            word_free(plist);
            plist = word_alloc("");
        }
        /* handle the end of a list */
        else if (*buf == ';') {
            if (!sawsomething)
                return ABORTED;
            for (i = 0; i < np; i++) {
                if (!typed[i] && foundin(plist, pname[i])) {
                    typed[i] = 1;
                    word_free(pname[i]);
                    pname[i] = word_append(tlist, plist);
                    typefixhack(pname[i]);
                    break;
                }
            }
            tlistdone = 0;
            word_free(tlist); word_free(plist);
            tlist = word_alloc("");
            plist = word_alloc("");
        }
        /* handle the  beginning of the function */
        else if (strcmp(buf, "{}")==0) break;
        /* otherwise, throw the word into the list (except for "register") */
        else if (strcmp(buf, "register")) {
            addword(plist, buf);
            if (*buf == '(') inparen++;
            else if (*buf == ')') inparen--;
            else sawsomething = 1;
        }
    }

    /* Now take the info we have and build a prototype list */

    /* empty parameter list means "void" */
    if (np == 0) return word_alloc("void");

    plist = tlist = word_alloc("");

    /* how to handle parameters which contain only one word ?
     *
     * void -> void
     * xxx  -> int xxx        (if AUTO_INT is defined)
     * int  -> int dummyXX    (otherwise)
     */

    /* #define AUTO_INT */

    {
#if !defined(AUTO_INT)
        int dummy_counter = 1;
        char dummy_name[] = "dummy_xx";
#define DUMMY_COUNTER_POS 6
#endif

        for (i = 0; i < np; i++) {
#if !defined(AUTO_INT)
            int add_dummy_name = 0;
#endif
            {
                Word *pn_list = pname[i];
                int cnt = 0;
                int is_void = 0;

                while (pn_list) { /* count words */
                    if (pn_list->string[0]) {
                        ++cnt;
                        if (strcmp(pn_list->string, "void")==0) is_void = 1;
                    }
                    pn_list = pn_list->next;
                }
                if (cnt==1 && !is_void) { /* only name, but not void */
                    /* no type or no parameter name */
#ifdef AUTO_INT
                    /* If no type provided, make it an "int" */
                    addword(tlist, "int"); /* add int to tlist before adding pname[i] */
#else
                    add_dummy_name = 1; /* add a dummy name after adding pname[i] */
#endif
                }
            }

            while (tlist->next) tlist = tlist->next;
            tlist->next               = pname[i];

#if !defined(AUTO_INT)
            if (add_dummy_name) {
                assert(dummy_counter<100);
                dummy_name[DUMMY_COUNTER_POS] = (dummy_counter/10)+'0';
                dummy_name[DUMMY_COUNTER_POS] = (dummy_counter%10)+'0';
                addword(tlist, dummy_name);
                dummy_counter++;
            }
#endif

            if (i < np - 1) addword(tlist, ",");
        }
    }

    return plist;
}

static void emit(Word *wlist, Word *plist, long startline) {
    /* emit a function declaration. The attributes and name of the function
     * are in wlist; the parameters are in plist.
     */

    Word *w;
    int   count     = 0;
    int   needspace = 0;
    int   isstatic  = 0;
    int   ismain    = 0;
    int   refs      = 0;

    if (promote_lines) print_promotions();

    for (w = wlist; w; w = w->next) {
        if (w->string[0]) {
            count ++;
            if      (strcmp(w->string, "static") == 0) isstatic = 1;
            else if (strcmp(w->string, "main") == 0) ismain = 1;
        }
    }

    if (ismain && !use_main) return;

    if (aisc) {
        if (count < 2) {
            printf("int\t");
            w = wlist;
        }
        else {
            refs = 0;
            for (w = wlist; w; w = w->next) {
                if (w->string[0]) {
                    printf("%s,\t", w->string);
                    w = w->next;
                    break;
                }
            }
        }
        for (; w; w = w->next) {
            if (w->string[0] == '*') {
                refs++;
                continue;
            }
            if (w->string[0]) {
                printf("%s,", w->string);
                break;
            }
        }
        if (refs) {
            printf("\tlink%i", refs);
            refs = 0;
        }
        else {
            printf("\tterm");
        }

        refs = 0;

        if (strcmp(plist->string, "void") != 0) {    /* if parameter is not 'void' */
            printf(",\t{\n");
            printf("\t@TYPE,\t@IDENT,\t@REF;\n");

            int name_seen       = 0;
            int unnamed_counter = 1;
            for (w = plist; w; w = w->next) {
                if (w->string[0] == '*') {
                    refs++;
                    name_seen = 0;
                    continue;
                }
                if (w->string[0] == ',') {
                    if (refs) {
                        printf("\tlink%i;\n", refs);
                        refs = 0;
                        continue;
                    }
                    else {
                        printf("\tterm;\n");
                        continue;
                    }
                }
                if (w->string[0]) {
                    printf("\t%s,", w->string);
                    name_seen = 1;
                }
            }
            if (refs) {
                if (!name_seen) {
                    /* automatically insert missing parameter names */ 
                    printf("\tunnamed%i,", unnamed_counter++);
                }
                printf("\tlink%i;\n", refs);
                refs = 0;
            }
            else {
                printf("\tterm;\n");
            }
            printf("}");
        }
        printf(";\n\n");
        return;
    }
    DEBUG_PRINT("emit called\n");
    if (donum)
        printf("/*%8ld */ ", startline);


    /* if the -e flag was given, and it's not a static function, print "extern" */

    if (print_extern && !isstatic) {
        printf("extern ");
    }

    if (count < 2) {
        printf("int");
        needspace = 1;
    }

    for (w = wlist; w; w = w->next) {
        if (needspace)
            putchar(' ');
        printf("%s", w->string);
        needspace = ISCSYM(w->string[0]);
    }

    if (use_macro) printf(" %s((", macro_name);
    else putchar('(');

    needspace = 0;
    for (w = plist; w; w = w->next) {
        if (no_parm_names && IS_PARM_NAME(w))
            continue;
        if (w->string[0] == ',')
            needspace = 1;
        else if (w->string[0] == '[')
            needspace = 0;
        else
        {
            if (needspace)
                putchar(' ');
            needspace = ISCSYM(w->string[0]);
        }
        printf("%s", w->string);
    }

    if (use_macro)  PRINT("))");
    else            PRINT(")");

    if (found__attribute__) { PRINT(" "); PRINT(found__attribute__); }
    if (found__ATTR__) { PRINT(" "); PRINT(found__ATTR__); }
    
    PRINT(";\n");
}

static void getdecl(FILE *f, const char *header) {
    // parse all function declarations and print to STDOUT

    Word *plist, *wlist  = NULL;
    char  buf[80];
    int   sawsomething;
    long  startline;            /* line where declaration started */
    int   oktoprint;
    int   header_printed = 0;

    current_file = strdup(header);

 again :
    DEBUG_PRINT("again\n");
    word_free(wlist);
    wlist         = word_alloc("");
    sawsomething  = 0;
    oktoprint     = 1;
    extern_c_seen = 0;

    for (;;) {
        DEBUG_PRINT("main getdecl loop\n");
        if (getsym(buf, f) < 0) {
            DEBUG_PRINT("EOF in getdecl loop\n");
            return;
        }

        DEBUG_PRINT("getdecl: '");
        DEBUG_PRINT(buf);
        DEBUG_PRINT("'\n");

        /* try to guess when a declaration is not an external function definition */
        if (strcmp(buf, ",")==0 ||
            strcmp(buf, "=")==0 ||
            strcmp(buf, "typedef")==0 ||
            strcmp(buf, "[")==0)
        {
            skipit(buf, f);
            goto again;
        }

        if (strcmp(buf, "{}")==0) {
            if (!extern_c_seen) skipit(buf, f);
            goto again;
        }

        if (strcmp(buf, "extern")==0) {
            if (getsym(buf, f)<0) {
                DEBUG_PRINT("EOF in getdecl loop\n");
                return;
            }

            DEBUG_PRINT("test auf extern \"C\": '");
            DEBUG_PRINT(buf);
            DEBUG_PRINT("'\n");

            if (strcmp(buf, "$")==0) { /* symbol used if "C" was found */
                extern_c_seen = 1;
                if (promote_extern_c) {
                    addword(wlist, "extern");
                    addword(wlist, "\"C\" ");
                    sawsomething = 1;
                }
                continue;
            }

            skipit(buf, f);
            goto again;
        }

        if (oktoprint && !dostatic && strcmp(buf, "static")==0) {
            oktoprint = 0;
        }
        if (oktoprint && !doinline && strcmp(buf, "inline")==0) {
            oktoprint = 0;
        }

        if (strcmp(buf, ";")      == 0) goto again;

        /* A left parenthesis *might* indicate a function definition */
        if (strcmp(buf, "(")==0) {
            startline = linenum;
            if (!sawsomething || !(plist = getparamlist(f))) {
                skipit(buf, f);
                goto again;
            }
            if (plist == ABORTED)
                goto again;

            /* It seems to have been what we wanted */

            if (oktoprint) { /* check function-name */
                Word *w;

                for (w=wlist; w->next && oktoprint; w=w->next) {
                    if (w->string[0]==':' && w->string[1]==0) oktoprint = 0; /* do not emit prototypes for member functions */
                }

                if (oktoprint && symParts && !matchesSymPart(w->string)) { /* name does not contain sym_part */
                    oktoprint = 0; /* => do not emit prototype */
                }
            }

            if (oktoprint) {
                if (!header_printed) {
                    if (aisc) printf("\n# %s\n", header);
                    else printf("\n/* %s */\n", header);
                    header_printed = 1;
                }
                emit(wlist, plist, startline);
            }
            clear_found_attribute();

            word_free(plist);
            goto again;
        }

        addword(wlist, buf);
        sawsomething = 1;
    }
}

static void Usage(void) {
    fprintf(stderr, "Usage: %s [flags] [files ...]", ourname);
    fputs("\nSupported flags:"
          "\n   -a               make a function list for aisc_includes (default: generate C prototypes)"
          "\n"
          "\n   -e               put an explicit \"extern\" keyword in declarations"
          "\n"
          "\n   -n               put line numbers of declarations as comments" 
          "\n"
          "\n   -s               promote declarations for static functions"
          "\n   -i               promote declarations for inline functions"
          "\n   -m               promote declaration of 'main()' (default is to skip it)"
          "\n   -F part[,part]*  only promote declarations for functionnames containing one of the parts"
          "                      if 'part' starts with a '^' functionname has to start with rest of part\n"
          "\n"
          "\n   -W               don't promote types in old style declarations"
          "\n   -x               omit parameter names in prototypes"
          "\n"
          "\n   -p sym           use \"sym\" as the prototype macro (default \"P_\")" 
          "\n   -z               omit prototype macro definition"
          "\n   -A               omit prototype macro; header files are strict ANSI"
          "\n"
          "\n   -C               insert 'extern \"C\"'"
          "\n   -E               prefix 'extern \"C\"' at prototype"
          "\n"
          "\n   -g               search for GNU extension __attribute__ in comment behind function header"
          "\n   -G               search for ARB macro     __ATTR__      in comment behind function header"
          "\n"
          "\n   -P               promote /*AISC_MKPT_PROMOTE:forHeader*/ to header"
          "\n"
          "\n   -w               add standard include wrapper"
          "\n"
          "\n   -c \"text\"      add text as comment into header"
          "\n"
          "\n   -V               print version number"
          "\n"
          , stderr);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    FILE *f;
    char *t, *iobuf;
    int exit_if_noargs = 0;

    if (argv[0] && argv[0][0]) {

        ourname = strrchr(argv[0], '/');
        if (!ourname)
            ourname = argv[0];
    }
    else {
        ourname = "mkptypes";
    }

    argv++; argc--;

    iobuf = (char *)malloc(NEWBUFSIZ);
    while (*argv && **argv == '-') {
        t = *argv++; --argc; t++;
        while (*t) {
            if (*t == 'e')              print_extern        = 1;
            else if (*t == 'A')         use_macro           = 0;
            else if (*t == 'C')         cansibycplus        = 1;
            else if (*t == 'E')         promote_extern_c    = 1;
            else if (*t == 'W')         dont_promote        = 1;
            else if (*t == 'a')         aisc                = 1;
            else if (*t == 'g')         search__attribute__ = 1;
            else if (*t == 'G')         search__ATTR__      = 1;
            else if (*t == 'n')         donum               = 1;
            else if (*t == 's')         dostatic            = 1;
            else if (*t == 'i')         doinline            = 1;
            else if (*t == 'x')         no_parm_names       = 1; /* no parm names, only types (sg) */
            else if (*t == 'z')         define_macro        = 0;
            else if (*t == 'P')         promote_lines       = 1;
            else if (*t == 'm')         use_main            = 1;
            else if (*t == 'p') {
                t = *argv++; --argc;
                if (!t) Usage();
                use_macro = 1;
                macro_name = t;
                break;
            }
            else if (*t == 'c') {
                t = *argv++; --argc;
                if (!t) Usage();
                header_comment = t;
                break;
            }
            else if (*t == 'w') {
                t = *argv++; --argc;
                if (!t) Usage();
                include_wrapper = t;
                break;
            }
            else if (*t == 'F') {
                t = *argv++; --argc;
                if (!t) Usage();
                addSymParts(t);
                break;
            }
            else if (*t == 'V') {
                exit_if_noargs = 1;
                Version();
            }
            else Usage();
            t++;
        }
    }

    if (search__ATTR__ && search__attribute__) {
        fputs("Either use option -g or -G (not both)", stderr);
        exit(EXIT_FAILURE);
    }

    if (argc == 0 && exit_if_noargs) {
        exit(EXIT_FAILURE);
    }

    char *include_macro = 0;
    if (aisc) {
        if (header_comment) {
            printf("# %s\n#\n", header_comment);
        }
        fputs("# This file is generated by aisc_mkpt.\n"
              "# Any changes you make here will be overwritten later!\n"
              "\n"
              "@FUNCTION_TYPE, @FUNCTION, @FUNCTION_REF;", stdout);
    }
    else {
        fputs("/*", stdout);
        if (header_comment) printf(" %s.\n *\n *", header_comment);
        fputs(" This file is generated by aisc_mkpt.\n"
              " * Any changes you make here will be overwritten later!\n"
              " */\n"
              "\n"
              , stdout);

        if (include_wrapper) {
            int p;
            include_macro = strdup(include_wrapper);
            for (p = 0; include_macro[p]; p++) {
                char c           = include_macro[p];
                c                = strchr(".-", c) ? '_' : toupper(c);
                include_macro[p] = c;
            }

            printf("#ifndef %s\n"
                   "#define %s\n"
                   "\n", 
                   include_macro, 
                   include_macro);
        }

        if (use_macro) {
            if (define_macro) {
                fprintf(stdout,
                        "#ifndef %s\n"
                        "# if defined(__STDC__) || defined(__cplusplus)\n"
                        "#  define %s(s) s\n"
                        "# else\n"
                        "#  define %s(s) ()\n"
                        "# endif\n"
                        "#else\n"
                        "# error %s already defined elsewhere\n"
                        "#endif\n\n",
                        macro_name, macro_name, macro_name, macro_name);
            }
            else {
                fprintf(stdout,
                        "#ifndef %s\n"
                        "# error %s is not defined\n"
                        "#endif\n\n",
                        macro_name, macro_name);
            }
        }
        if (search__attribute__) {
            fputs("/* hide __attribute__'s for non-gcc compilers: */\n"
                  "#ifndef __GNUC__\n"
                  "# ifndef __attribute__\n"
                  "#  define __attribute__(x)\n"
                  "# endif\n"
                  "#endif\n\n", stdout);
        }
        if (search__ATTR__) {
            fputs("/* define ARB attributes: */\n"
                  "#ifndef ATTRIBUTES_H\n"
                  "# include <attributes.h>\n"
                  "#endif\n\n", stdout);
        }
        if (cansibycplus) {
            fputs("#ifdef __cplusplus\n"
                  "extern \"C\" {\n"
                  "#endif\n\n", stdout);
        }
    }
    
    current_dir = strdup(getcwd(0, 255));
    if (argc == 0) {
        getdecl(stdin, "<from stdin>");
    }
    else {
        
        while (argc > 0 && *argv) {
            DEBUG_PRINT("trying new file '");
            DEBUG_PRINT(*argv);
            DEBUG_PRINT("'\n");
            
            if (!(f = fopen(*argv, "r"))) {
                perror(*argv);
                exit(EXIT_FAILURE);
            }
            if (iobuf) setvbuf(f, iobuf, _IOFBF, NEWBUFSIZ);

            linenum      = 1;
            newline_seen = 1;
            glastc       = ' ';
            getdecl(f, *argv);
            argc--; argv++;
            fclose(f);

            free(current_file);
            current_file = 0;
        }
    }
    if (aisc) {
    }
    else {
        if (cansibycplus) {
            fputs("\n#ifdef __cplusplus\n"
                  "}\n"
                  "#endif\n", stdout);
        }
        if (use_macro && define_macro) {
            printf("\n#undef %s\n", macro_name);    /* clean up namespace */
        }

        if (include_wrapper) {
            printf("\n"
                   "#else\n"
                   "#error %s included twice\n"
                   "#endif /* %s */\n",
                   include_wrapper,
                   include_macro);
        }
    }

    free(include_macro);
    freeSymParts();
    free(current_file);
    free(current_dir);

    return EXIT_SUCCESS;
}

static void Version(void) {
    fprintf(stderr, "%s 1.1 ARB\n", ourname);
}
