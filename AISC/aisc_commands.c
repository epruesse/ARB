#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
/* #include <malloc.h> */
#include <memory.h>

#include "aisc.h"
#include "aisc_proto.h"

int      contains_tabs = 0;

int print_error(const char *err) {
    /* fprintf(stderr, "ERROR: %s: line %i in file %s\n", err, gl->pc->linenr, gl->pc->path); */
    fprintf(stderr, "%s:%i: Error: %s\n", gl->pc->path, gl->pc->linenr, err);
    return 1;
}

#define ERRBUFSIZE 200

int printf_error(const char *format, ...) {
    static char *errbuf = 0;
    if (!errbuf) { errbuf = (char*)malloc(ERRBUFSIZE+1); }

    va_list argPtr;
    va_start(argPtr, format);
    int     chars = vsprintf(errbuf, format, argPtr);
    if (chars>ERRBUFSIZE) {
        fprintf(stderr, "%s:%i: Error buffer overflow!\n", __FILE__, __LINE__);
        vfprintf(stderr, format, argPtr);
        fputc('\n', stderr);
        
        va_end(argPtr);
        exit(EXIT_FAILURE);
    }
    va_end(argPtr);

    return print_error(errbuf);
}

#undef ERRBUFSIZE

void memcopy(char *dest, const char *source, int len)
{
    register int    i;
    register const char  *s;
    register char  *d;

    i = len;
    s = source;
    d = dest;
    if (s < d) {
        s += i;
        d += i;
        while (i--) {
            *(--d) = *(--s);
        }
    } else {
        while (i--) {
            *(d++) = *(s++);
        }
    }
}

char *find_string(const char *str,const char *key)
{
    register const char *p1,*p2;
    for (p1=str,p2=key;*p1;) {
        if (!*p2) {
            return (char*)(str);
        }else{
            if (*p2==*p1){
                p1 ++; p2++;
            }else{
                p2 = key;
                p1 = (++str);
            }
        }
    }
    if (!*p2) return (char*)(str);
    return 0;
}

