/* ================================================================ */
/*                                                                  */
/*   File      : aisc_commands.c                                    */
/*   Purpose   :                                                    */
/*                                                                  */
/*   Institute of Microbiology (Technical University Munich)        */
/*   http://www.arb-home.de/                                        */
/*                                                                  */
/* ================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <memory.h>

#include "aisc.h"

int contains_tabs = 0;
static int error_count = 0;

void print_error_internal(const char *err, const char *launcher_file, int launcher_line) {
    fprintf(stderr, "./%s:%i: Error: %s\n", gl->pc->path, gl->pc->linenr, err);
    if (launcher_file) fprintf(stderr, "../AISC/%s:%i: error was launched from here\n", launcher_file, launcher_line);
    error_count++;
}

void print_warning_internal(const char *err, const char *launcher_file, int launcher_line) {
    fprintf(stderr, "./%s:%i: Warning: %s\n", gl->pc->path, gl->pc->linenr, err);
    if (launcher_file) fprintf(stderr, "../AISC/%s:%i: error was launched from here\n", launcher_file, launcher_line);
}

#define ERRBUFSIZE 200
const char *formatted(const char *format, ...) {
    /* goes to header:  __ATTR__FORMAT(1) */

    static char *errbuf = 0;
    if (!errbuf) { errbuf = (char*)malloc(ERRBUFSIZE+1); }

    va_list argPtr;
    va_start(argPtr, format);
    int     chars = vsprintf(errbuf, format, argPtr);
    if (chars>ERRBUFSIZE) {
        fprintf(stderr, "%s:%i: Error: Buffer overflow!\n", __FILE__, __LINE__);
        vfprintf(stderr, format, argPtr);
        fputc('\n', stderr);

        va_end(argPtr);
        exit(EXIT_FAILURE);
    }
    va_end(argPtr);

    return errbuf;
}
#undef ERRBUFSIZE

static char *calc_rest_line( /* const */ char *str, int size, int presize) {
    /* wertet einen Puffer str aus , str[-1] muss existieren !!! */
    char *ld;
    char *p;
    char *br;
    char *path;
    char *fi;
    int lenf, len;
    char *lastbr;
    char c;

    ld = strstr(&str[2], "$(");
    fi = 0;
    if (ld) {
        lastbr = calc_rest_line(ld, size - (ld - str), presize+2);
        if (!lastbr)
            return 0;
    }
    else {
        lastbr = str+2;
    }
    p = str+2;
    SKIP_SPACE_LF(p);
    if (*p == 0) {
        print_error("unbalanced brackets; EOL found");
        return 0;
    }
    br = strchr(p, ')');
    if (!br) {
        fprintf(stderr, "%s#%s\n", str, lastbr);
        print_error("unbalanced brackets; missing ')'");
        return 0;
    }
    *br = 0;
    br++;
    if (*p == 0) {              /* empty $() */
        fi = strdup("");
    }
    else if (presize && (str[-1] == '$')) {    /* quoted */
        br[-1] = ')';
        c = *br; *br = 0;
        fi = strdup(str+1);
        *br = c;
    }
    else if (*p == '+') {
        path = p + 1;
        SKIP_SPACE_LF(path);
        fi = strpbrk(path, "+,");
        if (!fi) {
            print_error("NO '+,;' found in ADD FUNCTION");
            return 0;
        }
        *(fi++) = 0;
        SKIP_SPACE_LF(fi);
        sprintf(string_buf, "%li", atol(path) + atol(fi));
        fi = strdup(string_buf);
    }
    else if (*p == '*') {
        path = p + 1;
        SKIP_SPACE_LF(path);
        fi = strpbrk(path, "*,");
        if (!fi) {
            print_error("NO '*,;' found in ADD FUNCTION");
            return 0;
        }
        *(fi++) = 0;
        SKIP_SPACE_LF(fi);
        sprintf(string_buf, "%li", atol(path) * atol(fi));
        fi = strdup(string_buf);
    }
    else if (*p == '#') {
        if (!strncmp(p, "#FILE", 5)) {
            for (path = p + 5; is_SPACE_LF_EOS(path[0]); path++) ;
            fi = read_aisc_file(path);
            if (!fi) return 0;

            {
                int file_len = strlen(fi);

                aisc_assert(gl->line_path);

                const char *previous_file = gl->line_path ? gl->line_path : "unknown.file";
                int         previous_line = gl->line_path ? gl->line_cnt : 0;
                int         buflen        = file_len+strlen(path)+strlen(previous_file)+100;
                char       *buffer        = (char *)malloc(buflen);

                /* Inject code to restore correct code file and line (needed for proper error messages) */
                int printed = sprintf(buffer, "@SETSOURCE %s,%i@%s@SETSOURCE %s,%i@", path, 1, fi, previous_file, previous_line);

                if (printed >= buflen) {
                    fprintf(stderr, "%s:%i: Error: buffer overflow\n", __FILE__, __LINE__);
                }

                free(fi);
                fi = buffer;
            }
        }
        else {
            printf_error("unknown Var_Command '%s'", p);
            return 0;
        }
    }
    else {
        fi = get_var_string(p);
    }

    if (fi) {
        lenf = strlen(fi);
        len = strlen(br);
        if (len + lenf > size) {
            print_error("bufsize exceeded");
            free(fi);
            return 0;
        }
        memmove(str + lenf, br, len + 1);
        memcpy(str, fi, lenf);
        free(fi);
        ld = strstr(str, "$(");
        if (ld) {
            lastbr = calc_rest_line(ld, size - (ld - str), presize+(ld-str));
            if (!lastbr)
                return 0;
        }
        return str+lenf;
    }
    
    if (gl->pc->command != IF) {
        printf_error("unknown Var_Reference '%s'", p);
    }
    return 0;
}


