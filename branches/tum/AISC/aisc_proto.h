#ifndef P_
#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif
#endif


/* aisc.c */
char *read_aisc_file P_((char *path));
void aisc_init P_((void));
void p_err_eof P_((void));
void p_error_brih P_((void));
void p_error_nobr P_((void));
void p_error_nocbr P_((void));
void p_error_emwbr P_((void));
void p_error_hewnoid P_((void));
void p_error_mixhnh P_((void));
void p_error_misscom P_((void));
void p_error_missco P_((void));
void p_error_exp_string P_((void));
char *read_aisc_string P_((char **in, int *is_m));
AD *make_AD P_((void));
HS *make_HS P_((void));
CL *make_CL P_((void));
AD *read_aisc_line P_((char **in, HS **hs));
AD *read_aisc P_((char **in));
CL *read_prog P_((char **in, char *file));
int main P_((int argc, char **argv));

/* aisc_commands.c */
int print_error P_((const char *err));
void memcopy P_((char *dest, const char *source, int len));
char *find_string P_((const char *str, const char *key));
char *calc_rest_line P_((char *str, int size, int presize));
int calc_line P_((char *str, char *buf));
int calc_line2 P_((char *str, char *buf));
void write_aisc P_((AD *ad, FILE *out, int deep));
void write_prg P_((CL *cl, FILE *out, int deep));
int do_com_dbg P_((char *str));
int do_com_data P_((char *str));
int do_com_write P_((FILE *out, char *str));
int do_com_print P_((char *str));
int do_com_print2 P_((char *str));
int do_com_tabstop P_((char *str));
int do_com_tab P_((char *str));
int do_com_error P_((char *str));
int do_com_open P_((char *str));
void aisc_remove_files P_((void));
int do_com_close P_((char *str));
int do_com_out P_((char *str));
int do_com_moveto P_((char *str));
int do_com_set P_((char *str));
int do_com_create P_((char *str));
int do_com_if P_((char *str));
int do_com_for_add P_((CL *co));
int do_com_for_sub P_((CL *co));
int do_com_push P_((const char *str));
int do_com_pop P_((const char *str));
int do_com_gosub P_((char *str));
int do_com_goto P_((char *str));
int do_com_return P_((char *str));
int do_com_exit P_((char *str));
int do_com_for P_((char *str));
int do_com_next P_((const char *str));
int run_prg P_((void));

/* aisc_mix.c */
CL *aisc_calc_blocks P_((CL *co, CL *afor, CL *aif, int up));
int aisc_calc_special_commands P_((void));
int hash_index P_((const char *key, int size));
struct hash_struct *create_hash P_((int size));
char *read_hash_local P_((char *key, struct hash_struct **hs));
char *read_hash P_((struct hash_struct *hs, char *key));
char *write_hash P_((struct hash_struct *hs, const char *key, const char *val));
int free_hash P_((struct hash_struct *hs));

/* aisc_var_ref.c */
AD *aisc_match P_((AD *var, char *varid, char *varct));
AD *aisc_find_var_hier P_((AD *cursor, char *str, int next, int extended, int goup));
AD *aisc_find_var P_((AD *cursor, char *str, int next, int extended, int goup));
char *get_var_string P_((char *var));

#undef P_
