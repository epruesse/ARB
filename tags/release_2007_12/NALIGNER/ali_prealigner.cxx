#include <stdlib.h>
#include "ali_prealigner.hxx"
#include "ali_aligner.hxx"

unsigned long   random_stat[6] = {0, 0, 0, 0, 0, 0};

/*****************************************************************************
 *
 * structure ali_prealigner_mask
 *
 *****************************************************************************/

/*
 * Insert a new map
 */
void            ali_prealigner_mask::
insert(ALI_MAP * in_map, float costs)
{
    unsigned long   i;

    calls++;
    if (map == 0) {
        map = in_map;
        cost_of_binding = costs;
    } else {
        if (costs > cost_of_binding) {
            delete          in_map;
            return;
        }
        if (map->first_base() != in_map->first_base() ||
            map->last_base() != in_map->last_base() ||
            map->first_reference_base() != in_map->first_reference_base() ||
            map->last_reference_base() != in_map->last_reference_base()) {
            ali_fatal_error("Incompatible maps",
                            "ali_prealigner_mask::insert()");
        }
        if (costs < cost_of_binding) {
            delete          map;
            map = in_map;
            cost_of_binding = costs;
            last_new = calls;
            last_joins = 0;
            return;
        }
        joins++;
        last_joins++;
        for (i = map->first_base(); i <= map->last_base(); i++)
            if ((!map->is_undefined(i)) && map->position(i) != in_map->position(i))
                map->undefine(i);
        delete          in_map;
    }
}

/*
 * Delete expensive parts of solution
 */
void            ali_prealigner_mask::
delete_expensive(ALI_PREALIGNER_CONTEXT * context,
                 ALI_PROFILE * profile)
{
    ALI_MAP        *inverse_map;
    unsigned long   start_hel, end_hel;
    unsigned long   start_seq, end_seq;
    unsigned long   start_mapped, end_mapped;
    unsigned long   start_ok=0, end_ok=0;
    int             start_ok_flag;
    unsigned long   found_helix;
    unsigned long   error_counter;

    unsigned long   map_pos, i, j;
    float           max_cost, helix_cost;
    unsigned long   helix_counter;
    long            compl_pos;
    unsigned char   base1, base2;

    printf("MASK : calls = %ld  joins = %ld last_new = %ld last_joins = %ld\n",
           calls, joins, last_new, last_joins);

    max_cost = profile->w_sub_maximum() * context->max_cost_of_sub_percent;

    /*
     * Delete expensive Bases
     */
    error_counter = 0;
    for (i = map->first_base(); i <= map->last_base(); i++) {
        if (!(map->is_inserted(i)) &&
            profile->w_sub(map->position(i), i) > max_cost) {
            error_counter++;
            if (error_counter > context->error_count)
                map->undefine(i);
            else {
                if (error_counter == context->error_count) {
                    for (j = i - error_counter + 1; j <= i; j++)
                        map->undefine(j);
                }
            }
        } else {
            /*
             * If error was in helix => delete helix total
             */
            if (error_counter > 0 &&
                profile->is_in_helix(map->position(i - 1), &start_hel, &end_hel)) {
                for (j = i - 1; map->position(j) >= long(start_hel); j--);
                for (; map->position(j) <= long(end_hel); j++)
                    map->undefine(j);
            }
            error_counter = 0;
        }
    }

    /*
     * Delete expensive Helizes
     */
    inverse_map = map->inverse_without_inserts();
    for (i = inverse_map->first_base(); i <= inverse_map->last_base(); i++) {
        /*
         * found a helix
         */
        if (profile->is_in_helix(i, &start_hel, &end_hel)) {
            if (i != start_hel)
                ali_fatal_error("Inconsistent positions",
                                "ali_prealigner_mask::delete_expensive()");
            compl_pos = profile->complement_position(start_hel);
            /*
             * only forward bindings
             */
            if (compl_pos > long(end_hel)) {
                helix_cost = 0.0;
                helix_counter = 0;
                while (i <= end_hel) {
                    /*
                     * is binding ?
                     */
                    if (compl_pos > 0) {
                        if (!inverse_map->is_undefined(i)) {
                            base1 = (profile->sequence())->base(
                                                                inverse_map->first_reference_base() +
                                                                inverse_map->position(i));
                        } else {
                            base1 = ALI_GAP_CODE;
                        }
                        if (!inverse_map->is_undefined(compl_pos)) {
                            base2 = (profile->sequence())->base(
                                                                inverse_map->first_reference_base() +
                                                                inverse_map->position(compl_pos));
                        } else {
                            base2 = ALI_GAP_CODE;
                        }
                        if (base1 != ALI_GAP_CODE || base2 != ALI_GAP_CODE) {
                            helix_cost += profile->w_bind(i, base1, compl_pos, base2);
                            helix_counter++;
                        }
                    }
                    i++;
                    compl_pos = profile->complement_position(i);
                }
                if (helix_counter > 0) helix_cost /= helix_counter;
                if (helix_cost > context->max_cost_of_helix) {
                    for (j = start_hel; j <= end_hel; j++) {
                        if (!inverse_map->is_undefined(j)) {
                            map->undefine(inverse_map->first_reference_base() + inverse_map->position(j));
                        }
                    }
                    for (j = profile->complement_position(end_hel); long(j) <= profile->complement_position(start_hel); j++) {
                        if (!inverse_map->is_undefined(j)) {
                            map->undefine(inverse_map->first_reference_base() + inverse_map->position(j));
                        }
                    }
                }
            }
            i = end_hel;
        }
    }
    delete          inverse_map;

    /*
     * Check for good parts
     */
    for (map_pos = map->first_base(); map_pos <= map->last_base(); map_pos++) {
        /*
         * search next defined segment
         */
        if (!map->is_undefined(map_pos)) {
            /*
             * find start and end of segment
             */
            start_seq = map_pos;
            start_mapped = map->position(map_pos);
            for (map_pos++; map_pos <= map->last_base() &&
                     (!map->is_undefined(map_pos)); map_pos++);
            end_seq = map_pos - 1;
            end_mapped = map->position(end_seq);

            /*
             * Check segment for helizes
             */
            found_helix = 0;
            start_ok_flag = 0;
            for (i = start_seq; i <= end_seq; i++) {
                if (profile->is_in_helix(map->position(i), &start_hel, &end_hel)) {
                    found_helix++;
                    /*
                     * Helix is inside the segment
                     */
                    if (start_hel >= start_mapped && end_hel <= end_mapped) {
                        if (start_ok_flag == 0) {
                            start_ok = start_hel;
                            start_ok_flag = 1;
                        }
                        end_ok = end_hel;
                    }
                }
            }

            /*
             * Found good helizes
             */
            if (start_ok_flag == 1) {
                for (i = start_seq; map->position(i) < long(start_ok); i++) map->undefine(i);
                for (i = end_seq; map->position(i) > long(end_ok); i--) map->undefine(i);
            } else {
                /*
                 * Found bad helizes
                 */
                if (found_helix > 0) {
                    for (i = start_seq; i <= end_seq; i++)
                        map->undefine(i);
                }
                /*
                 * Segment without helix
                 */
                else {
                    if (end_seq - start_seq + 1 >= (unsigned long)((2 * context->intervall_border) + context->intervall_center)) {
                        for (i = start_seq; i < start_seq + context->intervall_border; i++) map->undefine(i);
                        for (i = end_seq; i > end_seq - context->intervall_border; i--) map->undefine(i);
                    } else {
                        for (i = start_seq; i <= end_seq; i++) map->undefine(i);
                    }
                }
            }
        }
    }
}


