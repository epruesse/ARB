/* Internal database interface.
 *
 * This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef GB_PROT_H
#define GB_PROT_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* adTest.cxx */
void gb_testDB(GBDATA *gbd);

/* ad_load.cxx */
GB_ERROR gb_read_ascii(const char *path, GBCONTAINER *gbd);
long gb_read_bin_rek(FILE *in, GBCONTAINER *gbd, long nitems, long version, long reversed);
long gb_recover_corrupt_file(GBCONTAINER *gbd, FILE *in, GB_ERROR recovery_reason);
long gb_read_bin_rek_V2(FILE *in, GBCONTAINER *gbd, long nitems, long version, long reversed, long deep);
GBDATA *gb_search_system_folder_rek(GBDATA *gbd);
void gb_search_system_folder(GBDATA *gb_main);
long gb_read_bin(FILE *in, GBCONTAINER *gbd, int diff_file_allowed);
GB_MAIN_IDX gb_make_main_idx(GB_MAIN_TYPE *Main);
void gb_release_main_idx(GB_MAIN_TYPE *Main);
GB_ERROR gb_login_remote(GB_MAIN_TYPE *Main, const char *path, const char *opent);

/* ad_save_load.cxx */
char *gb_findExtension(char *path);
GB_CSTR gb_oldQuicksaveName(GB_CSTR path, int nr);
GB_CSTR gb_quicksaveName(GB_CSTR path, int nr);
GB_CSTR gb_mapfile_name(GB_CSTR path);
GB_CSTR gb_overwriteName(GB_CSTR path);
GB_CSTR gb_reffile_name(GB_CSTR path);
GB_ERROR gb_delete_reference(const char *master);
GB_ERROR gb_create_reference(const char *master);
GB_ERROR gb_add_reference(const char *master, const char *changes);
GB_ERROR gb_remove_all_but_main(GB_MAIN_TYPE *Main, const char *path);
long gb_ascii_2_bin(const char *source, GBDATA *gbd);
GB_BUFFER gb_bin_2_ascii(GBDATA *gbd);
long gb_read_in_long(FILE *in, long reversed);
long gb_read_number(FILE *in);
void gb_put_number(long i, FILE *out);
long gb_read_bin_error(FILE *in, GBDATA *gbd, const char *text);
long gb_write_out_long(long data, FILE *out);
int gb_is_writeable(gb_header_list *header, GBDATA *gbd, long version, long diff_save);
int gb_write_bin_sub_containers(FILE *out, GBCONTAINER *gbc, long version, long diff_save, int is_root);
long gb_write_bin_rek(FILE *out, GBDATA *gbd, long version, long diff_save, long index_of_master_file);
int gb_write_bin(FILE *out, GBDATA *gbd, long version);
char *gb_full_path(const char *path);
GB_ERROR gb_check_saveable(GBDATA *gbd, const char *path, const char *flags);

/* adcomm.cxx */
GB_ERROR gbcm_unfold_client(GBCONTAINER *gbd, long deep, long index_pos) __ATTR__USERESULT;
GB_ERROR gbcmc_begin_sendupdate(GBDATA *gbd);
GB_ERROR gbcmc_end_sendupdate(GBDATA *gbd);
GB_ERROR gbcmc_sendupdate_create(GBDATA *gbd);
GB_ERROR gbcmc_sendupdate_delete(GBDATA *gbd);
GB_ERROR gbcmc_sendupdate_update(GBDATA *gbd, int send_headera);
GB_ERROR gbcmc_begin_transaction(GBDATA *gbd);
GB_ERROR gbcmc_init_transaction(GBCONTAINER *gbd);
GB_ERROR gbcmc_commit_transaction(GBDATA *gbd);
GB_ERROR gbcmc_abort_transaction(GBDATA *gbd);
GB_ERROR gbcms_add_to_delete_list(GBDATA *gbd);
long gbcmc_key_alloc(GBDATA *gbd, const char *key);
GB_ERROR gbcmc_send_undo_commands(GBDATA *gbd, enum gb_undo_commands command) __ATTR__USERESULT;
char *gbcmc_send_undo_info_commands(GBDATA *gbd, enum gb_undo_commands command);
GB_ERROR gbcm_login(GBCONTAINER *gb_main, const char *loginname);
GBCM_ServerResult gbcmc_close(gbcmc_comm *link);
GB_ERROR gbcm_logout(GB_MAIN_TYPE *Main, const char *loginname);

