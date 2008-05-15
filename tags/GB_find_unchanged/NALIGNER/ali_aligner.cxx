#include <stdlib.h>

#include "ali_aligner.hxx"

/*****************************************************************************
 *
 * STRUCT: ali_aligner_dellist
 *
 *****************************************************************************/


/*
 * increase all multi gaps and update the apropriate costs
 */
float ali_aligner_dellist::update(unsigned long position) {
    float minimal_costs = 0.0;
    ali_aligner_dellist_elem *elem;

    if (!list_of_dels.is_empty()) {
        elem = list_of_dels.first();
        elem->costs += profile->w_del(elem->start,position);
        minimal_costs = elem->costs;

        while (list_of_dels.is_next()) {
            elem = list_of_dels.next();
            elem->costs += profile->w_del(elem->start,position);
            if (elem->costs < minimal_costs)
                minimal_costs = elem->costs;
        }
    }

    return minimal_costs;
}

/*
 * select all gaps with defined costs and put them together 
 * inside an ALI_TARRAY
 */
ALI_TARRAY<ali_pathmap_up_pointer> *ali_aligner_dellist::starts(float costs,
                                                                unsigned long y_offset) {
    unsigned long counter = 0;
    ali_aligner_dellist_elem *elem;
    ALI_TARRAY<ali_pathmap_up_pointer> *array = 0;
    ali_pathmap_up_pointer up;

    if (!list_of_dels.is_empty()) {
        /*
         * Count all elements with given costs
         */
        elem = list_of_dels.first();
        if (elem->costs == costs)
            counter++;
        while (list_of_dels.is_next()) {
            elem = list_of_dels.next();
            if (elem->costs == costs)
                counter++;
        }

        /*
         * copy all elements with given costs to an array
         */
        array = new ALI_TARRAY<ali_pathmap_up_pointer>(counter);
        counter = 0;
        elem = list_of_dels.first();
        if (elem->costs == costs) {
            up.start = elem->start - y_offset;
            up.operation = elem->operation;
            array->set(counter++,up);
        }
        while (list_of_dels.is_next()) {
            elem = list_of_dels.next();
            if (elem->costs == costs) {
                up.start = elem->start - y_offset;
                up.operation = elem->operation;
                array->set(counter++,up);
            }
        }
    }

    return array;
}

/*
 * optimize the list of multi gaps. 
 * delete all gaps which costs will ALLWAYS be higher than others
 */
void ali_aligner_dellist::optimize(unsigned long position) {
    ali_aligner_dellist_elem *ref, *akt;
    int del_flag;

    if (!list_of_dels.is_empty()) {
        ref = list_of_dels.first();
        while (ref) {
            del_flag = 0;
            list_of_dels.mark_element();
            if (list_of_dels.is_next())
                akt = list_of_dels.next();
            else
                akt = 0;
            while (akt) {
                if (akt->costs > ref->costs && 
                    profile->gap_percent(akt->start,position) <=
                    profile->gap_percent(ref->start,position)) {
                    del_flag = 1;
                }
                else {
                    if (ref->costs > akt->costs &&
                        profile->gap_percent(ref->start,position) <=
                        profile->gap_percent(akt->start,position)) {
                        ref->costs = akt->costs;
                        ref->start = akt->start;
                        ref->operation = akt->operation;
                        del_flag = 1;
                    }
                }
                if (del_flag) {
                    delete akt;
                    if (list_of_dels.is_next()) {
                        list_of_dels.delete_element();
                        akt = list_of_dels.current();
                    }
                    else {
                        list_of_dels.delete_element();
                        akt = 0;
                    }
                }
                else {
                    if (list_of_dels.is_next())
                        akt = list_of_dels.next();
                    else
                        akt = 0;
                }
            }
            list_of_dels.marked();
            if (list_of_dels.is_next())
                ref = list_of_dels.next();
            else
                ref = 0;
        }
    }
}


/*****************************************************************************
 *
 * CLASS: ALI_ALIGNER
 *
 * PRIVAT PART
 *
 *****************************************************************************/

GB_INLINE float ALI_ALIGNER::minimum2(float v1, float v2)
{
    return (v1 < v2) ? v1 : v2;
}

GB_INLINE float ALI_ALIGNER::minimum3(float v1, float v2, float v3)
{
    return (v1 < v2) ? ((v1 < v3) ? v1 : v3) : ((v2 < v3) ? v2 : v3);
}

GB_INLINE void ALI_ALIGNER::calculate_first_column_first_cell(
                                                              ali_aligner_cell *akt_cell, ali_aligner_dellist *del_list)
{
    del_list->make_empty();

    /*
     *   Substitution part
     */
    akt_cell->d1 = profile->w_ins_cheap(start_x,start_y) +
        profile->w_del_cheap(start_y);
    akt_cell->d2 = profile->w_sub(start_y,start_x);
    akt_cell->d3 = akt_cell->d1;

    del_list->insert(start_y,akt_cell->d1,ALI_LEFT);
}

