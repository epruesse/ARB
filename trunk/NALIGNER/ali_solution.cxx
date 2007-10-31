

// #include <malloc.h>
#include <stdlib.h>
#include "ali_misc.hxx"
#include "ali_solution.hxx"


/*****************************************************************************
 *
 * ALI_MAP
 *
 *****************************************************************************/

ALI_MAP::ALI_MAP(unsigned long first_seq, unsigned long last_seq,
                 unsigned long first_ref, unsigned long last_ref)
{
    unsigned long l;

    insert_counter = 0;
    first_seq_base = first_seq;
    last_seq_base = last_seq;
    first_ref_base = first_ref;
    last_ref_base = last_ref;

    mapping = (long **) CALLOC((unsigned int) (last_seq_base - first_seq_base + 1),sizeof(long));
    //mapping = (long (*) [1]) CALLOC((unsigned int) (last_seq_base - first_seq_base + 1),sizeof(long));
    inserted = (unsigned char **) CALLOC((unsigned int) ((last_seq_base - first_seq_base)/8) + 1,sizeof(unsigned char));
    //inserted = (unsigned char (*) [1]) CALLOC((unsigned int) ((last_seq_base - first_seq_base)/8) + 1,sizeof(unsigned char));
    undefined = (unsigned char **) CALLOC((unsigned int) ((last_seq_base - first_seq_base)/8) + 1,sizeof(unsigned char));
    //undefined = (unsigned char (*) [1]) CALLOC((unsigned int) ((last_seq_base - first_seq_base)/8) + 1,sizeof(unsigned char));
    if (mapping == 0 || inserted == 0 || undefined == 0) {
        ali_fatal_error("Out of memory");
    }

    for (l = 0; l < (last_seq_base - first_seq_base)/8 + 1; l++)
        (*undefined)[l] = 0xff;
}

ALI_MAP::ALI_MAP(ALI_MAP *map)
{
    unsigned long l;

    first_seq_base = map->first_seq_base;
    last_seq_base = map->last_seq_base;
    first_ref_base = map->first_ref_base;
    last_ref_base = map->last_ref_base;

    mapping = (long **) CALLOC((unsigned int) (last_seq_base - first_seq_base + 1), sizeof(long));
    //mapping = (long (*) [1]) CALLOC((unsigned int) (last_seq_base - first_seq_base + 1), sizeof(long));
    inserted = (unsigned char **) CALLOC((unsigned int) ((last_seq_base - first_seq_base) / 8) + 1,sizeof(unsigned char));
    //inserted = (unsigned char (*) [1]) CALLOC((unsigned int) ((last_seq_base - first_seq_base) / 8) + 1,sizeof(unsigned char));
    undefined = (unsigned char **) CALLOC((unsigned int) ((last_seq_base - first_seq_base) / 8) + 1,sizeof(unsigned char));
    //undefined = (unsigned char (*) [1]) CALLOC((unsigned int) ((last_seq_base - first_seq_base) / 8) + 1,sizeof(unsigned char));
    if (mapping == 0 || inserted == 0 || undefined == 0) {
        ali_fatal_error("Out of memory");
    }

    for (l = 0; l < last_seq_base - first_seq_base + 1; l++) {
        if (l < (last_seq_base - first_seq_base)/8 + 1) {
            (*inserted)[l] = (*map->inserted)[l];
            (*undefined)[l] = (*map->undefined)[l];
        }
        (*mapping)[l] = (*map->mapping)[l];
    }
}

int ALI_MAP::is_konsistent(void)
{
    unsigned long i;

    for (i = 1; i <= last_seq_base - first_seq_base; i++)
        if ((*mapping)[i-1] >= (*mapping)[i])
            return 0;

    return 1;
}

