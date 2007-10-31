

#ifndef _ALI_PREALIGNER_INC_
#define _ALI_PREALIGNER_INC_

#include "ali_profile.hxx"
#include "ali_pathmap.hxx"
#include "ali_tstack.hxx"
#include "ali_solution.hxx"

#define ALI_PREALIGNER_INS        1
#define ALI_PREALIGNER_SUB        2
#define ALI_PREALIGNER_DEL        3
#define ALI_PREALIGNER_MULTI_FLAG 4



struct ALI_PREALIGNER_CONTEXT {
    long max_number_of_maps;
    long max_number_of_maps_aligner;
    int intervall_border;
    int intervall_center;
    float max_cost_of_sub_percent;
    float max_cost_of_helix;
    unsigned long error_count;
};

/*
 * Structure for a cell in the (simple) distance matrix
 */
struct ali_prealigner_cell {
    float d;

    ali_prealigner_cell(void) {
        d = 0.0;
    }
};

/*
 * Structure for a column in the (simple) distance matrix
 */
struct ali_prealigner_column {
    unsigned long column_length;
    //ali_prealigner_cell (*cells)[];
    ali_prealigner_cell **cells;

    ali_prealigner_column(unsigned long length) {
        column_length = length;
        //cells = (ali_prealigner_cell (*) [0]) calloc((unsigned int) column_length, sizeof(ali_prealigner_cell));
        //cells = (ali_prealigner_cell (*) [1]) calloc((unsigned int) column_length, sizeof(ali_prealigner_cell));
        cells = (ali_prealigner_cell **) calloc((unsigned int) column_length, sizeof(ali_prealigner_cell));
        if (cells == 0)
            ali_fatal_error("Out of memory");
    }
    ~ali_prealigner_column(void) {
        if (cells)
            free((char *) cells);
    }
};

/*
 * Structure for the intersection of maps
 */
struct ali_prealigner_mask {
    ALI_MAP *map;
    float cost_of_binding;

    unsigned long calls, joins, last_new, last_joins;

    ali_prealigner_mask(void) {
        last_new = last_joins = 0;
        map = 0;
        calls = joins = 0;
    }

    void insert(ALI_MAP *in_map, float costs);
    void delete_expensive(ALI_PREALIGNER_CONTEXT *context,ALI_PROFILE *profile);
    void clear(void) {
        last_new = last_joins = 0;
        delete map;
        calls = joins = 0;
    }
};

/*
 * Structure for a approximation of a map
 */
struct ali_prealigner_approx_element {
    ALI_MAP *map;
    char *ins_marker;

    ali_prealigner_approx_element(ALI_MAP *m = 0, char *im = 0) {
        map = m;
        ins_marker = im;
    }
    ~ali_prealigner_approx_element(void) {
        /*
          if (map)
          delete map;
          if (ins_marker)
          free((char *) ins_marker);
        */
    }
    void print(void) {
        printf("map = %p, ins_marker = %p\n",map,ins_marker);
        map->print();
        printf("<%20s>\n",ins_marker);
    }
};

/*
 * Structure for a list of approximated maps
 */
struct ali_prealigner_approximation {
    ALI_TLIST<ali_prealigner_approx_element *> *approx_list;
    float cost_of_binding;

    ali_prealigner_approximation(void) {
        cost_of_binding = 0.0;
        approx_list = new ALI_TLIST<ali_prealigner_approx_element *>;
    }
    ~ali_prealigner_approximation(void) {
        if (approx_list) {
            clear();
            delete approx_list;
        }
    }

    /*
     * Insert a new approximation (dependence of the costs)
     */
    void insert(ALI_MAP *in_map, char *in_insert_marker, float costs) {
        ali_prealigner_approx_element *approx_elem;

        if (approx_list == 0)
            approx_list = new ALI_TLIST<ali_prealigner_approx_element *>;

        if (costs < cost_of_binding || approx_list->is_empty()) {
            cost_of_binding = costs;
            clear();
        }

        if (costs == cost_of_binding) {
            if (approx_list->cardinality() < 20) {
                approx_elem = new ali_prealigner_approx_element(in_map,
                                                                in_insert_marker);

                approx_list->append_end(approx_elem);
            }
            else {
                delete in_map;
                delete in_insert_marker;
            }
        }
        else {
            delete in_map;
            delete in_insert_marker;
        }
    }
    ALI_TLIST<ali_prealigner_approx_element *> *list(void) {
        ALI_TLIST<ali_prealigner_approx_element *> *ret_list;
        ret_list = approx_list;
        approx_list = 0;
        return ret_list;
    }
    void clear(void) {
        if (approx_list) {
            if (!approx_list->is_empty()) {
                delete approx_list->first();
                while (approx_list->is_next())
                    delete approx_list->next();
                approx_list->make_empty();
            }
        }
    }
    void print(void) {
        ali_prealigner_approx_element *approx_elem;
        printf("\nList of Approximations\n");
        printf("cost_of_binding = %f\n",cost_of_binding);
        if (approx_list) {
            if (!approx_list->is_empty()) {
                approx_elem = approx_list->first();
                approx_elem->print();
                while (approx_list->is_next()) {
                    approx_elem = approx_list->next();
                    approx_elem->print();
                }
            }
            else {
                printf("List is empty\n");
            }
        }
        else {
            printf("List not existent\n");
        }
    }
};



