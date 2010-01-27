/* ================================================================ */
/*                                                                  */
/*   File      : aisc_mix.c                                         */
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


CL *aisc_calc_blocks(CL * co, CL * afor, CL * aif, int up) {
    CL *oif;
    CL *ofor;
    CL *aelse;
    CL *anext;
    
    while (co) {
        while (co && (!co->command)) {
            gl->pc = co;
            co->IF = aif;
            co->FOR = afor;
            co = co->next;
        }
        if (!co) return 0;
        gl->pc = co;
        co->IF = aif;
        co->FOR = afor;
        gl->pc = co;
        switch (co->command) {
            case NEXT:
            case ENDFOR:
            case ELSE:
            case ELSEIF:
            case ENDIF:
                return co;
            case IF:
                oif = aif;
                aif = co;
                co = aisc_calc_blocks(co->next, afor, aif, 0);
                if (!co) {
                    print_error("IF without ELSE or ENDIF");
                    return 0;
                }
                if (co->command == ELSE) {
                    aif->ELSE=co;
                    aelse = co;
                    co = aisc_calc_blocks(co->next, afor, aif, 0);
                    if (!co) {
                        gl->pc = aif;
                        print_error("ELSE without ENDIF");
                        return 0;
                    }
                    if  (co->command!=ENDIF) {
                        print_error("ELSE without ENDIF");
                        return 0;
                    }
                    aif->ENDIF=co;
                    aelse->ENDIF=co;
                    if (up) return co;
                }
                else if (co->command == ELSEIF) {
                    CL *cod;
                    cod = calloc_CL();
                    *cod = *co;
                    cod->command = IF;
                    co->command = ELSE;
                    co ->next = cod;
                    co->str = NULL;
                    co->path = strdup(co->path);
                    aif->ELSE=co;
                    aelse = co;
                    co = aisc_calc_blocks(cod, afor, aif, 1);
                    if (!co) {
                        gl->pc = aif;
                        print_error("ELSEIF without ENDIF or ELSE");
                        return 0;
                    }
                    if  (co->command!=ENDIF) {
                        print_error("ELSEIF without ENDIF");
                        return 0;
                    }
                    aif->ENDIF=co;
                    cod->ENDIF=co;
                    aelse->ENDIF=co;
                    if (up) return co;
                }
                else if (co->command == ENDIF) {
                    aif->ELSE=co;
                    aif->ENDIF=co;
                    if (up) return co;
                }
                else {
                    print_error("IF without ELSE or ENDIF");
                    return 0;
                }
                aif = oif;
                break;
            case FOR:
                ofor = afor;
                afor = co;
                co = aisc_calc_blocks(co->next, afor, aif, 0);
                if (!co) {
                    gl->pc = afor;
                    print_error("FOR without NEXT or ENDFOR");
                    return 0;
                }
                if (co->command == NEXT) {
                    afor->NEXT=co;
                    anext = co;
                    co = aisc_calc_blocks(co->next, afor, aif, 0);
                    if (!co) {
                        gl->pc = aif;
                        print_error("NEXT without ENDFOR");
                        return 0;
                    }
                    if  (co->command!=ENDFOR) {
                        print_error("NEXT without ENDFOR");
                        return 0;
                    }
                    afor->ENDFOR=co;
                    anext->ENDFOR=co;

                }
                else if (co->command == ENDFOR) {
                    afor->ENDFOR=co;
                    afor->NEXT=co;
                    co->command = NEXT;
                }
                else {
                    print_error("FOR without NEXT or ENDFOR");
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

int aisc_calc_special_commands()
{
    CL *co;
    char *buf1, *buf2;
    for (co=gl->prg; co; co=co->next) {
        if (!strncmp(co->str, "IF", 2)) {
            buf1 = co->str+2;
            co->command = IF;
            SKIP_SPACE_LF(buf1);
            buf2 = strdup(buf1);
            free(co->str);
            co->str = buf2;
            continue;
        }
        if (!strncmp(co->str, "ELSEIF", 6)) {
            buf1 = co->str+6;
            co->command = ELSEIF;
            SKIP_SPACE_LF(buf1);
            buf2 = strdup(buf1);
            free(co->str);
            co->str = buf2;
            continue;
        }
        if (!strncmp(co->str, "ELSE", 4)) {
            buf1 = co->str+4;
            co->command = ELSE;
            SKIP_SPACE_LF(buf1);
            buf2 = strdup(buf1);
            free(co->str);
            co->str = buf2;
            continue;
        }
        if (!strncmp(co->str, "ENDIF", 5)) {
            buf1 = co->str+5;
            co->command = ENDIF;
            SKIP_SPACE_LF(buf1);
            buf2 = strdup(buf1);
            free(co->str);
            co->str = buf2;
            continue;
        }
        if (!strncmp(co->str, "FOR", 3)) {
            buf1 = co->str+3;
            co->command = FOR;
            SKIP_SPACE_LF(buf1);
            buf2 = strdup(buf1);
            free(co->str);
            co->str = buf2;
            continue;
        }
        if (!strncmp(co->str, "ENDFOR", 6)) {
            buf1 = co->str+6;
            co->command = ENDFOR;
            SKIP_SPACE_LF(buf1);
            buf2 = strdup(buf1);
            free(co->str);
            co->str = buf2;
            continue;
        }
        if (!strncmp(co->str, "NEXT", 4)) {
            buf1 = co->str+4;
            co->command = NEXT;
            SKIP_SPACE_LF(buf1);
            buf2 = strdup(buf1);
            free(co->str);
            co->str = buf2;
            continue;
        }
        if (!strncmp(co->str, "FUNCTION", 8)) {
            char *s, *s2;
            buf1 = co->str+8;
            co->command = FUNCTION;
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
            sprintf(string_buf, "%li", (long)co);
            write_hash(gl->fns, buf2, string_buf);
            free(buf2);
            continue;
        }
        if (!strncmp(co->str, "LABEL", 5)) {
            buf1 = co->str+5;
            co->command = LABEL;
            SKIP_SPACE_LF(buf1);
            buf2 = strdup(buf1);
            free(co->str);
            co->str = buf2;
            sprintf(string_buf, "%li", (long)co);
            write_hash(gl->fns, buf2, string_buf);
            continue;
        }
    }
    return 0;
}

static int hash_index(const char *key, int size)
{
    int         x;
    const char *p;
    char        c;

    p = key;
    x = 1;
    while ((c=*(p++))) {
        x = (x<<1) ^ c;
    }
    x %= size;
    if (x<0) x += size;
    return x;
}


struct hash_struct *create_hash(int size)
{
    struct hash_struct *hs;
    hs = (struct hash_struct *)calloc(sizeof(struct hash_struct), 1);
    hs->size = size;
    hs->entries = (struct hash_entry **)calloc(sizeof(struct hash_entry *), size);
    return hs;
}

char *read_hash_local(char *key, struct hash_struct **hs)
{
    struct stack_struct *ss;
    int i;
    struct hash_entry *e;
    i = hash_index(key, gl->st->hs->size);
    for (ss = gl->st; ss; ss=ss->next) {
        for (e=ss->hs->entries[i]; e; e=e->next)
        {
            if (!strcmp(e->key, key)) {
                if (hs) *hs = ss->hs;
                return e->val;
            }
        }
    }
    return 0;
}


char *read_hash(struct hash_struct *hs, char *key)
{
    struct hash_entry *e;
    int i;
    i = hash_index(key, hs->size);
    for (e=hs->entries[i]; e; e=e->next)
    {
        if (!strcmp(e->key, key)) return e->val;
    }
    return 0;
}

char *write_hash(struct hash_struct *hs, const char *key, const char *val)
{
    struct hash_entry *e;
    char *str2;
    int i;
    i = hash_index(key, hs->size);
    for (e=hs->entries[i]; e; e=e->next)
    {
        if (!strcmp(e->key, key)) {
            str2 = e->val;
            if (e->val) free(e->val);
            if (val) {
                e->val = strdup(val);
            }
            else {
                e->val = 0;
            }

            return str2;
        }
    }
    e = (struct hash_entry *)calloc(sizeof(struct hash_entry), 1);
    e->next = hs->entries[i];
    e->key = strdup(key);
    if (val)    e->val = strdup(val);
    hs->entries[i] = e;

    return 0;
}

int free_hash(struct hash_struct *hs) {
    int i;
    int e2;
    struct hash_entry *e, *ee;
    e2 = hs->size;
    for (i=0; i<e2; i++) {
        for (e=hs->entries[i]; e; e=ee) {
            if (e->val) free(e->val);
            free(e->key);
            ee = e->next;
            free((char *)e);
        }
    }
    free(hs->entries);
    free(hs);
    return 0;
}
