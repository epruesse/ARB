// ================================================================ //
//                                                                  //
//   File      : aisc.h                                             //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef AISC_H
#define AISC_H

#define SIMPLE_ARB_ASSERT
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef DUPSTR_H
#include <dupstr.h>
#endif

#define aisc_assert(cond) arb_assert(cond)

// ------------------------------------------------------------

#define OPENFILES 16
#define HASHSIZE  1024
#define STACKSIZE 20

enum aisc_commands {
    no_command,
    IF,
    ENDIF,
    ELSE,
    FOR,
    NEXT,
    ENDFOR,
    ELSEIF,
    FUNCTION,
    LABEL,
    MAX_COM
};

struct hash_entry {
    char *key;
    char *val;
    struct hash_entry *next;
};
typedef struct hash_struct {
    int size;
    struct hash_entry **entries;
} hash;

// ------------------------------------------------------------

inline char *copy_string_part(const char *first, const char *last) {
    int   size = last-first+1;
    char *mem  = (char*)malloc(size+1);

    memcpy(mem, first, size);
    mem[size] = 0;

    return mem;
}

// ------------------------------------------------------------
// structures holding data read from *.aisc files

class TokenList;
class TokenListBlock;

// --------------
//      Token

class Token {
    Token     *next;                                // (owned)
    TokenList *parent;

    bool  isBlock;
    char *key;
    union {
        TokenListBlock *sub;                        // (owned), NULL = empty block
        char           *val;                        // NULL means ""
    } content;

public:
    Token(const char *key_, const char *val_)
        : next(NULL)
        , parent(NULL)
        , isBlock(false)
        , key(strdup(key_))
    {
        content.val = val_ ? strdup(val_) : NULL;
    }
    Token(const char *key_, TokenListBlock *block_); // takes ownage of block_
    ~Token();

    void append(Token *tok) { next = tok; }
    void set_parent(TokenList *list) { aisc_assert(!parent); parent = list; }
    
    const char *get_key() const { return key; }

    bool is_block() const { return isBlock; }
    const TokenListBlock *get_content() const { aisc_assert(isBlock); return content.sub; }

    bool has_value() const { return !is_block() && content.val; }
    const char *get_value() const { aisc_assert(has_value()); return content.val; }

    const Token *next_token() const { return next; }
    const TokenList *parent_list() const { return parent; }
    const Token *parent_block_token() const;
};

// ------------------
//      TokenList

class TokenList {
    Token          *head;                           // list of tokens (owned)
    Token          *tail;
    TokenList      *next;                           // owned
    TokenListBlock *parent;

public:
    TokenList() {
        head   = NULL;
        tail   = NULL;
        next   = NULL;
        parent = NULL;
    }
    ~TokenList() {
        delete head;
        delete next;
    }

    void append(Token *tok) {
        if (!head) head = tok;
        else tail->append(tok);

        tail = tok;
        tok->set_parent(this);
    }
    void append(TokenList *cmd) { next = cmd; }
    void set_parent(TokenListBlock *block) { parent = block; }

    bool empty() const { return !head; }
    const Token *first_token() const { return head; }
    const TokenList *next_list() const { return next; }
    const TokenListBlock *parent_block() const { return parent; }
};

// -----------------------
//      TokenListBlock

class TokenListBlock {
    TokenList   *head;                              // list of TokenLists (owned)
    TokenList   *tail;
    const Token *parent;

public:
    TokenListBlock() {
        head   = NULL;
        tail   = NULL;
        parent = NULL;
    }
    ~TokenListBlock() { delete head; }

    bool empty() const { return !head; }
    void append(TokenList *cmd) {
        if (!head) head = cmd;
        else tail->append(cmd);

        tail = cmd;
        cmd->set_parent(this);
    }
    void set_block_token(Token *tok) { aisc_assert(!parent); parent = tok; }

    const TokenList *first_list() const { return head; }
    const Token *first_token() const { return head->first_token(); }
    const Token *block_token() const { return parent; }
};

// ------------------------------------------------------------

