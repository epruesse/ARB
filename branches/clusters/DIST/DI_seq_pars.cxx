#include <stdio.h>
//#include <memory.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <awt_tree.hxx>
#include "dist.hxx"

char    *AP_sequence_parsimony::table;
AP_weights *AP_sequence::weights;
AP_rates *AP_sequence::rates;

AP_sequence_parsimony::AP_sequence_parsimony(void)
{
    update = 0;
    sequence_len = 0;
    sequence = 0;
    is_set_flag = AP_FALSE;
}

AP_sequence_parsimony::~AP_sequence_parsimony(void)
{
    delete sequence;
}

AP_sequence *AP_sequence_parsimony::dup(void)
{
    return (AP_sequence *)new AP_sequence_parsimony;
}

void AP_sequence_parsimony::build_table(void)
{
    int     i;
    AP_BASES        val;
    table = new char[256];
    for (i=0;i<256;i++) {
        switch ((char)i) {
            case 'a':
            case 'A':
                val = AP_A;
                break;
            case 'g':
            case 'G':
                val = AP_G;
                break;
            case 'c':
            case 'C':
                val = AP_C;
                break;
            case 't':
            case 'T':
            case 'u':
            case 'U':
                val = AP_T;
                break;
            case 'n':
            case 'N':
                val = (AP_BASES)(AP_A + AP_G + AP_C + AP_T);
                break;
            case '?':
            case '.':
                val = (AP_BASES)(AP_A + AP_G + AP_C + AP_T + AP_S);
                break;
            case '-':
                val = AP_S;
                break;
            case 'm':
            case 'M':
                val = (AP_BASES)(AP_A+AP_C);
                break;
            case 'r':
            case 'R':
                val = (AP_BASES)(AP_A+AP_G);
                break;
            case 'w':
            case 'W':
                val = (AP_BASES)(AP_A+AP_T);
                break;
            case 's':
            case 'S':
                val = (AP_BASES)(AP_C+AP_G);
                break;
            case 'y':
            case 'Y':
                val = (AP_BASES)(AP_C+AP_T);
                break;
            case 'k':
            case 'K':
                val = (AP_BASES)(AP_G+AP_T);
                break;
            case 'v':
            case 'V':
                val = (AP_BASES)(AP_A+AP_C+AP_G);
                break;
            case 'h':
            case 'H':
                val = (AP_BASES)(AP_A+AP_C+AP_T);
                break;
            case 'd':
            case 'D':
                val = (AP_BASES)(AP_A+AP_G+AP_T);
                break;
            case 'b':
            case 'B':
                val = (AP_BASES)(AP_C+AP_G+AP_T);
                break;
            default:
                val = AP_N;
                break;
        }
        table[i] = (char)val;
    } /*for*/
}

        
        
void AP_sequence_parsimony::set(char *isequence)
{
    char *s,*d,*f,c;
    sequence_len = filter->real_len;
    sequence = new char[sequence_len+1];
    memset(sequence,AP_N,(size_t)sequence_len);
    s = isequence;
    d = sequence;
    f = filter->filter;
    if (!table) this->build_table();
    while (c = (*s++) ) {
        if (*(f++)) {
            *(d++) = table[c];
        }               
    }
    update = AP_timer();
    is_set_flag = AP_TRUE;
}



AP_FLOAT AP_sequence_parsimony::combine(const AP_sequence *lefts, const AP_sequence *rights) {
    float tree_length = 0.0;
    AP_sequence_parsimony *left = (AP_sequence_parsimony *)lefts;
    AP_sequence_parsimony *right = (AP_sequence_parsimony *)rights;
        
    if (!sequence)  {
        sequence = new char[filter->real_len +1];
    }
    for (int i = 0; i <= sequence_len;i++ ) {
        if ((left->sequence[i] & right->sequence[i]) == 0) {
            sequence[i] = (left->sequence[i] | right->sequence[i]); }
        else {
            sequence[i] = (left->sequence[i] & right->sequence[i]);
            tree_length = tree_length + weights->weights[i] + rates->rates[i];             
        }
    }
    is_set_flag = AP_TRUE;                  
    return tree_length;
}



void AP_sequence_parsimony::calc_base_rates(AP_rates *ratesi,
                                            const AP_sequence* lefts, AP_FLOAT leftl,
                                            const AP_sequence *rights, AP_FLOAT rightl)
{
    ;

}


