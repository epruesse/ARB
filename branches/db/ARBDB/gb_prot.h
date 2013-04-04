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


/* ad_core.cxx */
void gb_touch_entry(GBDATA *gbd, GB_CHANGE val);
void gb_touch_header(GBCONTAINER *gbc);
void gb_untouch_children(GBCONTAINER *gbc);
void gb_untouch_me(GBENTRY *gbe);
void gb_untouch_children_and_me(GBCONTAINER *gbc);
void gb_create_header_array(GBCONTAINER *gbc, int size);
void gb_create_extended(GBDATA *gbd);
GB_MAIN_TYPE *gb_make_gb_main_type(const char *path);
void gb_destroy_main(GB_MAIN_TYPE *Main);
GBDATA *gb_make_pre_defined_entry(GBCONTAINER *father, GBDATA *gbd, long index_pos, GBQUARK keyq);
GBENTRY *gb_make_entry(GBCONTAINER *father, const char *key, long index_pos, GBQUARK keyq, GB_TYPES type);
GBCONTAINER *gb_make_pre_defined_container(GBCONTAINER *father, GBCONTAINER *gbc, long index_pos, GBQUARK keyq);
GBCONTAINER *gb_make_container(GBCONTAINER *father, const char *key, long index_pos, GBQUARK keyq);
void gb_pre_delete_entry(GBDATA *gbd);
void gb_delete_entry(GBCONTAINER*& gbc);
void gb_delete_entry(GBENTRY*& gbe);
void gb_delete_entry(GBDATA*& gbd);
void gb_delete_dummy_father(GBCONTAINER*& gbc);
void gb_add_ref_gb_transaction_save(gb_transaction_save *ts);
void gb_del_ref_gb_transaction_save(gb_transaction_save *ts);
void gb_del_ref_and_extern_gb_transaction_save(gb_transaction_save *ts);
void gb_save_extern_data_in_ts(GBENTRY *gbe);
void gb_write_index_key(GBCONTAINER *father, long index, GBQUARK new_index);
void gb_create_key_array(GB_MAIN_TYPE *Main, int index);
long gb_create_key(GB_MAIN_TYPE *Main, const char *s, bool create_gb_key);
char *gb_abort_entry(GBDATA *gbd);
void gb_abort_transaction_local_rek(GBDATA*& gbd);
GB_ERROR gb_commit_transaction_local_rek(GBDATA*& gbd, long mode, int *pson_created);

/* ad_load.cxx */
GB_MAIN_IDX gb_make_main_idx(GB_MAIN_TYPE *Main);

/* ad_save_load.cxx */
char *gb_findExtension(char *path);
GB_CSTR gb_oldQuicksaveName(GB_CSTR path, int nr);
GB_CSTR gb_quicksaveName(GB_CSTR path, int nr);
GB_CSTR gb_mapfile_name(GB_CSTR path);
long gb_ascii_2_bin(const char *source, GBENTRY *gbe);
long gb_read_bin_error(FILE *in, GBDATA *gbd, const char *text);

/* adcache.cxx */
char *gb_read_cache(GBENTRY *gbe);
void gb_free_cache(GB_MAIN_TYPE *Main, GBENTRY *gbe);
void gb_uncache(GBENTRY *gbe);
char *gb_alloc_cache_index(GBENTRY *gbe, size_t size);

