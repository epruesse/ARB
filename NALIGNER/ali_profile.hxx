#ifndef _ALI_PROFILE_INC_
#define _ALI_PROFILE_INC_

#include "ali_sequence.hxx"
#include "ali_arbdb.hxx"
#include "ali_pt.hxx"
#include "ali_tlist.hxx"
#include "ali_misc.hxx"

#define ALI_PROFILE_BORDER_LEFT '['
#define ALI_PROFILE_BORDER_RIGHT ']'

typedef void ALI_MAP_; /* make modul independent */


typedef struct {
    ALI_ARBDB *arbdb;
    ALI_PT *pt;
        
    int find_family_mode;

    int exclusive_flag;
    int mark_family_flag;
    int mark_family_extension_flag;

    int min_family_size;
    int max_family_size;

    float min_weight;
    float ext_max_weight;

    float multi_gap_factor;

    float insert_factor;
    float multi_insert_factor;

    double substitute_matrix[5][5]; /* a c g u - */
    double binding_matrix[5][5]; /* a c g u - */

} ALI_PROFILE_CONTEXT;


/*
 * Class for a family member
 */
class ali_family_member {
public:
    ALI_SEQUENCE *sequence;
    float matches;
    float weight;
 
    ali_family_member(ALI_SEQUENCE *seq, float m, float w = 0.0) {
        sequence = seq;
        matches = m;
        weight = w;
    }
};


/*
 * Class for the profiling
 */
class ALI_PROFILE {
    ALI_NORM_SEQUENCE *norm_sequence;
    unsigned long prof_len;

    long **helix;                    /* base to base connection */
    char **helix_borders;            /* borders of the helix '[' and ']' */
    unsigned long helix_len;        

    float (**base_weights)[4];         /* relative weight of base i */
    float (**sub_costs)[6];            /* costs to substitute with base i */
    float sub_costs_maximum;

    float insert_cost;
    float multi_insert_cost;

    float multi_gap_factor;

    float (*binding_costs)[5][5];       /* Matrix for binding costs a c g u - */
    float w_bind_maximum;

    unsigned long *lmin, *lmax; 
    float ***gap_costs;
    float ***gap_percents;

    int is_binding_marker(char c);

    ALI_TLIST<ali_family_member *> *find_family(ALI_SEQUENCE *sequence, 
                                                ALI_PROFILE_CONTEXT *context);
    void calculate_costs(ALI_TLIST<ali_family_member *> *family_list,
                         ALI_PROFILE_CONTEXT *context);

    int find_next_helix(char h[], unsigned long h_len, unsigned long pos,
                        unsigned long *helix_nr,
                        unsigned long *start, unsigned long *end);
    int find_comp_helix(char h[], unsigned long h_len, unsigned long pos,
                        unsigned long helix_nr,
                        unsigned long *start, unsigned long *end);
    void delete_comp_helix(char h1[], char h2[], unsigned long h_len,
                           unsigned long start, unsigned long end);
    int map_helix(char h[], unsigned long h_len,
                  unsigned long start1, unsigned long end1,
                  unsigned long start2, unsigned long end2);
    void initialize_helix(ALI_PROFILE_CONTEXT *context);

public:

    ALI_PROFILE(ALI_SEQUENCE *sequence, ALI_PROFILE_CONTEXT *context);
    ~ALI_PROFILE(void);

    void print(int start = -1, int end = -1)
    {
        int i;
        size_t j;

        if (start < 0 || (unsigned long)(start) > prof_len - 1)
            start = 0;

        if ((unsigned long)(end) > prof_len - 1 || end < 0)
            end = (int)prof_len - 1;

        if (start > end) {
            i = start;
            start = end;
            end = i;
        }
                        

        printf("\nProfile from %d to %d\n",start,end);
        printf("Substitutions kosten:\n");
        for (i = start; i <= end; i++) {
            printf("%2d: ",i);
            for (j = 0; j < 6; j++)
                printf("%4.2f ",(*sub_costs)[i][j]);
            printf("\n");
        }

        printf("\nGap Bereiche:\n");
        for (i = start; i <= end; i++) {
            printf("%2d: %3ld %3ld\n",i,lmin[i],lmax[i]);
            printf("  : ");
            for (j = 0; j <= lmax[i] - lmin[i] + 1; j++)
                printf("%4.2f ",(*gap_percents)[i][j]);
            printf("\n  : ");
            for (j = 0; j <= lmax[i] - lmin[i] + 1; j++)
                printf("%4.2f ",(*gap_costs)[i][j]);
            printf("\n");
        }

    }

    int is_in_helix(unsigned long pos, 
                    unsigned long *first, unsigned long *last);
    int is_outside_helix(unsigned long pos, 
                         unsigned long *first, unsigned long *last);
    int complement_position(unsigned long pos) {
        if (pos >= helix_len)
            return -1;
        else
            return (*helix)[pos];
    }
    int is_in_helix(unsigned long pos) {
        long l;
        if ((*helix_borders)[pos] == ALI_PROFILE_BORDER_LEFT ||
            (*helix_borders)[pos] == ALI_PROFILE_BORDER_RIGHT)
            return 1;
        for (l = (long) pos - 1; l >= 0; l--)
            switch ((*helix_borders)[l]) {
                case ALI_PROFILE_BORDER_LEFT:
                    return 1;
                case ALI_PROFILE_BORDER_RIGHT:
                    return 0;
            }
        for (l = (long) pos + 1; l < (long) prof_len; l++)
            switch((*helix_borders)[l]) {
                case ALI_PROFILE_BORDER_LEFT:
                    return 0;
                case ALI_PROFILE_BORDER_RIGHT:
                    return 1;
            }
        return 0;
    }