/*****************************************************************************
 *
 * class ALI_PREALIGNER  (privat)
 *
 *****************************************************************************/

GB_INLINE float    ALI_PREALIGNER::
minimum2(float a, float b)
{
    return ((a < b) ? a : b);
}

GB_INLINE float    ALI_PREALIGNER::
minimum3(float a, float b, float c)
{
    return ((a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c));
}


GB_INLINE void     ALI_PREALIGNER::
calculate_first_column_first_cell(
                                  ali_prealigner_cell * akt_cell)
{
    float           v1, v2, v3;

    /***
        v1 = profile->w_ins(start_x,start_y) + profile->w_del(start_y,start_y);
    ***/
    v1 = profile->w_ins_multi_cheap(start_x, start_y) +
        profile->w_sub_multi_gap_cheap(start_y, start_y);
    v2 = profile->w_sub(start_y, start_x);
    v3 = v1;

    akt_cell->d = minimum2(v1, v2);

    if (akt_cell->d == v1)
        path_map->set(0, 0, ALI_UP | ALI_LEFT);
    if (akt_cell->d == v2)
        path_map->set(0, 0, ALI_DIAG);
}

GB_INLINE void     ALI_PREALIGNER::
calculate_first_column_cell(
                            ali_prealigner_cell * up_cell, ali_prealigner_cell * akt_cell,
                            unsigned long pos_y)
{
    float           v1, v2, v3;
    unsigned long   positiony;

    positiony = start_y + pos_y;

    /***
        v1 = up_cell->d + profile->w_del(positiony,positiony);
        v2 = profile->w_del_multi_unweighted(start_y, positiony - 1) +
        profile->w_sub(positiony, start_x);
        v3 = profile->w_del_multi_unweighted(start_y, positiony) +
        profile->w_ins(start_x,positiony);
    ***/
    v1 = up_cell->d + profile->w_sub_gap(positiony);
    v2 = profile->w_sub_multi_gap_cheap(start_y, positiony - 1) +
        profile->w_sub(positiony, start_x);
    v3 = profile->w_sub_multi_gap_cheap(start_y, positiony) +
        profile->w_ins(start_x, positiony);

    akt_cell->d = minimum3(v1, v2, v3);

    if (v1 == akt_cell->d)
        path_map->set(0, pos_y, ALI_UP);
    if (v2 == akt_cell->d)
        path_map->set(0, pos_y, ALI_DIAG);
    if (v3 == akt_cell->d)
        path_map->set(0, pos_y, ALI_LEFT);
}

void            ALI_PREALIGNER::
calculate_first_column(ali_prealigner_column * akt_column)
{
    unsigned long   pos_y;

    calculate_first_column_first_cell(&(*akt_column->cells)[0]);

    for (pos_y = 1; pos_y < akt_column->column_length; pos_y++)
        calculate_first_column_cell(&(*akt_column->cells)[pos_y - 1],
                                    &(*akt_column->cells)[pos_y],
                                    pos_y);
}


GB_INLINE void     ALI_PREALIGNER::
calculate_first_cell(
                     ali_prealigner_cell * left_cell, ali_prealigner_cell * akt_cell,
                     unsigned long pos_x)
{
    float           v1, v2, v3;
    unsigned long   positionx;

    positionx = start_x + pos_x;

    /***
        v1 = profile->w_ins_multi_unweighted(start_x, positionx) +
        profile->w_del(start_y,start_y);
        v2 = profile->w_ins_multi_unweighted(start_x, positionx - 1) +
        profile->w_sub(start_y, positionx);
    ***/
    v1 = profile->w_ins_multi_cheap(start_x, positionx) +
        profile->w_sub_gap(start_y);
    v2 = profile->w_ins_multi_cheap(start_x, positionx - 1) +
        profile->w_sub(start_y, positionx);
    v3 = left_cell->d + profile->w_ins(positionx, start_y);

    akt_cell->d = minimum3(v1, v2, v3);

    if (v1 == akt_cell->d)
        path_map->set(pos_x, 0, ALI_UP);
    if (v2 == akt_cell->d)
        path_map->set(pos_x, 0, ALI_DIAG);
    if (v3 == akt_cell->d)
        path_map->set(pos_x, 0, ALI_LEFT);
}

