

#ifndef _ALI_ALIGNER_INC_
#define _ALI_ALIGNER_INC_

// #include <malloc.h>

#include "ali_solution.hxx"
#include "ali_tstack.hxx"
#include "ali_tlist.hxx"
#include "ali_tarray.hxx"
#include "ali_profile.hxx"
#include "ali_pathmap.hxx"

#define ALI_ALIGNER_INS        1
#define ALI_ALIGNER_SUB        2
#define ALI_ALIGNER_DEL        3
#define ALI_ALIGNER_MULTI_FLAG 4

struct ALI_ALIGNER_CONTEXT {
    long max_number_of_maps;
};

/*
 * Structure of a cell of the distance matrix
 */
struct ali_aligner_cell {
    float d1, d2, d3;
    ALI_TARRAY<ali_pathmap_up_pointer> *starts;

    ali_aligner_cell(void) {
        d1 = d2 = d3 = 0.0;
        starts = 0;
    }

    void print(void) {
        printf("%4.2f %4.2f %4.2f %8p",d1,d2,d3,starts);
    }
};

/*
 * Structure of a column of the distance matrix
 */
struct ali_aligner_column {
    unsigned long column_length;
    //ali_aligner_cell (*cells)[];
    ali_aligner_cell **cells;

    ali_aligner_column(unsigned long length) {
        column_length = length;
        cells = (ali_aligner_cell**) calloc((unsigned int) column_length, sizeof(ali_aligner_cell));
        //cells = (ali_aligner_cell (*) [0]) calloc((unsigned int) column_length, sizeof(ali_aligner_cell));
        //cells = (ali_aligner_cell (*) [1]) calloc((unsigned int) column_length, sizeof(ali_aligner_cell));
        if (cells == 0)
            ali_fatal_error("Out of memory");
    }
    ~ali_aligner_column(void) {
        if (cells)
            free((char  *) cells);
    }

    void print(void) {
        unsigned int i;
        for (i = 0; i < column_length; i++) {
            printf("%2d: ",i);
            (*cells)[i].print();
            printf("\n");
        }
    }
};

/*
 * Structure of a LONG deletion (multi gap) in the distance matrix
 */
struct ali_aligner_dellist_elem {
    unsigned long start;
    float costs;
    unsigned char operation;

    ali_aligner_dellist_elem(unsigned long s = 0, float c = 0.0,
                             unsigned char op = 0) {
        start = s;
        costs = c;
        operation = op;
    }

    void print(void) {
        printf("(%3ld %4.2f %2d)",start,costs,operation);
    }
};

/*
 * Structure of the list of LONG deletions in the distance matrix
 */
struct ali_aligner_dellist {
    ALI_PROFILE *profile;
    ALI_TLIST<ali_aligner_dellist_elem *> list_of_dels;

    ali_aligner_dellist(ALI_PROFILE *p) {
        profile = p;
    }
    ~ali_aligner_dellist(void) {
        ali_aligner_dellist_elem * elem;
        if (!list_of_dels.is_empty()) {
            elem = list_of_dels.first();
            while (list_of_dels.is_next()) {
                delete elem;
                elem = list_of_dels.next();
            }
            delete elem;
        }
    }

    void print(void) {
        printf("DEL_LIST: ");
        if (!list_of_dels.is_empty()) {
            list_of_dels.first()->print();
            while (list_of_dels.is_next()) {
                printf(", ");
                list_of_dels.next()->print();
            }
        }
        printf("\n");
    }
    void print_cont(unsigned long position) {
        ali_aligner_dellist_elem *elem;
        if (!list_of_dels.is_empty()) {
            elem = list_of_dels.first();
            while (list_of_dels.is_next()) {
                elem->print();
                printf("\t %f\n",profile->gap_percent(elem->start,position));
                elem = list_of_dels.next();
            }
            elem->print();
            printf("\t %f\n",profile->gap_percent(elem->start,position));
        }
    }
    unsigned long length(void) {
        return list_of_dels.cardinality();
    }
    void make_empty(void) {
        ali_aligner_dellist_elem *elem;
        if (!list_of_dels.is_empty()) {
            elem = list_of_dels.first();
            while (list_of_dels.is_next()) {
                delete elem;
                elem = list_of_dels.next();
            }
            delete elem;
        }
        list_of_dels.make_empty();
    }
    void insert(unsigned long start, float costs, unsigned char operation) {
        ali_aligner_dellist_elem *new_elem;

        new_elem = new ali_aligner_dellist_elem(start,costs,operation);
        list_of_dels.append_end(new_elem);
    }
    float update(unsigned long position);
    ALI_TARRAY<ali_pathmap_up_pointer> *starts(float costs,
                                               unsigned long y_offset);
    void optimize(unsigned long position);
};


