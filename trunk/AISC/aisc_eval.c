//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //

#include "aisc_eval.h"
#include "aisc_inline.h"
#include "aisc_proto.h"

char *Expression::eval_math(char *expr, char op_char) {
    char sep[] = "?,;";
    sep[0]     = op_char;
    
    char *brk = strpbrk(expr, sep);
    if (!brk) {
        print_error(&loc, formatted("Expected to see '%c', ',' or ';' in '%s'", op_char, expr));
        return NULL;
    }

    brk[0] = 0;
    int i1 = atoi(expr);
    brk[0] = op_char;
    int i2 = atoi(brk+1);
    int r;
    switch (op_char) {
        case '+': r = i1+i2; break;
        case '*': r = i1*i2; break;
        default :
            printf_error(&loc, "Unknown operator '%c'", op_char);
            return NULL;
    }
    
    return strdup(formatted("%i", r));
}

char *Expression::evalPart(int start, int end, int& resLen) {
    aisc_assert(start>0);

    char *res = NULL;
    resLen = -1;

    char  c           = ebuffer[end+1];
    ebuffer[end+1] = 0;
    char *part        = ebuffer+start;
    SKIP_SPACE_LF(part);

    if (start>end) { // empty "$()"
        res    = strdup("");
        resLen = 0;
    }
    else if (part[0] == '+' || part[0] == '*') {
        res = eval_math(part+1, part[0]);
    }
    else if (part[0] == '#') {
        if (strncmp(part, "#FILE", 5) == 0) {
            char *path = part+5;
            while (is_SPACE_LF_EOS(*path)) ++path;

            char *file = read_aisc_file(path, &loc);
            if (!file) {
                printf_error(&loc, "couldn't read file '%s'", path);
            }
            else {
                int         fileLen    = strlen(file);
                const char *sourcename = loc.get_path();
                int         sourceline = loc.get_linenr();
                aisc_assert(sourcename);

                int buflen = fileLen+strlen(path)+strlen(sourcename)+100;
                res        = (char *)malloc(buflen+1);

                // Inject code to restore correct code file and line (needed for proper error messages)
                int printed = sprintf(res, "@SETSOURCE %s,%i@%s@SETSOURCE %s,%i@",
                                      path, 1,
                                      file,
                                      sourcename, sourceline);
                if (printed >= buflen) {
                    fprintf(stderr, "%s:%i: Error: buffer overflow\n", __FILE__, __LINE__);
                }
                free(file);
            }
        }
        else {
            printf_error(&loc, "unknown eval-command '%s'", part);
        }
    }
    else {
        res = get_var_string(data, part, allow_missing_ref);
    }

    ebuffer[end+1] = c;

    if (resLen == -1 && res) resLen = strlen(res);
    return res;
}

bool Expression::evalRest(const int offset) {
    // evaluate content of 'ebuffer' starting at offset
    // return true on success
    // may reallocate ebuffer - so old pointers might get invalidated

    aisc_assert(strncmp(ebuffer+offset, "$(", 2) == 0);

    {
        const char *more_to_eval = strstr(ebuffer+offset+2, "$(");
        if (more_to_eval) {
            if (!evalRest(more_to_eval-ebuffer)) return false;
        }
    }

    int closing_paren;
    {
        const char *paren = strchr(ebuffer+offset+2, ')');
        if (!paren) {
            print_error(&loc, "unbalanced parens; missing ')'");
            return false;
        }
        closing_paren = paren-ebuffer;
    }

    int   eval_len;
    char *evaluated;

    if (offset>0 && ebuffer[offset-1] == '$') { // quoted "$$(...)"
        eval_len  = closing_paren-offset;
        evaluated = strduplen(ebuffer+offset+1, eval_len);
    }
    else {
        evaluated = evalPart(offset+2, closing_paren-1, eval_len);
    }

    if (!evaluated) {
        return false;
    }

    int org_len = closing_paren-offset+1;

    char *rest    = ebuffer+closing_paren+1;
    int   restlen = used-closing_paren;

    if (eval_len <= org_len) {
        if (eval_len < org_len) {
            aisc_assert(closing_paren+1 <= used);
            memmove(ebuffer+offset+eval_len, rest, restlen);
            used -= (org_len-eval_len);
        }
        memcpy(ebuffer+offset, evaluated, eval_len);
    }
    else {
        int growth = eval_len-org_len;
        if ((used+growth+1)>bufsize) { // need to increase ebuffer size
            int   new_bufsize = (used+growth+1)*3/2;
            char *new_linebuf = (char*)malloc(new_bufsize);

            memcpy(new_linebuf, ebuffer, offset);
            memcpy(new_linebuf+offset, evaluated, eval_len);
            aisc_assert(closing_paren+1 <= used);
            memcpy(new_linebuf+offset+eval_len, rest, restlen);

            free(ebuffer);
            ebuffer = new_linebuf;
            bufsize    = new_bufsize;
        }
        else {
            aisc_assert(closing_paren+1 <= used);
            memmove(ebuffer+offset+eval_len, rest, restlen);
            memcpy(ebuffer+offset, evaluated, eval_len);
        }
        used += growth;
        aisc_assert(used<bufsize);
    }

    aisc_assert(ebuffer[used] == 0);

    free(evaluated);

    char *ld = strstr(ebuffer+offset, "$(");
    if (ld) {
        int next_offset = ld-ebuffer;
        if (next_offset == offset) { // infinite loop in evaluation
            static int deadlock_offset;
            static int deadlock_count;

            if (next_offset == deadlock_offset) {
                ++deadlock_count;
                if (deadlock_count > 50) { // more than 50 evals at same offset in expression
                    printf_error(&loc, "detected (endless?) recursive evaluation in expression '%s'", ld);
                    return false;
                }
            }
            else {
                deadlock_offset = next_offset;
                deadlock_count  = 1;
            }
        }
        return evalRest(next_offset);
    }

    return true;
}

