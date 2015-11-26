/*! \file   ed4_protein_2nd_structure.hxx
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
 *  \attention The used method for secondary structure prediction is fast which
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

// #define SHOW_PROGRESS ///< Print information about progress to screen (for finding and extending structures and resolving overlaps).

#define PFOLD_AWAR_ENABLE            "Pfold/enable"       //!< Enable structure match.
#define PFOLD_AWAR_SELECTED_SAI      "Pfold/selected_SAI" //!< Selected reference protein secondary structure SAI (i.e. the SAI that is used for structure comparison).
#define PFOLD_AWAR_PAIR_TEMPLATE     "Pfold/pairs/%s"     //!< Structure pairs that define the match quality (see #pfold_pairs) as used for match methods #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT.
#define PFOLD_AWAR_SYMBOL_TEMPLATE   "Pfold/symbols/%s"   //!< Symbols for the match quality as used for match methods #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT.
#define PFOLD_AWAR_SYMBOL_TEMPLATE_2 "Pfold/symbols2"     //!< Symbols for the match quality as used for match method #SECSTRUCT_SEQUENCE.
#define PFOLD_AWAR_MATCH_METHOD      "Pfold/match_method" //!< Selected method for computing the match quality (see #PFOLD_MATCH_METHOD).
#define PFOLD_AWAR_SAI_FILTER        "Pfold/SAI_filter"   //!< Filter SAIs for given criteria (string); used in option menu for SAI selection.

// TODO: move static variables to .cpp file?

/*! \brief Protein secondary structure types.
 *
 *  Defines the various types of protein secondary structure. The order
 *  (or at least the individual values) are important, because they are
 *  used to access various arrays.
 */
enum PFOLD_STRUCTURE {
    ALPHA_HELIX       = 0, //!< Alpha-helix
    BETA_SHEET        = 1, //!< Beta-sheet
    BETA_TURN         = 2, //!< Beta-turn
    STRUCTURE_SUMMARY = 3, //!< Structure summary
//  THREE_TURN        = 4, ///< Three turn
//  FOUR_TURN         = 5, ///< Four turn
//  FIVE_TURN         = 6  ///< Five turn
};

//! Defines a name-value pair (e.g. for awars, menu entries, etc.).
struct name_value_pair {
    const char *name; //!< Name or description
    int        value; //!< Value attached to \a name
};

//! Match quality for secondary structure match.
enum PFOLD_MATCH_TYPE {
    STRUCT_PERFECT_MATCH,  //!< Perfect match
    STRUCT_GOOD_MATCH,     //!< Good match
    STRUCT_MEDIUM_MATCH,   //!< Medium match
    STRUCT_BAD_MATCH,      //!< Bad match
    STRUCT_NO_MATCH,       //!< No match
    STRUCT_UNKNOWN,        //!< Unknown structure
    PFOLD_MATCH_TYPE_COUNT //!< Number of match types
};

//! Awars for the match type; binds the #PFOLD_MATCH_TYPE to the corresponding awar name.
extern name_value_pair pfold_match_type_awars[];

#define PFOLD_PAIRS 6

//! Match pair definition (see #PFOLD_MATCH_TYPE) as used for match methods #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT in ED4_pfold_calculate_secstruct_match().
extern char *pfold_pairs[PFOLD_PAIRS];

//! Symbols for the match quality (defined by #PFOLD_MATCH_TYPE) as used for match methods #SECSTRUCT_SECSTRUCT and #SECSTRUCT_SEQUENCE_PREDICT in ED4_pfold_calculate_secstruct_match().
extern char *pfold_pair_chars[PFOLD_PAIRS];

/*! \brief Symbols for the match quality as used for match method
 *         #SECSTRUCT_SEQUENCE in ED4_pfold_calculate_secstruct_match().
 *
 *  The ten symbols represent the match quality ranging from 0 - 100% in
 *  steps of 10%.
 */

#define PFOLD_PAIR_CHARS_2 "##++~~--  "

