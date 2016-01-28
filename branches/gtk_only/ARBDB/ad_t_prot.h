/* ARB toolkit.
 *
 * This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef AD_T_PROT_H
#define AD_T_PROT_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* adChangeKey.cxx */
GBDATA *GBT_get_changekey(GBDATA *gb_main, const char *key, const char *change_key_path);
GB_TYPES GBT_get_type_of_changekey(GBDATA *gb_main, const char *field_name, const char *change_key_path);
GB_ERROR GBT_add_new_changekey_to_keypath(GBDATA *gb_main, const char *name, int type, const char *keypath);
GB_ERROR GBT_add_new_changekey(GBDATA *gb_main, const char *name, int type);
GB_ERROR GBT_add_new_gene_changekey(GBDATA *gb_main, const char *name, int type);
GB_ERROR GBT_add_new_experiment_changekey(GBDATA *gb_main, const char *name, int type);
GB_ERROR GBT_convert_changekey(GBDATA *gb_main, const char *name, GB_TYPES target_type);

/* adRevCompl.cxx */
char *GBT_reverseNucSequence(const char *s, int len);
char *GBT_complementNucSequence(const char *s, int len, char T_or_U);
NOT4PERL GB_ERROR GBT_determine_T_or_U(GB_alignment_type alignment_type, char *T_or_U, const char *supposed_target);
NOT4PERL void GBT_reverseComplementNucSequence(char *seq, long length, char T_or_U);

/* adali.cxx */
GBDATA *GBT_get_presets(GBDATA *gb_main);
int GBT_count_alignments(GBDATA *gb_main);
GB_ERROR GBT_check_data(GBDATA *Main, const char *alignment_name);
void GBT_get_alignment_names(ConstStrArray& names, GBDATA *gbd);
GB_ERROR GBT_check_alignment_name(const char *alignment_name);
GBDATA *GBT_create_alignment(GBDATA *gbd, const char *name, long len, long aligned, long security, const char *type);
GB_ERROR GBT_rename_alignment(GBDATA *gbMain, const char *source, const char *dest, int copy, int dele);
NOT4PERL GBDATA *GBT_add_data(GBDATA *species, const char *ali_name, const char *key, GB_TYPES type) __ATTR__DEPRECATED_TODO("better use GBT_create_sequence_data()");
NOT4PERL GBDATA *GBT_create_sequence_data(GBDATA *species, const char *ali_name, const char *key, GB_TYPES type, int security_write);
GBDATA *GBT_gen_accession_number(GBDATA *gb_species, const char *ali_name);
int GBT_is_partial(GBDATA *gb_species, int default_value, bool define_if_undef);
GBDATA *GBT_find_sequence(GBDATA *gb_species, const char *aliname);
char *GBT_get_default_alignment(GBDATA *gb_main);
GB_ERROR GBT_set_default_alignment(GBDATA *gb_main, const char *alignment_name);
GBDATA *GBT_get_alignment(GBDATA *gb_main, const char *aliname);
long GBT_get_alignment_len(GBDATA *gb_main, const char *aliname);
GB_ERROR GBT_set_alignment_len(GBDATA *gb_main, const char *aliname, long new_len);
char *GBT_get_alignment_type_string(GBDATA *gb_main, const char *aliname);
GB_alignment_type GBT_get_alignment_type(GBDATA *gb_main, const char *aliname);
bool GBT_is_alignment_protein(GBDATA *gb_main, const char *alignment_name);
NOT4PERL char *GBT_read_gene_sequence_and_length(GBDATA *gb_gene, bool use_revComplement, char partSeparator, size_t *gene_length);
char *GBT_read_gene_sequence(GBDATA *gb_gene, bool use_revComplement, char partSeparator);

