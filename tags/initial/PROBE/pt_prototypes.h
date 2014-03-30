#ifndef P_
# if defined(__STDC__) || defined(__cplusplus)
#  define P_(s) s
# else
#  define P_(s) ()
# endif
#else
# error P_ already defined elsewhere
#endif


/* PT_new_design.cxx */
extern "C" int pt_init_bond_matrix P_((PT_pdc *THIS));
extern "C" long ptnd_compare_quality P_((void *PT_tprobes_ptr1, void *PT_tprobes_ptr2, char *));
extern "C" long ptnd_compare_sequence P_((void *PT_tprobes_ptr1, void *PT_tprobes_ptr2, char *));
void ptnd_sort_probes_by P_((PT_pdc *pdc, int mode));
void ptnd_probe_delete_all_but P_((PT_pdc *pdc, int count));
int pt_get_gc_content P_((char *probe));
double pt_get_temperature P_((const char *probe));
int ptnd_check_pure P_((char *probe));
void ptnd_calc_quality P_((PT_pdc *pdc));
double ptnd_check_max_bond P_((PT_pdc *pdc, char base));
double ptnd_check_split P_((PT_pdc *pdc, char *probe, int pos, char ref));
int ptnd_chain_count_mishits P_((int name, int apos, int rpos, long dummy));
int ptnd_count_mishits2 P_((POS_TREE *pt));
extern "C" char *get_design_info P_((PT_tprobes *tprobe));
extern "C" char *get_design_hinfo P_((PT_tprobes *tprobe));
int ptnd_count_mishits P_((char *probe, POS_TREE *pt, int height));
void ptnd_first_check P_((PT_pdc *pdc));
void ptnd_check_position P_((PT_pdc *pdc));
void ptnd_check_bonds P_((PT_pdc *pdc, int match));
void ptnd_cp_tprobe_2_probepart P_((PT_pdc *pdc));
void ptnd_duplikate_probepart_rek P_((PT_pdc *pdc, char *insequence, int deep, double dt, double sum_bonds, PT_probeparts *parts));
void ptnd_duplikate_probepart P_((PT_pdc *pdc));
extern "C" long ptnd_compare_parts P_((void *PT_probeparts_ptr1, void *PT_probeparts_ptr2, char *));
void ptnd_sort_parts P_((PT_pdc *pdc));
void ptnd_remove_duplikated_probepart P_((PT_pdc *pdc));
void ptnd_check_part_inc_dt P_((PT_pdc *pdc, PT_probeparts *parts, int name, int apos, int rpos, double dt, double sum_bonds));
int ptnd_check_inc_mode P_((PT_pdc *pdc, PT_probeparts *parts, double dt, double sum_bonds));
int ptnd_chain_check_part P_((int name, int apos, int rpos, long split));
void ptnd_check_part_all P_((POS_TREE *pt, double dt, double sum_bonds));
void ptnd_check_part P_((char *probe, POS_TREE *pt, int height, double dt, double sum_bonds, int split));
void ptnd_check_probepart P_((PT_pdc *pdc));
GB_INLINE int ptnd_check_tprobe P_((PT_pdc *pdc, char *probe, int len));
void ptnd_add_sequence_to_hash P_((PT_pdc *pdc, GB_HASH *hash, register char *sequence, int seq_len, register int probe_len, char *prefix, int prefix_len));
void ptnd_build_tprobes P_((PT_pdc *pdc));
void ptnd_print_probes P_((PT_pdc *pdc));
extern "C" int PT_start_design P_((PT_pdc *pdc, int));
void ptnd_new_match P_((PT_local *locs, char *probestring));

/* PT_family.cxx */
int mark_all_matches_chain_handle P_((int name, int, int, long));
int mark_all_matches P_((PT_local *locs, POS_TREE *pt, char *probe, int length, int mismatches, int height, int max_mismatches));
void clear_statistic P_((void));
void make_match_statistic P_((int probe_len));
int make_PT_family_list P_((PT_local *locs));
int probe_is_ok P_((char *probe, int probe_len, char first_c, char second_c));
extern "C" int find_family P_((PT_local *locs, bytestring *species));

/* PT_prefixtree.cxx */
void PT_init_count_bits P_((void));
char *PTM_get_mem P_((int size));
int PTM_destroy_mem P_((void));
void PTM_free_mem P_((char *data, int size));
void PTM_debug_mem P_((void));
PTM2 *PT_init P_((int base_count));
int PTD_chain_print P_((int name, int apos, int rpos, long clientdata));
int PTD P_((POS_TREE *node));
void PT_change_father P_((POS_TREE *father, POS_TREE *source, POS_TREE *dest));
POS_TREE *PT_add_to_chain P_((PTM2 *ptmain, POS_TREE *node, int name, int apos, int rpos));
POS_TREE *PT_change_leaf_to_node P_((PTM2 *, POS_TREE *node));
POS_TREE *PT_leaf_to_chain P_((PTM2 *ptmain, POS_TREE *node));
POS_TREE *PT_create_leaf P_((PTM2 *ptmain, POS_TREE **pfather, PT_BASES base, int rpos, int apos, int name));
void PTD_clear_fathers P_((PTM2 *ptmain, POS_TREE *node));
void PTD_put_int P_((FILE *out, ulong i));
void PTD_put_short P_((FILE *out, ulong i));
void PTD_set_object_to_saved_status P_((POS_TREE *node, long pos, int size));
long PTD_write_tip_to_disk P_((FILE *out, PTM2 *, POS_TREE *node, long pos));
int ptd_count_chain_entries P_((char *entry));
void ptd_set_chain_references P_((char *entry, char **entry_tab));
void ptd_write_chain_entries P_((FILE *out, long *ppos, PTM2 *, char **entry_tab, int n_entries, int mainapos));
long PTD_write_chain_to_disk P_((FILE *out, PTM2 *ptmain, POS_TREE *node, long pos));
void PTD_debug_nodes P_((void));
long PTD_write_node_to_disk P_((FILE *out, PTM2 *ptmain, POS_TREE *node, long *r_poss, long pos));
int PTD_write_leafs_to_disk P_((FILE *out, PTM2 *ptmain, POS_TREE *node, long pos, long *pnodepos, int *pblock));
void PTD_read_leafs_from_disk P_((char *fname, PTM2 *ptmain, POS_TREE **pnode));

