// =============================================================== //
//                                                                 //
//   File      : ED4_protein_2nd_structure.cxx                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


/*! \file   ED4_protein_2nd_structure.cxx
 *  \brief  Implements the functions defined in ed4_protein_2nd_structure.hxx.
 *  \author Markus Urban
 *  \date   2008-02-08
 *  \sa     Refer to ed4_protein_2nd_structure.hxx for details, please.
*/

#include "ed4_protein_2nd_structure.hxx"
#include "ed4_class.hxx"
#include "ed4_awars.hxx"

#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include "arbdbt.h"

#include <iostream>
#include <awt_config_manager.hxx>

#define e4_assert(bed) arb_assert(bed)

using namespace std;

// --------------------------------------------------------------------------------
// exported data

//! Awars for the match type; binds the #PFOLD_MATCH_TYPE to the corresponding awar name.
name_value_pair pfold_match_type_awars[] = {
    { "Perfect_match", STRUCT_PERFECT_MATCH   },
    { "Good_match",    STRUCT_GOOD_MATCH      },
    { "Medium_match",  STRUCT_MEDIUM_MATCH    },
    { "Bad_match",     STRUCT_BAD_MATCH       },
    { "No_match",      STRUCT_NO_MATCH        },
    { "Unknown_match", STRUCT_UNKNOWN         },
    { 0,               PFOLD_MATCH_TYPE_COUNT }
};

//! Symbols for the match quality (defined by #PFOLD_MATCH_TYPE) as used for match methods #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT in ED4_pfold_calculate_secstruct_match().
char *pfold_pair_chars[PFOLD_PAIRS] = {
    strdup(" "), // STRUCT_PERFECT_MATCH
    strdup("-"), // STRUCT_GOOD_MATCH
    strdup("~"), // STRUCT_MEDIUM_MATCH
    strdup("+"), // STRUCT_BAD_MATCH
    strdup("#"), // STRUCT_NO_MATCH
    strdup("?")  // STRUCT_UNKNOWN
};

//! Match pair definition (see #PFOLD_MATCH_TYPE) as used for match methods #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT in ED4_pfold_calculate_secstruct_match().
char *pfold_pairs[PFOLD_PAIRS] = {
    strdup("HH GG II TT EE BB SS -- -. .."),          // STRUCT_PERFECT_MATCH
    strdup("HG HI HS EB ES TS H- G- I- T- E- B- S-"), // STRUCT_GOOD_MATCH
    strdup("HT GT IT"),                               // STRUCT_MEDIUM_MATCH
    strdup("ET BT"),                                  // STRUCT_BAD_MATCH
    strdup("EH BH EG EI"),                            // STRUCT_NO_MATCH
    strdup("")                                        // STRUCT_UNKNOWN
};

static struct pfold_mem_handler {
    ~pfold_mem_handler() {
        for (int i = 0; i<PFOLD_PAIRS; ++i) {
            freenull(pfold_pairs[i]);
            freenull(pfold_pair_chars[i]);
        }
    }
} pfold_dealloc;

// --------------------------------------------------------------------------------

/*! \brief Specifies the characters used for amino acid one letter code.
 *
 *  These are the characters that represent amino acids in one letter code.
 *  The order is important as the array initializes #char2AA which is used to
 *  access array elements in the tables #cf_parameters and #cf_parameters_norm.
 */
static const char *amino_acids = "ARDNCEQGHILKMFPSTWYV";

/*! \brief Maps character to amino acid one letter code.
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

/*! \brief Characters representing protein secondary structure.
 *
 *  Defines the characters representing secondary structure as output by the
 *  function ED4_pfold_predict_structure(). According to common standards,
 *  these are: <BR>
 *  H = alpha-helix, <BR>
 *  E = beta-sheet, <BR>
 *  T = beta-turn.
 */
static char structure_chars[3] = { 'H', 'E', 'T' };

//! Amino acids that break a certain structure (#ALPHA_HELIX or #BETA_SHEET) as used in ED4_pfold_extend_nucleation_sites().
static const char *structure_breaker[2] = {
        "NYPG",
        "PDESGK"
};

//! Amino acids that are indifferent for a certain structure (#ALPHA_HELIX or #BETA_SHEET) as used in ED4_pfold_extend_nucleation_sites().
static const char *structure_indifferent[2] = {
        "RTSC",
        "RNHA"
};

//! Awars for the match method; binds the #PFOLD_MATCH_METHOD to the corresponding name that is used to create the menu in ED4_pfold_create_props_window().
static name_value_pair pfold_match_method_awars[4] = {
    { "Secondary Structure <-> Secondary Structure",        SECSTRUCT_SECSTRUCT        },
    { "Secondary Structure <-> Sequence",                   SECSTRUCT_SEQUENCE         },
    { "Secondary Structure <-> Sequence (Full Prediction)", SECSTRUCT_SEQUENCE_PREDICT },
    { 0,                                                    PFOLD_MATCH_METHOD_COUNT }
};

