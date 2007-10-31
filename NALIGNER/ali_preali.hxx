#include "ali_profile.hxx"

class ALI_PREALI {
private:
    long (*helix)[];
    char (*helix_border)[];
    unsigned long helix_len;

    ALI_PROFILE prof;

    char normalize_char(char in);
    long normalize_sequence(char *gen_seq, char **norm_seq);
    void print_table(char *norm_seq, char *path_matrix, long line, long col);
    int map_path(char *path_matrix, long pcol_len, long seq_map[],
                 long col, long line);
    int prealigner(char *norm_seq, unsigned long norm_seq_len, long **seq_map);
    int get_sequence(char *norm_seq, unsigned long norm_seq_len, long *seq_map,
                     char **prealigned_seq, char **error_seq,
                     float cost_low, float cost_middle, float cost_high);
    int check_helix(char prealigned_seq[], char error_seq[], 
                    unsigned long seq_len, double (*bind_matrix) [5][5]);

    int find_next_helix(char h1[], long h_len, int start_pos,
                        int *helix_nr, int *start, int *end);
    int find_comp_helix(char h1[], long h_len, int start_pos,
                        int helix_nr, int *start, int *end);
    void delete_comp_helix(char h1[], char h2[], long h_len,
                           int start_pos, int end_pos);
    int map_helix(char h2[], long h_len, int start1, int end1,
                  int start2, int end2);
    int make_helix(char *h1, char *h2);
    int make_intervalls(char *prealigned_seq, char *error_seq,
                        unsigned long seq_len,
                        int intervall_border, int intervall_center,
                        char **intervall_seq);

public:
    int insert_reference(char *gen_seq, double weight = 1.0);
    int calculate_costs(double *cost_matrix);
    int prealign_sequence(char *gen_seq, char *helix1, char *helix2,
                          char **prealigned_seq, char **error_seq,
                          char **intervall_seq,
                          float cost_low, float cost_middle, float cost_high,
                          double *bind_matrix,
                          int intervall_border, int intervall_center);
    int get_reference(char **reference);
    int get_helix_border(char **border);
};

