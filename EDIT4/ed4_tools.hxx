
#define IUPAC_EMPTY 	" "
#define IS_NUCLEOTIDE	(ED4_ROOT->alignment_type==GB_AT_RNA || ED4_ROOT->alignment_type==GB_AT_DNA)
#define IS_AMINO 	(ED4_ROOT->alignment_type==GB_AT_AA)
#define IUPAC_GROUPS 	6

extern int ED4_iupac_group[26];
char ED4_encode_iupac(const char bases[/*4*/], GB_alignment_type ali);
const char* ED4_decode_iupac(char iupac, GB_alignment_type ali);

void ED4_set_clipping_rectangle(AW_rectangle *rect);
// void ED4_reduce_clipping_rectangle(AW_pos x1, AW_pos y1, AW_pos x2, AW_pos y2);

void ED4_aws_init(AW_root *root, AW_window_simple *aws, GB_CSTR macro_format, GB_CSTR window_format, GB_CSTR typeId);

const char *ED4_propertyName(int mode);