static double max_former_value[3]  = { 1.42, 1.62, 156 }; //!< Maximum former value for alpha-helix, beta-sheet (in #cf_parameters) and beta-turn (in #cf_parameters_norm).
static double min_former_value[3]  = { 0.0,  0.0,  47  }; //!< Minimum former value for alpha-helix, beta-sheet (in #cf_parameters) and beta-turn (in #cf_parameters_norm).
static double max_breaker_value[3] = { 1.21, 2.03, 0.0 }; //!< Maximum breaker value for alpha-helix, beta-sheet (in #cf_parameters) and beta-turn (no breaker values => 0).

// --------------------------------------------------------------------------------

// TODO: is there a way to prevent doxygen from stripping the comments from the table?
// I simply added the parameter table as verbatim environment to show the comments in
// the documentation.
/*! \brief Former and breaker values for alpha-helices and beta-sheets (= strands).
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
    /* Helix Former   Strand Former   Helix Breaker   Strand Breaker   Amino
      Value          Value           Value           Value            Acid
    ----------------------------------------------------------------------- */
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
    { 1.00,          1.62,           0.00,           0.00 } };        // V

/*! \brief Normalized former values for alpha-helices, beta-sheets (= strands)
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
    /* P(a)  P(b)  P(turn)  f(i)    f(i+1)  f(i+2)  f(i+3)     Amino Acid
    -------------------------------------------------------------------- */
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
    { 106,  170,   50,     0.062,  0.048,  0.028,  0.053 } }; // V

// --------------------------------------------------------------------------------

/*! \brief Symmetric arithmetic rounding of a double value to an integer value.
 *
 *  \param[in] d Value to be rounded
 *  \return    Rounded value
 *
 *  Rounds a double value to an integer value using symmetric arithmetic rounding,
 *  i.e. a number \f$x.y\f$ is rounded to \f$x\f$ if \f$y < 5\f$ and to \f$x+1\f$
 *  otherwise.
 */
inline int ED4_pfold_round_sym(double d) {
    return int(d + .5);
}


/*! \brief Initializes static variables.
 *
 *  So far, this function only concerns #char2AA which gets initialized here.
 *  See #char2AA for details on the values. It is called by
 *  ED4_pfold_predict_structure() and ED4_pfold_calculate_secstruct_match().
 *
 *  \attention If any other prediction function is used alone before calling one
 *             of the mentioned functions, this function has to be called first.
 */
static void ED4_pfold_init_statics() {
    // specify the characters used for amino acid one letter code
    if (!char2AA) {
        char2AA = new int [256];
        for (int i = 0; i < 256; i++) {
            char2AA[i] = -1;
        }
        for (int i = 0; amino_acids[i]; i++) {
            char2AA[(unsigned char)amino_acids[i]] = i;
        }
    }
}