static int calc_line(char *str, char *buf)
{
    char           *ld;
    int             len;
    char            *lb;
    len = strlen(str);
    if (len > gl->bufsize)
        len = gl->bufsize;
    memcpy(buf, str, len + 1);
    ld = strstr(buf, "$(");
    if (!ld)
        return 0;
    contains_tabs = 0;
    lb = calc_rest_line(ld, gl->bufsize - (ld - buf), ld-buf);
    if (!lb) return 1;
    return 0;
}

static int calc_line2(char *str, char *buf) {
    /* erstes $( nicht auswerten ) !!! */
    int len = strlen(str);
    if (len > gl->bufsize - 10) len = gl->bufsize - 10;

    memcpy(buf, str, len + 1);

    char *ld = buf;
    SKIP_SPACE_LF(ld);
    
    ld = strstr(ld, "$(");
    if (ld && strncmp(ld, "$(", 2) == 0) {
        ld = strstr(ld + 2, "$(");
        contains_tabs = 0;
        if (ld) {
            char *lb = calc_rest_line(ld, gl->bufsize - (ld - buf), 2);
            if (!lb) return 1;
        }
        return 0;
    }

    print_error("No Identifier specified");
    return 1;
}

inline void print_tabs(int count, FILE *out) {
    while (count--) fputc('\t', out);
}

