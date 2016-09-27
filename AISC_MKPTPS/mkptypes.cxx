/* Program to extract function declarations from C/C++ source code
 *
 * Written by Eric R. Smith and placed in the public domain
 *
 * Thanks to:
 *
 * Jwahar R. Bammi, for catching several bugs and providing the Unix makefiles
 * Byron T. Jenings Jr. for cleaning up the space code, providing a Unix
 *   manual page, and some ANSI and C++ improvements.
 * Skip Gilbrech for code to skip parameter names in prototypes.
 * ... and many others for helpful comments and suggestions.
 *
 * Many extension were made for use in ARB build process
 * by Ralf Westram <ralf@arb-home.de>
 */

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <unistd.h>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <cstdarg>

#include <arb_early_check.h>
#include <attributes.h>

#include <arb_simple_assert.h>
#include <arbtools.h>

#define mp_assert(cond) arb_assert(cond)

static void Version();

#if defined(DEBUG)
// #define DEBUG_PRINTS
#endif // DEBUG

#ifdef DEBUG_PRINTS
#define DEBUG_PRINT(s)                  fputs((s), stderr)
#define DEBUG_PRINT_STRING(name, str)   fprintf(stderr, "%s'%s'\n", name, str)
#else
#define DEBUG_PRINT(s)
#define DEBUG_PRINT_STRING(name, str)
#endif

#define PRINT(s) fputs((s), stdout)

#define IS_CSYM(x) ((x) > 0 && (isalnum(x) || (x) == '_'))
#define ABORTED    ((Word *) -1)
#define MAXPARAM   20                               // max. number of parameters to a function
#define NEWBUFSIZ  (20480*sizeof(char))             // new buffer size


static int donum               = 0;                 // print line numbers?
static int define_macro        = 0;                 // define macro for K&R prototypes?
static int use_macro           = 0;                 // use a macro to support K&R prototypes?
static int use_main            = 0;                 // add prototype for main?
static int no_parm_names       = 0;                 // no parm names - only types
static int print_extern        = 0;                 // use "extern" before function declarations
static int dont_promote        = 0;                 // don't promote prototypes
static int promote_lines       = 0;                 // promote 'AISC_MKPT_PROMOTE'-lines
static int aisc                = 0;                 // aisc compatible output
static int cansibycplus        = 0;                 // produce extern "C"
static int promote_extern_c    = 0;                 // produce extern "C" into prototype
static int extern_c_seen       = 0;                 // true if extern "C" was parsed
static int search__ATTR__      = 0;                 // search for ARB-__attribute__-macros (__ATTR__) ?

static const char *include_wrapper = NULL;          // add include wrapper (contains name of header or NULL)

static int inquote      = 0;                        // in a quote??
static int newline_seen = 1;                        // are we at the start of a line
static int glastc       = ' ';                      // last char. seen by getsym()

static char       *current_file   = 0;              // name of current file
static char       *current_dir    = 0;              // name of current directory
static const char *header_comment = 0;              // comment written into header
static long        linenum        = 1L;             // line number in current file

static char const *macro_name = "P_";               //   macro to use for prototypes
static char const *ourname;                         // our name, from argv[] array

// ------------------------------------------------------------

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

static void errorf(const char *format, ...) __attribute__((format(__printf__, 1, 2)));
static void errorf(const char *format, ...) {
    const int BUFFERSIZE = 1000;
    char      buffer[BUFFERSIZE];
    va_list   args;

    va_start(args, format);
    int printed = vsprintf(buffer, format, args);
    if (printed >= BUFFERSIZE) {
        fputs("buffer overflow\n", stderr);
        exit(EXIT_FAILURE);
    }
    va_end(args);

    error(buffer);
}

// ------------------------------------------------------------

struct SymPart {
    char *part;
    int   len;                                      // len of part
    bool  atStart;                                  // part has to match at start of name

    SymPart *And;
    SymPart *next;
};

static SymPart* makeSymPart(char *token) {
    SymPart *sp = (SymPart*)malloc(sizeof(*sp));

    sp->atStart = token[0] == '^';
    char *part  = sp->atStart ? token+1 : token;
    char *plus  = strchr(part, '+');

    if (plus) {
        *plus++ = 0;
        sp->And = makeSymPart(plus);
    }
    else {
        sp->And = NULL;
    }
    sp->part = strdup(part);
    sp->len = strlen(sp->part);

    sp->next = NULL;

    return sp;
}

static void addSymParts(SymPart*& symParts, const char *parts) {
    char       *p   = strdup(parts);
    const char *sep = ",";
    char       *s   = strtok(p, sep);

    while (s) {
        SymPart *sp = makeSymPart(s);
        sp->next    = symParts;
        symParts    = sp;
        s           = strtok(0, sep);
    }

    free(p);
}