GB_INLINE void ALI_ALIGNER::calculate_first_column_cell(
                                                        ali_aligner_cell *up_cell, ali_aligner_cell *akt_cell,
                                                        unsigned long pos_y,
                                                        ali_aligner_dellist *del_list)
{
    float v1, v2, v3;
    float costs;
    unsigned long positiony;

    positiony = start_y + pos_y;

    /*
     * Deletion part
     */
    costs = profile->w_del(positiony,positiony);
    v1 = del_list->update(positiony);
    v2 = up_cell->d2 + costs;
    v3 = up_cell->d3 + costs;
    akt_cell->d1 = minimum3(v1,v2,v3);

    if (v1 == akt_cell->d1)
        path_map[0]->set(0,pos_y,ALI_LUP,del_list->starts(v1,start_y));
    if (v2 == akt_cell->d1)
        path_map[0]->set(0,pos_y,ALI_DIAG);
    if (v3 == akt_cell->d1)
        path_map[0]->set(0,pos_y,ALI_LEFT);

    if (v2 < v3) {
        del_list->insert(positiony,v2,ALI_DIAG);
    }
    else {
        if (v3 < v2) 
            del_list->insert(positiony,v3,ALI_LEFT);
        else
            del_list->insert(positiony,v2,ALI_DIAG | ALI_LEFT);
    }
    del_list->optimize(positiony);

    /*
     * Substitution part
     */
    akt_cell->d2 = profile->w_del_multi_cheap(start_y,positiony - 1) +
        profile->w_sub(positiony,start_x);

    /*
     * Insertation part
     */
    akt_cell->d3 = profile->w_del_multi_cheap(start_y,positiony) +
        profile->w_ins(start_x,positiony);
}

void ALI_ALIGNER::calculate_first_column(
                                         ali_aligner_column *akt_col, ali_aligner_dellist *del_list)
{
    unsigned long cell;

    calculate_first_column_first_cell(&(*akt_col->cells)[0],del_list);

    for (cell = 1; cell < akt_col->column_length; cell++) {
        calculate_first_column_cell(&(*akt_col->cells)[cell - 1],
                                    &(*akt_col->cells)[cell],
                                    cell, del_list);
    }

    last_cell->update_left(&(*akt_col->cells)[akt_col->column_length - 1],
                           0 + 1, start_x, end_x);

    path_map[0]->optimize(0);
}

GB_INLINE void ALI_ALIGNER::calculate_first_cell(
                                                 ali_aligner_cell *left_cell, ali_aligner_cell *akt_cell,
                                                 unsigned long pos_x, ali_aligner_dellist *del_list)
{
    float v1, v2, v3;
    unsigned long positionx;

    positionx = start_x + pos_x;

    del_list->make_empty();

    /*
     *   Deletion part
     */
    akt_cell->d1 = profile->w_ins_multi_cheap(start_x,positionx) +
        profile->w_del(start_y,start_y);

    del_list->insert(start_y,akt_cell->d1,ALI_LEFT);

    /*
     *   Substitution part 
     */
    akt_cell->d2 = profile->w_ins_multi_cheap(start_x,positionx - 1) +
        profile->w_sub(start_y,positionx);

    /*
     *   Insertation part
     */
    v1 = left_cell->d1 + profile->w_ins(positionx,start_y);
    v2 = left_cell->d2 + profile->w_ins(positionx,start_y);
    v3 = left_cell->d3 + profile->w_ins_cheap(positionx,start_y);
    akt_cell->d3 = minimum3(v1,v2,v3);

    if (v1 == akt_cell->d3)
        path_map[2]->set(pos_x,0,ALI_UP);
    if (v2 == akt_cell->d3)
        path_map[2]->set(pos_x,0,ALI_DIAG);
    if (v3 == akt_cell->d3)
        path_map[2]->set(pos_x,0,ALI_LEFT);
}

GB_INLINE void ALI_ALIGNER::calculate_cell(
                                           ali_aligner_cell *diag_cell, ali_aligner_cell *left_cell,
                                           ali_aligner_cell *up_cell, ali_aligner_cell *akt_cell,
                                           unsigned long pos_x, unsigned long pos_y,
                                           ali_aligner_dellist *del_list)
{
    float v1, v2, v3;
    float costs;
    unsigned long positionx, positiony;

    positionx = start_x + pos_x;
    positiony = start_y + pos_y;

    /*
     * Deletion part
     */
    costs = profile->w_del(positiony,positiony);
    v1 = del_list->update(positiony);
    v2 = up_cell->d2 + costs;
    v3 = up_cell->d3 + costs;
    akt_cell->d1 = minimum3(v1,v2,v3);


    if (v1 == akt_cell->d1) 
        path_map[0]->set(pos_x,pos_y,ALI_LUP,del_list->starts(v1,start_y));
    if (v2 == akt_cell->d1)
        path_map[0]->set(pos_x,pos_y,ALI_DIAG);
    if (v3 == akt_cell->d1)
        path_map[0]->set(pos_x,pos_y,ALI_LEFT);

    if (v2 < v3)
        del_list->insert(positiony,v2,ALI_DIAG);
    else {
        if (v3 < v2)
            del_list->insert(positiony,v3,ALI_LEFT);
        else
            del_list->insert(positiony,v2,ALI_DIAG | ALI_LEFT);
    }
                
    del_list->optimize(positiony);

    /*
     * Substitution part
     */
    costs = profile->w_sub(positiony,positionx);
    v1 = diag_cell->d1 + costs;
    v2 = diag_cell->d2 + costs;
    v3 = diag_cell->d3 + costs;
    akt_cell->d2 = minimum3(v1,v2,v3);

    if (v1 == akt_cell->d2)
        path_map[1]->set(pos_x,pos_y,ALI_UP);
    if (v2 == akt_cell->d2)
        path_map[1]->set(pos_x,pos_y,ALI_DIAG);
    if (v3 == akt_cell->d2)
        path_map[1]->set(pos_x,pos_y,ALI_LEFT);

    /*
     * Insertation part
     */
    costs = profile->w_ins(positionx,positiony);
    v1 = left_cell->d1 + costs;
    v2 = left_cell->d2 + costs;
    v3 = left_cell->d3 + profile->w_ins_cheap(positionx,positiony);
    akt_cell->d3 = minimum3(v1,v2,v3);

    if (v1 == akt_cell->d3)
        path_map[2]->set(pos_x,pos_y,ALI_UP);
    if (v2 == akt_cell->d3)
        path_map[2]->set(pos_x,pos_y,ALI_DIAG);
    if (v3 == akt_cell->d3)
        path_map[2]->set(pos_x,pos_y,ALI_LEFT);
}

