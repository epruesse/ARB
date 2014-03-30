/*
 * This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 *
 */

#ifndef PT_PROTOTYPES_H
#define PT_PROTOTYPES_H

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
int ptnd_check_pure P_((char *probe));
double ptnd_check_split P_((PT_pdc *pdc, char *probe, int pos, char ref));
extern "C" char *get_design_info P_((PT_tprobes *tprobe));
extern "C" char *get_design_hinfo P_((PT_tprobes *tprobe));
extern "C" int PT_start_design P_((PT_pdc *pdc, int dummy_1x));
void ptnd_new_match P_((PT_local *locs, char *probestring));

/* PT_family.cxx */
extern "C" int ff_find_family P_((PT_local *locs, bytestring *species));

/* PT_prefixtree.cxx */
void PT_init_count_bits P_((void));
char *PTM_get_mem P_((int size));
int PTM_destroy_mem P_((void));
void PTM_free_mem P_((char *data, int size));
void PTM_debug_mem P_((void));
PTM2 *PT_init P_((int base_count));
int PTD P_((POS_TREE *node));
void PT_change_father P_((POS_TREE *father, POS_TREE *source, POS_TREE *dest));
POS_TREE *PT_add_to_chain P_((PTM2 *ptmain, POS_TREE *node, int name, int apos, int rpos));
POS_TREE *PT_change_leaf_to_node P_((PTM2 *, POS_TREE *node));
POS_TREE *PT_leaf_to_chain P_((PTM2 *ptmain, POS_TREE *node));
POS_TREE *PT_create_leaf P_((PTM2 *ptmain, POS_TREE **pfather, PT_BASES base, int rpos, int apos, int name));
void PTD_clear_fathers P_((PTM2 *ptmain, POS_TREE *node));
void PTD_put_longlong P_((FILE *out, ULONG i));
void PTD_put_int P_((FILE *out, ULONG i));
void PTD_put_short P_((FILE *out, ULONG i));
void PTD_set_object_to_saved_status P_((POS_TREE *node, long pos, int size));
long PTD_write_tip_to_disk P_((FILE *out, PTM2 *, POS_TREE *node, long pos));
int ptd_count_chain_entries P_((char *entry));
void ptd_set_chain_references P_((char *entry, char **entry_tab));
void ptd_write_chain_entries P_((FILE *out, long *ppos, PTM2 *, char **entry_tab, int n_entries, int mainapos));
long PTD_write_chain_to_disk P_((FILE *out, PTM2 *ptmain, POS_TREE *node, long pos));
void PTD_debug_nodes P_((void));
long PTD_write_node_to_disk P_((FILE *out, PTM2 *ptmain, POS_TREE *node, long *r_poss, long pos));
long PTD_write_leafs_to_disk P_((FILE *out, PTM2 *ptmain, POS_TREE *node, long pos, long *pnodepos, int *pblock));
void PTD_read_leafs_from_disk P_((char *fname, PTM2 *ptmain, POS_TREE **pnode));

/* PT_main.cxx */
char *pt_init_main_struct P_((PT_main *main, char *filename));
extern "C" int server_shutdown P_((PT_main *pm, aisc_string passwd));
extern "C" int broadcast P_((PT_main *main, int dummy));
void PT_init_psg P_((void));
void PT_init_map P_((void));

/* PT_io.cxx */
int compress_data P_((char *probestring));
void PT_base_2_string P_((char *id_string, long len));
void probe_read_data_base P_((char *name));
int probe_compress_sequence P_((char *seq, int seqsize));
char *probe_read_alignment P_((int j, int *psize));
void probe_read_alignments P_((void));
void PT_build_species_hash P_((void));
long PT_abs_2_rel P_((long pos));
long PT_rel_2_abs P_((long pos));

/* PT_etc.cxx */
void set_table_for_PT_N_mis P_((void));
void pt_export_error P_((PT_local *locs, const char *error));
extern "C" const char *virt_name P_((PT_probematch *ml));
extern "C" const char *virt_fullname P_((PT_probematch *ml));
int *table_copy P_((int *mis_table, int length));
void table_add P_((int *mis_tabled, int *mis_tables, int length));
char *ptpd_read_names P_((PT_local *locs, const char *names_list, const char *checksums, const char *&error));
extern "C" bytestring *PT_unknown_names P_((struct_PT_pdc *pdc));
int get_clip_max_from_length P_((int length));
void PT_init_base_string_counter P_((char *str, char initval, int size));
void PT_inc_base_string_count P_((char *str, char initval, char maxval, int size));

/* PT_buildtree.cxx */
POS_TREE *build_pos_tree P_((POS_TREE *pt, int anfangs_pos, int apos, int RNS_nr, unsigned int end));
long PTD_save_partial_tree P_((FILE *out, PTM2 *ptmain, POS_TREE *node, char *partstring, int partsize, long pos, long *ppos));
void enter_stage_1_build_tree P_((PT_main *main, char *tname));
void enter_stage_3_load_tree P_((PT_main *main, char *tname));
void PT_analyse_tree P_((POS_TREE *pt, int height));
void PT_debug_tree P_((void));

/* PT_match.cxx */
int read_names_and_pos P_((PT_local *locs, POS_TREE *pt));
int get_info_about_probe P_((PT_local *locs, char *probe, POS_TREE *pt, int mismatches, double wmismatches, int N_mismatches, int height));
void pt_sort_match_list P_((PT_local *locs));
char *reverse_probe P_((char *probe, int probe_length));
int PT_complement P_((int base));
void complement_probe P_((char *probe, int probe_length));
void pt_build_pos_to_weight P_((PT_MATCH_TYPE type, const char *sequence));
extern "C" int probe_match P_((PT_local *locs, aisc_string probestring));
extern "C" bytestring *match_string P_((PT_local *locs));
extern "C" bytestring *MP_match_string P_((PT_local *locs));
extern "C" bytestring *MP_all_species_string P_((PT_local *));
extern "C" int MP_count_all_species P_((PT_local *));

/* PT_findEx.cxx */
extern "C" int PT_find_exProb P_((PT_exProb *pep));

/* probe_tree.hxx */
GB_INLINE const char *PT_READ_CHAIN_ENTRY P_((const char *ptr, int mainapos, int *name, int *apos, int *rpos));
GB_INLINE char *PT_WRITE_CHAIN_ENTRY P_((const char *const ptr, const int mainapos, int name, const int apos, const int rpos));
GB_INLINE POS_TREE *PT_read_son P_((PTM2 *ptmain, POS_TREE *node, PT_BASES base));
GB_INLINE POS_TREE *PT_read_son_stage_1 P_((PTM2 *ptmain, POS_TREE *node, PT_BASES base));
GB_INLINE PT_NODE_TYPE PT_read_type P_((POS_TREE *node));
GB_INLINE int PT_read_name P_((PTM2 *ptmain, POS_TREE *node));
GB_INLINE int PT_read_rpos P_((PTM2 *ptmain, POS_TREE *node));
GB_INLINE int PT_read_apos P_((PTM2 *ptmain, POS_TREE *node));
template <typename T >int PT_read_chain P_((PTM2 *ptmain, POS_TREE *node, T func));

#undef P_

#else
#error pt_prototypes.h included twice
#endif /* PT_PROTOTYPES_H */