/* aditem.cxx */
GBDATA *GBT_find_or_create_item_rel_item_data(GBDATA *gb_item_data, const char *itemname, const char *id_field, const char *id, bool markCreated);
GBDATA *GBT_find_or_create_species_rel_species_data(GBDATA *gb_species_data, const char *name);
GBDATA *GBT_find_or_create_species(GBDATA *gb_main, const char *name);
GBDATA *GBT_find_or_create_SAI(GBDATA *gb_main, const char *name);
GBDATA *GBT_find_item_rel_item_data(GBDATA *gb_item_data, const char *id_field, const char *id_value);
GBDATA *GBT_get_species_data(GBDATA *gb_main);
GBDATA *GBT_first_marked_species_rel_species_data(GBDATA *gb_species_data);
GBDATA *GBT_first_marked_species(GBDATA *gb_main);
GBDATA *GBT_next_marked_species(GBDATA *gb_species);
GBDATA *GBT_first_species_rel_species_data(GBDATA *gb_species_data);
GBDATA *GBT_first_species(GBDATA *gb_main);
GBDATA *GBT_next_species(GBDATA *gb_species);
GBDATA *GBT_find_species_rel_species_data(GBDATA *gb_species_data, const char *name);
GBDATA *GBT_find_species(GBDATA *gb_main, const char *name);
GBDATA *GBT_expect_species(GBDATA *gb_main, const char *name);
GBDATA *GBT_get_SAI_data(GBDATA *gb_main);
GBDATA *GBT_first_SAI_rel_SAI_data(GBDATA *gb_sai_data);
GBDATA *GBT_first_SAI(GBDATA *gb_main);
GBDATA *GBT_next_SAI(GBDATA *gb_sai);
GBDATA *GBT_find_SAI_rel_SAI_data(GBDATA *gb_sai_data, const char *name);
GBDATA *GBT_find_SAI(GBDATA *gb_main, const char *name);
GBDATA *GBT_expect_SAI(GBDATA *gb_main, const char *name);
long GBT_get_species_count(GBDATA *gb_main);
long GBT_get_SAI_count(GBDATA *gb_main);
char *GBT_create_unique_species_name(GBDATA *gb_main, const char *default_name);
void GBT_mark_all(GBDATA *gb_main, int flag);
void GBT_mark_all_that(GBDATA *gb_main, int flag, int (*condition)(GBDATA *, void *), void *cd);
long GBT_count_marked_species(GBDATA *gb_main);
char *GBT_store_marked_species(GBDATA *gb_main, bool unmark_all);
NOT4PERL GB_ERROR GBT_with_stored_species(GBDATA *gb_main, const char *stored, species_callback doit, int *clientdata);
GB_ERROR GBT_restore_marked_species(GBDATA *gb_main, const char *stored_marked);
GB_CSTR GBT_read_name(GBDATA *gb_item);
const char *GBT_get_name(GBDATA *gb_item);
GBDATA **GBT_gen_species_array(GBDATA *gb_main, long *pspeccnt);

/* adname.cxx */
GB_ERROR GBT_begin_rename_session(GBDATA *gb_main, int all_flag);
GB_ERROR GBT_rename_species(const char *oldname, const char *newname, bool ignore_protection);
GB_ERROR GBT_abort_rename_session(void);
GB_ERROR GBT_commit_rename_session(void) __ATTR__USERESULT;

/* adseqcompr.cxx */
GB_ERROR GBT_compress_sequence_tree2(GBDATA *gbd, const char *tree_name, const char *ali_name) __ATTR__USERESULT;
void GBT_compression_test(struct Unfixed_cb_parameter *, GBDATA *gb_main);

/* adtables.cxx */
GB_ERROR GBT_install_table_link_follower(GBDATA *gb_main);
GBDATA *GBT_open_table(GBDATA *gb_table_root, const char *table_name, bool read_only);
GBDATA *GBT_first_table(GBDATA *gb_main);
GBDATA *GBT_next_table(GBDATA *gb_table);
GBDATA *GBT_first_table_entry(GBDATA *gb_table);
GBDATA *GBT_next_table_entry(GBDATA *gb_table_entry);
GBDATA *GBT_first_table_field(GBDATA *gb_table);
GBDATA *GBT_next_table_field(GBDATA *gb_table_field);
GBDATA *GBT_find_table_field(GBDATA *gb_table, const char *id);
GBDATA *GBT_open_table_field(GBDATA *gb_table, const char *fieldname, GB_TYPES type_of_field);

/* adtools.cxx */
GBDATA *GBT_create(GBDATA *father, const char *key, long delete_level);
GBDATA *GBT_find_or_create(GBDATA *father, const char *key, long delete_level);
char *GBT_get_default_helix(GBDATA *);
char *GBT_get_default_helix_nr(GBDATA *);
char *GBT_get_default_ref(GBDATA *);
void GBT_scan_db(StrArray& fieldNames, GBDATA *gbd, const char *datapath);
void GBT_install_message_handler(GBDATA *gb_main);
void GBT_message(GBDATA *gb_main, const char *msg);
char *GBT_read_string(GBDATA *gb_container, const char *fieldpath);
char *GBT_read_as_string(GBDATA *gb_container, const char *fieldpath);
const char *GBT_read_char_pntr(GBDATA *gb_container, const char *fieldpath);
NOT4PERL long *GBT_read_int(GBDATA *gb_container, const char *fieldpath);
NOT4PERL float *GBT_read_float(GBDATA *gb_container, const char *fieldpath);
char *GBT_readOrCreate_string(GBDATA *gb_container, const char *fieldpath, const char *default_value);
const char *GBT_readOrCreate_char_pntr(GBDATA *gb_container, const char *fieldpath, const char *default_value);
NOT4PERL long *GBT_readOrCreate_int(GBDATA *gb_container, const char *fieldpath, long default_value);
NOT4PERL float *GBT_readOrCreate_float(GBDATA *gb_container, const char *fieldpath, float default_value);
GB_ERROR GBT_write_string(GBDATA *gb_container, const char *fieldpath, const char *content);
GB_ERROR GBT_write_int(GBDATA *gb_container, const char *fieldpath, long content);
GB_ERROR GBT_write_byte(GBDATA *gb_container, const char *fieldpath, unsigned char content);
GB_ERROR GBT_write_float(GBDATA *gb_container, const char *fieldpath, float content);
GBDATA *GBT_open(const char *path, const char *opent);
GB_ERROR GB_set_macro_error(GBDATA *gb_main, const char *curr_error);
GB_ERROR GB_get_macro_error(GBDATA *gb_main);
GB_ERROR GB_clear_macro_error(GBDATA *gb_main);
NOT4PERL GB_ERROR GBT_remote_action_with_timeout(GBDATA *gb_main, const char *application, const char *action_name, const class ARB_timeout *timeout, bool verbose);
GB_ERROR GBT_remote_action(GBDATA *gb_main, const char *application, const char *action_name);
GB_ERROR GBT_remote_awar(GBDATA *gb_main, const char *application, const char *awar_name, const char *value);
GB_ERROR GBT_remote_read_awar(GBDATA *gb_main, const char *application, const char *awar_name);
const char *GBT_relativeMacroname(const char *macro_name);
GB_ERROR GBT_macro_execute(const char *macro_name, bool loop_marked, bool run_async);
char *GB_generate_notification(GBDATA *gb_main, void (*cb)(const char *message, void *client_data), const char *message, void *client_data);
GB_ERROR GB_remove_last_notification(GBDATA *gb_main);
GB_ERROR GB_notify(GBDATA *gb_main, int id, const char *message);

