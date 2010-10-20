/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef PROTOTYPES_H
#define PROTOTYPES_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* date.cxx */
const char *genbank_date(const char *other_date);
const char *today_date(void);
const char *gcg_date(const char *input);

/* embl.cxx */
void embl_in(Embl &embl, Seq &seq, Reader &reader);
void embl_in_simple(Embl &embl, Seq &seq, Reader &reader);
void embl_key_word(const char *line, int index, char *key, int length);
void embl_origin(Seq &seq, Reader &reader);
void embl_to_macke(const char *inf, const char *outf, Format inType);
void embl_to_embl(const char *inf, const char *outf);
void embl_to_genbank(const char *inf, const char *outf);
void genbank_to_embl(const char *inf, const char *outf);
void macke_to_embl(const char *inf, const char *outf);

/* fconv.cxx */
void throw_unsupported_input_format(Format inType) __ATTR__NORETURN;
void throw_conversion_not_supported(Format inType, Format ouType) __ATTR__NORETURN;
void throw_conversion_failure(Format inType, Format ouType) __ATTR__NORETURN;
void throw_incomplete_entry(void) __ATTR__NORETURN;
void log_processed(int seqCount);
void convert(const char *inf, const char *outf, Format inType, Format ouType);

/* gcg.cxx */
void to_gcg(const char *inf, const char *outf, Format inType);

/* genbank.cxx */
void genbank_key_word(const char *line, int index, char *key, int length);
void genbank_one_entry_in(char *&datastring, Reader &reader);
void genbank_source(GenBank &gbk, Reader &reader);
void genbank_skip_unidentified(Reader &reader, int blank_num);
void genbank_reference(GenBank &gbk, Reader &reader);
void genbank_in(GenBank &gbk, Seq &seq, Reader &reader);
void genbank_origin(Seq &seq, Reader &reader);
void genbank_in_simple(GenBank &gbk, Seq &seq, Reader &reader);
void genbank_out(const GenBank &gbk, const Seq &seq, Writer &write);
void genbank_to_genbank(const char *inf, const char *outf);

/* macke.cxx */
void macke_origin(Seq &seq, const char *key, Reader &reader);
int macke_abbrev(const char *line, char *key, int index);
bool macke_is_continued_remark(const char *str);
void macke_in_simple(Macke &macke, Seq &seq, Reader &reader);
void macke_out_header(Writer &write);
void macke_seq_display_out(const Macke &macke, Writer &write, Format inType, bool first_sequence);
void macke_seq_info_out(const Macke &macke, Writer &write);
int macke_key_word(const char *line, int index, char *key, int length);
void macke_seq_data_out(const Seq &seq, const Macke &macke, Writer &write);

/* main.cxx */

/* mg.cxx */
void genbank_to_macke(const char *inf, const char *outf);
void macke_to_genbank(const char *inf, const char *outf);
int mtog(const Macke &macke, GenBank &gbk, const Seq &seq) __ATTR__USERESULT;
int gtom(const GenBank &gbk, Macke &macke) __ATTR__USERESULT;

/* paup.cxx */
void to_paup(const char *inf, const char *outf, Format inType);

/* phylip.cxx */
void to_phylip(const char *inf, const char *outf, Format inType, int readstdin);

/* printable.cxx */
void to_printable(const char *inf, const char *outf, Format inType);

/* rdp_info.cxx */
bool parse_RDP_comment(RDP_comments &comments, RDP_comment_parser one_comment_entry, const char *key, int index, Reader &reader);

/* util.cxx */
bool scan_token(char *to, const char *from) __ATTR__USERESULT;
void scan_token_or_die(char *to, const char *from);
void scan_token_or_die(char *to, Reader &reader, int offset);
void throw_error(int error_num, const char *error_message) __ATTR__NORETURN;
char *strf(const char *format, ...) __ATTR__FORMAT(1);
void throw_errorf(int error_num, const char *error_messagef, ...) __ATTR__FORMAT(2) __ATTR__NORETURN;
FILE *open_input_or_die(const char *filename);
FILE *open_output_or_die(const char *filename);
void warning(int warning_num, const char *warning_message);
void warningf(int warning_num, const char *warning_messagef, ...) __ATTR__FORMAT(2);
char *Reallocspace(void *block, unsigned int size);
int Skip_white_space(const char *line, int index);
char *Fgetline(char *line, size_t maxread, FILE_BUFFER fb);
void Getstr(char *line, int linenum);
void terminate_with(char *&str, char ch);
void skip_eolnl_and_append(char *&string1, const char *string2);
void skip_eolnl_and_append_spaced(char *&string1, const char *string2);
void Append(char *&string1, const char *string2);
void Append(char *&string1, char ch);
void upcase(char *str);
int fputs_len(const char *str, int len, Writer &write);
int find_pattern(const char *text, const char *pattern);
int skip_pattern(const char *text, const char *pattern);
int find_subspecies(const char *str, char expect_behind);
int skip_subspecies(const char *str, char expect_behind);
int find_strain(const char *str, char expect_behind);
int skip_strain(const char *str, char expect_behind);
const char *stristr(const char *str, const char *substring);
int ___lookup_keyword(const char *keyword, const char *const *lookup_table, int lookup_table_size);

#else
#error prototypes.h included twice
#endif /* PROTOTYPES_H */
