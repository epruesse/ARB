#ifndef P_
# if defined(__STDC__) || defined(__cplusplus)
#  define P_(s) s
# else
#  define P_(s) ()
# endif
#else
# error P_ already defined elsewhere
#endif


/* ORS_S_main.cxx */
int ors_server_shutdown P_((void));
extern "C" int server_shutdown P_((ORS_main *pm, aisc_string passwd));
extern "C" int server_save P_((ORS_main *, int));
extern "C" bytestring *search_probe P_((ORS_local *locs));
extern "C" char *OS_find_dailypw P_((ORS_local *locs));
void OS_locs_error P_((ORS_local *locs, char *msg));
void OS_locs_error_pointer P_((ORS_local *locs, char *msg));
char *gen_dailypw P_((int debug));
int main P_((int argc, char **argv));

/* ORS_S_userdb.cxx */
GB_ERROR OS_open_userdb P_((void));
extern "C" GB_ERROR OS_save_userdb P_((void));
int OS_can_read_user P_((ORS_local *locs));
int OS_can_read_sel_user P_((ORS_local *locs));
int OS_user_exists_in_userdb P_((char *userpath));
char *OS_new_user P_((ORS_local *locs));
char *OS_update_user P_((ORS_local *locs));
char *OS_update_sel_user P_((ORS_local *locs));
char *OS_change_sel_parent_user P_((ORS_local *locs, char *old_userpath, char *new_userpath));
char *OS_delete_sel_user P_((ORS_local *locs));
int OS_read_user_info_int P_((char *userpath, char *fieldname));
char *OS_read_user_info_string P_((char *userpath, char *fieldname));
char *OS_find_user_and_password P_((char *user, char *password));
GB_ERROR OS_write_user_info_string P_((char *userpath, char *fieldname, char *content));
GB_ERROR OS_write_user_info_int P_((char *userpath, char *fieldname, int content));
GB_ERROR OS_write_gb_user_info_string P_((GBDATA *gb_user, char *fieldname, char *content));
GB_ERROR OS_write_gb_user_info_int P_((GBDATA *gb_user, char *fieldname, int content));
char *OS_read_dailypw_info P_((char *dailypw, char *fieldname));
char *OS_list_of_subusers P_((char *userpath, int levels, char *exclude, char *exclude_from));
char *OS_set_dailypw P_((char *, char *));
char *OS_allowed_to_create_user P_((char *userpath, char *new_son));
char *OS_construct_sel_userpath P_((char *sel_par_userpath, char *sel_user));
long OS_who_loop_all P_((const char *key, long val));
long OS_who_loop_user P_((const char *key, long val));
long OS_who_loop P_((const char *, long val, int mode));
char *OS_who P_((ORS_local *locs, char *userpath));
int OS_user_has_sub_users P_((char *userpath));
void OS_change_curr_user_counts P_((char *sel_userpath, int additor));

/* ORS_S_probedb.cxx */
void OS_read_probedb_fields_into_pdb_list P_((ORS_main *ors_main));
GB_ERROR OS_open_probedb P_((void));
extern "C" GB_ERROR OS_save_probedb P_((void));
char *OS_new_probe P_((ORS_local *locs));
char *OS_update_probe P_((ORS_local *locs));
char *OS_delete_probe P_((ORS_local *locs));
char *OS_read_probe_info P_((char *probe_id, char *fieldname));
GB_ERROR OS_write_gb_probe_info_string P_((GBDATA *gb_probe, char *fieldname, char *content));
char *OS_read_gb_probe_info_string P_((GBDATA *gb_probe, char *fieldname));
OS_pdb_list *OS_next_pdb_list_elem P_((OS_pdb_list *head, OS_pdb_list *elem));
OS_pdb_list *OS_find_pdb_list_elem_by_name P_((OS_pdb_list *head, char *section, char *name));
char *OS_create_new_probe_id P_((char *userpath));
int OS_probe_struct_date_cmp P_((struct OS_probe *elem2, struct OS_probe *elem1));
int OS_probe_struct_author_cmp P_((struct OS_probe *elem1, struct OS_probe *elem2));
bytestring *OS_list_of_probes P_((ORS_local *locs, char *userpath, int list_type, int list_count));
int OS_read_access_allowed P_((char *userpath, char *publicity));
int OS_owners_of_pdb_list_allowed P_((ORS_local *locs, char *mode));
int OS_owner_of_probe P_((ORS_local *locs));
int OS_count_probes_of_author P_((char *userpath));
void OS_probe_user_transfer P_((ORS_local *locs, char *from_userpath, char *to_userpath, char *probe_id));

/* ORS_S_lib.cxx */
extern "C" void write_logfile P_((ORS_local *locs, char *comment));
void quit_with_error P_((char *message));
char *ORS_read_a_line_in_a_file P_((char *filename, char *env));

/* ORS_S_cf.cxx */
int store_query P_((ORS_local *locs));
extern "C" aisc_string calc_dailypw P_((ORS_local *locs));
extern "C" int logout_user P_((ORS_local *locs, char *dailypw));
extern "C" aisc_string read_user_field P_((ORS_local *locs));
extern "C" aisc_string dailypw_2_userpath P_((ORS_local *locs));
extern "C" aisc_string get_sel_userdata P_((ORS_local *locs));
extern "C" aisc_string list_of_users P_((ORS_local *locs));
extern "C" int work_on_user P_((ORS_local *locs, aisc_string));
extern "C" void work_on_sel_user P_((ORS_local *locs, char *action));
extern "C" void save_userdb P_((ORS_local *locs, char *));
extern "C" void work_on_probe P_((ORS_local *locs, char *action));
extern "C" bytestring *get_probe_list P_((ORS_local *locs));
extern "C" void probe_select P_((ORS_local *locs, char *probe_id));
extern "C" bytestring *get_probe_field P_((ORS_local *locs));
extern "C" bytestring *probe_query P_((ORS_local *locs));
extern "C" void put_probe_field P_((ORS_local *locs, char *field_name));
extern "C" void clear_probe_fields P_((ORS_local *locs));
extern "C" void save_probedb P_((ORS_local *locs, char *));
extern "C" void probe_user_transfer P_((ORS_local *locs, char *sel_userpath2));

/* ORS_lib.cxx */
char *ORS_crypt P_((char *password));
char *ORS_export_error P_((char *templat, ...));
char *ORS_sprintf P_((char *templat, ...));
char *ORS_time_and_date_string P_((int type, long time));
int ORS_str_char_count P_((char *str, char seek));
int ORS_strncmp P_((char *str1, char *str2));
int ORS_strncase_tail_cmp P_((char *str1, char *str2));
char *ORS_trim P_((char *str));
int ORS_contains_word P_((char *buffer, char *word, char seperator));
int ORS_seq_matches_target_seq P_((char *seq, char *target, int complemented));
char *ORS_calculate_4gc_2at P_((char *sequence));
char *ORS_calculate_gc_ratio P_((char *sequence));
extern "C" int ORS_strcmp P_((char *str1, char *str2));
int OC_find_next_chr P_((char **start, char **end, char chr));
int ORS_is_parent_of P_((char *user1, char *user2));
int ORS_is_parent_or_equal P_((char *user1, char *user2));
char *ORS_get_parent_of P_((char *userpath));
char *ORS_str_to_upper P_((char *str1));

#undef P_