inline Token::Token(const char *key_, TokenListBlock *block_)
    : next(NULL)
    , parent(NULL)
    , isBlock(true)
    , key(strdup(key_))
{
    aisc_assert(block_);
    content.sub = block_;
    if (content.sub) {
        content.sub->set_block_token(this);
    }
}
inline Token::~Token() {
    free(key);
    if (isBlock) delete content.sub;
    else free(content.val);
    delete next;
}
inline const Token *Token::parent_block_token() const {
    return parent_list()->parent_block()->block_token();
}

// ------------------------------------------------------------

struct for_data_struct {
    char        *forstr;
    const Token *forcursor;
    long         forval;
    long         forend;

    struct for_data_struct *next;
};



typedef struct command_lines {
    struct command_lines   *next;
    char                   *str;
    int                     linenr;
    char                   *path;
    enum aisc_commands      command;
    struct for_data_struct *fd;
    struct command_lines   *IF;
    struct command_lines   *ELSE;
    struct command_lines   *ENDIF;
    struct command_lines   *FOR;
    struct command_lines   *NEXT;
    struct command_lines   *ENDFOR;
} CL;

struct stack_struct {
    const Token *cursor;
    CL          *pc;
    hash        *hs;

    struct stack_struct *next;
};

struct param_struct {
    char *param;
    struct param_struct *next;
};

struct global_struct {
    int                  error_flag;
    char                 outtab[256];
    const Token         *cursor;
    TokenListBlock      *root;
    CL                  *prg;
    CL                  *pc;
    CL                  *nextpc;
    struct stack_struct *st;
    int                  sp;
    int                  line_cnt;
    const char          *last_line_start;
    char                *line_path;
    int                  lastchar;
    char                *linebuf;
    int                  bufsize;
    int                  s_pos;
    FILE                *out;
    FILE                *outs[OPENFILES];
    char                *fouts[OPENFILES];          // type of file
    char                *fouts_name[OPENFILES];     // file-system-name of file

    int   tabstop;
    int   tabs[10];
    hash *fns;
    int   tabpos;

};

extern struct global_struct *gl;
extern char                  string_buf[256];

#define EOSTR    0
#define BEG_STR1 '('
#define BEG_STR2 '~'
#define END_STR1 '~'
#define END_STR2 ')'

inline bool is_SPACE(char c) { return c == ' '|| c == '\t'; }
inline bool is_SEP(char c) { return c == ',' || c == ';'; }
inline bool is_LF(char c) { return c == '\n'; }
inline bool is_EOS(char c) { return c == EOSTR; }

inline bool is_SPACE_LF(char c) { return is_SPACE(c) || is_LF(c); }
inline bool is_SPACE_LF_EOS(char c) { return is_SPACE(c) || is_LF(c) || is_EOS(c); }
inline bool is_SPACE_SEP_LF_EOS(char c) { return is_SEP(c) || is_SPACE_LF_EOS(c); }
inline bool is_SEP_LF_EOS(char c) { return is_SEP(c) || is_LF(c) || is_EOS(c); }
inline bool is_LF_EOS(char c) { return is_LF(c) || is_EOS(c); }

inline void SKIP_SPACE_LF(char *& var) { while (is_SPACE_LF(*var)) ++var; }
inline void SKIP_SPACE_LF_BACKWARD(char *& var) { while (is_SPACE_LF(*var)) --var; }

enum LookupScope {
    LOOKUP_LIST,
    LOOKUP_BLOCK,
    LOOKUP_BLOCK_REST,
    LOOKUP_LIST_OR_PARENT_LIST,
};

#include "aisc_proto.h"

// #define SHOW_CALLER // show where error was raised

#ifdef SHOW_CALLER

#define print_error(err)          print_error_internal(err, __FILE__, __LINE__)
#define print_warning(err)        print_warning_internal(err, __FILE__, __LINE__)
#define printf_error(format, arg) print_error_internal(formatted(format, arg), __FILE__, __LINE__)

#else

#define print_error(err)          print_error_internal(err, NULL, 0)
#define print_warning(err)        print_warning_internal(err, NULL, 0)
#define printf_error(format, arg) print_error_internal(formatted(format, arg), NULL, 0)

#endif

#else
#error aisc.h included twice
#endif // AISC_H