ALI_SEQUENCE *ALI_MAP::sequence(ALI_NORM_SEQUENCE *ref_seq)
{
    int ins_counter;
    int begin_flag = 0, undefined_flag;
    unsigned long map_pos, seq_pos;
    ALI_SEQUENCE *sequence;
    unsigned char *seq, *seq_buffer;

    seq_buffer = (unsigned char *) CALLOC((unsigned int)
                                          (last_ref_base - first_ref_base + insert_counter + 1),
                                          sizeof(unsigned char));

    if (seq_buffer == 0)
        ali_fatal_error("Out of memory");

    seq = seq_buffer;
    seq_pos = 0;
    undefined_flag = 0;
    ins_counter = 0;
    for (map_pos = 0; map_pos <= last_seq_base - first_seq_base; map_pos++) {
        if (!undefined_flag)
            begin_flag = ref_seq->is_begin(first_seq_base + map_pos);
        if (!(((*undefined)[map_pos/8]>>(7-(map_pos%8))) & 0x01)) {
            for (;seq_pos < (unsigned long)((*mapping)[map_pos] + ins_counter); seq_pos++) {
                if (begin_flag)
                    *seq++ = ALI_DOT_CODE;
                else
                    *seq++ = ALI_GAP_CODE;
            }
            *seq++ = ref_seq->base(first_seq_base + map_pos);
            seq_pos++;
            undefined_flag = 0;
        }
        else {
            undefined_flag = 1;
        }
        if ((*inserted)[map_pos/8]>>(7-(map_pos%8)) & 0x01)
            ins_counter++;
    }

    begin_flag = ref_seq->is_begin(first_seq_base + map_pos);
    for (;seq_pos <= last_ref_base - first_ref_base + ins_counter;seq_pos++) {
        if (begin_flag)
            *seq++ = ALI_DOT_CODE;
        else
            *seq++ = ALI_GAP_CODE;
    }

    sequence = new ALI_SEQUENCE(ref_seq->name(),seq_buffer,
                                last_ref_base - first_ref_base + insert_counter + 1);

    return sequence;
}

ALI_SEQUENCE *ALI_MAP::sequence_without_inserts(ALI_NORM_SEQUENCE *ref_seq)
{
    int begin_flag = 0, undefined_flag;
    unsigned long map_pos, seq_pos;
    ALI_SEQUENCE *sequence;
    unsigned char *seq, *seq_buffer;

    seq_buffer = (unsigned char *) CALLOC((unsigned int)
                                          (last_ref_base - first_ref_base + 1), sizeof(unsigned char));
    if (seq_buffer == 0)
        ali_fatal_error("Out of memory");

    seq = seq_buffer;
    seq_pos = 0;
    undefined_flag = 0;
    for (map_pos = 0; map_pos <= last_seq_base - first_seq_base; map_pos++) {
        if (!undefined_flag)
            begin_flag = ref_seq->is_begin(first_seq_base + map_pos);
        if (!((*undefined)[map_pos/8]>>(7-(map_pos%8)) & 0x01) &&
            !((*inserted)[map_pos/8]>>(7-(map_pos%8)) & 0x01)) {
            for (;seq_pos < (unsigned long)((*mapping)[map_pos]); seq_pos++) {
                if (begin_flag)
                    *seq++ = ALI_DOT_CODE;
                else
                    *seq++ = ALI_GAP_CODE;
            }
            *seq++ = ref_seq->base(first_seq_base + map_pos);
            seq_pos++;
            undefined_flag = 0;
        }
        else
            undefined_flag = 1;
    }

    begin_flag = ref_seq->is_begin(first_seq_base + map_pos);
    for (;seq_pos <= last_ref_base - first_ref_base; seq_pos++) {
        if (begin_flag)
            *seq++ = ALI_DOT_CODE;
        else
            *seq++ = ALI_GAP_CODE;
    }

    sequence = new ALI_SEQUENCE(ref_seq->name(),seq_buffer,
                                last_ref_base - first_ref_base + 1);

    return sequence;
}

ALI_MAP *ALI_MAP::inverse_without_inserts(void)
{
    unsigned long map_pos;
    ALI_MAP *inv_map;

    inv_map = new ALI_MAP(first_ref_base, last_ref_base,
                          first_seq_base, last_seq_base);

    for (map_pos = 0; map_pos <= last_seq_base - first_seq_base; map_pos++) {
        if (!((*undefined)[map_pos/8]>>(7-(map_pos%8)) & 0x01) &&
            !((*inserted)[map_pos/8]>>(7-(map_pos%8)) & 0x01)) {
            inv_map->set(first_ref_base + (*mapping)[map_pos],
                         map_pos);
        }
    }

    return inv_map;
}

char *ALI_MAP::insert_marker(void)
{
    int ins_counter;
    unsigned long map_pos, seq_pos;
    char *seq, *seq_buffer;

    seq_buffer = (char *) CALLOC( (last_ref_base - first_ref_base + insert_counter + 2),
                                  sizeof(char));

    seq = seq_buffer;
    seq_pos = 0;
    ins_counter = 0;
    for (map_pos = 0; map_pos <= last_seq_base - first_seq_base; map_pos++) {
        if (!(((*undefined)[map_pos/8]>>(7-(map_pos%8))) & 0x01)) {
            for (;seq_pos < (unsigned long)((*mapping)[map_pos] + ins_counter); seq_pos++) {
                *seq++ = '.';
            }
            if ((*inserted)[map_pos/8]>>(7-(map_pos%8)) & 0x01)
                *seq++ = 'X';
            else
                *seq++ = '.';
            seq_pos++;
        }
        if ((*inserted)[map_pos/8]>>(7-(map_pos%8)) & 0x01)
            ins_counter++;
    }

    for (;seq_pos <= last_ref_base - first_ref_base + ins_counter;seq_pos++) {
        *seq++ = '.';
    }

    *seq = '\0';

    return seq_buffer;
}