static void write_aisc(const TokenListBlock *block, FILE *out, int indentation) {
    for (const TokenList *list = block->first_list(); list; list = list->next_list()) {
        print_tabs(indentation, out);

        const Token *first = list->first_token();
        for (const Token *tok = first; tok; tok = tok->next_token()) {
            if (tok != first) fputc(',', out);

            if (tok->is_block()) {
                fprintf(out, "'%s'={\n", tok->get_key());
                const TokenListBlock *content = tok->get_content();
                if (content) write_aisc(content, out, indentation + 1);
                print_tabs(indentation+1, out);
                fputc('}', out);
            }
            else {
                fprintf(out, "\t'%s'=(~%s~)",
                        tok->get_key(),
                        tok->has_value() ? tok->get_value() : "");
            }
        }
        fprintf(out, ";\n");
    }
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_dumpdata(const char *filename)) {
    if (!gl->root) {
        print_error("DUMPDATA can only be used after DATA");
        return 1;
    }
    FILE *out = stdout;
    if (filename[0]) {
        printf("Dumping data to '%s'\n", filename);
        out = fopen(filename, "wt");
        if (!out) {
            printf_error("cant open file '%s' to DUMPDATA", filename);
            return 1;
        }
    }
    else {
        puts("DUMPDATA:");
    }
    write_aisc(gl->root, out, 0);
    if (filename[0]) fclose(out);
    return 0;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_data(char *str)) {
    if (strlen(str) < 2) {
        printf_error("no parameter '%s'", gl->pc->str);
        return 1;
    }

    const char *in = str;

    gl->lastchar        = ' ';
    gl->last_line_start = in;
    gl->line_cnt        = 1;
    free(gl->line_path);
    gl->line_path       = strdup(gl->pc->path);

    gl->root = parse_aisc_TokenListBlock(in);

    if (!gl->root) {
        printf_error("occurred in following script-part: '%s'", gl->pc->str);
        return 1;
    }
    return 0;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_write(FILE * out, char *str)) {
    char *p;
    char  c;
    int   no_nl = 0;
    int   pos;
    int   lt, rt;

    p = str;
    while ((c = *(p++))) {
        if (c == '$') {
            c = *(p++);
            if (!c)
                break;
            if (!gl->outtab[(unsigned)(c)]) {
                if (c == '\\') {
                    no_nl = 1;
                    continue;
                }
                if ((c >= '0') && (c <= '9')) {
                    pos = gl->tabs[c - '0'];
                    if (pos < gl->tabpos)
                        continue;
                    lt = gl->tabpos / gl->tabstop;
                    rt = pos / gl->tabstop;
                    while (lt < rt) {
                        putc('\t', out);
                        lt++;
                        gl->tabpos /= gl->tabstop;
                        gl->tabpos++;
                        gl->tabpos *= gl->tabstop;
                    }
                    while (gl->tabpos < pos) {
                        putc(' ', out);
                        gl->tabpos++;
                    }

                    continue;
                }
                continue;
            }
            else {
                c = gl->outtab[(unsigned)(c)];
            }
        }
        if (c == '\t') {
            putc(c, out);
            gl->tabpos /= gl->tabstop;
            gl->tabpos++;
            gl->tabpos *= gl->tabstop;
        }
        else if (c == '\n') {
            if (no_nl) {
                no_nl = 0;
            }
            else {
                putc(c, out);
                gl->tabpos = 0;
            }
        }
        else if (c == '@') {
            if (strncmp(p, "SETSOURCE", 9) == 0) { /* skip '@SETSOURCE file, line@' */
                p = strchr(p, '@');
                if (!p) {
                    print_error("expected '@' after '@SETSOURCE' (injected code)");
                    return 1;
                }
                p++;
            }
            else {
                putc(c, out);
                gl->tabpos++;
            }
        }
        else {
            putc(c, out);
            gl->tabpos++;
        }
    }
    if (!no_nl) {
        putc('\n', out);
        gl->tabpos = 0;
    }
    return 0;
}

ATTRIBUTED(__ATTR__USERESULT, inline int do_com_print(char *str)) { return do_com_write(gl->out, str); }
ATTRIBUTED(__ATTR__USERESULT, inline int do_com_print2(char *str)) { return do_com_write(stdout, str); }

