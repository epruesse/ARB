
#ifndef _ALI_SEQUENCE_INC_
#define _ALI_SEQUENCE_INC_

#include <string.h>
// #include <malloc.h>

#include "ali_misc.hxx"


class ALI_SEQUENCE {
    unsigned char *seq;
    char *seq_name;
    unsigned long seq_len;
public:
    ALI_SEQUENCE(char *name, char *str) {
        seq = (unsigned char *) str;
        seq_len = strlen(str);
        seq_name = strdup(name);
        ali_string_to_sequence(str);
        if (seq_name == 0) ali_fatal_error("Out of memory");
    }
    ALI_SEQUENCE(char *name, unsigned char *str, unsigned long str_len) {
        seq = str;
        seq_len = str_len;
        seq_name = strdup(name);
    }
    ~ALI_SEQUENCE(void) {
        if (seq)
            free((char *) seq);
        if (seq_name)
            free((char *) seq_name);
    }
    unsigned char *sequence(void) {
        return seq;
    }
    unsigned char base(unsigned long position) {
        return seq[position];
    }
    int check(void);
    char *string(void);
    char *name(void) {
        return seq_name;
    }
    unsigned long length(void) {
        return seq_len;
    }
};


class ALI_NORM_SEQUENCE {
    unsigned char *seq;
    char *seq_name;
    unsigned char **dots;
    unsigned long seq_len;
public:
    ALI_NORM_SEQUENCE(char *name, char *str);
    ALI_NORM_SEQUENCE(ALI_SEQUENCE *sequence);
    ~ALI_NORM_SEQUENCE(void) {
        if (seq)
            free((char *) seq);
        if (seq_name)
            free((char *) seq_name);
        if (dots)
            free((char *) dots);
    }
    unsigned char *sequence(void) {
        return seq;
    }
    unsigned char base(unsigned long position) {
        return seq[position];
    }
    char *string(void);
    char *name(void) {
        return seq_name;
    }
    unsigned long length(void) {
        return seq_len;
    }
    int is_begin(unsigned long pos) {
        if (dots == 0)
            return 0;
        else {
            if (pos > seq_len)
                return 1;
            else
                return (((*dots)[pos/8] & (unsigned char) (1<<(7-(pos%8)))) != 0);
        }
    }
};

#endif
