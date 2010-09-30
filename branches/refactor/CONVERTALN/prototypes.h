/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef PROTOTYPES_H
#define PROTOTYPES_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* convert.cxx */
int realloc_sequence_data(int total_seqs);
void free_sequence_data(int used_entries);

/* date.cxx */
const char *genbank_date(const char *other_date);
void find_date(const char *date_string, int *month, int *day, int *year);
bool two_char(const char *str, char determ);
void find_date_long_form(const char *date_string, int *month, int *day, int *year);
int ismonth(const char *str);
int isdatenum(char *string);
int is_genbank_date(const char *str);
const char *today_date(void);
const char *gcg_date(const char *input);

/* embl.cxx */
void init_em_data(void);
void init_embl(void);
char embl_in(FILE_BUFFER fp);
char embl_in_id(FILE_BUFFER fp);
void embl_key_word(char *line, int index, char *key, int length);
int embl_check_blanks(char *line, int numb);
char *embl_continue_line(char *pattern, char **string, char *line, FILE_BUFFER fp);
char *embl_one_entry(char *line, FILE_BUFFER fp, char **entry, char *key);
void embl_verify_title(int refnum);
char *embl_date(char *line, FILE_BUFFER fp);
char *embl_version(char *line, FILE_BUFFER fp);
char *embl_comments(char *line, FILE_BUFFER fp);
char *embl_skip_unidentified(char *pattern, char *line, FILE_BUFFER fp);
int embl_comment_key(char *line, char *key);
char *embl_one_comment_entry(FILE_BUFFER fp, char **datastring, char *line, int start_index);
char *embl_origin(char *line, FILE_BUFFER fp);
void embl_out(FILE *fp);
void embl_print_lines(FILE *fp, const char *key, char *Data, int flag, const char *separators);
void embl_out_comments(FILE *fp);
void embl_print_comment(FILE *fp, const char *key, char *string, int offset, int indent);
void embl_out_origin(FILE *fp);
void embl_to_macke(char *inf, char *outf, int format);
int etom(void);
void embl_to_embl(char *inf, char *outf);
void embl_to_genbank(char *inf, char *outf);
int etog(void);
void etog_reference(void);
char *etog_author(char *string);
char *etog_journal(char *string);
void etog_comments(void);
void genbank_to_embl(char *inf, char *outf);
int gtoe(void);
void gtoe_reference(void);
char *gtoe_author(char *author);
char *gtoe_journal(char *string);
void gtoe_comments(void);
void macke_to_embl(char *inf, char *outf);
int partial_mtoe(void);

/* fconv.cxx */
const char *format2name(int format_type);
void throw_conversion_not_supported(int input_format, int output_format) __ATTR__NORETURN;
void log_processed(int seqCount);
void convert(const char *cinf, const char *coutf, int intype, int outype);
void init(void);
void init_seq_data(void);

/* gcg.cxx */
void to_gcg(char *inf, char *outf, int intype);
void gcg_seq_out(FILE *ofp, char *key);
void gcg_doc_out(char *line, FILE *ofp);
int checksum(char *string, int numofstr);
void gcg_out_origin(FILE *fp);
void gcg_output_filename(char *prefix, char *name);
int gcg_seq_length(void);

/* genbank.cxx */
void init_genbank(void);
char genbank_in(FILE_BUFFER fp);
void genbank_key_word(char *line, int index, char *key, int length);
int genbank_comment_subkey_word(char *line, int index, char *key, int length);
int genbank_check_blanks(char *line, int numb);
char *genbank_continue_line(char **string, char *line, int numb, FILE_BUFFER fp);
char *genbank_one_entry_in(char **datastring, char *line, FILE_BUFFER fp);
char *genbank_one_comment_entry(char **datastring, char *line, int start_index, FILE_BUFFER fp);
char *genbank_source(char *line, FILE_BUFFER fp);
char *genbank_reference(char *line, FILE_BUFFER fp);
const char *genbank_comments(char *line, FILE_BUFFER fp);
char *genbank_origin(char *line, FILE_BUFFER fp);
char *genbank_skip_unidentified(char *line, FILE_BUFFER fp, int blank_num);
void genbank_verify_accession(void);
void genbank_verify_keywords(void);
char genbank_in_locus(FILE_BUFFER fp);
void genbank_out(FILE *fp);
void genbank_out_one_entry(FILE *fp, char *string, const char *key, int flag, const char *patterns, int period);
void genbank_out_one_comment(FILE *fp, char *string, const char *key, int skindent, int cnindent);
void genbank_print_lines(FILE *fp, char *string, int flag, const char *separators);
void genbank_print_comment(FILE *fp, const char *key, char *string, int offset, int indent);
void genbank_out_origin(FILE *fp);
void genbank_to_genbank(char *inf, char *outf);
void init_reference(Reference *ref, int flag);

