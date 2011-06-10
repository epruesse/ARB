//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //

#ifndef AISC_TOKEN_H
#define AISC_TOKEN_H

#ifndef AISC_DEF_H
#include "aisc_def.h"
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

// ------------------------------------------------------------
// structures holding data read from *.aisc files

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



class TokenList;
class TokenListBlock;

// --------------
//      Token

class Token : virtual Noncopyable {
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

class TokenList : virtual Noncopyable {
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

class TokenListBlock : virtual Noncopyable {
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

#else
#error aisc_token.h included twice
#endif // AISC_TOKEN_H