/*
 * Structure of the virtual cell at the left buttom
 */
struct ali_aligner_last_cell {
    ALI_PROFILE *profile;
    float d1, d2, d3;
    ALI_TLIST<ali_pathmap_up_pointer> left_starts;
    ALI_TLIST<ali_pathmap_up_pointer> up_starts;

    ali_aligner_last_cell(ALI_PROFILE *prof) {
        profile = prof;
        d1 = d2 = d3 = -1.0;
    }

    void insert_left(unsigned long start, unsigned char operation, float costs) {
        ali_pathmap_up_pointer left;
        if (costs < d3 || d3 == -1.0) {
            left_starts.make_empty();
            d3 = costs;
        }
        if (costs == d3) {
            left.start = start;
            left.operation = operation;
            left_starts.append_end(left);
        }
    }
    void insert_up(unsigned long start, unsigned char operation, float costs) {
        ali_pathmap_up_pointer up;
        if (costs < d1 || d1 == -1.0) {
            up_starts.make_empty();
            d1 = costs;
        }
        if (costs == d1) {
            up.start = start;
            up.operation = operation;
            up_starts.append_end(up);
        }
    }
    void update_border(unsigned long start_x, unsigned long end_x,
                       unsigned long start_y, unsigned long end_y)
    {
        float cost;

        cost = profile->w_del_multi(start_y, end_y) +
            profile->w_ins_multi(start_x, end_x);

        insert_left(0, ALI_UP, cost);
        insert_up(0, ALI_LEFT, cost);
    }
    void update_left(ali_aligner_cell *akt_cell, unsigned long akt_pos,
                     unsigned long start_x, unsigned long end_x) {
        float min;
        unsigned char operation = 0;

        min = (akt_cell->d1 < akt_cell->d2) ? akt_cell->d1 : akt_cell->d2;
        if (akt_cell->d1 == min)
            operation |= ALI_UP;
        if (akt_cell->d2 == min)
            operation |= ALI_DIAG;

        insert_left(akt_pos, operation,
                    min + profile->w_ins_multi_cheap(start_x + akt_pos, end_x));
    }
    void update_up(ali_aligner_cell *akt_cell, unsigned long akt_pos,
                   unsigned long start_y, unsigned long end_y) {
        float min;
        unsigned char operation = 0;

        min = (akt_cell->d3 < akt_cell->d2) ? akt_cell->d3 : akt_cell->d2;
        if (akt_cell->d3 == min)
            operation |= ALI_LEFT;
        if (akt_cell->d2 == min)
            operation |= ALI_DIAG;

        insert_up(akt_pos,operation,
                  min + profile->w_del_multi_cheap(start_y + akt_pos, end_y));
    }
    void update_up(ali_aligner_column *akt_col,
                   unsigned long start_y, unsigned long end_y) {
        unsigned long cell;

        /*
         * Value for start := start + 1 (because of -1)
         */

        for (cell = 0; cell < akt_col->column_length - 1; cell++)
            update_up(&(*akt_col->cells)[cell], cell + 1, start_y, end_y);

        d2 = (*akt_col->cells)[akt_col->column_length - 1].d2;
    }

    void print(void) {
        ali_pathmap_up_pointer elem;
        printf("d1 = %f, d2 = %f, d3 = %f\n",d1,d2,d3);
        printf("left starts = ");
        if (!left_starts.is_empty()) {
            elem = left_starts.first();
            printf("<(%ld %d)",(long)elem.start,elem.operation);
            while (left_starts.is_next()) {
                elem = left_starts.next();
                printf(", (%ld %d)",(long)elem.start,elem.operation);
            }
            printf(">\n");
        }
        else {
            printf("empty\n");
        }
        printf("up starts = ");
        if (!up_starts.is_empty()) {
            elem = up_starts.first();
            printf("<(%ld %d)",elem.start,elem.operation);
            while (up_starts.is_next()) {
                elem = up_starts.next();
                printf(", (%ld %d)",(long)elem.start,elem.operation);
            }
            printf(">\n");
        }
        else {
            printf("empty\n");
        }
    }
};


/*
 * Structure for collecting all possible solution
 */
struct ali_aligner_result {
    ALI_TLIST<ALI_MAP *> *map_list;

    ali_aligner_result(void) {
        map_list = new ALI_TLIST<ALI_MAP *>;
    }
    ~ali_aligner_result(void) {
        if (map_list) {
            clear();
            delete map_list;
        }
    }