GB_INLINE void     ALI_PREALIGNER::
calculate_cell(
               ali_prealigner_cell * diag_cell, ali_prealigner_cell * left_cell,
               ali_prealigner_cell * up_cell, ali_prealigner_cell * akt_cell,
               unsigned long pos_x, unsigned long pos_y)
{
    float           v1, v2, v3;
    unsigned long   positionx, positiony;

    positionx = start_x + pos_x;
    positiony = start_y + pos_y;

    /***
        v1 = up_cell->d + profile->w_del(positiony,positiony);
    ***/
    v1 = up_cell->d + profile->w_sub_gap(positiony);
    v2 = diag_cell->d + profile->w_sub(positiony, positionx);
    v3 = left_cell->d + profile->w_ins(positionx, positiony);

    akt_cell->d = minimum3(v1, v2, v3);

    if (v1 == akt_cell->d)
        path_map->set(pos_x, pos_y, ALI_UP);
    if (v2 == akt_cell->d)
        path_map->set(pos_x, pos_y, ALI_DIAG);
    if (v3 == akt_cell->d)
        path_map->set(pos_x, pos_y, ALI_LEFT);
}

void            ALI_PREALIGNER::
calculate_column(
                 ali_prealigner_column * prev_col, ali_prealigner_column * akt_col,
                 unsigned long pos_x)
{
    unsigned long   pos_y;

    calculate_first_cell(&(*prev_col->cells)[0], &(*akt_col->cells)[0], pos_x);

    for (pos_y = 1; pos_y < akt_col->column_length; pos_y++)
        calculate_cell(&(*prev_col->cells)[pos_y - 1], &(*prev_col->cells)[pos_y],
                       &(*akt_col->cells)[pos_y - 1], &(*akt_col->cells)[pos_y],
                       pos_x, pos_y);
}

GB_INLINE void     ALI_PREALIGNER::
calculate_last_column_first_cell(
                                 ali_prealigner_cell * left_cell, ali_prealigner_cell * akt_cell,
                                 unsigned long pos_x)
{
    float           v1, v2, v3;
    unsigned long   positionx;

    positionx = start_x + pos_x;

    /***
        v1 = profile->w_ins_multi_unweighted(start_x, positionx) +
        profile->w_del(start_y,start_y);
        v2 = profile->w_ins_multi_unweighted(start_x, positionx - 1) +
        profile->w_sub(start_y, positionx);
    ***/
    v1 = profile->w_ins_multi_cheap(start_x, positionx) +
        profile->w_sub_gap_cheap(start_y);
    v2 = profile->w_ins_multi_cheap(start_x, positionx - 1) +
        profile->w_sub(start_y, positionx);
    v3 = left_cell->d + profile->w_ins(positionx, start_y);

    akt_cell->d = minimum3(v1, v2, v3);

    if (v1 == akt_cell->d)
        path_map->set(pos_x, 0, ALI_UP);
    if (v2 == akt_cell->d)
        path_map->set(pos_x, 0, ALI_DIAG);
    if (v3 == akt_cell->d)
        path_map->set(pos_x, 0, ALI_LEFT);
}

GB_INLINE void     ALI_PREALIGNER::
calculate_last_column_cell(
                           ali_prealigner_cell * diag_cell, ali_prealigner_cell * left_cell,
                           ali_prealigner_cell * up_cell, ali_prealigner_cell * akt_cell,
                           unsigned long pos_x, unsigned long pos_y)
{
    float           v1, v2, v3;
    unsigned long   positionx, positiony;

    positionx = start_x + pos_x;
    positiony = start_y + pos_y;

    /***
        v1 = up_cell->d + profile->w_del(positiony,positiony);
    ***/
    v1 = up_cell->d + profile->w_sub_gap_cheap(positiony);
    v2 = diag_cell->d + profile->w_sub(positiony, positionx);
    v3 = left_cell->d + profile->w_ins(positionx, positiony);

    akt_cell->d = minimum3(v1, v2, v3);

    if (v1 == akt_cell->d)
        path_map->set(pos_x, pos_y, ALI_UP);
    if (v2 == akt_cell->d)
        path_map->set(pos_x, pos_y, ALI_DIAG);
    if (v3 == akt_cell->d)
        path_map->set(pos_x, pos_y, ALI_LEFT);
}

void            ALI_PREALIGNER::
calculate_last_column(
                      ali_prealigner_column * prev_col, ali_prealigner_column * akt_col,
                      unsigned long pos_x)
{
    unsigned long   pos_y;

    calculate_last_column_first_cell(&(*prev_col->cells)[0],
                                     &(*akt_col->cells)[0], pos_x);

    for (pos_y = 1; pos_y < akt_col->column_length; pos_y++)
        calculate_last_column_cell(&(*prev_col->cells)[pos_y - 1],
                                   &(*prev_col->cells)[pos_y],
                                   &(*akt_col->cells)[pos_y - 1], &(*akt_col->cells)[pos_y],
                                   pos_x, pos_y);
}


