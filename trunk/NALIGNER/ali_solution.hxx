

#ifndef _ALI_SOLUTION_INC_
#define _ALI_SOLUTION_INC_



// #include <malloc.h>

#include "ali_profile.hxx"
#include "ali_sequence.hxx"
#include "ali_tlist.hxx"

class ALI_MAP {
    unsigned long first_seq_base, last_seq_base;
    unsigned long first_ref_base, last_ref_base;
    long **mapping;
    unsigned char **inserted;
    unsigned char **undefined;
    unsigned long insert_counter;
public:
    ALI_MAP(unsigned long first_seq_base, unsigned long last_seq_base,
            unsigned long first_ref_base, unsigned long last_ref_base);
    ALI_MAP(ALI_MAP *map);
    ~ALI_MAP(void) {
        if (mapping)
            free((char *) mapping);
        if (inserted)
            free((char *) inserted);
        if (undefined)
            free((char *) undefined);
    }
    unsigned long first_base(void) {
        return first_seq_base;
    }
    unsigned long last_base(void) {
        return last_seq_base;
    }
    unsigned long first_reference_base(void) {
        return first_ref_base;
    }
    unsigned long last_reference_base(void) {
        return last_ref_base;
    }
    /*
     * Set position of base to position (relative to first_ref_base)
     */
    void set(unsigned long base, unsigned long pos, int insert = -1) {
        unsigned long b;

        if (base < first_seq_base || base > last_seq_base)
            ali_fatal_error("Base number out of range","ALI_MAP::set()");

        if (pos > last_ref_base - first_ref_base)
            ali_fatal_error("Position out of range","ALI_MAP::set()");

        b = base - first_seq_base;
        (*mapping)[b] = pos;
        (*undefined)[b/8] &= (unsigned char) ~(0x01<<(7-(b%8)));
        switch(insert) {
            case 0:
                if ((*inserted)[b/8]>>(7-(b%8)) & 0x01) {
                    if (insert_counter > 0)
                        insert_counter--;
                    else
                        ali_fatal_error("Inconsistent insert_counter",
                                        "ALI_MAP::set()");
                    (*inserted)[b/8] &= (unsigned char) ~(0x01<<(7-(b%8)));
                }
                break;
            case 1:
                if (!((*inserted)[b/8]>>(7-(b%8)) & 0x01)) {
                    (*inserted)[b/8] |= (unsigned char) (0x01<<(7-(b%8)));
                    insert_counter++;
                }
        }
    }
    long position(unsigned long base) {
        if (base < first_seq_base || base > last_seq_base)
            ali_fatal_error("Out of range","ALI_MAP::position()");
        return (*mapping)[base - first_seq_base];
    }
    unsigned long insertations(void) {
        return insert_counter;
    }
    int is_inserted(unsigned long base) {
        unsigned long b;
        if (base < first_seq_base && base > last_seq_base)
            ali_fatal_error("Out of range","ALI_MAP::inserted");
        b = base - first_seq_base;
        if (((*inserted)[b/8]>>(7-(b%8))) & 0x01)
            return 1;
        else
            return 0;
    }
    void undefine(unsigned long base) {
        unsigned long b;
        if (base < first_seq_base && base > last_seq_base)
            ali_fatal_error("Out of range","ALI_MAP::undefine()");
        b = base - first_seq_base;
        (*undefined)[b/8] |= (unsigned char) (0x01<<(7-(b%8)));
    }
    void unundefine(unsigned long base) {
        unsigned long b;
        if (base < first_seq_base && base > last_seq_base)
            ali_fatal_error("Out of range","ALI_MAP::unundefine()");
        b = base - first_seq_base;
        (*undefined)[b/8] &= (unsigned char) ~(0x01<<(7-(b%8)));
    }
    int is_undefined(unsigned long base) {
        unsigned long b;
        if (base < first_seq_base && base > last_seq_base)
            ali_fatal_error("Out of range","ALI_MAP::undefined()");
        b = base - first_seq_base;
        if (((*undefined)[b/8]>>(7-(b%8))) & 0x01)
            return 1;
        else
            return 0;
    }
    int have_undefined(void) {
        unsigned long b;
        for (b = first_seq_base; b <= last_seq_base; b++)
            if (is_undefined(b))
                return 1;
        return 0;
    }

    int is_konsistent(void);
    int is_equal(ALI_MAP *map) {
        unsigned long i;
        if (first_seq_base != map->first_seq_base ||
            last_seq_base != map->last_seq_base ||
            first_ref_base != map->first_ref_base ||
            last_ref_base != map->last_ref_base)
            return 0;
        for (i = 0; i < last_seq_base - first_seq_base + 1; i++)
            if ((*mapping)[i] != (*map->mapping)[i])
                return 0;
        for (i = 0; i < ((last_seq_base - first_seq_base) / 8) + 1; i++)
            if ((*inserted)[i] != (*map->inserted)[i])
                return 0;
        return 1;
    }
    ALI_SEQUENCE *sequence(ALI_NORM_SEQUENCE *ref_seq);
    ALI_SEQUENCE *sequence_without_inserts(ALI_NORM_SEQUENCE *ref_seq);
    ALI_MAP *inverse_without_inserts(void);
    char *insert_marker(void);
    void print(void) {
        unsigned long i;
        printf("Map: Bases %ld to %ld, Positions %ld to %ld\n",
               first_seq_base,last_seq_base,first_ref_base,last_ref_base);
        printf("Undefined : ");
        for (i = first_seq_base; i <= last_seq_base; i++)
            if (is_undefined(i))
                printf("%ld ",i);
        printf("\n");
        /*
          for (i = 0; i <= (last_seq_base - first_seq_base); i++)
          printf("%d, ",first_ref_base + (*mapping)[i]);
          printf("\n");
        */
    }
};

class ALI_SUB_SOLUTION {
    ALI_PROFILE *profile;
    ALI_TLIST<ALI_MAP *> map_list;

public:

    ALI_SUB_SOLUTION(ALI_PROFILE *prof) : map_list() {
        profile = prof;
    }
    ALI_SUB_SOLUTION(ALI_PROFILE *prof, ALI_MAP *map) : map_list(map) {
        profile = prof;
    }
    ALI_SUB_SOLUTION(ALI_SUB_SOLUTION *solution);
    ~ALI_SUB_SOLUTION(void);
    int free_area(unsigned long *start, unsigned long *end,
                  unsigned long *start_ref, unsigned long *end_ref,
                  unsigned long area_number = 0);
    unsigned long number_of_free_areas(void);
    int is_konsistent(ALI_MAP *map);
    int insert(ALI_MAP *map);
    int delete_map(ALI_MAP *map);
    ALI_MAP *make_one_map(void);
    void print(void);
};


#endif
