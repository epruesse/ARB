#ifndef P_
# error P_ is not defined
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* adsort.c */
char *GBT_quicksort P_((void **array, long start, long end, gb_compare_two_items_type compare, char *client_data));
char *GB_mergesort P_((void **array, long start, long end, gb_compare_two_items_type compare, char *client_data));

/* adlang1.c */

/* adstring.c */
char *GB_find_latest_file P_((const char *dir, const char *mask));
GB_ERROR GB_export_error P_((const char *templat, ...));
GB_ERROR GB_print_error P_((void));
GB_ERROR GB_get_error P_((void));
GB_CSTR GBS_global_string P_((const char *templat, ...));
char *GBS_string_2_key P_((const char *str));
GB_ERROR GB_check_key P_((const char *key));
GB_ERROR GB_check_link_name P_((const char *key));
GB_ERROR GB_check_hkey P_((const char *key));
char *GBS_strdup P_((const char *s));
GB_CPNTR GBS_find_string P_((const char *str, const char *key, long match_mode));
long GBS_string_scmp P_((const char *str, const char *search, long upper_case));
long GBS_string_cmp P_((const char *str, const char *search, long upper_case));
char *GBS_remove_escape P_((char *com));
void *GBS_stropen P_((long init_size));
char *GBS_strclose P_((void *strstruct, int optimize));
GB_CPNTR GBS_mempntr P_((void *strstruct));
long GBS_memoffset P_((void *strstruct));
void GBS_str_cut_tail P_((void *strstruct, int byte_count));
void GBS_strcat P_((void *strstruct, const char *ptr));
void GBS_strncat P_((void *strstruct, const char *ptr, long len));
void GBS_strnprintf P_((void *strstruct, long len, const char *templat, ...));
void GBS_chrcat P_((void *strstruct, char ch));
void GBS_intcat P_((void *strstruct, long val));
void GBS_floatcat P_((void *strstruct, double val));
char *GBS_string_eval P_((const char *insource, const char *icommand, GBDATA *gb_container));
char *GBS_eval_env P_((const char *p));
char *GBS_read_arb_tcp P_((const char *env));
char *GBS_find_lib_file P_((const char *filename, const char *libprefix));
char **GBS_read_dir P_((const char *dir, const char *filter));
GB_ERROR GBS_free_names P_((char **names));
long GBS_gcgchecksum P_((const char *seq));
long GB_checksum P_((const char *seq, long length, int uppercase, const char *exclude));
long GBS_checksum P_((const char *seq, int uppercase, const char *exclude));
long GB_merge_sort_strcmp P_((void *v0, void *v1, char *not_used));
char *GBS_extract_words P_((const char *source, const char *chars, float minlen, GB_BOOL sort_output));
int GBS_do_core P_((void));
void GB_install_error_handler P_((gb_error_handler_type aw_message));
void GB_internal_error P_((const char *templat, ...));
void GB_warning P_((const char *templat, ...));
void GB_install_warning P_((gb_warning_func_type warn));
int GB_status P_((double val));
void GB_install_status P_((gb_status_func_type func));
int GB_status2 P_((const char *val));
void GB_install_status2 P_((gb_status_func2_type func2));
GB_CPNTR GBS_regsearch P_((const char *in, const char *regexprin));
char *GBS_regreplace P_((const char *in, const char *regexprin, GBDATA *gb_species));
GB_CPNTR GBS_regsearch P_((const char *in, const char *regexprin));
char *GBS_regreplace P_((const char *in, const char *regexprin, GBDATA *gb_species));
char *GBS_merge_tagged_strings P_((const char *s1, const char *tag1, const char *replace1, const char *s2, const char *tag2, const char *replace2));
char *GBS_string_eval_tagged_string P_((GBDATA *gb_main, const char *s, const char *dt, const char *tag, const char *srt, const char *aci, GBDATA *gbd));
char *GB_read_as_tagged_string P_((GBDATA *gbd, const char *tagi));
GBDATA_SET *GB_create_set P_((int size));
void GB_add_set P_((GBDATA_SET *set, GBDATA *item));
void GB_delete_set P_((GBDATA_SET *set));
GB_ERROR GBS_fwrite_string P_((const char *strngi, FILE *out));
char *GBS_fread_string P_((FILE *in, int optimize));
char *GBS_replace_tabs_by_spaces P_((const char *text));