/* macke.cxx */
void init_macke(void);
char *macke_one_entry_in(FILE_BUFFER fp, const char *key, char *oldname, char **var, char *line, int index);
char *macke_continue_line(const char *key, char *oldname, char **var, char *line, FILE_BUFFER fp);
char *macke_origin(char *key, char *line, FILE_BUFFER fp);
int macke_abbrev(char *line, char *key, int index);
int macke_rem_continue_line(char **strings, int index);
char macke_in_name(FILE_BUFFER fp);
void macke_out_header(FILE *fp);
void macke_out0(FILE *fp, int format);
void macke_out1(FILE *fp);
void macke_print_keyword_rem(int index, FILE *fp);
void macke_print_line_78(FILE *fp, char *line1, char *line2);
int macke_key_word(char *line, int index, char *key, int length);
int macke_in_one_line(char *string);
void macke_out2(FILE *fp);

/* main.cxx */

/* mg.cxx */
void init_gm_data(void);
void genbank_to_macke(char *inf, char *outf);
int gtom(void);
void gtom_remarks(void);
void gtom_copy_remark(char *string, const char *key, int *remnum);
char *genbank_get_strain(void);
char *genbank_get_subspecies(void);
void correct_subspecies(char *subspecies);
char *genbank_get_atcc(void);
char *get_atcc(char *source);
int paren_string(char *string, char *pstring, int index);
int num_of_remark(void);
void macke_to_genbank(char *inf, char *outf);
int mtog(void);
void mtog_decode_ref_and_remarks(void);
void mtog_copy_remark(char **string, int *indi, int indj);
char *macke_copyrem(char **strings, int *index, int maxline, int pointer);
void mtog_genbank_def_and_source(void);
void get_string(char *line, char *temp, int index);
void get_atcc_string(char *line, char *temp, int index);

/* paup.cxx */
void init_paup(void);
void to_paup(char *inf, char *outf, int informat);
void to_paup_1x1(char *inf, char *outf, int informat);
void paup_verify_name(char **string);
void paup_print_line(char *string, char *sequence, int seq_length, int index, int first_line, FILE *fp);
void paup_print_header(FILE *ofp);

/* phylip.cxx */
void init_phylip(void);
void to_phylip(char *inf, char *outf, int informat, int readstdin);
void to_phylip_1x1(char *inf, char *outf, int informat);
void phylip_print_line(char *name, char *sequence, int seq_length, int index, FILE *fp);

/* printable.cxx */
void to_printable(char *inf, char *outf, int informat);
void to_printable_1x1(char *inf, char *outf, int informat);
void printable_print_line(char *id, char *sequence, int start, int base_count, FILE *fp);

/* routines.cxx */
void count_base(int *base_a, int *base_t, int *base_g, int *base_c, int *base_other);
void replace_entry(char **string1, const char *string2);

/* util.cxx */
bool scan_token(const char *from, char *to) __ATTR__USERESULT;
void scan_token_or_die(const char *from, char *to, FILE_BUFFER *fb);
void Freespace(void *pointer);
void throw_error(int error_num, const char *error_message) __ATTR__NORETURN;
void throw_errorf(int error_num, const char *error_messagef, ...) __ATTR__FORMAT(2) __ATTR__NORETURN;
void throw_cant_open_input(const char *filename);
void throw_cant_open_output(const char *filename);
FILE *open_input_or_die(const char *filename);
FILE *open_output_or_die(const char *filename);
void warning(int warning_num, const char *warning_message);
void warningf(int warning_num, const char *warning_messagef, ...) __ATTR__FORMAT(2);
char *Reallocspace(void *block, unsigned int size);
char *Dupstr(const char *string);
char *Catstr(char *s1, const char *s2);
int Lenstr(const char *s1);
void Cpystr(char *s1, const char *s2);
int Skip_white_space(char *line, int index);
int Reach_white_space(char *line, int index);
char *Fgetline(char *line, size_t maxread, FILE_BUFFER fb);
void Getstr(char *line, int linenum);
void Append_char(char **string, char ch);
void Append_rm_eoln(char **string1, const char *string2);
void Append_rp_eoln(char **string1, char *string2);
void Append(char **string1, const char *string2);
int find_pattern(const char *text, const char *pattern);
int not_ending_mark(char ch);
int last_word(char ch);
int is_separator(char ch, const char *separators);
int same_char(char ch1, char ch2);
void Upper_case(char *string);
int Blank_num(char *string);

#else
#error prototypes.h included twice
#endif /* PROTOTYPES_H */
