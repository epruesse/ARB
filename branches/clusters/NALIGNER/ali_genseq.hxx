

#ifndef _GENSEQ_INC_
#define _GENSEQ_INC_

class ALI_GENSEQ {
    unsigned long sequence_len;
    unsigned char (*sequence)[];

public:
    ALI_GENSEQ() {
        sequence_len = 0;
        sequence = 0;
    }
    ALI_GENSEQ(char *seq) {
        sequence_len = strlen(seq);
        sequence = (unsigned char (*)[]) strdup(seq);
    }
    ~ALI_GENSEQ() {
        if (sequence != 0) 
            free(sequence);
    }
    const unsigned long length() {
        return sequence_len;
    }
    char *const string() {
        return (char *) sequence;
    }
};


class ALI_NORM_GENSEQ {
    unsigned long num_of_bases;
    unsigned char (*bases)[];
    unsigned char (*start_pos)[];

    int is_base(char b);
    int is_base(int b);
    int normalize_base(char b);
    unsigned long number_of_bases(char *seq);
public:
    ALI_NORM_GENSEQ(char *a);
    ALI_NORM_GENSEQ(ALI_GENSEQ &a);

    int is_start(unsigned long pos) {
        if (pos > num_of_bases) 
            return 0;
        if ((*start_pos)[(pos/8)] & 1<<(7-(pos%8)))
            return 1;
        return 0;
    }
};