/* arbdb.c */
char *GB_rel P_((void *struct_adress, long rel_adress));
double GB_atof P_((const char *str));
GB_CPNTR GB_give_buffer P_((long size));
int GB_give_buffer_size P_((void));
GB_CPNTR GB_give_buffer2 P_((long size));
GB_CPNTR GB_give_other_buffer P_((const char *buffer, long size));
GB_ERROR GB_close P_((GBDATA *gbd));
GB_ERROR GB_exit P_((GBDATA *gbd));
long GB_read_int P_((GBDATA *gbd));
int GB_read_byte P_((GBDATA *gbd));
double GB_read_float P_((GBDATA *gbd));
long GB_read_count P_((GBDATA *gbd));
long GB_read_memuse P_((GBDATA *gbd));
GB_CPNTR GB_read_pntr P_((GBDATA *gbd));
GB_CPNTR GB_read_char_pntr P_((GBDATA *gbd));
char *GB_read_string P_((GBDATA *gbd));
long GB_read_string_count P_((GBDATA *gbd));
GB_CPNTR GB_read_link_pntr P_((GBDATA *gbd));
char *GB_read_link P_((GBDATA *gbd));
long GB_read_link_count P_((GBDATA *gbd));
long GB_read_bits_count P_((GBDATA *gbd));
GB_CPNTR GB_read_bits_pntr P_((GBDATA *gbd, char c_0, char c_1));
char *GB_read_bits P_((GBDATA *gbd, char c_0, char c_1));
GB_CPNTR GB_read_bytes_pntr P_((GBDATA *gbd));
long GB_read_bytes_count P_((GBDATA *gbd));
char *GB_read_bytes P_((GBDATA *gbd));
GB_CUINT4 *GB_read_ints_pntr P_((GBDATA *gbd));
long GB_read_ints_count P_((GBDATA *gbd));
GB_UINT4 *GB_read_ints P_((GBDATA *gbd));
GB_CFLOAT *GB_read_floats_pntr P_((GBDATA *gbd));
long GB_read_floats_count P_((GBDATA *gbd));
float *GB_read_floats P_((GBDATA *gbd));
char *GB_read_as_string P_((GBDATA *gbd));
GB_ERROR GB_write_byte P_((GBDATA *gbd, int i));
GB_ERROR GB_write_int P_((GBDATA *gbd, long i));
GB_ERROR GB_write_float P_((GBDATA *gbd, double f));
GB_ERROR GB_write_pntr P_((GBDATA *gbd, const char *s, long bytes_size, long stored_size));
GB_ERROR GB_write_string P_((GBDATA *gbd, const char *s));
GB_ERROR GB_write_link P_((GBDATA *gbd, const char *s));
GB_ERROR GB_write_bits P_((GBDATA *gbd, const char *bits, long size, char c_0));
GB_ERROR GB_write_bytes P_((GBDATA *gbd, const char *s, long size));
GB_ERROR GB_write_ints P_((GBDATA *gbd, const GB_UINT4 *i, long size));
GB_ERROR GB_write_floats P_((GBDATA *gbd, const float *f, long size));
GB_ERROR GB_write_as_string P_((GBDATA *gbd, const char *val));
int GB_read_security_write P_((GBDATA *gbd));
int GB_read_security_read P_((GBDATA *gbd));
int GB_read_security_delete P_((GBDATA *gbd));
int GB_get_my_security P_((GBDATA *gbd));
GB_ERROR GB_write_security_write P_((GBDATA *gbd, unsigned long level));
GB_ERROR GB_write_security_read P_((GBDATA *gbd, unsigned long level));
GB_ERROR GB_write_security_delete P_((GBDATA *gbd, unsigned long level));
GB_ERROR GB_write_security_levels P_((GBDATA *gbd, unsigned long readlevel, unsigned long writelevel, unsigned long deletelevel));
GB_ERROR GB_change_my_security P_((GBDATA *gbd, int level, const char *passwd));
GB_ERROR GB_push_my_security P_((GBDATA *gbd));
GB_ERROR GB_pop_my_security P_((GBDATA *gbd));
GB_TYPES GB_read_type P_((GBDATA *gbd));
char *GB_read_key P_((GBDATA *gbd));
GB_CSTR GB_read_key_pntr P_((GBDATA *gbd));
GBQUARK GB_key_2_quark P_((GBDATA *gbd, const char *s));
long GB_read_clock P_((GBDATA *gbd));
long GB_read_transaction P_((GBDATA *gbd));
GBDATA *GB_get_father P_((GBDATA *gbd));
GBDATA *GB_get_root P_((GBDATA *gbd));
GB_BOOL GB_check_father P_((GBDATA *gbd, GBDATA *gb_maybefather));
GBDATA *GB_create P_((GBDATA *father, const char *key, GB_TYPES type));
GBDATA *GB_create_container P_((GBDATA *father, const char *key));
GB_ERROR GB_delete P_((GBDATA *source));
GB_ERROR GB_copy P_((GBDATA *dest, GBDATA *source));
char *GB_get_subfields P_((GBDATA *gbd));
GB_ERROR GB_set_compression P_((GBDATA *gb_main, GB_COMPRESSION_MASK disable_compression));
GB_ERROR GB_set_temporary P_((GBDATA *gbd));
GB_ERROR GB_clear_temporary P_((GBDATA *gbd));
long GB_read_temporary P_((GBDATA *gbd));
GB_ERROR GB_push_local_transaction P_((GBDATA *gbd));
GB_ERROR GB_pop_local_transaction P_((GBDATA *gbd));
GB_ERROR GB_push_transaction P_((GBDATA *gbd));
GB_ERROR GB_pop_transaction P_((GBDATA *gbd));
GB_ERROR GB_begin_transaction P_((GBDATA *gbd));
GB_ERROR GB_no_transaction P_((GBDATA *gbd));
GB_ERROR GB_abort_transaction P_((GBDATA *gbd));
GB_ERROR GB_commit_transaction P_((GBDATA *gbd));
GB_ERROR GB_update_server P_((GBDATA *gbd));
void *GB_read_old_value P_((void));
long GB_read_old_size P_((void));
GB_ERROR GB_add_callback P_((GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata));
GB_ERROR GB_remove_callback P_((GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata));
GB_ERROR GB_ensure_callback P_((GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata));
GB_ERROR GB_release P_((GBDATA *gbd));
int GB_testlocal P_((GBDATA *gbd));
int GB_nsons P_((GBDATA *gbd));
GB_ERROR GB_disable_quicksave P_((GBDATA *gbd, const char *reason));
GB_ERROR GB_resort_data_base P_((GBDATA *gb_main, GBDATA **new_order_list, long listsize));
GB_ERROR GB_resort_system_folder_to_top P_((GBDATA *gb_main));
GB_ERROR GB_write_usr_public P_((GBDATA *gbd, long flags));
long GB_read_usr_public P_((GBDATA *gbd));
long GB_read_usr_private P_((GBDATA *gbd));
GB_ERROR GB_write_usr_private P_((GBDATA *gbd, long ref));
GB_ERROR GB_write_flag P_((GBDATA *gbd, long flag));
int GB_read_flag P_((GBDATA *gbd));
GB_ERROR GB_touch P_((GBDATA *gbd));
GB_ERROR GB_print_debug_information P_((void *dummy, GBDATA *gb_main));
int GB_info P_((GBDATA *gbd));
long GB_number_of_subentries P_((GBDATA *gbd));
long GB_rescan_number_of_subentries P_((GBDATA *gbd));