void            ALI_PREALIGNER::
calculate_matrix(void)
{
    unsigned long   pos_x;
    ali_prealigner_column *akt_col, *prev_col, *h_col;

    akt_col = new ali_prealigner_column(end_y - start_y + 1);
    prev_col = new ali_prealigner_column(end_y - start_y + 1);

    calculate_first_column(prev_col);

    for (pos_x = 1; pos_x < end_x - start_x + 1; pos_x++) {
        if (pos_x == end_x - start_x || profile->is_internal_last(pos_x))
            calculate_last_column(prev_col, akt_col, pos_x);
        else
            calculate_column(prev_col, akt_col, pos_x);
        h_col = akt_col;
        akt_col = prev_col;
        prev_col = h_col;
    }

    delete          akt_col;
    delete          prev_col;
}


#if 0

int             ALI_PREALIGNER::
find_multiple_path(
                   unsigned long start_x, unsigned long start_y,
                   unsigned long *end_x, unsigned long *end_y)
{
    unsigned char   value;
    long            x1, y1, x2, y2;

    value = path_map->get_value(start_x, start_y);

    /*
     * (x1,y1) is moving left, diag, up
     */
    if (value & ALI_LEFT) {
        x1 = start_x - 1;
        y1 = start_y;
    } else {
        if (value & ALI_DIAG) {
            x1 = start_x - 1;
            y1 = start_y - 1;
        } else {
            *end_x = start_x;
            *end_y = start_y - 1;
            return 1;
        }
    }

    /*
     * (x2,y2) is moving up, diag, left
     */
    if (value & ALI_UP) {
        x2 = start_x;
        y2 = start_y - 1;
    } else {
        if (value & ALI_DIAG) {
            x2 = start_x - 1;
            y2 = start_y - 1;
        } else {
            *end_x = start_x - 1;
            *end_y = start_y;
            return 1;
        }
    }

    while (x1 > 0 && x2 > 0 && y1 > 0 && y2 > 0 &&
           (x1 != x2 || y1 != y2)) {
        /*
         * move (x2,y2)
         */
        if (y2 < y1) {
            value = path_map->get_value((unsigned long) x1, (unsigned long) y1);
            if (value & ALI_LEFT) {
                x1--;
            } else {
                if (value & ALI_DIAG) {
                    x1--;
                    y1--;
                } else {
                    y1--;
                }
            }
        }
        /*
         * move (x1,y2)
         */
        else {
            value = path_map->get_value((unsigned long) x2, (unsigned long) y2);
            if (value & ALI_UP) {
                y2--;
            } else {
                if (value & ALI_DIAG) {
                    x2--;
                    y2--;
                } else {
                    x2--;
                }
            }
        }
    }

    if (x1 == x2 && y1 == y2) {
        *end_x = x1;
        *end_y = y1;
        return 1;
    }
    return 0;
}
#endif


/*
 * Generate a sub_solution by deleting all undefined segments
 */
void            ALI_PREALIGNER::
generate_solution(ALI_MAP * map)
{
    ALI_MAP        *seg_map;
    unsigned long   map_pos, map_len;
    unsigned long   start_seg, end_seg, pos_seg;

    sub_solution = new ALI_SUB_SOLUTION(profile);

    map_len = map->last_base() - map->first_base() + 1;
    for (map_pos = map->first_base(); map_pos <= map->last_base(); map_pos++) {
        /*
         * search for segment
         */
        for (start_seg = map_pos; start_seg <= map->last_base() &&
                 map->is_undefined(start_seg); start_seg++);
        if (start_seg <= map->last_base()) {
            for (end_seg = start_seg; end_seg <= map->last_base() &&
                     (!map->is_undefined(end_seg)); end_seg++);
            end_seg--;

            seg_map = new ALI_MAP(start_seg,
                                  end_seg,
                                  map->position(start_seg),
                                  map->position(end_seg));
            /*
             * Copy segment
             */
            for (pos_seg = start_seg; pos_seg <= end_seg; pos_seg++) {
                if (map->is_inserted(pos_seg))
                    seg_map->set(pos_seg,
                                 map->position(pos_seg) - map->position(start_seg), 1);
                else
                    seg_map->set(pos_seg,
                                 map->position(pos_seg) - map->position(start_seg), 0);
            }

            if (sub_solution->insert(seg_map) != 1)
                ali_fatal_error("Inconsistent solution?",
                                "ALI_PREALIGNER::generate_solution()");

            map_pos = end_seg;
        }
    }
}

/*
 * generate the result mask from an stack of operations
 */
void            ALI_PREALIGNER::
generate_result_mask(ALI_TSTACK < unsigned char >*stack)
{
    ALI_SEQUENCE   *sequence;
    float           cost_of_bindings;
    ALI_MAP        *map;
    unsigned long   seq_pos, dest_pos;
    long            i;

    map = new ALI_MAP(start_x, end_x, start_y, end_y);

    seq_pos = start_x;
    dest_pos = 0;
    for (i = (long) stack->akt_size() - 1; i >= 0; i--) {
        switch (stack->get(i)) {
            case ALI_PREALIGNER_INS:
                map->set(seq_pos++, dest_pos, 1);
                break;
            case ALI_PREALIGNER_SUB:
                map->set(seq_pos++, dest_pos++, 0);
                break;
            case ALI_PREALIGNER_DEL:
                dest_pos++;
                break;
            case ALI_PREALIGNER_INS | ALI_PREALIGNER_MULTI_FLAG:
                map->set(seq_pos, dest_pos, 1);
                map->undefine(seq_pos++);
                break;
            case ALI_PREALIGNER_SUB | ALI_PREALIGNER_MULTI_FLAG:
                map->set(seq_pos, dest_pos++, 0);
                map->undefine(seq_pos++);
                break;
            case ALI_PREALIGNER_DEL | ALI_PREALIGNER_MULTI_FLAG:
                dest_pos++;
                break;
            default:
                ali_fatal_error("Unexpected value",
                                "ALI_PREALIGNER::generate_result_mask()");
        }
    }

    if (result_mask_counter > 0)
        result_mask_counter--;

    sequence = map->sequence_without_inserts(profile->sequence());
    cost_of_bindings = profile->w_binding(map->first_reference_base(), sequence);
    delete          sequence;

    /*
     * make the intersection
     */
    result_mask.insert(map, cost_of_bindings);
}