char *calc_rest_line(/*const*/ char *str, int size, int presize)
{				/* wertet einen Puffer str aus , str[-1] muss exestieren !!! */
    char *ld;
    char *p;
    char *br;
    char *path;
    char *fi;
    int lenf, len;
    char *lastbr;
    char c;

    ld = find_string(&str[2], "$(");
    fi = 0;
    if (ld) {
        lastbr = calc_rest_line(ld, size - (ld - str),presize+2);
        if (!lastbr)
            return 0;
    }else{
        lastbr = str+2;
    }
    p = str+2;
    READ_SPACES(p);
    if (*p == 0) {
        print_error("unbalanced brackets; EOL found");
        return 0;
    }
    br = strchr(p, ')');
    if (!br) {
        fprintf(stderr, "%s#%s\n",str,lastbr);
        print_error("unbalanced brackets; missing ')'");
        return 0;
    }
    *br = 0;
    br++;
    if (*p == 0) {		/* empty $() */
        fi = strdup("");
    }else if (presize && (str[-1] == '$')) {	/* quoted */
        br[-1] = ')';
        c = *br; *br = 0;
        fi = strdup(str+1);
        /*printf("%s#%s\n",fi,br);*/
        *br = c;
    }else if (*p == '+') {
        path = p + 1;
        READ_SPACES(path);
        fi = strpbrk(path, "+,");
        if (!fi) {
            print_error("NO '+,;' found in ADD FUNKTION");
            return 0;
        }
        *(fi++) = 0;
        READ_SPACES(fi);
        sprintf(error_buf, "%li", atol(path) + atol(fi));
        fi = strdup(error_buf);
    }else if (*p == '*') {
        path = p + 1;
        READ_SPACES(path);
        fi = strpbrk(path, "*,");
        if (!fi) {
            print_error("NO '*,;' found in ADD FUNKTION");
            return 0;
        }
        *(fi++) = 0;
        READ_SPACES(fi);
        sprintf(error_buf, "%li", atol(path) * atol(fi));
        fi = strdup(error_buf);
    } else if (*p == '#') {
        if (!strncmp(p, "#FILE", 5)) {
            for (path = p + 5; gl->s2_tab[(unsigned)(path[0])]; path++);
            fi = read_aisc_file(path);
            if (!fi) return 0;

            {
                int         file_len      = strlen(fi);
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
    } else {
        fi = get_var_string(p);
    }
    if (fi) {
        lenf = strlen(fi);
        len = strlen(br);
        if (len + lenf > size) {
            print_error("bufsize exceeded");
            return 0;
        }
        memcopy(str + lenf, br, len + 1);
        memcpy(str, fi, lenf);
        free(fi);
        ld = find_string(str, "$(");
        if (ld) {
            lastbr = calc_rest_line(ld, size - (ld - str),presize+(ld-str));
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


int
calc_line(char *str, char *buf)
{
    char           *ld;
    int             len;
    char            *lb;
    len = strlen(str);
    if (len > gl->bufsize)
        len = gl->bufsize;
    memcpy(buf, str, len + 1);
    ld = find_string(buf, "$(");
    if (!ld)
        return 0;
    contains_tabs = 0;
    lb = calc_rest_line(ld, gl->bufsize - (ld - buf),ld-buf);
    if (!lb) return 1;
    return 0;
}

int
calc_line2(char *str, char *buf)
{				/* erstes $( nicht auswerten ) !!! */
    char           *ld;
    char           *lb;
    int             len;
    len = strlen(str);
    if (len > gl->bufsize - 10)
        len = gl->bufsize - 10;
    memcpy(buf, str, len + 1);
    ld = buf;
    READ_SPACES(ld);
    ld = find_string(buf, "$(");
    if (strncmp(ld,"$(",2)) {
        print_error("No Identifier specified");
        return 1;
    }
    ld = find_string(ld + 2, "$(");
    contains_tabs = 0;
    if (ld) {
        lb = calc_rest_line(ld, gl->bufsize - (ld - buf),2);
        if (!lb) return 1;
    }
    return 0;
}

void
write_aisc(AD * ad, FILE * out, int deep)
{
    AD             *item, *line;
    int             i;
    int             flag;
    for (line = ad; line; line = line->next_line) {
        for (i = 0; i < deep; i++)
            fprintf(out, "\t");
        flag = 0;
        for (item = line; item; item = item->next_item) {
            if (flag)
                fprintf(out, ",");
            flag = 1;
            if (item->sub) {
                fprintf(out, "%s {\n", item->key);
                write_aisc(item->sub, out, deep + 1);
                for (i = 0; i <= deep; i++)
                    fprintf(out, "\t");
                fprintf(out, "}");
            } else {
                fprintf(out, "\t%s (~%s~)", item->key, item->val);
            }
        }
        fprintf(out, ";\n");
    }
}

void
write_prg(CL * cl, FILE * out, int deep)
{
    CL             *line;
    int             i;
    for (line = cl; line; line = line->next) {
        for (i = 0; i < deep; i++)
            fprintf(out, "\t");
        fprintf(out, "%5i  %s\n", line->linenr, line->str);
    }
}


int
do_com_dbg(char *str)
{
    write_aisc(gl->root, stdout, 0);
    str = str;
    return 0;
}
int
do_com_data(char *str)
{
    char           *in;
    if (strlen(str) < 2) {
        printf_error("no parameter '%s'", gl->pc->str);
        return 1;
    }
    in = str;
    gl->lastchar = ' ';
    gl->line_cnt = 1;
    gl->line_path = gl->pc->path;
    gl->root = read_aisc(&in);
    if (!gl->root) {
        printf_error("occurred in following script-part: '%s'", gl->pc->str);
        return 1;
    }
    return 0;
}

int
do_com_write(FILE * out, char *str)
{
    register char  *p;
    register char   c;
    int             no_nl = 0;
    int             pos;
    int             lt, rt;
    p = str;
    while ( (c = *(p++)) ) {
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
            } else {
                c = gl->outtab[(unsigned)(c)];
            }
        }
        if (c == '\t') {
            putc(c, out);
            gl->tabpos /= gl->tabstop;
            gl->tabpos++;
            gl->tabpos *= gl->tabstop;
        } else if (c == '\n') {
            if (no_nl) {
                no_nl = 0;
            } else {
                putc(c, out);
                gl->tabpos = 0;
            }
        } else if (c == '@') {
            if (strncmp(p, "SETSOURCE", 9) == 0) { /* skip '@SETSOURCE file, line@' */
                p = strchr(p, '@');
                if (!p) {
                    printf_error("expected '@' after '@SETSOURCE' (injected code)");
                    return 1;
                }
                p++;
            }
            else {
                putc(c, out);
                gl->tabpos++;
            }
        } else {
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

int
do_com_print(char *str)
{
    do_com_write(gl->out, str);
    return 0;
}
int
do_com_print2(char *str)
{
    do_com_write(stdout, str);
    return 0;
}

int
do_com_tabstop(char *str)
{
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

int
do_com_tab(char *str)
{
    int             ts, val;
    char           *s1, *s2;
    s1 = strtok(str, " \t,;");
    s2 = strtok(0, " \t,;");
    ts = atoi(s1);
    val = atoi(s2);
    if ((ts < 0) || (ts > 9)) {
        print_error("wrong TABCOUNT");
        return 1;
    }
    if ((val < 0) || (val > 1000)) {
        print_error("wrong TABVALUE");
        return 1;
    }
    gl->tabs[ts] = val;
    return 0;
}

int
do_com_error(char *str)
{
    print_error(str);
    return 1;
}
int do_com_open(char *str)
{
    FILE           *file;
    int             i;
    char           *fn;
    READ_SPACES(str);
    str = strtok(str, " \t,;\n");
    fn = strtok(0, " \t,;\n");
    if (!fn) {
        print_error("No Filename found (<3)");
        return 1;
    }
    if (strlen(fn) < 3) {
        print_error("Filename too short (<3)");
        return 1;
    }
    for (i = 0; i < OPENFILES; i++) {
        if (gl->fouts[i]) {
            if (!strcmp(gl->fouts[i], str)) {
                print_error("File already opened");
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
        print_error("Too many Files open");
        return 1;
    }
    file = fopen(fn, "w");
    if (!file) {
        print_error("Cannot open File");
        return 1;
    }

    gl->fouts[i]      = strdup(str);
    gl->fouts_name[i] = strdup(fn);
    gl->outs[i]       = file;

    /* fprintf(stderr, "do_com_open creates file '%s' (type='%s')\n", gl->fouts_name[i], gl->fouts[i]); */

    return 0;
}
void aisc_remove_files()
{
    int             i;
    for (i = 0; i < OPENFILES; i++) {
        if (gl->fouts[i]) {
            fclose(gl->outs[i]);
            if (gl->fouts_name[i]) {
                fprintf(stderr, "Unlinking %s\n", gl->fouts_name[i]);
                unlink(gl->fouts_name[i]);
            }
        }
    }
}

int do_com_close(char *str)
{
    int             i;
    READ_SPACES(str);
    str = strtok(str, " \t,;\n");
    for (i = 0; i < OPENFILES; i++) {
        if (gl->fouts[i]) {
            if (!strcmp(gl->fouts[i], str)) {
                fclose(gl->outs[i]);
                /* fprintf(stderr, "do_com_close(%s)\n", gl->fouts_name[i]); */
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

int do_com_out(char *str)
{
    int             i;
    READ_SPACES(str);
    str = strtok(str, " \t,;\n");
    for (i = 0; i < OPENFILES; i++) {
        if (gl->fouts[i]) {
            if (!strcmp(gl->fouts[i], str)) {
                gl->out = gl->outs[i];
                return 0;
            }
        }
    }
    print_error("File not opened for OUT Command");
    return 1;
}

int do_com_moveto(char *str)
{
    AD             *fo;
    char           *st;
    char           *co;
    char *p;
    st = find_string(str, "$(");
    if (!st) {
        print_error("no $( found");
        return 1;
    };
    st += 2;
    READ_SPACES(st);
    co = strchr(st, ')');
    if (!co) {
        print_error("no ) found");
        return 1;
    };
    *co = 0;
    p = strrchr(st, '/');
    if (p) {
        fo = aisc_find_var_hier(gl->cursor, st, 0, 0, 0);
    } else {
        fo = aisc_find_var_hier(gl->cursor, st, 0, 1, 0);
    }
    if (!fo){
    }else{
        gl->cursor = fo;
    }
    return 0;
}

int do_com_set(char *str)
{
    char           *st;
    char           *co;
    char           *def;
    struct hash_struct *hs;

    st = find_string(str, "$(");
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
    def = read_hash_local(st,&hs);
    if (!def) {
        sprintf(error_buf, "not defined Ident '%s' in SET (use CREATE first)", st);
        print_error(error_buf);
        return 1;
    }
    READ_SPACES(co);
    if (*co == '=') {
        *(co++) = 0;
        READ_SPACES(co);
    }
    write_hash(hs, st, co);
    return 0;
}

int do_com_create(char *str)
{
    char           *st;
    char           *co;
    char           *def;
    st = find_string(str, "$(");
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
    def = read_hash( gl->st->hs,st);
    if (def) {
        sprintf(error_buf, "Ident '%s' in CREATE already defined", st);
        print_error(error_buf);
        return 1;
    }
    READ_SPACES(co);
    if (*co == '=') {
        *(co++) = 0;
        READ_SPACES(co);
    }
    write_hash(gl->st->hs, st, co);
    return 0;
}

int
do_com_if(char *str)
{
    char           *equ;
    char           *kom;
    char           *la;
    char           *kom2;
    int		op = 0;	/* 0=	1~ 2< 3> !+8 */
    for (equ = str;*equ;equ++) {
        if (*equ =='=') { op = 0; break;}
        if (*equ =='~') { op = 1; break;}
        /*if (*equ =='<') { op = 2; break;}
          if (*equ =='>') { op = 3; break;}*/
    }
    if (equ[-1] == '!') op +=8;
    la = equ;
    if (!*la) return 0;
    READ_RSPACES(la);
    *(++la) = 0;
    equ++;
    while (equ) {
        READ_SPACES(equ);
        kom2 = kom = strchr(equ, ',');
        if (kom) {
            kom2++;
            READ_RSPACES(kom);
            *(++kom) = 0;
        }
        switch (op) {
            case 0:	if (!strcmp(str, equ))
                return 0;
            break;
            case 8:	if (strcmp(str, equ))
                return 0;
            break;
            case 1:	if (find_string(str, equ))
                return 0;
            break;
            case 9:	if (!find_string(str, equ))
                return 0;
            break;
            case 2:	if (strcmp(str, equ)<0)
                return 0;
            break;
            case 10:if (strcmp(str, equ)>=0)
                return 0;
            break;
            case 3:	if (strcmp(str, equ)>0)
                return 0;
            break;
            case 11:if (strcmp(str, equ)<=0)
                return 0;
            break;
        }
        equ = kom2;
    }
    gl->nextpc = gl->pc->ELSE->next;
    return 0;
}

int do_com_for_add(CL *co)
{
    struct for_data_struct *fd;
    fd = (struct for_data_struct *)calloc(sizeof(struct for_data_struct),1);
    fd->next = co->fd;
    co->fd = fd;
    return 0;
}

int do_com_for_sub(CL *co)
{
    struct for_data_struct *fd;
    fd = co->fd;
    if (fd->forstr)
        free(fd->forstr);
    co->fd = fd->next;
    free((char *)fd);
    return 0;
}

int do_com_push(const char *str)
{
    struct stack_struct *st;
    st = (struct stack_struct *)calloc(sizeof(struct stack_struct),1);
    st->cursor = gl->cursor;
    st->pc = gl->pc;
    if (gl->sp++ >= STACKSIZE) {
        print_error("Stack size exceeded");
        return 1;
    }
    st->hs = create_hash(HASHSIZE);
    st->next = gl->st;
    gl->st = st;
    str = str;
    return 0;
}

int do_com_pop(const char *str)
{
    struct stack_struct *st;
    st = gl->st;
    free_hash(st->hs);
    if (gl->sp-- <=1) {
        print_error("Nothing to Pop");
        return 1;
    }
    gl->cursor = st->cursor;
    gl->st = st->next;
    free((char *)st);
    str = str;
    return 0;
}

int do_com_gosub(char *str)
{
    char *fn;
    char *s;
    char *params,*para,*fpara,*npara=0,*nfpara=0;
    CL *fun;
    int err;
    if (do_com_push("")) return 1;
    for (s = str; !gl->b_tab[(int)(*s)]; s++);
    if (*s) {
        *s = 0;
        s++;
        READ_SPACES(s);
        params = strdup(s);
    }else{
        params = strdup("");
    }
    fn = read_hash(gl->fns,str);
    if (!fn) {
        print_error("Funktion not found");
        return 1;
    }
    gl->pc = fun = (CL *)atol(fn);

    err = calc_line(gl->pc->str, gl->linebuf);
    if (err)	return err;
    fpara = gl->linebuf;
    READ_SPACES(fpara);
    for (para = params; *para;para=npara,fpara=nfpara){
        if (!*fpara) {
            sprintf(error_buf,"Too many Parameters %s",para);
            print_error(error_buf);
            return 1;
        }
        for (s = para; !gl->c_tab[(int)(*s)]; s++);
        if (*s) {npara = s+1;READ_SPACES(npara);} else npara = s;
        *s = 0;
        for (s = fpara; !gl->c_tab[(int)(*s)]; s++);
        if (*s) {nfpara = s+1;READ_SPACES(nfpara);} else nfpara = s;
        *s = 0;
        s = read_hash( gl->st->hs,para);
        if (s) {	sprintf(error_buf,"duplikated formal parameter %s",para);
        print_error(error_buf);
        return 1;
        }
        write_hash(gl->st->hs, fpara, para);
    }
    if (*fpara) {
        sprintf(error_buf,"Too less Parameters %s",fpara);
        print_error(error_buf);
        return 1;
    }
    gl->nextpc = fun ->next;
    free(params);
    return 0;
}

int do_com_goto(char *str)
{
    char *fn;
    fn = read_hash(gl->fns,str);
    if (!fn) {
        print_error("Function not found");
        return 1;
    }
    gl->nextpc = ( (CL *)atol(fn) ) ->next;
    return 0;
}

int do_com_return(char *str)
{
    gl->nextpc = gl->st->pc->next;
    if (do_com_pop("")) return 1;
    str = str;
    return 0;
}
int do_com_exit(char *str)
{
    str = str;
    return 1;
}

int
do_com_for(char *str)
{
    AD             *fo;
    char           *st;
    char           *co;
    char           *p;
    char           *eq;
    struct hash_struct *hs;
    st = find_string(str, "$(");
    st += 2;
    READ_SPACES(st);
    co = strchr(st, ')');
    *(co++) = 0;
    eq = strchr(co, '=');
    if (eq) {
        char           *to;
        *(eq++) = 0;
        READ_SPACES(eq);
        to = find_string(eq, "TO");
        if (!to) {
            print_error("TO not found in FOR :( FOR $(i) = a TO b )");
            return 1;
        }
        *to = 0;
        to += 2;
        READ_SPACES(to);
        if (do_com_for_add(gl->pc)) return 1;
        gl->pc->fd->forval = atol(eq);
        gl->pc->fd->forend = atol(to);
        if (gl->pc->fd->forval > gl->pc->fd->forend) {
            gl->nextpc = gl->pc->NEXT->next;
            return do_com_for_sub(gl->pc);
        }
        p = (char *) read_hash_local(st, &hs);
        if (!p) {
            sprintf(error_buf, "not defined Ident '%s' in FOR (use CREATE first)", st);
            print_error(error_buf);
            return 1;
        }
        sprintf(error_buf, "%li", gl->pc->fd->forval);
        write_hash(hs, st, error_buf);
        gl->pc->fd->forstr = strdup(st);
    } else {
        p = strrchr(st, '/');
        if (p)
            str = p + 1;
        else
            str = st;
        if (p) {
            fo = aisc_find_var_hier(gl->cursor, st, 0, 0, 0);
        } else {
            fo = aisc_find_var_hier(gl->cursor, st, 0, 1, 0);
        }
        if (!fo) {
            gl->nextpc = gl->pc->NEXT->next;
            return 0;
        }
        do_com_for_add(gl->pc);
        gl->pc->fd->forstr = strdup(str);
        gl->pc->fd->forcursor = gl->cursor;
        gl->cursor = fo;
    }
    return 0;
}

int do_com_next(const char *str)
{
    AD             *fo;
    char           *p;
    struct hash_struct *hs;
    if (gl->pc->FOR->fd->forcursor) {
        fo = aisc_find_var(gl->cursor, gl->pc->FOR->fd->forstr, 1, 1, 0);
        if (!fo) {
            gl->cursor = gl->pc->FOR->fd->forcursor;
            gl->nextpc = gl->pc->FOR->ENDFOR->next;
            do_com_for_sub(gl->pc->FOR);
        } else {
            gl->nextpc = gl->pc->FOR->next;
            gl->cursor = fo;
        }
    } else {
        gl->pc->FOR->fd->forval++;
        if (gl->pc->FOR->fd->forval > gl->pc->FOR->fd->forend) {
            gl->nextpc = gl->pc->FOR->ENDFOR->next;
            do_com_for_sub(gl->pc->FOR);
        } else {
            gl->nextpc = gl->pc->FOR->next;
            p = read_hash_local(gl->pc->FOR->fd->forstr,&hs);
            sprintf(error_buf, "%li", gl->pc->FOR->fd->forval);
            write_hash(hs, gl->pc->FOR->fd->forstr, error_buf);
        }

    }
    str = str;
    return 0;
}
#define COMMAND(str,string,len,func) if ( string[0]==str[0]\
		&&!strncmp(string,str,len)) { char *s=str+len;\
		READ_SPACES(s); if (func(s)) break; continue;}
#define COMMAND2(str,string,len,func) if ( string[0]==str[0]\
		&&!strncmp(string,str,len)) { char *s=str+len;\
		if (func(s)) break; continue;}
#define COMMAND2_NOFAIL(str,string,len,func) if ( string[0]==str[0]\
		&&!strncmp(string,str,len)) { char *s=str+len;\
		if (func(s)) { return -1; } continue;}
int
run_prg(void)
{
    int             err;
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
            if (calc_line2(gl->pc->str + 7, gl->linebuf))
                break;
            if (do_com_moveto(gl->linebuf))
                break;
            continue;
        }
        if (!strncmp(gl->pc->str, "SET", 3)) {
            if (calc_line2(gl->pc->str + 4, gl->linebuf))
                break;
            if (do_com_set(gl->linebuf))
                break;
            continue;
        }
        if (!strncmp(gl->pc->str, "CREATE", 6)) {
            if (calc_line2(gl->pc->str + 7, gl->linebuf))
                break;
            if (do_com_create(gl->linebuf))
                break;
            continue;
        }
        gl->s_pos = 0;
        gl->line_path = 0;
        err = calc_line(gl->pc->str, gl->linebuf);
        if (err)
            return err;

        COMMAND2(gl->linebuf,"PRINT",5,do_com_print)
            COMMAND2(gl->linebuf,"P ",2,do_com_print)
            COMMAND2(gl->linebuf,"P\t",2,do_com_print)
            COMMAND(gl->linebuf,"GOSUB",5,do_com_gosub)
            COMMAND(gl->linebuf,"CALL",4,do_com_gosub)
            COMMAND(gl->linebuf,"GOTO",4,do_com_goto)
            COMMAND(gl->linebuf,"RETURN",6,do_com_return)
            COMMAND(gl->linebuf,"PUSH",4,do_com_push)
            COMMAND(gl->linebuf,"POP",3,do_com_pop)
            COMMAND(gl->linebuf,"CONTINUE",8,do_com_next)


            COMMAND(gl->linebuf,"OPEN",4,do_com_open)
            COMMAND(gl->linebuf,"CLOSE",5,do_com_close)
            COMMAND(gl->linebuf,"OUT",3,do_com_out)
            COMMAND2_NOFAIL(gl->linebuf,"ERROR",5,do_com_error)
            COMMAND(gl->linebuf,"TABSTOP",7,do_com_tabstop)
            COMMAND(gl->linebuf,"TAB",3,do_com_tab)
            COMMAND(gl->linebuf,"PP",2,do_com_print2)
            COMMAND(gl->linebuf,"EXIT",4,do_com_exit)
            COMMAND(gl->linebuf,"DATA",4,do_com_data)
            COMMAND(gl->linebuf,"DBG",3,do_com_dbg)
            printf_error("Unknown Command '%s'", gl->pc->str);
        return -1;
    }
    return 0;
}