/* ad_core.c */

/* admath.c */
double GB_log_fak P_((int n));

/* adoptimize.c */
GB_ERROR GB_optimize P_((GBDATA *gb_main));

/* adsystem.c */

/* adindex.c */
GB_ERROR GB_create_index P_((GBDATA *gbd, const char *key, long estimated_size));
GB_ERROR GB_request_undo_type P_((GBDATA *gb_main, GB_UNDO_TYPE type));
GB_UNDO_TYPE GB_get_requested_undo_type P_((GBDATA *gb_main));
GB_ERROR GB_undo P_((GBDATA *gb_main, GB_UNDO_TYPE type));
char *GB_undo_info P_((GBDATA *gb_main, GB_UNDO_TYPE type));
GB_ERROR GB_set_undo_mem P_((GBDATA *gbd, long memsize));

/* adperl.c */
GB_UNDO_TYPE GBP_undo_type P_((char *type));
int GBP_search_mode P_((char *search_mode));
const char *GBP_type_to_string P_((GB_TYPES type));
GB_TYPES GBP_gb_types P_((char *type_name));
GB_UNDO_TYPE GBP_undo_types P_((const char *type_name));
const char *GBP_undo_type_2_string P_((GB_UNDO_TYPE type));

/* adlink.c */
GBDATA *GB_follow_link P_((GBDATA *gb_link));
GB_ERROR GB_install_link_follower P_((GBDATA *gb_main, const char *link_type, GB_Link_Follower link_follower));

