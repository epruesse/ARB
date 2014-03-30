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
GBDATA *GBT_get_species_data P_((GBDATA *gb_main));
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
char *GBT_store_marked_species P_((GBDATA *gb_main, int unmark_all));
GB_ERROR GBT_with_stored_species P_((GBDATA *gb_main, const char *stored, species_callback doit, int *clientdata));
GB_ERROR GBT_restore_marked_species P_((GBDATA *gb_main, const char *stored_marked));
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
char *GBT_read_gene_sequence P_((GBDATA *gb_gene));

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

/* adRevCompl.c */
char GBT_complementNucleotide P_((char c, char T_or_U));
char *GBT_reverseNucSequence P_((const char *s, int len));
char *GBT_complementNucSequence P_((const char *s, int len, char T_or_U));
GB_ERROR GBT_reverseComplementNucSequence P_((char *seq, long length, GB_alignment_type alignment_type));

#ifdef __cplusplus
}
#endif