/* Program to extract function declarations from C source code
 * Written by Eric R. Smith and placed in the public domain
 * Thanks to:
 * Jwahar R. Bammi, for catching several bugs and providing the Unix makefiles
 * Byron T. Jenings Jr. for cleaning up the space code, providing a Unix
 *   manual page, and some ANSI and C++ improvements.
 * Skip Gilbrech for code to skip parameter names in prototypes.
 * ... and many others for helpful comments and suggestions.
 */

/* if your compiler claims to be ANSI but doesn't have stddef.h or stdlib.h,
 * change the next line.
 * (and then complain to the supplier of the defective compiler)
 */

/*
 * many extension were made for use in ARB build process
 * by Ralf Westram <ralf@arb-home.de>
*/

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS  0
#define EXIT_FAILURE  1
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>

static void Version(void);


#define check_heap_sanity() do{ char *x = malloc(10); free(x); }while(0)

/* #define DEBUG_PRINT(s) do{ fputs((s), stderr); check_heap_sanity(); }while(0) */
/* #define DEBUG_PRINT(s) fputs((s), stderr) */
#define DEBUG_PRINT(s)

#define PRINT(s) fputs((s), stdout)

#define ISCSYM(x) ((x) > 0 && (isalnum(x) || (x) == '_'))
#define ABORTED ( (Word *) -1 )
#define MAXPARAM 20         /* max. number of parameters to a function */
#define NEWBUFSIZ (20480*sizeof(char)) /* new buffer size */

static int  dostatic            = 0;   /* include static functions? */
static int  doinline            = 0;   /* include inline functions? */
static int  donum               = 0;   /* print line numbers? */
static int  define_macro        = 1;   /* define macro for prototypes? */
static int  use_macro           = 1;   /* use a macro for prototypes? */
static int  no_parm_names       = 0;   /* no parm names - only types */
static int  print_extern        = 0;   /* use "extern" before function declarations */
static int  dont_promote        = 0;   /* don't promote prototypes */
static int  promote_lines       = 0;   /* promote 'AISC_MKPT_PROMOTE'-lines */
static int  aisc                = 0;   /* aisc compatible output */
static int  cansibycplus        = 0;   /* produce extern "C" */
static int  promote_extern_c    = 0;   /* produce extern "C" into prototype */
static int  extern_c_seen       = 0;   /* true if extern "C" was parsed */
static int  search__attribute__ = 0;   /* search for gnu-extension __attribute__(()) ? */
static int  inquote             = 0;   /* in a quote?? */
static int  newline_seen        = 1;   /* are we at the start of a line */
static long linenum             = 1L;  /* line number in current file */
static int  glastc              = ' '; /* last char. seen by getsym() */

static char const *macro_name = "P_";  /*   macro to use for prototypes */
static char const *ourname;            /* our name, from argv[] array */

/* char *sym_part = 0;*/        /* create only prototypes starting with 'sym_start' */
/* int sym_part_len = 0; */

typedef struct sym_part {
    char *part;
    int len; /* strlen(part) */
    struct sym_part *next;
} SymPart;

SymPart *symParts = 0; /* create only prototypes for parts in this list */

static void addSymParts(const char *parts) {
    char *p = strdup(parts);
    const char *sep = ",";
    char *s = strtok(p, sep);

    while (s) {
        SymPart *sp = malloc(sizeof(*sp));

        sp->part = strdup(s);
        sp->len = strlen(s);
        sp->next = symParts;

        symParts = sp;

        s = strtok(0, sep);
    }

    free(p);
}