/* adcomm.cxx */
GB_ERROR gbcm_unfold_client(GBCONTAINER *gbc, long deep, long index_pos) __ATTR__USERESULT;
GB_ERROR gbcmc_begin_sendupdate(GBDATA *gbd);
GB_ERROR gbcmc_end_sendupdate(GBDATA *gbd);
GB_ERROR gbcmc_sendupdate_create(GBDATA *gbd);
GB_ERROR gbcmc_sendupdate_delete(GBDATA *gbd);
GB_ERROR gbcmc_sendupdate_update(GBDATA *gbd, int send_headera);
GB_ERROR gbcmc_begin_transaction(GBDATA *gbd);
GB_ERROR gbcmc_init_transaction(GBCONTAINER *gbc);
GB_ERROR gbcmc_commit_transaction(GBDATA *gbd);
GB_ERROR gbcmc_abort_transaction(GBDATA *gbd);
GB_ERROR gbcms_add_to_delete_list(GBDATA *gbd);
long gbcmc_key_alloc(GBDATA *gbd, const char *key);
GB_ERROR gbcmc_send_undo_commands(GBDATA *gbd, enum gb_undo_commands command) __ATTR__USERESULT;
char *gbcmc_send_undo_info_commands(GBDATA *gbd, enum gb_undo_commands command);
GB_ERROR gbcm_login(GBCONTAINER *gb_main, const char *loginname);
GBCM_ServerResult gbcmc_close(gbcmc_comm *link);
GB_ERROR gbcm_logout(GB_MAIN_TYPE *Main, const char *loginname);

/* adcompr.cxx */
gb_compress_tree *gb_build_uncompress_tree(const unsigned char *data, long short_flag, char **end);
void gb_free_compress_tree(gb_compress_tree *tree);
gb_compress_list *gb_build_compress_list(const unsigned char *data, long short_flag, long *size);
char *gb_compress_bits(const char *source, long size, const unsigned char *c_0, long *msize);
GB_BUFFER gb_uncompress_bits(const char *source, long size, char c_0, char c_1);
void gb_compress_equal_bytes_2(const char *source, size_t size, size_t *msize, char *dest);
GB_BUFFER gb_uncompress_bytes(GB_CBUFFER source, size_t size, size_t *new_size);
GB_BUFFER gb_uncompress_longs_old(GB_CBUFFER source, size_t size, size_t *new_size);
GB_DICTIONARY *gb_get_dictionary(GB_MAIN_TYPE *Main, GBQUARK key);
GB_BUFFER gb_compress_data(GBDATA *gbd, int key, GB_CBUFFER source, size_t size, size_t *msize, GB_COMPRESSION_MASK max_compr, bool pre_compressed);
GB_CBUFFER gb_uncompress_data(GBDATA *gbd, GB_CBUFFER source, size_t size);

/* adfile.cxx */
GB_ERROR gb_scan_directory(char *basename, gb_scandir *sd) __ATTR__USERESULT_TODO;

/* adhash.cxx */
size_t gbs_get_a_prime(size_t above_or_equal_this);

/* adindex.cxx */
char *gb_index_check_in(GBENTRY *gbe);
void gb_index_check_out(GBENTRY *gbe);
void gb_destroy_indices(GBCONTAINER *gbc);
GBDATA *gb_index_find(GBCONTAINER *gbf, gb_index_files *ifs, GBQUARK quark, const char *val, GB_CASE case_sens, int after_index);
void gb_init_undo_stack(GB_MAIN_TYPE *Main);
void gb_free_undo_stack(GB_MAIN_TYPE *Main);
char *gb_set_undo_sync(GBDATA *gb_main);
char *gb_disable_undo(GBDATA *gb_main);
void gb_check_in_undo_create(GB_MAIN_TYPE *Main, GBDATA *gbd);
void gb_check_in_undo_modify(GB_MAIN_TYPE *Main, GBDATA *gbd);
void gb_check_in_undo_delete(GB_MAIN_TYPE *Main, GBDATA*& gbd);

/* adlang1.cxx */
void gbl_install_standard_commands(GBDATA *gb_main);

/* admalloc.cxx */
void gbm_flush_mem(void);
void gbm_init_mem(void);
void gbm_debug_mem(void);

/* admap.cxx */
GB_ERROR gb_save_mapfile(GB_MAIN_TYPE *Main, GB_CSTR path);
int gb_is_valid_mapfile(const char *path, gb_map_header *mheader, int verbose);
GBDATA *gb_map_mapfile(const char *path);
int gb_isMappedMemory(void *mem);