/* adsocket.c */
void GB_usleep P_((long usec));
void GB_usleep P_((long usec));
void GB_usleep P_((long usec));
GB_ULONG GB_time_of_file P_((const char *path));
long GB_size_of_file P_((const char *path));
long GB_mode_of_file P_((const char *path));
long GB_mode_of_link P_((const char *path));
int GB_is_regularfile P_((const char *path));
long GB_getuid P_((void));
long GB_getpid P_((void));
long GB_getuid_of_file P_((char *path));
int GB_unlink P_((const char *path));
char *GB_follow_unix_link P_((const char *path));
GB_ERROR GB_symlink P_((const char *name1, const char *name2));
GB_ERROR GB_set_mode_of_file P_((const char *path, long mode));
GB_ERROR GB_rename P_((const char *oldpath, const char *newpath));
char *GB_read_file P_((const char *path));
char *GB_map_FILE P_((FILE *in, int writeable));
char *GB_map_file P_((const char *path, int writeable));
long GB_size_of_FILE P_((FILE *in));
GB_ULONG GB_time_of_day P_((void));
long GB_last_saved_clock P_((GBDATA *gb_main));
GB_ULONG GB_last_saved_time P_((GBDATA *gb_main));
void GB_edit P_((const char *path));
void GB_textprint P_((const char *path));
GB_CSTR GB_getcwd P_((void));
void GB_xterm P_((void));
void GB_xcmd P_((const char *cmd, GB_BOOL background));
GB_CSTR GB_getenvUSER P_((void));
GB_CSTR GB_getenvHOME P_((void));
GB_CSTR GB_getenvARBHOME P_((void));
GB_CSTR GB_getenvARBMACROHOME P_((void));
GB_CSTR GB_getenvGS P_((void));
GB_CSTR GB_getenv P_((const char *env));
int GB_host_is_local P_((const char *hostname));
GB_ULONG GB_get_physical_memory P_((void));

