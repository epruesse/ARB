/* ================================================================ */
/*                                                                  */
/*   File      : aisc_var_ref.c                                     */
/*   Purpose   :                                                    */
/*                                                                  */
/*   Institute of Microbiology (Technical University Munich)        */
/*   http://www.arb-home.de/                                        */
 /*                                                                  */
 /* ================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aisc.h"

class TokenMatcher {
    char *key;                  // NULL matches "any key"
    char *value;                // NULL matches "any value"

    char *copy_expression_part(const char *start, const char *end) {
        if (start >= end || (end == (start+1) && start[0] == '*')) return NULL;
        return copy_string_part(start, end);
    }

 public:
    TokenMatcher(const char *search_expr) {
        // search_expr is "key.value"
        //
        // "*.value"    searches value (regardless of key)
        // "key.*"      searches key (regardless of value)
        //
        // "key"    = "key." = "key.*"
        // ".value" = "*.value"

        const char *dot = strchr(search_expr, '.');
        if (!dot) {
            key = strdup(search_expr);
            value = NULL;
        }
        else {
            const char *end = strchr(dot+1, 0);

            key   = copy_expression_part(search_expr, dot-1);
            value = copy_expression_part(dot+1, end-1);
        }
    }

    ~TokenMatcher() {
        free(key);
        free(value);
    }

    bool matchesKeyOf(const Token *token) const {
        return !key || strcmp(token->get_key(), key) == 0;
    }
    bool matchesValueOf(const Token *token) const {
        return !value || (!token->is_block() && strcmp(value, token->get_value()) == 0);
    }
    bool matches(const Token *token) const {
        return matchesKeyOf(token) && matchesValueOf(token);
    }
};

static const Token *nextToken(const Token *tok, bool cont_in_next_list) {
    const Token *next_token = tok->next_token();
    if (!next_token && cont_in_next_list) {
        const TokenList *next_list  = tok->parent_list()->next_list();
        if (next_list) next_token = next_list->first_token();
    }
    return next_token;
}

static const Token *nextTokenMatching(const Token *tok, const TokenMatcher& wanted, bool cont_in_next_list) {
    while (tok) {
        if (wanted.matches(tok)) break;
        tok = nextToken(tok, cont_in_next_list);
    }
    return tok;
}

const Token *aisc_find_var(const Token *cursor, char *str, LookupScope scope) {
    TokenMatcher  wanted(str);
    const Token  *line  = cursor ? cursor : gl->root->first_token();
    const Token  *found = NULL;

    while (line && !found) {
        bool cont_in_next_list = false;
        switch (scope) {
            case LOOKUP_LIST_OR_PARENT_LIST:
            case LOOKUP_LIST:
                line              = line->parent_list()->first_token();
                break;
            case LOOKUP_BLOCK:
                line              = line->parent_list()->parent_block()->first_token();
                cont_in_next_list = true;
                break;
            case LOOKUP_BLOCK_REST:
                line              = nextToken(line, true);
                cont_in_next_list = true;
                break;
        }

        found = nextTokenMatching(line, wanted, cont_in_next_list);

        if (scope == LOOKUP_LIST_OR_PARENT_LIST) {
            line = line->parent_block_token();
        }
        else {
            line = 0;
        }
    }

    return found;
}

const Token *aisc_find_var_hier(const Token *cursor, char *str, LookupScope scope) {
    char        *slash;
    const Token *found;

    if (*str == '/') {
        cursor = 0;
        str++;
    }
    slash = strchr(str, '/');
    found = 0;
    while (slash) {
        *(slash++) = 0;

        found = aisc_find_var(cursor, str, scope);
        if (!found) return 0;
        if (!found->is_block()) return 0;

        cursor = found->get_content()->first_token();

        str   = slash;
        slash = strchr(str, '/');
        scope = LOOKUP_BLOCK;
    }
    found = aisc_find_var(cursor, str, scope);
    return found;

}


char *get_var_string(char *var) {
    /* Calculates result for an expression like '$(IDENT:fdg|"sdfg")' */
    ;
    ;

    SKIP_SPACE_LF(var);
    int use_path = 0;
    while (var[0] == '&') {
        use_path++;
        var++;
        SKIP_SPACE_LF(var);
    }

    char *doppelpunkt = strchr(var, ':'); if (doppelpunkt) *(doppelpunkt++) = 0;
    char *bar         = strchr(var, '|'); if (bar) *(bar++) = 0;

    char *in = read_hash_local(var, 0);
    if (in) {
        in = strdup(in);
    }
    else {
        const Token *cur = aisc_find_var_hier(gl->cursor, var, LOOKUP_LIST_OR_PARENT_LIST);
        if (!cur) {
            if (!bar) {
                if (gl->pc->command == IF) {
                    return 0;
                }
                printf_error("Ident '%s' not found", var);
                return 0;
            }
            return strdup(bar);
        }
        if (use_path) {
            in = strdup(cur->get_key());
            if (use_path>1) {
                while (1) {
                    const Token *up = cur->parent_block_token();
                    if (!up) break;

                    cur        = up;
                    int   len  = strlen(in) + strlen(cur->get_key());
                    char *buf1 = (char *) calloc(sizeof(char), len + 2);
                    sprintf(buf1, "%s/%s", cur->get_key(), in);
                    
                    free(in);
                    in = buf1;
                }

            }
        }
        else {
            if (cur->is_block()) {
                printf_error("Ident '%s' is a hierarchical type", var);
                return 0;
            }
            else {
                in = strdup(cur->has_value() ? cur->get_value() : "");
            }
        }
    }

    aisc_assert(in); // 'in' has to point to a heap copy

    if (doppelpunkt) {
        int  len = strlen(in);
        bool err = false;
        while (doppelpunkt && !err) {
            char *nextdp            = strchr(doppelpunkt, ':');
            if (nextdp) *(nextdp++) = 0;
            if (!doppelpunkt[0]) {
                print_error("Ident replacement is missing an ':'");
                err = true;
            }
            else {
                bar = strchr(doppelpunkt+1, '=');
                if (!bar) {
                    print_error("Ident replacement is missing an '='");
                    err = true;
                }
                else {
                    *(bar++)     = 0;
                    int findl    = strlen(doppelpunkt);
                    int replacel = strlen(bar);

                    char *buf2;
                    for (char *finds = strstr(in, doppelpunkt); finds; finds = strstr(buf2, doppelpunkt)) {
                        len += replacel - findl;

                        char *buf1   = (char *) calloc(sizeof(char), len + 1);
                        int   offset = finds - in;
                        
                        memcpy(buf1, in, offset);
                        memcpy(buf1 + offset, bar, replacel);

                        buf2 = buf1 + offset + replacel;
                        memcpy(buf2, in + offset + findl, len - replacel - offset);

                        free(in);
                        in = buf1;
                        
                        buf2 = in + offset + replacel;
                    }
                    doppelpunkt = nextdp;
                }
            }
        }
        if (err) {
            free(in);
            in = NULL;
        }
    }
    return in;
}