/* adoptimize.cxx */
GB_ERROR gb_convert_V2_to_V3(GBDATA *gb_main);
char *gb_uncompress_by_dictionary(GBDATA *gbd, GB_CSTR s_source, size_t size, size_t *new_size);
char *gb_compress_by_dictionary(GB_DICTIONARY *dict, GB_CSTR s_source, size_t size, size_t *msize, int last_flag, int search_backward, int search_forward);

/* adquery.cxx */
GBDATA *gb_find_by_nr(GBCONTAINER *father, int index);
void gb_init_ctype_table(void);
GBDATA *gb_search(GBCONTAINER *gbc, const char *key, GB_TYPES create, int internflag);
void gb_install_command_table(GBDATA *gb_main, struct GBL_command_table *table, size_t table_size);
char *gbs_search_second_bracket(const char *source);

/* adsocket.cxx */
void gbcms_sigpipe(int dummy_1x);
void gbcm_read_flush(void);
long gbcm_read(int socket, char *ptr, long size);
GBCM_ServerResult gbcm_write_flush(int socket);
GBCM_ServerResult gbcm_write(int socket, const char *ptr, long size);
GB_ERROR gbcm_open_socket(const char *path, long delay2, long do_connect, int *psocket, char **unix_name);
long gbcms_close(gbcmc_comm *link);
gbcmc_comm *gbcmc_open(const char *path);
long gbcm_write_two(int socket, long a, long c);
GBCM_ServerResult gbcm_read_two(int socket, long a, long *b, long *c);
GBCM_ServerResult gbcm_write_string(int socket, const char *key);
char *gbcm_read_string(int socket);
GBCM_ServerResult gbcm_write_long(int socket, long data);
long gbcm_read_long(int socket);

/* adsystem.cxx */
GB_ERROR gb_load_dictionary_data(GBDATA *gb_main, const char *key, char **dict_data, long *size);
void gb_load_single_key_data(GBDATA *gb_main, GBQUARK q);
GB_ERROR gb_save_dictionary_data(GBDATA *gb_main, const char *key, const char *dict, int size);
GB_ERROR gb_load_key_data_and_dictionaries(GB_MAIN_TYPE *Main) __ATTR__USERESULT;

/* arbdb.cxx */
GB_ERROR gb_unfold(GBCONTAINER *gbc, long deep, int index_pos);
void gb_close_unclosed_DBs(void);
int gb_read_nr(GBDATA *gbd);
GB_ERROR gb_write_compressed_pntr(GBENTRY *gbe, const char *s, long memsize, long stored_size);
int gb_get_compression_mask(GB_MAIN_TYPE *Main, GBQUARK key, int gb_type);
GB_CSTR gb_read_key_pntr(GBDATA *gbd);
GBQUARK gb_find_existing_quark(GB_MAIN_TYPE *Main, const char *key);
GBQUARK gb_find_or_create_quark(GB_MAIN_TYPE *Main, const char *key);
GBQUARK gb_find_or_create_NULL_quark(GB_MAIN_TYPE *Main, const char *key);
GBCONTAINER *gb_get_root(GBENTRY *gbe);
GBCONTAINER *gb_get_root(GBCONTAINER *gbc);
GBENTRY *gb_create(GBCONTAINER *father, const char *key, GB_TYPES type);
GBCONTAINER *gb_create_container(GBCONTAINER *father, const char *key);
GB_ERROR gb_delete_force(GBDATA *source);
void gb_add_changed_callback_list(GBDATA *gbd, gb_transaction_save *old, const gb_cb_spec& cb);
void gb_add_delete_callback_list(GBDATA *gbd, gb_transaction_save *old, const gb_cb_spec& cb);
GB_MAIN_TYPE *gb_get_main_during_cb(void);
GB_ERROR gb_resort_system_folder_to_top(GBCONTAINER *gb_main);

#else
#error gb_prot.h included twice
#endif /* GB_PROT_H */