/*
 * Fill the stack with rest DELs or INSs
 */
void            ALI_PREALIGNER::
mapper_post(ALI_TSTACK < unsigned char >*stack,
            unsigned long ins_nu, unsigned long del_nu)
{
    if (ins_nu > 0 && del_nu > 0)
        ali_fatal_error("Unexpected values",
                        "ALI_PREALIGNER::mapper_post()");

    if (ins_nu > 0) {
        stack->push(ALI_PREALIGNER_INS, ins_nu);
        generate_result_mask(stack);
        stack->pop(ins_nu);
    } else {
        if (del_nu > 0) {
            stack->push(ALI_PREALIGNER_DEL, del_nu);
            generate_result_mask(stack);
            stack->pop(del_nu);
        } else
            generate_result_mask(stack);
    }
}

/*
 * Fill the stack with rest DELs or INSs (with MULTI_FLAG)
 */
void            ALI_PREALIGNER::
mapper_post_multi(ALI_TSTACK < unsigned char >*stack,
                  unsigned long ins_nu, unsigned long del_nu)
{
    if (ins_nu > 0 && del_nu > 0)
        ali_fatal_error("Unexpected values",
                        "ALI_PREALIGNER::mapper_post_multi()");

    if (ins_nu > 0) {
        stack->push(ALI_PREALIGNER_INS | ALI_PREALIGNER_MULTI_FLAG, ins_nu);
        generate_result_mask(stack);
        stack->pop(ins_nu);
    } else {
        if (del_nu > 0) {
            stack->push(ALI_PREALIGNER_DEL | ALI_PREALIGNER_MULTI_FLAG, del_nu);
            generate_result_mask(stack);
            stack->pop(del_nu);
        } else
            generate_result_mask(stack);
    }
}

/*
 * generate a stack of operations by taking a random path of the pathmap
 */
void            ALI_PREALIGNER::
mapper_random(ALI_TSTACK < unsigned char >*stack,
              unsigned long pos_x, unsigned long pos_y)
{
    unsigned long   next_x, next_y;
    unsigned long   random;
    unsigned char   value;
    unsigned long   stack_counter = 0;



    next_x = pos_x;
    next_y = pos_y;
    while (next_x <= pos_x && next_y <= pos_y) {
        stack_counter++;

        random = (unsigned long) (rand() >> 4) % 6;

        value = path_map->get_value(next_x, next_y);
        if (value == 0)
            ali_fatal_error("Unexpected value (1)",
                            "ALI_PREALIGNER::mapper_random()");

        switch (random) {
            case 0:
                if (value & ALI_UP) {
                    stack->push(ALI_PREALIGNER_DEL);
                    next_y--;
                } else {
                    if (value & ALI_DIAG) {
                        stack->push(ALI_PREALIGNER_SUB);
                        next_x--;
                        next_y--;
                    } else {
                        stack->push(ALI_PREALIGNER_INS);
                        next_x--;
                    }
                }
                break;
            case 1:
                if (value & ALI_UP) {
                    stack->push(ALI_PREALIGNER_DEL);
                    next_y--;
                } else {
                    if (value & ALI_LEFT) {
                        stack->push(ALI_PREALIGNER_INS);
                        next_x--;
                    } else {
                        stack->push(ALI_PREALIGNER_SUB);
                        next_x--;
                        next_y--;
                    }
                }
                break;
            case 2:
                if (value & ALI_DIAG) {
                    stack->push(ALI_PREALIGNER_SUB);
                    next_x--;
                    next_y--;
                } else {
                    if (value & ALI_UP) {
                        stack->push(ALI_PREALIGNER_DEL);
                        next_y--;
                    } else {
                        stack->push(ALI_PREALIGNER_INS);
                        next_x--;
                    }
                }
                break;
            case 3:
                if (value & ALI_DIAG) {
                    stack->push(ALI_PREALIGNER_SUB);
                    next_x--;
                    next_y--;
                } else {
                    if (value & ALI_LEFT) {
                        stack->push(ALI_PREALIGNER_INS);
                        next_x--;
                    } else {
                        stack->push(ALI_PREALIGNER_DEL);
                        next_y--;
                    }
                }
                break;
            case 4:
                if (value & ALI_LEFT) {
                    stack->push(ALI_PREALIGNER_INS);
                    next_x--;
                } else {
                    if (value & ALI_UP) {
                        stack->push(ALI_PREALIGNER_DEL);
                        next_y--;
                    } else {
                        stack->push(ALI_PREALIGNER_SUB);
                        next_x--;
                        next_y--;
                    }
                }
                break;
            case 5:
                if (value & ALI_LEFT) {
                    stack->push(ALI_PREALIGNER_INS);
                    next_x--;
                } else {
                    if (value & ALI_DIAG) {
                        stack->push(ALI_PREALIGNER_SUB);
                        next_x--;
                        next_y--;
                    } else {
                        stack->push(ALI_PREALIGNER_DEL);
                        next_y--;
                    }
                }
                break;
            default:
                ali_fatal_error("Unexpected random value",
                                "ALI_PREALIGNER::mapper_random()");
        }
    }

    if (next_x <= pos_x) {
        mapper_post(stack, next_x + 1, 0);
    } else {
        if (next_y <= pos_y) {
            mapper_post(stack, 0, next_y + 1);
        } else {
            mapper_post(stack, 0, 0);
        }
    }

    if (stack_counter > 0)
        stack->pop(stack_counter);
}