void ALI_ALIGNER::calculate_column(
                                   ali_aligner_column *prev_col, ali_aligner_column *akt_col,
                                   unsigned long pos_x, ali_aligner_dellist *del_list)
{
    unsigned long cell;

    calculate_first_cell(&(*prev_col->cells)[0],&(*akt_col->cells)[0],
                         pos_x,del_list);

    for (cell = 1; cell < akt_col->column_length; cell++) {
        calculate_cell(&(*prev_col->cells)[cell - 1], &(*prev_col->cells)[cell],
                       &(*akt_col->cells)[cell - 1], &(*akt_col->cells)[cell],
                       pos_x, cell, del_list);
    }

    if (pos_x < end_x) {
        last_cell->update_left(&(*akt_col->cells)[akt_col->column_length - 1],
                               pos_x + 1, start_x, end_x);
    }
        
    path_map[0]->optimize(pos_x);
}

void ALI_ALIGNER::calculate_matrix(void)
{
    ali_aligner_dellist *del_list;
    ali_aligner_column *akt_col, *prev_col, *h_col;
    unsigned long col;

    del_list = new ali_aligner_dellist(profile);
    prev_col = new ali_aligner_column(end_y - start_y + 1);
    akt_col = new ali_aligner_column(end_y - start_y + 1);
   
    last_cell->update_border(start_x,end_x,start_y,end_y);

    calculate_first_column(prev_col,del_list);

    for (col = 1; col <= end_x - start_x; col++) {
        calculate_column(prev_col,akt_col,col,del_list);
        h_col = prev_col;
        prev_col = akt_col;
        akt_col = h_col;
    }

    last_cell->update_up(prev_col,start_y,end_y);

    delete del_list;
    delete prev_col;
    delete akt_col;
}


/*
 * generate a result sequence from an stack of operations
 */
void ALI_ALIGNER::generate_result(ALI_TSTACK<unsigned char> *stack) 
{
    /*****
          unsigned long ic, sc, dc;
    *****/

    ALI_MAP *map;
    long seq_pos, dest_pos;
    long i;
 
    map = new ALI_MAP(start_x,end_x,start_y,end_y);
 
    /*****************************
ic = sc = dc = 0;
for (i = (long) stack->akt_size() - 1; i >= 0; i--)
        switch(stack->get(i)) {
                case ALI_ALIGNER_INS: ic++; break;
                case ALI_ALIGNER_SUB: sc++; break;
                case ALI_ALIGNER_DEL: dc++; break;
                default: printf("*");
        }
printf("\nStack content: %d * I, %d * S, %d * D\n",ic,sc,dc);
if (sc + ic != end_x - start_x + 1)
        printf("ACHTUNG: Insertionen und Substitutionen nicht passend zu Sequenzlaenge (%d)\n",end_x - start_x + 1);
    *****************************/

    seq_pos = start_x - 1;
    dest_pos = 0 - 1;
    i = (long) stack->akt_size() - 1;

    /*
     * handle first part of path
     */
    switch (stack->get(i)) {
        case ALI_ALIGNER_INS:
            for (; stack->get(i) == ALI_ALIGNER_INS; i--) {
                seq_pos++;
                map->set(seq_pos,0,1);
            }
            break;
        case ALI_ALIGNER_DEL:
            for (; stack->get(i) == ALI_ALIGNER_DEL; i--)
                dest_pos++;
            break;
    }

    /*
     * handle rest of path
     */
    for (; i >= 0; i--) {
        switch(stack->get(i)) {
            case ALI_ALIGNER_INS:
                seq_pos++;
                map->set(seq_pos,dest_pos,1);
                break;
            case ALI_ALIGNER_SUB:
                seq_pos++;
                dest_pos++;
                map->set(seq_pos,dest_pos,0);
                break;
            case ALI_ALIGNER_DEL:
                dest_pos++;
                break;
            default:
                ali_fatal_error("Unexpected value",
                                "ALI_ALIGNER::generate_result()");
        }
    }
 
    if ((unsigned long)(seq_pos) != map->last_base())
        ali_error("Stack and map length inconsistent",
                  "ALI_ALIGNER::generate_result()");

    if (result_counter > 0)
        result_counter--;

    result.insert(map);
}
                                

/*
 * Fill the stack with initial DELs or INSs
 */