static bool matchesSymPart(const SymPart* symParts, const char *name) {
    const SymPart *sp      = symParts;
    bool           matches = false;

    while (sp && !matches) {
        if (sp->atStart) {
            matches = strncmp(name, sp->part, sp->len) == 0;
        }
        else {
            matches = strstr(name, sp->part) != NULL;
        }
        if (matches && sp->And) matches = matchesSymPart(sp->And, name);
        sp = sp->next;
    }

    return matches;
}

static void freeSymParts(SymPart*& symParts) {
    SymPart *next = symParts;

    while (next) {
        SymPart *del = next;
        next         = del->next;

        if (del->And) freeSymParts(del->And);
        free(del->part);
        free(del);
    }
    
    symParts = NULL;
}

// ----------------------------------------

static SymPart *requiredSymParts = 0; // only create prototypes if function-name matches one of these parts
static SymPart *excludedSymParts = 0; // DONT create prototypes if function-name matches one of these parts

inline void addRequiredSymParts(const char *parts) { addSymParts(requiredSymParts, parts); }
inline void addExcludedSymParts(const char *parts) { addSymParts(excludedSymParts, parts); }

inline void freeRequiredSymParts() { freeSymParts(requiredSymParts); }
inline void freeExcludedSymParts() { freeSymParts(excludedSymParts); }

inline bool hasRequiredSymPart(const char *name) { return !requiredSymParts || matchesSymPart(requiredSymParts, name); }
inline bool hasExcludedSymPart(const char *name) { return excludedSymParts  && matchesSymPart(excludedSymParts, name); }

inline bool wantPrototypeFor(const char *name) {
    return hasRequiredSymPart(name) && !hasExcludedSymPart(name);
}

// ----------------------------------------

struct Word {
    Word *next;
    char  string[1];
};

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

static void word_free(Word*& word) {
    Word *w = word;
    while (w) {
        Word *oldw = w;
        w = w->next;
        free(oldw);
    }
    word = NULL;
}

static int List_len(Word *w) {
    // return the length of a list
    // empty words are not counted
    int count = 0;

    while (w) {
        if (*w->string) count++;
        w = w->next;
    }
    return count;
}