/* adcomm.c */
GB_ERROR GBCMS_open P_((const char *path, long timeout, GBDATA *gb_main));
GB_ERROR GBCMS_shutdown P_((GBDATA *gbd));
GB_BOOL GBCMS_accept_calls P_((GBDATA *gbd, GB_BOOL wait_extra_time));
long GB_read_clients P_((GBDATA *gbd));
GBDATA *GBCMC_find P_((GBDATA *gbd, const char *key, const char *str, enum gb_search_types gbs));
int GBCMC_system P_((GBDATA *gbd, const char *ss));
GB_ERROR GB_tell_server_dont_wait P_((GBDATA *gbd));
GB_CSTR GBC_get_hostname P_((void));
GB_ERROR GB_install_pid P_((int mode));

/* adhash.c */
GB_HASH *GBS_create_hash P_((long size, int upper_case));
char *GBS_hashtab_2_string P_((GB_HASH *hash));
char *GBS_string_2_hashtab P_((GB_HASH *hash, char *data));
long GBS_read_hash P_((GB_HASH *hs, const char *key));
long GBS_write_hash P_((GB_HASH *hs, const char *key, long val));
long GBS_write_hash_no_strdup P_((GB_HASH *hs, char *key, long val));
long GBS_incr_hash P_((GB_HASH *hs, const char *key));
void GBS_free_hash_entries P_((GB_HASH *hs));
void GBS_free_hash P_((GB_HASH *hs));
void GBS_free_hash_entries_free_pointer P_((GB_HASH *hs));
void GBS_free_hash_free_pointer P_((GB_HASH *hs));
void GBS_hash_do_loop P_((GB_HASH *hs, gb_hash_loop_type func));
void GBS_hash_do_loop2 P_((GB_HASH *hs, gb_hash_loop_type2 func, void *parameter));
void GBS_hash_next_element P_((GB_HASH *hs, const char **key, long *val));
void GBS_hash_first_element P_((GB_HASH *hs, const char **key, long *val));
void GBS_hash_do_sorted_loop P_((GB_HASH *hs, gb_hash_loop_type func, gbs_hash_sort_func_type sorter));
long GBS_create_hashi P_((long size));
long GBS_read_hashi P_((long hashi, long key));
long GBS_write_hashi P_((long hashi, long key, long val));
long GBS_free_hashi P_((long hash));
char *GB_set_cache_size P_((GBDATA *gbd, long size));

/* adquery.c */
GBDATA *GB_find_sub_by_quark P_((GBDATA *father, int key_quark, const char *val, GBDATA *after));
GBDATA *GB_find_sub_sub_by_quark P_((GBDATA *father, const char *key, int sub_key_quark, const char *val, GBDATA *after));
GBDATA *GB_find P_((GBDATA *gbd, const char *key, const char *str, long gbs));
char *GB_first_non_key_char P_((const char *str));
GBDATA *GB_search P_((GBDATA *gbd, const char *str, long create));
GBDATA *GB_search_last_son P_((GBDATA *gbd));
long GB_number_of_marked_subentries P_((GBDATA *gbd));
GBDATA *GB_first_marked P_((GBDATA *gbd, const char *keystring));
GBDATA *GB_next_marked P_((GBDATA *gbd, const char *keystring));
void GB_install_command_table P_((GBDATA *gb_main, struct GBL_command_table *table));
char *GB_command_interpreter P_((GBDATA *gb_main, const char *str, const char *commands, GBDATA *gbd));

/* ad_save_load.c */
GB_ERROR GB_save P_((GBDATA *gb, const char *path, const char *savetype));
GB_ERROR GB_save_in_home P_((GBDATA *gb, const char *path, const char *savetype));
GB_ERROR GB_save_as P_((GBDATA *gb, const char *path, const char *savetype));
GB_ERROR GB_delete_database P_((GB_CSTR filename));
GB_ERROR GB_save_quick_as P_((GBDATA *gb_main, char *path));
GB_ERROR GB_save_quick P_((GBDATA *gb, char *refpath));
GB_ERROR GB_disable_path P_((GBDATA *gbd, const char *path));

/* adcompr.c */

