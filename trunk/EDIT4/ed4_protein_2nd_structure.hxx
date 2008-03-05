#ifndef ED4_PROTEIN_2ND_STRUCTURE_HXX
#define ED4_PROTEIN_2ND_STRUCTURE_HXX

#ifndef AW_WINDOW_HXX
#include "aw_window.hxx"
#endif

//#define SHOW_PROGRESS
//#define STEP_THROUGH_FIND_NS
//#define STEP_THROUGH_EXT_NS
#define NOCREATE_DISABLED
#define NOREPLACE_DISABLED
//#define START_EXTENSION_INSIDE
//TODO: check this. can hang or crash if enabled! 

#define PFOLD_AWAR_ENABLE            "Pfold/enable"       /// Enable structure match
#define PFOLD_AWAR_SELECTED_SAI      "Pfold/selected_SAI" /// Selected SAI (the SAI that is used for structure comparison)
#define PFOLD_AWAR_PAIR_TEMPLATE     "Pfold/pairs/%s"     /// Structure pairs that define the match quality
#define PFOLD_AWAR_SYMBOL_TEMPLATE   "Pfold/symbols/%s"   /// Symbols that define the match quality
#define PFOLD_AWAR_SYMBOL_TEMPLATE_2 "Pfold/symbols2"     /// Symbols that define the match quality
#define PFOLD_AWAR_MATCH_METHOD      "Pfold/match_method" /// Method for computing the match quality
#define PFOLD_AWAR_SAI_FILTER        "Pfold/SAI_filter"   /// Filter SAIs for given criteria (string), used in option menu for SAI selection

static const char *sequence;       /// Pointer to sequence of protein
static int length;                 /// Length of sequence
static char *structures[4];        /// Predicted structures
static char *probabilities[2];     /// Probabilities of prediction

static const char *amino_acids = "ARNDCQEGHILKMFPSTWYV"; /// Specifies the characters used for amino acid one letter code
static int *char2AA = 0; /// Maps characters to amino acid one letter code
enum structure {three_turn = 4, four_turn = 5, five_turn = 6, helix = 0, sheet = 1, beta_turn = 2, summary = 3}; /// Defines the structure types

typedef enum {
    STRUCT_PERFECT_MATCH,
    STRUCT_GOOD_MATCH,
    STRUCT_MEDIUM_MATCH,
    STRUCT_BAD_MATCH,
    STRUCT_NO_MATCH,
    STRUCT_UNKNOWN,
    PFOLD_MATCH_TYPE_COUNT
} PFOLD_MATCH_TYPE;

struct {
    const char *awar;
    PFOLD_MATCH_TYPE match_type;
} static pfold_match_type_awars[] = {
    { "Perfect_match", STRUCT_PERFECT_MATCH   },
    { "Good_match",    STRUCT_GOOD_MATCH      },
    { "Medium_match",  STRUCT_MEDIUM_MATCH    },
    { "Bad_match",     STRUCT_BAD_MATCH       },
    { "No_match",      STRUCT_NO_MATCH        },
    { "Unknown_match", STRUCT_UNKNOWN         },
    { 0,               PFOLD_MATCH_TYPE_COUNT }
};

static char *pfold_pairs[] = {
    strdup("HH EE TT"),           // STRUCT_PERFECT_MATCH
    strdup("HG HI HS EB ES TS"),  // STRUCT_GOOD_MATCH
    strdup("HT GT IT"),           // STRUCT_MEDIUM_MATCH
    strdup("ET BT"),              // STRUCT_BAD_MATCH
    strdup("EH BH EG EI"),        // STRUCT_NO_MATCH
    strdup("  ")                  // STRUCT_UNKNOWN
};

static char *pfold_pair_chars[] = {
    strdup(" "), // STRUCT_PERFECT_MATCH
    strdup("-"), // STRUCT_GOOD_MATCH
    strdup("~"), // STRUCT_MEDIUM_MATCH
    strdup("+"), // STRUCT_BAD_MATCH
    strdup("#"), // STRUCT_NO_MATCH
    strdup("?")  // STRUCT_UNKNOWN
};

static char pfold_pair_chars_2[] = "##++~~--  "; 
//static char *pfold_pair_chars_2[] = {
//    strdup("#"),
//    strdup("#"),
//    strdup("+"),
//    strdup("+"),
//    strdup("~"),
//    strdup("~"),
//    strdup("-"),
//    strdup("-"),
//    strdup(" "),
//    strdup(" ")
//};

typedef enum {
    SECSTRUCT,
    SECSTRUCT_SEQUENCE,
    SECSTRUCT_SEQUENCE_PREDICT,
    PFOLD_MATCH_METHOD_COUNT
} PFOLD_MATCH_METHOD;

struct {
    const char *awar;
    PFOLD_MATCH_METHOD match_method;
} static pfold_match_method_awars[] = {
    { "Secondary Structure <-> Secondary Structure",        SECSTRUCT                  },
    { "Secondary Structure <-> Sequence",                   SECSTRUCT_SEQUENCE         },
    { "Secondary Structure <-> Sequence (Full Prediction)", SECSTRUCT_SEQUENCE_PREDICT },
    { 0,                                                    PFOLD_MATCH_METHOD_COUNT   }
};

