#ifndef P_
# if defined(__STDC__) || defined(__cplusplus)
#  define P_(s) s
# else
#  define P_(s) ()
# endif
#else
# error P_ already defined elsewhere
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* main.c */
int main P_((int argc, char **argv));
int file_type P_((char *filename));
int isnum P_((char *string));
int file_exist P_((char *file_name));
void change_file_suffix P_((char *old_file, char *file_name, int type));

/* fconv.c */
void convert P_((char *inf, char *outf, int intype, int outype));
void init P_((void));
void init_seq_data P_((void));

/* mg.c */
void init_gm_data P_((void));
void genbank_to_macke P_((char *inf, char *outf));
int gtom P_((void));
void gtom_remarks P_((void));
void gtom_copy_remark P_((char *string, const char *key, int *remnum));
char *genbank_get_strain P_((void));
char *genbank_get_subspecies P_((void));
void correct_subspecies P_((char *subspecies));
char *genbank_get_atcc P_((void));
char *get_atcc P_((char *source));
int paren_string P_((char *string, char *pstring, int index));
int num_of_remark P_((void));
void macke_to_genbank P_((char *inf, char *outf));
int mtog P_((void));
void mtog_decode_ref_and_remarks P_((void));
void mtog_copy_remark P_((char **string, int *indi, int indj));
char *macke_copyrem P_((char **strings, int *index, int maxline, int pointer));
void mtog_genbank_def_and_source P_((void));
void get_string P_((char *line, char *temp, int index));
void get_atcc_string P_((char *line, char *temp, int index));

/* genbank.c */
void init_genbank P_((void));
char genbank_in P_((FILE *fp));
void genbank_key_word P_((char *line, int index, char *key, int length));
int genbank_comment_subkey_word P_((char *line, int index, char *key, int length));
int genbank_check_blanks P_((char *line, int numb));
char *genbank_continue_line P_((char **string, char *line, int numb, FILE *fp));
char *genbank_one_entry_in P_((char **datastring, char *line, FILE *fp));
char *genbank_one_comment_entry P_((char **datastring, char *line, int start_index, FILE *fp));
char *genbank_source P_((char *line, FILE *fp));
char *genbank_reference P_((char *line, FILE *fp));
const char *genbank_comments P_((char *line, FILE *fp));
char *genbank_origin P_((char *line, FILE *fp));
char *genbank_skip_unidentified P_((char *line, FILE *fp, int blank_num));
void genbank_verify_accession P_((void));
void genbank_verify_keywords P_((void));
char genbank_in_locus P_((FILE *fp));
void genbank_out P_((FILE *fp));
void genbank_out_one_entry P_((FILE *fp, char *string, const char *key, int flag, const char *patterns, int period));
void genbank_out_one_comment P_((FILE *fp, char *string, const char *key, int skindent, int cnindent));
void genbank_print_lines P_((FILE *fp, char *string, int flag, const char *separators));
void genbank_print_comment P_((FILE *fp, const char *key, char *string, int offset, int indent));
void genbank_out_origin P_((FILE *fp));
void genbank_to_genbank P_((char *inf, char *outf));
void init_reference P_((Reference *ref, int flag));

/* macke.c */
void init_macke P_((void));
char macke_in P_((FILE *fp1, FILE *fp2, FILE *fp3));
char *macke_one_entry_in P_((FILE *fp, const char *key, char *oldname, char **var, char *line, int index));
char *macke_continue_line P_((const char *key, char *oldname, char **var, char *line, FILE *fp));
char *macke_origin P_((char *key, char *line, FILE *fp));
int macke_abbrev P_((char *line, char *key, int index));
int macke_rem_continue_line P_((char **strings, int index));
char macke_in_name P_((FILE *fp));
void macke_out_header P_((FILE *fp));
void macke_out0 P_((FILE *fp, int format));
void macke_out1 P_((FILE *fp));
void macke_print_keyword_rem P_((int index, FILE *fp));
void macke_print_line_78 P_((FILE *fp, char *line1, char *line2));
int macke_key_word P_((char *line, int index, char *key, int length));
int macke_in_one_line P_((char *string));
void macke_out2 P_((FILE *fp));

/* phylip.c */
void init_phylip P_((void));
void to_phylip P_((char *inf, char *outf, int informat, int readstdin));
void to_phylip_1x1 P_((char *inf, char *outf, int informat));
void phylip_print_line P_((char *name, char *sequence, int index, FILE *fp));

/* paup.c */
void init_paup P_((void));
void to_paup P_((char *inf, char *outf, int informat));
void to_paup_1x1 P_((char *inf, char *outf, int informat));
void paup_verify_name P_((char **string));
void paup_print_line P_((char *string, char *sequence, int index, int first_line, FILE *fp));
void paup_print_header P_((FILE *ofp));