void ALI_ALIGNER::mapper_pre(ALI_TSTACK<unsigned char> *stack,
                             unsigned long pos_x, unsigned long pos_y,
                             unsigned char operation, int random_mapping_flag)
{
    unsigned long random;
    int plane = 0;

    if ((pos_x < end_x - start_x && pos_y < end_y - start_y) ||
        (pos_x > end_x - start_x) || (pos_y > end_y - start_y))
            ali_fatal_error("Unexpected Values","ALI_ALIGNRE::mapper_pre");

if (pos_x < end_x - start_x)
    stack->push(ALI_ALIGNER_INS,end_x - start_x - pos_x);
if (pos_y < end_y - start_y)
    stack->push(ALI_ALIGNER_DEL,end_y - start_y - pos_y);

if (random_mapping_flag == 1) {
    random = (rand()>>4) % 6;
    switch(random) {
        case 0: 
            if (operation & ALI_UP) {
                plane = 0;
            }
            else {
                if (operation & ALI_DIAG)
                    plane = 1;
                else
                    plane = 2;
            }
            break;
        case 1:
            if (operation & ALI_UP) {
                plane = 0;
            }
            else {
                if (operation & ALI_LEFT)
                    plane = 2;
                else
                    plane = 1;
            }
            break;
        case 2:
            if (operation & ALI_DIAG) {
                plane = 1;
            }
            else {
                if (operation & ALI_UP)
                    plane = 0;
                else
                    plane = 2;
            }
            break;
        case 3:
            if (operation & ALI_DIAG) {
                plane = 1;
            }
            else {
                if (operation & ALI_LEFT)
                    plane = 2;
                else
                    plane = 0;
            }
            break;
        case 4:
            if (operation & ALI_LEFT) {
                plane = 2;
            }
            else {
                if (operation & ALI_UP)
                    plane = 0;
                else
                    plane = 1;
            }
            break;
        case 5:
            if (operation & ALI_LEFT) {
                plane = 2;
            }
            else {
                if (operation & ALI_DIAG)
                    plane = 1;
                else
                    plane = 0;
            }
            break;
    }


    mapper_random(stack,plane,pos_x,pos_y);
 }
 else {
     if (operation & ALI_UP)
         mapper(stack,0,pos_x,pos_y);
     if (operation & ALI_DIAG)
         mapper(stack,1,pos_x,pos_y);
     if (operation & ALI_LEFT)
         mapper(stack,2,pos_x,pos_y);
 }

if (pos_x < end_x - start_x)
    stack->pop(end_x - start_x - pos_x);
if (pos_y < end_y - start_y)
    stack->pop(end_y - start_y - pos_y);
}


/*
 * Fill the stack with rest DELs or INSs
 */
void ALI_ALIGNER::mapper_post(ALI_TSTACK<unsigned char> *stack,
                              unsigned long ins_nu, unsigned long del_nu)
{
    if (ins_nu > 0 && del_nu > 0)
        ali_fatal_error("Unexpected values",
                        "ALI_ALIGNER::mapper_post()");

    if (ins_nu > 0) {
        stack->push(ALI_ALIGNER_INS,ins_nu);
        generate_result(stack);
        stack->pop(ins_nu);
    }
    else
        if (del_nu > 0) {
            stack->push(ALI_ALIGNER_DEL,del_nu);
            generate_result(stack);
            stack->pop(del_nu);
        }
        else
            generate_result(stack);
}


/*
 * Fill the stack with a random path (of path matrix)
 */
void ALI_ALIGNER::mapper_random(ALI_TSTACK<unsigned char> *stack, int plane,
                                unsigned long pos_x, unsigned long pos_y) 
{
    unsigned long random;
    unsigned long stack_counter = 0;
    unsigned char value;
    ALI_TARRAY<ali_pathmap_up_pointer> *up_pointer;
    ali_pathmap_up_pointer up;
    unsigned long next_x, next_y;

    next_x = pos_x;
    next_y = pos_y;
    while (next_x <= pos_x && next_y <= pos_y) {
        stack_counter++;

        path_map[plane]->get(next_x,next_y,&value,&up_pointer); 
        if (value == 0 && next_x != 0 && next_y != 0)
            ali_fatal_error("Unexpected value (1)",
                            "ALI_ALIGNER::mapper_random()");

        /*
         * Set the operation
         */
        switch(plane) {
            case 0: 
                stack->push(ALI_ALIGNER_DEL);
                next_y = next_y - 1; 
                break;
            case 1: 
                stack->push(ALI_ALIGNER_SUB);
                next_x = next_x - 1; 
                next_y = next_y - 1;
                break;
            case 2: 
                stack->push(ALI_ALIGNER_INS);
                next_x = next_x - 1; 
                break;
            default:
                ali_fatal_error("Unexpected plane","ALI_ALIGNER::mapper_random()");
        }

        /*
         * special handling for LUP values
         */
        if (value & ALI_LUP) {
            if (plane != 0)
                ali_fatal_error("LUP only in plane 0 allowed",
                                "ALI_ALIGNER::mapper_random()");

            random = (unsigned long) (rand()>>4) % 2;

            /*
             * Go the LUP way
             */
            if (random == 0 || value == ALI_LUP) {
                /*
                 * Take a pointer by random
                 */
                random = (unsigned long) (rand()>>4) % up_pointer->size();
                up = up_pointer->get(random);

                if (next_y < up.start)
                    ali_fatal_error("Unexpected LUP reference",
                                    "ALI_ALIGNER::mapper_random()");
                if (up.operation & ALI_UP || up.operation & ALI_LUP)
                    ali_fatal_error("Unexpected LUP operation",
                                    "ALI_ALIGNER::mapper_random()");

                if (up.start <= next_y) {
                    stack->push(ALI_ALIGNER_DEL,next_y - up.start + 1);
                    stack_counter += (next_y - up.start + 1);
                }

                next_y = up.start - 1;
                value = up.operation;
            }
        }

        /*
         * Take the next plane by random
         */
        random = (unsigned long) (rand()>>4) % 6;
        switch(random) {
            case 0:
                if (value & ALI_UP) {
                    plane = 0;
                }
                else {
                    if (value & ALI_DIAG) {
                        plane = 1;
                    }
                    else {
                        plane = 2;
                    }
                }
                break;
            case 1:
                if (value & ALI_UP) {
                    plane = 0;
                }
                else {
                    if (value & ALI_LEFT) {
                        plane = 2;
                    }
                    else {
                        plane = 1;
                    }
                }
                break;
            case 2:
                if (value & ALI_DIAG) {
                    plane = 1;
                }
                else {
                    if (value & ALI_UP) {
                        plane = 0;
                    }
                    else {
                        plane = 2;
                    }
                }
                break;
            case 3:
                if (value & ALI_DIAG) {
                    plane = 1;
                }
                else {
                    if (value & ALI_LEFT) {
                        plane = 2;
                    }
                    else {
                        plane = 0;
                    }
                }
                break;
            case 4:
                if (value & ALI_LEFT) {
                    plane = 2;
                }
                else {
                    if (value & ALI_UP) {
                        plane = 0;
                    }
                    else {
                        plane = 1;
                    }
                }
                break;
            case 5:
                if (value & ALI_LEFT) {
                    plane = 2;
                }
                else {
                    if (value & ALI_DIAG) {
                        plane = 1;
                    }
                    else {
                        plane = 0;
                    }
                }
                break;
            default:
                ali_fatal_error("Unexpected random value"
                                "ALI_ALIGNER::mapper_random()");
        }
    }

    if (next_x <= pos_x) {
        mapper_post(stack,next_x + 1,0);
    }
    else {
        if (next_y <= pos_y) {
            mapper_post(stack,0,next_y + 1);
        }
        else {
            mapper_post(stack,0,0);
        }
    }

    if (stack_counter > 0)
        stack->pop(stack_counter);
}