    int is_internal_last(unsigned long pos) {
        return norm_sequence->is_begin(pos + 1);
    }
                          
    char *cheapest_sequence(void);      
    char *borders_sequence(void) {
        unsigned long i;
        char *str, *str_buffer;

        str = (char *) calloc((unsigned int) prof_len, sizeof(char));
        if (str == 0) ali_fatal_error("Out of memory");

        str_buffer = str;
        for (i = 0; i < prof_len; i++)
            switch ((*helix_borders)[i]) {
                case ALI_PROFILE_BORDER_LEFT: *str++ = '['; break;
                case ALI_PROFILE_BORDER_RIGHT: *str++ = ']'; break;
                default: *str++ = ' ';
            }
        return str_buffer;
    }

    unsigned long length(void) {
        return prof_len;
    }
    ALI_NORM_SEQUENCE *sequence(void) {
        return norm_sequence;
    }
    unsigned long sequence_length(void) {
        return norm_sequence->length();
    }

    float base_weight(unsigned long pos, unsigned char c) {
        if (c > 3)
            ali_fatal_error("Out of range","ALI_PROFILE::base_weight()");
        return (*base_weights)[pos][c];
    }

    float w_sub(unsigned long positiony, unsigned long positionx) {
        return (*sub_costs)[positiony][norm_sequence->base(positionx)];
    }
    float w_sub_gap(unsigned long positiony) {
        return (*sub_costs)[positiony][4];
    }
    float w_sub_gap_cheap(unsigned long positiony) {
        return (*sub_costs)[positiony][4] * multi_gap_factor;
    }
    float w_sub_multi_gap_cheap(unsigned long start, unsigned long end) {
        float costs = 0.0;
        unsigned long i;
        for (i = start; i <= end; i++)
            costs += w_sub_gap(i);
        return costs * multi_gap_factor;
    }

    float w_sub_maximum(void) {
        return sub_costs_maximum;
    }

    float w_del(unsigned long begin, unsigned long end) {
        if (begin <= lmin[end])
            return (*gap_costs)[end][0];
        else {
            if (begin <= lmax[end])
                return (*gap_costs)[end][begin - lmin[end]];
            else
                return (*gap_costs)[end][lmax[end] - lmin[end] + 1];
        }
    }
    float w_del_cheap(unsigned long position) {
        return (*gap_costs)[position][0];
    }
    float w_del_multi(unsigned long start, unsigned long end) {
        float costs = 0.0;
        unsigned long i;
        for (i = start; i <= end; i++)
            costs += w_del(start,i);
        return costs;
    }
    float w_del_multi_cheap(unsigned long start, unsigned long end) {
        float costs = 0.0;
        unsigned long i;
        for (i = start; i <= end; i++)
            costs += w_del_cheap(i);
        return costs;
    }
    float w_del_multi_unweighted(unsigned long start, unsigned long end) {
        float costs = 0.0;
        unsigned long i;
        for (i = start; i <= end; i++)
            costs += w_del(i,i);
        return costs;
    }

    float w_ins(unsigned long /*positionx*/, unsigned long /*positiony*/) {
        return insert_cost;
    }
    float w_ins_cheap(unsigned long /*positionx*/, unsigned long /*positiony*/) {
        return multi_insert_cost;
    }
    float w_ins_multi_unweighted(unsigned long startx, unsigned long endx) {
        return (endx - startx + 1) * insert_cost;
    }
    float w_ins_multi(unsigned long startx, unsigned long endx) {
        return insert_cost + (endx - startx)*multi_insert_cost;
    }
    float w_ins_multi_cheap(unsigned long startx, unsigned long endx) {
        return (endx - startx + 1)*multi_insert_cost;
    }


    float gap_percent(unsigned long begin, unsigned long end) {
        if (begin <= lmin[end])
            return (*gap_percents)[end][0];
        else {
            if (begin <= lmax[end])
                return (*gap_percents)[end][begin - lmin[end]];
            else
                return (*gap_percents)[end][lmax[end] - lmin[end] + 1];
        }
    }

    float w_bind(unsigned long pos_1, unsigned char base_1, 
                 unsigned long pos_2, unsigned char base_2) {
        int i, j;
        float cost_in, cost = 0;
        if (ali_is_real_base_or_gap(base_1)) {
            if (ali_is_real_base_or_gap(base_2)) {
                return (*binding_costs)[base_1][base_2];
            }
            for (i = 0; i < 4; i++)
                cost += (*base_weights)[pos_2][i] * (*binding_costs)[base_1][i];
            return cost;
        }
        if (ali_is_real_base_or_gap(base_2)) {
            for (i = 0; i < 4; i++)
                cost += (*base_weights)[pos_1][i] * (*binding_costs)[i][base_2];
            return cost;
        }
        for (i = 0; i < 4; i++) {
            for (j = 0, cost_in = 0; j < 4; j++)
                cost_in += (*base_weights)[pos_2][j] * (*binding_costs)[i][j];
            cost += (*base_weights)[pos_1][i] * cost_in;
        }
        return cost;
    }
                
    float w_binding(unsigned long first_position_of_sequence,
                    ALI_SEQUENCE *sequence);
};


#endif