/* adhash.cxx */
size_t gbs_get_a_prime(size_t above_or_equal_this);

/* adcache.cxx */
void gb_init_cache(GB_MAIN_TYPE *Main);
void gb_destroy_cache(GB_MAIN_TYPE *Main);
char *gb_read_cache(GBDATA *gbd);
void gb_free_cache(GB_MAIN_TYPE *Main, GBDATA *gbd);
char *gb_alloc_cache_index(GBDATA *gbd, size_t size);
void gb_flush_cache(GBDATA *gbd);

/* adlang1.cxx */
void gbl_install_standard_commands(GBDATA *gb_main);

/* admalloc.cxx */
void gbm_flush_mem(void);
void gbm_init_mem(void);
void gbm_debug_mem(void);

/* adoptimize.cxx */
GB_ERROR gb_convert_V2_to_V3(GBDATA *gb_main);
char *gb_uncompress_by_dictionary(GBDATA *gbd, GB_CSTR s_source, long size, long *new_size);
char *gb_compress_by_dictionary(GB_DICTIONARY *dict, GB_CSTR s_source, long size, long *msize, int last_flag, int search_backward, int search_forward);
GB_ERROR gb_create_dictionaries(GB_MAIN_TYPE *Main, long maxmem);

/* adstring.cxx */
void gbs_uppercase(char *str);
void gbs_memcopy(char *dest, const char *source, long len);
char *gbs_add_path(char *path, char *name);

/* adfile.cxx */
GB_ERROR gb_scan_directory(char *basename, gb_scandir *sd) __ATTR__USERESULT_TODO;

/* adsystem.cxx */
GB_ERROR gb_load_dictionary_data(GBDATA *gb_main, const char *key, char **dict_data, long *size);
GB_DICTIONARY *gb_create_dict(GBDATA *gb_dict);
void gb_system_key_changed_cb(GBDATA *gbd, int *cl, GB_CB_TYPE type);
void gb_system_master_changed_cb(GBDATA *gbd, int *cl, GB_CB_TYPE type);
void gb_load_single_key_data(GBDATA *gb_main, GBQUARK q);
GB_ERROR gb_save_dictionary_data(GBDATA *gb_main, const char *key, const char *dict, int size);
GB_ERROR gb_load_key_data_and_dictionaries(GBDATA *gb_main) __ATTR__USERESULT;

/* arbdb.cxx */
GBDATA *gb_remembered_db(void);
GB_ERROR gb_unfold(GBCONTAINER *gbd, long deep, int index_pos);
void gb_close_unclosed_DBs(void);
int gb_read_nr(GBDATA *gbd);
GB_ERROR gb_write_compressed_pntr(GBDATA *gbd, const char *s, long memsize, long stored_size);
int gb_get_compression_mask(GB_MAIN_TYPE *Main, GBQUARK key, int gb_type);
GB_ERROR gb_security_error(GBDATA *gbd) __ATTR__USERESULT;
GB_CSTR gb_read_key_pntr(GBDATA *gbd);
GBQUARK gb_key_2_existing_quark(GB_MAIN_TYPE *Main, const char *key);
GBQUARK gb_key_2_quark(GB_MAIN_TYPE *Main, const char *key);
GBDATA *gb_create(GBDATA *father, const char *key, GB_TYPES type);
GBDATA *gb_create_container(GBDATA *father, const char *key);
void gb_rename(GBCONTAINER *gbc, const char *new_key);
GB_ERROR gb_delete_force(GBDATA *source);
GB_ERROR gb_set_compression(GBDATA *source);
GB_ERROR gb_init_transaction(GBCONTAINER *gbd);
void gb_add_changed_callback_list(GBDATA *gbd, gb_transaction_save *old, GB_CB_TYPE gbtype, GB_CB func, int *clientdata);
void gb_add_delete_callback_list(GBDATA *gbd, gb_transaction_save *old, GB_CB func, int *clientdata);
GB_ERROR gb_do_callback_list(GB_MAIN_TYPE *Main);
GB_MAIN_TYPE *gb_get_main_during_cb(void);
GB_CSTR gb_read_pntr_ts(GBDATA *gbd, gb_transaction_save *ts);
int gb_info(GBDATA *gbd, int deep);