ATTRIBUTED(__ATTR__USERESULT, static int do_com_tabstop(char *str)) {
    int             ts, i;
    ts = atoi(str);
    if ((ts < 1) || (ts > 1024)) {
        print_error("wrong TABSTOP");
        return 1;
    }
    gl->tabstop = ts;
    for (i = 0; i < 9; i++)
        gl->tabs[i] = i * ts;
    return 0;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_tab(char *str)) {
    char *s1 = strtok(str, " \t,;");
    if (!s1) print_error("empty input");
    else {
        char *s2 = strtok(0, " \t,;");
        if (!s2) print_error("expected parameter");
        else {
            int ts = atoi(s1);
            if ((ts < 0) || (ts > 9)) print_error("wrong TABCOUNT");
            else {
                int val = atoi(s2);
                if ((val < 0) || (val > 1000)) print_error("wrong TABVALUE");
                else {
                    gl->tabs[ts] = val;
                    return 0;
                }
            }
        }
    }
    return 1;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_error(char *str)) {
    print_error(str);
    return 1;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_open(char *str)) {
    FILE           *file;
    int             i;
    char           *fn;
    SKIP_SPACE_LF(str);
    str = strtok(str, " \t,;\n");
    if (!str) {
        print_error("empty input");
        return 1;
    }
    fn = strtok(0, " \t,;\n");
    if (!fn) {
        print_error("No Filename found (<3)");
        return 1;
    }
    if (strlen(fn) < 3) {
        printf_error("Filename '%s' too short (<3)", fn);
        return 1;
    }
    for (i = 0; i < OPENFILES; i++) {
        if (gl->fouts[i]) {
            if (strcmp(gl->fouts[i], str) == 0) {
                printf_error("File '%s' already opened", str);
                return 1;
            }
        }
    }
    for (i = 0; i < OPENFILES; i++) {
        if (!gl->fouts[i]) {
            break;
        }
    }
    if (i == OPENFILES) {
        print_error("Too many open files");
        return 1;
    }
    file = fopen(fn, "w");
    if (!file) {
        printf_error("Cannot open file '%s'", fn);
        return 1;
    }

    gl->fouts[i]      = strdup(str);
    gl->fouts_name[i] = strdup(fn);
    gl->outs[i] = file;

    return 0;
}
void aisc_remove_files() {
    for (int i = 0; i < OPENFILES; i++) {
        if (gl->fouts[i]) {
            fclose(gl->outs[i]);
            gl->outs[i] = NULL;
            if (gl->fouts_name[i]) {
                fprintf(stderr, "Unlinking %s\n", gl->fouts_name[i]);
                unlink(gl->fouts_name[i]);
            }
        }
    }
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_close(char *str)) {
    int i;
    SKIP_SPACE_LF(str);
    str = strtok(str, " \t,;\n");
    if (!str) {
        print_error("no input");
        return 1;
    }
    for (i = 0; i < OPENFILES; i++) {
        if (gl->fouts[i]) {
            if (!strcmp(gl->fouts[i], str)) {
                fclose(gl->outs[i]);
                gl->outs[i] = NULL;

                free(gl->fouts[i]);
                free(gl->fouts_name[i]);
                gl->fouts[i]      = 0;
                gl->fouts_name[i] = 0;
                if (gl->outs[i] == gl->out) {
                    gl->out = stdout;
                }
                return 0;
            }
        }
    }
    return 0;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_out(char *str)) {
    int             i;
    SKIP_SPACE_LF(str);
    str = strtok(str, " \t,;\n");
    if (!str) {
        print_error("no input");
        return 1;
    }
    for (i = 0; i < OPENFILES; i++) {
        if (gl->fouts[i]) {
            if (!strcmp(gl->fouts[i], str)) {
                gl->out = gl->outs[i];
                return 0;
            }
        }
    }
    printf_error("File '%s' not opened for OUT command", str);
    return 1;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_moveto(char *str)) {
    const Token *fo;
    char        *st;
    char        *co;
    char        *p;

    st = strstr(str, "$(");
    if (!st) {
        print_error("no $( found");
        return 1;
    };
    st += 2;
    SKIP_SPACE_LF(st);
    co = strchr(st, ')');
    if (!co) {
        print_error("no ) found");
        return 1;
    };
    *co = 0;
    p = strrchr(st, '/');
    if (p) {
        fo = aisc_find_var_hier(gl->cursor, st, LOOKUP_LIST);
    }
    else {
        fo = aisc_find_var_hier(gl->cursor, st, LOOKUP_BLOCK);
    }
    if (!fo) {
    }
    else {
        gl->cursor = fo;
    }
    return 0;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_set(char *str)) {
    char           *st;
    char           *co;
    char           *def;
    struct hash_struct *hs;

    st = strstr(str, "$(");
    if (!st) {
        print_error("no $( found");
        return 1;
    };
    st += 2;
    co = strchr(st, ')');
    if (!co) {
        print_error("no ) found");
        return 1;
    };
    *(co++) = 0;
    def = read_hash_local(st, &hs);
    if (!def) {
        printf_error("undefined Ident '%s' in SET (use CREATE first)", st);
        return 1;
    }
    SKIP_SPACE_LF(co);
    if (*co == '=') {
        *(co++) = 0;
        SKIP_SPACE_LF(co);
    }
    write_hash(hs, st, co);
    return 0;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_create(char *str)) {
    char           *st;
    char           *co;
    char           *def;
    st = strstr(str, "$(");
    if (!st) {
        print_error("no $( found");
        return 1;
    };
    st += 2;
    co = strchr(st, ')');
    if (!co) {
        print_error("no ) found");
        return 1;
    };
    *(co++) = 0;
    def = read_hash(gl->st->hs, st);
    if (def) {
        printf_error("Ident '%s' in CREATE already defined", st);
        return 1;
    }
    SKIP_SPACE_LF(co);
    if (*co == '=') {
        *(co++) = 0;
        SKIP_SPACE_LF(co);
    }
    write_hash(gl->st->hs, st, co);
    return 0;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_if(char *str)) {
    char           *equ;
    char           *kom;
    char           *la;
    char           *kom2;

    int op = 0;                                     /* 0=   1~ 2< 3> !+8 */
    for (equ = str; *equ; equ++) {
        if (*equ == '=') { op = 0; break; }
        if (*equ == '~') { op = 1; break; }
    }
    la = equ;
    if (!*la) { // no operator found -> assume condition true, even if empty or undefined
        return 0;
    }

    aisc_assert(equ>str);
    if (equ[-1] == '!') op += 8;

    --la;
    SKIP_SPACE_LF_BACKWARD(la);
    *(++la) = 0;
    equ++;
    while (equ) {
        SKIP_SPACE_LF(equ);
        kom2 = kom = strchr(equ, ',');
        if (kom) {
            kom2++;
            --kom;
            SKIP_SPACE_LF_BACKWARD(kom);
            *(++kom) = 0;
        }
        switch (op) {
            case 0:     if (!strcmp(str, equ)) return 0; break;
            case 8:     if (strcmp(str, equ)) return 0; break;
            case 1:     if (strstr(str, equ)) return 0; break;
            case 9:     if (!strstr(str, equ)) return 0; break;
            default:
                printf_error("Unhandled operator (op=%i)", op);
                return 1;
        }
        equ = kom2;
    }

    // condition wrong -> goto else
    gl->nextpc = gl->pc->ELSE->next;
    return 0;
}

