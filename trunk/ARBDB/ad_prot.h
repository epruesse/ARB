/* ARB database interface.
 *
 * This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef AD_PROT_H
#define AD_PROT_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* adTest.cxx */
const char *GB_get_type_name(GBDATA *gbd);
const char *GB_get_db_path(GBDATA *gbd);
void GB_dump_db_path(GBDATA *gbd);
void GB_dump(GBDATA *gbd);
void GB_dump_no_limit(GBDATA *gbd);
GB_ERROR GB_fix_database(GBDATA *gb_main);

/* ad_load.cxx */
void GB_set_verbose(void);
void GB_set_next_main_idx(long idx);
GBDATA *GB_login(const char *cpath, const char *opent, const char *user);
GBDATA *GB_open(const char *path, const char *opent);

/* ad_save_load.cxx */
GB_ERROR GB_save(GBDATA *gb, const char *path, const char *savetype);
GB_ERROR GB_create_directory(const char *path);
GB_ERROR GB_save_in_home(GBDATA *gb, const char *path, const char *savetype);
GB_ERROR GB_save_as(GBDATA *gb, const char *path, const char *savetype);
GB_ERROR GB_delete_database(GB_CSTR filename);
GB_ERROR GB_save_quick_as(GBDATA *gb_main, const char *path);
GB_ERROR GB_save_quick(GBDATA *gb, const char *refpath);
void GB_disable_path(GBDATA *gbd, const char *path);

/* adcomm.cxx */
GB_ERROR GBCMS_open(const char *path, long timeout, GBDATA *gb_main);
void GBCMS_shutdown(GBDATA *gbd);
bool GBCMS_accept_calls(GBDATA *gbd, bool wait_extra_time);
long GB_read_clients(GBDATA *gbd);
bool GB_is_server(GBDATA *gbd);
bool GB_is_client(GBDATA *gbd);
GBDATA *GBCMC_find(GBDATA *gbd, const char *key, GB_TYPES type, const char *str, GB_CASE case_sens, GB_SEARCH_TYPE gbs);
int GBCMC_system(GBDATA *gbd, const char *ss) __ATTR__USERESULT;
GB_ERROR GB_tell_server_dont_wait(GBDATA *gbd);
GB_CSTR GB_get_hostname(void);
GB_ERROR GB_install_pid(int mode);
const char *GB_date_string(void);

/* adhash.cxx */
GB_HASH *GBS_create_hash(long estimated_elements, GB_CASE case_sens);
GB_HASH *GBS_create_dynaval_hash(long estimated_elements, GB_CASE case_sens, void (*freefun)(long));
void GBS_dynaval_free(long val);
void GBS_optimize_hash(GB_HASH *hs);
char *GBS_hashtab_2_string(GB_HASH *hash);
void GBS_string_2_hashtab(GB_HASH *hash, char *data);
long GBS_read_hash(const GB_HASH *hs, const char *key);
long GBS_write_hash(GB_HASH *hs, const char *key, long val);
long GBS_write_hash_no_strdup(GB_HASH *hs, char *key, long val);
long GBS_incr_hash(GB_HASH *hs, const char *key);
double GBS_hash_mean_access_costs(GB_HASH *hs);
void GBS_erase_hash(GB_HASH *hs);
void GBS_free_hash(GB_HASH *hs);
void GBS_clear_hash_statistic_summary(const char *id);
void GBS_print_hash_statistic_summary(const char *id);
void GBS_calc_hash_statistic(GB_HASH *hs, const char *id, int print);
void GBS_hash_do_loop(GB_HASH *hs, gb_hash_loop_type func, void *client_data);
size_t GBS_hash_count_elems(GB_HASH *hs);
size_t GBS_hash_count_value(GB_HASH *hs, long val);
const char *GBS_hash_next_element_that(GB_HASH *hs, const char *last_key, bool (*condition)(const char *key, long val, void *cd), void *cd);
void GBS_hash_do_sorted_loop(GB_HASH *hs, gb_hash_loop_type func, gbs_hash_compare_function sorter, void *client_data);
int GBS_HCF_sortedByKey(const char *k0, long dummy_1x, const char *k1, long dummy_2x);
GB_NUMHASH *GBS_create_numhash(size_t estimated_elements);
long GBS_read_numhash(GB_NUMHASH *hs, long key);
long GBS_write_numhash(GB_NUMHASH *hs, long key, long val);
long GBS_numhash_count_elems(GB_NUMHASH *hs);
void GBS_erase_numhash(GB_NUMHASH *hs);
void GBS_free_numhash(GB_NUMHASH *hs);