//! Defines the methods for match computation. For details refer to ED4_pfold_calculate_secstruct_match().
enum PFOLD_MATCH_METHOD {
    SECSTRUCT_SECSTRUCT,        //!< Compare two protein secondary structures
    SECSTRUCT_SEQUENCE,         //!< Compare an amino acid sequence with a reference protein secondary structure
    SECSTRUCT_SEQUENCE_PREDICT, //!< Compare a full prediction of the protein secondary structure from its amino acid sequence with a reference protein secondary structure
    PFOLD_MATCH_METHOD_COUNT    //!< Number of match methods
};

/*! \brief Returns the former value of an amino acid depending on the given structure type.
 *
 *  The definition is used for method #SECSTRUCT_SEQUENCE in
 *  ED4_pfold_calculate_secstruct_match() to get the former value of an amino acid
 *  depending on the found structure type at its position. It addresses #cf_parameters
 *  for #ALPHA_HELIX and #BETA_SHEET and #cf_parameters_norm for #BETA_TURN.
 */
#define cf_former(aa, strct) ((strct!=2) ? cf_parameters[aa][strct] : cf_parameters_norm[aa][strct])

/*! \brief Returns the breaker value of an amino acid depending on the given structure type.
 *
 *  The definition is used for method #SECSTRUCT_SEQUENCE in
 *  ED4_pfold_calculate_secstruct_match() to get the breaker value of an amino acid
 *  depending on the found structure type at its position. It addresses #cf_parameters
 *  for #ALPHA_HELIX and #BETA_SHEET and returns 0 for #BETA_SHEET, because it has no
 *  breaker values.
 */
#define cf_breaker(aa, strct) ((strct!=2) ? cf_parameters[aa][strct+2] : 0)


// General ED4 functions //////////////////////////////////////////////////////////////////////////////////////////////////////////



/*! \brief Compares a protein secondary structure with a primary structure or another
 *         secondary structure.
 *
 *  \param[in]  structure_sai Reference protein structure SAI (secondary structure)
 *  \param[in]  structure_cmp Protein structure to compare (primary or secondary structure)
 *  \param[in]  start         The start of the match computation (visible area in editor)
 *  \param[in]  end           The end of the match computation (visible area in editor)
 *  \param[out] result_buffer Result buffer for match symbols
 *  \param[in]  match_method  Method for structure match computation
 *  \return     Error description, if an error occurred; 0 otherwise
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
 *       it is mapped to the match symbols defined by #PFOLD_PAIR_CHARS_2. Note
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
GB_ERROR ED4_pfold_calculate_secstruct_match(const unsigned char *structure_sai, const unsigned char *structure_cmp, int start, int end, char *result_buffer, PFOLD_MATCH_METHOD match_method = SECSTRUCT_SEQUENCE);

/*! \brief Sets the reference protein secondary structure SAI.
 *
 *  \param[out] protstruct     Pointer to reference protein secondary structure SAI
 *  \param[in]  gb_main        Main database
 *  \param[in]  alignment_name Name of the alignment to search for
 *  \param[out] protstruct_len Length of reference protein secondary structure SAI
 *  \return     Error description, if an error occurred; 0 otherwise
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

/*! \brief Creates the "Protein Match Settings" window.
 *
 *  \param[in] awr   Root window
 *  \param[in] cb    Callback struct
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
 *  and #pfold_pair_chars or #PFOLD_PAIR_CHARS_2). Via a filter (bound to
 *  #PFOLD_AWAR_SAI_FILTER) the SAIs shown in the option menu can be narrowed down to
 *  a selection of SAIs whose names contain the specified string. The callback function
 *  #ED4_pfold_select_SAI_and_update_option_menu() is bound to the SAI option menu and
 *  the SAI filter to update the selected SAI in the editor or the selection in the
 *  SAI option menu.
 */
AW_window *ED4_pfold_create_props_window(AW_root *awr, const WindowCallback *refreshCallback);

#endif