static void do_com_for_add(CL *co) {
    struct for_data_struct *fd;
    fd = (struct for_data_struct *)calloc(sizeof(struct for_data_struct), 1);
    fd->next = co->fd;
    co->fd = fd;
}

static void do_com_for_sub(CL *co) {
    struct for_data_struct *fd;
    fd = co->fd;
    if (fd->forstr) free(fd->forstr);
    co->fd = fd->next;
    free((char *)fd);
}

int do_com_push(const char *) {
    /* goes to header:  __ATTR__USERESULT */
    if (gl->sp++ >= STACKSIZE) {
        print_error("Stack size exceeded");
        return 1;
    }

    struct stack_struct *st = (struct stack_struct *)calloc(sizeof(struct stack_struct), 1);

    st->cursor = gl->cursor;
    st->pc     = gl->pc;
    st->hs     = create_hash(HASHSIZE);
    st->next   = gl->st;

    gl->st = st;
    
    return 0;
}

static void pop_stack() {
    aisc_assert(gl->sp>0);

    struct stack_struct *st = gl->st;
    free_hash(st->hs);
    gl->cursor = st->cursor;
    gl->st     = st->next;
    free(st);
    gl->sp--;

}

void free_stack() {
    while (gl->st) {
        pop_stack();
    }
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_pop(const char *)) {
    if (gl->sp<2) {
        print_error("Nothing to Pop");
        return 1;
    }
    pop_stack();
    return 0;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_gosub(char *str)) {
    int result = 1;
    if (do_com_push("") == 0) {
        char *s;
        char *params;
        
        for (s = str; !is_SPACE_SEP_LF_EOS(*s); s++) ;
        if (*s) {
            *s = 0;
            s++;
            SKIP_SPACE_LF(s);
            params = strdup(s);
        }
        else {
            params = strdup("");
        }
        
        char *fn = read_hash(gl->fns, str);
        if (!fn) {
            printf_error("Function '%s' not found", str);
        }
        else {
            CL *fun = (CL *)atol(fn);
            gl->pc  = fun;

            int err = calc_line(gl->pc->str, gl->linebuf);
            if (!err) {
                char *fpara = gl->linebuf;
                SKIP_SPACE_LF(fpara);

                char *npara  = 0;
                char *nfpara = 0;
                for (char *para = params; *para && !err; para=npara, fpara=nfpara) {
                    if (!*fpara) {
                        printf_error("Too many Parameters %s", para);
                        err = 1;
                    }
                    else {
                        for (s = para; !is_SEP_LF_EOS(*s); s++) ;
                        if (*s) { npara = s+1; SKIP_SPACE_LF(npara); } else npara = s;
                        *s = 0;

                        for (s = fpara; !is_SEP_LF_EOS(*s); s++) ;
                        if (*s) { nfpara = s+1; SKIP_SPACE_LF(nfpara); } else nfpara = s;
                        *s = 0;

                        s = read_hash(gl->st->hs, para);
                        if (s) {
                            printf_error("duplicated formal parameter %s", para);
                            err = 1;
                        }
                        else {
                            write_hash(gl->st->hs, fpara, para);
                        }
                    }
                }

                if (!err && *fpara) {
                    printf_error("Not enough parameters '%s'", fpara);
                    err = 1;
                }

                if (!err) {
                    gl->nextpc = fun ->next;
                    result     = 0;
                }
            }
        }
        free(params);
    }
    return result;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_goto(char *str)) {
    char *fn = read_hash(gl->fns, str);
    if (!fn) {
        printf_error("Function '%s' not found", str);
        return 1;
    }
    gl->nextpc = ((CL *)atol(fn)) ->next;
    return 0;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_return(char *)) {
    gl->nextpc = gl->st->pc->next;
    return do_com_pop("");
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_exit(char *)) {
    return 1;
}