static int containsSymPart(const char *name) {
    SymPart *sp = symParts;
    int contains = 0;

    while (sp && !contains) {
        contains = strstr(name, sp->part)!=0;
        sp = sp->next;
    }

    return contains;
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

/* #include "mkptypes.h" */

/*
 * Routines for manipulating lists of words.
 */

static Word *word_alloc(const char *s)
{
    Word *w;

    /* note that sizeof(Word) already contains space for a terminating null
     * however, we add 1 so that typefixhack can promote "float" to "double"
     *  by just doing a strcpy.
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

/* return the length of a list; empty words are not counted */
static int List_len(Word *w)
{
    int count = 0;

    while (w) {
        if (*w->string) count++;
        w = w->next;
    }
    return count;
}

/* Append two lists, and return the result */

static Word *word_append(Word *w1, Word *w2){
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

static int foundin(Word *w1, Word *w2){
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

static void addword(Word *w, const char *s){
    while (w->next) w = w->next;
    w->next = word_alloc(s);

    DEBUG_PRINT("addword: '");
    DEBUG_PRINT(s);
    DEBUG_PRINT("'\n");
}

/* typefixhack: promote formal parameters of type "char", "unsigned char",
   "short", or "unsigned short" to "int".
*/

static void typefixhack(Word *w){
    Word *oldw = 0;

    if (dont_promote)
        return;

    while (w) {
        if (*w->string) {
            if ((strcmp(w->string, "char")==0 || strcmp(w->string, "short")==0) && (List_len(w->next) < 2) ) {
                /* delete any "unsigned" specifier present -- yes, it's supposed to do this */
                if (oldw && strcmp(oldw->string, "unsigned")==0) {
                    oldw->next = w->next;
                    free((char *)w);
                    w = oldw;
                }
                strcpy(w->string, "int");
            }
            else if (strcmp(w->string, "float")==0 && List_len(w->next) < 2 ) {
                strcpy(w->string, "double");
            }
        }
        w = w->next;
    }
}

/* read a character: if it's a newline, increment the line count */

static int ngetc(FILE *f){
    int c;

    c = getc(f);
    if (c == '\n') linenum++;

    return c;
}

#define MAX_COMMENT_SIZE 10000

static char  last_comment[MAX_COMMENT_SIZE];
static int   lc_size            = 0;
static char *found__attribute__ = 0;

static void clear_found_attribute() {
    free(found__attribute__);
    found__attribute__ = 0;
}

static void search_comment_for_attribute() {
    char *att;

    if (found__attribute__) return; // only take first __attribute__

    last_comment[lc_size] = 0;  // close string
    att                   = strstr(last_comment, "__attribute__");

    if (att != 0) {
        char *a  = att+13;
        int   parens = 1;

        while (*a && *a != '(') ++a; // search '('
        if (*a++ == '(')  {   // if '(' found
            while (parens) {
                switch (*a++) {
                    case '(': parens++; break;
                    case ')': parens--; break;
                }
            }
            *a = 0;
            DEBUG_PRINT("attribute found!\n");
            found__attribute__ = strdup(att);
        }
    }
}

struct promotion;
struct promotion {
    char             *to_promote; // text to promote to header
    struct promotion *next;
};

static struct promotion *promotions = 0;

static void add_promotion(char *to_promote) {
    struct promotion *new_promotion = malloc(sizeof(struct promotion));
    new_promotion->to_promote       = to_promote;
    new_promotion->next             = 0;

    if (!promotions) {
        promotions = new_promotion;
    }
    else { // append
        struct promotion *last = promotions;
        while (last->next) last = last->next;

        last->next = new_promotion;
    }
}

static void print_promotions() {
    struct promotion *p = promotions;

    if (promotions) fputc('\n', stdout);

    while (p) {
        struct promotion *next = p->next;

        printf("%s\n", p->to_promote);
        free(p->to_promote);
        free(p);

        p = next;
    }

    if (promotions) fputc('\n', stdout);
    promotions = 0;
}

static void search_comment_for_promotion() {
    char       *promotion_found;
    const char *promotion_tag     = "AISC_MKPT_PROMOTE:";
    int         promotion_tag_len = 18;

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

        assert(eol);
        if (!eol) {
            promotion_found = 0;
        }
        else {
            int   promo_length = eol-behind_promo;
            char *to_promote   = malloc(promo_length+1);

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
 */


static int fnextch(FILE *f){
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
                    return c;
                }
            }
            if (search__attribute__) search_comment_for_attribute();
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
                else if (c < 0) {
                    return c;
                }
            }
            if (search__attribute__) search_comment_for_attribute();
            if (promote_lines) search_comment_for_promotion();
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

static int nextch(FILE *f){
    int c, n;
    char *p, numbuf[10];

    c = fnextch(f);

    /* skip preprocessor directives */
    /* EXCEPTION: #line nnn or #nnn lines are interpreted */

    if (newline_seen && c == '#') {
        /* skip blanks */
        do {
            c = fnextch(f);
        } while ( c >= 0 && (c == '\t' || c == ' ') );
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
    skip_rest_of_line:
        while (c >= 0 && c != '\n')
            c = fnextch(f);
        if (c < 0)
            return c;
    }
    newline_seen = (c == '\n');

    if (c == '\'' || c == '\"') {
        char buffer[11];
        int index = 0;

        DEBUG_PRINT("nextch: in a quote\n");
        inquote = c;
        while ( (c = fnextch(f)) >= 0 ) {
            if (c == inquote) {
                DEBUG_PRINT("nextch: out of quote\n");
                buffer[index] = 0;

                DEBUG_PRINT("buffer='");
                DEBUG_PRINT(buffer);
                DEBUG_PRINT("'\n");

                if (inquote=='\"' && strcmp(buffer, "C")==0) {
                    inquote = 0;
                    return '$';
                }
                inquote = 0;
                return '0';
            }
            else  {
                if (index<10) buffer[index++] = c;
            }
        }
        DEBUG_PRINT("nextch: EOF in a quote\n");
    }
    return c;
}

/*
 * Get the next symbol from the file, skipping blanks.
 * Return 0 if OK, -1 for EOF.
 * Also collapses everything between { and }
 */

static int getsym(char *buf, FILE *f){
    register int c;
    int inbrack = 0;

    DEBUG_PRINT("in getsym\n");
    c = glastc;
    while ((c > 0) && isspace(c)) {
        c = nextch(f);
    }
    DEBUG_PRINT("getsym: spaces skipped\n");
    if (c < 0) {
        DEBUG_PRINT("EOF read in getsym\n");
        return -1;
    }
    if (c == '{') {
        inbrack = 1;
        DEBUG_PRINT("getsym: in bracket\n");
        while (inbrack) {
            c = nextch(f);
            if (c < 0) {
                DEBUG_PRINT("getsym: EOF seen in bracket loop\n");
                glastc = c;
                return c;
            }
            if (c == '{') inbrack++;
            else if (c == '}') inbrack--;
        }
        strcpy(buf, "{}");
        glastc = nextch(f);
        DEBUG_PRINT("getsym: out of in bracket loop\n");
        return 0;
    }
    if (!ISCSYM(c)) {
        *buf++ = c;
        *buf = 0;
        glastc = nextch(f);
        DEBUG_PRINT("getsym: returning special symbol\n");
        return 0;
    }
    while (ISCSYM(c)) {
        *buf++ = c;
        c = nextch(f);
    }
    *buf = 0;
    glastc = c;
    DEBUG_PRINT("getsym: returning word '");
    DEBUG_PRINT(buf);
    DEBUG_PRINT("'\n");
    return 0;
}

/*
 * skipit: skip until a ";" or the end of a function declaration is seen
 */
int skipit(char *buf, FILE *f){
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

/*
 * find most common type specifiers for purpose of ruling them out as
 * parm names
 */

static int is_type_word(char *s){
    static const char *typewords[] = {
        "char",     "const",    "double",   "enum",
        "float",    "int",      "long",     "short",
        "signed",   "struct",   "union",    "unsigned",
        "void",     "volatile", (char *)0
    };

    register const char **ss;

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


/*
 * given a list representing a type and a variable name, extract just
 * the base type, e.g. "struct word *x" would yield "struct word"
 */

static Word *typelist(Word *p){
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

/*
 * Get a parameter list; when this is called the next symbol in line
 * should be the first thing in the list.
 */

static Word *getparamlist(FILE *f){
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
    for(;;) {
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
            }
#endif

            if (i < np - 1) addword(tlist, ",");
        }
    }

    /* debugging output */
#if 0
    printf("/* ");
    tlist = plist;
    while (tlist) {
        printf("'%s', ", tlist->string);
        tlist = tlist->next;
    }
    printf(" */\n");
#endif

    return plist;
}

/*
 * emit a function declaration. The attributes and name of the function
 * are in wlist; the parameters are in plist.
 */

static void emit(Word *wlist, Word *plist, long startline){
    Word *w;
    int   count     = 0;
    int   needspace = 0;
    int   isstatic  = 0;
    int   refs      = 0 ;

    if (promote_lines) print_promotions();

    for (w = wlist; w; w = w->next) {
        if (w->string[0]) {
            count ++;
            if (strcmp(w->string, "static")==0) isstatic = 1;
        }
    }

    if (aisc) {
        if (count < 2) {
            printf("int\t");
            w = wlist;
        } else {
            refs = 0;
            for (w = wlist; w; w = w->next) {
                if (w->string[0]) {
                    printf("%s,\t", w->string);
                    w = w->next;
                    break;
                }
            }
        }
        for (;w; w = w->next) {
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
            printf("\tlink%i,\t{\n", refs);
            refs = 0;
        }else{
            printf("\tterm,\t{\n");
        }
        refs = 0;
        printf("\t@TYPE,\t@IDENT,\t@REF;\n");
        if (strcmp(plist->string, "void")) { /* if parameter is not 'void' */
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
                    } else {
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
            } else {
                printf("\tterm;\n");
            }
        }
        printf("};\n\n");
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

    if (found__attribute__) {
        PRINT(" ");
        PRINT(found__attribute__);
    }
    PRINT(";\n");
}

/*
 * get all the function declarations
 */

static void getdecl(FILE *f){
    Word *plist, *wlist = NULL;
    char buf[80];
    int sawsomething;
    long startline;     /* line where declaration started */
    int oktoprint;
 again:
    DEBUG_PRINT("again\n");
    word_free(wlist);
    wlist = word_alloc("");
    sawsomething = 0;
    oktoprint = 1;
    extern_c_seen = 0;

    for(;;) {
        DEBUG_PRINT("main getdecl loop\n");
        if (getsym(buf,f) < 0) {
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

        /* for the benefit of compilers that allow "inline" declarations */
        /* if (strcmp(buf, "inline") == 0 && !sawsomething) continue; */
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

                for (w=wlist; w->next && oktoprint; w=w->next)  {
                    if (w->string[0]==':' && w->string[1]==0) oktoprint = 0; /* do not emit prototypes for member functions */
                }

                if (oktoprint && symParts && !containsSymPart(w->string)) { /* name does not contain sym_part */
                    oktoprint = 0; /* => do not emit prototype */
                }
            }

            if (oktoprint) {
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

static void Usage(void){
    fprintf(stderr, "Usage: %s [flags] [files ...]\n", ourname);

    fputs("Supported flags:\n",stderr);
    fputs("   -a: make a funcion list for aisc_includes\n",stderr);
    fputs("   -e: put an explicit \"extern\" keyword in declarations\n", stderr);
    fputs("   -n: put line numbers of declarations as comments\n",stderr);
    fputs("   -p sym: use \"sym\" as the prototype macro (default \"P_\")\n", stderr);
    fputs("   -s: include declarations for static functions\n", stderr);
    fputs("   -i: include declarations for inline functions\n", stderr);
    fputs("   -x: omit parameter names in prototypes\n", stderr);
    fputs("   -z: omit prototype macro definition\n", stderr);
    fputs("   -A: omit prototype macro; header files are strict ANSI\n", stderr);
    fputs("   -V: print version number\n", stderr);
    fputs("   -W: don't promote types in old style declarations\n", stderr);
    fputs("   -F sym_part[,sym_part]*: create prototypes only for function-names containing one of the sym_parts\n", stderr);
    fputs("   -C: insert 'extern \"C\"'\n", stderr);
    fputs("   -E: promote 'extern \"C\"' to prototype\n", stderr);
    fputs("   -g: search for GNU extension __attribute__ in comment behind function header\n", stderr);
    fputs("   -P: promote /*AISC_MKPT_PROMOTE:forHeader*/ to header\n", stderr);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv){
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
            else if (*t == 'n')         donum               = 1;
            else if (*t == 's')         dostatic            = 1;
            else if (*t == 'i')         doinline            = 1;
            else if (*t == 'x')         no_parm_names       = 1; /* no parm names, only types (sg) */
            else if (*t == 'z')         define_macro        = 0;
            else if (*t == 'P')         promote_lines       = 1;
            else if (*t == 'p') {
                t = *argv++; --argc;
                if (!t) Usage();
                use_macro = 1;
                macro_name = t;
                break;
            }
            else if (*t == 'F') {
                t = *argv++; --argc;
                if (!t) Usage();
                addSymParts(t);
/*                 sym_part = t; */
/*                 sym_part_len = strlen(sym_part); */
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

    if (argc == 0 && exit_if_noargs) {
        exit(EXIT_FAILURE);
    }

    if (aisc) {
        printf("# *********************************************************\n");
        printf("# This file is generated by aisc_mkpt.\n");
        printf("# Any changes you make here will be overwritten later!\n");
        printf("# *********************************************************\n");
        printf("\n");
        printf("@FUNCTION_TYPE, @FUNCTION, @FUNCTION_REF;");
    }
    else {
        printf("/*\n");
        printf(" * This file is generated by aisc_mkpt.\n");
        printf(" * Any changes you make here will be overwritten later!\n");
        printf(" *\n");
        printf(" */\n\n");

        if (use_macro) {
            if (define_macro) {
                printf("#ifndef %s\n",macro_name);
                printf("# if defined(__STDC__) || defined(__cplusplus)\n");
                printf("#  define %s(s) s\n", macro_name);
                printf("# else\n");
                printf("#  define %s(s) ()\n", macro_name);
                printf("# endif\n");
                printf("#else\n");
                printf("# error %s already defined elsewhere\n", macro_name);
                printf("#endif\n\n");
            }
            else {
                printf("#ifndef %s\n",macro_name);
                printf("# error %s is not defined\n", macro_name);
                printf("#endif\n\n");
            }
        }
        if (search__attribute__) {
            printf("/* hide GNU extensions for non-gnu compilers: */\n");
            printf("#ifndef GNU\n");
            printf("# ifndef __attribute__\n");
            printf("#  define __attribute__(x)\n");
            printf("# endif\n");
            printf("#endif\n\n");
        }
        if (cansibycplus) {
            printf("#ifdef __cplusplus\n");
            printf("extern \"C\" {\n");
            printf("#endif\n\n");
        }
    }
    if (argc == 0) {
        getdecl(stdin);
    }
    else {

        while (argc > 0 && *argv) {
            DEBUG_PRINT("trying a new file\n");
            if (!(f = fopen(*argv, "r"))) {
                perror(*argv);
                exit(EXIT_FAILURE);
            }
            if (iobuf)
                setvbuf(f, iobuf, _IOFBF, NEWBUFSIZ);
            if (aisc)
                printf("\n#%s\n", *argv);
            else
                printf("\n/* %s */\n", *argv);
            linenum = 1;
            newline_seen = 1;
            glastc = ' ';
            DEBUG_PRINT("calling getdecl\n");
            getdecl(f);
            DEBUG_PRINT("back from getdecl\n");
            argc--; argv++;
            fclose(f);
            DEBUG_PRINT("back from fclose\n");
        }
    }
    if (aisc) {
    }
    else {
        if (cansibycplus) {
            printf("\n#ifdef __cplusplus\n");
            printf("}\n");
            printf("#endif\n");
        }
        if (use_macro && define_macro) {
            printf("\n#undef %s\n", macro_name);    /* clean up namespace */
        }
    }

    freeSymParts();

    return 0;
}


#include "patchlev.h"

static void Version(void) {
    fprintf(stderr, "%s 1.0 patchlevel %d\n", ourname, PATCHLEVEL);
}
