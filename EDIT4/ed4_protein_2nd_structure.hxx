/** \file   ed4_protein_2nd_structure.hxx
 *  \brief  Adds support for protein structure prediction, comparison of two
 *          protein secondary structures and of amino acid sequences with protein
 *          secondary structures as well as visualization of the match quality in EDIT4.
 *  \author Markus Urban
 *  \date   2008-02-08
 *
 *  This file contains functions that predict a protein secondary structure from
 *  its primary structure (i.e. the amino acid sequence) and for visualizing
 *  how good a sequence matches a given secondary structure. Two secondary
 *  structures can be compared, too. The initial values for the match symbols
 *  and other settings are defined here, as well as functions that create a 
 *  "Protein Match Settings" window allowing the user to change the default
 *  properties for match computation.
 * 
 *  \sa The functions for protein structure prediction are based on a statistical
 *      method known as Chou-Fasman algorithm. For details refer to "Chou, P. and
 *      Fasman, G. (1978). Prediction of the secondary structure of proteins from
 *      their amino acid sequence. Advanced Enzymology, 47, 45-148.".
 * 
 *  \attention The used method for secondary structure prediciton is fast which
 *             was the main reason for choosing it. Performance is important
 *             for a large number of sequences loaded in the editor. However, it
 *             is not very accurate and should only be used as rough estimation.
 *             For our purpose, the algorithm as well as own adaptions to it are 
 *             used to get an approximate overview if a given amino acid sequence 
 *             does not match a certain secondary structure.
*/

#ifndef ED4_PROTEIN_2ND_STRUCTURE_HXX
#define ED4_PROTEIN_2ND_STRUCTURE_HXX

#ifndef AW_WINDOW_HXX
#include "aw_window.hxx"
#endif

//#define SHOW_PROGRESS ///< Print information about progress to screen (for finding and extending structures and resolving overlaps).

#define PFOLD_AWAR_ENABLE            "Pfold/enable"       ///< Enable structure match.
#define PFOLD_AWAR_SELECTED_SAI      "Pfold/selected_SAI" ///< Selected reference protein secondary structure SAI (i.e. the SAI that is used for structure comparison).
#define PFOLD_AWAR_PAIR_TEMPLATE     "Pfold/pairs/%s"     ///< Structure pairs that define the match quality (see #pfold_pairs) as used for match methods #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT.
#define PFOLD_AWAR_SYMBOL_TEMPLATE   "Pfold/symbols/%s"   ///< Symbols for the match quality as used for match methods #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT.
#define PFOLD_AWAR_SYMBOL_TEMPLATE_2 "Pfold/symbols2"     ///< Symbols for the match quality as used for match method #SECSTRUCT_SEQUENCE.
#define PFOLD_AWAR_MATCH_METHOD      "Pfold/match_method" ///< Selected method for computing the match quality (see #PFOLD_MATCH_METHOD).
#define PFOLD_AWAR_SAI_FILTER        "Pfold/SAI_filter"   ///< Filter SAIs for given criteria (string); used in option menu for SAI selection.

//TODO: move satic vaiables to .cpp file?

/** \brief Protein secondary structure types.
 *
 *  Defines the various types of protein secondary structure. The order
 *  (or at least the individual values) are important, because they are 
 *  used to access various arrays.
 */
enum PFOLD_STRUCTURE {
    ALPHA_HELIX       = 0, ///< Alpha-helix
    BETA_SHEET        = 1, ///< Beta-sheet
    BETA_TURN         = 2, ///< Beta-turn
    STRUCTURE_SUMMARY = 3, ///< Structure summary
//  THREE_TURN        = 4, ///< Three turn 
//  FOUR_TURN         = 5, ///< Four turn
//  FIVE_TURN         = 6  ///< Five turn
};

/** \brief Specifies the characters used for amino acid one letter code.
 *
 *  These are the characters that represent amino acids in one letter code.
 *  The order is important as the array initializes #char2AA which is used to
 *  access array elements in the tables #cf_parameters and #cf_parameters_norm. 
 */