/* adcache.cxx */
char *GB_set_cache_size(GBDATA *gbd, size_t size);

/* adhashtools.cxx */
void GBT_add_item_to_hash(GBDATA *gb_item, GB_HASH *item_hash);
GB_HASH *GBT_create_species_hash_sized(GBDATA *gb_main, long species_count);
GB_HASH *GBT_create_species_hash(GBDATA *gb_main);
GB_HASH *GBT_create_marked_species_hash(GBDATA *gb_main);
GB_HASH *GBT_create_SAI_hash(GBDATA *gb_main);
GB_HASH *GBT_create_organism_hash(GBDATA *gb_main);

/* adlang1.cxx */
NOT4PERL void GB_set_export_sequence_hook(gb_export_sequence_cb escb);
void GB_set_ACISRT_trace(int enable);
int GB_get_ACISRT_trace(void);

/* adlink.cxx */
GBDATA *GB_follow_link(GBDATA *gb_link);
GB_ERROR GB_install_link_follower(GBDATA *gb_main, const char *link_type, GB_Link_Follower link_follower);

/* admalloc.cxx */
NOT4PERL void *GB_calloc(unsigned int nelem, unsigned int elsize);
char *GB_strpartdup(const char *start, const char *end);
char *GB_strndup(const char *start, int len);
NOT4PERL void *GB_recalloc(void *ptr, unsigned int oelem, unsigned int nelem, unsigned int elsize);
void GB_memerr(void);

/* admatch.cxx */
GBS_string_matcher *GBS_compile_matcher(const char *search_expr, GB_CASE case_flag);
void GBS_free_matcher(GBS_string_matcher *matcher);
GBS_regex *GBS_compile_regexpr(const char *regexpr, GB_CASE case_flag, GB_ERROR *error);
void GBS_free_regexpr(GBS_regex *toFree);
const char *GBS_unwrap_regexpr(const char *regexpr_in_slashes, GB_CASE *case_flag, GB_ERROR *error);
const char *GBS_regmatch_compiled(const char *str, GBS_regex *comreg, size_t *matchlen);
const char *GBS_regmatch(const char *str, const char *regExpr, size_t *matchlen, GB_ERROR *error);
char *GBS_regreplace(const char *str, const char *regReplExpr, GB_ERROR *error);
GB_CSTR GBS_find_string(GB_CSTR str, GB_CSTR substr, int match_mode);
bool GBS_string_matches(const char *str, const char *search, GB_CASE case_sens);
bool GBS_string_matches_regexp(const char *str, const GBS_string_matcher *expr);
char *GBS_string_eval(const char *insource, const char *icommand, GBDATA *gb_container);

/* admath.cxx */
double GB_log_fak(int n);
double GB_frandom(void);
int GB_random(int range);

/* adoptimize.cxx */
GB_ERROR GB_optimize(GBDATA *gb_main);

/* adperl.cxx */
GB_ERROR GBC_await_error(void);

/* adsort.cxx */
void GB_sort(void **array, size_t first, size_t behind_last, gb_compare_function compare, void *client_data);
int GB_string_comparator(const void *v0, const void *v1, void *);