/*
 * Class of the prealigner
 */
class ALI_PREALIGNER {
    ALI_PROFILE *profile;
    ALI_PATHMAP *path_map;
    ALI_SUB_SOLUTION *sub_solution;

    ali_prealigner_mask result_mask;
    unsigned long result_mask_counter;
    unsigned long start_x, end_x, start_y, end_y;
    ali_prealigner_approximation result_approx;

    float minimum2(float a, float b);
    float minimum3(float a, float b, float c);

    void calculate_first_column_first_cell(ali_prealigner_cell *akt_cell);
    void calculate_first_column_cell(ali_prealigner_cell *up_cell,
                                     ali_prealigner_cell *akt_cell,
                                     unsigned long pos_y);
    void calculate_first_column(ali_prealigner_column *akt_column);

    void calculate_first_cell(ali_prealigner_cell *left_cell,
                              ali_prealigner_cell *akt_cell,
                              unsigned long pos_x);
    void calculate_cell(
                        ali_prealigner_cell *diag_cell, ali_prealigner_cell *left_cell,
                        ali_prealigner_cell *up_cell, ali_prealigner_cell *akt_cell,
                        unsigned long pos_x, unsigned long pos_y);
    void calculate_column(ali_prealigner_column *prev_col,
                          ali_prealigner_column *akt_col,
                          unsigned long pos_x);

    void calculate_last_column_first_cell(ali_prealigner_cell *left_cell,
                                          ali_prealigner_cell *akt_cell,
                                          unsigned long pos_x);
    void calculate_last_column_cell(
                                    ali_prealigner_cell *diag_cell, ali_prealigner_cell *left_cell,
                                    ali_prealigner_cell *up_cell, ali_prealigner_cell *akt_cell,
                                    unsigned long pos_x, unsigned long pos_y);
    void calculate_last_column(ali_prealigner_column *prev_col,
                               ali_prealigner_column *akt_col,
                               unsigned long pos_x);

    void calculate_matrix(void);

    int find_multiple_path(unsigned long start_x, unsigned long start_y,
                           unsigned long *end_x, unsigned long *end_y);
    void generate_solution(ALI_MAP *map);
    void generate_result_mask(ALI_TSTACK<unsigned char> *stack);
    void mapper_pre(ALI_TSTACK<unsigned char> *stack,
                    unsigned long pos_x, unsigned long pos_y);
    void mapper_post(ALI_TSTACK<unsigned char> *stack,
                     unsigned long pos_x, unsigned long pos_y);
    void mapper_post_multi(ALI_TSTACK<unsigned char> *stack,
                           unsigned long pos_x, unsigned long pos_y);
    void mapper_random(ALI_TSTACK<unsigned char> *stack,
                       unsigned long pos_x, unsigned long pos_y);
    void mapper(ALI_TSTACK<unsigned char> *stack,
                unsigned long pos_x, unsigned long pos_y);
    void quick_mapper(ALI_TSTACK<unsigned char> *stack,
                      unsigned long pos_x, unsigned long pos_y);
    void make_map(void);
    void make_quick_map(void);

    void generate_approximation(ALI_SUB_SOLUTION *work_sol);
    void mapper_approximation(unsigned long area_number,
                              ALI_TARRAY<ALI_TLIST<ALI_MAP *> *> *map_lists,
                              ALI_SUB_SOLUTION *work_sol);
    void make_approximation(ALI_PREALIGNER_CONTEXT *context);

    unsigned long number_of_solutions(void);

public:

    ALI_PREALIGNER(ALI_PREALIGNER_CONTEXT *context, ALI_PROFILE *profile,
                   unsigned long start_x, unsigned long end_x,
                   unsigned long start_y, unsigned long end_y);
    ~ALI_PREALIGNER(void) {
        if (sub_solution)
            delete sub_solution;
    }

    ALI_SEQUENCE *sequence(void) {
        return result_mask.map->sequence(profile->sequence());
    }
    ALI_SEQUENCE *sequence_without_inserts(void) {
        return result_mask.map->sequence_without_inserts(profile->sequence());
    }
    ALI_SUB_SOLUTION *solution(void) {
        ALI_SUB_SOLUTION *ret_sol = sub_solution;
        sub_solution = 0;
        return ret_sol;
    }
    ALI_TLIST<ali_prealigner_approx_element *> *approximation(void) {
        return result_approx.list();
    }
};


#endif

