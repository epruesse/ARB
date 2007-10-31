

#include <string.h>
#include <stdlib.h>
// #include <malloc.h>

#include "ali_misc.hxx"
#include "ali_sequence.hxx"



/*****************************************************************************
 *
 *  ALI_SEQUENCE
 *
 *****************************************************************************/


int ALI_SEQUENCE::check(void)
{
    unsigned char *seq_buf;
    unsigned long i;

    seq_buf = seq;
    for (i = 0; i < seq_len; i++, seq_buf++)
        if (*seq_buf > 6)
            ali_fatal_error("STOP");

    return 1;
}

char *ALI_SEQUENCE::string(void)
{
    char *str, *str_buf;
    unsigned char *seq_buf;
    unsigned long i;

    str = (char *) CALLOC((unsigned int) seq_len + 1,sizeof(char));

    str_buf = str;
    seq_buf = seq;
    for (i = 0; i < seq_len; i++) {
        *(str_buf++) = *(seq_buf++);
    }
    *str_buf = '\0';
    ali_sequence_to_string((unsigned char*) str,seq_len);

    return str;
}

/*****************************************************************************
 *
 *  ALI_NORM_SEQUENCE
 *
 *****************************************************************************/

ALI_NORM_SEQUENCE::ALI_NORM_SEQUENCE(char *name, char *string)
{
    unsigned long counter;
    unsigned char *s;
    int dot_flag;
    char *str;

    /*
     * Count only _BASES_
     */
    for (counter = 0, str = string; *str != '\0'; str++)
        if (ali_is_base(*str))
            counter++;
    seq_len = counter;

    seq = (unsigned char*) CALLOC((unsigned int) seq_len,sizeof(unsigned char));
    dots = (unsigned char **) CALLOC((unsigned int) (seq_len/8)+1, sizeof(unsigned char));
    //dots = (unsigned char (*) [1]) CALLOC((unsigned int) (seq_len/8)+1, sizeof(unsigned char));
    seq_name = strdup(name);
    if (seq == 0 || dots == 0 || seq_name == 0) {
        ali_fatal_error("Out of memory");
    }

    dot_flag = 0;
    (*dots)[0] |= (unsigned char) (1<<7);
    for (counter = 0, str = string, s = seq; *str != '\0'; str++) {
        if (ali_is_base(*str)) {
            *s++ = ali_base_to_number(*str);
            dot_flag = 0;
            counter++;
        }
        else {
            if (dot_flag == 0 && ali_is_dot(*str)) {
                (*dots)[(counter/8)] |= (unsigned char) (1<<(7-(counter%8)));
                dot_flag = 1;
            }
        }
    }
}

ALI_NORM_SEQUENCE::ALI_NORM_SEQUENCE(ALI_SEQUENCE *sequence)
{
    unsigned long counter, pos;
    unsigned char *s;
    int dot_flag;
    unsigned char *str;

    for (counter = pos = 0, str = sequence->sequence();
         pos < sequence->length(); pos++, str++)
        if (ali_is_base(*str))
            counter++;
    seq_len = counter;

    seq = (unsigned char*) CALLOC((unsigned int) seq_len,sizeof(unsigned char));
    dots = (unsigned char **) CALLOC((unsigned int) (seq_len/8)+1, sizeof(unsigned char));
    //dots = (unsigned char (*) [1]) CALLOC((unsigned int) (seq_len/8)+1, sizeof(unsigned char));
    seq_name = strdup(sequence->name());
    if (seq == 0 || dots == 0 || seq_name == 0) {
        ali_fatal_error("Out of memory");
    }

    dot_flag = 0;
    (*dots)[0] |= (unsigned char) (1<<7);
    for (counter = pos = 0, str = sequence->sequence(), s = seq;
         pos < sequence->length(); str++, pos++) {
        if (ali_is_base(*str)) {
            *s++ = *str;
            dot_flag = 0;
            counter++;
        }
        else {
            if (dot_flag == 0 && ali_is_dot(*str)) {
                (*dots)[(counter/8)] |= (unsigned char) (1<<(7-(counter%8)));
                dot_flag = 1;
            }
        }
    }
}

char *ALI_NORM_SEQUENCE::string()
{
    char *str, *str_buf;
    unsigned char *seq_buf;
    unsigned long i;

    str = (char *) CALLOC((unsigned int) seq_len + 1,sizeof(char));
    if (str == 0)
        ali_fatal_error("Out of memory");

    for (i = seq_len, str_buf = str, seq_buf = seq; i-- > 0;)
        *str_buf++ = *seq_buf++;
    *str_buf = '\0';

    ali_sequence_to_string((unsigned char*) str,seq_len);

    return str;
}