/*
 * generate a stack of operations by taking every path
 */
void            ALI_PREALIGNER::
mapper(ALI_TSTACK < unsigned char >*stack,
       unsigned long pos_x, unsigned long pos_y)
{
    unsigned char   value;
    unsigned long   stack_counter = 0;

    value = path_map->get_value(pos_x, pos_y);

    if (pos_x == 0 || pos_y == 0) {
        if (value & ALI_UP) {
            stack->push(ALI_PREALIGNER_DEL);
            if (pos_y == 0)
                mapper_post(stack, pos_x + 1, 0);
            else
                mapper(stack, pos_x, pos_y - 1);
            stack->pop();
        }
        if (value & ALI_DIAG) {
            stack->push(ALI_PREALIGNER_SUB);
            if (pos_y > 0) {
                mapper_post(stack, 0, pos_y);
            } else {
                if (pos_x > 0) {
                    mapper_post(stack, pos_x, 0);
                } else {
                    mapper_post(stack, 0, 0);
                }
            }
            stack->pop();
        }
        if (value & ALI_LEFT) {
            stack->push(ALI_PREALIGNER_INS);
            if (pos_x == 0)
                mapper_post(stack, 0, pos_y + 1);
            else
                mapper(stack, pos_x - 1, pos_y);
            stack->pop();
        }
        return;
    }
    /*
     * follow an unique path
     */
    while (value == ALI_UP || value == ALI_DIAG || value == ALI_LEFT) {
        stack_counter++;
        switch (value) {
            case ALI_UP:
                stack->push(ALI_PREALIGNER_DEL);
                pos_y--;
                break;
            case ALI_DIAG:
                stack->push(ALI_PREALIGNER_SUB);
                pos_x--;
                pos_y--;
                break;
            case ALI_LEFT:
                stack->push(ALI_PREALIGNER_INS);
                pos_x--;
                break;
        }

        value = path_map->get_value(pos_x, pos_y);

        if (pos_x == 0 || pos_y == 0) {
            if (value & ALI_UP) {
                stack->push(ALI_PREALIGNER_DEL);
                if (pos_y == 0)
                    mapper_post(stack, pos_x + 1, 0);
                else
                    mapper(stack, pos_x, pos_y - 1);
                stack->pop();
            }
            if (value & ALI_DIAG) {
                stack->push(ALI_PREALIGNER_SUB);
                if (pos_y > 0) {
                    mapper_post(stack, 0, pos_y);
                } else {
                    if (pos_x > 0) {
                        mapper_post(stack, pos_x, 0);
                    } else {
                        mapper_post(stack, 0, 0);
                    }
                }
                stack->pop();
            }
            if (value & ALI_LEFT) {
                stack->push(ALI_PREALIGNER_INS);
                if (pos_x == 0)
                    mapper_post(stack, 0, pos_y + 1);
                else
                    mapper(stack, pos_x - 1, pos_y);
                stack->pop();
            }
            if (stack_counter > 0)
                stack->pop(stack_counter);

            return;
        }
    }

    if (value & ALI_UP) {
        stack->push(ALI_PREALIGNER_DEL);
        mapper(stack, pos_x, pos_y - 1);
        stack->pop();
    }
    if (value & ALI_DIAG) {
        stack->push(ALI_PREALIGNER_SUB);
        mapper(stack, pos_x - 1, pos_y - 1);
        stack->pop();
    }
    if (value & ALI_LEFT) {
        stack->push(ALI_PREALIGNER_INS);
        mapper(stack, pos_x - 1, pos_y);
        stack->pop();
    }
    if (stack_counter > 0)
        stack->pop(stack_counter);
}

#if 0

void            ALI_PREALIGNER::
quick_mapper(ALI_TSTACK < unsigned char >*stack,
             unsigned long pos_x, unsigned long pos_y)
{
    unsigned long   end_x, end_y;
    unsigned char   value;


    value = path_map->get_value(pos_x, pos_y);

    while (pos_x > 0 && pos_y > 0) {
        if (value == ALI_UP || value == ALI_DIAG || value == ALI_LEFT) {
            switch (value) {
                case ALI_UP:
                    stack->push(ALI_PREALIGNER_DEL);
                    pos_y--;
                    break;
                case ALI_DIAG:
                    stack->push(ALI_PREALIGNER_SUB);
                    pos_x--;
                    pos_y--;
                    break;
                case ALI_LEFT:
                    stack->push(ALI_PREALIGNER_INS);
                    pos_x--;
                    break;
            }
            value = path_map->get_value(pos_x, pos_y);
        } else {
            if (find_multiple_path(pos_x, pos_y, &end_x, &end_y) == 0)
                end_x = end_y = 0;
            while (pos_x != end_x || pos_y != end_y) {
                if (value & ALI_UP) {
                    stack->push(ALI_PREALIGNER_DEL | ALI_PREALIGNER_MULTI_FLAG);
                    pos_y--;
                } else {
                    if (value & ALI_DIAG) {
                        stack->push(ALI_PREALIGNER_SUB | ALI_PREALIGNER_MULTI_FLAG);
                        pos_x--;
                        pos_y--;
                    } else {
                        stack->push(ALI_PREALIGNER_INS | ALI_PREALIGNER_MULTI_FLAG);
                        pos_x--;
                    }
                }
                value = path_map->get_value(pos_x, pos_y);
            }
        }
    }

    if (pos_x == 0) {
        while (pos_y > 0 && value == ALI_UP) {
            stack->push(ALI_PREALIGNER_DEL);
            pos_y--;
            value = path_map->get_value(pos_x, pos_y);
        }
        switch (value) {
            case ALI_UP:
                stack->push(ALI_PREALIGNER_DEL);
                mapper_post(stack, pos_x, pos_y);
                break;
            case ALI_DIAG:
                stack->push(ALI_PREALIGNER_SUB);
                mapper_post(stack, pos_x, pos_y);
                break;
            case ALI_LEFT:
                stack->push(ALI_PREALIGNER_INS);
                mapper_post(stack, pos_x, pos_y);
                break;
            default:
                stack->push(ALI_PREALIGNER_SUB | ALI_PREALIGNER_MULTI_FLAG);
                mapper_post_multi(stack, pos_x, pos_y);
        }
    } else {
        while (pos_x > 0 && value == ALI_LEFT) {
            stack->push(ALI_PREALIGNER_INS);
            pos_x--;
            value = path_map->get_value(pos_x, pos_y);
        }
        switch (value) {
            case ALI_UP:
                stack->push(ALI_PREALIGNER_DEL);
                mapper_post(stack, pos_x, pos_y);
                break;
            case ALI_DIAG:
                stack->push(ALI_PREALIGNER_SUB);
                mapper_post(stack, pos_x, pos_y);
                break;
            case ALI_LEFT:
                stack->push(ALI_PREALIGNER_INS);
                mapper_post(stack, pos_x, pos_y);
                break;
            default:
                stack->push(ALI_PREALIGNER_SUB | ALI_PREALIGNER_MULTI_FLAG);
                mapper_post_multi(stack, pos_x, pos_y);
        }
    }
}