/* adstring.cxx */
void GB_raise_critical_error(const char *msg);
char *GBS_string_2_key_with_exclusions(const char *str, const char *additional);
char *GBS_string_2_key(const char *str);
char *GB_memdup(const char *source, size_t len);
GB_ERROR GB_check_key(const char *key) __ATTR__USERESULT;
GB_ERROR GB_check_link_name(const char *key) __ATTR__USERESULT;
GB_ERROR GB_check_hkey(const char *key) __ATTR__USERESULT;
char *GBS_remove_escape(char *com);
char *GBS_escape_string(const char *str, const char *chars_to_escape, char escape_char);
char *GBS_unescape_string(const char *str, const char *escaped_chars, char escape_char);
GBS_strstruct *GBS_stropen(long init_size);
char *GBS_strclose(GBS_strstruct *strstr);
void GBS_strforget(GBS_strstruct *strstr);
GB_BUFFER GBS_mempntr(GBS_strstruct *strstr);
long GBS_memoffset(GBS_strstruct *strstr);
void GBS_str_cut_tail(GBS_strstruct *strstr, size_t byte_count);
void GBS_strncat(GBS_strstruct *strstr, const char *ptr, size_t len);
void GBS_strcat(GBS_strstruct *strstr, const char *ptr);
void GBS_strnprintf(GBS_strstruct *strstr, long len, const char *templat, ...) __ATTR__FORMAT(3);
void GBS_chrcat(GBS_strstruct *strstr, char ch);
void GBS_chrncat(GBS_strstruct *strstr, char ch, size_t n);
void GBS_intcat(GBS_strstruct *strstr, long val);
void GBS_floatcat(GBS_strstruct *strstr, double val);
char *GBS_eval_env(GB_CSTR p);
long GBS_gcgchecksum(const char *seq);
uint32_t GB_checksum(const char *seq, long length, int ignore_case, const char *exclude);
uint32_t GBS_checksum(const char *seq, int ignore_case, const char *exclude);
char *GBS_extract_words(const char *source, const char *chars, float minlen, bool sort_output);
size_t GBS_shorten_repeated_data(char *data);
char *GBS_merge_tagged_strings(const char *s1, const char *tag1, const char *replace1, const char *s2, const char *tag2, const char *replace2);
char *GBS_string_eval_tagged_string(GBDATA *gb_main, const char *s, const char *dt, const char *tag, const char *srt, const char *aci, GBDATA *gbd);
char *GB_read_as_tagged_string(GBDATA *gbd, const char *tagi);
void GBS_fwrite_string(const char *strngi, FILE *out);
char *GBS_fread_string(FILE *in);
char *GBS_fconvert_string(char *buffer);
char *GBS_replace_tabs_by_spaces(const char *text);
const char *GBS_readable_size(unsigned long long size);
char *GBS_trim(const char *str);
char *GBS_log_dated_action_to(const char *comment, const char *action);

/* adfile.cxx */
GB_CSTR GB_getcwd(void);
char *GB_find_all_files(const char *dir, const char *mask, bool filename_only);
char *GB_find_latest_file(const char *dir, const char *mask);
char *GBS_find_lib_file(const char *filename, const char *libprefix, bool warn_when_not_found);
char **GBS_read_dir(const char *dir, const char *mask);
bool GB_test_textfile_difflines(const char *file1, const char *file2, int expected_difflines, int special_mode);
bool GB_test_files_equal(const char *file1, const char *file2);

/* adsystem.cxx */
DictData *GB_get_dictionary(GBDATA *gb_main, const char *key);
GB_ERROR GB_set_dictionary(GBDATA *gb_main, const char *key, const DictData *dd);
void GB_free_dictionary(DictData *dd);

/* adtcp.cxx */
const char *GBS_scan_arb_tcp_param(const char *ipPort, const char *wantedParam);

#if (UNIT_TESTS == 1)
#define TEST_SERVER_ID (-666)
#endif

const char *GBS_nameserver_tag(const char *add_field);
const char *GBS_ptserver_tag(int id);
const char *GBS_read_arb_tcp(const char *env);
const char *const *GBS_get_arb_tcp_entries(const char *matching);
const char *GBS_ptserver_logname(void);
void GBS_add_ptserver_logentry(const char *entry);
char *GBS_ptserver_id_to_choice(int i, int showBuild);

