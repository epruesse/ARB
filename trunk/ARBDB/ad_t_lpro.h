#ifndef P_
# error P_ is not defined
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* adtools.c */
char *gbt_insert_delete P_((const char *source, long len, long destlen, long *newsize, long pos, long nchar, long mod, char insert_what, char insert_tail));
GB_ERROR gbt_insert_character_gbd P_((GBDATA *gb_data, long len, long pos, long nchar, const char *delete_chars, const char *species_name));
GB_ERROR gbt_insert_character_species P_((GBDATA *gb_species, const char *ali_name, long len, long pos, long nchar, const char *delete_chars));
GB_ERROR gbt_insert_character P_((GBDATA *gb_species_data, const char *species, const char *name, long len, long pos, long nchar, const char *delete_chars));
long gbt_write_tree_nodes P_((GBDATA *gb_tree, GBT_TREE *node, long startid));
GB_CPNTR gbt_write_tree_rek_new P_((GBDATA *gb_tree, GBT_TREE *node, char *dest, long mode));
GBT_TREE *gbt_read_tree_rek P_((char **data, long *startid, GBDATA **gb_tree_nodes, long structure_size, int size_of_tree));
GB_ERROR gbt_link_tree_to_hash_rek P_((GBT_TREE *tree, GBDATA *gb_species_data, long nodes, long *counter));
double gbt_read_number P_((FILE *input));
char *gbt_read_quoted_string P_((FILE *input));
int gbt_sum_leafs P_((GBT_TREE *tree));
GB_CSTR *gbt_fill_species_names P_((GB_CSTR *des, GBT_TREE *tree));
void gbt_export_tree_node_print_remove P_((char *str));
void gbt_export_tree_rek P_((GBT_TREE *tree, FILE *out));
void gbt_scan_db_rek P_((GBDATA *gbd, char *prefix, int deep));
long gbs_scan_db_count P_((const char *key, long val));
long gbs_scan_db_insert P_((const char *key, long val, void *v_datapath));
long gbs_scan_db_compare P_((const char *left, const char *right));
GB_ERROR gbt_rename_tree_rek P_((GBT_TREE *tree, int tree_index));

/* adseqcompr.c */
char *gb_compress_seq_by_master P_((const char *master, int master_len, int master_index, GBQUARK q, const char *seq, long seq_len, long *memsize, int old_flag));
char *gb_compress_sequence_by_master P_((GBDATA *gbd, const char *master, int master_len, int master_index, GBQUARK q, const char *seq, int seq_len, long *memsize));
char *gb_uncompress_by_sequence P_((GBDATA *gbd, const char *s, long size, GB_ERROR *error));

/* adtables.c */
GBDATA *gbt_table_link_follower P_((GBDATA *gb_main, GBDATA *gb_link, const char *link));

/* adRevCompl.c */

#ifdef __cplusplus
}
#endif