#endif

/*
 * make the result map from the path matrix
 */
void            ALI_PREALIGNER::
make_map(void)
{
    unsigned long   number_of_sol;
    ALI_TSTACK < unsigned char >*stack;

    stack = new ALI_TSTACK < unsigned char >(end_x - start_x + end_y - start_y + 3);
    if (stack == 0)
        ali_fatal_error("Out of memory");

    srand((unsigned int) (GB_time_of_day() % 1000));

    number_of_sol = number_of_solutions();
    printf("%lu solutions generated\n", number_of_sol);

    if (number_of_sol == 0 || number_of_sol > result_mask_counter) {
        ali_message("Starting random mapping");
        do {
            mapper_random(stack, end_x - start_x, end_y - start_y);
        } while (result_mask_counter > 0);
    } else {
        ali_message("Starting systematic mapping");
        mapper(stack, end_x - start_x, end_y - start_y);
    }

    delete          stack;

    ali_message("Mapping finished");
}

#if 0

void            ALI_PREALIGNER::
make_quick_map(void)
{
    ALI_TSTACK < unsigned char >*stack;

    ali_message("Starting quick mapping");

    stack = new ALI_TSTACK < unsigned char >(end_x - start_x + end_y - start_y + 3);
    if (stack == 0)
        ali_fatal_error("Out of memory");

    quick_mapper(stack, end_x - start_x, end_y - start_y);

    delete          stack;

    ali_message("Quick mapping finished");
}

#endif

/*
 * generate an approximation of a complete solution
 */
void            ALI_PREALIGNER::
generate_approximation(ALI_SUB_SOLUTION * work_sol)
{
    ALI_MAP        *map;
    ALI_SEQUENCE   *sequence;
    char           *ins_marker;
    float           binding_costs;

    map = work_sol->make_one_map();
    if (map == 0)
        ali_fatal_error("Can't make one map",
                        "ALI_PREALIGNER::generate_approximation()");

    sequence = map->sequence_without_inserts(profile->sequence());
    binding_costs = profile->w_binding(map->first_base(), sequence);
    delete          sequence;

    ins_marker = map->insert_marker();

    result_approx.insert(map, ins_marker, binding_costs);
    // delete ins_marker;   @@@
    // delete map;          @@@
}

/*
 * combine subsolutions for an approximation
 */
void            ALI_PREALIGNER::
mapper_approximation(unsigned long area_no,
                     ALI_TARRAY < ALI_TLIST < ALI_MAP * >*>*map_lists,
                     ALI_SUB_SOLUTION * work_sol)
{
    ALI_TLIST < ALI_MAP * >*map_list;
    ALI_MAP        *map;

    /*
     * stop mapping at last area
     */
    if (area_no > map_lists->size())
        return;

    if (area_no == map_lists->size()) {
        generate_approximation(work_sol);
        return;
    }
    /*
     * map area number 'area_no'
     */
    map_list = map_lists->get(area_no);
    if (map_list->is_empty())
        ali_fatal_error("Found empty list",
                        "ALI_PREALIGNER::mapper_approximation()");

    /*
     * combine all possibilities
     */
    map = map_list->first();
    do {
        if (!work_sol->insert(map))
            ali_fatal_error("Can't insert map",
                            "ALI_PREALIGNER::mapper_approximation()");

        mapper_approximation(area_no + 1, map_lists, work_sol);

        if (!work_sol->delete_map(map))
            ali_fatal_error("Can't delete map",
                            "ALI_PREALIGNER::mapper_approximation()");

        if (map_list->is_next())
            map = map_list->next();
        else
            map = 0;
    } while (map != 0);
}

/*
 * Make an approximation by aligning the undefined sections
 */