/* arbdb.cxx */
char *GB_rel(void *struct_address, long rel_address);
NOT4PERL GB_ERROR GB_safe_atof(const char *str, double *res);
double GB_atof(const char *str);
GB_BUFFER GB_give_buffer(size_t size);
GB_BUFFER GB_increase_buffer(size_t size);
NOT4PERL int GB_give_buffer_size(void);
GB_BUFFER GB_give_buffer2(long size);
int GB_is_in_buffer(GB_CBUFFER ptr);
char *GB_check_out_buffer(GB_CBUFFER buffer);
GB_BUFFER GB_give_other_buffer(GB_CBUFFER buffer, long size);
void GB_atexit(void (*exitfun)());
void GB_exit_gb(void);
void GB_init_gb(void);
int GB_open_DBs(void);
void GB_atclose(GBDATA *gbd, void (*fun)(GBDATA *gb_main, void *client_data), void *client_data);
void GB_close(GBDATA *gbd);
long GB_read_int(GBDATA *gbd);
int GB_read_byte(GBDATA *gbd);
void *GB_read_pointer(GBDATA *gbd);
double GB_read_float(GBDATA *gbd);
long GB_read_count(GBDATA *gbd);
long GB_read_memuse(GBDATA *gbd);
GB_CSTR GB_read_pntr(GBDATA *gbd);
GB_CSTR GB_read_char_pntr(GBDATA *gbd);
char *GB_read_string(GBDATA *gbd);
long GB_read_string_count(GBDATA *gbd);
GB_CSTR GB_read_link_pntr(GBDATA *gbd);
char *GB_read_link(GBDATA *gbd);
long GB_read_link_count(GBDATA *gbd);
long GB_read_bits_count(GBDATA *gbd);
GB_CSTR GB_read_bits_pntr(GBDATA *gbd, char c_0, char c_1);
char *GB_read_bits(GBDATA *gbd, char c_0, char c_1);
GB_CSTR GB_read_bytes_pntr(GBDATA *gbd);
long GB_read_bytes_count(GBDATA *gbd);
char *GB_read_bytes(GBDATA *gbd);
GB_CUINT4 *GB_read_ints_pntr(GBDATA *gbd);
long GB_read_ints_count(GBDATA *gbd);
GB_UINT4 *GB_read_ints(GBDATA *gbd);
GB_CFLOAT *GB_read_floats_pntr(GBDATA *gbd);
long GB_read_floats_count(GBDATA *gbd);
float *GB_read_floats(GBDATA *gbd);
char *GB_read_as_string(GBDATA *gbd);
long GB_read_from_ints(GBDATA *gbd, long index);
double GB_read_from_floats(GBDATA *gbd, long index);
GB_ERROR GB_write_byte(GBDATA *gbd, int i);
GB_ERROR GB_write_int(GBDATA *gbd, long i);
GB_ERROR GB_write_pointer(GBDATA *gbd, void *pointer);
GB_ERROR GB_write_float(GBDATA *gbd, double f);
GB_ERROR GB_write_pntr(GBDATA *gbd, const char *s, long bytes_size, long stored_size);
GB_ERROR GB_write_string(GBDATA *gbd, const char *s);
GB_ERROR GB_write_link(GBDATA *gbd, const char *s);
GB_ERROR GB_write_bits(GBDATA *gbd, const char *bits, long size, const char *c_0);
GB_ERROR GB_write_bytes(GBDATA *gbd, const char *s, long size);
GB_ERROR GB_write_ints(GBDATA *gbd, const GB_UINT4 *i, long size);
GB_ERROR GB_write_floats(GBDATA *gbd, const float *f, long size);
GB_ERROR GB_write_as_string(GBDATA *gbd, const char *val);
int GB_read_security_write(GBDATA *gbd);
int GB_read_security_read(GBDATA *gbd);
int GB_read_security_delete(GBDATA *gbd);
int GB_get_my_security(GBDATA *gbd);
GB_ERROR GB_write_security_write(GBDATA *gbd, unsigned long level);
GB_ERROR GB_write_security_read(GBDATA *gbd, unsigned long level);
GB_ERROR GB_write_security_delete(GBDATA *gbd, unsigned long level);
GB_ERROR GB_write_security_levels(GBDATA *gbd, unsigned long readlevel, unsigned long writelevel, unsigned long deletelevel);
void GB_change_my_security(GBDATA *gbd, int level);
void GB_push_my_security(GBDATA *gbd);
void GB_pop_my_security(GBDATA *gbd);
GB_TYPES GB_read_type(GBDATA *gbd);
char *GB_read_key(GBDATA *gbd);
GB_CSTR GB_read_key_pntr(GBDATA *gbd);
GBQUARK GB_key_2_quark(GBDATA *gbd, const char *key);
GBQUARK GB_get_quark(GBDATA *gbd);
bool GB_has_key(GBDATA *gbd, const char *key);
long GB_read_clock(GBDATA *gbd);
long GB_read_transaction(GBDATA *gbd);
GBDATA *GB_get_father(GBDATA *gbd);
GBDATA *GB_get_grandfather(GBDATA *gbd);
GBDATA *GB_get_root(GBDATA *gbd);
bool GB_check_father(GBDATA *gbd, GBDATA *gb_maybefather);
int GB_rename(GBDATA *gbc, const char *new_key);
GBDATA *GB_create(GBDATA *father, const char *key, GB_TYPES type);
GBDATA *GB_create_container(GBDATA *father, const char *key);
GB_ERROR GB_delete(GBDATA *source);
GB_ERROR GB_copy(GBDATA *dest, GBDATA *source);
GB_ERROR GB_copy_with_protection(GBDATA *dest, GBDATA *source, bool copy_all_protections);
char *GB_get_subfields(GBDATA *gbd);
bool GB_allow_compression(GBDATA *gb_main, bool allow_compression);
GB_ERROR GB_set_temporary(GBDATA *gbd);
GB_ERROR GB_clear_temporary(GBDATA *gbd);
bool GB_is_temporary(GBDATA *gbd);
bool GB_in_temporary_branch(GBDATA *gbd);
GB_ERROR GB_push_local_transaction(GBDATA *gbd);
GB_ERROR GB_pop_local_transaction(GBDATA *gbd);
GB_ERROR GB_push_transaction(GBDATA *gbd);
GB_ERROR GB_pop_transaction(GBDATA *gbd);
GB_ERROR GB_begin_transaction(GBDATA *gbd);
GB_ERROR GB_no_transaction(GBDATA *gbd);
GB_ERROR GB_abort_transaction(GBDATA *gbd);
GB_ERROR GB_commit_transaction(GBDATA *gbd);
GB_ERROR GB_end_transaction(GBDATA *gbd, GB_ERROR error);
void GB_end_transaction_show_error(GBDATA *gbd, GB_ERROR error, void (*error_handler)(GB_ERROR));
int GB_get_transaction_level(GBDATA *gbd);
GB_ERROR GB_update_server(GBDATA *gbd);
NOT4PERL bool GB_inside_callback(GBDATA *of_gbd, GB_CB_TYPE cbtype);
GBDATA *GB_get_gb_main_during_cb(void);
NOT4PERL const void *GB_read_old_value(void);
long GB_read_old_size(void);
char *GB_get_callback_info(GBDATA *gbd);
GB_ERROR GB_add_priority_callback(GBDATA *gbd, GB_CB_TYPE type, GB_CB func, int *clientdata, int priority);
GB_ERROR GB_add_callback(GBDATA *gbd, GB_CB_TYPE type, GB_CB func, int *clientdata);
void GB_remove_callback(GBDATA *gbd, GB_CB_TYPE type, GB_CB func, int *clientdata);
void GB_remove_all_callbacks_to(GBDATA *gbd, GB_CB_TYPE type, GB_CB func);
GB_ERROR GB_ensure_callback(GBDATA *gbd, GB_CB_TYPE type, GB_CB func, int *clientdata);
GB_ERROR GB_release(GBDATA *gbd);
int GB_testlocal(GBDATA *gbd);
int GB_nsons(GBDATA *gbd);
void GB_disable_quicksave(GBDATA *gbd, const char *reason);
GB_ERROR GB_resort_data_base(GBDATA *gb_main, GBDATA **new_order_list, long listsize);
GB_ERROR GB_resort_system_folder_to_top(GBDATA *gb_main);
GB_ERROR GB_write_usr_public(GBDATA *gbd, long flags);
long GB_read_usr_public(GBDATA *gbd);
long GB_read_usr_private(GBDATA *gbd);
GB_ERROR GB_write_usr_private(GBDATA *gbd, long ref);
void GB_write_flag(GBDATA *gbd, long flag);
int GB_read_flag(GBDATA *gbd);
void GB_touch(GBDATA *gbd);
char GB_type_2_char(GB_TYPES type);
GB_ERROR GB_print_debug_information(void *, GBDATA *gb_main);
int GB_info(GBDATA *gbd);
long GB_number_of_subentries(GBDATA *gbd);

