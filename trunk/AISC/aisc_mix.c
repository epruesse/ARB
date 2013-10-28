// ================================================================
/*                                                                  */
//   File      : aisc_mix.c
//   Purpose   :
/*                                                                  */
//   Institute of Microbiology (Technical University Munich)
//   http://www.arb-home.de/
/*                                                                  */
// ================================================================

#include "aisc_interpreter.h"

static Code *aisc_calc_blocks(Code * co, Code * afor, Code * aif, int up) {
    Code *oif;
    Code *ofor;
    Code *aelse;
    Code *anext;

    while (co) {
        while (co && (!co->command)) {
            co->IF = aif;
            co->FOR = afor;
            co = co->next;
        }
        if (!co) return 0;
        co->IF = aif;
        co->FOR = afor;
        switch (co->command) {
            case CT_NEXT:
                if (!co->FOR) print_error(co, "NEXT without FOR");
                return co;
            case CT_ENDFOR:
                if (!co->FOR) print_error(co, "ENDFOR without FOR");
                return co;
            case CT_ELSE:
                if (!co->IF) print_error(co, "ELSE without IF");
                return co;
            case CT_ELSEIF:
                if (!co->IF) print_error(co, "ELSEIF without IF");
                return co;
            case CT_ENDIF:
                if (!co->IF) print_error(co, "ENDIF without IF");
                return co;

            case CT_IF:
                oif = aif;
                aif = co;
                co  = aisc_calc_blocks(co->next, afor, aif, 0);
                if (!co) {
                    print_error(aif, "IF without ELSE or ENDIF");
                    return 0;
                }
                if (co->command == CT_ELSE) {
                    aif->ELSE=co;
                    aelse = co;
                    co = aisc_calc_blocks(co->next, afor, aif, 0);
                    if (!co) {
                        print_error(aif, "ELSE without ENDIF");
                        return 0;
                    }
                    if  (co->command!=CT_ENDIF) {
                        print_error(aif, "ELSE without ENDIF");
                        print_error(co, "<detected here>");
                        return 0;
                    }
                    aif->ENDIF=co;
                    aelse->ENDIF=co;
                    if (up) return co;
                }
                else if (co->command == CT_ELSEIF) {
                    Code *co_if    = new Code(*co);
                    co_if->command = CT_IF;   // when jumped to (from prev failed IF, act like IF)
                    co->command    = CT_ELSE; // when running into, act like else
                    co ->next      = co_if;
                    freenull(co->str);
                    aif->ELSE = co;
                    aelse     = co;
                    co        = aisc_calc_blocks(co_if, afor, aif, 1);
                    if (!co) {
                        print_error(aif->ELSE, "ELSEIF without ENDIF or ELSE");
                        return 0;
                    }
                    if  (co->command!=CT_ENDIF) {
                        print_error(aif->ELSE, "ELSEIF without ENDIF");
                        print_error(co, "<detected here>");
                        return 0;
                    }
                    aif->ENDIF   = co;
                    co_if->ENDIF = co;
                    aelse->ENDIF = co;
                    if (up) return co;
                }
                else if (co->command == CT_ENDIF) {
                    aif->ELSE  = co;
                    aif->ENDIF = co;
                    if (up) return co;
                }
                else {
                    print_error(aif, "IF without ELSE or ENDIF");
                    print_error(co, "<detected here>");
                    return 0;
                }
                aif = oif;
                break;
            case CT_FOR:
                ofor = afor;
                afor = co;
                co = aisc_calc_blocks(co->next, afor, aif, 0);
                if (!co) {
                    print_error(afor, "FOR without NEXT or ENDFOR");
                    return 0;
                }
                if (co->command == CT_NEXT) {
                    afor->NEXT=co;
                    anext = co;
                    co = aisc_calc_blocks(co->next, afor, aif, 0);
                    if (!co) {
                        print_error(afor->NEXT, "NEXT without ENDFOR");
                        return 0;
                    }
                    if  (co->command!=CT_ENDFOR) {
                        print_error(afor->NEXT, "NEXT without ENDFOR");
                        print_error(co, "<detected here>");
                        return 0;
                    }
                    afor->ENDFOR  = co;
                    anext->ENDFOR = co;

                }
                else if (co->command == CT_ENDFOR) {
                    afor->ENDFOR = co;
                    afor->NEXT   = co;
                    co->command  = CT_NEXT;
                }
                else {
                    print_error(afor, "FOR without NEXT or ENDFOR");
                    print_error(co, "<detected here>");
                    return 0;
                }
                afor = ofor;
                break;
            default:
                break;

        }
        co = co->next;
    }
    return 0;
}

