//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //

#ifndef AISC_PARSER_H
#define AISC_PARSER_H

#ifndef AISC_TOKEN_H
#include "aisc_token.h"
#endif
#ifndef AISC_LOCATION_H
#include "aisc_location.h"
#endif
#ifndef AISC_INLINE_H
#include "aisc_inline.h"
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif

enum CommandType {
    NO_COMMAND, 
    CT_IF = 1,
    CT_ENDIF,
    CT_ELSE,
    CT_FOR,
    CT_NEXT,
    CT_ENDFOR,
    CT_ELSEIF,
    CT_FUNCTION,
    CT_LABEL,
    CT_OTHER_CMD,
};

struct Code {
    Code *next;
    char *str;

    Location source;

    CommandType    command;
    const Command *cmd;

    mutable struct for_data *fd;

    Code *IF;
    Code *ELSE;
    Code *ENDIF;
    Code *FOR;
    Code *NEXT;
    Code *ENDFOR;

    Code() {
        memset(this, 0, sizeof(*this));
    }
    Code(const Code& other)
        : next(other.next),
          str(nulldup(other.str)),
          source(other.source), 
          command(other.command),
          cmd(other.cmd),
          fd(other.fd),
          IF(other.IF),
          ELSE(other.ELSE),
          ENDIF(other.ENDIF),
          FOR(other.FOR),
          NEXT(other.NEXT),
          ENDFOR(other.ENDFOR)
    {}

    DECLARE_ASSIGNMENT_OPERATOR(Code);
    ~Code() {
        delete next;
        free(str);
    }

    void set_command(CommandType command_, const char *args) {
        command = command_;
        SKIP_SPACE_LF(args);
        freedup(str, args);
    }

    void print_error_internal(const char *err, const char *launcher_file, int launcher_line) const {
        source.print_error_internal(err, launcher_file, launcher_line);
    }
    void print_warning_internal(const char *warn, const char *launcher_file, int launcher_line) const {
        source.print_warning_internal(warn, launcher_file, launcher_line);
    }
};


class Parser : virtual Noncopyable {
    // used to parse 'Data' and 'Code'
    
    int         lastchar;
    const char *last_line_start;

    Location loc;

    int error_flag;

    void get_byte(const char *& io) {
        lastchar = *(io++);
        if (is_LF(lastchar)) {
            last_line_start = io;
            ++loc;
        }
    }

    const char *currentLocation(const char *io);

    void p_err(const char *io, const char *error);
    void p_errf(const char *io, const char *formatString, ...) __ATTR__FORMAT_MEMBER(2);
    
    void p_err_empty_braces(const char *io) { p_err(io, "{} found, missing contents"); }
    void p_err_exp_line_terminator(const char *io) { p_err(io, "missing ',' or ';' or 'newline'"); }
    void p_err_ill_atWord(const char *io) { p_err(io, "only header definitions may start with '@'"); }
    void p_err_exp_atWord(const char *io) { p_err(io, "all words in header-definitions have to start with '@'"); }
    
    void p_err_expected(const char *io, const char *expect)                     { p_errf(io, "Expected to see %s", expect); }
    void p_err_exp_but_saw(const char *io, const char *expect, const char *saw) { p_errf(io, "Expected to see %s, but saw %s", expect, saw); }

    void p_err_exp_string_but_saw(const char *io, const char *saw) { p_err_exp_but_saw(io, "string", saw); }
    void p_err_exp_but_saw_EOF(const char *io, const char *expect) { p_err_exp_but_saw(io, expect, "end of file"); }

    inline void expect_line_terminator(const char *in) {
        if (!is_SEP_LF_EOS(lastchar)) p_err_exp_line_terminator(in);
    }

    void expect_and_skip_closing_brace(const char *& in, const char *openingBraceLocation) {
        if (lastchar != '}') {
            p_err_expected(in, "'}'");
            fprintf(stderr, "%s: opening brace was here\n", openingBraceLocation);
        }
        get_byte(in);
    }
    void expect_and_skip(const char *& in, char expect) {
        if (lastchar != expect) {
            char buf[] = "'x'";
            buf[1]     = expect;
            p_err_expected(in, buf);
        }
        get_byte(in);
    }

    void skip_over_spaces(const char *& in) { while (is_SPACE(lastchar)) get_byte(in); }
    void skip_over_spaces_and_comments(const char *& in) {
        skip_over_spaces(in);
        if (lastchar == '#') { // comment -> skip rest of line
            while (!is_LF_EOS(lastchar)) get_byte(in);
        }
    }
    void skip_over_spaces_and_comments_multiple_lines(const char *& in) {
        while (1) {
            skip_over_spaces_and_comments(in);
            if (!is_LF(lastchar)) break;
            get_byte(in);
        }
    }

    void copyWordTo(const char*& in, char*& out) {
        while (!is_SPACE_SEP_LF_EOS(lastchar)) {
            *(out++) = lastchar;
            get_byte(in);
        }
    }

    void  copyTillQuotesTo(const char*& in, char*& out);
    char *readWord(const char *& in);

    char *SETSOURCE(const char *& in, enum TOKEN& foundTokenType);
    char *parse_token(const char *& in, enum TOKEN& foundTokenType);

    Token *parseBrace(const char *& in, const char *key);
    TokenList *parseTokenList(const char *& in, class HeaderList& headerList);

public:
    Parser() {
        lastchar = ' ';
        last_line_start = NULL;
        error_flag      = 0;
    }

    int get_sourceline() const { return loc.get_linenr(); }
    const char *get_sourcename() const { return loc.get_path(); }
    const Location& get_location() const { return loc; }

    void set_source(const Location& other) {
        aisc_assert(loc != other);
        loc = other;
    }
    void set_source(const char *path, int linenumber) {
        set_source(Location(linenumber, path));
    }

    void set_line_start(const char *start, int offset_in_line) {
        last_line_start = start-offset_in_line;
        lastchar        = ' ';
    }

    TokenListBlock *parseTokenListBlock(const char *& in);
    class Code *parse_program(const char *in, const char *filename);
};


#else
#error aisc_parser.h included twice
#endif // AISC_PARSER_H