/*! \brief Finds nucleation sites that initiate the specified structure.
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
static void ED4_pfold_find_nucleation_sites(const unsigned char *sequence, char *structure, int length, const PFOLD_STRUCTURE s) {
#ifdef SHOW_PROGRESS
    cout << endl << "Searching for nucleation sites:" << endl;
#endif
    e4_assert(s == ALPHA_HELIX || s == BETA_SHEET); // incorrect value for s
    e4_assert(char2AA); // char2AA not initialized; ED4_pfold_init_statics() failed or hasn't been called yet

    char   *gap_chars    = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string(); // gap characters
    int     windowSize   = (s == ALPHA_HELIX ? 6 : 5); // window size used for finding nucleation sites
    double  sumOfFormVal = 0, sumOfBreakVal = 0; // sum of former resp. breaker values in window
    int     pos;                // current position in sequence
    int     count;              // number of amino acids found in window

    for (int i = 0; i < ((length + 1) - windowSize); i++) {
        int aa = 0;             // amino acid

        pos = i;
        for (count = 0; count < windowSize; count++) {
            // skip gaps
            while (pos < ((length + 1) - windowSize) &&
                    strchr(gap_chars, sequence[pos + count])) {
                pos++;
            }
            aa = char2AA[sequence[pos + count]];
            if (aa == -1) break; // unknown character found

            // compute former and breaker values
            sumOfFormVal += cf_parameters[aa][s];
            sumOfBreakVal += cf_parameters[aa][s+2];
        }

        // assign sequence and save start and end of nucleation site for later extension
        if ((sumOfFormVal > (windowSize - 2)) && (sumOfBreakVal < 2)) {
            for (int j = i; j < (pos + count); j++) {
                if (char2AA[sequence[j]] != -1) structure[j] = structure_chars[s];
            }
        }
        if (aa == -1) i = pos + count; // skip unknown character
        sumOfFormVal = 0, sumOfBreakVal = 0;
    }

    free(gap_chars);
#ifdef SHOW_PROGRESS
    cout << structure << endl;
#endif
}


/*! \brief Extends the found nucleation sites in both directions.
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
static void ED4_pfold_extend_nucleation_sites(const unsigned char *sequence, char *structure, int length, const PFOLD_STRUCTURE s) {
#ifdef SHOW_PROGRESS
    cout << endl << "Extending nucleation sites:" << endl;
#endif
    e4_assert(s == ALPHA_HELIX || s == BETA_SHEET); // incorrect value for s
    e4_assert(char2AA); // char2AA not initialized; ED4_pfold_init_statics() failed or hasn't been called yet

    bool break_structure = false;       // break the current structure
    int  start           = 0, end = 0;  // start and end of nucleation site
    int  neighbour       = 0;           // neighbour of start or end

    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string();       // gap characters

    // find nucleation sites and extend them in both directions (for whole sequence)
    for (int indStruct = 0; indStruct < length; indStruct++) {

        // search start and end of nucleated region
        while (indStruct < length &&
               ((structure[indStruct] == ' ') || strchr(gap_chars, sequence[indStruct]))
               ) indStruct++;

        if (indStruct >= length) break;
        // get next amino acid that is not included in nucleation site
        start = indStruct - 1;
        while (indStruct < length &&
                (structure[indStruct] != ' ' || strchr(gap_chars, sequence[indStruct]))) {
            indStruct++;
        }
        // get next amino acid that is not included in nucleation site
        end = indStruct;

        // extend nucleated region in both directions
        // left side:
        while (start > 1 && strchr(gap_chars, sequence[start])) {
            start--; // skip gaps
        }
        // break if no amino acid is found
        if (start >= 0) break_structure = (char2AA[sequence[start]] == -1);
        while (!break_structure && (start > 1) && (structure[start] == ' ')) {
            // break if absolute breaker (P or E) is found
            break_structure = (sequence[start] == 'P');
            if (s == BETA_SHEET) break_structure |= (sequence[start] == 'E');
            if (break_structure) break;
            // check for breaker at current position
            break_structure = (strchr(structure_breaker[s], sequence[start]) != 0);
            neighbour = start - 1; // get neighbour
            while (neighbour > 0 && strchr(gap_chars, sequence[neighbour])) {
                neighbour--; // skip gaps
            }
            // break if out of bounds or no amino acid is found
            if (neighbour <= 0 || char2AA[sequence[neighbour]] == -1) {
                break;
            }
            // break if another breaker or indifferent amino acid is found
            break_structure &=
                (strchr(structure_breaker[s], sequence[neighbour]) != 0) ||
                (strchr(structure_indifferent[s], sequence[neighbour]) != 0);
            if (!break_structure) {
                structure[start] = structure_chars[s];
            }
            start = neighbour;  // continue with neighbour
        }

        // right side:
        while (end < (length - 2) && strchr(gap_chars, sequence[end])) {
            end++; // skip gaps
        }
        // break if no amino acid is found
        if (end <= (length - 1)) break_structure = (char2AA[sequence[end]] == -1);
        while (!break_structure && (end < (length - 2))) {
            // break if absolute breaker (P or E) is found
            break_structure = (sequence[end] == 'P');
            if (s == BETA_SHEET) break_structure |= (sequence[end] == 'E');
            if (break_structure) break;
            // check for breaker at current position
            break_structure = (strchr(structure_breaker[s], sequence[end]) != 0);
            neighbour = end + 1; // get neighbour
            while (neighbour < (length - 2) && strchr(gap_chars, sequence[neighbour])) {
                neighbour++; // skip gaps
            }
            // break if out of bounds or no amino acid is found
            if (neighbour >= (length - 1) || char2AA[sequence[neighbour]] == -1) {
                end = neighbour;
                break;
            }
            // break if another breaker or indifferent amino acid is found
            break_structure &=
                (strchr(structure_breaker[s], sequence[neighbour]) != 0) ||
                (strchr(structure_indifferent[s], sequence[neighbour]) != 0);
            if (!break_structure) {
                structure[end] = structure_chars[s];
            }
            end = neighbour; // continue with neighbour
        }
        indStruct = end; // continue with end
    }

    free(gap_chars);
#ifdef SHOW_PROGRESS
    cout << structure << endl;
#endif
}


/*! \brief Predicts beta-turns from the given amino acid sequence
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
static void ED4_pfold_find_turns(const unsigned char *sequence, char *structure, int length) {
#ifdef SHOW_PROGRESS
    cout << endl << "Searching for beta-turns: " << endl;
#endif
    e4_assert(char2AA); // char2AA not initialized; ED4_pfold_init_statics() failed or hasn't been called yet

    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string(); // gap characters
    const int windowSize = 4; // window size
    double P_a = 0, P_b = 0, P_turn = 0; // former values for helix, sheet and beta-turn
    double p_t = 1; // probability for beta-turn
    int pos;        // position in sequence
    int count;      // position in window
    int aa;         // amino acid

    for (int i = 0; i < ((length + 1) - windowSize); i++) {
        pos = i;
        for (count = 0; count < windowSize; count++) {
            // skip gaps
            while (pos < ((length + 1) - windowSize) &&
                    strchr(gap_chars, sequence[pos + count])) {
                pos++;
            }
            aa = char2AA[sequence[pos + count]];
            if (aa == -1) break; // unknown character found

            // compute former values and turn probability
            P_a    += cf_parameters_norm[aa][0];
            P_b    += cf_parameters_norm[aa][1];
            P_turn += cf_parameters_norm[aa][2];
            p_t    *= cf_parameters_norm[aa][3 + count];
        }
        if (count != 0) {
            P_a /= count;
            P_b /= count;
            P_turn /= count;
            if ((p_t > 0.000075) && (P_turn > 100) && (P_turn > P_a) && (P_turn > P_b)) {
                for (int j = i; j < (pos + count); j++) {
                    if (char2AA[sequence[j]] != -1) structure[j] = structure_chars[BETA_TURN];
                }
            }
        }
        if (aa == -1) i = pos + count; // skip unknown character
        p_t = 1, P_a = 0, P_b = 0, P_turn = 0;
    }

    free(gap_chars);
#ifdef SHOW_PROGRESS
    cout << structure << endl;
#endif
}


/*! \brief Resolves overlaps of predicted secondary structures and creates structure summary.
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
static void ED4_pfold_resolve_overlaps(const unsigned char *sequence, char *structures[4], int length) {
#ifdef SHOW_PROGRESS
    cout << endl << "Resolving overlaps: " << endl;
#endif
    e4_assert(char2AA); // char2AA not initialized; ED4_pfold_init_statics() failed or hasn't been called yet

    int start = -1;    // start of overlap
    int end   = -1;    // end of overlap
    double P_a = 0;    // sum of former values for alpha-helix in overlapping regions
    double P_b = 0;    // sum of former values for beta-sheet in overlapping regions
    PFOLD_STRUCTURE s; // structure with the highest former values
    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string(); // gap characters

    // scan structures for overlaps
    for (int pos = 0; pos < length; pos++) {

        // if beta-turn is found at position pos -> summary is beta-turn
        if (structures[BETA_TURN][pos] != ' ') {
            structures[STRUCTURE_SUMMARY][pos] = structure_chars[BETA_TURN];

        // if helix and sheet are overlapping and no beta-turn is found -> check which structure has the highest sum of former values
        }
        else if ((structures[ALPHA_HELIX][pos] != ' ') && (structures[BETA_SHEET][pos] != ' ')) {

            // search start and end of overlap (as long as no beta-turn is found)
            start = pos;
            end = pos;
            while (structures[ALPHA_HELIX][end] != ' ' && structures[BETA_SHEET][end] != ' ' &&
                    structures[BETA_TURN][end] == ' ') {
                end++;
            }

            // calculate P_a and P_b for overlap
            for (int i = start; i < end; i++) {
                // skip gaps
                while (i < end && strchr(gap_chars, sequence[i])) {
                    i++;
                }
                int aa = char2AA[sequence[i]];
                if (aa != -1) {
                    P_a += cf_parameters[aa][ALPHA_HELIX];
                    P_b += cf_parameters[aa][BETA_SHEET];
                }
            }

            // check which structure is more likely and set s appropriately
            s = (P_a > P_b) ? ALPHA_HELIX : BETA_SHEET;

            // set structure for overlapping region
            for (int i = start; i < end; i++) {
                structures[STRUCTURE_SUMMARY][i] = structure_chars[s];
            }

            // set variables for next pass of loop resp. end of loop
            P_a = 0, P_b = 0;
            pos = end - 1;

        // if helix and sheet are not overlapping and no beta-turn is found -> set structure accordingly
        }
        else {
            // summary at position pos is helix resp. sheet
            if (structures[ALPHA_HELIX][pos] != ' ') {
                structures[STRUCTURE_SUMMARY][pos] = structure_chars[ALPHA_HELIX];
            }
            else if (structures[BETA_SHEET][pos] != ' ') {
                structures[STRUCTURE_SUMMARY][pos] = structure_chars[BETA_SHEET];
            }
        }
    }

    free(gap_chars);
#ifdef SHOW_PROGRESS
    cout << structures[summary] << endl;
#endif
}


/*! \brief Predicts protein secondary structures from the amino acid sequence.
 *
 *  \param[in]  sequence   Amino acid sequence
 *  \param[out] structures Predicted secondary structures (#ALPHA_HELIX, #BETA_SHEET,
 *                         #BETA_TURN and #STRUCTURE_SUMMARY, in this order)
 *  \param[in]  length     Size of \a sequence and \a structures[i]
 *  \return     Error description, if an error occurred; 0 otherwise
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
static GB_ERROR ED4_pfold_predict_structure(const unsigned char *sequence, char *structures[4], int length) {
#ifdef SHOW_PROGRESS
    cout << endl << "Predicting secondary structure for sequence:" << endl << sequence << endl;
#endif
    GB_ERROR error = 0;
    e4_assert((int)strlen((const char *)sequence) == length);

    // init memory
    ED4_pfold_init_statics();
    e4_assert(char2AA);

    // predict structure
    ED4_pfold_find_nucleation_sites(sequence, structures[ALPHA_HELIX], length, ALPHA_HELIX);
    ED4_pfold_find_nucleation_sites(sequence, structures[BETA_SHEET], length, BETA_SHEET);
    ED4_pfold_extend_nucleation_sites(sequence, structures[ALPHA_HELIX], length, ALPHA_HELIX);
    ED4_pfold_extend_nucleation_sites(sequence, structures[BETA_SHEET], length, BETA_SHEET);
    ED4_pfold_find_turns(sequence, structures[BETA_TURN], length);
    ED4_pfold_resolve_overlaps(sequence, structures, length);

    return error;
}

GB_ERROR ED4_pfold_calculate_secstruct_match(const unsigned char *structure_sai, const unsigned char *structure_cmp, const int start, const int end, char *result_buffer, PFOLD_MATCH_METHOD match_method) {
    GB_ERROR error = 0;
    e4_assert(structure_sai);
    e4_assert(structure_cmp);
    e4_assert(start >= 0);
    e4_assert(result_buffer);
    e4_assert(match_method >= 0 && match_method < PFOLD_MATCH_METHOD_COUNT);
    ED4_pfold_init_statics();
    e4_assert(char2AA);

    e4_assert(end >= start);

    size_t end_minus_start = size_t(end-start); // @@@ use this

    size_t length    = strlen((const char *)structure_sai);
    size_t match_end = std::min(std::min(end_minus_start, length), strlen((const char *)structure_cmp));

    enum { BEND = 3, NOSTRUCT = 4 };
    char *struct_chars[] = {
        strdup("HGI"),  // helical structures (enum ALPHA_HELIX)
        strdup("EB"),   // sheet-like structures (enum BETA_SHEET)
        strdup("T"),    // beta-turn (enum BETA_TURN)
        strdup("S"),    // bends (enum BEND)
        strdup("")      // no structure (enum NOSTRUCT)
    };

    // init awars
    AW_root *awr       = ED4_ROOT->aw_root;
    char    *gap_chars = awr->awar(ED4_AWAR_GAP_CHARS)->read_string();
    char *pairs[PFOLD_MATCH_TYPE_COUNT] = { 0 };
    char *pair_chars[PFOLD_MATCH_TYPE_COUNT] = { 0 };
    char *pair_chars_2 = awr->awar(PFOLD_AWAR_SYMBOL_TEMPLATE_2)->read_string();
    char  awar[256];

    for (int i = 0; pfold_match_type_awars[i].name; i++) {
        sprintf(awar, PFOLD_AWAR_PAIR_TEMPLATE, pfold_match_type_awars[i].name);
        pairs[i] = awr->awar(awar)->read_string();
        sprintf(awar, PFOLD_AWAR_SYMBOL_TEMPLATE, pfold_match_type_awars[i].name);
        pair_chars[i] = awr->awar(awar)->read_string();
    }

    int    struct_start   = start;
    int    struct_end     = start;
    size_t count          = 0;
    int    current_struct = 4;
    int    aa             = -1;
    double prob           = 0;

    // TODO: move this check to callback for the corresponding field?
    if (strlen(pair_chars_2) != 10) {
        error = GB_export_error("You have to define 10 match symbols.");
    }

    if (!error) {
        switch (match_method) {

        case SECSTRUCT_SECSTRUCT:
            // TODO: one could try to find out, if structure_cmp is really a secondary structure and not a sequence (define awar for allowed symbols in secondary structure)
            for (count = 0; count < match_end; count++) {
                result_buffer[count] = *pair_chars[STRUCT_UNKNOWN];
                for (int n_pt = 0; n_pt < PFOLD_MATCH_TYPE_COUNT; n_pt++) {
                    int len = strlen(pairs[n_pt])-1;
                    char *p = pairs[n_pt];
                    for (int j = 0; j < len; j += 3) {
                        if ((p[j] == structure_sai[count + start] && p[j+1] == structure_cmp[count + start]) ||
                             (p[j] == structure_cmp[count + start] && p[j+1] == structure_sai[count + start])) {
                            result_buffer[count] = *pair_chars[n_pt];
                            n_pt = PFOLD_MATCH_TYPE_COUNT; // stop searching the pair types
                            break; // stop searching the pairs array
                        }
                    }
                }
            }

            // fill the remaining buffer with spaces
            while (count <= end_minus_start) {
                result_buffer[count] = ' ';
                count++;
            }
            break;

        case SECSTRUCT_SEQUENCE:
            // clear result buffer
            for (size_t i = 0; i <= end_minus_start; i++) result_buffer[i] = ' ';

            // skip gaps
            while (structure_sai[struct_start] != '\0' && structure_cmp[struct_start] != '\0' &&
                    strchr(gap_chars, structure_sai[struct_start]) &&
                    strchr(gap_chars, structure_cmp[struct_start])) {
                struct_start++;
            }
            if (structure_sai[struct_start] == '\0' || structure_cmp[struct_start] == '\0') break;

            // check structure at the first displayed position and find out where it starts
            for (current_struct = 0; current_struct < 4 && !strchr(struct_chars[current_struct], structure_sai[struct_start]); current_struct++) {
                ;
            }
            if (current_struct != BEND && current_struct != NOSTRUCT) {
                struct_start--; // check structure left of start
                while (struct_start >= 0) {
                    // skip gaps
                    while (struct_start > 0 &&
                            strchr(gap_chars, structure_sai[struct_start]) &&
                            strchr(gap_chars, structure_cmp[struct_start])) {
                        struct_start--;
                    }
                    aa = char2AA[structure_cmp[struct_start]];
                    if (struct_start == 0 && aa == -1) { // nothing was found
                        break;
                    }
                    else if (strchr(struct_chars[current_struct], structure_sai[struct_start]) && aa != -1) {
                        prob += cf_former(aa, current_struct) - cf_breaker(aa, current_struct); // sum up probabilities
                        struct_start--;
                        count++;
                    }
                    else {
                        break;
                    }
                }
            }

            // parse structures
            struct_start = start;
            // skip gaps
            while (structure_sai[struct_start] != '\0' && structure_cmp[struct_start] != '\0' &&
                    strchr(gap_chars, structure_sai[struct_start]) &&
                    strchr(gap_chars, structure_cmp[struct_start])) {
                struct_start++;
            }
            if (structure_sai[struct_start] == '\0' || structure_cmp[struct_start] == '\0') break;
            struct_end = struct_start;
            while (struct_end < end) {
                aa = char2AA[structure_cmp[struct_end]];
                if (current_struct == NOSTRUCT) { // no structure found -> move on
                    struct_end++;
                }
                else if (aa == -1) { // structure found but no corresponding amino acid -> doesn't fit at all
                    result_buffer[struct_end - start] = pair_chars_2[0];
                    struct_end++;
                }
                else if (current_struct == BEND) { // bend found -> fits perfectly everywhere
                    result_buffer[struct_end - start] = pair_chars_2[9];
                    struct_end++;
                }
                else { // helix, sheet or beta-turn found -> while structure doesn't change: sum up probabilities
                    while (structure_sai[struct_end] != '\0') {
                        // skip gaps
                        while (strchr(gap_chars, structure_sai[struct_end]) &&
                                strchr(gap_chars, structure_cmp[struct_end]) &&
                                structure_sai[struct_end] != '\0' && structure_cmp[struct_end] != '\0') {
                            struct_end++;
                        }
                        aa = char2AA[structure_cmp[struct_end]];
                        if (structure_sai[struct_end] != '\0' && structure_cmp[struct_end] != '\0' &&
                             strchr(struct_chars[current_struct], structure_sai[struct_end]) && aa != -1) {
                            prob += cf_former(aa, current_struct) - cf_breaker(aa, current_struct); // sum up probabilities
                            struct_end++;
                            count++;
                        }
                        else {
                            break;
                        }
                    }

                    if (count != 0) {
                        // compute average and normalize probability
                        prob /= count;
                        prob = (prob + max_breaker_value[current_struct] - min_former_value[current_struct]) / (max_breaker_value[current_struct] + max_former_value[current_struct] - min_former_value[current_struct]);

#if 0 // code w/o effect
                        // map to match characters and store in result_buffer
                        int prob_normalized = ED4_pfold_round_sym(prob * 9);
                        // e4_assert(prob_normalized >= 0 && prob_normalized <= 9); // if this happens check if normalization is correct or some undefined characters mess everything up
                        char prob_symbol = *pair_chars[STRUCT_UNKNOWN];
                        if (prob_normalized >= 0 && prob_normalized <= 9) {
                            prob_symbol = pair_chars_2[prob_normalized];
                        }
#endif
                    }
                }

                // find next structure type
                if (structure_sai[struct_end] == '\0' || structure_cmp[struct_end] == '\0') {
                    break;
                }
                else {
                    prob = 0;
                    count = 0;
                    struct_start = struct_end;
                    for (current_struct = 0; current_struct < 4 && !strchr(struct_chars[current_struct], structure_sai[struct_start]); current_struct++) {
                        ;
                    }
                }
            }
            break;

        case SECSTRUCT_SEQUENCE_PREDICT:
            // predict structures from structure_cmp and compare with structure_sai
            char *structures[4];
            for (int i = 0; i < 4 && !error; i++) {
                structures[i] = new char [length + 1];
                if (!structures[i]) {
                    error = "Out of memory";
                }
                else {
                    for (size_t j = 0; j < length; j++) {
                        structures[i][j] = ' ';
                    }
                    structures[i][length] = '\0';
                }
            }
            if (!error) error = ED4_pfold_predict_structure(structure_cmp, structures, length);
            if (!error) {
                for (count = 0; count < match_end; count++) {
                    result_buffer[count] = *pair_chars[STRUCT_UNKNOWN];
                    if (!strchr(gap_chars, structure_sai[count + start]) && strchr(gap_chars, structure_cmp[count + start])) {
                        result_buffer[count] = *pair_chars[STRUCT_NO_MATCH];
                    } else if (strchr(gap_chars, structure_sai[count + start]) ||
                                (structures[ALPHA_HELIX][count + start] == ' ' && structures[BETA_SHEET][count + start] == ' ' && structures[BETA_TURN][count + start] == ' ')) {
                        result_buffer[count] = *pair_chars[STRUCT_PERFECT_MATCH];
                    }
                    else {
                        // search for good match first
                        // if found: stop searching
                        // otherwise: continue searching for a less good match
                        for (int n_pt = 0; n_pt < PFOLD_MATCH_TYPE_COUNT; n_pt++) {
                            int len = strlen(pairs[n_pt])-1;
                            char *p = pairs[n_pt];
                            for (int n_struct = 0; n_struct < 3; n_struct++) {
                                for (int j = 0; j < len; j += 3) {
                                    if ((p[j] == structures[n_struct][count + start] && p[j+1] == structure_sai[count + start]) ||
                                         (p[j] == structure_sai[count + start] && p[j+1] == structures[n_struct][count + start])) {
                                        result_buffer[count] = *pair_chars[n_pt];
                                        n_struct = 3; // stop searching the structures
                                        n_pt = PFOLD_MATCH_TYPE_COUNT; // stop searching the pair types
                                        break; // stop searching the pairs array
                                    }
                                }
                            }
                        }
                    }
                }
                // fill the remaining buffer with spaces
                while (count <= end_minus_start) {
                    result_buffer[count] = ' ';
                    count++;
                }
            }
            // free buffer
            for (int i = 0; i < 4; i++) {
                if (structures[i]) {
                    delete structures[i];
                    structures[i] = 0;
                }
            }
            break;

        default:
            e4_assert(0); // function called with invalid argument for 'match_method'
            break;
        }
    }

    free(gap_chars);
    free(pair_chars_2);
    for (int i = 0; pfold_match_type_awars[i].name; i++) {
        free(pairs[i]);
        free(pair_chars[i]);
    }
    if (error) for (size_t i = 0; i <= end_minus_start; i++) result_buffer[i] = ' '; // clear result buffer
    return error;
}


GB_ERROR ED4_pfold_set_SAI(char **protstruct, GBDATA *gb_main, const char *alignment_name, long *protstruct_len) {
    GB_ERROR        error         = 0;
    GB_transaction  ta(gb_main);
    AW_root        *aw_root       = ED4_ROOT->aw_root;
    char           *SAI_name      = aw_root->awar(PFOLD_AWAR_SELECTED_SAI)->read_string();
    GBDATA         *gb_protstruct = GBT_find_SAI(gb_main, SAI_name);

    freenull(*protstruct);

    if (gb_protstruct) {
        GBDATA *gb_data = GBT_find_sequence(gb_protstruct, alignment_name);
        if (gb_data) *protstruct = GB_read_string(gb_data);
    }

    if (*protstruct) {
        if (protstruct_len) *protstruct_len = (long)strlen(*protstruct);
    }
    else {
        if (protstruct_len) protstruct_len = 0;
        if (aw_root->awar(PFOLD_AWAR_ENABLE)->read_int()) {
            error = GBS_global_string("SAI \"%s\" does not exist.\nDisabled protein structure display!", SAI_name);
            aw_root->awar(PFOLD_AWAR_ENABLE)->write_int(0);
        }
    }

    free(SAI_name);
    return error;
}

/*! \brief Callback function to select the reference protein structure SAI and to
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

static void ED4_pfold_select_SAI_and_update_option_menu(AW_window *aww, AW_CL oms, AW_CL set_sai) {
    e4_assert(aww);
    AW_option_menu_struct *_oms = ((AW_option_menu_struct*)oms);
    e4_assert(_oms);
    char *selected_sai = ED4_ROOT->aw_root->awar(PFOLD_AWAR_SELECTED_SAI)->read_string();
    char *sai_filter   = ED4_ROOT->aw_root->awar(PFOLD_AWAR_SAI_FILTER)->read_string();

    if (set_sai) {
        const char *err = ED4_pfold_set_SAI(&ED4_ROOT->protstruct, GLOBAL_gb_main, ED4_ROOT->alignment_name, &ED4_ROOT->protstruct_len);
        if (err) aw_message(err);
    }

    aww->clear_option_menu(_oms);
    aww->insert_default_option(selected_sai, "", selected_sai);
    GB_transaction ta(GLOBAL_gb_main);

    for (GBDATA *sai = GBT_first_SAI(GLOBAL_gb_main);
         sai;
         sai = GBT_next_SAI(sai))
    {
        const char *sai_name = GBT_read_name(sai);
        if (strcmp(sai_name, selected_sai) != 0 && strstr(sai_name, sai_filter) != 0) {
            aww->callback(ED4_pfold_select_SAI_and_update_option_menu, (AW_CL)_oms, true);
            aww->insert_option(sai_name, "", sai_name);
        }
    }

    free(selected_sai);
    free(sai_filter);
    aww->update_option_menu();
    // ED4_expose_all_windows();
    // @@@ need update here ?
}

static void setup_pfold_config(AWT_config_definition& cdef) {
    cdef.add(PFOLD_AWAR_ENABLE, "enable");
    cdef.add(PFOLD_AWAR_SELECTED_SAI, "sai");
    cdef.add(PFOLD_AWAR_SAI_FILTER, "saifilter");
    cdef.add(PFOLD_AWAR_MATCH_METHOD, "matchmethod");
    cdef.add(PFOLD_AWAR_SYMBOL_TEMPLATE_2, "matchsymbols2");

    char awar[256];
    for (int i = 0; pfold_match_type_awars[i].name; i++) {
        const char *name = pfold_match_type_awars[i].name;
        sprintf(awar, PFOLD_AWAR_PAIR_TEMPLATE, name);
        cdef.add(awar, name);
        sprintf(awar, PFOLD_AWAR_SYMBOL_TEMPLATE, name);
        cdef.add(awar, GBS_global_string("%s_symbol", name));
    }
}

AW_window *ED4_pfold_create_props_window(AW_root *awr, const WindowCallback *refreshCallback) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(awr, "PFOLD_PROPS", "PROTEIN_MATCH_SETTINGS");

    // create close button
    aws->at(10, 10);
    aws->auto_space(5, 2);
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    // create help button
    aws->callback(makeHelpCallback("pfold_props.hlp"));
    aws->create_button("HELP", "HELP");

    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "pfold", makeConfigSetupCallback(setup_pfold_config));

    aws->at_newline();

    aws->label_length(27);
    int  ex = 0, ey = 0;
    char awar[256];

    // create toggle field for showing the protein structure match
    aws->label("Show protein structure match?");
    aws->callback(*refreshCallback);
    aws->create_toggle(PFOLD_AWAR_ENABLE);
    aws->at_newline();

    // create SAI option menu
    aws->label_length(30);
    aws->label("Selected Protein Structure SAI");
    AW_option_menu_struct *oms_sai = aws->create_option_menu(PFOLD_AWAR_SELECTED_SAI, true);
    ED4_pfold_select_SAI_and_update_option_menu(aws, (AW_CL)oms_sai, 0);
    aws->at_newline();
    aws->label("-> Filter SAI names for");
    aws->callback(ED4_pfold_select_SAI_and_update_option_menu, (AW_CL)oms_sai, 0);
    aws->create_input_field(PFOLD_AWAR_SAI_FILTER, 10);
    aws->at_newline();

    // create match method option menu
    PFOLD_MATCH_METHOD match_method = (PFOLD_MATCH_METHOD) ED4_ROOT->aw_root->awar(PFOLD_AWAR_MATCH_METHOD)->read_int();
    aws->label_length(12);
    aws->label("Match Method");
    aws->create_option_menu(PFOLD_AWAR_MATCH_METHOD, true);
    for (int i = 0; const char *mm_aw = pfold_match_method_awars[i].name; i++) {
        aws->callback(*refreshCallback);
        if (match_method == pfold_match_method_awars[i].value) {
            aws->insert_default_option(mm_aw, "", match_method);
        }
        else {
            aws->insert_option(mm_aw, "", pfold_match_method_awars[i].value);
        }
    }
    aws->update_option_menu();
    aws->at_newline();

    // create match symbols and/or match types input fields
    // TODO: show only fields that are relevant for current match method -> bind to callback function?
    aws->label_length(40);
    aws->label("Match Symbols (Range 0-100% in steps of 10%)");
    aws->callback(*refreshCallback);
    aws->create_input_field(PFOLD_AWAR_SYMBOL_TEMPLATE_2, 10);
    aws->at_newline();
    for (int i = 0; pfold_match_type_awars[i].name; i++) {
        aws->label_length(12);
        sprintf(awar, PFOLD_AWAR_PAIR_TEMPLATE, pfold_match_type_awars[i].name);
        aws->label(pfold_match_type_awars[i].name);
        aws->callback(*refreshCallback);
        aws->create_input_field(awar, 30);
        // TODO: is it possible to disable input field for STRUCT_UNKNOWN?
        // if (pfold_match_type_awars[i].value == STRUCT_UNKNOWN)
        if (!i) aws->get_at_position(&ex, &ey);
        sprintf(awar, PFOLD_AWAR_SYMBOL_TEMPLATE, pfold_match_type_awars[i].name);
        aws->callback(*refreshCallback);
        aws->create_input_field(awar, 3);
        aws->at_newline();
    }

    aws->window_fit();
    return (AW_window *)aws;
}