/* admalloc.c */
void *GB_calloc P_((unsigned int nelem, unsigned int elsize));
char *GB_strdup P_((const char *p));
void *GB_recalloc P_((void *ptr, unsigned int oelem, unsigned int nelem, unsigned int elsize));
void GB_memerr P_((void));

/* ad_load.c */
void GB_set_next_main_idx P_((long idx));
GBDATA *GB_login P_((const char *path, const char *opent, const char *user));
GBDATA *GB_open P_((const char *path, const char *opent));
void GB_set_verbose P_((void));

/* admap.c */

/* adTest.c */
void GB_dump P_((GBDATA *gbd));
char *GB_ralfs_test P_((GBDATA *gb_main));
char *GB_ralfs_menupoint P_((GBDATA *main_data));

/* adtune.c */

#ifdef __cplusplus
}
#endif
#ifndef P_
# error P_ is not defined
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* adtools.c */
char **GBT_get_alignment_names P_((GBDATA *gbd));
GB_ERROR GBT_check_alignment_name P_((const char *alignment_name));
GBDATA *GBT_create_alignment P_((GBDATA *gbd, const char *name, long len, long aligned, long security, const char *type));
GB_ERROR GBT_check_alignment P_((GBDATA *Main, GBDATA *preset_alignment));
GB_ERROR GBT_rename_alignment P_((GBDATA *gbd, const char *source, const char *dest, int copy, int dele));
GBDATA *GBT_find_or_create P_((GBDATA *Main, const char *key, long delete_level));
GB_ERROR GBT_check_data P_((GBDATA *Main, char *alignment_name));
GB_ERROR GBT_check_lengths P_((GBDATA *Main, const char *alignment_name));
GB_ERROR GBT_insert_character P_((GBDATA *Main, char *alignment_name, long pos, long count, char *char_delete));
GB_ERROR GBT_delete_tree P_((GBT_TREE *tree));
GB_ERROR GBT_write_tree P_((GBDATA *gb_main, GBDATA *gb_tree, char *tree_name, GBT_TREE *tree));
GB_ERROR GBT_write_tree_rem P_((GBDATA *gb_main, const char *tree_name, const char *remark));
GBT_TREE *GBT_read_tree_and_size P_((GBDATA *gb_main, const char *tree_name, long structure_size, int *tree_size));
GBT_TREE *GBT_read_tree P_((GBDATA *gb_main, const char *tree_name, long structure_size));
GB_ERROR GBT_link_tree P_((GBT_TREE *tree, GBDATA *gb_main, GB_BOOL show_status));
GBT_TREE *GBT_load_tree P_((char *path, int structuresize));
GBDATA *GBT_get_tree P_((GBDATA *gb_main, const char *tree_name));
long GBT_size_of_tree P_((GBDATA *gb_main, const char *tree_name));
char *GBT_find_largest_tree P_((GBDATA *gb_main));
char *GBT_find_latest_tree P_((GBDATA *gb_main));
char *GBT_tree_info_string P_((GBDATA *gb_main, const char *tree_name));
GB_ERROR GBT_check_tree_name P_((const char *tree_name));
char **GBT_get_tree_names P_((GBDATA *Main));
char *GBT_get_next_tree_name P_((GBDATA *gb_main, const char *tree));
GB_ERROR GBT_free_names P_((char **names));
GB_CSTR *GBT_get_species_names_of_tree P_((GBT_TREE *tree));
char *GBT_existing_tree P_((GBDATA *Main, const char *tree));
GB_ERROR GBT_export_tree P_((GBDATA *gb_main, FILE *out, GBT_TREE *tree, GB_BOOL triple_root));
GBDATA *GBT_create_species P_((GBDATA *gb_main, const char *name));
GBDATA *GBT_create_species_rel_species_data P_((GBDATA *gb_species_data, const char *name));
GBDATA *GBT_create_SAI P_((GBDATA *gb_main, const char *name));
GBDATA *GBT_add_data P_((GBDATA *species, const char *ali_name, const char *key, GB_TYPES type));
GB_ERROR GBT_write_sequence P_((GBDATA *gb_data, const char *ali_name, long ali_len, const char *sequence));
GBDATA *GBT_gen_accession_number P_((GBDATA *gb_species, const char *ali_name));
GBDATA *GBT_first_marked_species_rel_species_data P_((GBDATA *gb_species_data));
GBDATA *GBT_first_marked_species P_((GBDATA *gb_main));
GBDATA *GBT_next_marked_species P_((GBDATA *gb_species));
GBDATA *GBT_first_species_rel_species_data P_((GBDATA *gb_species_data));
GBDATA *GBT_first_species P_((GBDATA *gb_main));
GBDATA *GBT_next_species P_((GBDATA *gb_species));
GBDATA *GBT_find_species_rel_species_data P_((GBDATA *gb_species_data, const char *name));
GBDATA *GBT_find_species P_((GBDATA *gb_main, const char *name));
GBDATA *GBT_first_marked_extended P_((GBDATA *gb_extended_data));
GBDATA *GBT_next_marked_extended P_((GBDATA *gb_extended));
GBDATA *GBT_first_SAI P_((GBDATA *gb_main));
GBDATA *GBT_next_SAI P_((GBDATA *gb_extended));
GBDATA *GBT_find_SAI P_((GBDATA *gb_main, const char *name));
GBDATA *GBT_first_SAI_rel_exdata P_((GBDATA *gb_extended_data));
GBDATA *GBT_find_SAI_rel_exdata P_((GBDATA *gb_extended_data, const char *name));
char *GBT_create_unique_species_name P_((GBDATA *gb_main, const char *default_name));
GBDATA *GBT_find_configuration P_((GBDATA *gb_main, const char *name));
GBDATA *GBT_create_configuration P_((GBDATA *gb_main, const char *name));
void GBT_mark_all P_((GBDATA *gb_main, int flag));
long GBT_count_marked_species P_((GBDATA *gb_main));
long GBT_count_species P_((GBDATA *gb_main));
long GBT_recount_species P_((GBDATA *gb_main));
GBDATA *GBT_read_sequence P_((GBDATA *gb_species, const char *use));
char *GBT_read_name P_((GBDATA *gb_species));
char *GBT_get_default_alignment P_((GBDATA *gb_main));
GB_ERROR GBT_set_default_alignment P_((GBDATA *gb_main, const char *alignment_name));
char *GBT_get_default_helix P_((GBDATA *gb_main));
char *GBT_get_default_helix_nr P_((GBDATA *gb_main));
char *GBT_get_default_ref P_((GBDATA *gb_main));
GBDATA *GBT_get_alignment P_((GBDATA *gb_main, const char *use));
long GBT_get_alignment_len P_((GBDATA *gb_main, const char *use));
GB_ERROR GBT_set_alignment_len P_((GBDATA *gb_main, const char *use, long new_len));
int GBT_get_alignment_aligned P_((GBDATA *gb_main, const char *use));
char *GBT_get_alignment_type_string P_((GBDATA *gb_main, const char *use));
GB_alignment_type GBT_get_alignment_type P_((GBDATA *gb_main, const char *use));
GB_BOOL GBT_is_alignment_protein P_((GBDATA *gb_main, const char *alignment_name));
GB_ERROR GBT_check_arb_file P_((const char *name));
char **GBT_scan_db P_((GBDATA *gbd));
GB_ERROR GBT_message P_((GBDATA *gb_main, const char *msg));
GB_HASH *GBT_generate_species_hash P_((GBDATA *gb_main, int ncase));
GB_HASH *GBT_generate_marked_species_hash P_((GBDATA *gb_main));
GB_ERROR GBT_begin_rename_session P_((GBDATA *gb_main, int all_flag));
GB_ERROR GBT_rename_species P_((const char *oldname, const char *newname));
GB_ERROR GBT_abort_rename_session P_((void));
GB_ERROR GBT_commit_rename_session P_((int (*show_status )(double gauge )));
GBDATA **GBT_gen_species_array P_((GBDATA *gb_main, long *pspeccnt));
char *GBT_read_string P_((GBDATA *gb_main, const char *awar_name));
long GBT_read_int P_((GBDATA *gb_main, const char *awar_name));
double GBT_read_float P_((GBDATA *gb_main, const char *awar_name));
GBDATA *GBT_search_string P_((GBDATA *gb_main, const char *awar_name, const char *def));
GBDATA *GBT_search_int P_((GBDATA *gb_main, const char *awar_name, int def));
GBDATA *GBT_search_float P_((GBDATA *gb_main, const char *awar_name, double def));
char *GBT_read_string2 P_((GBDATA *gb_main, const char *awar_name, const char *def));
long GBT_read_int2 P_((GBDATA *gb_main, const char *awar_name, long def));
double GBT_read_float2 P_((GBDATA *gb_main, const char *awar_name, double def));
GB_ERROR GBT_write_string P_((GBDATA *gb_main, const char *awar_name, const char *def));
GB_ERROR GBT_write_int P_((GBDATA *gb_main, const char *awar_name, long def));
GB_ERROR GBT_write_float P_((GBDATA *gb_main, const char *awar_name, double def));
GBDATA *GB_test_link_follower P_((GBDATA *gb_main, GBDATA *gb_link, const char *link));
GBDATA *GBT_open P_((const char *path, const char *opent, const char *disabled_path));
GB_ERROR GBT_remote_action P_((GBDATA *gb_main, const char *application, const char *action_name));
GB_ERROR GBT_remote_awar P_((GBDATA *gb_main, const char *application, const char *awar_name, const char *value));