static Word *word_append(Word *w1, Word *w2) {
    // Append two lists, and return the result

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

static int foundin(Word *w1, Word *w2) {
    // see if the last entry in w2 is in w1

    while (w2->next)
        w2 = w2->next;

    while (w1) {
        if (strcmp(w1->string, w2->string)==0)
            return 1;
        w1 = w1->next;
    }
    return 0;
}

static void addword(Word *w, const char *s) {
    // add the string s to the given list of words

    while (w->next) w = w->next;
    w->next = word_alloc(s);

    DEBUG_PRINT("addword: '");
    DEBUG_PRINT(s);
    DEBUG_PRINT("'\n");
}

static void typefixhack(Word *w) {
    // typefixhack: promote formal parameters of type "char", "unsigned char",
    // "short", or "unsigned short" to "int".

    Word *oldw = 0;

    if (dont_promote)
        return;

    while (w) {
        if (*w->string) {
            if ((strcmp(w->string, "char")==0 || strcmp(w->string, "short")==0) && (List_len(w->next) < 2)) {
                // delete any "unsigned" specifier present -- yes, it's supposed to do this
                if (oldw && strcmp(oldw->string, "unsigned")==0) {
                    oldw->next = w->next;
                    free(w);
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

static int ngetc(FILE *f) {
    // read a character: if it's a newline, increment the line count

    int c;

    c = getc(f);
    if (c == '\n') linenum++;

    return c;
}

#define MAX_COMMENT_SIZE 10000

static char  last_comment[MAX_COMMENT_SIZE];
static int   lc_size       = 0;
static char *found__ATTR__ = 0;

static void clear_found_attribute() {
    free(found__ATTR__);
    found__ATTR__ = 0;
}

static const char *nextNonSpace(const char* ptr) {
    while (isspace(*ptr)) ++ptr;
    return ptr;
}
static const char *nextNonWord(const char* ptr) {
    while (isalnum(*ptr) || *ptr == '_') ++ptr;
    return ptr;
}

inline const char *matchingParen(const char *from) {
    int open = 1;
    int i;
    for (i = 1; from[i] && open>0; ++i) {
        switch (from[i]) {
            case '(': open++; break;
            case ')': open--; break;
        }
    }
    if (open) return NULL; // no matching closing paren
    return from+i-1;
}

// -----------------
//      LinePart

class LinePart {
    const char *line;
    size_t      linelen; // of line

    size_t pos;
    size_t size;

    bool part_is_legal() { return (pos <= linelen) && ((pos+size) <= linelen); }

    void set(size_t newPos, size_t newSize) {
        pos  = newPos;
        size = newSize;
        mp_assert(part_is_legal());
    }
    
public:

    LinePart(const char *wholeLine, size_t len = -1U)
        : line(wholeLine),
          linelen(len != -1U ? len : strlen(line))
    {
        mp_assert(line[linelen] == 0);
        undefine();
    }

    size_t get_size() const { return size; }
    bool   is_empty() const { return !size; }

    const char *whole_line() const { return line; }

    void define(const char *start, const char *end) { set(start-line, end-start+1); }
    void undefine() { set(0, 0); }

    void copyTo(char *buffer) const {
        mp_assert(!is_empty());
        memcpy(buffer, line+pos, size);
        buffer[size] = 0;
    }

    LinePart behind() const {
        mp_assert(!is_empty());
        size_t behind_offset = pos+size;
        mp_assert(linelen >= behind_offset);
        return LinePart(line+behind_offset, linelen-behind_offset);
    }

    void error(const char *format) {
        // 'format' has to contain one '%s'
        mp_assert(!is_empty());
        char part[get_size()+1];
        copyTo(part);
        errorf(format, part);
        undefine();
    }
};


// ------------------
//      PartQueue

class PartQueue : virtual Noncopyable {
    LinePart   first;
    PartQueue *next;
    size_t     size;

    bool is_empty() const { return !size; }
    size_t get_size() const { return size; }

    void copyPartsTo(char *buffer) const {
        mp_assert(!first.is_empty());
        first.copyTo(buffer);
        buffer += first.get_size();
        if (next) {
            *buffer++ = ' ';
            next->copyPartsTo(buffer);
        }
    }

public:
    PartQueue(LinePart first_)
        : first(first_),
          next(0),
          size(first.get_size())
    {}
    ~PartQueue() { delete next; }

    void append(PartQueue *mp) {
        mp_assert(!next);
        next  = mp;
        size += 1+mp->get_size(); // add pos for space
    }

    char *to_string() {
        char *result = (char *)malloc(get_size()+1);
        copyPartsTo(result);
        mp_assert(get_size() == strlen(result));
        return result;
    }
};

// ------------------------
//      AttributeParser

class AttributeParser {
    const char *attrName;
    size_t      attrNameLen;
    bool        expandName;   // try to expand 'attrName'
    bool        expectParens; // otherwise parens are optional

public:
    AttributeParser(const char *attrName_, bool expandName_, bool expectParens_)
        : attrName(attrName_),
          attrNameLen(strlen(attrName)),
          expandName(expandName_),
          expectParens(expectParens_)
    {}

private:
    void parse_one_attr(LinePart& part) const {
        const char *found = strstr(part.whole_line(), attrName);
        if (found) {
            const char *behind     = found+attrNameLen;
            if (expandName) behind = nextNonWord(behind);
            const char *openParen  = nextNonSpace(behind);

            if (*openParen == '(') {
                const char *closeParen = matchingParen(openParen);
                if (closeParen) part.define(found, closeParen);
                else {
                    part.define(found, openParen);
                    part.error("Could not find matching paren after '%s'");
                }
            }
            else {
                part.define(found, behind-1);
                if (expectParens) part.error("Expected to see '(' after '%s'");
            }
        }
    }

    // rest is not really part of this class - may go to abstract base class if needed

    PartQueue *parse_all(LinePart from) const {
        parse_one_attr(from);
        if (from.is_empty()) return NULL;

        PartQueue *head = new PartQueue(from);
        PartQueue *rest = parse_all(from.behind());
        if (rest) head->append(rest);

        return head;
    }

public:
    char *parse(const char *toParse, size_t toParseSize) const {
        char      *result           = NULL;
        PartQueue *found_attributes = parse_all(LinePart(toParse, toParseSize));

        if (found_attributes) {
            result = found_attributes->to_string();
            delete found_attributes;
        }
        return result;
    }
};

static void search_comment_for_attribute() {
    if (!found__ATTR__) { // only parse once (until reset)
        if (search__ATTR__) {
            last_comment[lc_size] = 0;  // close string

            static AttributeParser ATTR_parser("__ATTR__", true, false);
            found__ATTR__ = ATTR_parser.parse(last_comment, lc_size);
        }
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
        mp_assert(behind_promo[-1] == ':'); // wrong promotion_tag_len

        char *eol     = strchr(behind_promo, '\n');
        if (!eol) eol = strchr(behind_promo, 0);

        if (eol) {
            while (eol>behind_promo && eol[-1] == ' ') --eol; // trim spaces at eol
        }

        mp_assert(eol);
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
 * if a comment contains __ATTR__ and search__ATTR__ != 0
 * the attribute string is stored in found__ATTR__
 */


static int fnextch(FILE *f) {
    int c, lastc, incomment;

    c = ngetc(f);
    while (c == '\\') {
        DEBUG_PRINT("fnextch: in backslash loop\n");
        ngetc(f);                               // skip a character
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
                mp_assert(lc_size<MAX_COMMENT_SIZE);

                if (lastc == '*' && c == '/') incomment = 0;
                else if (c < 0) {
                    error("EOF reached in comment");
                    errorAt(commentStartLine, "comment started here");
                    return c;
                }
            }
            if (search__ATTR__) search_comment_for_attribute();
            if (promote_lines) search_comment_for_promotion();
            return fnextch(f);
        }
        else if (c == '/') {                        // C++ style comment
            incomment = 1;
            c         = ' ';
            DEBUG_PRINT("fnextch: C++ comment seen\n");
            lc_size   = 0;

            while (incomment) {
                lastc                   = c;
                c                       = ngetc(f);
                last_comment[lc_size++] = c;
                mp_assert(lc_size<MAX_COMMENT_SIZE);

                if (lastc != '\\' && c == '\n') incomment = 0;
                else if (c < 0) break;
            }
            if (search__ATTR__) search_comment_for_attribute();
            if (promote_lines) search_comment_for_promotion();

            if (c == '\n') return c;
            return fnextch(f);
        }
        else {
            // if we pre-fetched a linefeed, remember to adjust the line number
            if (c == '\n') linenum--;
            ungetc(c, f);
            return '/';
        }
    }
    return c;
}


static int nextch(FILE *f) {
    // Get the next "interesting" character. Comments are skipped, and strings
    // are converted to "0". Also, if a line starts with "#" it is skipped.

    int c = fnextch(f);

    // skip preprocessor directives
    // EXCEPTION: #line nnn or #nnn lines are interpreted

    if (newline_seen && c == '#') {
        // skip blanks
        do {
            c = fnextch(f);
        } while (c >= 0 && (c == '\t' || c == ' '));

        // check for #line
        if (c == 'l') {
            c = fnextch(f);
            if (c != 'i')   // not a #line directive
                goto skip_rest_of_line;
            do {
                c = fnextch(f);
            } while (c >= 0 && c != '\n' && !isdigit(c));
        }

        // if we have a digit it's a line number, from the preprocessor
        if (c >= 0 && isdigit(c)) {
            char numbuf[10];
            char *p = numbuf;
            for (int n = 8; n >= 0; --n) {
                *p++ = c;
                c = fnextch(f);
                if (c <= 0 || !isdigit(c))
                    break;
            }
            *p = 0;
            linenum = atol(numbuf) - 1;
        }

        // skip the rest of the line
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
                    return '$';                     // found "C" (needed for 'extern "C"')
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
#endif // DEBUG_PRINTS

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
#endif // DEBUG_PRINTS
            }
            else if (c == '}') {
                inbrack--;
#if defined(DEBUG_PRINTS)
                fprintf(stderr, "inbrack=%i (line=%li)\n", inbrack, linenum);
#endif // DEBUG_PRINTS
            }
        }
        strcpy(buf, "{}");
        glastc = nextch(f);
        DEBUG_PRINT("getsym: returning brackets '");
    }
    else if (!IS_CSYM(c)) {
        *buf++ = c;
        *buf   = 0;
        glastc = nextch(f);

        DEBUG_PRINT("getsym: returning special symbol '");
    }
    else {
        while (IS_CSYM(c)) {
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
    // find most common type specifiers for purpose of ruling them out as parm names

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
    (IS_CSYM(*(w)->string) && !is_type_word((w)->string) && \
    (!(w)->next || *(w)->next->string == ',' || \
     *(w)->next->string == '['))


static Word *typelist(Word *p) {
    // given a list representing a type and a variable name, extract just
    // the base type, e.g. "struct word *x" would yield "struct word"

    Word *w, *r;

    r = w = word_alloc("");
    while (p && p->next) {
        // handle int *x --> int
        if (p->string[0] && !IS_CSYM(p->string[0]))
            break;
        // handle int x[] --> int
        if (p->next->string[0] == '[')
            break;
        w->next = word_alloc(p->string);
        w = w->next;
        p = p->next;
    }
    return r;
}

static Word *getparamlist(FILE *f) {
    // Get a parameter list; when this is called the next symbol in line
    // should be the first thing in the list.

    Word *pname[MAXPARAM];                   // parameter names
    Word *tlist   = NULL;                    // type name
    Word *plist;                             // temporary
    int   np      = 0;                       // number of parameters
    int   typed[MAXPARAM];                   // parameter has been given a type
    int   tlistdone;                         // finished finding the type name
    int   sawsomething;
    int   i;
    int   inparen = 0;
    char  buf[80];

    Word *result = NULL;

    DEBUG_PRINT("in getparamlist\n");
    for (i = 0; i < MAXPARAM; i++)
        typed[i] = 0;

    plist = word_alloc("");

    // first, get the stuff inside brackets (if anything)

    sawsomething = 0;                               // gets set nonzero when we see an arg
    for (;;) {
        if (getsym(buf, f) < 0) {
            goto cleanup;
        }
        if (*buf == ')' && (--inparen < 0)) {
            if (sawsomething) {                     // if we've seen an arg
                pname[np] = plist;
                plist = word_alloc(""); // @@@ Value stored to 'plist' is never read
                np++;
            }
            break;
        }
        if (*buf == ';') {                          // something weird
            result = ABORTED;
            goto cleanup;
        }
        sawsomething = 1;                           // there's something in the arg. list
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

    // next, get the declarations after the function header

    inparen = 0;

    word_free(plist);

    tlist = word_alloc("");
    plist = word_alloc("");

    tlistdone    = 0;
    sawsomething = 0;

    for (;;) {
        if (getsym(buf, f) < 0) {
            goto cleanup;
        }

        if (*buf == ',' && !inparen) {              // handle a list like "int x,y,z"
            if (!sawsomething) {
                goto cleanup;
            }
            for (i = 0; i < np; i++) {
                if (!typed[i] && foundin(plist, pname[i])) {
                    typed[i] = 1;
                    word_free(pname[i]);
                    pname[i] = word_append(tlist, plist);
                    // promote types
                    typefixhack(pname[i]);
                    break;
                }
            }
            if (!tlistdone) {
                word_free(tlist);
                tlist     = typelist(plist);
                tlistdone = 1;
            }
            word_free(plist);
            plist = word_alloc("");
        }
        else if (*buf == ';') {                     // handle the end of a list
            if (!sawsomething) {
                result = ABORTED;
                goto cleanup;
            }
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
            word_free(tlist);
            word_free(plist);
            tlist = word_alloc("");
            plist = word_alloc("");
        }
        else if (strcmp(buf, "{}") == 0) break;     // handle the  beginning of the function
        else if (strcmp(buf, "register")) {         // otherwise, throw the word into the list (except for "register")
            addword(plist, buf);
            if (*buf == '(') inparen++;
            else if (*buf == ')') inparen--;
            else sawsomething = 1;
        }
    }

    // Now take the info we have and build a prototype list

    word_free(plist);
    word_free(tlist);

    // empty parameter list means "void"
    if (np == 0) {
        result = word_alloc("void");
        goto cleanup;
    }

    plist = tlist = word_alloc("");

    /* how to handle parameters which contain only one word ?
     *
     * void -> void
     * UNFIXED -> UNFIXED     see ../SL/CB/cb_base.h@UNFIXED
     * xxx  -> int xxx        (if AUTO_INT is defined)
     * int  -> int dummyXX    (otherwise)
     */

    // #define AUTO_INT

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
                Word *pn_list    = pname[i];
                int   cnt        = 0;
                bool  is_void    = false;
                bool  is_UNFIXED = false;

                while (pn_list) {                   // count words
                    if (pn_list->string[0]) {
                        ++cnt;
                        if (strcmp(pn_list->string, "void")==0) is_void = true;
                        if (strcmp(pn_list->string, "UNFIXED")==0) is_UNFIXED = true;
                    }
                    pn_list = pn_list->next;
                }
                if (cnt==1 && !is_void && !is_UNFIXED) { // only name, but neighter 'void' nor 'UNFIXED'
                    // no type or no parameter name
#ifdef AUTO_INT
                    // If no type provided, make it an "int"
                    addword(tlist, "int"); // add int to tlist before adding pname[i]
#else
                    add_dummy_name = 1; // add a dummy name after adding pname[i]
#endif
                }
            }

            while (tlist->next) tlist = tlist->next;

            tlist->next = pname[i];
            pname[i]    = NULL;

#if !defined(AUTO_INT)
            if (add_dummy_name) {
                mp_assert(dummy_counter<100);
                dummy_name[DUMMY_COUNTER_POS] = (dummy_counter/10)+'0';
                dummy_name[DUMMY_COUNTER_POS] = (dummy_counter%10)+'0';
                addword(tlist, dummy_name);
                dummy_counter++;
            }
#endif

            if (i < np - 1) addword(tlist, ",");
        }
    }

    result = plist;
    plist  = NULL;
    tlist  = NULL;

  cleanup:

    word_free(plist);
    word_free(tlist);
    for (int n = 0; n<np; ++n) {
        word_free(pname[n]);
    }

    return result;
}

inline Word *getLastPtrRef(Word *w) {
    Word *last = NULL;
    while (w) {
        if (strchr("&*", w->string[0]) == NULL) break;
        last = w;
        w    = w->next;
    }
    return last;
}

static void emit(Word *wlist, Word *plist, long startline) {
    // emit a function declaration. The attributes and name of the function
    // are in wlist; the parameters are in plist.

    Word *w;
    int   count     = 0;
    int   isstatic  = 0;
    int   ismain    = 0;
    int   refs      = 0;

    if (promote_lines) print_promotions();

    for (w = wlist; w; w = w->next) {
        if (w->string[0]) {
            count ++;
            if      (strcmp(w->string, "static") == 0) isstatic = 1;
            else if (strcmp(w->string, "main")   == 0) ismain = 1;
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

        if (strcmp(plist->string, "void") != 0) {   // if parameter is not 'void'
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
                if (!name_seen) {                   // automatically insert missing parameter names
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


    // if the -e flag was given, and it's not a static function, print "extern"

    if (print_extern && !isstatic) {
        printf("extern ");
    }

    int spaceBeforeNext = 0;
    if (count < 2) {
        printf("int");
        spaceBeforeNext = 1;
    }

    for (w = wlist; w; w = w->next) {
        if (spaceBeforeNext) {
            DEBUG_PRINT("emit[1] ' '\n");
            putchar(' ');
        }
        printf("%s", w->string);
        DEBUG_PRINT_STRING("emit[2] ", w->string);
        spaceBeforeNext = IS_CSYM(w->string[0]);
    }

    if (use_macro) printf(" %s((", macro_name);
    else putchar('(');
    DEBUG_PRINT("emit[3] '('\n");

    spaceBeforeNext = 0;
    for (w = plist; w; w = w->next) {
        if (no_parm_names && IS_PARM_NAME(w)) continue;

        const char *token    = w->string;
        char        tokStart = token[0];

        if (!tokStart) continue;                    // empty token

        if (tokStart == ',')                       spaceBeforeNext = 1;
        else if (strchr("[])", tokStart) != NULL)  spaceBeforeNext = 0;
        else {
            int nextSpaceBeforeNext;
            if (strchr("&*", tokStart) != NULL) {
                if (spaceBeforeNext) {
                    Word *lastPtrRef = getLastPtrRef(w);
                    if (lastPtrRef->string[0] == '&') spaceBeforeNext = 0;
                }
                nextSpaceBeforeNext = tokStart == '&';
            }
            else {
                nextSpaceBeforeNext = IS_CSYM(tokStart);;
            }
            if (spaceBeforeNext) {
                putchar(' ');
                DEBUG_PRINT("emit[4] ' '\n");
            }
            spaceBeforeNext = nextSpaceBeforeNext;
        }
        fputs(token, stdout);
        DEBUG_PRINT_STRING("emit[5] ", token);
    }

    if (use_macro)  PRINT("))");
    else            PRINT(")");
    DEBUG_PRINT("emit[6] ')'\n");

    if (found__ATTR__) { PRINT(" "); PRINT(found__ATTR__); }

    PRINT(";\n");
}

static void getdecl(FILE *f, const char *header) {
    // parse all function declarations and print to STDOUT

    Word *wlist                 = NULL;
    char  buf[80];
    int   sawsomething;
    long  startline;                                // line where declaration started
    int   oktoprint;
    int   header_printed        = 0;

    current_file = strdup(header);

  again :
    DEBUG_PRINT("again\n");

    word_free(wlist);
    wlist = word_alloc("");

    bool seen_static_or_inline = false;
    bool seen__ATTR            = false;

    sawsomething  = 0;
    oktoprint     = 1;
    extern_c_seen = 0;

    for (;;) {
        DEBUG_PRINT("main getdecl loop\n");
        if (getsym(buf, f) < 0) {
            DEBUG_PRINT("EOF in getdecl loop\n");
            goto end;
        }

        DEBUG_PRINT("getdecl: '");
        DEBUG_PRINT(buf);
        DEBUG_PRINT("'\n");

        // try to guess when a declaration is not an external function definition
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
                goto end;
            }

            DEBUG_PRINT("test auf extern \"C\": '");
            DEBUG_PRINT(buf);
            DEBUG_PRINT("'\n");

            if (strcmp(buf, "$") == 0) {            // symbol used if "C" was found
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

        if (strncmp(buf, "__ATTR__", 8) == 0) { // prefix attribute (should only be used for static and inline)
            DEBUG_PRINT("seen prefix __ATTR__: '");
            DEBUG_PRINT(buf);
            DEBUG_PRINT("'\n");

            seen__ATTR = true;
        }

        if (strcmp(buf, "static") == 0 || strcmp(buf, "inline") == 0) {
            seen_static_or_inline = true;
            oktoprint = 0;
        }


        if (strcmp(buf, ";") == 0) goto again;

        // A left parenthesis *might* indicate a function definition
        if (strcmp(buf, "(")==0) {
            startline   = linenum;
            Word *plist = NULL;
            if (!sawsomething || !(plist = getparamlist(f))) {
                skipit(buf, f);
                goto again;
            }
            if (plist == ABORTED)
                goto again;

            // It seems to have been what we wanted

            if (oktoprint) {                        // check function-name
                Word *w;

                for (w=wlist; w->next && oktoprint; w=w->next) {
                    if (w->string[0]==':' && w->string[1]==0) oktoprint = 0; // do not emit prototypes for member functions
                }

                if (oktoprint && !wantPrototypeFor(w->string)) {
                    oktoprint = 0;                  // => do not emit prototype
                }
            }

            if (seen__ATTR && oktoprint) {
                DEBUG_PRINT("attempt to emit seen__ATTR (suppressed)");
                oktoprint = 0;
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

  end:
    word_free(wlist);
}

__ATTR__NORETURN static void Usage(const char *msg = NULL) {
    fprintf(stderr,
            "\naisc_mkpts - ARB prototype generator"
            "\nUsage: %s [options] [files ...]", ourname);
    fputs("\nSupported options:"
          "\n   -F part[,part]*  only promote declarations for functionnames containing one of the parts"
          "\n                    if 'part' starts with a '^' functionname has to start with rest of part"
          "\n   -S (like -F)     do NOT promote matching declarations (defaults to -S '^TEST_,^NOTEST_')"
          "\n"
          "\n                    Instead of ',' (=or) you may use '+' (=and)"
          "\n"
          "\n   -G               search for ARB macro __ATTR__ in comment behind function header"
          "\n   -P               promote /*AISC_MKPT_PROMOTE:forHeader*/ to header"
          "\n"
          "\n   -w gen.h         add standard include wrapper (needs name of generated header)"
          "\n   -c \"text\"        add text as comment into header"
          "\n"
          "\n   -C               insert 'extern \"C\"'"
          "\n   -E               prefix 'extern \"C\"' at prototype"
          "\n"
          "\n   -W               don't promote types in old style declarations"
          "\n   -x               omit parameter names in prototypes"
          "\n"
          "\n   -K               use K&R prototype macro (default assumes header files are strict ANSI)"
          "\n   -D               define K&R prototype macro"
          "\n   -p sym           use \"sym\" as the prototype macro (default \"P_\")"
          "\n"
          "\n   -m               promote declaration of 'main()' (default is to skip it)"
          "\n   -a               make a function list for aisc_includes (default: generate C prototypes)"
          "\n   -e               put an explicit \"extern\" keyword in declarations"
          "\n   -n               put line numbers of declarations as comments"
          "\n"
          "\n   -V               print version number"
          "\n   -h               print this help"
          "\n"
          , stderr);
    if (msg) fprintf(stderr, "\nError: %s", msg);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

static int string_comparator(const void *v0, const void *v1) {
    return strcmp(*(const char **)v0, *(const char **)v1);
}

__ATTR__NORETURN static void MissingArgumentFor(char option) {
    char buffer[100];
    sprintf(buffer, "option -%c expects an argument", option);
    Usage(buffer);
}
__ATTR__NORETURN static void UnknownOption(char option) {
    char buffer[100];
    sprintf(buffer, "unknown option -%c", option);
    Usage(buffer);
}

int ARB_main(int argc, char *argv[]) {
    bool exit_if_noargs = false;

    if (argv[0] && argv[0][0]) {
        ourname = strrchr(argv[0], '/');
        if (!ourname) ourname = argv[0];
    }
    else ourname = "mkptypes";

    argv++; argc--;

    addExcludedSymParts("^TEST_,^NOTEST_"); // exclude unit tests

    char *iobuf = (char *)malloc(NEWBUFSIZ);
    while (*argv && **argv == '-') {
        const char *t = *argv++; --argc; t++;
        while (*t) {
            if (*t == 'e')      print_extern        = 1;
            else if (*t == 'C') cansibycplus        = 1;
            else if (*t == 'E') promote_extern_c    = 1;
            else if (*t == 'W') dont_promote        = 1;
            else if (*t == 'a') aisc                = 1;
            else if (*t == 'G') search__ATTR__      = 1;
            else if (*t == 'n') donum               = 1;
            else if (*t == 'x') no_parm_names       = 1; // no parm names, only types (sg)
            else if (*t == 'D') define_macro        = 1;
            else if (*t == 'K') use_macro           = 1;
            else if (*t == 'P') promote_lines       = 1;
            else if (*t == 'm') use_main            = 1;
            else if (*t == 'p') {
                t = *argv++; --argc;
                if (!t) MissingArgumentFor(*t);
                use_macro = 1;
                macro_name = t;
                break;
            }
            else if (*t == 'c') {
                t = *argv++; --argc;
                if (!t) MissingArgumentFor(*t);
                header_comment = t;
                break;
            }
            else if (*t == 'w') {
                t = *argv++; --argc;
                if (!t) MissingArgumentFor(*t);
                include_wrapper = t;
                break;
            }
            else if (*t == 'F') {
                t = *argv++; --argc;
                if (!t) MissingArgumentFor(*t);
                addRequiredSymParts(t);
                break;
            }
            else if (*t == 'S') {
                t = *argv++; --argc;
                if (!t) MissingArgumentFor(*t);
                freeExcludedSymParts();
                addExcludedSymParts(t);
                break;
            }
            else if (*t == 'V') {
                exit_if_noargs = true;
                Version();
            }
            else if (*t == 'h') Usage();
            else UnknownOption(*t);
            t++;
        }
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

    current_dir = getcwd(0, 255);
    if (argc == 0) {
        getdecl(stdin, "<from stdin>");
    }
    else {
        const char *filename[1000];
        int         fcount = 0;

        while (argc > 0 && *argv) {
            filename[fcount++] = *argv;
            argc--; argv++;
        }

        qsort(&filename, fcount, sizeof(filename[0]), string_comparator);

        for (int i = 0; i<fcount; ++i) {
            DEBUG_PRINT("trying new file '");
            DEBUG_PRINT(filename[i]);
            DEBUG_PRINT("'\n");

            FILE *f = fopen(filename[i], "r");
            if (!f) {
                perror(filename[i]);
                exit(EXIT_FAILURE);
            }
            if (iobuf) setvbuf(f, iobuf, _IOFBF, NEWBUFSIZ);

            linenum      = 1;
            newline_seen = 1;
            glastc       = ' ';
            getdecl(f, filename[i]);
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
            printf("\n#undef %s\n", macro_name);    // clean up namespace
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

    freeRequiredSymParts();
    freeExcludedSymParts();

    free(current_file);
    free(current_dir);

    free(iobuf);

    return EXIT_SUCCESS;
}

static void Version() {
    fprintf(stderr, "%s 1.1 ARB\n", ourname);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

inline const char *test_extract(const char *str) {
    search__ATTR__ = true;

    clear_found_attribute();

    strcpy(last_comment, str);
    lc_size = strlen(last_comment);

    search_comment_for_attribute();

    return found__ATTR__;
}

#define TEST_ATTR_____(comment,extracted) TEST_EXPECT_EQUAL(test_extract(comment), extracted)

void TEST_attribute_parser() {
    TEST_ATTR_____("",             (const char*)NULL);
    TEST_ATTR_____("nothing here", (const char*)NULL);

    TEST_ATTR_____("bla bla __ATTR__DEPRECATED(\" my reason \") more content", "__ATTR__DEPRECATED(\" my reason \")");
    TEST_ATTR_____("bla bla __ATTR__FORMAT(pos) more content",                 "__ATTR__FORMAT(pos)");
    
    TEST_ATTR_____("__ATTR__DEPRECATED",       "__ATTR__DEPRECATED");
    TEST_ATTR_____("__ATTR__FORMAT(pos)",      "__ATTR__FORMAT(pos)");
    TEST_ATTR_____("bla __ATTR__FORMAT(pos)",  "__ATTR__FORMAT(pos)");
    TEST_ATTR_____("__ATTR__FORMAT(pos) bla",  "__ATTR__FORMAT(pos)");
    TEST_ATTR_____("    __ATTR__FORMAT(pos) ", "__ATTR__FORMAT(pos)");
    
    TEST_ATTR_____("__ATTR__FORMAT((pos)",           (const char*)NULL);
    TEST_ATTR_____("__attribute__(pos",              (const char*)NULL);
    TEST_ATTR_____("__ATTR__FORMAT(pos))",           "__ATTR__FORMAT(pos)");
    TEST_ATTR_____("__ATTR__FORMAT((pos))",          "__ATTR__FORMAT((pos))");
    TEST_ATTR_____("__ATTR__FORMAT((pos)+((sop)))",  "__ATTR__FORMAT((pos)+((sop)))");
    TEST_ATTR_____("__ATTR__FORMAT(((pos)))+(sop))", "__ATTR__FORMAT(((pos)))");

    TEST_ATTR_____("bla bla __ATTR__DEPRECATED __ATTR__FORMAT(pos) more content", "__ATTR__DEPRECATED __ATTR__FORMAT(pos)");
    TEST_ATTR_____("bla __ATTR__DEPRECATED bla more__ATTR__FORMAT(pos)content", "__ATTR__DEPRECATED __ATTR__FORMAT(pos)");
    
    TEST_ATTR_____(" goes to header: __ATTR__NORETURN  */", "__ATTR__NORETURN");

    clear_found_attribute();
}
TEST_PUBLISH(TEST_attribute_parser);

#endif // UNIT_TESTS
