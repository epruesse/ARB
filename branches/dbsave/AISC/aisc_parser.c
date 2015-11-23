//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //

#include "aisc_parser.h"

const char *Parser::currentLocation(const char *io) {
    const int   LOCBUFFERSIZE = 1024;
    static char loc_buf[LOCBUFFERSIZE+1];
    int         printed;

    if (last_line_start) {
        int column = io - last_line_start + 1;

        aisc_assert(column >= 0 && column<10000);
        printed = sprintf(loc_buf, "%s:%i:%i", loc.get_path(), loc.get_linenr(), column);
    }
    else {
        printed = sprintf(loc_buf, "%s:%i", loc.get_path(), loc.get_linenr());
    }

    if (printed>LOCBUFFERSIZE) {
        fprintf(stderr, "AISC: Internal buffer overflow detected -- terminating [loc_buf]\n");
        error_flag = 1;
    }

    return loc_buf;
}

void Parser::p_err(const char *io, const char *error) {
    fprintf(stderr, "%s: Error: %s\n", currentLocation(io), error);
    error_flag = 1;
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
void Parser::p_errf(const char *io, const char *formatString, ...) {
    static char buf[ERRBUFFERSIZE+1];
    int         printed;
    va_list     varArgs;

    va_start(varArgs, formatString);
    printed = PRINT2BUFFER(buf, ERRBUFFERSIZE, formatString, varArgs);
    va_end(varArgs);

    if (printed>ERRBUFFERSIZE) {
        fprintf(stderr, "AISC: Internal buffer overflow detected -- terminating [err]\n");
        error_flag = 1;
    }
    p_err(io, buf);
}

void Parser::copyTillQuotesTo(const char*& in, char*& out) {
    // closing quotes are END_STR1 plus END_STR2
    bool quotes_closed = false;
    while (!quotes_closed) {
        while ((lastchar != EOSTR) && (lastchar != END_STR1)) {
            *(out++) = lastchar;
            get_byte(in);
        }

        if (lastchar == END_STR1) {
            get_byte(in);
            if (lastchar == END_STR2) {
                get_byte(in);
                quotes_closed = true;
            }
            else {
                *(out++) = END_STR1;
            }
        }
    }
}

char *Parser::readWord(const char *& in) {
    char  buf[1024];
    char *cp = buf;

    if (lastchar == BEG_STR1) {
        get_byte(in);
        if (lastchar == BEG_STR2) {
            get_byte(in);
            copyTillQuotesTo(in, cp);
        }
        else {
            *(cp++) = BEG_STR1;
            copyWordTo(in, cp);
        }
    }
    else copyWordTo(in, cp);

    if (lastchar != EOSTR) skip_over_spaces_and_comments(in);

    char *result = NULL;
    if (lastchar == EOSTR) {
        p_err_exp_but_saw_EOF(in, "terminator after string");
    }
    else if (cp != buf) { // do not return empty string, return NULL instead
        *cp    = 0;
        result = strdup(buf);
    }

    return result;
}

char *Parser::SETSOURCE(const char *& in, enum TOKEN& foundTokenType) {
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
                char *filename = copy_string_part(file, comma-1);
                set_source(filename, atoi(comma+1));
                free(filename);

                in = end+1;
                get_byte(in);

                result = parse_token(in, foundTokenType);
            }
        }
    }
    return result;
}

char *Parser::parse_token(const char *& in, enum TOKEN& foundTokenType) {
    skip_over_spaces_and_comments_multiple_lines(in);

    char *result   = NULL;
    foundTokenType = TOK_INVALID;

    switch (lastchar) {
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
                if (lastchar == EOSTR) {
                    p_err_exp_but_saw_EOF(in, "ID behind '@'");
                }
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

class Header : virtual Noncopyable {
    char   *key;
    Header *next;
 public:
    Header(const char *key_) { key = strdup(key_); next = NULL; }
    ~Header() { free(key); delete next; }

    void set_next(Header *header) { aisc_assert(!next); next = header; }

    const char *get_key() const { return key; }
    const Header *next_header() const { return next; }
};

class HeaderList : virtual Noncopyable {
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

Token *Parser::parseBrace(const char *& in, const char *key) {
    Token          *res     = NULL;
    if (!error_flag) {
        char           *openLoc = strdup(currentLocation(in));
        TokenListBlock *block   = parseTokenListBlock(in);

        if (block) {
            res = new Token(key, block);
            block = NULL;
            expect_and_skip_closing_brace(in, openLoc);
        }
        else {
            p_err_empty_braces(in);
        }
        delete block;
        free(openLoc);
    }
    return res;
}

TokenList *Parser::parseTokenList(const char *& in, HeaderList& headerList) {
    TokenList    *items  = new TokenList;
    const Header *header = headerList.first_header();

    get_byte(in);

    bool reached_end_of_list = false;
    while (!error_flag && !reached_end_of_list) {
        TOKEN  foundTokenType;
        char  *str = parse_token(in, foundTokenType);

        if (!error_flag) {
            switch (foundTokenType) {
                case TOK_SEMI:
                case TOK_BRACE_CLOSE:
                    reached_end_of_list = true;
                    break;

                case TOK_BRACE_OPEN: {
                    Token *sub = parseBrace(in, header ? header->get_key() : "{");
                    if (sub) items->append(sub);
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
                            headerList.set_first_header(new Header(str), currentLocation(in));
                            header = headerList.first_header();
                        }
                        while (lastchar == ',' && !error_flag) {
                            get_byte(in);
                            char *str2 = parse_token(in, foundTokenType);
                            if (foundTokenType != TOK_AT_WORD) p_err_exp_atWord(in);
                            else headerList.append(new Header(str2));
                            free(str2);
                        }
                        if (!error_flag) expect_and_skip(in, ';');
                        if (!error_flag) {
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
                                new_token = parseBrace(in, str);
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

                    aisc_assert(new_token || error_flag);

                    if (new_token) items->append(new_token);

                    if (!error_flag) {
                        if (lastchar == ';') {
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

        if (!error_flag && header) header = header->next_header();

        free(str);
    }

    if (error_flag || items->empty()) {
        delete items;
        items = NULL;
    }

    return items;
}


TokenListBlock *Parser::parseTokenListBlock(const char *& in) {
    TokenListBlock *block = new TokenListBlock;
    HeaderList      headerList;

    while ((EOSTR != lastchar) && (lastchar != '}')) {
        TokenList *list = parseTokenList(in, headerList);
        if (!error_flag && list) {
            block->append(list);
        }
    }

    if (block->empty() || error_flag) {
        delete block;
        block = NULL;
    }

    return block;
}

Code *Parser::parse_program(const char *in, const char *filename) {
    Code *first_cl = NULL;
    Code *cl       = NULL;

    set_source(filename, 0);

    while (lastchar != EOSTR) {
        skip_over_spaces_and_comments_multiple_lines(in);

        if (lastchar == EOSTR) break;

        const char *p = in-1;
        while ((lastchar != EOSTR) && (lastchar != '\n')) get_byte(in);

        {
            Code    *hcl  = new Code;
            Code *&  next = cl ? cl->next : first_cl;
            cl          = next = hcl;
        }
        cl->str    = copy_string_part(p, in-2);
        cl->source = Location(loc.get_linenr(), filename);
    }

    return first_cl;
}