/* adExperiment.cxx */
GBDATA *EXP_get_experiment_data(GBDATA *gb_species);
GBDATA *EXP_find_experiment_rel_exp_data(GBDATA *gb_experiment_data, const char *name);
GBDATA *EXP_find_experiment(GBDATA *gb_species, const char *name);
GBDATA *EXP_expect_experiment(GBDATA *gb_species, const char *name);
GBDATA *EXP_first_experiment_rel_exp_data(GBDATA *gb_experiment_data);
GBDATA *EXP_next_experiment(GBDATA *gb_experiment);
GBDATA *EXP_find_or_create_experiment_rel_exp_data(GBDATA *gb_experiment_data, const char *name);

/* adGene.cxx */
bool GEN_is_genome_db(GBDATA *gb_main, int default_value);
GBDATA *GEN_findOrCreate_gene_data(GBDATA *gb_species);
GBDATA *GEN_find_gene_data(GBDATA *gb_species);
GBDATA *GEN_expect_gene_data(GBDATA *gb_species);
GBDATA *GEN_find_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name);
GBDATA *GEN_find_gene(GBDATA *gb_species, const char *name);
GBDATA *GEN_create_nonexisting_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name);
GBDATA *GEN_create_nonexisting_gene(GBDATA *gb_species, const char *name);
GBDATA *GEN_find_or_create_gene_rel_gene_data(GBDATA *gb_gene_data, const char *name);
GBDATA *GEN_find_or_create_gene(GBDATA *gb_species, const char *name);
GBDATA *GEN_first_gene(GBDATA *gb_species);
GBDATA *GEN_first_gene_rel_gene_data(GBDATA *gb_gene_data);
GBDATA *GEN_next_gene(GBDATA *gb_gene);
GBDATA *GEN_first_marked_gene(GBDATA *gb_species);
GBDATA *GEN_next_marked_gene(GBDATA *gb_gene);
GEN_position *GEN_new_position(int parts, bool joinable);
void GEN_use_uncertainties(GEN_position *pos);
void GEN_free_position(GEN_position *pos);
GEN_position *GEN_read_position(GBDATA *gb_gene);
GB_ERROR GEN_write_position(GBDATA *gb_gene, const GEN_position *pos);
void GEN_sortAndMergeLocationParts(GEN_position *location);
const char *GEN_origin_organism(GBDATA *gb_pseudo);
const char *GEN_origin_gene(GBDATA *gb_pseudo);
bool GEN_is_pseudo_gene_species(GBDATA *gb_species);
GB_ERROR GEN_organism_not_found(GBDATA *gb_pseudo);
GBDATA *GEN_read_pseudo_species_from_hash(GB_HASH *pseudo_hash, const char *organism_name, const char *gene_name);
void GEN_add_pseudo_species_to_hash(GBDATA *gb_pseudo, GB_HASH *pseudo_hash);
GB_HASH *GEN_create_pseudo_species_hash(GBDATA *gb_main, int additionalSize);
GBDATA *GEN_find_pseudo_species(GBDATA *gb_main, const char *organism_name, const char *gene_name, GB_HASH *pseudo_hash);
GBDATA *GEN_find_origin_organism(GBDATA *gb_pseudo, GB_HASH *organism_hash);
GBDATA *GEN_find_origin_gene(GBDATA *gb_pseudo, GB_HASH *organism_hash);
GBDATA *GEN_first_pseudo_species(GBDATA *gb_main);
GBDATA *GEN_first_pseudo_species_rel_species_data(GBDATA *gb_species_data);
GBDATA *GEN_next_pseudo_species(GBDATA *gb_species);
GBDATA *GEN_first_marked_pseudo_species(GBDATA *gb_main);
GBDATA *GEN_next_marked_pseudo_species(GBDATA *gb_species);
bool GEN_is_organism(GBDATA *gb_species);
GBDATA *GEN_find_organism(GBDATA *gb_main, const char *name);
GBDATA *GEN_first_organism(GBDATA *gb_main);
GBDATA *GEN_next_organism(GBDATA *gb_organism);
long GEN_get_organism_count(GBDATA *gb_main);
GBDATA *GEN_first_marked_organism(GBDATA *gb_main);
GBDATA *GEN_next_marked_organism(GBDATA *gb_organism);
char *GEN_global_gene_identifier(GBDATA *gb_gene, GBDATA *gb_organism);