/* ad_core.cxx */
void gb_touch_entry(GBDATA *gbd, GB_CHANGE val);
void gb_touch_header(GBCONTAINER *gbc);
void gb_untouch_children(GBCONTAINER *gbc);
void gb_untouch_me(GBDATA *gbc);
void gb_set_update_in_server_flags(GBCONTAINER *gbc);
void gb_create_header_array(GBCONTAINER *gbc, int size);
void gb_link_entry(GBCONTAINER *father, GBDATA *gbd, long index_pos);
void gb_unlink_entry(GBDATA *gbd);
void gb_create_extended(GBDATA *gbd);
GB_MAIN_TYPE *gb_make_gb_main_type(const char *path);
char *gb_destroy_main(GB_MAIN_TYPE *Main);
GBDATA *gb_make_pre_defined_entry(GBCONTAINER *father, GBDATA *gbd, long index_pos, GBQUARK keyq);
void gb_rename_entry(GBCONTAINER *gbc, const char *new_key);
GBDATA *gb_make_entry(GBCONTAINER *father, const char *key, long index_pos, GBQUARK keyq, GB_TYPES type);
GBCONTAINER *gb_make_pre_defined_container(GBCONTAINER *father, GBCONTAINER *gbd, long index_pos, GBQUARK keyq);
GBCONTAINER *gb_make_container(GBCONTAINER *father, const char *key, long index_pos, GBQUARK keyq);
void gb_pre_delete_entry(GBDATA *gbd);
void gb_delete_entry(GBDATA **gbd_ptr);
void gb_delete_entry(GBCONTAINER **gbc_ptr);
void gb_delete_dummy_father(GBCONTAINER **dummy_father);
gb_transaction_save *gb_new_gb_transaction_save(GBDATA *gbd);
void gb_add_ref_gb_transaction_save(gb_transaction_save *ts);
void gb_del_ref_gb_transaction_save(gb_transaction_save *ts);
void gb_del_ref_and_extern_gb_transaction_save(gb_transaction_save *ts);
void gb_abortdata(GBDATA *gbd);
void gb_save_extern_data_in_ts(GBDATA *gbd);
void gb_write_index_key(GBCONTAINER *father, long index, GBQUARK new_index);
void gb_write_key(GBDATA *gbd, const char *s);
void gb_create_key_array(GB_MAIN_TYPE *Main, int index);
long gb_create_key(GB_MAIN_TYPE *Main, const char *s, bool create_gb_key);
void gb_free_all_keys(GB_MAIN_TYPE *Main);
char *gb_abort_entry(GBDATA *gbd);
int gb_abort_transaction_local_rek(GBDATA *gbd, long mode);
GB_ERROR gb_commit_transaction_local_rek(GBDATA *gbd, long mode, int *pson_created);