/*
 * Fill the stack with a deterministic path (of path matrix)
 */
void ALI_ALIGNER::mapper(ALI_TSTACK<unsigned char> *stack, int plane,
                         unsigned long pos_x, unsigned long pos_y) 
{
    unsigned long l;
    unsigned char value;
    ALI_TARRAY<ali_pathmap_up_pointer> *up_pointer;
    ali_pathmap_up_pointer up;
    unsigned long next_x, next_y;

    /*
     * set the operation
     */

    switch(plane) {
        case 0: 
            stack->push(ALI_ALIGNER_DEL);
            next_x = pos_x; 
            next_y = pos_y - 1; 
            break;
        case 1: 
            stack->push(ALI_ALIGNER_SUB);
            next_x = pos_x - 1; 
            next_y = pos_y - 1;
            break;
        case 2: 
            stack->push(ALI_ALIGNER_INS);
            next_x = pos_x - 1; 
            next_y = pos_y; 
            break;
        default:
            ali_fatal_error("Unexpected plane","ALI_ALIGNER::mapper()");
    }

    /*
     * Check if mapping found a end
     */

    if (next_x > pos_x || next_y > pos_y) {
        if (next_y <= pos_y) {
            if (plane == 0) 
                ali_fatal_error("Unexpected plane (1)",
                                "ALI_ALIGNER::mapper()");
            mapper_post(stack,0,next_y + 1);
        }
        else {
            if (next_x <= pos_x) {
                if (plane == 2)
                    ali_fatal_error("Unexpected plane (2)",
                                    "ALI_ALIGNER::mapper()");
                mapper_post(stack,next_x + 1,0);
            }
            else {
                mapper_post(stack,0,0);
            }
        }
    }
    else {
        path_map[plane]->get(pos_x,pos_y,&value,&up_pointer);   

        if (value & ALI_UP) {
            mapper(stack, 0, next_x, next_y);
        }
        if (value & ALI_DIAG) {
            mapper(stack, 1, next_x, next_y);
        }
        if (value & ALI_LEFT) {
            mapper(stack, 2, next_x, next_y);
        }

        /*
         * Special handling for long up-pointers
         */

        if (value & ALI_LUP) {
            if (plane != 0)
                printf("LUP should never be in plane %d\n",plane);

            for (l = 0; l < up_pointer->size(); l++) {
                up = up_pointer->get(l);

                if (next_y < up.start) {
                    printf("LUP reference incorrect %d:(%ld,%ld) %ld > %ld\n",
                           plane,pos_x,pos_y,next_y,up.start);
                }
                if (up.operation & ALI_UP || up.operation & ALI_LUP) {
                    printf("LIP reference incorrect %d: wrong operation %d\n",
                           plane,up.operation);
                }

                if (up.start <= next_y)
                    stack->push(ALI_ALIGNER_DEL,next_y - up.start + 1);

                if (up.operation & ALI_DIAG)
                    mapper(stack, 1, next_x, up.start - 1);
                if (up.operation & ALI_LEFT)
                    mapper(stack, 2, next_x, up.start - 1);

                if (up.start <= next_y)
                    stack->pop(next_y - up.start + 1);
            }
        }
    }

    stack->pop();
}


/*
 * Select a random entry point from a list of valid entry points
 */
void ALI_ALIGNER::mapper_pre_random_up(ALI_TSTACK<unsigned char> *stack,
                                       ALI_TLIST<ali_pathmap_up_pointer> *list)
{
    unsigned long number;
    ali_pathmap_up_pointer p;

    number = (rand()>>4) % list->cardinality();

    if (list->is_empty())
        ali_fatal_error("Empty list",
                        "ALI_ALIGNER::mapper_pre_random_up()");

    p = list->first();
    for (; number > 0; number--) {
        if (!list->is_next())
            ali_fatal_error("List is inconsistent",
                            "ALI_ALIGNER::mapper_pre_random_up()");
        p = list->next();
    }

    mapper_pre(stack, end_x - start_x, p.start - 1, p.operation, 1);
}