/* adtree.cxx */
GBDATA *GBT_get_tree_data(GBDATA *gb_main);
TreeNode *GBT_remove_leafs(TreeNode *tree, GBT_TreeRemoveType mode, const GB_HASH *species_hash, int *removed, int *groups_removed);
GB_ERROR GBT_write_group_name(GBDATA *gb_group_name, const char *new_group_name);
GB_ERROR GBT_write_tree(GBDATA *gb_main, const char *tree_name, TreeNode *tree);
GB_ERROR GBT_overwrite_tree(GBDATA *gb_tree, TreeNode *tree);
GB_ERROR GBT_write_tree_remark(GBDATA *gb_main, const char *tree_name, const char *remark);
GB_ERROR GBT_log_to_tree_remark(GBDATA *gb_tree, const char *log_entry);
GB_ERROR GBT_log_to_tree_remark(GBDATA *gb_main, const char *tree_name, const char *log_entry);
GB_ERROR GBT_write_tree_with_remark(GBDATA *gb_main, const char *tree_name, TreeNode *tree, const char *remark);
TreeNode *GBT_read_tree_and_size(GBDATA *gb_main, const char *tree_name, TreeRoot *troot, int *tree_size);
TreeNode *GBT_read_tree(GBDATA *gb_main, const char *tree_name, TreeRoot *troot);
size_t GBT_count_leafs(const TreeNode *tree);
GB_ERROR GBT_is_invalid(const TreeNode *tree);
GB_ERROR GBT_link_tree(TreeNode *tree, GBDATA *gb_main, bool show_status, int *zombies, int *duplicates);
void GBT_unlink_tree(TreeNode *tree);
GBDATA *GBT_find_tree(GBDATA *gb_main, const char *tree_name);
GBDATA *GBT_find_largest_tree(GBDATA *gb_main);
GBDATA *GBT_tree_infrontof(GBDATA *gb_tree);
GBDATA *GBT_tree_behind(GBDATA *gb_tree);
GBDATA *GBT_find_top_tree(GBDATA *gb_main);
GBDATA *GBT_find_bottom_tree(GBDATA *gb_main);
const char *GBT_existing_tree(GBDATA *gb_main, const char *tree_name);
GBDATA *GBT_find_next_tree(GBDATA *gb_tree);
const char *GBT_get_tree_name(GBDATA *gb_tree);
GB_ERROR GBT_check_tree_name(const char *tree_name);
const char *GBT_name_of_largest_tree(GBDATA *gb_main);
const char *GBT_name_of_bottom_tree(GBDATA *gb_main);
const char *GBT_tree_info_string(GBDATA *gb_main, const char *tree_name, int maxTreeNameLen);
long GBT_size_of_tree(GBDATA *gb_main, const char *tree_name);
void GBT_get_tree_names(ConstStrArray& names, GBDATA *gb_main, bool sorted);
NOT4PERL GB_ERROR GBT_move_tree(GBDATA *gb_moved_tree, GBT_ORDER_MODE mode, GBDATA *gb_target_tree);
GB_ERROR GBT_copy_tree(GBDATA *gb_main, const char *source_name, const char *dest_name);
GB_ERROR GBT_rename_tree(GBDATA *gb_main, const char *source_name, const char *dest_name);
GB_CSTR *GBT_get_names_of_species_in_tree(const TreeNode *tree, size_t *count);
char *GBT_tree_2_newick(const TreeNode *tree, NewickFormat format, bool compact);

#else
#error ad_t_prot.h included twice
#endif /* AD_T_PROT_H */