/* adcompr.cxx */
gb_compress_tree *gb_build_uncompress_tree(const unsigned char *data, long short_flag, char **end);
void gb_free_compress_tree(gb_compress_tree *tree);
gb_compress_list *gb_build_compress_list(const unsigned char *data, long short_flag, long *size);
char *gb_compress_bits(const char *source, long size, const unsigned char *c_0, long *msize);
GB_BUFFER gb_uncompress_bits(const char *source, long size, char c_0, char c_1);
void gb_compress_equal_bytes_2(const char *source, long size, long *msize, char *dest);
GB_BUFFER gb_compress_equal_bytes(const char *source, long size, long *msize, int last_flag);
void gb_compress_huffmann_add_to_list(long val, gb_compress_list *element);
long gb_compress_huffmann_pop(long *val, gb_compress_list **element);
char *gb_compress_huffmann_rek(gb_compress_list *bc, int bits, int bitcnt, char *dest);
GB_BUFFER gb_compress_huffmann(GB_CBUFFER source, long size, long *msize, int last_flag);
GB_BUFFER gb_uncompress_bytes(GB_CBUFFER source, long size, long *new_size);
GB_BUFFER gb_uncompress_longs_old(GB_CBUFFER source, long size, long *new_size);
GB_BUFFER gb_compress_longs(GB_CBUFFER source, long size, int last_flag);
GB_DICTIONARY *gb_get_dictionary(GB_MAIN_TYPE *Main, GBQUARK key);
GB_BUFFER gb_compress_data(GBDATA *gbd, int key, GB_CBUFFER source, long size, long *msize, GB_COMPRESSION_MASK max_compr, bool pre_compressed);
GB_CBUFFER gb_uncompress_data(GBDATA *gbd, GB_CBUFFER source, long size);

/* adindex.cxx */
char *gb_index_check_in(GBDATA *gbd);
void gb_index_check_out(GBDATA *gbd);
void gb_destroy_indices(GBCONTAINER *gbc);
GBDATA *gb_index_find(GBCONTAINER *gbf, gb_index_files *ifs, GBQUARK quark, const char *val, GB_CASE case_sens, int after_index);
void gb_init_undo_stack(GB_MAIN_TYPE *Main);
void gb_free_undo_stack(GB_MAIN_TYPE *Main);
char *gb_set_undo_sync(GBDATA *gb_main);
char *gb_disable_undo(GBDATA *gb_main);
void gb_check_in_undo_create(GB_MAIN_TYPE *Main, GBDATA *gbd);
void gb_check_in_undo_modify(GB_MAIN_TYPE *Main, GBDATA *gbd);
void gb_check_in_undo_delete(GB_MAIN_TYPE *Main, GBDATA *gbd, int deep);

/* admap.cxx */
GB_ERROR gb_save_mapfile(GB_MAIN_TYPE *Main, GB_CSTR path);
int gb_is_valid_mapfile(const char *path, gb_map_header *mheader, int verbose);
GBDATA *gb_map_mapfile(const char *path);
int gb_isMappedMemory(void *mem);

/* adquery.cxx */
GBDATA *gb_find_by_nr(GBDATA *father, int index);
void gb_init_ctype_table(void);
GBDATA *gb_search(GBDATA *gbd, const char *str, GB_TYPES create, int internflag);
GBDATA *gb_search_marked(GBCONTAINER *gbc, GBQUARK key_quark, int firstindex, size_t skip_over);
void gb_install_command_table(GBDATA *gb_main, struct GBL_command_table *table, size_t table_size);
char *gbs_search_second_x(const char *str);
char *gbs_search_second_bracket(const char *source);
char *gbs_search_next_separator(const char *source, const char *seps);

/* adsocket.cxx */
void gbcms_sigpipe(int dummy_1x);
void gbcm_read_flush(void);
long gbcm_read(int socket, char *ptr, long size);
GBCM_ServerResult gbcm_write_flush(int socket);
GBCM_ServerResult gbcm_write(int socket, const char *ptr, long size);
GB_ERROR gbcm_open_socket(const char *path, long delay2, long do_connect, int *psocket, char **unix_name);
gbcmc_comm *gbcmc_open(const char *path);
void gbcmc_restore_sighandlers(gbcmc_comm *link);
long gbcm_write_two(int socket, long a, long c);
GBCM_ServerResult gbcm_read_two(int socket, long a, long *b, long *c);
GBCM_ServerResult gbcm_write_string(int socket, const char *key);
char *gbcm_read_string(int socket);
GBCM_ServerResult gbcm_write_long(int socket, long data);
long gbcm_read_long(int socket);

#else
#error gb_prot.h included twice
#endif /* GB_PROT_H */
