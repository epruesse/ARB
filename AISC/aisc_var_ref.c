#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <malloc.h> */
#include "aisc.h"

static AD *aisc_match(AD * var, char *varid, char *varct) {
    if (varid) {
        if (strcmp(var->key, varid)) {
            return 0;
        }
    }
    if (varct) {
        if (var->sub) {
            return 0;
        }
        if (strcmp(var->val, varct)) {
            return 0;
        }
    }
    return var;
}
#define NEXT_ITEM(se,extended) if (extended) {  \
        if (se->next_item) se = se->next_item;  \
        else se=se->first_item->next_line;      \
    }else{                                      \
        se = se->next_item;                     \
    }

AD *aisc_find_var_hier(AD * cursor, char *str, int next, int extended, int goup) {
    char *slash;
    AD   *found;
    
    if (*str == '/') {
        cursor = 0;
        str++;
    }
    slash = strchr(str, '/');
    found = 0;
    while (slash) {
        *(slash++) = 0;
        found = aisc_find_var(cursor, str, next, extended, goup);
        if (!found)
            return 0;
        if (!found->sub) {
            return 0;
        }
        cursor = found->sub;
        goup = 0;
        extended = 1;
        str = slash;
        slash = strchr(str, '/');
        next = 0;
    }
    found = aisc_find_var(cursor, str, next, extended, goup);
    return found;

}

AD *aisc_find_var(AD * cursor, char *str, int next, int extended, int goup) {
    /* if next = 1 search next entry
     * else search all
     * if extended != 0 search in parallel sentences,
     * if goup search whole upper hierarchy
     */
    AD             *se, *line;
    AD             *found;
    char           *varid, *varct;
    char           *pp, *buf;
    found = 0;

    if (cursor) {
        line = cursor;
    } else {
        line = gl->root;
    }

    pp = strchr(str, '.');
    if (!pp) {
        varid = strdup(str);
        varct = 0;
    } else {
        buf = strdup(str);
        pp = strchr(buf, '.');
        *(pp++) = 0;
        if ((!strcmp(buf, "*")) || (!buf[0])) {
            varid = 0;
        } else {
            varid = strdup(buf);
        }
        if ((!strcmp(pp, "*")) || (!pp[0])) {
            varct = 0;
        } else {
            varct = strdup(pp);
        }
        free(buf);
    }
    while (line && !found) {
        if (next) {
            NEXT_ITEM(line, extended);
        } else {
            if (extended) {
                line = line->first_item->first_line;
            } else {
                line = line->first_item;
            }
        }
        for (se = line; se;) {
            if ( (found = aisc_match(se, varid, varct))) {
                break;
            }
            NEXT_ITEM(se, extended);
        }
        if (goup) {
            line = line->first_item->first_line->father;
        } else {
            line = 0;
        }
    }
    if (varid)
        free(varid);
    if (varct)
        free(varct);
    return found;
}


char           *
get_var_string(char *var)
/* Berechnet zu einem Ausdruch der Form $(IDENT:fdg|"sdfg") das Ergebnis */
{
    AD             *cur;
    char           *doppelpunkt;
    char           *bar;
    char           *nextdp;
    char           *finds;
    int             len;
    int             findl;
    int             replacel;
    int             offset;
    int             use_path;
    char           *in, *buf1, *buf2;



    READ_SPACES(var);
    use_path = 0;
    while (var[0] == '&') {
        use_path++;
        var++;
        READ_SPACES(var);
    }
    doppelpunkt = strchr(var, ':');
    if (doppelpunkt)
        *(doppelpunkt++) = 0;
    bar = strchr(var, '|');
    if (bar)
        *(bar++) = 0;
    in = read_hash_local(var,0);
    if (in) {
        in = strdup(in);
    } else {
        cur = aisc_find_var_hier(gl->cursor, var, 0, 0, 1);
        if (!cur) {
            if (!bar) {
                if (gl->pc->command == IF) {
                    return 0;
                }
                /* fprintf(stderr, "%s: ", var); */
                printf_error("Ident '%s' not found", var);
                return 0;
            }
            return strdup(bar);
        }
        if (use_path) {
            in = strdup(cur->key);
            if (use_path == 1) {
            } else {
                while (cur->first_item->first_line->father) {
                    cur = cur->first_item->first_line->father;
                    len = strlen(in) + strlen(cur->key);
                    buf1 = (char *) calloc(sizeof(char), len + 2);
                    sprintf(buf1, "%s/%s", cur->key, in);
                    free(in);
                    in = buf1;
                }

            }
        } else {
            if (cur->sub) {
                printf_error("Ident '%s' is a hierarchical type", var);
                return 0;
            } else {
                if (cur->val) {
                    in = strdup(cur->val);
                }else{
                    in = strdup("");
                }
            }
        }
    }
    if (!doppelpunkt)
        return in;
    len = strlen(in);
    while (doppelpunkt) {
        nextdp = strchr(doppelpunkt, ':');
        if (nextdp)
            *(nextdp++) = 0;
        if (!doppelpunkt[0]) {
            print_error("Ident replacement is missing an '='");
            return 0;
        }

        bar = strchr(doppelpunkt+1, '=');
        if (bar) {
            *(bar++) = 0;
        } else {
            print_error("Ident replacement is missing an '='");
            return 0;
        }
        findl = strlen(doppelpunkt);
        replacel = strlen(bar);
        for (finds = strstr(in, doppelpunkt); finds; finds = strstr(buf2, doppelpunkt)) {
            len += replacel - findl;
            buf1 = (char *) calloc(sizeof(char), len + 1);
            offset = finds - in;
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
    return in;
}
