#ifndef P_
# error P_ is not defined
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* adsort.c */

/* adlang1.c */
GB_ERROR gbl_command P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_count P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_len P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_remove P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_keep P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_echo P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR _gbl_mid P_((int argcinput, GBL *argvinput, int *argcout, GBL **argvout, int start, int mstart, int end, int relend));
GB_ERROR gbl_dd P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_head P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_tail P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_mid0 P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_mid P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_tab P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_cut P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_extract_words P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_extract_sequence P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_check P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_gcgcheck P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_srt P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_calculator P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_readdb P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_sequence P_((GBDATA *gb_item, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_sequence_type P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_format_sequence P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GBDATA *gbl_search_sai_species P_((const char *species, const char *sai));
GB_ERROR gbl_filter P_((GBDATA *gb_spec, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_diff P_((GBDATA *gb_spec, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_change_gc P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
GB_ERROR gbl_exec P_((GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam, GBL *argvparam, int *argcout, GBL **argvout));
void gbl_install_standard_commands P_((GBDATA *gb_main));

/* adstring.c */
GB_ERROR gb_scan_directory P_((char *basename, struct gb_scandir *sd));
void gbs_uppercase P_((char *str));
void gbs_memcopy P_((char *dest, const char *source, long len));
char *gbs_malloc_copy P_((const char *source, long len));
char *gbs_add_path P_((char *path, char *name));
char *gbs_compress_command P_((const char *com));
void gbs_strensure_mem P_((void *strstruct, long len));
GB_ERROR gbs_build_replace_string P_((void *strstruct, char *bar, char *wildcards, long max_wildcard, char **mwildcards, long max_mwildcard, GBDATA *gb_container));
void gbs_regerror P_((int en));
GB_CPNTR gb_compile_regexpr P_((const char *regexprin, char **subsout));

/* arbdb.c */
void gb_init_gb P_((void));
GB_CPNTR gb_increase_buffer P_((long size));
char *gb_check_out_buffer P_((const char *buffer));
GB_ERROR gb_unfold P_((GBCONTAINER *gbd, long deep, int index_pos));
int gb_read_nr P_((GBDATA *gbd));
GB_ERROR gb_write_compressed_pntr P_((GBDATA *gbd, const char *s, long memsize, long stored_size));
int gb_get_compression_mask P_((GB_MAIN_TYPE *Main, GBQUARK key, int gb_type));
GB_ERROR gb_security_error P_((GBDATA *gbd));
GB_CSTR gb_read_key_pntr P_((GBDATA *gbd));
GBQUARK gb_key_2_quark P_((GB_MAIN_TYPE *Main, const char *s));
GBDATA *gb_create P_((GBDATA *father, const char *key, GB_TYPES type));
GBDATA *gb_create_container P_((GBDATA *father, const char *key));
GB_ERROR gb_delete_force P_((GBDATA *source));
GB_ERROR gb_set_compression P_((GBDATA *source));
GB_ERROR gb_init_transaction P_((GBCONTAINER *gbd));
GB_ERROR gb_add_changed_callback_list P_((GBDATA *gbd, struct gb_transaction_save *old, GB_CB_TYPE gbtype, GB_CB func, int *clientdata));
GB_ERROR gb_add_delete_callback_list P_((GBDATA *gbd, struct gb_transaction_save *old, GB_CB func, int *clientdata));
GB_ERROR gb_do_callback_list P_((GBDATA *gbd));
GB_MAIN_TYPE *gb_get_main_during_cb P_((void));
GB_CPNTR gb_read_pntr_ts P_((GBDATA *gbd, struct gb_transaction_save *ts));
int gb_info P_((GBDATA *gbd, int deep));

/* ad_core.c */
void gb_touch_entry P_((GBDATA *gbd, GB_CHANGED val));
void gb_touch_header P_((GBCONTAINER *gbc));
void gb_untouch_children P_((GBCONTAINER *gbc));
void gb_untouch_me P_((GBDATA *gbc));
void gb_set_update_in_server_flags P_((GBCONTAINER *gbc));
void gb_create_header_array P_((GBCONTAINER *gbc, int size));
void gb_link_entry P_((GBCONTAINER *father, GBDATA *gbd, long index_pos));
void gb_unlink_entry P_((GBDATA *gbd));
void gb_create_extended P_((GBDATA *gbd));
struct gb_main_type *gb_make_gb_main_type P_((const char *path));
char *gb_destroy_main P_((struct gb_main_type *Main));
GBDATA *gb_make_pre_defined_entry P_((GBCONTAINER *father, GBDATA *gbd, long index_pos, GBQUARK keyq));
GBDATA *gb_make_entry P_((GBCONTAINER *father, const char *key, long index_pos, GBQUARK keyq, GB_TYPES type));
GBCONTAINER *gb_make_pre_defined_container P_((GBCONTAINER *father, GBCONTAINER *gbd, long index_pos, GBQUARK keyq));
GBCONTAINER *gb_make_container P_((GBCONTAINER *father, const char *key, long index_pos, GBQUARK keyq));
void gb_pre_delete_entry P_((GBDATA *gbd));
void gb_delete_entry P_((GBDATA *gbd));
struct gb_transaction_save *gb_new_gb_transaction_save P_((GBDATA *gbd));
void gb_add_ref_gb_transaction_save P_((struct gb_transaction_save *ts));
void gb_del_ref_gb_transaction_save P_((struct gb_transaction_save *ts));
void gb_del_ref_and_extern_gb_transaction_save P_((struct gb_transaction_save *ts));
void gb_abortdata P_((GBDATA *gbd));
void gb_save_extern_data_in_ts P_((GBDATA *gbd));
char *gb_write_index_key P_((GBCONTAINER *father, long index, GBQUARK new_index));
char *gb_write_key P_((GBDATA *gbd, const char *s));
void gb_create_key_array P_((GB_MAIN_TYPE *Main, int index));
long gb_create_key P_((GB_MAIN_TYPE *Main, const char *s, GB_BOOL create_gb_key));
void gb_free_all_keys P_((GB_MAIN_TYPE *Main));
char *gb_abort_entry P_((GBDATA *gbd));
int gb_abort_transaction_local_rek P_((GBDATA *gbd, long mode));
GB_ERROR gb_commit_transaction_local_rek P_((GBDATA *gbd, long mode, int *pson_created));

/* admath.c */

/* adoptimize.c */
GB_ERROR gb_convert_compression P_((GBDATA *source));
GB_ERROR gb_convert_V2_to_V3 P_((GBDATA *gb_main));
char *gb_uncompress_by_dictionary_internal P_((GB_DICTIONARY *dict, GB_CSTR s_source, long size, GB_BOOL append_zero));
char *gb_uncompress_by_dictionary P_((GBDATA *gbd, GB_CSTR s_source, long size));
char *gb_compress_by_dictionary P_((GB_DICTIONARY *dict, GB_CSTR s_source, long size, long *msize, int last_flag, int search_backward, int search_forward));
GB_ERROR gb_create_dictionaries P_((GB_MAIN_TYPE *Main, long maxmem));

/* adsystem.c */
GB_DICTIONARY *gb_create_dict P_((GBDATA *gb_dict));
void delete_gb_dictionary P_((GB_DICTIONARY *dict));
void gb_system_key_changed_cb P_((GBDATA *gbd, int *cl, GB_CB_TYPE type));
void gb_system_master_changed_cb P_((GBDATA *gbd, int *cl, GB_CB_TYPE type));
void gb_load_single_key_data P_((GBDATA *gb_main, GBQUARK q));
GB_ERROR gb_save_dictionary P_((GBDATA *gb_main, const char *key, const char *dict, int size));
GB_ERROR gb_load_key_data_and_dictionaries P_((GBDATA *gb_main));

/* adindex.c */
char *gb_index_check_in P_((GBDATA *gbd));
char *gb_index_check_out P_((GBDATA *gbd));
GBDATA *gb_index_find P_((GBCONTAINER *gbf, struct gb_index_files_struct *ifs, GBQUARK quark, const char *val, int after_index));
char *gb_set_undo_type P_((GBDATA *gb_main, GB_UNDO_TYPE type));
void gb_init_undo_stack P_((struct gb_main_type *Main));
void gb_free_undo_stack P_((struct gb_main_type *Main));
char *gb_set_undo_sync P_((GBDATA *gb_main));
char *gb_free_all_undos P_((GBDATA *gb_main));
char *gb_disable_undo P_((GBDATA *gb_main));
void gb_check_in_undo_create P_((GB_MAIN_TYPE *Main, GBDATA *gbd));
void gb_check_in_undo_modify P_((GB_MAIN_TYPE *Main, GBDATA *gbd));
void gb_check_in_undo_delete P_((GB_MAIN_TYPE *Main, GBDATA *gbd, int deep));

/* adperl.c */
GB_TYPES GBP_gb_types P_((char *type_name));

/* adlink.c */

/* adsocket.c */
GB_ERROR gbcm_test_address P_((long *address, long key));
long gbcm_test_address_end P_((void));
void *gbcm_sig_violation P_((long sig, long code, long *scpin, char *addr));
void *gbcms_sigpipe P_((void));
void gbcm_read_flush P_((int socket));
long gbcm_read_buffered P_((int socket, char *ptr, long size));
long gbcm_read P_((int socket, char *ptr, long size));
int gbcm_write_flush P_((int socket));
int gbcm_write P_((int socket, const char *ptr, long size));
void *gbcm_sigio P_((void));
GB_ERROR gbcm_get_m_id P_((const char *path, char **m_name, long *id));
GB_ERROR gbcm_open_socket P_((const char *path, long delay2, long do_connect, int *psocket, char **unix_name));
long gbcms_close P_((struct gbcmc_comm *link));
struct gbcmc_comm *gbcmc_open P_((const char *path));
long gbcm_write_two P_((int socket, long a, long c));
long gbcm_read_two P_((int socket, long a, long *b, long *c));
long gbcm_write_string P_((int socket, const char *key));
char *gbcm_read_string P_((int socket));

/* adcomm.c */
void *gbcms_sighup P_((void));
void gbcms_shift_delete_list P_((void *hsi, void *soi));
int gbcms_write_deleted P_((int socket, GBDATA *gbd, long hsin, long client_clock, long *buffer));
int gbcms_write_updated P_((int socket, GBDATA *gbd, long hsin, long client_clock, long *buffer));
int gbcms_write_keys P_((int socket, GBDATA *gbd));
int gbcms_talking_unfold P_((int socket, long *hsin, void *sin, GBDATA *gb_in));
int gbcms_talking_get_update P_((int socket, long *hsin, void *sin, GBDATA *gbd));
int gbcms_talking_put_update P_((int socket, long *hsin, void *sin, GBDATA *gbd_dummy));
int gbcms_talking_updated P_((int socket, long *hsin, void *sin, GBDATA *gbd));
int gbcms_talking_init_transaction P_((int socket, long *hsin, void *sin, GBDATA *gb_dummy));
int gbcms_talking_begin_transaction P_((int socket, long *hsin, void *sin, long client_clock));
int gbcms_talking_commit_transaction P_((int socket, long *hsin, void *sin, GBDATA *gbd));
int gbcms_talking_abort_transaction P_((int socket, long *hsin, void *sin, GBDATA *gbd));
int gbcms_talking_close P_((int socket, long *hsin, void *sin, GBDATA *gbd));
int gbcms_talking_system P_((int socket, long *hsin, void *sin, GBDATA *gbd));
int gbcms_talking_undo P_((int socket, long *hsin, void *sin, GBDATA *gbd));
int gbcms_talking_find P_((int socket, long *hsin, void *sin, GBDATA *gbd));
int gbcms_talking_key_alloc P_((int socket, long *hsin, void *sin, GBDATA *gbd));
int gbcms_talking_disable_wait_for_new_request P_((int socket, long *hsin, void *sin, GBDATA *gbd));
int gbcms_talking P_((int con, long *hs, void *sin));
GB_ERROR gbcm_write_bin P_((int socket, GBDATA *gbd, long *buffer, long mode, long deep, int send_headera));
long gbcm_read_bin P_((int socket, GBCONTAINER *gbd, long *buffer, long mode, GBDATA *gb_source, void *cs_main));
GB_ERROR gbcm_unfold_client P_((GBCONTAINER *gbd, long deep, long index_pos));
GB_ERROR gbcmc_begin_sendupdate P_((GBDATA *gbd));
GB_ERROR gbcmc_end_sendupdate P_((GBDATA *gbd));
GB_ERROR gbcmc_sendupdate_create P_((GBDATA *gbd));
GB_ERROR gbcmc_sendupdate_delete P_((GBDATA *gbd));
GB_ERROR gbcmc_sendupdate_update P_((GBDATA *gbd, int send_headera));
GB_ERROR gbcmc_read_keys P_((int socket, GBDATA *gbd));
GB_ERROR gbcmc_begin_transaction P_((GBDATA *gbd));
GB_ERROR gbcmc_init_transaction P_((GBCONTAINER *gbd));
GB_ERROR gbcmc_commit_transaction P_((GBDATA *gbd));
GB_ERROR gbcmc_abort_transaction P_((GBDATA *gbd));
GB_ERROR gbcms_add_to_delete_list P_((GBDATA *gbd));
GB_ERROR gbcmc_unfold_list P_((int socket, GBDATA *gbd));
long gbcmc_key_alloc P_((GBDATA *gbd, const char *key));
GB_ERROR gbcmc_send_undo_commands P_((GBDATA *gbd, enum gb_undo_commands command));
char *gbcmc_send_undo_info_commands P_((GBDATA *gbd, enum gb_undo_commands command));
GB_ERROR gbcm_login P_((GBCONTAINER *gb_main, const char *user));
long gbcmc_close P_((struct gbcmc_comm *link));
GB_ERROR gbcm_logout P_((GBCONTAINER *gb_main, char *user));

/* adhash.c */
long gbs_hash_to_strstruct P_((const char *key, long val));
long gbs_hashi_index P_((long key, long size));
void gb_init_cache P_((GB_MAIN_TYPE *Main));
char *gb_read_cache P_((GBDATA *gbd));
void *gb_free_cache P_((GB_MAIN_TYPE *Main, GBDATA *gbd));
char *gb_flush_cache P_((GBDATA *gbd));
char *gb_alloc_cache_index P_((GBDATA *gbd, long size));

/* adquery.c */
GBDATA *gb_find_by_nr P_((GBDATA *father, int index));
void gb_init_ctype_table P_((void));
GBDATA *gb_search P_((GBDATA *gbd, const char *str, long create, int internflag));
GBDATA *gb_search_marked P_((GBCONTAINER *gbc, GBQUARK key_quark, int firstindex));
char *gbs_search_second_x P_((const char *str));
char *gbs_search_second_bracket P_((const char *source));
char *gbs_search_next_seperator P_((const char *source, const char *seps));

/* ad_save_load.c */
GB_CPNTR gb_findExtension P_((GB_CSTR path));
GB_CSTR gb_oldQuicksaveName P_((GB_CSTR path, int nr));
GB_CSTR gb_quicksaveName P_((GB_CSTR path, int nr));
GB_CSTR gb_mapfile_name P_((GB_CSTR path));
GB_CSTR gb_overwriteName P_((GB_CSTR path));
GB_CSTR gb_reffile_name P_((GB_CSTR path));
GB_ERROR gb_delete_reference P_((const char *master));
GB_ERROR gb_create_reference P_((const char *master));
GB_ERROR gb_add_reference P_((char *master, char *changes));
GB_ERROR gb_remove_quick_saved P_((GB_MAIN_TYPE *Main, const char *path));
GB_ERROR gb_remove_all_but_main P_((GB_MAIN_TYPE *Main, const char *path));
long gb_ascii_2_bin P_((const char *source, GBDATA *gbd));
GB_CPNTR gb_bin_2_ascii P_((GBDATA *gbd));
long gb_test_sub P_((GBDATA *gbd));
long gb_write_rek P_((FILE *out, GBCONTAINER *gbc, long deep, long big_hunk));
long gb_read_in_long P_((FILE *in, long reversed));
long gb_read_number P_((FILE *in));
void gb_put_number P_((long i, FILE *out));
long gb_read_bin_error P_((FILE *in, GBDATA *gbd, const char *text));
long gb_write_out_long P_((long data, FILE *out));
int gb_is_writeable P_((struct gb_header_list_struct *header, GBDATA *gbd, long version, long diff_save));
int gb_write_bin_sub_containers P_((FILE *out, GBCONTAINER *gbc, long version, long diff_save, int is_root));
long gb_write_bin_rek P_((FILE *out, GBDATA *gbd, long version, long diff_save, long index_of_master_file));
int gb_write_bin P_((FILE *out, GBDATA *gbd, long version));
GB_ERROR gb_check_quick_save P_((GBDATA *gb_main, char *refpath));
char *gb_full_path P_((const char *path));
GB_ERROR gb_check_saveable P_((GBDATA *gbd, const char *path, const char *flags));

/* adcompr.c */
GB_ERROR gb_check_huffmann_tree P_((struct gb_compress_tree *t));
struct gb_compress_tree *gb_build_uncompress_tree P_((const unsigned char *data, long short_flag, char **end));
void gb_free_compress_tree P_((struct gb_compress_tree *tree));
struct gb_compress_list *gb_build_compress_list P_((const unsigned char *data, long short_flag, long *size));
char *gb_compress_bits P_((const char *source, long size, char c_0, long *msize));
GB_CPNTR gb_uncompress_bits P_((const char *source, long size, char c_0, char c_1));
void gb_compress_equal_bytes_2 P_((const char *source, long size, long *msize, char *dest));
GB_CPNTR gb_compress_equal_bytes P_((const char *source, long size, long *msize, int last_flag));
void gb_compress_huffmann_add_to_list P_((long val, struct gb_compress_list *element));
long gb_compress_huffmann_pop P_((long *val, struct gb_compress_list **element));
GB_CPNTR gb_compress_huffmann_rek P_((struct gb_compress_list *bc, int bits, int bitcnt, char *dest));
GB_CPNTR gb_compress_huffmann P_((const char *source, long size, long *msize, int last_flag));
GB_CPNTR gb_uncompress_equal_bytes P_((const char *s, long size));
GB_CPNTR gb_uncompress_huffmann P_((const char *source, long maxsize));
GB_CPNTR gb_uncompress_bytes P_((const char *source, long size));
GB_CPNTR gb_uncompress_longs P_((char *source, long size));
GB_CPNTR gb_uncompress_longsnew P_((const char *data, long size));
GB_CPNTR gb_compress_longs P_((const char *source, long size, int last_flag));
GB_DICTIONARY *gb_get_dictionary P_((GB_MAIN_TYPE *Main, GBQUARK key));
GB_CPNTR gb_compress_data P_((GBDATA *gbd, int key, const char *source, long size, long *msize, GB_COMPRESSION_MASK max_compr, GB_BOOL pre_compressed));
GB_CPNTR gb_uncompress_data P_((GBDATA *gbd, const char *source, long size));

/* admalloc.c */
void gbm_init_mem P_((void));
void gbb_put_memblk P_((char *memblk, size_t size));
char *gbm_get_mem P_((size_t size, long index));
void gbm_free_mem P_((char *data, size_t size, long index));
void gbm_debug_mem P_((GB_MAIN_TYPE *Main));

/* ad_load.c */
long gb_read_ascii P_((const char *path, GBCONTAINER *gbd));
char **gb_read_file P_((const char *path));
void gb_file_loc_error P_((char **pdat, const char *s));
char **gb_read_rek P_((char **pdat, GBCONTAINER *gbd));
long gb_read_bin_rek P_((FILE *in, GBCONTAINER *gbd, long nitems, long version, long reversed));
long gb_recover_corrupt_file P_((GBCONTAINER *gbd, FILE *in));
long gb_read_bin_rek_V2 P_((FILE *in, GBCONTAINER *gbd, long nitems, long version, long reversed, long deep));
GBDATA *gb_search_system_folder_rek P_((GBDATA *gbd));
void gb_search_system_folder P_((GBDATA *gb_main));
long gb_read_bin P_((FILE *in, GBCONTAINER *gbd, int diff_file_allowed));
GB_MAIN_IDX gb_make_main_idx P_((GB_MAIN_TYPE *Main));
GB_ERROR gb_login_remote P_((struct gb_main_type *gb_main, const char *path, const char *opent));

/* admap.c */
int gb_save_mapfile P_((GB_MAIN_TYPE *Main, GB_CSTR path));
int gb_is_valid_mapfile P_((const char *path, struct gb_map_header *mheader));
GBDATA *gb_map_mapfile P_((const char *path));
int gb_isMappedMemory P_((char *mem));

/* adTest.c */
void gb_testDB P_((GBDATA *gbd));

/* adtune.c */

#ifdef __cplusplus
}
#endif