void            ALI_PREALIGNER::
make_approximation(ALI_PREALIGNER_CONTEXT * context)
{
    ALI_SUB_SOLUTION *work_solution;
    ALI_ALIGNER_CONTEXT aligner_context;
    ALI_TARRAY < ALI_TLIST < ALI_MAP * >*>*map_lists;
    ALI_ALIGNER    *aligner;
    unsigned long   area_number;
    unsigned long   start_seq, end_seq, start_ref, end_ref;

    ali_message("Align free areas");

    work_solution = new ALI_SUB_SOLUTION(sub_solution);

    aligner_context.max_number_of_maps = context->max_number_of_maps_aligner;

    area_number = sub_solution->number_of_free_areas();
    printf("number of areas = %ld (Maximal %ld solutions)\n", area_number,
           aligner_context.max_number_of_maps);

    map_lists = new ALI_TARRAY < ALI_TLIST < ALI_MAP * >*>(area_number);

    /*
     * generate Solutions for all free areas
     */
    area_number = 0;
    while (sub_solution->free_area(&start_seq, &end_seq, &start_ref, &end_ref,
                                   area_number)) {
        printf("aligning area %ld (%ld,%ld) - (%ld,%ld)\n",
               area_number, start_seq, end_seq, start_ref, end_ref);

        aligner = new ALI_ALIGNER(&aligner_context, profile,
                                  start_seq, end_seq, start_ref, end_ref);
        map_lists->set(area_number, aligner->solutions());

        printf("%d solutions generated\n",
               map_lists->get(area_number)->cardinality());

        delete          aligner;
        area_number++;
    }

    /*
     * combine and evaluate the solutions
     */
    mapper_approximation(0, map_lists, work_solution);

    delete          work_solution;
    // delete               map_lists;      // @@@

    ali_message("Free areas aligned");
}


/*
 * approximate the number of solutions in the pathmap
 */
unsigned long   ALI_PREALIGNER::
number_of_solutions(void)
{
#define INFINIT 1000000
#define ADD(a,b)  if (a>=INFINIT || b>=INFINIT) {a = INFINIT;} else {a += b;}

    unsigned long   pos_x, pos_y, col_length;
    unsigned long   number;
    unsigned char   value;
    unsigned long  *column1, *column2, *elem_akt_col, *elem_left_col;

    col_length = end_y - start_y + 1;
    column1 = (unsigned long *) CALLOC((unsigned int) col_length,
                                       sizeof(unsigned long));
    column2 = (unsigned long *) CALLOC((unsigned int) col_length,
                                       sizeof(unsigned long));
    if (column1 == 0 || column2 == 0)
        ali_fatal_error("Out of memory");

    ali_message("Start: Checking number of solutions");

    if (end_x - start_x & 0x01) {
        elem_akt_col = column1 + col_length - 1;
        elem_left_col = column2 + col_length - 1;
    } else {
        elem_akt_col = column2 + col_length - 1;
        elem_left_col = column1 + col_length - 1;
    }

    number = 0;
    *elem_akt_col = 1;
    for (pos_x = end_x - start_x; pos_x > 0;) {
        *(elem_left_col) = 0;
        for (pos_y = end_y - start_y; pos_y > 0; pos_y--) {
            *(elem_left_col - 1) = 0;
            value = path_map->get_value(pos_x, pos_y);
            if (value & ALI_UP) {
                /* *(elem_akt_col - 1) += *elem_akt_col; */
                ADD(*(elem_akt_col - 1), *elem_akt_col);
            }
            if (value & ALI_DIAG) {
                /* *(elem_left_col - 1) += *elem_akt_col; */
                ADD(*(elem_left_col - 1), *elem_akt_col);
            }
            if (value & ALI_LEFT) {
                /* *(elem_left_col) += *elem_akt_col; */
                ADD(*(elem_left_col), *elem_akt_col);
            }
            elem_akt_col--;
            elem_left_col--;
        }
        value = path_map->get_value(pos_x, 0);
        if (value & ALI_UP) {
            /* number += *elem_akt_col; */
            ADD(number, *elem_akt_col);
        }
        if (value & ALI_DIAG) {
            /* number += *elem_akt_col; */
            ADD(number, *elem_akt_col);
        }
        if (value & ALI_LEFT) {
            /* *(elem_left_col) += *elem_akt_col; */
            ADD(*(elem_left_col), *elem_akt_col);
        }
        pos_x--;
        /*
         * toggle the columns
         */
        if (pos_x & 0x01) {
            elem_akt_col = column1 + col_length - 1;
            elem_left_col = column2 + col_length - 1;
        } else {
            elem_akt_col = column2 + col_length - 1;
            elem_left_col = column1 + col_length - 1;
        }
    }

    for (pos_y = end_y - start_y; pos_y > 0; pos_y--) {
        value = path_map->get_value(0, pos_y);
        if (value & ALI_UP) {
            /* *(elem_akt_col - 1) += *elem_akt_col; */
            ADD(*(elem_akt_col - 1), *elem_akt_col);
        }
        if (value & ALI_DIAG) {
            /* number += *elem_akt_col; */
            ADD(number, *elem_akt_col);
        }
        if (value & ALI_LEFT) {
            /* number += *elem_akt_col; */
            ADD(number, *elem_akt_col);
        }
        elem_akt_col--;
    }

    /* number += *elem_akt_col; */
    ADD(number, *elem_akt_col);

    ali_message("End: Checking number of solutions");

    free((char *) column1);
    free((char *) column2);

    return number;
}


/*****************************************************************************
 *
 * class ALI_PREALIGNER (public)
 *
 *****************************************************************************/

ALI_PREALIGNER::ALI_PREALIGNER(ALI_PREALIGNER_CONTEXT * context,
                               ALI_PROFILE * prof,
                               unsigned long sx, unsigned long ex,
                               unsigned long sy, unsigned long ey)
{
    profile = prof;

    start_x = sx;
    end_x = ex;
    start_y = sy;
    end_y = ey;

    result_mask_counter = context->max_number_of_maps;

    ali_message("Prealigning");

    path_map = new ALI_PATHMAP(end_x - start_x + 1, end_y - start_y + 1);

    calculate_matrix();

    make_map();

    result_mask.delete_expensive(context, profile);
    delete          path_map;

    generate_solution(result_mask.map);

    make_approximation(context);

    ali_message("Prealigning finished");
}
