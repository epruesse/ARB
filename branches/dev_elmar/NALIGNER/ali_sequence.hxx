// =============================================================== //
//                                                                 //
//   File      : ali_sequence.hxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ALI_SEQUENCE_HXX
#define ALI_SEQUENCE_HXX

#ifndef _GLIBCXX_CSTRING
#include <cstring>
#endif
#ifndef ALI_MISC_HXX
#include "ali_misc.hxx"
#endif


class ALI_SEQUENCE : virtual Noncopyable {
    unsigned char *seq;
    char *seq_name;
    unsigned long seq_len;
public:
    ALI_SEQUENCE(char *Name, char *str) {
        seq = (unsigned char *) str;
        seq_len = strlen(str);
        seq_name = strdup(Name);
        ali_string_to_sequence(str);
        if (seq_name == 0) ali_fatal_error("Out of memory");
    }
    ALI_SEQUENCE(char *Name, unsigned char *str, unsigned long str_len) {
        seq = str;
        seq_len = str_len;
        seq_name = strdup(Name);
    }
    ~ALI_SEQUENCE() {
        if (seq)
            free((char *) seq);
        if (seq_name)
            free((char *) seq_name);
    }
    unsigned char *sequence() {
        return seq;
    }
    unsigned char base(unsigned long position) {
        return seq[position];
    }
    int check();
    char *string();
    char *name() {
        return seq_name;
    }
    unsigned long length() {
        return seq_len;
    }
};


class ALI_NORM_SEQUENCE : virtual Noncopyable {
    unsigned char *seq;
    char *seq_name;
    unsigned char **dots;
    unsigned long seq_len;
public:
    ALI_NORM_SEQUENCE(char *name, char *str);
    ALI_NORM_SEQUENCE(ALI_SEQUENCE *sequence);
    ~ALI_NORM_SEQUENCE() {
        if (seq)
            free((char *) seq);
        if (seq_name)
            free((char *) seq_name);
        if (dots)
            free((char *) dots);
    }
    unsigned char *sequence() {
        return seq;
    }
    unsigned char base(unsigned long position) {
        return seq[position];
    }
    char *string();
    char *name() {
        return seq_name;
    }
    unsigned long length() {
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

#else
#error ali_sequence.hxx included twice
#endif // ALI_SEQUENCE_HXX