ATTRIBUTED(__ATTR__USERESULT, static int do_com_for(char *str)) {
    int   result = 1;
    char *st     = strstr(str, "$(");
    if (!st) print_error("no input");
    else {
        st += 2;
        SKIP_SPACE_LF(st);

        char *co = strchr(st, ')');
        if (!co) print_error("Expected ')'");
        else {
            *(co++) = 0;

            char *eq = strchr(co, '=');
            if (eq) {
                *(eq++) = 0;
                SKIP_SPACE_LF(eq);
                
                char *to = strstr(eq, "TO");
                if (!to) print_error("TO not found in FOR :( FOR $(i) = a TO b )");
                else {
                    *to  = 0;
                    to  += 2;
                    SKIP_SPACE_LF(to);

                    do_com_for_add(gl->pc);
                    gl->pc->fd->forval = atol(eq);
                    gl->pc->fd->forend = atol(to);

                    if (gl->pc->fd->forval > gl->pc->fd->forend) {
                        gl->nextpc = gl->pc->NEXT->next;
                        do_com_for_sub(gl->pc);
                        result = 0;
                    }
                    else {
                        struct hash_struct *hs;
                        char *p = (char *) read_hash_local(st, &hs);
                        if (!p) {
                            printf_error("Undefined Ident '%s' in FOR (use CREATE first)", st);
                        }
                        else {
                            sprintf(string_buf, "%li", gl->pc->fd->forval);
                            write_hash(hs, st, string_buf);
                            gl->pc->fd->forstr = strdup(st);

                            result = 0;
                        }
                    }
                }
            }
            else {
                char *p = strrchr(st, '/');
                str     = p ? p+1 : st;
                
                const Token *fo = aisc_find_var_hier(gl->cursor, st, p ? LOOKUP_LIST : LOOKUP_BLOCK);
                if (!fo) {
                    gl->nextpc = gl->pc->NEXT->next;
                }
                else {
                    do_com_for_add(gl->pc);
                    gl->pc->fd->forstr = strdup(str);
                    gl->pc->fd->forcursor = gl->cursor;
                    gl->cursor = fo;
                }
                result = 0;
            }
        }
    }
    return result;
}