static double cf_parameters[20][4] = {
    /*Helix Former   Strand Former   Helix Breaker   Strand Breaker   Amino
      Value (helix)  Value (sheet)   Value (helix+2) Value (sheet+2)  Acid
    -----------------------------------------------------------------------*/
    { 1.34,          0.00,           0.00,           0.00 },          // A
    { 0.00,          0.00,           0.00,           0.00 },          // R
    { 0.50,          0.00,           0.00,           1.39 },          // D
    { 0.00,          0.00,           1.03,           0.00 },          // N
    { 0.00,          1.13,           0.00,           0.00 },          // C
    { 1.42,          0.00,           0.00,           2.03 },          // E
    { 1.05,          1.05,           0.00,           0.00 },          // Q
    { 0.00,          0.00,           1.21,           1.00 },          // G
    { 0.50,          0.00,           0.00,           0.00 },          // H
    { 1.02,          1.52,           0.00,           0.00 },          // I
    { 1.14,          1.24,           0.00,           0.00 },          // L
    { 1.09,          0.00,           0.00,           1.01 },          // K
    { 1.37,          1.00,           0.00,           0.00 },          // M
    { 1.07,          1.31,           0.00,           0.00 },          // F
    { 0.00,          0.00,           1.21,           1.36 },          // P
    { 0.00,          0.00,           0.00,           1.00 },          // S
    { 0.00,          1.13,           0.00,           0.00 },          // T
    { 1.02,          1.30,           0.00,           0.00 },          // W
    { 0.00,          1.40,           1.00,           0.00 },          // Y
    { 1.00,          1.62,           0.00,           0.00 }};         // V

static double cf_parameters_norm[20][7] = {
    /*P(a)  P(b)  P(turn)  f(i)    f(i+1)  f(i+2)  f(i+3)     Amino Acid
    --------------------------------------------------------------------*/
    { 142,   83,   66,     0.060,  0.076,  0.035,  0.058 },   // A
    {  98,   93,   95,     0.070,  0.106,  0.099,  0.085 },   // R
    { 101,   54,  146,     0.147,  0.110,  0.179,  0.081 },   // D
    {  67,   89,  156,     0.161,  0.083,  0.191,  0.091 },   // N
    {  70,  119,  119,     0.149,  0.050,  0.117,  0.128 },   // C
    { 151,   37,   74,     0.056,  0.060,  0.077,  0.064 },   // E
    { 111,  110,   98,     0.074,  0.098,  0.037,  0.098 },   // Q
    {  57,   75,  156,     0.102,  0.085,  0.190,  0.152 },   // G
    { 100,   87,   95,     0.140,  0.047,  0.093,  0.054 },   // H
    { 108,  160,   47,     0.043,  0.034,  0.013,  0.056 },   // I
    { 121,  130,   59,     0.061,  0.025,  0.036,  0.070 },   // L
    { 116,   74,  101,     0.055,  0.115,  0.072,  0.095 },   // K
    { 145,  105,   60,     0.068,  0.082,  0.014,  0.055 },   // M
    { 113,  138,   60,     0.059,  0.041,  0.065,  0.065 },   // F
    {  57,   55,  152,     0.102,  0.301,  0.034,  0.068 },   // P
    {  77,   75,  143,     0.120,  0.139,  0.125,  0.106 },   // S
    {  83,  119,   96,     0.086,  0.108,  0.065,  0.079 },   // T
    { 108,  137,   96,     0.077,  0.013,  0.064,  0.167 },   // W
    {  69,  147,  114,     0.082,  0.065,  0.114,  0.125 },   // Y
    { 106,  170,   50,     0.062,  0.048,  0.028,  0.053 }};  // V

#define cf_former(strct,aa)  ((strct!=2)?cf_parameters[aa][strct]:cf_parameters_norm[aa][strct])
#define cf_breaker(strct,aa) ((strct!=2)?cf_parameters[aa][strct+2]:0)

static double maxProb[]      = { 1.42, 1.62, 156 }; /// Maximum former value for  helix, sheet, beta_turn 
static double maxProbBreak[] = { 1.21, 2.03, 0.0 }; /// Maximum breaker value for helix, sheet, beta_turn
static double minProb[]      = { 0.0,  0.0,  47  }; /// Minimum probability for helix, sheet, beta_turn


// General ED4 functions
GB_ERROR ED4_pfold_predict_summary(const char *sequence_, char *result_buffer, int result_buffer_size); /// Predicts protein secondary structure from its primary structure (amino acid sequence)
GB_ERROR ED4_pfold_calculate_secstruct_match(const char *structure_sai, const char *structure_cmp, int start, int end, char *result_buffer, PFOLD_MATCH_METHOD match_type = SECSTRUCT_SEQUENCE); /// Compares a protein secondary structure with a primary structure (amino acid sequence) or another secondary structure (depending on match_type)
GB_ERROR ED4_pfold_set_SAI(char **protstruct, GBDATA *gb_main, const char *alignment_name, long *protstruct_len = 0);
char     *ED4_pfold_find_protein_structure_SAI(const char *SAI_name, GBDATA *gb_main, const char *alignment_name);
    
// AW related functions
AW_window   *ED4_pfold_create_props_window(AW_root *awr, AW_cb_struct * /*owner*/awcbs);
static void ED4_pfold_select_SAI_and_update_option_menu(AW_window *aww, AW_CL oms, AW_CL set_sai);

// Internal init functions
void     ED4_pfold_init_statics(); /// Inits static variables 
GB_ERROR ED4_pfold_init_memory(const char *sequence_); /// Inits memory

//TODO: add sequence and structures as arguments and remove globals
// Internal functions for structure prediction:
static void ED4_pfold_find_nucleation_sites(const structure s);   /// Finds nucleation sites that initiate the specified structure (alpha-helix or beta-sheet)
static void ED4_pfold_extend_nucleation_sites(const structure s); /// Extends the found nucleation sites in both directions
static void ED4_pfold_find_turns();       /// Finds beta-turns
static void ED4_pfold_resolve_overlaps(); /// Resolves overlaps of predicted structures and creates summary

// Internal helper functions
static inline int ED4_pfold_round_sym(double d); /// Symmetric arithmetic rounding of a double value to an integer value

#endif