/*
 * Select a random entry point from a list of valid entry points
 */
void ALI_ALIGNER::mapper_pre_random_left(
                                         ALI_TSTACK<unsigned char> *stack,
                                         ALI_TLIST<ali_pathmap_up_pointer> *list)
{
    unsigned long number;
    ali_pathmap_up_pointer p;

    number = (rand()>>4) % list->cardinality();

    if (list->is_empty())
        ali_fatal_error("Empty list",
                        "ALI_ALIGNER::mapper_pre_random_left()");

    p = list->first();
    for (; number > 0; number--) {
        if (!list->is_next())
            ali_fatal_error("List is inconsistent",
                            "ALI_ALIGNER::mapper_pre_random_left()");
        p = list->next();
    }

    mapper_pre(stack, p.start - 1, end_y - start_y, p.operation, 1);
}


/*
 * Find on path by random and make a map
 */
void ALI_ALIGNER::make_map_random(ALI_TSTACK<unsigned char> *stack)
{
    unsigned long random;
    float min;
 
    min = minimum3(last_cell->d1,last_cell->d2,last_cell->d3);
    random = (unsigned long) (rand()>>4) % 6;


    switch(random) {
        case 0:
            if (last_cell->d1 == min) {
                mapper_pre_random_up(stack,&last_cell->up_starts);
            }
            else {
                if (last_cell->d2 == min) {
                    mapper_random(stack, 1, end_x-start_x, end_y-start_y);
                }
                else {
                    if (last_cell->d3 == min) {
                        mapper_pre_random_left(stack,&last_cell->left_starts);
                    }
                }
            }
            break;
        case 1:
            if (last_cell->d1 == min) {
                mapper_pre_random_up(stack,&last_cell->up_starts);
            }
            else {
                if (last_cell->d3 == min) {
                    mapper_pre_random_left(stack,&last_cell->left_starts);
                }
                else {
                    if (last_cell->d2 == min) {
                        mapper_random(stack, 1, end_x-start_x, end_y-start_y);
                    }
                }
            }
            break;
        case 2:
            if (last_cell->d2 == min) {
                mapper_random(stack, 1, end_x-start_x, end_y-start_y);
            }
            else {
                if (last_cell->d1 == min) {
                    mapper_pre_random_up(stack,&last_cell->up_starts);
                }
                else {
                    if (last_cell->d3 == min) {
                        mapper_pre_random_left(stack,&last_cell->left_starts);
                    }
                }
            }
            break;
        case 3:
            if (last_cell->d2 == min) {
                mapper_random(stack, 1, end_x-start_x, end_y-start_y);
            }
            else {
                if (last_cell->d3 == min) {
                    mapper_pre_random_left(stack,&last_cell->left_starts);
                }
                else {
                    if (last_cell->d1 == min) {
                        mapper_pre_random_up(stack,&last_cell->up_starts);
                    }
                }
            }
            break;
        case 4:
            if (last_cell->d3 == min) {
                mapper_pre_random_left(stack,&last_cell->left_starts);
            }
            else {
                if (last_cell->d2 == min) {
                    mapper_random(stack, 1, end_x-start_x, end_y-start_y);
                }
                else {
                    if (last_cell->d1 == min) {
                        mapper_pre_random_up(stack,&last_cell->up_starts);
                    }
                }
            }
            break;
        case 5:
            if (last_cell->d3 == min) {
                mapper_pre_random_left(stack,&last_cell->left_starts);
            }
            else {
                if (last_cell->d1 == min) {
                    mapper_pre_random_up(stack,&last_cell->up_starts);
                }
                else {
                    if (last_cell->d2 == min) {
                        mapper_random(stack, 1, end_x-start_x, end_y-start_y);
                    }
                }
            }
            break;
    }
}


/*
 * find ALL paths and make the apropriate maps
 */
void ALI_ALIGNER::make_map_systematic(ALI_TSTACK<unsigned char> *stack) 
{
    float min;
    ali_pathmap_up_pointer p;
    ALI_TLIST<ali_pathmap_up_pointer> *list;

    min = minimum3(last_cell->d1,last_cell->d2,last_cell->d3);

    if (last_cell->d1 == min) {
        list = &(last_cell->up_starts);
        if (!list->is_empty()) {
            p = list->first();
            mapper_pre(stack, end_x - start_x, p.start - 1, p.operation);
            while (list->is_next()) {
                p = list->next();
                mapper_pre(stack, end_x - start_x, p.start - 1, p.operation);
            }
        }
    }

    if (last_cell->d2 == min) {
        mapper(stack, 1, end_x - start_x, end_y - start_y);
    }

    if (last_cell->d3 == min) {
        list = &(last_cell->left_starts);
        if (!list->is_empty()) {
            p = list->first();
            mapper_pre(stack, p.start - 1, end_y - start_y, p.operation);
            while (list->is_next()) {
                p = list->next();
                mapper_pre(stack, p.start - 1, end_y - start_y, p.operation);
            }
        }
    }
}


/*
 * make the list of result maps
 */