/* PT_main.cxx */
char *pt_init_main_struct P_((PT_main *main, char *filename));
extern "C" int server_shutdown P_((PT_main *pm, aisc_string passwd));
extern "C" int broadcast P_((PT_main *main, int dummy));
void PT_init_psg P_((void));
int main P_((int argc, char **argv));

/* PT_io.cxx */
int compress_data P_((char *probestring));
int PT_base_2_string P_((char *id_string, long len));
void probe_read_data_base P_((char *name));
int probe_compress_sequence P_((char *seq));
char *probe_read_string_append_point P_((GBDATA *gb_data, int *psize));
char *probe_read_alignment P_((int j, int *psize));
void probe_read_alignments P_((void));
void PT_build_species_hash P_((void));
long PT_abs_2_rel P_((long pos));
long PT_rel_2_abs P_((long pos));

/* PT_etc.cxx */
void set_table_for_PT_N_mis P_((void));
void pt_export_error P_((PT_local *locs, const char *error));
extern "C" char *virt_name P_((PT_probematch *ml));
extern "C" char *virt_fullname P_((PT_probematch *ml));
int *table_copy P_((int *mis_table, int length));
void table_add P_((int *mis_tabled, int *mis_tables, int length));
char *ptpd_read_names P_((PT_local *locs, char *names_listi, char *checksumsi));
extern "C" bytestring *PT_unknown_names P_((struct_PT_pdc *pdc));
int get_clip_max_from_length P_((int length));
void PT_init_base_string_counter P_((char *str, char initval, int size));
void PT_inc_base_string_count P_((char *str, char initval, char maxval, int size));

/* PT_secundaer.cxx */

/* PT_buildtree.cxx */
POS_TREE *build_pos_tree P_((POS_TREE *pt, int anfangs_pos, int apos, int RNS_nr, unsigned int end));
long PTD_save_partial_tree P_((FILE *out, PTM2 *ptmain, POS_TREE *node, char *partstring, int partsize, long pos, long *ppos));
GB_INLINE int ptd_string_shorter_than P_((const char *s, int len));
void enter_stage_1_build_tree P_((PT_main *main, char *tname));
void enter_stage_3_load_tree P_((PT_main *main, char *tname));
int PT_chain_debug P_((int name, int apos, int rpos, long height));
void PT_analyse_tree P_((POS_TREE *pt, int height));
void PT_debug_tree P_((void));

/* PT_match.cxx */
GB_INLINE double ptnd_get_wmismatch P_((PT_pdc *pdc, char *probe, int pos, char ref));
int PT_chain_print P_((int name, int apos, int rpos, long ilocs));
int read_names_and_pos P_((PT_local *locs, POS_TREE *pt));
int get_info_about_probe P_((PT_local *locs, char *probe, POS_TREE *pt, int mismatches, double wmismatches, int N_mismatches, int height));
void pt_sort_match_list P_((PT_local *locs));
char *reverse_probe P_((char *probe, int probe_length));
int PT_complement P_((int base));
void complement_probe P_((char *probe, int probe_length));
double calc_position_wmis P_((int pos, int seq_len, double y1, double y2));
void pt_build_pos_to_weight P_((PT_MATCH_TYPE type, const char *sequence));
extern "C" int probe_match P_((PT_local *locs, aisc_string probestring));
extern "C" char *get_match_info P_((PT_probematch *ml));
extern "C" char *get_match_hinfo P_((PT_probematch *ml));
extern "C" bytestring *match_string P_((PT_local *locs));
extern "C" bytestring *MP_match_string P_((PT_local *locs));
extern "C" bytestring *MP_all_species_string P_((PT_local *));
extern "C" int MP_count_all_species P_((PT_local *));

/* probe_tree.hxx */
GB_INLINE char *PT_WRITE_CHAIN_ENTRY P_((char *ptr, int mainapos, int name, int apos, int rpos));
GB_INLINE POS_TREE *PT_read_son P_((PTM2 *ptmain, POS_TREE *node, PT_BASES base));
GB_INLINE POS_TREE *PT_read_son_stage_1 P_((PTM2 *ptmain, POS_TREE *node, PT_BASES base));
GB_INLINE PT_NODE_TYPE PT_read_type P_((POS_TREE *node));
GB_INLINE int PT_read_name P_((PTM2 *ptmain, POS_TREE *node));
GB_INLINE int PT_read_rpos P_((PTM2 *ptmain, POS_TREE *node));
GB_INLINE int PT_read_apos P_((PTM2 *ptmain, POS_TREE *node));
GB_INLINE int PT_read_chain P_((PTM2 *ptmain, POS_TREE *node, int func (int name, int apos, int rpos, long clientdata ), long clientdata));

#undef P_