/* util.c */
int Cmpcasestr P_((const char *s1, const char *s2));
int Cmpstr P_((const char *s1, const char *s2));
void Freespace P_((void *pointer));
void error P_((int error_num, const char *error_message));
void warning P_((int warning_num, const char *warning_message));
char *Reallocspace P_((void *block, unsigned size));
char *Dupstr P_((const char *string));
char *Catstr P_((char *s1, const char *s2));
int Lenstr P_((const char *s1));
void Cpystr P_((char *s1, const char *s2));
int Skip_white_space P_((char *line, int index));
int Reach_white_space P_((char *line, int index));
char *Fgetline P_((char *line, int linenum, FILE *fp));
void Getstr P_((char *line, int linenum));
void Append_char P_((char **string, int ch));
void Append_rm_eoln P_((char **string1, const char *string2));
void Append_rp_eoln P_((char **string1, char *string2));
void Append P_((char **string1, const char *string2));
int find_pattern P_((const char *text, const char *pattern));
int not_ending_mark P_((int ch));
int last_word P_((int ch));
int is_separator P_((int ch, const char *separators));
int same_char P_((int ch1, int ch2));
void Upper_case P_((char *string));
int Blank_num P_((char *string));

/* date.c */
char *genbank_date P_((char *other_date));
void find_date P_((char *date_string, int *month, int *day, int *year));
int two_char P_((char *string, int determ));
void find_date_long_form P_((char *date_string, int *month, int *day, int *year));
int ismonth P_((char *string));
int isdatenum P_((char *string));
int is_genbank_date P_((char *string));
char *today_date P_((void));
char *gcg_date P_((char *date_string));

/* embl.c */
void init_em_data P_((void));
void init_embl P_((void));
char embl_in P_((FILE *fp));
char embl_in_id P_((FILE *fp));
void embl_key_word P_((char *line, int index, char *key, int length));
int embl_check_blanks P_((char *line, int numb));
char *embl_continue_line P_((char *pattern, char **string, char *line, FILE *fp));
char *embl_one_entry P_((char *line, FILE *fp, char **entry, char *key));
void embl_verify_title P_((int refnum));
char *embl_date P_((char *line, FILE *fp));
char *embl_version P_((char *line, FILE *fp));
char *embl_comments P_((char *line, FILE *fp));
char *embl_skip_unidentified P_((char *pattern, char *line, FILE *fp));
int embl_comment_key P_((char *line, char *key));
char *embl_one_comment_entry P_((FILE *fp, char **datastring, char *line, int start_index));
char *embl_origin P_((char *line, FILE *fp));
void embl_out P_((FILE *fp));
void embl_print_lines P_((FILE *fp, const char *key, char *data, int flag, const char *separators));
void embl_out_comments P_((FILE *fp));
void embl_print_comment P_((FILE *fp, const char *key, char *string, int offset, int indent));
void embl_out_origin P_((FILE *fp));
void embl_to_macke P_((char *inf, char *outf, int format));
int etom P_((void));
void embl_to_embl P_((char *inf, char *outf, int format));
void embl_to_genbank P_((char *inf, char *outf, int format));
int etog P_((void));
void etog_reference P_((void));
char *etog_author P_((char *string));
char *etog_journal P_((char *string));
void etog_comments P_((void));
void genbank_to_embl P_((char *inf, char *outf));
int gtoe P_((void));
void gtoe_reference P_((void));
char *gtoe_author P_((char *author));
char *gtoe_journal P_((char *string));
void gtoe_comments P_((void));
void macke_to_embl P_((char *inf, char *outf));
int partial_mtoe P_((void));

/* gcg.c */
void to_gcg P_((int intype, char *inf));
void gcg_seq_out P_((FILE *ofp, char *key));
void gcg_doc_out P_((char *line, FILE *ofp));
int checksum P_((char *string, int numofstr));
void gcg_out_origin P_((FILE *fp));
void gcg_output_filename P_((char *prefix, char *name));
int gcg_seq_length P_((void));

/* printable.c */
void to_printable P_((char *inf, char *outf, int informat));
void to_printable_1x1 P_((char *inf, char *outf, int informat));
void printable_print_line P_((char *id, char *sequence, int start, int base_count, FILE *fp));

/* alma.c */
void init_alma P_((void));
void alma_to_macke P_((char *inf, char *outf));
void alma_to_genbank P_((char *inf, char *outf));
char alma_in P_((FILE *fp));
int alma_key_word P_((char *line, int index, char *key, int length));
void alma_one_entry P_((char *line, int index, char **datastring));
char *alma_in_gaps P_((FILE *fp));
void alma_in_sequence P_((void));
void nbrf_in P_((FILE *fp));
void gcg_in P_((FILE *fp));
void staden_in P_((FILE *fp));
int atom P_((void));
void embl_to_alma P_((char *inf, char *outf));
void genbank_to_alma P_((char *inf, char *outf));
void macke_to_alma P_((char *inf, char *outf));
int etoa P_((void));
void alma_out_header P_((FILE *fp));
void alma_out P_((FILE *fp, int format));
void alma_out_entry_header P_((FILE *fp, char *entry_id, char *filename, int format_type));
void alma_out_gaps P_((FILE *fp));

/* routines.c */
void count_base P_((int *base_a, int *base_t, int *base_g, int *base_c, int *base_other));
void replace_entry P_((char **string1, const char *string2));

#ifdef __cplusplus
}
#endif

#undef P_