void ALI_ALIGNER::make_map(void)
{
    ALI_TSTACK<unsigned char> *stack;

    srand((unsigned int) (GB_time_of_day() % 1000));

    stack = new ALI_TSTACK<unsigned char>(end_x - start_x + end_y - start_y + 5);

    /********** ACHTUNG ACHTUNG
                number_of_solutions() noch nicht zuverlaessig!!
                => es wird IMMER nur EINE Loesung herausgenommen!!

                number_of_sol = number_of_solutions();

                if (result_counter <= number_of_sol) {
                ali_message("Starting systematic mapping");
                make_map_systematic(stack);
                }
                else {
                ali_message("Starting random mapping");
                do {
                make_map_random(stack);
                } while (result_counter > 0);
                }
    ***********/

    make_map_random(stack);

    delete stack;
}




struct ali_aligner_tripel {
    unsigned long v1, v2, v3;
};

/*
 * approximate the number of solutions produced by the path matrix
 */
unsigned long ALI_ALIGNER::number_of_solutions(void)
{
    float min;
    unsigned long pos_x, pos_y, col_length;
    unsigned long number;
    unsigned long l;
    unsigned char value;
    ALI_TARRAY<ali_pathmap_up_pointer> *up_pointer;
    ALI_TLIST<ali_pathmap_up_pointer> *list;
    ali_pathmap_up_pointer up;
    ali_aligner_tripel *column1, *column2, *elem_akt_col, *elem_left_col;

    col_length = end_y - start_y + 1;
    column1 = (ali_aligner_tripel *) CALLOC((unsigned int) col_length,
                                            sizeof(ali_aligner_tripel));
    column2 = (ali_aligner_tripel *) CALLOC((unsigned int) col_length,
                                            sizeof(ali_aligner_tripel));
    if (column1 == 0 || column2 == 0)
        ali_fatal_error("Out of memory");

    if (end_x - start_x & 0x01) {
        elem_akt_col = column1 + col_length - 1;
        elem_left_col = column2 + col_length - 1;
    }
    else {
        elem_akt_col = column2 + col_length - 1;
        elem_left_col = column1 + col_length - 1;
    }

    number = 0;

    /*
     * Initialize all end points in the last column
     */
    min = minimum3(last_cell->d1,last_cell->d2,last_cell->d3);

    if (last_cell->d1 == min) {
        list = &(last_cell->up_starts);
        if (!list->is_empty()) {
            up = list->first();
            if (up.start == 0) {
                number += 1;
            }
            else {
                switch (up.operation) {
                    case ALI_UP:
                        ali_fatal_error("Unexpected Value (1)",
                                        "ALI_ALIGNER::number_of_solutions()");
                        break;
                    case ALI_DIAG:
                        (elem_akt_col - col_length + 1 + up.start - 1)->v2 += 1;
                        break;
                    case ALI_LEFT:
                        (elem_akt_col - col_length + 1 + up.start - 1)->v3 += 1;
                        break;
                }
            }
            while (list->is_next()) {
                up = list->next();
                if (up.start == 0) {
                    number += 1;
                }
                else {
                    switch (up.operation) {
                        case ALI_UP:
                            ali_fatal_error("Unexpected Value (1)",
                                            "ALI_ALIGNER::number_of_solutions()");
                            break;
                        case ALI_DIAG:
                            (elem_akt_col - col_length + 1 + up.start - 1)->v2 += 1;
                            break;
                        case ALI_LEFT:
                            (elem_akt_col - col_length + 1 + up.start - 1)->v3 += 1;
                            break;
                    }
                }
            }
        }
    }

    if (last_cell->d2 == min) {
        (elem_akt_col)->v2 += 1;
    }

    /*
     * Calculate all columns, from last to first (without the first)
     */
    for (pos_x = end_x - start_x; pos_x > 0;) {

        elem_left_col->v1 = elem_left_col->v2 = elem_left_col->v3 = 0;

        /*
         * Calculate all cells, from last to first (without the first)
         */
        for (pos_y = end_y - start_y; pos_y > 0; pos_y--) {

            (elem_left_col - 1)->v1 = (elem_left_col - 1)->v2 =
                (elem_left_col - 1)->v3 = 0;

            path_map[0]->get(pos_x,pos_y,&value,&up_pointer);
            if (value & ALI_UP) 
                (elem_akt_col - 1)->v1 += elem_akt_col->v1;
            if (value & ALI_DIAG) 
                (elem_akt_col - 1)->v2 += elem_akt_col->v1;
            if (value & ALI_LEFT) 
                (elem_akt_col - 1)->v3 += elem_akt_col->v1;
            if (value & ALI_LUP) {
                for (l = 0; l < up_pointer->size(); l++) {
                    up = up_pointer->get(l);
                    if (pos_y - 1 < up.start || up.operation & ALI_UP ||
                        up.operation & ALI_LUP) 
                        ali_fatal_error("Inconsistent LUP reference",
                                        "ALI_ALIGNER::number_of_solutions()");
                    if (up.start == 0) {
                        number += elem_akt_col->v1;
                    }
                    if (up.operation & ALI_DIAG)
                        (elem_akt_col - pos_y + up.start - 1)->v2 += elem_akt_col->v1;
                    if (up.operation & ALI_LEFT)
                        (elem_akt_col - pos_y + up.start - 1)->v3 += elem_akt_col->v1;
                }
            }

            path_map[1]->get(pos_x,pos_y,&value,&up_pointer);
            if (value & ALI_UP)
                (elem_left_col - 1)->v1 += elem_akt_col->v2;
            if (value & ALI_DIAG)
                (elem_left_col - 1)->v2 += elem_akt_col->v2;
            if (value & ALI_LEFT)
                (elem_left_col - 1)->v3 += elem_akt_col->v2;
            if (value & ALI_LUP)
                ali_fatal_error("Unexpected value",
                                "ALI_ALIGNER::number_of_solutions()");

            path_map[2]->get(pos_x,pos_y,&value,&up_pointer);
            if (value & ALI_UP)
                (elem_left_col)->v1 += elem_akt_col->v3;
            if (value & ALI_DIAG)
                (elem_left_col)->v2 += elem_akt_col->v3;
            if (value & ALI_DIAG)
                (elem_left_col)->v3 += elem_akt_col->v3;
            if (value & ALI_LUP)
                ali_fatal_error("Unexpected value",
                                "ALI_ALIGNER::number_of_solutions()");

            elem_akt_col--;
            elem_left_col--;
        }

        /*
         * Calculate the first cell
         */

        number += elem_akt_col->v1;
        number += elem_akt_col->v2;

        path_map[2]->get(pos_x,pos_y,&value,&up_pointer);
        if (value & ALI_UP)
            (elem_left_col)->v1 += elem_akt_col->v3;
        if (value & ALI_DIAG)
            (elem_left_col)->v2 += elem_akt_col->v3;
        if (value & ALI_DIAG)
            (elem_left_col)->v3 += elem_akt_col->v3;
        if (value & ALI_LUP)
            ali_fatal_error("Unexpected value",
                            "ALI_ALIGNER::number_of_solutions()");

        pos_x--;
        /*
         * toggle the columns
         */
        if (pos_x & 0x01) {
            elem_akt_col = column1 + col_length - 1;
            elem_left_col = column2 + col_length - 1;
        }
        else {
            elem_akt_col = column2 + col_length - 1;
            elem_left_col = column1 + col_length - 1;
        }

        /*
         * Initialize end point at last cell of column
         */
        if (last_cell->d3 == min) {
            (elem_akt_col)->v1 = (elem_akt_col)->v2 = (elem_akt_col)->v3 = 0;
            list = &(last_cell->left_starts);
            if (!list->is_empty()) {
                up = list->first();
                if (up.start - 1 == pos_x) {
                    switch (up.operation) {
                        case ALI_UP:
                            (elem_akt_col)->v1 += 1;
                            break;
                        case ALI_DIAG:
                            (elem_akt_col)->v2 += 1;
                            break;
                        case ALI_LEFT:
                            ali_fatal_error("Unexpected value",
                                            "ALI_ALIGNER::number_of_solutions()");
                            break;
                    }
                }
                while (list->is_next()) {
                    up = list->next();
                    if (up.start == pos_x) {
                        switch (up.operation) {
                            case ALI_UP:
                                (elem_akt_col)->v1 += 1;
                                break;
                            case ALI_DIAG:
                                (elem_akt_col)->v2 += 1;
                                break;
                            case ALI_LEFT:
                                ali_fatal_error("Unexpected value",
                                                "ALI_ALIGNER::number_of_solutions()");
                                break;
                        }
                    }
                }
            }
        }
    }

    /*
     * Calculate the cells of the first column, 
     * from last to first (without first)
     */
    for (pos_y = end_y - start_y; pos_y > 0; pos_y--) {

        path_map[0]->get(pos_x,pos_y,&value,&up_pointer);
        if (value & ALI_UP) 
            (elem_akt_col - 1)->v1 += elem_akt_col->v1;
        if (value & ALI_DIAG) {
            number += elem_akt_col->v1;
        }
        if (value & ALI_LEFT) {
            number += elem_akt_col->v1;
        }
        if (value & ALI_LUP) {
            for (l = 0; l < up_pointer->size(); l++) {
                up = up_pointer->get(l);
                if (pos_y - 1 < up.start || up.operation & ALI_UP ||
                    up.operation & ALI_LUP) 
                    ali_fatal_error("Inconsistent LUP reference",
                                    "ALI_ALIGNER::number_of_solutions()");
                if (up.operation & ALI_DIAG) {
                    number += elem_akt_col->v1;
                }
                if (up.operation & ALI_LEFT) {
                    number += elem_akt_col->v1;
                }
            }
        }

        number += elem_akt_col->v2;
        number += elem_akt_col->v3;

        elem_akt_col--;
    }

    number += elem_akt_col->v1;
    number += elem_akt_col->v2;
    number += elem_akt_col->v3;

    free ((char *) column1);
    free ((char *) column2);

    return number;
}




/*****************************************************************************
 *
 * CLASS: ALI_ALIGNER
 *
 * PUBLIC PART
 *
 *****************************************************************************/

ALI_ALIGNER::ALI_ALIGNER(ALI_ALIGNER_CONTEXT *context, ALI_PROFILE *prof,
                         unsigned long sx, unsigned long ex,
                         unsigned long sy, unsigned long ey)
{
    profile = prof;
    start_x = sx;
    end_x = ex;
    start_y = sy;
    end_y = ey;

    ali_message("Starting main aligner");

    result_counter = context->max_number_of_maps;

    last_cell = new ali_aligner_last_cell(prof);

    path_map[0] = new ALI_PATHMAP(end_x - start_x + 1,end_y - start_y + 1);
    path_map[1] = new ALI_PATHMAP(end_x - start_x + 1,end_y - start_y + 1);
    path_map[2] = new ALI_PATHMAP(end_x - start_x + 1,end_y - start_y + 1);

    calculate_matrix();

    make_map();

    /*
     * Delete all unused objects
     */
    delete last_cell;
    delete path_map[0];
    delete path_map[1];
    delete path_map[2];

    ali_message("Main aligner finished");
}
