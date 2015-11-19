// ================================================================= //
//                                                                   //
//   File      : ed4_tools.hxx                                       //
//   Purpose   :                                                     //
//                                                                   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef ED4_TOOLS_HXX
#define ED4_TOOLS_HXX

#define ED4_IUPAC_EMPTY " "

#define IS_NUCLEOTIDE() (ED4_ROOT->alignment_type==GB_AT_RNA || ED4_ROOT->alignment_type==GB_AT_DNA)
#define IS_RNA()        (ED4_ROOT->alignment_type==GB_AT_RNA)
#define IS_DNA()        (ED4_ROOT->alignment_type==GB_AT_DNA)
#define IS_AMINO()      (ED4_ROOT->alignment_type==GB_AT_AA)

char ED4_encode_iupac(const char bases[ /* 4 */ ], GB_alignment_type ali);
const char* ED4_decode_iupac(char iupac, GB_alignment_type ali);

void ED4_set_clipping_rectangle(AW_screen_area *rect);

const char *ED4_propertyName(int mode);

#else
#error ed4_tools.hxx included twice
#endif // ED4_TOOLS_HXX