ATTRIBUTED(__ATTR__USERESULT, static int do_com_next(const char *)) {
    if (gl->pc->FOR->fd->forcursor) {
        const Token *fo = aisc_find_var(gl->cursor, gl->pc->FOR->fd->forstr, LOOKUP_BLOCK_REST);
        if (!fo) {
            gl->cursor = gl->pc->FOR->fd->forcursor;
            gl->nextpc = gl->pc->FOR->ENDFOR->next;
            do_com_for_sub(gl->pc->FOR);
        }
        else {
            gl->nextpc = gl->pc->FOR->next;
            gl->cursor = fo;
        }
    }
    else {
        gl->pc->FOR->fd->forval++;
        if (gl->pc->FOR->fd->forval > gl->pc->FOR->fd->forend) {
            gl->nextpc = gl->pc->FOR->ENDFOR->next;
            do_com_for_sub(gl->pc->FOR);
        }
        else {
            gl->nextpc = gl->pc->FOR->next;

            struct hash_struct *hs;
            char *p = read_hash_local(gl->pc->FOR->fd->forstr, &hs);
            aisc_assert(p);
            if (p) {
                sprintf(string_buf, "%li", gl->pc->FOR->fd->forval);
                write_hash(hs, gl->pc->FOR->fd->forstr, string_buf);
            }
        }

    }
    return 0;
}

#define COMMAND(str, string, len, func)                         \
    if (string[0] == str[0] && !strncmp(string, str, len)) {    \
        char *s=str+len;                                        \
        SKIP_SPACE_LF(s);                                       \
        if (func(s)) break;                                     \
        continue;                                               \
    }

#define COMMAND_NOFAIL(str, string, len, func)                  \
    if (string[0] == str[0] && !strncmp(string, str, len)) {    \
        char *s=str+len;                                        \
        SKIP_SPACE_LF(s);                                       \
        if (func(s)) return -1;                                 \
        continue;                                               \
    }

#define COMMAND2(str, string, len, func)                        \
    if (string[0] == str[0] && !strncmp(string, str, len)) {    \
        char *s=str+len;                                        \
        if (func(s)) break;                                     \
        continue;                                               \
    }