/* adcompr.cxx */
bool GB_is_dictionary_compressed(GBDATA *gbd);

/* adindex.cxx */
GB_ERROR GB_create_index(GBDATA *gbd, const char *key, GB_CASE case_sens, long estimated_size);
NOT4PERL void GB_dump_indices(GBDATA *gbd);
GB_ERROR GB_request_undo_type(GBDATA *gb_main, GB_UNDO_TYPE type) __ATTR__USERESULT;
GB_UNDO_TYPE GB_get_requested_undo_type(GBDATA *gb_main);
GB_ERROR GB_undo(GBDATA *gb_main, GB_UNDO_TYPE type) __ATTR__USERESULT;
char *GB_undo_info(GBDATA *gb_main, GB_UNDO_TYPE type);
GB_ERROR GB_set_undo_mem(GBDATA *gbd, long memsize);

/* adquery.cxx */
const char *GB_get_GBDATA_path(GBDATA *gbd);
GBDATA *GB_find_sub_by_quark(GBDATA *father, GBQUARK key_quark, GBDATA *after, size_t skip_over);
NOT4PERL GBDATA *GB_find_subcontent_by_quark(GBDATA *father, GBQUARK key_quark, GB_TYPES type, const char *val, GB_CASE case_sens, GBDATA *after, size_t skip_over);
GBDATA *GB_find(GBDATA *gbd, const char *key, GB_SEARCH_TYPE gbs);
GBDATA *GB_find_string(GBDATA *gbd, const char *key, const char *str, GB_CASE case_sens, GB_SEARCH_TYPE gbs);
NOT4PERL GBDATA *GB_find_int(GBDATA *gbd, const char *key, long val, GB_SEARCH_TYPE gbs);
GBDATA *GB_child(GBDATA *father);
GBDATA *GB_nextChild(GBDATA *child);
GBDATA *GB_entry(GBDATA *father, const char *key);
GBDATA *GB_nextEntry(GBDATA *entry);
GBDATA *GB_followingEntry(GBDATA *entry, size_t skip_over);
GBDATA *GB_brother(GBDATA *entry, const char *key);
char *GB_first_non_key_char(const char *str);
GBDATA *GB_search(GBDATA *gbd, const char *fieldpath, GB_TYPES create);
GBDATA *GB_searchOrCreate_string(GBDATA *gb_container, const char *fieldpath, const char *default_value);
GBDATA *GB_searchOrCreate_int(GBDATA *gb_container, const char *fieldpath, long default_value);
GBDATA *GB_searchOrCreate_float(GBDATA *gb_container, const char *fieldpath, double default_value);
GBDATA *GB_search_last_son(GBDATA *gbd);
long GB_number_of_marked_subentries(GBDATA *gbd);
GBDATA *GB_first_marked(GBDATA *gbd, const char *keystring);
GBDATA *GB_following_marked(GBDATA *gbd, const char *keystring, size_t skip_over);
GBDATA *GB_next_marked(GBDATA *gbd, const char *keystring);
char *GB_command_interpreter(GBDATA *gb_main, const char *str, const char *commands, GBDATA *gbd, const char *default_tree_name);