static const char *amino_acids = "ARDNCEQGHILKMFPSTWYV";

/** \brief Maps character to amino acid one letter code.
 *
 *  This array maps a character to an integer value. It is initialized with the 
 *  function ED4_pfold_init_statics() which creates an array of the size 256
 *  (for ISO/IEC 8859-1 character encoding). Characters that represent an amino
 *  acid get values from 0 to 19 (according to their position in #amino_acids)
 *  and all others get the value -1. That way, it can be used to get parameters
 *  from the tables #cf_parameters and #cf_parameters_norm or to check if a
 *  certain character represents an amino acid.
 */
static int *char2AA = 0;

/** \brief Characters representing protein secondary structure.
 *
 *  Defines the characters representing secondary structure as output by the 
 *  function ED4_pfold_predict_structure(). According to common standards, 
 *  these are: <BR>  
 *  H = alpha-helix, <BR>
 *  E = beta-sheet, <BR>
 *  T = beta-turn.
 */
static char structure_chars[3] = {'H', 'E', 'T'};

/// Amino acids that break a certain structure (#ALPHA_HELIX or #BETA_SHEET) as used in ED4_pfold_extend_nucleation_sites().
static char *structure_breaker[2] = {
        strdup("NYPG"),
        strdup("PDESGK")
};

/// Amino acids that are indifferent for a certain structure (#ALPHA_HELIX or #BETA_SHEET) as used in ED4_pfold_extend_nucleation_sites().
static char *structure_indifferent[2] = {
        strdup("RTSC"),
        strdup("RNHA")
};

/// Defines a name-value pair (e.g. for awars, menu entries, etc.).
typedef struct name_value_pair {
    const char *name; ///< Name or description
    int        value; ///< Value attached to \a name
};

/// Match quality for secondary structure match.
typedef enum {
    STRUCT_PERFECT_MATCH,  ///< Perfect match
    STRUCT_GOOD_MATCH,     ///< Good match
    STRUCT_MEDIUM_MATCH,   ///< Medium match
    STRUCT_BAD_MATCH,      ///< Bad match
    STRUCT_NO_MATCH,       ///< No match
    STRUCT_UNKNOWN,        ///< Unknown structure
    PFOLD_MATCH_TYPE_COUNT ///< Number of match types
} PFOLD_MATCH_TYPE;

/// Awars for the match type; binds the #PFOLD_MATCH_TYPE to the corresponding awar name.
static name_value_pair pfold_match_type_awars[] = {
    { "Perfect_match", STRUCT_PERFECT_MATCH   },
    { "Good_match",    STRUCT_GOOD_MATCH      },
    { "Medium_match",  STRUCT_MEDIUM_MATCH    },
    { "Bad_match",     STRUCT_BAD_MATCH       },
    { "No_match",      STRUCT_NO_MATCH        },
    { "Unknown_match", STRUCT_UNKNOWN         },
    { 0,               PFOLD_MATCH_TYPE_COUNT }
}; 

/// Match pair definition (see #PFOLD_MATCH_TYPE) as used for match methods #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT in ED4_pfold_calculate_secstruct_match().
static char *pfold_pairs[6] = {
    strdup("HH GG II TT EE BB SS -- -. .."),          // STRUCT_PERFECT_MATCH
    strdup("HG HI HS EB ES TS H- G- I- T- E- B- S-"), // STRUCT_GOOD_MATCH
    strdup("HT GT IT"),                               // STRUCT_MEDIUM_MATCH
    strdup("ET BT"),                                  // STRUCT_BAD_MATCH
    strdup("EH BH EG EI"),                            // STRUCT_NO_MATCH
    strdup("")                                        // STRUCT_UNKNOWN
};

/// Symbols for the match quality (defined by #PFOLD_MATCH_TYPE) as used for match methods #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT in ED4_pfold_calculate_secstruct_match().
static char *pfold_pair_chars[6] = {
    strdup(" "), // STRUCT_PERFECT_MATCH
    strdup("-"), // STRUCT_GOOD_MATCH
    strdup("~"), // STRUCT_MEDIUM_MATCH
    strdup("+"), // STRUCT_BAD_MATCH
    strdup("#"), // STRUCT_NO_MATCH
    strdup("?")  // STRUCT_UNKNOWN
};

