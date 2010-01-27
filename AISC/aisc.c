/* ================================================================ */
/*                                                                  */
/*   File      : aisc.c                                             */
/*   Purpose   : ARB integrated source compiler                     */
/*                                                                  */
/*   Institute of Microbiology (Technical University Munich)        */
/*   http://www.arb-home.de/                                        */
/*                                                                  */
/* ================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "aisc.h"

const int linebufsize = 200000;
char      string_buf[256];

struct global_struct *gl;

inline void get_byte(const char *& io) {
    gl->lastchar = *(io++);
    if (is_LF(gl->lastchar)) {
        gl->last_line_start = io;
        ++gl->line_cnt;
    }
}

char *read_aisc_file(char *path)
{
    FILE *input;
    int   data_size;
    char *buffer = 0;
    if ((input = fopen(path, "r")) == 0) {
        fprintf(stderr, " file %s not found\n", path);
    }
    else {
        if (fseek(input, 0, 2)==-1) {
            fprintf(stderr, "file %s not seekable\n", path);
        }
        else {
            data_size = (int)ftell(input);

            if (data_size == 0) {
                fprintf(stderr, "%s:0: file is empty\n", path);
            }
            else {
                data_size++;
                rewind(input);
                buffer =  (char *)malloc(data_size+1);
                data_size = fread(buffer, 1, data_size, input);
                buffer[data_size] = 0;
            }
        }
        fclose(input);
    }
    return buffer;
}

static void aisc_init() {
    gl = (struct global_struct *)calloc(sizeof(struct global_struct), 1);

    gl->linebuf         = (char *)calloc(sizeof(char), linebufsize+2);
    gl->line_cnt        = 1;
    gl->last_line_start = NULL;
    gl->error_flag      = 0;
    gl->lastchar        = ' ';
    gl->bufsize         = linebufsize;
    gl->out             = stdout;
    gl->outs[0]         = stdout;
    gl->fouts[0]        = strdup("stdout");
    gl->outs[1]         = stdout;
    gl->fouts[1]        = strdup("*");
    gl->tabstop         = 8;

    do_com_push("");
    gl->fns = create_hash(HASHSIZE);

    for (int i = 0; i < 256; i++) {
        gl->outtab[i] = i;
    }

    gl->outtab[(unsigned char)'n']  = '\n';
    gl->outtab[(unsigned char)'t']  = '\t';
    gl->outtab[(unsigned char)'0']  = 0;
    gl->outtab[(unsigned char)'1']  = 0;
    gl->outtab[(unsigned char)'2']  = 0;
    gl->outtab[(unsigned char)'3']  = 0;
    gl->outtab[(unsigned char)'4']  = 0;
    gl->outtab[(unsigned char)'5']  = 0;
    gl->outtab[(unsigned char)'6']  = 0;
    gl->outtab[(unsigned char)'7']  = 0;
    gl->outtab[(unsigned char)'8']  = 0;
    gl->outtab[(unsigned char)'9']  = 0;
    gl->outtab[(unsigned char)'\\'] = 0;
}
static void aisc_exit() {
    delete gl->root;
    for (int i = 0; i<OPENFILES; i++) {
        free(gl->fouts[i]);
        free(gl->fouts_name[i]);
        if (gl->outs[i] && gl->outs[i] != stdout) {
            fclose(gl->outs[i]);
        }
    }
    free_CL(gl->prg);
    free_stack();               // frees gl->st
    free_hash(gl->fns);
    free(gl->linebuf);
    free(gl->line_path);
    free(gl);
    gl = NULL;
}

#define LOCBUFFERSIZE 1024
static const char *getCurrentLocation(const char *io) {
    static char loc[LOCBUFFERSIZE+1];
    int         printed;

    if (gl->last_line_start) {
        int column = io - gl->last_line_start + 1;

        aisc_assert(column >= 0 && column<10000);
        printed = sprintf(loc, "%s:%i:%i", gl->line_path, gl->line_cnt, column);
    }
    else {
        printed = sprintf(loc, "%s:%i", gl->line_path, gl->line_cnt);
    }

    if (printed>LOCBUFFERSIZE) {
        fprintf(stderr, "AISC: Internal buffer overflow detected -- terminating [loc]\n");
        gl->error_flag = 1;
    }

    return loc;
}

static void p_err(const char *io, const char *error) {
    fprintf(stderr, "%s: Error: %s\n", getCurrentLocation(io), error);
    gl->error_flag = 1;
}

#ifdef LINUX
# define HAVE_VSNPRINTF
#endif
#ifdef HAVE_VSNPRINTF
# define PRINT2BUFFER(buffer, bufsize, templat, parg) vsnprintf(buffer, bufsize, templat, parg);
#else
# define PRINT2BUFFER(buffer, bufsize, templat, parg) vsprintf(buffer, templat, parg);
#endif

#define ERRBUFFERSIZE 1024
static void p_errf(const char *io, const char *formatString, ...) {
    static char buf[ERRBUFFERSIZE+1];
    int         printed;
    va_list     varArgs;

    va_start(varArgs, formatString);
    printed = PRINT2BUFFER(buf, ERRBUFFERSIZE, formatString, varArgs);
    va_end(varArgs);

    if (printed>ERRBUFFERSIZE) {
        fprintf(stderr, "AISC: Internal buffer overflow detected -- terminating [err]\n");
        gl->error_flag = 1;
    }
    p_err(io, buf);
}
static void p_err_expected(const char *io, const char *expect)                     { p_errf(io, "Expected to see %s", expect); }
static void p_err_exp_but_saw(const char *io, const char *expect, const char *saw) { p_errf(io, "Expected to see %s, but saw %s", expect, saw); }

static void p_err_exp_string_but_saw(const char *io, const char *saw) { p_err_exp_but_saw(io, "string", saw); }
static void p_err_exp_but_saw_EOF(const char *io, const char *expect) { p_err_exp_but_saw(io, expect, "end of file"); }

static void p_err_empty_braces(const char *io) { p_err(io, "{} found, missing contents"); }
static void p_err_exp_line_terminator(const char *io) { p_err(io, "missing ',' or ';' or 'newline'"); }
static void p_err_ill_atWord(const char *io) { p_err(io, "only header definitions may start with '@'"); }
static void p_err_exp_atWord(const char *io) { p_err(io, "all words in header-definitions have to start with '@'"); }


inline void expect_line_terminator(const char *in) {
    if (!is_SEP_LF_EOS(gl->lastchar)) p_err_exp_line_terminator(in);
}
inline void expect_and_skip_closing_brace(const char *& in, const char *openingBraceLocation) {
    if (gl->lastchar != '}') {
        p_err_expected(in, "'}'");
        fprintf(stderr, "%s: opening brace was here\n", openingBraceLocation);
    }
    get_byte(in);
}
inline void expect_and_skip(const char *& in, char expect) {
    if (gl->lastchar != expect) {
        char buf[] = "'x'";
        buf[1]     = expect;
        p_err_expected(in, buf);
    }
    get_byte(in);
}

inline void skip_over_spaces(const char *& in) {
    while (is_SPACE(gl->lastchar)) get_byte(in);
}
inline void skip_over_spaces_and_comments(const char *& in) {
    skip_over_spaces(in);
    if (gl->lastchar == '#') { // comment -> skip rest of line
        while (!is_LF_EOS(gl->lastchar)) get_byte(in);
    }
}
inline void skip_over_spaces_and_comments_multiple_lines(const char *& in) {
    while (1) {
        skip_over_spaces_and_comments(in);
        if (!is_LF(gl->lastchar)) break;
        get_byte(in);
    }
}

inline void copyWordTo(const char*& in, char*& out) {
    while (!is_SPACE_SEP_LF_EOS(gl->lastchar)) {
        *(out++) = gl->lastchar;
        get_byte(in);
    }
}
inline void copyTillQuotesTo(const char*& in, char*& out) {
    // closing quotes are END_STR1 plus END_STR2
    bool quotes_closed = false;
    while (!quotes_closed) {
        while ((gl->lastchar != EOSTR) && (gl->lastchar != END_STR1)) {
            *(out++) = gl->lastchar;
            get_byte(in);
        }

        if (gl->lastchar == END_STR1) {
            get_byte(in);
            if (gl->lastchar == END_STR2) {
                get_byte(in);
                quotes_closed = true;
            }
            else {
                *(out++) = END_STR1;
            }
        }
    }
}

static char *readWord(const char *& in) {
    char  buf[1024];
    char *cp = buf;

    if (gl->lastchar == BEG_STR1) {
        get_byte(in);
        if (gl->lastchar == BEG_STR2) {
            get_byte(in);
            copyTillQuotesTo(in, cp);
        }
        else {
            *(cp++) = BEG_STR1;
            copyWordTo(in, cp);
        }
    }
    else copyWordTo(in, cp);

    if (gl->lastchar != EOSTR) skip_over_spaces_and_comments(in);

    char *result = NULL;
    if (gl->lastchar == EOSTR) {
        p_err_exp_but_saw_EOF(in, "terminator after string");
    }
    else if (cp != buf) { // do not return empty string, return NULL instead
        *cp    = 0;
        result = strdup(buf);
    }

    return result;
}

enum TOKEN {
    TOK_WORD = 100,             // normal words
    TOK_AT_WORD,                // words starting with '@'
    TOK_BRACE_CLOSE,
    TOK_BRACE_OPEN,
    TOK_COMMA,
    TOK_SEMI,
    TOK_EOS  = TOK_SEMI,        // simulate a semicolon at EOSTR

    TOK_INVALID,
};

static char *parse_token(const char *& in, enum TOKEN& foundTokenType);

static char *SETSOURCE(const char *& in, enum TOKEN& foundTokenType) {
    char       *result = NULL;
    const char *space  = in+9;

    if (*space != ' ') p_err(in, "space expected after '@SETSOURCE' (injected code)");
    else {
        in = space;
        get_byte(in);
        skip_over_spaces(in);

        const char *file  = in-1;
        const char *comma = strchr(file, ',');

        if (!comma) p_err(in, "comma expected after '@SETSOURCE filename' (injected code)");
        else {
            const char *end = strchr(comma, '@');

            if (!end) p_err(in, "'@' expected after '@SETSOURCE filename,line' (injected code)");
            else {
                free(gl->line_path);
                gl->line_path = copy_string_part(file, comma-1);
                gl->line_cnt  = atoi(comma+1);

                in = end+1;
                get_byte(in);

                result = parse_token(in, foundTokenType);
            }
        }
    }
    return result;
}

static char *parse_token(const char *& in, enum TOKEN& foundTokenType) {
    skip_over_spaces_and_comments_multiple_lines(in);

    char *result   = NULL;
    foundTokenType = TOK_INVALID;

    switch (gl->lastchar) {
        case EOSTR: foundTokenType = TOK_EOS;         break;
        case '{':   foundTokenType = TOK_BRACE_OPEN;  break;
        case '}':   foundTokenType = TOK_BRACE_CLOSE; break;
        case ',':   foundTokenType = TOK_COMMA;       break;
        case ';':   foundTokenType = TOK_SEMI;        break;

        case '@':
            if (strncmp(in, "SETSOURCE", 9) == 0) {
                result = SETSOURCE(in, foundTokenType);
            }
            else {
                get_byte(in);
                skip_over_spaces(in);
                if (gl->lastchar == EOSTR) p_err_exp_but_saw_EOF(in, "ID behind '@'");
                else {
                    result         = readWord(in);
                    foundTokenType = TOK_AT_WORD;
                }
            }
            break;

        default:
            result         = readWord(in);
            foundTokenType = TOK_WORD;
            break;
    }

    return result;
}

class Header {
    char   *key;
    Header *next;
 public:
    Header(const char *key_) { key = strdup(key_); next = NULL; }
    ~Header() { free(key); delete next; }

    void set_next(Header *header) { aisc_assert(!next); next = header; }

    const char *get_key() const { return key; }
    const Header *next_header() const { return next; }
};

class HeaderList {
    Header *head;
    Header *tail;

    char *loc_defined_at;

 public:
    HeaderList() {
        head = tail = NULL;
        loc_defined_at = NULL;
    }
    void reset() {
        delete head;
        head           = tail = NULL;
        free(loc_defined_at);
        loc_defined_at = NULL;
    }
    ~HeaderList() { reset(); }

    void set_first_header(Header *header, const char *location) {
        aisc_assert(!head);
        head           = tail = header;
        loc_defined_at = strdup(location);
    }
    void append(Header *header) {
        aisc_assert(head);      // use set_first_header()
        tail->set_next(header);
        tail = header;
    }

    const Header *first_header() const { return head; }
    const char *defined_at() const { return loc_defined_at; }
};

static TokenList *parse_aisc_TokenList(const char *& in, HeaderList& headerList) {
    TokenList    *items  = new TokenList;
    const Header *header = headerList.first_header();

    get_byte(in);

    bool reached_end_of_list = false;
    while (!gl->error_flag && !reached_end_of_list) {
        TOKEN  foundTokenType;
        char  *str = parse_token(in, foundTokenType);

        if (!gl->error_flag) {
            switch (foundTokenType) {
                case TOK_SEMI:
                case TOK_BRACE_CLOSE:
                    reached_end_of_list = true;
                    break;

                case TOK_BRACE_OPEN: {
                    char           *openLoc = strdup(getCurrentLocation(in));
                    TokenListBlock *block   = parse_aisc_TokenListBlock(in);
                    if (!gl->error_flag) {
                        if (block) {
                            Token *sub = new Token(header ? header->get_key() : "{", block);
                            block = NULL;
                            items->append(sub);
                            expect_and_skip_closing_brace(in, openLoc);
                        }
                        else {
                            p_err_empty_braces(in);
                        }
                    }
                    delete block;
                    free(openLoc);
                    break;
                }
                case TOK_COMMA:
                    if (!header) p_err_exp_but_saw(in, "string", "','");
                    else get_byte(in);
                    break;

                case TOK_AT_WORD:
                    if (header != headerList.first_header()) {
                        p_err_ill_atWord(in);
                    }
                    else {
                        if (!str) {
                            p_err_expected(in, "ID behind '@'");
                        }
                        else {
                            headerList.reset();
                            headerList.set_first_header(new Header(str), getCurrentLocation(in));
                            header = headerList.first_header();
                        }
                        while (gl->lastchar == ',' && !gl->error_flag) {
                            get_byte(in);
                            char *str2 = parse_token(in, foundTokenType);
                            if (foundTokenType != TOK_AT_WORD) p_err_exp_atWord(in);
                            else headerList.append(new Header(str2));
                            free(str2);
                        }
                        if (!gl->error_flag) expect_and_skip(in, ';');
                        if (!gl->error_flag) {
                            aisc_assert(headerList.first_header()->get_key());
                            reached_end_of_list = true;
                        }
                    }
                    break;

                case TOK_WORD: {
                    Token *new_token = NULL;
                    if (header) {
                        new_token = new Token(header->get_key(), str);
                        expect_line_terminator(in);
                    }
                    else {
                        char *str2 = parse_token(in, foundTokenType);
                        switch (foundTokenType) {
                            case TOK_BRACE_OPEN: {
                                char           *openLoc = strdup(getCurrentLocation(in));
                                TokenListBlock *block   = parse_aisc_TokenListBlock(in);
                                aisc_assert(block || gl->error_flag);
                                if (!gl->error_flag) {
                                    aisc_assert(block);
                                    new_token = new Token(str, block);
                                    block     = NULL;
                                    expect_and_skip_closing_brace(in, openLoc);
                                }
                                delete block;
                                free(openLoc);
                                break;
                            }
                            case TOK_WORD:
                                new_token = new Token(str, str2);
                                expect_line_terminator(in);
                                break;

                            case TOK_COMMA:
                            case TOK_SEMI:
                                new_token = new Token(str, "");
                                break;

                            case TOK_AT_WORD:     p_err_exp_string_but_saw(in, "'@'"); break;
                            case TOK_BRACE_CLOSE: p_err_exp_string_but_saw(in, "'}'"); break;

                            case TOK_INVALID: aisc_assert(0); break;
                        }
                        free(str2);
                    }

                    aisc_assert(new_token || gl->error_flag);

                    if (new_token) items->append(new_token);

                    if (!gl->error_flag) {
                        if (gl->lastchar == ';') {
                            const Header *missingVal = header ? header->next_header() : NULL;
                            if (missingVal) {
                                char buf[1000];
                                sprintf(buf, "value for @%s", missingVal->get_key());
                                p_err_exp_but_saw(in, buf, "';'");
                            }
                            else reached_end_of_list = true;
                            get_byte(in);
                        }
                        else get_byte(in);
                    }
                    break;
                }

                case TOK_INVALID:
                    p_err(in, "Invalid token (internal error)");
                    break;
            }
        }

        if (!gl->error_flag && header) header = header->next_header();

        free(str);
    }

    if (gl->error_flag || items->empty()) {
        delete items;
        items = NULL;
    }

    return items;
}

TokenListBlock *parse_aisc_TokenListBlock(const char *& in) {
    TokenListBlock *block = new TokenListBlock;
    HeaderList      headerList;

    while ((EOSTR != gl->lastchar) && (gl->lastchar != '}')) {
        TokenList *list = parse_aisc_TokenList(in, headerList);
        if (!gl->error_flag && list) {
            block->append(list);
        }
    }

    if (block->empty() || gl->error_flag) {
        delete block;
        block = NULL;
    }

    return block;
}

CL *calloc_CL() {
    return (CL *) calloc(sizeof(CL), 1);
}
void free_CL(CL *cl) {
    while (cl) {
        CL *next = cl->next;

        free(cl->str);
        free(cl->path);
        free(cl);

        cl = next;
    }
}

static CL *read_prog(const char *in, const char *file) {
    CL *first_cl = NULL;
    CL *cl       = NULL;
    gl->line_cnt = 0;

    while (gl->lastchar != EOSTR) {
        skip_over_spaces_and_comments_multiple_lines(in);

        if (gl->lastchar == EOSTR) break;

        const char *p = in-1;
        while ((gl->lastchar != EOSTR) && (gl->lastchar != '\n')) get_byte(in);

        {
            CL    *hcl  = calloc_CL();
            CL *&  next = cl ? cl->next : first_cl;
            cl          = next = hcl;
        }
        cl->str    = copy_string_part(p, in-2);
        cl->path   = strdup(file);
        cl->linenr = gl->line_cnt;
    }

    return first_cl;
}



int main(int argc, char ** argv) {
    int exitcode = EXIT_FAILURE;

    if (argc < 2) {
        fprintf(stderr, "AISC - ARB integrated source compiler\n");
        fprintf(stderr, "Usage: aisc [fileToCompile]+\n");
        fprintf(stderr, "Error: missing file name\n");
    }
    else {
        aisc_init();
        {
            char abuf[20];
            for (int i=0; i<argc; i++) {
                sprintf(abuf, "argv[%i]", i);
                write_hash(gl->st->hs, abuf, argv[i]);
            }
            sprintf(abuf, "%i", argc);
            write_hash(gl->st->hs, "argc", abuf);
        }

        char *buf = read_aisc_file(argv[1]);
        if (buf) {
            gl->prg = read_prog(buf, argv[1]);
            if (gl->prg) {
                aisc_calc_special_commands();
                CL *co = aisc_calc_blocks(gl->prg, 0, 0, 0);
                if (co) {
                    print_error("unexpected end of file");
                }
                else {
                    int errorCode = run_prg();
                    if (errorCode) {
                        aisc_remove_files();
                        fflush(stdout);
                    }
                    else {
                        exitcode = EXIT_SUCCESS;
                    }
                }
            }
        }
        free(buf);
        aisc_exit();
    }
    return exitcode;
}
