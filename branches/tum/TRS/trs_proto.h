#define P_(s) s
long TRS_create_hash P_((long size));
long TRS_read_hash P_((long hash, const char *key));
long TRS_write_hash P_((long hash, const char *key, long val));
long TRS_write_hash_no_strdup P_((long hash, char *key, long val));
long TRS_incr_hash P_((long hash, const char *key));
long TRS_free_hash_entries P_((long hash));
long TRS_free_hash P_((long hash));
long TRS_free_hash_entries_free_pointer P_((long hash));
long TRS_free_hash_free_pointer P_((long hash));
long TRS_hash_do_loop P_((long hash, long func (const char *key, long val )));
long TRS_create_hashi P_((long size));
long TRS_read_hashi P_((long hashi, long key));
long TRS_write_hashi P_((long hashi, long key, long val));
long TRS_free_hashi P_((long hash));
char *TRS_export_error P_((const char *templat, ...));
char *TRS_get_error P_((void));
void *TRS_stropen P_((long init_size));
char *TRS_strclose P_((void *strstruct, int optimize));
void TRS_strcat P_((void *strstruct, const char *ptr));
void TRS_chrcat P_((void *strstruct, char ch));
char *TRS_mergesort P_((void **array, long start, long end, long (*compare )(void *, void *, char *cd ), char *client_data));
char *TRS_read_file P_((const char *path));
char *TRS_map_FILE P_((FILE *in, int writeable));
char *TRS_map_file P_((const char *path, int writeable));
long TRS_size_of_file P_((const char *path));
long TRS_size_of_FILE P_((FILE *in));
char *T2J_send_tree P_((CAT_node_id focus));
char *T2J_transform P_((int mode, char *path_of_tree, struct T2J_transfer_struct *data, CAT_node_id focus, FILE *out));
char *T2J_send_bit_coded_tree P_((char *path_of_tree, FILE *out));
char *T2J_send_branch_lengths P_((char *path_of_tree, FILE *out));
char *T2J_send_newick_tree P_((const char *path_of_tree, char *changedNodes, char *selectedNodes, const char *grouped_nodes, FILE *out));
struct T2J_transfer_struct *T2J_read_query_result_from_data P_((char *data, CAT_FIELDS catfield));
struct T2J_transfer_struct *T2J_read_query_result_from_pts P_((char *data));
struct T2J_transfer_struct *T2J_read_query_result_from_file P_((char *path, CAT_FIELDS catfield));
char *T2J_transfer_fullnames1 P_((char *path_of_tree, FILE *out));
char *T2J_transfer_fullnames2 P_((char *path_of_tree, FILE *out));
char *T2J_transfer_group_names P_((char *path_of_tree, FILE *out));
void T2J_convert_colors_into_selection P_((void));
char *T2J_get_selection P_((char *path_of_tree, char *sel, const char *varname, int all_nodes, CAT_FIELDS field_name, CAT_node_id *focusout, char **maxnodeout, double *maxnodehits));
void T2J_set_color_of_selection P_((char *sel));
#undef P_