/* adsocket.cxx */
void GB_usleep(long usec);
GB_ULONG GB_time_of_file(const char *path);
long GB_size_of_file(const char *path);
long GB_mode_of_file(const char *path);
long GB_mode_of_link(const char *path);
bool GB_is_regularfile(const char *path);
bool GB_is_link(const char *path);
bool GB_is_executablefile(const char *path);
bool GB_is_privatefile(const char *path, bool read_private);
bool GB_is_readablefile(const char *filename);
bool GB_is_directory(const char *path);
long GB_getuid_of_file(const char *path);
int GB_unlink(const char *path);
void GB_unlink_or_warn(const char *path, GB_ERROR *error);
char *GB_follow_unix_link(const char *path);
GB_ERROR GB_symlink(const char *target, const char *link);
GB_ERROR GB_set_mode_of_file(const char *path, long mode);
GB_ERROR GB_rename_file(const char *oldpath, const char *newpath);
char *GB_read_fp(FILE *in);
char *GB_read_file(const char *path);
char *GB_map_FILE(FILE *in, int writeable);
char *GB_map_file(const char *path, int writeable);
long GB_size_of_FILE(FILE *in);
GB_ULONG GB_time_of_day(void);
long GB_last_saved_clock(GBDATA *gb_main);
GB_ULONG GB_last_saved_time(GBDATA *gb_main);
GB_ERROR GB_textprint(const char *path) __ATTR__USERESULT;
GB_ERROR GB_system(const char *system_command) __ATTR__USERESULT;
GB_ERROR GB_xterm(void) __ATTR__USERESULT;
GB_ERROR GB_xcmd(const char *cmd, bool background, bool wait_only_if_error) __ATTR__USERESULT;
char *GB_executable(GB_CSTR exe_name);
char *GB_find_executable(GB_CSTR description_of_executable, ...) __ATTR__SENTINEL;
GB_CSTR GB_getenvUSER(void);
GB_CSTR GB_getenvHOME(void);
GB_CSTR GB_getenvARBHOME(void);
GB_CSTR GB_getenvARBMACRO(void);
GB_CSTR GB_getenvARBMACROHOME(void);
GB_CSTR GB_getenvARBCONFIG(void);
GB_CSTR GB_getenvPATH(void);
GB_CSTR GB_getenvARB_GS(void);
GB_CSTR GB_getenvARB_PDFVIEW(void);
GB_CSTR GB_getenvARB_TEXTEDIT(void);
GB_CSTR GB_getenvDOCPATH(void);
GB_CSTR GB_getenvHTMLDOCPATH(void);
NOT4PERL gb_getenv_hook GB_install_getenv_hook(gb_getenv_hook hook);
GB_CSTR GB_getenv(const char *env);
int GB_host_is_local(const char *hostname);
GB_ULONG GB_get_physical_memory(void);
GB_CSTR GB_append_suffix(const char *name, const char *suffix);
GB_CSTR GB_canonical_path(const char *anypath);
GB_CSTR GB_concat_path(GB_CSTR anypath_left, GB_CSTR anypath_right);
GB_CSTR GB_concat_full_path(const char *anypath_left, const char *anypath_right);
GB_CSTR GB_unfold_path(const char *pwd_envar, const char *path);
GB_CSTR GB_path_in_ARBHOME(const char *relative_path);
GB_CSTR GB_path_in_ARBLIB(const char *relative_path);
GB_CSTR GB_path_in_ARBHOME(const char *relative_path_left, const char *anypath_right);
GB_CSTR GB_path_in_ARBLIB(const char *relative_path_left, const char *anypath_right);
FILE *GB_fopen_tempfile(const char *filename, const char *fmode, char **res_fullname);
char *GB_create_tempfile(const char *name);
char *GB_unique_filename(const char *name_prefix, const char *suffix);
void GB_remove_on_exit(const char *filename);
void GB_split_full_path(const char *fullpath, char **res_dir, char **res_fullname, char **res_name_only, char **res_suffix);

#else
#error ad_prot.h included twice
#endif /* AD_PROT_H */
