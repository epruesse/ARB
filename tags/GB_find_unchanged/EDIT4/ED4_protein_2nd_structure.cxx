/** \file   ED4_protein_2nd_structure.cxx
 *  \brief  Implements the functions defined in ed4_protein_2nd_structure.hxx.
 *  \author Markus Urban
 *  \date   2008-02-08
 *  \sa     Refer to ed4_protein_2nd_structure.hxx for details, please.
*/

#include <iostream>
#include <malloc.h>

#include "arbdb.h"
#include "arbdbt.h"
#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_protein_2nd_structure.hxx"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define e4_assert(bed) arb_assert(bed)

using namespace std;


void ED4_pfold_init_statics() {
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


GB_ERROR ED4_pfold_predict_structure(const char *sequence, char *structure, int length) {
    GB_ERROR error = 0;
    
    // allocate memory
    char *structures[4];
    for (int i = 0; i < 4; i++) {
        structures[i] = new char [length + 1];
        if (!structures[i]) {
            error = GB_export_error("Out of memory.");
            return error;
        }
        for (int j = 0; j < length; j++) {
            structures[i][j] = ' ';
        }
        structures[i][length] = '\0';
    }
    
    // predict structure
    error = ED4_pfold_predict_structure(sequence, structures, length);
    
    // write predicted summary to result_buffer
    if (structure) {
        for (int i = 0; i < length; i++) {
            structure[i] = structures[STRUCTURE_SUMMARY][i];
        }
        structure[length] = '\0';
    }
    
    // free buffer and return
    for (int i = 0; i < 4; i++) {
        if (structures[i]) {
            delete structures[i];
            structures[i] = 0;
        }
    }
    return error;
}


GB_ERROR ED4_pfold_predict_structure(const char *sequence, char *structures[4], int length) {
#ifdef SHOW_PROGRESS
    cout << endl << "Predicting secondary structure for sequence:" << endl << sequence << endl;
#endif
    GB_ERROR error = 0;
    e4_assert((int) strlen(sequence) == length);
    
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


GB_ERROR ED4_pfold_calculate_secstruct_match(const char *structure_sai, const char *structure_cmp, int start, int end, char *result_buffer, PFOLD_MATCH_METHOD match_method /*= SECSTRUCT_SEQUENCE*/) {
    GB_ERROR error = 0;
    e4_assert(structure_sai);
    e4_assert(structure_cmp);
    e4_assert(start >= 0);
    e4_assert(result_buffer);
    e4_assert(match_method >= 0 && match_method < PFOLD_MATCH_METHOD_COUNT);
    ED4_pfold_init_statics();
    e4_assert(char2AA);
    int length = (int) strlen(structure_sai);
    int match_end = min( min(end - start, length), (int) strlen(structure_cmp) );
    
    enum {BEND = 3, NOSTRUCT = 4};
    char *struct_chars[] = {
        strdup("HGI"),  // helical structures (enum ALPHA_HELIX)
        strdup("EB"),   // sheet-like structures (enum BETA_SHEET)
        strdup("T"),    // beta-turn (enum BETA_TURN)
        strdup("S"),    // bends (enum BEND)
        strdup("")      // no structure (enum NOSTRUCT)
    };
    
    // init awars
    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string();
    char *pairs[PFOLD_MATCH_TYPE_COUNT] = {0};
    char *pair_chars[PFOLD_MATCH_TYPE_COUNT] = {0};
    char *pair_chars_2 = ED4_ROOT->aw_root->awar(PFOLD_AWAR_SYMBOL_TEMPLATE_2)->read_string();
    char awar[256];
    for (int i = 0; pfold_match_type_awars[i].name; i++) {
        sprintf(awar, PFOLD_AWAR_PAIR_TEMPLATE, pfold_match_type_awars[i].name);
        pairs[i] = strdup(ED4_ROOT->aw_root->awar(awar)->read_string());
        sprintf(awar, PFOLD_AWAR_SYMBOL_TEMPLATE, pfold_match_type_awars[i].name);
        pair_chars[i] = strdup(ED4_ROOT->aw_root->awar(awar)->read_string());
    }
    
    int struct_start = start;
    int struct_end = start;
    int count = 0;
    int current_struct = 4;
    int aa = -1;
    double prob = 0;
    
    //TODO: move this check to callback for the corresponding field?
    if (strlen(pair_chars_2) != 10) {
        error = GB_export_error("You have to define 10 match symbols.");
    }
    
    if (!error) {
        switch (match_method) {
        
        case SECSTRUCT_SECSTRUCT:
            //TODO: one could try to find out, if structure_cmp is really a secondary structure and not a sequence (define awar for allowed symbols in secondary structure)
            for (count = 0; count < match_end; count++) {
                result_buffer[count] = *pair_chars[STRUCT_UNKNOWN];
                for (int n_pt = 0; n_pt < PFOLD_MATCH_TYPE_COUNT; n_pt++) {
                    int len = strlen(pairs[n_pt])-1;
                    char *p = pairs[n_pt];
                    for (int j = 0; j < len; j += 3) {
                        if ( (p[j] == structure_sai[count + start] && p[j+1] == structure_cmp[count + start]) ||
                             (p[j] == structure_cmp[count + start] && p[j+1] == structure_sai[count + start]) ) {
                            result_buffer[count] = *pair_chars[n_pt];
                            n_pt = PFOLD_MATCH_TYPE_COUNT; // stop searching the pair types 
                            break; // stop searching the pairs array
                        }
                    }
                }
            }
            
            // fill the remaining buffer with spaces
            while (count <= end - start) {
                result_buffer[count] = ' ';
                count++;
            }
            break;
            
        case SECSTRUCT_SEQUENCE:
            // clear result buffer
            for (int i = 0; i <= end - start; i++) result_buffer[i] = ' ';
                
            // skip gaps
            while ( structure_sai[struct_start] != '\0' && structure_cmp[struct_start] != '\0' &&
                    strchr(gap_chars, structure_sai[struct_start]) && 
                    strchr(gap_chars, structure_cmp[struct_start]) ) {
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
                    while ( struct_start > 0 &&
                            strchr(gap_chars, structure_sai[struct_start]) && 
                            strchr(gap_chars, structure_cmp[struct_start]) ) {
                        struct_start--;
                    }
                    aa = char2AA[structure_cmp[struct_start]];
                    if (struct_start == 0 && aa == -1) { // nothing was found
                        break;
                    } else if (strchr(struct_chars[current_struct], structure_sai[struct_start]) && aa != -1) {
                        prob += cf_former(aa, current_struct) - cf_breaker(aa, current_struct); // sum up probabilities
                        struct_start--;
                        count++;
                    } else {
                        break;
                    }
                }
            }
            
            // parse structures
            struct_start = start;
            // skip gaps
            while ( structure_sai[struct_start] != '\0' && structure_cmp[struct_start] != '\0' &&
                    strchr(gap_chars, structure_sai[struct_start]) && 
                    strchr(gap_chars, structure_cmp[struct_start]) ) {
                struct_start++;
            }
            if (structure_sai[struct_start] == '\0' || structure_cmp[struct_start] == '\0') break;
            struct_end = struct_start;
            while (struct_end < end ) {
                aa = char2AA[structure_cmp[struct_end]];
                if (current_struct == NOSTRUCT) { // no structure found -> move on
                    struct_end++;
                } else if (aa == -1) { // structure found but no corresponding amino acid -> doesn't fit at all 
                    result_buffer[struct_end - start] = pair_chars_2[0];
                    struct_end++;
                } else if (current_struct == BEND) { // bend found -> fits perfectly everywhere
                    result_buffer[struct_end - start] = pair_chars_2[9];
                    struct_end++;
                } else { // helix, sheet or beta-turn found -> while structure doesn't change: sum up probabilities
                    while (structure_sai[struct_end] != '\0') {
                        // skip gaps
                        while ( strchr(gap_chars, structure_sai[struct_end]) && 
                                strchr(gap_chars, structure_cmp[struct_end]) &&
                                structure_sai[struct_end] != '\0' && structure_cmp[struct_end] != '\0' ) {
                            struct_end++;
                        }
                        aa = char2AA[structure_cmp[struct_end]];
                        if ( structure_sai[struct_end] != '\0' && structure_cmp[struct_end] != '\0' &&
                             strchr(struct_chars[current_struct], structure_sai[struct_end]) && aa != -1 ) {
                            prob += cf_former(aa, current_struct) - cf_breaker(aa, current_struct); // sum up probabilities
                            struct_end++;
                            count++;
                        } else {
                            break;
                        }
                    }
                    
                    if (count != 0) {
                        // compute average and normalize probability
                        prob /= count;
                        prob = (prob + max_breaker_value[current_struct] - min_former_value[current_struct]) / (max_breaker_value[current_struct] + max_former_value[current_struct] - min_former_value[current_struct]);
                        
                        // map to match characters and store in result_buffer
                        int prob_normalized = ED4_pfold_round_sym(prob * 9);
                        //e4_assert(prob_normalized >= 0 && prob_normalized <= 9); // if this happens check if normalization is correct or some undefined charachters mess everything up
                        char prob_symbol = *pair_chars[STRUCT_UNKNOWN];
                        if (prob_normalized >= 0 && prob_normalized <= 9) {
                            prob_symbol = pair_chars_2[prob_normalized]; 
                        }
                        for (int i = struct_start - start; i < struct_end - start && i < (end - start); i++) {
                            if (char2AA[structure_cmp[i + start]] != -1) result_buffer[i] = prob_symbol;
                        }
                    }
                }
                
                // find next structure type
                if (structure_sai[struct_end] == '\0' || structure_cmp[struct_end] == '\0') {
                    break;
                } else {
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
            for (int i = 0; i < 4; i++) {
                structures[i] = new char [length + 1];
                if (!structures[i]) {
                    error = GB_export_error("Out of memory.");
                    return error;
                }
                for (int j = 0; j < length; j++) {
                    structures[i][j] = ' ';
                }
                structures[i][length] = '\0';
            }
            error = ED4_pfold_predict_structure(structure_cmp, structures, length);
            if (!error) {
                for (count = 0; count < match_end; count++) {
                    result_buffer[count] = *pair_chars[STRUCT_UNKNOWN];
                    if (!strchr(gap_chars, structure_sai[count + start]) && strchr(gap_chars, structure_cmp[count + start])) {
                        result_buffer[count] = *pair_chars[STRUCT_NO_MATCH];
                    } else if ( strchr(gap_chars, structure_sai[count + start]) || 
                                (structures[ALPHA_HELIX][count + start] == ' ' && structures[BETA_SHEET][count + start] == ' ' && structures[BETA_TURN][count + start] == ' ') ) {
                        result_buffer[count] = *pair_chars[STRUCT_PERFECT_MATCH];
                    } else {
                        // search for good match first
                        // if found: stop searching
                        // otherwise: continue searching for a less good match
                        for (int n_pt = 0; n_pt < PFOLD_MATCH_TYPE_COUNT; n_pt++) {
                            int len = strlen(pairs[n_pt])-1;
                            char *p = pairs[n_pt];
                            for (int n_struct = 0; n_struct < 3; n_struct++) {
                                for (int j = 0; j < len; j += 3) {
                                    if ( (p[j] == structures[n_struct][count + start] && p[j+1] == structure_sai[count + start]) ||
                                         (p[j] == structure_sai[count + start] && p[j+1] == structures[n_struct][count + start]) ) {
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
                while (count <= end - start) {
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
    if (error) for (int i = 0; i <= end - start; i++) result_buffer[i] = ' '; // clear result buffer
    return error;
}


GB_ERROR ED4_pfold_set_SAI(char **protstruct, GBDATA *gb_main, const char *alignment_name, long *protstruct_len /*= 0*/) {
    GB_ERROR error = 0;
    GB_transaction dummy(gb_main);
    char *SAI_name = ED4_ROOT->aw_root->awar(PFOLD_AWAR_SELECTED_SAI)->read_string();
    GBDATA *gb_protstruct = GBT_find_SAI(gb_main, SAI_name);
    
    free(*protstruct);
    *protstruct = 0;
    
    if (gb_protstruct) {
        GBDATA *gb_data = GBT_read_sequence(gb_protstruct, alignment_name);
        if (gb_data) {
            *protstruct = GB_read_string(gb_data);
        }
    }
    
    if (*protstruct) {
        if (protstruct_len) *protstruct_len = (long)strlen(*protstruct);
    } else {
        if (protstruct_len) protstruct_len = 0;
        error = GB_export_error( "No SAI \"%s\" found. Protein structure information will not be displayed. "
                                 "Check \"Properties -> Protein Match Settings\".", SAI_name );
    }
    
    free(SAI_name);
    return error;
}


AW_window *ED4_pfold_create_props_window(AW_root *awr, AW_cb_struct *awcbs) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init( awr, "PFOLD_PROPS", "PROTEIN_MATCH_SETTINGS");
    
    // create close button
    aws->at(10, 10);
    aws->auto_space(5, 2);
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    // create help button
    aws->callback(AW_POPUP_HELP, (AW_CL)"pfold_props.hlp");
    aws->create_button("HELP", "HELP");
    aws->at_newline();

    aws->label_length(27);
    int  ex = 0, ey = 0;
    char awar[256];
    
    // create toggle field for showing the protein structure match
    aws->label("Show protein structure match?");
    aws->callback(awcbs);
    aws->create_toggle(PFOLD_AWAR_ENABLE);
    aws->at_newline();
    
    // create SAI option menu
    aws->label_length(30);
    AW_option_menu_struct *oms_sai = aws->create_option_menu(PFOLD_AWAR_SELECTED_SAI, "Selected Protein Structure SAI", "");
    ED4_pfold_select_SAI_and_update_option_menu(aws, (AW_CL)oms_sai, 0);
    aws->at_newline();
    aws->label("-> Filter SAI names for");
    aws->callback(ED4_pfold_select_SAI_and_update_option_menu, (AW_CL)oms_sai, 0);
    aws->create_input_field(PFOLD_AWAR_SAI_FILTER, 10);
    aws->at_newline();
    
    // create match method option menu
    PFOLD_MATCH_METHOD match_method = (PFOLD_MATCH_METHOD) ED4_ROOT->aw_root->awar(PFOLD_AWAR_MATCH_METHOD)->read_int();
    aws->label_length(12);
    aws->create_option_menu(PFOLD_AWAR_MATCH_METHOD, "Match Method", "");
    for (int i = 0; const char *mm_aw = pfold_match_method_awars[i].name; i++) {
        aws->callback(awcbs);
        if (match_method == pfold_match_method_awars[i].value) {
            aws->insert_default_option(mm_aw, "", match_method);
        } else {
            aws->insert_option(mm_aw, "", pfold_match_method_awars[i].value);
        }
    }
    aws->update_option_menu();
    aws->at_newline();
    
    // create match symbols and/or match types input fields
    //TODO: show only fields that are relevant for current match method -> bind to callback function?
    //if (match_method == SECSTRUCT_SEQUENCE) {
        aws->label_length(40);
        aws->label("Match Symbols (Range 0-100% in steps of 10%)");
        aws->callback(awcbs);
        aws->create_input_field(PFOLD_AWAR_SYMBOL_TEMPLATE_2, 10);
        aws->at_newline();
    //} else {
        for (int i = 0; pfold_match_type_awars[i].name; i++) {
            aws->label_length(12);
            sprintf(awar, PFOLD_AWAR_PAIR_TEMPLATE, pfold_match_type_awars[i].name);
            aws->label(pfold_match_type_awars[i].name);
            aws->callback(awcbs);
            aws->create_input_field(awar, 30);
            //TODO: is it possible to disable input field for STRUCT_UNKNOWN?
            //if (pfold_match_type_awars[i].value == STRUCT_UNKNOWN)
            if (!i) aws->get_at_position(&ex, &ey);
            sprintf(awar, PFOLD_AWAR_SYMBOL_TEMPLATE, pfold_match_type_awars[i].name);
            aws->callback(awcbs);
            aws->create_input_field(awar, 3);
            aws->at_newline();
        }
    //}
    
    aws->window_fit();
    return (AW_window *)aws;
}


void ED4_pfold_select_SAI_and_update_option_menu(AW_window *aww, AW_CL oms, AW_CL set_sai) {
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
    GB_transaction dummy(GLOBAL_gb_main);
    GBDATA *sai = GBT_first_SAI(GLOBAL_gb_main);
    while (sai) {
        char *sai_name = GBT_read_name(sai);
        if (strcmp(sai_name, selected_sai) && strstr(sai_name, sai_filter)) {
            aww->callback(ED4_pfold_select_SAI_and_update_option_menu, (AW_CL)_oms, true);
            aww->insert_option(sai_name, "", sai_name);
        }
        sai = GBT_next_SAI(sai);
    }
    
    free(selected_sai);
    free(sai_filter);
    aww->update_option_menu();
    ED4_expose_all_windows();
}


void ED4_pfold_find_structure(const char *sequence, char *structure, int length, const PFOLD_STRUCTURE s) {
    e4_assert(s == ALPHA_HELIX || s == BETA_SHEET || s == BETA_TURN); // incorrect value for s
    e4_assert(char2AA); // char2AA not initialized; ED4_pfold_init_statics() failed or hasn't been called yet
    if (s == BETA_TURN) {
        ED4_pfold_find_turns(sequence, structure, length);
    } else {
        ED4_pfold_find_nucleation_sites(sequence, structure, length, s);
        ED4_pfold_extend_nucleation_sites(sequence, structure, length, s);
    }
}


void ED4_pfold_find_nucleation_sites(const char *sequence, char *structure, int length, const PFOLD_STRUCTURE s) {
#ifdef SHOW_PROGRESS
    cout << endl << "Searching for nucleation sites:" << endl;
#endif
    e4_assert(s == ALPHA_HELIX || s == BETA_SHEET); // incorrect value for s
    e4_assert(char2AA); // char2AA not initialized; ED4_pfold_init_statics() failed or hasn't been called yet
    
    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string(); // gap characters
    int windowSize = (s == ALPHA_HELIX ? 6 : 5); // window size used for finding nucleation sites
    double sumOfFormVal = 0, sumOfBreakVal = 0; // sum of former resp. breaker values in window
    int pos; // current position in sequence
    int count; // number of amino acids found in window
    int aa; // amino acid

    for (int i = 0; i < ((length + 1) - windowSize); i++) {
        pos = i;
        for (count = 0; count < windowSize; count++) {
            // skip gaps
            while ( pos < ((length + 1) - windowSize) && 
                    strchr(gap_chars, sequence[pos + count]) ) {
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


void ED4_pfold_extend_nucleation_sites(const char *sequence, char *structure, int length, const PFOLD_STRUCTURE s) {
#ifdef SHOW_PROGRESS
    cout << endl << "Extending nucleation sites:" << endl;
#endif
    e4_assert(s == ALPHA_HELIX || s == BETA_SHEET); // incorrect value for s
    e4_assert(char2AA); // char2AA not initialized; ED4_pfold_init_statics() failed or hasn't been called yet

    bool break_structure;   // break the current structure    
    int start = 0, end = 0; // start and end of nucleation site
    int neighbor = 0;       // neighbor of start or end
    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string(); // gap characters

    // find nucleation sites and extend them in both directions (for whole sequence)
    for (int indStruct = 0; indStruct < length; indStruct++) {
        
        // search start and end of nucleated region
        while ( indStruct < length && 
                (structure[indStruct] == ' ') || strchr(gap_chars, sequence[indStruct])) {
            indStruct++;
        }
        if (indStruct >= length) break;
        // get next amino acid that is not included in nucleation site
        start = indStruct - 1;
        while ( indStruct < length && 
                (structure[indStruct] != ' ' || strchr(gap_chars, sequence[indStruct])) ) {
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
            break_structure = strchr(structure_breaker[s], sequence[start]);
            neighbor = start - 1; // get neighbor
            while (neighbor > 0 && strchr(gap_chars, sequence[neighbor])) {
                neighbor--; // skip gaps
            }
            // break if out of bounds or no amino acid is found
            if (neighbor <= 0 || char2AA[sequence[neighbor]] == -1) {
                break;
            }
            // break if another breaker or indifferent amino acid is found
            break_structure &= strchr(structure_breaker[s], sequence[neighbor]) ||
                               strchr(structure_indifferent[s], sequence[neighbor]);
            if (!break_structure) {
                structure[start] = structure_chars[s];
            }
            start = neighbor;  // continue with neigbor
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
            break_structure = strchr(structure_breaker[s], sequence[end]);
            neighbor = end + 1; // get neighbor
            while (neighbor < (length - 2) && strchr(gap_chars, sequence[neighbor])) {
                neighbor++; // skip gaps
            }
            // break if out of bounds or no amino acid is found
            if (neighbor >= (length - 1) || char2AA[sequence[neighbor]] == -1) {
                end = neighbor;
                break;
            }
            // break if another breaker or indifferent amino acid is found
            break_structure &= strchr(structure_breaker[s], sequence[neighbor]) ||
                               strchr(structure_indifferent[s], sequence[neighbor]);
            if (!break_structure) {
                structure[end] = structure_chars[s];
            }
            end = neighbor; // continue with neigbor
        }
        indStruct = end; // continue with end
    }

    free(gap_chars);
#ifdef SHOW_PROGRESS
    cout << structure << endl;
#endif
}


void ED4_pfold_find_turns(const char *sequence, char *structure, int length) {
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
            while ( pos < ((length + 1) - windowSize) && 
                    strchr(gap_chars, sequence[pos + count]) ) {
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
        //if (count == 4) {
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


void ED4_pfold_resolve_overlaps(const char *sequence, char *structures[4], int length) {
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
        } else if ((structures[ALPHA_HELIX][pos] != ' ') && (structures[BETA_SHEET][pos] != ' ')) {

            // search start and end of overlap (as long as no beta-turn is found)
            start = pos;
            end = pos;
            while ( structures[ALPHA_HELIX][end] != ' ' && structures[BETA_SHEET][end] != ' ' && 
                    structures[BETA_TURN][end] == ' ' ) {
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
        } else {
            // summary at position pos is helix resp. sheet
            if (structures[ALPHA_HELIX][pos] != ' ') {
                structures[STRUCTURE_SUMMARY][pos] = structure_chars[ALPHA_HELIX];
            } else if (structures[BETA_SHEET][pos] != ' ') {
                structures[STRUCTURE_SUMMARY][pos] = structure_chars[BETA_SHEET];
            }
        }
    }
    
    free(gap_chars);
#ifdef SHOW_PROGRESS
    cout << structures[summary] << endl;
#endif
}


inline int ED4_pfold_round_sym(double d) {
    return int(d + .5);
}