/* adseqcompr.c */
GB_ERROR GBT_compress_sequence_tree P_((GBDATA *gb_main, GB_CTREE *tree, const char *ali_name));
GB_ERROR GBT_compress_sequence_tree2 P_((GBDATA *gb_main, const char *tree_name, const char *ali_name));
void GBT_compression_test P_((void *dummy, GBDATA *gb_main));

/* adtables.c */
GB_ERROR GBT_install_table_link_follower P_((GBDATA *gb_main));
GBDATA *GBT_open_table P_((GBDATA *gb_table_root, const char *table_name, GB_BOOL read_only));
GBDATA *GBT_first_table P_((GBDATA *gb_main));
GBDATA *GBT_next_table P_((GBDATA *gb_table));
GBDATA *GBT_first_table_entry P_((GBDATA *gb_table));
GBDATA *GBT_first_marked_table_entry P_((GBDATA *gb_table));
GBDATA *GBT_next_table_entry P_((GBDATA *gb_table_entry));
GBDATA *GBT_next_marked_table_entry P_((GBDATA *gb_table_entry));
GBDATA *GBT_find_table_entry P_((GBDATA *gb_table, const char *id));
GBDATA *GBT_open_table_entry P_((GBDATA *gb_table, const char *id));
GBDATA *GBT_first_table_field P_((GBDATA *gb_table));
GBDATA *GBT_first_marked_table_field P_((GBDATA *gb_table));
GBDATA *GBT_next_table_field P_((GBDATA *gb_table_field));
GBDATA *GBT_next_marked_table_field P_((GBDATA *gb_table_field));
GBDATA *GBT_find_table_field P_((GBDATA *gb_table, const char *id));
GB_TYPES GBT_get_type_of_table_entry_field P_((GBDATA *gb_table, const char *fieldname));
GB_ERROR GBT_savely_write_table_entry_field P_((GBDATA *gb_table, GBDATA *gb_entry, const char *fieldname, const char *value_in_ascii_format));
GBDATA *GBT_open_table_field P_((GBDATA *gb_table, const char *fieldname, GB_TYPES type_of_field));

#ifdef __cplusplus
}
#endif