/** \brief Symbols for the match quality as used for match method 
 *         #SECSTRUCT_SEQUENCE in ED4_pfold_calculate_secstruct_match().
 *
 *  The ten symbols represent the match quality ranging from 0 - 100% in
 *  steps of 10%.
 */
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

/// Defines the methods for match computation. For details refer to ED4_pfold_calculate_secstruct_match().
typedef enum {
    SECSTRUCT_SECSTRUCT,        ///< Compare two protein secondary structures
    SECSTRUCT_SEQUENCE,         ///< Compare an amino acid sequence with a reference protein secondary structure
    SECSTRUCT_SEQUENCE_PREDICT, ///< Compare a full prediction of the protein secondary structure from its amino acid sequence with a reference protein secondary structure
    PFOLD_MATCH_METHOD_COUNT    ///< Number of match methods
} PFOLD_MATCH_METHOD;

/// Awars for the match method; binds the #PFOLD_MATCH_METHOD to the corresponding name that is used to create the menu in ED4_pfold_create_props_window().
static name_value_pair pfold_match_method_awars[4] = {
    { "Secondary Structure <-> Secondary Structure",        SECSTRUCT_SECSTRUCT        },
    { "Secondary Structure <-> Sequence",                   SECSTRUCT_SEQUENCE         },
    { "Secondary Structure <-> Sequence (Full Prediction)", SECSTRUCT_SEQUENCE_PREDICT },
    { 0,                                                    PFOLD_MATCH_METHOD_COUNT   }
};

//TODO: is there a way to prevent doxygen from stripping the comments from the table?
// I simply added the parameter table as verbatim environment to show the comments in
// the documentation.
/** \brief Former and breaker values for alpha-helices and beta-sheets (= strands).
 *
 *  \hideinitializer
 *  \par Initial value:
 *  \verbatim
 {
 //   Helix Former   Strand Former   Helix Breaker   Strand Breaker   Amino
 //   Value          Value           Value           Value            Acid
 // -----------------------------------------------------------------------
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
    \endverbatim
 *
 *  The former and breaker values are used to find alpha-helix and beta-sheet 
 *  nucleation sites in ED4_pfold_find_nucleation_sites() and to resolve overlaps
 *  in ED4_pfold_resolve_overlaps(). Addressing the array with the enums
 *  #ALPHA_HELIX or #BETA_SHEET as second index gives the former values and
 *  addressing it with #ALPHA_HELIX+2 or #BETA_SHEET+2 gives the breaker values.
 *  The first index is for the amino acid. Use #char2AA to convert an amino acid
 *  character to the corresponding index.
 * 
 *  \sa Refer to the definition in the source code for commented table.
 */