/*****************************************************************************
 *
 * ALI_SUB_SOLUTION
 *
 *****************************************************************************/

ALI_SUB_SOLUTION::ALI_SUB_SOLUTION(ALI_SUB_SOLUTION *solution)
{
    ALI_MAP *map;
    ALI_TLIST<ALI_MAP *> *list;

    profile = solution->profile;

    if (!solution->map_list.is_empty()) {
        list = &solution->map_list;
        map = new ALI_MAP(list->first());
        map_list.append_end(map);
        while (list->is_next()) {
            map = new ALI_MAP(list->next());
            map_list.append_end(map);
        }
    }
}

ALI_SUB_SOLUTION::~ALI_SUB_SOLUTION(void)
{
    ALI_MAP *map;

    if (!map_list.is_empty()) {
        map = map_list.first();
        delete map;
        while (map_list.is_next()) {
            map = map_list.next();
            delete map;
        }
    }
}

int ALI_SUB_SOLUTION::free_area(
                                unsigned long *start, unsigned long *end,
                                unsigned long *start_ref, unsigned long *end_ref,
                                unsigned long area_number)
{
    ALI_MAP *map;
    unsigned long last_of_prev, last_of_prev_ref;
    unsigned long area_number_akt;

    if (map_list.is_empty()) {
        if (area_number > 0)
            return 0;
        *start = 0;
        *end = profile->sequence_length() - 1;
        *start_ref = 0;
        *end_ref = profile->length() - 1;
        return 1;
    }

    area_number_akt = 0;
    map = map_list.first();
    if (map->first_base() > 0 &&
        map->first_reference_base() > 0) {
        if (area_number_akt == area_number) {
            *start = 0;
            *end = map->first_base() - 1;
            *start_ref = 0;
            *end_ref = map->first_reference_base() - 1;
            return 1;
        }
        area_number_akt++;
    }

    last_of_prev = map->last_base();
    last_of_prev_ref = map->last_reference_base();
    while (map_list.is_next()) {
        map = map_list.next();
        if (map->first_base() > last_of_prev + 1) {
            if (area_number_akt == area_number) {
                *start = last_of_prev + 1;
                *end = map->first_base() - 1;
                *start_ref = last_of_prev_ref + 1;
                *end_ref = map->first_reference_base() - 1;
                return 1;
            }
            area_number_akt++;
        }
        last_of_prev = map->last_base();
        last_of_prev_ref = map->last_reference_base();
    }

    if (map->last_base() < profile->sequence_length() - 1 &&
        area_number_akt == area_number) {
        *start = map->last_base() + 1;
        *end = profile->sequence_length() - 1;
        *start_ref = map->last_reference_base() + 1;
        *end_ref = profile->length() - 1;
        return 1;
    }

    return 0;
}

unsigned long ALI_SUB_SOLUTION::number_of_free_areas(void)
{
    ALI_MAP *map;
    unsigned long last_of_prev;
    unsigned long counter;

    if (map_list.is_empty())
        return 1;

    counter = 0;
    map = map_list.first();
    if (map->first_base() > 0 && map->first_reference_base() > 0)
        counter++;

    last_of_prev = map->last_base();
    while (map_list.is_next()) {
        map = map_list.next();
        if (map->first_base() > last_of_prev + 1)
            counter++;
        last_of_prev = map->last_base();
    }

    if (map->last_base() < profile->sequence_length() - 1)
        counter++;

    return counter;
}


int ALI_SUB_SOLUTION::is_konsistent(ALI_MAP *in_map)
{
    ALI_MAP *map;
    unsigned long last_of_prev, last_of_prev_ref;

    if (map_list.is_empty()) {
        if (in_map->last_base() < profile->sequence_length() &&
            in_map->last_reference_base() < profile->length())
            return 1;
        return 0;
    }

    map = map_list.first();
    if (in_map->last_base() < map->first_base() &&
        in_map->last_reference_base() < map->first_reference_base()) {
        return 1;
    }

    last_of_prev = map->last_base();
    last_of_prev_ref = map->last_reference_base();
    while (map_list.is_next()) {
        map = map_list.next();
        if (last_of_prev < in_map->first_base() &&
            in_map->last_base() < map->first_base() &&
            last_of_prev_ref < in_map->first_reference_base() &&
            in_map->last_reference_base() < map->first_reference_base()) {
            return 1;
        }
        last_of_prev = map->last_base();
        last_of_prev_ref = map->last_reference_base();
    }

    if (map->last_base() < in_map->first_base() &&
        in_map->last_base() < profile->sequence_length() &&
        map->last_reference_base() < in_map->first_base() &&
        in_map->last_base() < profile->length()) {
        return 1;
    }

    return 0;
}

