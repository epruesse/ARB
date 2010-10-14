#include <stdio.h>
#include "global.h"

void count_bases(const Seq& seq, int *base_a, int *base_t, int *base_g, int *base_c, int *base_other) { // @@@ -> Seq
    // Count bases A, T, G, C and others
    int indi;

    (*base_a) = (*base_c) = (*base_t) = (*base_g) = (*base_other) = 0;

    const char *sequence = seq.get_seq();
    for (indi = 0; indi < seq.get_len(); indi++)
        switch (sequence[indi]) {
            case 'a':
            case 'A':
                (*base_a)++;
                break;
            case 't':
            case 'T':
            case 'u':
            case 'U':
                (*base_t)++;
                break;
            case 'g':
            case 'G':
                (*base_g)++;
                break;
            case 'c':
            case 'C':
                (*base_c)++;
                break;
            default:
                (*base_other)++;
        }
}

static const char *print_return_wrapped(FILE *fp, const char * const content, const int len, const WrapMode& wrapMode, const int rest_width, WrapBug behavior) {
    // @@@ currently reproduces all bugs found in the previously six individual wrapping functions

    ca_assert(content[len] == '\n');

    if (len<(rest_width+1)) {
        fputs(content, fp);
        return NULL; // no rest
    }

    int split_after = wrapMode.wrap_pos(content, rest_width);

    if (behavior&WRAPBUG_WRAP_AT_SPACE && split_after == 0) split_after = wrapMode.wrap_pos(content+1, rest_width-1)+1;
    if (behavior&WRAPBUG_WRAP_BEFORE_SEP && split_after>0) split_after--;
        
    ca_assert(split_after>0);
    if ((behavior&WRAPBUG_WRAP_BEFORE_SEP) == 0 && occurs_in(content[split_after], " \n")) split_after--;
    ca_assert(split_after >= 0);

    int continue_at = split_after+1;
    if (behavior&WRAPBUG_WRAP_AT_SPACE && split_after == rest_width) {
        // don't correct 'continue_at' in this case to emulate existing bug
    }
    else {
        while (occurs_in(content[continue_at], " \n")) continue_at++;
    }

    if (behavior&WRAPBUG_WRAP_BEFORE_SEP && content[split_after+1] == ' ' && content[split_after+2] == ' ') split_after++;

    ca_assert(content[split_after] != '\n');
    fputs_len(content, split_after+1, fp);
    fputc('\n', fp);

    ca_assert(content[len] == '\n');
    ca_assert(len>continue_at);
    
    return content+continue_at;
}

void print_wrapped(FILE *fp, const char *first_prefix, const char *other_prefix, const char *content, const WrapMode& wrapMode, int max_width, WrapBug behavior) { // @@@ WRAPPER
    // general function for line wrapping

    ca_assert(has_content(content));

    int len        = strlen(content)-1;
    int prefix_len = strlen(first_prefix);

    fputs(first_prefix, fp);
    const char *rest = print_return_wrapped(fp, content, len, wrapMode, max_width-prefix_len, behavior);

    if (rest) {
        prefix_len = strlen(other_prefix);

        while (rest) {
            len     -= rest-content;
            content  = rest;
            
            fputs(other_prefix, fp);
            rest = print_return_wrapped(fp, content, len, wrapMode, max_width-prefix_len, behavior);
        }
    }
}