static double cf_parameters[20][4] = {
    /*Helix Former   Strand Former   Helix Breaker   Strand Breaker   Amino
      Value          Value           Value           Value            Acid
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

/** \brief Normalized former values for alpha-helices, beta-sheets (= strands)
 *         and beta-turns as well as beta-turn probabilities.
 *
 *  \hideinitializer
 *  \par Initial value:
 *  \verbatim
 {
 //   P(a)  P(b)  P(turn)  f(i)    f(i+1)  f(i+2)  f(i+3)     Amino Acid
 // --------------------------------------------------------------------
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
    \endverbatim
 *
 *  The normalized former values are used to find beta-turns in an amino acid 
 *  sequence in ED4_pfold_find_turns(). Addressing the array with the enums 
 *  #ALPHA_HELIX, #BETA_SHEET or #BETA_TURN as second index gives the former 
 *  values and addressing it with #BETA_TURN+i \f$(1 <= i <= 4)\f$ gives the 
 *  turn probabilities. The first index is for the amino acid. Use #char2AA to 
 *  convert an amino acid character to the corresponding index.
 * 
 *  \sa Refer to the definition in the source code for commented table.
 */
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

static double max_former_value[3]  = { 1.42, 1.62, 156 }; ///< Maximum former value for alpha-helix, beta-sheet (in #cf_parameters) and beta-turn (in #cf_parameters_norm).
static double min_former_value[3]  = { 0.0,  0.0,  47  }; ///< Minimum former value for alpha-helix, beta-sheet (in #cf_parameters) and beta-turn (in #cf_parameters_norm).
static double max_breaker_value[3] = { 1.21, 2.03, 0.0 }; ///< Maximum breaker value for alpha-helix, beta-sheet (in #cf_parameters) and beta-turn (no breaker values => 0).

/** \brief Returns the former value of an amino acid depending on the given structure type.
 *
 *  The definition is used for method #SECSTRUCT_SEQUENCE in
 *  ED4_pfold_calculate_secstruct_match() to get the former value of an amino acid
 *  depending on the found structure type at its position. It addresses #cf_parameters
 *  for #ALPHA_HELIX and #BETA_SHEET and #cf_parameters_norm for #BETA_TURN.
 */
#define cf_former(aa,strct)  ( (strct!=2) ? cf_parameters[aa][strct] : cf_parameters_norm[aa][strct] )

/** \brief Returns the breaker value of an amino acid depending on the given structure type.
 *
 *  The definition is used for method #SECSTRUCT_SEQUENCE in
 *  ED4_pfold_calculate_secstruct_match() to get the breaker value of an amino acid
 *  depending on the found structure type at its position. It addresses #cf_parameters
 *  for #ALPHA_HELIX and #BETA_SHEET and returns 0 for #BETA_SHEET, because it has no
 *  breaker values. 
 */
#define cf_breaker(aa,strct) ( (strct!=2) ? cf_parameters[aa][strct+2] : 0 )


// General ED4 functions //////////////////////////////////////////////////////////////////////////////////////////////////////////

/** \brief Predicts protein secondary structures from the amino acid sequence.
 *
 *  \param[in]  sequence   Amino acid sequence
 *  \param[out] structures Predicted secondary structures (#ALPHA_HELIX, #BETA_SHEET, 
 *                         #BETA_TURN and #STRUCTURE_SUMMARY, in this order)
 *  \param[in]  length     Size of \a sequence and \a structures[i]
 *  \return     Error description, if an error occured; 0 otherwise
 * 
 *  This function predicts the protein secondary structures from the amino acid
 *  sequence according to the Chou-Fasman algorithm. In a first step, nucleation sites
 *  for alpha-helices and beta-sheets are found using ED4_pfold_find_nucleation_sites().
 *  In a next step, the found structures are extended obeying certain rules with
 *  ED4_pfold_extend_nucleation_sites(). Beta-turns are found with the function
 *  ED4_pfold_find_turns(). In a final step, overlapping regions are identified and 
 *  resolved to create a structure summary with ED4_pfold_resolve_overlaps().
 *  The results are written to \a structures[i] and can be accessed via the enums
 *  #ALPHA_HELIX, #BETA_SHEET, #BETA_TURN and #STRUCTURE_SUMMARY.   
 */
GB_ERROR ED4_pfold_predict_structure(const char *sequence, char *structures[4], int length);

/** \brief Predicts protein secondary structure from the amino acid sequence.
 *
 *  \param[in]  sequence  Amino acid sequence
 *  \param[out] structure Predicted secondary structure summary
 *  \param[in]  length    Size of \a sequence and \a structure
 *  \return     Error description, if an error occured; 0 otherwise
 *
 *  Basically the same as
 *  ED4_pfold_predict_structure(const char *sequence, char *structures[4], int length)
 *  except that it returns only the secondary structure summary in \a structure.
 */
GB_ERROR ED4_pfold_predict_structure(const char *sequence, char *structure, int length);

/** \brief Compares a protein secondary structure with a primary structure or another
 *         secondary structure.
 *
 *  \param[in]  structure_sai Reference protein structure SAI (secondary structure)
 *  \param[in]  structure_cmp Protein structure to compare (primary or secondary structure)
 *  \param[in]  start         The start of the match computation (visible area in editor)
 *  \param[in]  end           The end of the match computation (visible area in editor)
 *  \param[out] result_buffer Result buffer for match symbols
 *  \param[in]  match_method  Method for structure match computation
 *  \return     Error description, if an error occured; 0 otherwise
 *
 *  This function compares a protein secondary structure with a primary structure
 *  (= amino acid sequence) or another secondary structure depending on \a match_method.
 * 
 *  \par Match method SECSTRUCT_SECSTRUCT:
 *       Two secondary structures are compared one by one using the criteria defined 
 *       by #pfold_pairs. The match symbols are taken from #pfold_pair_chars.
 * 
 *  \par Match method SECSTRUCT_SEQUENCE:
 *       An amino acid sequence is compared with a secondary structure by taking
 *       cohesive parts of the structure - gaps in the alignment are skipped - and
 *       computing the normalized difference of former and breaker values for this
 *       region in the given sequence such that a value from 0 - 100% for the
 *       match quality is generated. By dividing this value into steps of 10%
 *       it is mapped to the match symbols defined by #pfold_pair_chars_2. Note
 *       that bends ('S') are assumed to fit everywhere (=> best match symbol), and
 *       if a structure is encountered but no corresponding amino acid the worst match 
 *       symbol is chosen.
 * 
 *  \par Match method SECSTRUCT_SEQUENCE_PREDICT:
 *       An amino acid sequence is compared with a secondary structure using a full
 *       prediction of the secondary structure from its sequence via
 *       ED4_pfold_predict_structure() and comparing it one by one with the reference
 *       structure. Note that not the structure summary is used for comparison, but
 *       the individual predicted structure types as returned in \a structures[4].
 *       The match criteria are defined in #pfold_pairs which is searched in ascending
 *       order, i.e. good matches first, then the worse ones. If a match is found
 *       the corresponding match symbol (as defined by #pfold_pair_chars) is chosen.
 *       Note that if a structure is encountered but no corresponding amino acid the
 *       worst match symbol is chosen.
 * 
 *  The match criteria (for #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT) as
 *  well as the match symbols (for all methods) can be adjusted by the user in the
 *  "Protein Match Settings" dialog. The result of the match computation (i.e. the
 *  match symbols) is written to the result buffer.
 */
GB_ERROR ED4_pfold_calculate_secstruct_match(const char *structure_sai, const char *structure_cmp, int start, int end, char *result_buffer, PFOLD_MATCH_METHOD match_method = SECSTRUCT_SEQUENCE);

/** \brief Sets the reference protein secondary structure SAI.
 *
 *  \param[out] protstruct     Pointer to reference protein secondary structure SAI
 *  \param[in]  gb_main        Main database
 *  \param[in]  alignment_name Name of the alignment to search for
 *  \param[out] protstruct_len Length of reference protein secondary structure SAI
 *  \return     Error description, if an error occured; 0 otherwise
 *
 *  The function searches the database \a gb_main for the currently selected SAI
 *  as defined by #PFOLD_AWAR_SELECTED_SAI and assigns the data of the alignment
 *  \a alignment_name to \a protstruct. If \a protstruct_len is specified the length
 *  of the new reference SAI is stored. The function is used in the editor to
 *  initialize the reference protein secondary structure SAI and to update it if the
 *  selected SAI is changed in the "Protein Match Settings" dialog. For this purpose
 *  it should be called with &ED4_ROOT->protstruct and &ED4_ROOT->protstruct_len. 
 */
GB_ERROR ED4_pfold_set_SAI(char **protstruct, GBDATA *gb_main, const char *alignment_name, long *protstruct_len = 0);


// AW related functions ///////////////////////////////////////////////////////////////////////////////////////////////

/** \brief Creates the "Protein Match Settings" window.
 *
 *  \param[in] awr   Root window
 *  \param[in] awcbs Callback struct
 *  \return    Window
 *
 *  The "Protein Match Settings" window allows the user to configure the properties
 *  for protein match computation. These settings include turning the match 
 *  computation on and off (bound to awar #PFOLD_AWAR_ENABLE), selecting the reference
 *  protein secondary structure SAI (bound to awar #PFOLD_AWAR_SELECTED_SAI), choosing
 *  the match method (bound to awar #PFOLD_AWAR_MATCH_METHOD, see #PFOLD_MATCH_METHOD)
 *  and the definition of the match pairs (bound to awar #PFOLD_AWAR_PAIR_TEMPLATE
 *  and #pfold_match_type_awars, see #PFOLD_MATCH_TYPE and #pfold_pairs) as well as
 *  the match symbols (bound to awar #PFOLD_AWAR_SYMBOL_TEMPLATE and
 *  #pfold_match_type_awars or #PFOLD_AWAR_SYMBOL_TEMPLATE_2, see #PFOLD_MATCH_TYPE
 *  and #pfold_pair_chars or #pfold_pair_chars_2). Via a filter (bound to
 *  #PFOLD_AWAR_SAI_FILTER) the SAIs shown in the option menu can be narrowed down to
 *  a selection of SAIs whose names contain the specified string. The callback function
 *  #ED4_pfold_select_SAI_and_update_option_menu() is bound to the SAI option menu and
 *  the SAI filter to update the selected SAI in the editor or the selection in the
 *  SAI option menu.
 */
AW_window *ED4_pfold_create_props_window(AW_root *awr, AW_cb_struct * /*owner*/awcbs);

/** \brief Callback function to select the reference protein structure SAI and to
 *         update the SAI option menu.
 *
 *  \param[in]     aww     The calling window
 *  \param[in,out] oms     The SAI option menu
 *  \param[in]     set_sai Specifies if SAI should be updated
 *
 *  The function is called whenever the selected SAI or the SAI filter is changed
 *  in the "Protein Match Settings" dialog (see ED4_pfold_create_props_window()).
 *  It can be called with \a set_sai defined to update the reference protein secondary
 *  structure SAI in the editor via ED4_pfold_set_SAI() and to update the selection in
 *  the SAI option menu. If \a set_sai is 0 only the option menu is updated. This is
 *  necessary if only the SAI filter changed but not the selected SAI.
 */
static void ED4_pfold_select_SAI_and_update_option_menu(AW_window *aww, AW_CL oms, AW_CL set_sai);


// Internal init functions ////////////////////////////////////////////////////////////////////////////////////////////

/** \brief Initializes static variables.
 *
 *  So far, this function only concerns #char2AA which gets initialized here.
 *  See #char2AA for details on the values. It is called by 
 *  ED4_pfold_predict_structure() and ED4_pfold_calculate_secstruct_match().
 * 
 *  \attention If any other prediction function is used alone before calling one
 *             of the mentioned functions, this function has to be called first.
 */
void ED4_pfold_init_statics();


//TODO: leave only functions that should be reached from outside, move the rest to .cpp file 
// Functions for structure prediction  //////////////////////////////////////////////////////////////////////

/** \brief Predicts the specified secondary structure type from the amino acid sequence.
 *
 *  \param[in]  sequence  Amino acid sequence
 *  \param[out] structure Predicted secondary structure
 *  \param[in]  length    Size of \a sequence and \a structure
 *  \param[in]  s         Secondary structure type (#ALPHA_HELIX, #BETA_SHEET or #BETA_TURN)
 *
 *  The function calls ED4_pfold_find_nucleation_sites() and ED4_pfold_extend_nucleation_sites()
 *  if s is #ALPHA_HELIX or #BETA_SHEET and ED4_pfold_find_turns() if s is #BETA_TURN.
 */
static void ED4_pfold_find_structure(const char *sequence, char *structure, int length, const PFOLD_STRUCTURE s);

/** \brief Finds nucleation sites that initiate the specified structure.
 *
 *  \param[in]  sequence  Amino acid sequence
 *  \param[out] structure Predicted secondary structure
 *  \param[in]  length    Size of \a sequence and \a structure
 *  \param[in]  s         Secondary structure type (either #ALPHA_HELIX or #BETA_SHEET)
 *
 *  This function finds nucleation sites that initiate the specified structure 
 *  (alpha-helix or beta-sheet). A window of a fixed size is moved over the
 *  sequence and former and breaker values (as defined by #cf_parameters) for 
 *  the amino acids in the window are summed up. If the former values in this
 *  region reach a certain value and the breaker values do not exceed a certain
 *  limit a nucleation site is formed, i.e. the region is assumed to be the 
 *  corresponding secondary structure. The result is stored in \a structure. 
 */
static void ED4_pfold_find_nucleation_sites(const char *sequence, char *structure, int length, const PFOLD_STRUCTURE s);

/** \brief Extends the found nucleation sites in both directions.
 *
 *  \param[in]  sequence  Amino acid sequence
 *  \param[out] structure Predicted secondary structure
 *  \param[in]  length    Size of \a sequence and \a structure
 *  \param[in]  s         Secondary structure type (either #ALPHA_HELIX or #BETA_SHEET)
 *
 *  The function extends the nucleation sites found by ED4_pfold_find_nucleation_sites()
 *  in both directions. Extension continues until a certain amino acid constellation
 *  is found. The amino acid 'P' breaks an alpha-helix and 'P' as well as 'E' break
 *  a beta-sheet. Also, two successive breakers or one breaker followed by an
 *  indifferent amino acid (as defined by #structure_breaker and #structure_indifferent)
 *  break the structure. The result is stored in \a structure.
 */
static void ED4_pfold_extend_nucleation_sites(const char *sequence, char *structure, int length, const PFOLD_STRUCTURE s);

/** \brief Predicts beta-turns from the given amino acid sequence
 * 
 *  \param[in]  sequence  Amino acid sequence
 *  \param[out] structure Predicted secondary structure
 *  \param[in]  length    Size of \a sequence and \a structure
 *
 *  A window of a fixed size is moved over the sequence and former values for
 *  alpha-helices, beta-sheets and beta-turns are summed up. In addition,
 *  beta-turn probabilities are multiplied. The values are specified in
 *  #cf_parameters_norm. If the former values for beta-turn are greater than
 *  the ones for alpha-helix and beta-sheet and the turn probabilities
 *  exceed a certain limit the region is assumed to be a beta-turn. The result
 *  is stored in \a structure.
 */
static void ED4_pfold_find_turns(const char *sequence, char *structure, int length);

/** \brief Resolves overlaps of predicted secondary structures and creates structure summary.
 * 
 *  \param[in]     sequence   Amino acid sequence
 *  \param[in,out] structures Predicted secondary structures (#ALPHA_HELIX, #BETA_SHEET, 
 *                            #BETA_TURN and #STRUCTURE_SUMMARY, in this order)
 *  \param[in]     length     Size of \a sequence and \a structures[i]
 *
 *  The function takes the given predicted structures (alpha-helix, beta-sheet 
 *  and beta-turn) and searches for overlapping regions. If a beta-turn is found
 *  the structure summary is assumed to be a beta-turn. For overlapping alpha-helices
 *  and beta-sheets the former values are summed up for this region and the
 *  structure summary is assumed to be the structure type with the higher former
 *  value. The result is stored in \a structures[3] (= \a structures[#STRUCTURE_SUMMARY]).
 * 
 *  \attention I couldn't find a standard procedure for resolving overlaps and 
 *             there might be other (possibly better) ways to do that.
 */
static void ED4_pfold_resolve_overlaps(const char *sequence, char *structures[4], int length);


// Internal helper functions ///////////////////////////////////////////////////////////////////////////////////////////

/** \brief Symmetric arithmetic rounding of a double value to an integer value.
 *
 *  \param[in] d Value to be rounded
 *  \return    Rounded value
 *
 *  Rounds a double value to an integer value using symmetric arithmetic rounding,
 *  i.e. a number \f$x.y\f$ is rounded to \f$x\f$ if \f$y < 5\f$ and to \f$x+1\f$ 
 *  otherwise.
 */
static inline int ED4_pfold_round_sym(double d);

#endif