    void insert(ALI_MAP *in_map) {
        map_list->append_end(in_map);
    }
    void clear(void) {
        if (!map_list->is_empty()) {
            delete map_list->first();
            while (map_list->is_next())
                delete map_list->next();
        }
    }
};


/*
 * Class of the extended aligner
 */
class ALI_ALIGNER {
    ALI_PROFILE *profile;
    ALI_PATHMAP *path_map[3];
    ali_aligner_last_cell *last_cell;

    ali_aligner_result result;
    unsigned long result_counter;
    unsigned long start_x, end_x, start_y, end_y;

    float minimum2(float a, float b);
    float minimum3(float a, float b, float c);

    void calculate_first_column_first_cell(
                                           ali_aligner_cell *akt_cell, ali_aligner_dellist *del_list);
    void calculate_first_column_second_cell(
                                            ali_aligner_cell *up_cell, ali_aligner_cell *akt_cell,
                                            ali_aligner_dellist *del_list);
    void calculate_first_column_cell(
                                     ali_aligner_cell *up_cell, ali_aligner_cell *akt_cell,
                                     unsigned long pos_y, ali_aligner_dellist *del_list);
    void calculate_first_column(ali_aligner_column *akt_col,
                                ali_aligner_dellist *del_list);

    void calculate_second_column_first_cell(
                                            ali_aligner_cell *left_cell, ali_aligner_cell *akt_cell,
                                            ali_aligner_dellist *del_list);
    void calculate_second_column_second_cell(
                                             ali_aligner_cell *diag_cell, ali_aligner_cell *left_cell,
                                             ali_aligner_cell *up_cell, ali_aligner_cell *akt_cell,
                                             ali_aligner_dellist *del_list);
    void calculate_second_column_cell(
                                      ali_aligner_cell *diag_cell, ali_aligner_cell *left_cell,
                                      ali_aligner_cell *up_cell, ali_aligner_cell *akt_cell,
                                      unsigned long pos_y, ali_aligner_dellist *del_list);
    void calculate_second_column(ali_aligner_column *prev_col,
                                 ali_aligner_column *akt_col,
                                 ali_aligner_dellist *del_list);

    void calculate_first_cell(
                              ali_aligner_cell *left_cell, ali_aligner_cell *akt_cell,
                              unsigned long pos_x, ali_aligner_dellist *del_list);
    void calculate_second_cell(
                               ali_aligner_cell *diag_cell, ali_aligner_cell *left_cell,
                               ali_aligner_cell *up_cell, ali_aligner_cell *akt_cell,
                               unsigned long pos_x, ali_aligner_dellist *del_list);
    void calculate_cell(ali_aligner_cell *diag_cell, ali_aligner_cell *left_cell,
                        ali_aligner_cell *up_cell, ali_aligner_cell *akt_cell,
                        unsigned long pos_x, unsigned long pos_y,
                        ali_aligner_dellist *del_list);
    void calculate_column(ali_aligner_column *prev_col,
                          ali_aligner_column *akt_col,
                          unsigned long pos_x,
                          ali_aligner_dellist *del_list);

    void calculate_matrix(void);

    void generate_result(ALI_TSTACK<unsigned char> *stack);
    void mapper_pre(ALI_TSTACK<unsigned char> *stack,
                    unsigned long pos_x, unsigned long pos_y,
                    unsigned char operation,
                    int random_mapping_flag = 0);
    void mapper_post(ALI_TSTACK<unsigned char> *stack,
                     unsigned long pos_x, unsigned long pos_y);
    void mapper_pre_random_up(
                              ALI_TSTACK<unsigned char> *stack,
                              ALI_TLIST<ali_pathmap_up_pointer> *list);
    void mapper_pre_random_left(
                                ALI_TSTACK<unsigned char> *stack,
                                ALI_TLIST<ali_pathmap_up_pointer> *list);
    void mapper_random(ALI_TSTACK<unsigned char> *stack, int plane,
                       unsigned long pos_x, unsigned long pos_y);
    void mapper(ALI_TSTACK<unsigned char> *stack, int plane,
                unsigned long pos_x, unsigned long pos_y);
    void make_map_random(ALI_TSTACK<unsigned char> *stack);
    void make_map_systematic(ALI_TSTACK<unsigned char> *stack);
    void make_map(void);

    unsigned long number_of_solutions();

public:
    ALI_ALIGNER(ALI_ALIGNER_CONTEXT *context, ALI_PROFILE *profile,
                unsigned long start_x, unsigned long end_x,
                unsigned long start_y, unsigned long end_y);
    ALI_TLIST<ALI_MAP *> *solutions(void) {
        ALI_TLIST<ALI_MAP *> *ret = result.map_list;
        result.map_list = 0;
        return ret;
    }
};



#endif