int ALI_SUB_SOLUTION::insert(ALI_MAP *in_map)
{
    ALI_MAP *map;
    unsigned long last_of_prev, last_of_prev_ref;

    if (map_list.is_empty()) {
        if (in_map->last_base() < profile->sequence_length() &&
            in_map->last_reference_base() < profile->length()) {
            map_list.append_end(in_map);
            return 1;
        }
        return 0;
    }

    map = map_list.first();
    if (in_map->last_base() < map->first_base() &&
        in_map->last_reference_base() < map->first_reference_base()) {
        map_list.insert_bevor(in_map);
        return 1;
    }

    last_of_prev = map->last_base();
    last_of_prev_ref = map->last_reference_base();
    while (map_list.is_next()) {
        map = map_list.next();
        if (last_of_prev < in_map->first_base() &&
            in_map->last_base() < map->first_base() &&
            last_of_prev_ref < in_map->first_reference_base() &&
            in_map->last_reference_base() < map->first_reference_base()) {
            map_list.insert_bevor(in_map);
            return 1;
        }
        last_of_prev = map->last_base();
        last_of_prev_ref = map->last_reference_base();
    }

    if (map->last_base() < in_map->first_base() &&
        in_map->last_base() < profile->sequence_length() &&
        map->last_reference_base() < in_map->first_reference_base() &&
        in_map->last_reference_base() < profile->length()) {
        map_list.append_end(in_map);
        return 1;
    }

    return 0;
}

int ALI_SUB_SOLUTION::delete_map(ALI_MAP *del_map)
{
    ALI_MAP *map;

    if (map_list.is_empty())
        return 0;

    map = map_list.first();
    if (map == del_map) {
        map_list.delete_element();
        return 1;
    }

    while (map_list.is_next()) {
        map = map_list.next();
        if (map == del_map) {
            map_list.delete_element();
            return 1;
        }
    }

    return 0;
}

ALI_MAP *ALI_SUB_SOLUTION::make_one_map(void)
{
    ALI_MAP *map, *new_map;
    unsigned long i;
    unsigned long last_pos;
    unsigned long first_base_of_first, first_reference_of_first;
    unsigned long last_base_of_last, last_reference_of_last;

    /*
     * check if maps are closed
     */
    if (map_list.is_empty())
        return 0;

    map = map_list.first();
    first_base_of_first = map->first_base();
    first_reference_of_first = map->first_reference_base();
    last_base_of_last = map->last_base();
    last_reference_of_last = map->last_reference_base();

    while (map_list.is_next()) {
        map = map_list.next();
        if (last_base_of_last != map->first_base() - 1 ||
            last_reference_of_last != map->first_reference_base() - 1)
            ali_fatal_error("maps are not compact",
                            "ALI_SUB_SOLUTION::make_one_map()");
        last_base_of_last = map->last_base();
        last_reference_of_last = map->last_reference_base();
    }

    new_map = new ALI_MAP(first_base_of_first,last_base_of_last,
                          first_reference_of_first,last_reference_of_last);

    map = map_list.first();
    do {
        last_pos = 0;
        for (i = map->first_base(); i <= map->last_base(); i++) {
            if (map->is_undefined(i))
                ali_fatal_error("Unexpected value",
                                "ALI_SUB_SOLUTION::make_one_map()");
            if ((unsigned long)(map->position(i)) < last_pos)
                ali_fatal_error("Inconsistent positions",
                                "ALI_SUB_SOLUTION::make_one_map()");
            last_pos = map->position(i);

            if (map->is_inserted(i))
                new_map->set(i,map->first_reference_base() +
                             map->position(i) - first_reference_of_first,1);
            else
                new_map->set(i,map->first_reference_base() +
                             map->position(i) - first_reference_of_first,0);
        }

        if (map_list.is_next())
            map = map_list.next();
        else
            map = 0;
    } while (map != 0);

    return new_map;
}

void ALI_SUB_SOLUTION::print(void)
{
    ALI_MAP *map;

    printf("ALI_SUB_SOLUTION:\n");
    if (!map_list.is_empty()) {
        map = map_list.first();
        printf("(%ld to %ld) -> (%ld to %ld)\n",
               map->first_base(), map->last_base(),
               map->first_reference_base(), map->last_reference_base());
        while (map_list.is_next()) {
            map = map_list.next();
            printf("(%ld to %ld) -> (%ld to %ld)\n",
                   map->first_base(), map->last_base(),
                   map->first_reference_base(), map->last_reference_base());
        }
    }
}