#define COMMAND2_NOFAIL(str, string, len, func)                 \
    if (string[0] == str[0] && !strncmp(string, str, len)) {    \
        char *s=str+len;                                        \
        if (func(s)) return -1;                                 \
        continue;                                               \
    }

int run_prg() {
    int err;

    for (gl->pc = gl->prg; gl->pc; gl->pc = gl->nextpc) {
        gl->nextpc = gl->pc->next;
        if (gl->pc->command) {
            switch (gl->pc->command) {
                case IF:
                    if (calc_line(gl->pc->str, gl->linebuf)) {
                        gl->nextpc = gl->pc->ELSE->next;
                        break;
                    }

                    if (do_com_if(gl->linebuf))
                        return 1;
                    break;
                case FOR:
                    if (calc_line2(gl->pc->str, gl->linebuf))
                        return 1;
                    if (do_com_for(gl->linebuf))
                        return 1;
                    break;
                case ELSE:
                    gl->nextpc = gl->pc->IF->ENDIF->next;
                    break;
                case NEXT:
                    if (do_com_next(""))
                        return 1;
                    break;
                case ENDFOR:
                case ENDIF:
                    break;
                default:
                    break;
            }
            continue;
        }
        if (!strncmp(gl->pc->str, "MOVETO", 6)) {
            if (calc_line2(gl->pc->str + 7, gl->linebuf)) break;
            if (do_com_moveto(gl->linebuf)) break;
            continue;
        }
        if (!strncmp(gl->pc->str, "SET", 3)) {
            if (calc_line2(gl->pc->str + 4, gl->linebuf)) break;
            if (do_com_set(gl->linebuf)) break;
            continue;
        }
        if (!strncmp(gl->pc->str, "CREATE", 6)) {
            if (calc_line2(gl->pc->str + 7, gl->linebuf)) break;
            if (do_com_create(gl->linebuf)) break;
            continue;
        }
        gl->s_pos = 0;

        free(gl->line_path);
        gl->line_path = strdup(gl->prg->path);

        err = calc_line(gl->pc->str, gl->linebuf);
        if (err) return err;

        COMMAND2(gl->linebuf, "PRINT", 5, do_com_print);
        COMMAND2(gl->linebuf, "P ", 2, do_com_print);
        COMMAND2(gl->linebuf, "P\t", 2, do_com_print);
        COMMAND(gl->linebuf, "GOSUB", 5, do_com_gosub);
        COMMAND(gl->linebuf, "CALL", 4, do_com_gosub);
        COMMAND(gl->linebuf, "GOTO", 4, do_com_goto);
        COMMAND(gl->linebuf, "RETURN", 6, do_com_return);
        COMMAND(gl->linebuf, "PUSH", 4, do_com_push);
        COMMAND(gl->linebuf, "POP", 3, do_com_pop);
        COMMAND(gl->linebuf, "CONTINUE", 8, do_com_next);


        COMMAND(gl->linebuf, "OPEN", 4, do_com_open);
        COMMAND(gl->linebuf, "CLOSE", 5, do_com_close);
        COMMAND(gl->linebuf, "OUT", 3, do_com_out);
        COMMAND2_NOFAIL(gl->linebuf, "ERROR", 5, do_com_error);
        COMMAND(gl->linebuf, "TABSTOP", 7, do_com_tabstop);
        COMMAND(gl->linebuf, "TAB", 3, do_com_tab);
        COMMAND(gl->linebuf, "PP", 2, do_com_print2);
        COMMAND(gl->linebuf, "EXIT", 4, do_com_exit);
        COMMAND_NOFAIL(gl->linebuf, "DATA", 4, do_com_data);
        COMMAND_NOFAIL(gl->linebuf, "DUMPDATA", 8, do_com_dumpdata);

        printf_error("Unknown command '%s'", gl->pc->str);
        return -1;
    }

    aisc_assert(error_count == 0);
    return 0;
}