int Interpreter::compile_program() {
    for (Code *co=prg; co; co=co->next) {
        if (!strncmp(co->str, "IF", 2))     { co->set_command(CT_IF, co->str+2); continue; }
        if (!strncmp(co->str, "ELSEIF", 6)) { co->set_command(CT_ELSEIF, co->str+6); continue; }
        if (!strncmp(co->str, "ELSE", 4))   { co->set_command(CT_ELSE, co->str+4); continue; }
        if (!strncmp(co->str, "ENDIF", 5))  { co->set_command(CT_ENDIF, co->str+5); continue; }
        if (!strncmp(co->str, "FOR", 3))    { co->set_command(CT_FOR, co->str+3); continue; }
        if (!strncmp(co->str, "ENDFOR", 6)) { co->set_command(CT_ENDFOR, co->str+6); continue; }
        if (!strncmp(co->str, "NEXT", 4))   { co->set_command(CT_NEXT, co->str+4); continue; }
        
        if (!strncmp(co->str, "LABEL", 5)) {
            co->set_command(CT_LABEL, co->str+5);
            define_fun(co->str, co);
            continue;
        }
        if (!strncmp(co->str, "FUNCTION", 8)) {
            char *buf1, *buf2;
            char *s, *s2;
            buf1 = co->str+8;
            co->command = CT_FUNCTION;
            SKIP_SPACE_LF(buf1);
            for (s=buf1; !is_SPACE_SEP_LF_EOS(*s); s++) ;
            if (*s) {
                *s = 0;
                s++;
                SKIP_SPACE_LF(s);
                s2 = strdup(s);
            }
            else {
                s2 = strdup("");
            }
            buf2    = strdup(buf1);
            free(co->str);
            co->str = s2;
            define_fun(buf2, co);
            free(buf2);
            continue;
        }

        co->command = CT_OTHER_CMD;
        co->cmd     = find_command(co);
    }

    Code *co = aisc_calc_blocks(prg, 0, 0, 0);
    if (co) {
        print_error(co, "program is not well formed");
    }

    return 0;
}

// -------------
//      hash

hash::hash(int size_) {
    size = size_;
    entries = (hash_entry **)calloc(sizeof(hash_entry *), size);
}
hash::~hash() {
    for (int i = 0; i<size; ++i) {
        hash_entry *enext;
        for (hash_entry *e=entries[i]; e; e=enext) {
            if (e->val) free(e->val);
            free(e->key);
            enext = e->next;
            free(e);
        }
    }
    free(entries);
}

int hash::Index(const char *key) const {
    const char *p = key;
    int         x = 1;
    char        c;
    
    while ((c=*(p++))) {
        x = (x<<1) ^ c;
    }
    x %= size;
    if (x<0) x += size;
    return x;
}

const hash_entry *hash::find_entry(const char *key, int idx) const {
    for (hash_entry *e = entries[idx]; e; e=e->next) {
        if (!strcmp(e->key, key)) return e;
    }
    return 0;
}

void hash::write(const char *key, const char *val) {
    int         idx = Index(key);
    hash_entry *e   = find_entry(key, idx);
    if (e) {
        freeset(e->val, nulldup(val));
    }
    else {
        e       = (hash_entry *)calloc(sizeof(hash_entry), 1);
        e->next = entries[idx];
        e->key  = strdup(key);
        e->val  = val ? strdup(val) : 0;
        
        entries[idx] = e;
    }
}

var_ref Interpreter::get_local(const char *key) {
    var_ref ref;
    for (Stack *s = stack; s && !ref; s = s->next) {
        ref = s->hs->ref(key);
    }
    return ref;
}

const char *Interpreter::read_local(const char *key) const {
    const var_ref local = const_cast<Interpreter*>(this)->get_local(key);
    return local.read();
}

