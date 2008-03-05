#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include <iostream>
#include <fstream>
#include <iomanip>

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


//TODO: remove all exits and establish proper error handling

GB_ERROR ED4_pfold_init_memory(const char *sequence_) {
    GB_ERROR error = 0;
    
    // assign sequence pointer and length
    e4_assert(sequence_);
    sequence = sequence_;
    length = strlen(sequence);
    e4_assert(int(strlen(sequence_)) == length);
    
    // allocate memory for structures and initialize it
    for (int i = 0; i < 4; i++) {
        if (structures[i]) {
            delete[] structures[i];
            structures[i] = 0;
        }
        structures[i] = new char [length + 1];
        if (!structures[i]) {
            error = "Out of memory";
        } else {
            for (int j = 0; j < length; j++) {
                structures[i][j] = ' ';
            }
            structures[i][length] = '\0';
        }
    }
    
    // allocate memory for probabilities
    for (int i = 0; i < 2; i++) {
        if (probabilities[i]) {
            delete[] probabilities[i];
            probabilities[i] = 0;
        }
        probabilities[i] = new char [length + 1];
        if (!probabilities[i]) {
            error = "Out of memory";
        } else {
            for (int j = 0; j < length; j++) {
                probabilities[i][j] = ' ';
            }
            probabilities[i][length] = '\0';
        }
    }
    
    return error;
}


GB_ERROR ED4_pfold_predict_summary(const char *sequence_, char *result_buffer, int result_buffer_size) {
    GB_ERROR error = 0;
    
    // init memory
    ED4_pfold_init_statics();
    error = ED4_pfold_init_memory(sequence_);
    if (error) return error;
    
    // predict structure
    ED4_pfold_find_nucleation_sites(helix);
    ED4_pfold_find_nucleation_sites(sheet);
    ED4_pfold_extend_nucleation_sites(helix);
    ED4_pfold_extend_nucleation_sites(sheet);
    ED4_pfold_find_turns();
    ED4_pfold_resolve_overlaps();
    
    // write predicted summary to result_buffer
    if (result_buffer) {
        e4_assert(int(strlen(result_buffer)) == result_buffer_size);
        e4_assert(result_buffer_size > length);
        for (int i = 0; i < length; i++) {
            result_buffer[i] = structures[summary][i];
        }
        result_buffer[length] = '\0';
    }
    
    return error;
}


GB_ERROR ED4_pfold_calculate_secstruct_match(const char *structure_sai, const char *structure_cmp, int start, int end, char *result_buffer, PFOLD_MATCH_METHOD match_type /*= SECSTRUCT_SEQUENCE*/) {
    GB_ERROR error = 0;
    e4_assert(structure_sai);
    e4_assert(structure_cmp);
    e4_assert(start >= 0);
    e4_assert(result_buffer);
    e4_assert(match_type >= 0 && match_type < PFOLD_MATCH_METHOD_COUNT);
    ED4_pfold_init_statics();
    int match_end = min( min(end - start, (int)strlen(structure_sai)), (int)strlen(structure_cmp) );
    
    enum {bend = 3, nostruct = 4};
    char *struct_chars[] = {
        strdup("HGI"),  // helical structures (enum helix)
        strdup("EB"),   // sheet-like structures (enum sheet)
        strdup("T"),    // beta turn (enum beta_turn)
        strdup("S"),    // bends (enum bend)
        strdup("")      // no structure (enum nostruct)
    };
    
    // init awars
    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string();
    char *pairs[PFOLD_MATCH_TYPE_COUNT] = {0};
    char *pair_chars[PFOLD_MATCH_TYPE_COUNT] = {0};
    char *pair_chars_2 = ED4_ROOT->aw_root->awar(PFOLD_AWAR_SYMBOL_TEMPLATE_2)->read_string();
    char awar[256];
    //TODO: check if this is neccessary or if static variables already contain the current values
    for (int i = 0; pfold_match_type_awars[i].awar; i++) {
        sprintf(awar, PFOLD_AWAR_PAIR_TEMPLATE, pfold_match_type_awars[i].awar);
        pairs[i] = strdup(ED4_ROOT->aw_root->awar(awar)->read_string());
        sprintf(awar, PFOLD_AWAR_SYMBOL_TEMPLATE, pfold_match_type_awars[i].awar);
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
        error = "You have to define 10 match symbols.";
    }
    
    if (!error) {
        switch (match_type) {
        
        case SECSTRUCT:
            //TODO: one could try to find out, if structure_cmp is really a secondary structure and not a sequence (define awar for allowed symbols in secondary structure)
            for (count = 0; count < match_end; count++) {
                result_buffer[count] = ' ';
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
            if (current_struct != bend && current_struct != nostruct) {
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
                        prob += cf_former(current_struct, aa) - cf_breaker(current_struct, aa); // sum up probabilities
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
                if (current_struct == nostruct) { // no structure found -> move on
                    struct_end++;
                } else if (aa == -1) { // structure found but no corresponding amino acid -> doesn't fit at all 
                    result_buffer[struct_end - start] = pair_chars_2[0];
                    struct_end++;
                } else if (current_struct == bend) { // bend found -> fits perfectly everywhere
                    result_buffer[struct_end - start] = pair_chars_2[9];
                    struct_end++;
                } else { // helix, sheet or beta turn found -> while structure doesn't change: sum up probabilities
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
                            prob += cf_former(current_struct, aa) - cf_breaker(current_struct, aa); // sum up probabilities
                            struct_end++;
                            count++;
                        } else {
                            break;
                        }
                    }
                    
                    if (count != 0) {
                        // compute average and normalize probability
                        prob /= count;
                        prob = (prob + maxProbBreak[current_struct] - minProb[current_struct]) / (maxProbBreak[current_struct] + maxProb[current_struct] - minProb[current_struct]);
                        
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
            error = ED4_pfold_predict_summary(structure_cmp, 0, 0);
            if (!error) {
                for (count = 0; count < match_end; count++) {
                    result_buffer[count] = *pair_chars[STRUCT_UNKNOWN];
                    if (!strchr(gap_chars, structure_sai[count + start]) && strchr(gap_chars, structure_cmp[count + start])) {
                        result_buffer[count] = *pair_chars[STRUCT_NO_MATCH];
                    } else if ( strchr(gap_chars, structure_sai[count + start]) || 
                                (structures[helix][count + start] == ' ' && structures[sheet][count + start] == ' ' && structures[beta_turn][count + start] == ' ') ) {
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
            break;
            
        default:
            break;
        }
    }
    
    free(gap_chars);
    free(pair_chars_2);
    for (int i = 0; pfold_match_type_awars[i].awar; i++) {
        if (pairs[i]) free(pairs[i]);
        if (pair_chars[i]) free(pair_chars[i]);
    }
    if (error) for (int i = 0; i <= end - start; i++) result_buffer[i] = ' '; // clear result buffer
    return error;
}


//TODO: Ralf zeigen; alte Funktionen löschen, wenn sie nicht mehr gebraucht werden
char *ED4_pfold_find_protein_structure_SAI(const char *SAI_name, GBDATA *gbm, const char *alignment_name) {
    GB_transaction dummy(gbm);
    GBDATA *gb_protstruct = GBT_find_SAI(gbm, SAI_name);
    if (gb_protstruct) {
        GBDATA *gb_data = GBT_read_sequence(gb_protstruct, alignment_name);
        if (gb_data) {
            return GB_read_string(gb_data);
        }
    }
    return 0;
}


GB_ERROR ED4_pfold_set_SAI(char **protstruct, GBDATA *gbm, const char *alignment_name, long *protstruct_len /*= 0*/) {
    GB_ERROR error = 0;
    char *SAI_name = ED4_ROOT->aw_root->awar(PFOLD_AWAR_SELECTED_SAI)->read_string();
    
    *protstruct = ED4_pfold_find_protein_structure_SAI(SAI_name, gbm, alignment_name);
    if (*protstruct) {
        if (protstruct_len) *protstruct_len = (long)strlen(*protstruct);
    } else {
        if (protstruct_len) protstruct_len = 0;
        error = GB_export_error( "No SAI \"%s\" found. Protein structure information will not be displayed. "
                                 "Check \"Properties -> Protein Match Settings\".", SAI_name );
    }
    return error;
}


AW_window *ED4_pfold_create_props_window(AW_root *awr, AW_cb_struct *awcbs) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init( awr, "PFOLD_PROPS", "PROTEIN_STRUCTURE_PROPERTIES");
    
    // create close button
    aws->at(10, 10);
    aws->auto_space(5, 2);
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    // create help button
    aws->callback(AW_POPUP_HELP, (AW_CL)"pfold.hlp");
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
    aws->label_length(9);
    aws->label("Filter for");
    aws->callback(ED4_pfold_select_SAI_and_update_option_menu, (AW_CL)oms_sai, 0);
    aws->create_input_field(PFOLD_AWAR_SAI_FILTER, 10);
    aws->at_newline();
    
    // create match method option menu
    PFOLD_MATCH_METHOD match_method = (PFOLD_MATCH_METHOD) ED4_ROOT->aw_root->awar(PFOLD_AWAR_MATCH_METHOD)->read_int();
    aws->label_length(12);
    aws->create_option_menu(PFOLD_AWAR_MATCH_METHOD, "Match Method", "");
    for (int i = 0; const char *mm_aw = pfold_match_method_awars[i].awar; i++) {
        aws->callback(awcbs);
        if (match_method == pfold_match_method_awars[i].match_method) {
            aws->insert_default_option(mm_aw, "", match_method);
        } else {
            aws->insert_option(mm_aw, "", pfold_match_method_awars[i].match_method);
        }
    }
    aws->update_option_menu();
    aws->at_newline();
    
    // create match symbols input field
    aws->label_length(40);
    aws->label("Match Symbols (Range 0-100% in steps of 10%)");
    aws->callback(awcbs);
    aws->create_input_field(PFOLD_AWAR_SYMBOL_TEMPLATE_2, 10);
    aws->at_newline();
    
    // create match type and symbols input fields 
    for (int i = 0; pfold_match_type_awars[i].awar; i++) {
        aws->label_length(12);
        sprintf(awar, PFOLD_AWAR_PAIR_TEMPLATE, pfold_match_type_awars[i].awar);
        aws->label(pfold_match_type_awars[i].awar);
        aws->callback(awcbs);
        aws->create_input_field(awar, 30);
        if (!i) aws->get_at_position(&ex,&ey);
        sprintf(awar, PFOLD_AWAR_SYMBOL_TEMPLATE, pfold_match_type_awars[i].awar);
        aws->callback(awcbs);
        aws->create_input_field(awar, 3);
        aws->at_newline();
    }
    
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
    
    if (selected_sai) free(selected_sai);
    if (sai_filter)   free(sai_filter);
    aww->update_option_menu();
    ED4_expose_all_windows();
}


inline int ED4_pfold_round_sym(double d) {
    return int(d + .5);
}


void ED4_pfold_find_nucleation_sites(const structure s) {
#ifdef SHOW_PROGRESS
    cout << endl << "Searching for nucleation sites";
#endif

    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string(); // gap characters
    char c; // character representing the structure (H for helix or E for beta sheet)
    int windowSize; // window size used for finding nucleation sites
    double sumOfFormVal = 0, sumOfBreakVal = 0; // sum of former resp. breaker values in window
    int pos; // current position in sequence
    int count; // number of amino acids found in window
    int aa;

    // check structure and set parameters
    switch (s) {
        case helix:
#ifdef SHOW_PROGRESS
            cout << " (helix):" << endl;
#endif
            windowSize = 6;
            c = 'H';
            break;
        case sheet:
#ifdef SHOW_PROGRESS
            cout << " (sheet):" << endl;
#endif
            windowSize = 5;
            c = 'E';
            break;
        default:
            e4_assert(0); // incorrect value for structure: possible candidates are 'helix' and 'sheet'
            return;
    }

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
        if ((sumOfFormVal > (windowSize - 2)) && (sumOfBreakVal < 2)) {
            for (int j = i; j < (pos + count); j++) {
                if (char2AA[sequence[j]] != -1) structures[s][j] = c;
            }
        }
        if (aa == -1) i = pos + count; // skip unknown character
        sumOfFormVal = 0, sumOfBreakVal = 0;
    }

#ifdef SHOW_PROGRESS
    cout << structures[s] << endl;
#endif
}


//TODO: check this function again and simplify it
void ED4_pfold_extend_nucleation_sites(const structure s) {
#ifdef SHOW_PROGRESS
    cout << endl << "Extending nucleation sites";
#endif

    // definition of local variables:
    // c specifies the character representing a particular structure
    char c;
    // specifies the breakers for the particular structure
    char breakers[7];
    // specifies the indifferent amino acids for the particular structure
    char indiff[5];
    // indicates if breaker is found at current position (index 0) resp. at neighbouring position (index 1)
    bool structBreak[2];
    // indicates if indifferen amino acid is found at neighbouring position
    bool structIndiff;
    // indicates if structure breaks (two consecutive breakers resp. breaker followed by indifferent amino acid)
    bool breakStruct;
    // indicates if structure is continued (breakStruct == false and no occurence of special amino acids)
    bool contStruct = true;
    // specify start and end of nucleation site
    int start = 0, end = 0;
    // neighbour of end
    int neighbour;
    // just for testing...
#ifdef STEP_THROUGH_EXT_NS
    char temp;
#endif

    // check which structure is specified and set variables appropriately
    switch (s) {
        case helix:
#ifdef SHOW_PROGRESS
            cout << " (helix):" << endl;
#endif
            c = 'H';
            strcpy(breakers, "NYPG");
            strcpy(indiff, "RTSC");
            break;
        case sheet:
#ifdef SHOW_PROGRESS
            cout << " (sheet):" << endl;
#endif
            c = 'E';
            strcpy(breakers, "PDESGK");
            strcpy(indiff, "RNHA");
            break;
        default:
#ifdef SHOW_PROGRESS
            cout << ":" << endl;
#endif
            e4_assert(0); // incorrect value for structure: possible candidates are 'helix' and 'sheet'
            return;
    }

    // find nucleation sites and extend them in both directions:
    // for whole structure
    for (int indStruct = 0; indStruct < length; indStruct++) {
        //  while (structures[s][indStruct] != '\0') {

        // search beginning of sequence
        while (!(strchr(amino_acids, sequence[indStruct])) && (sequence[indStruct] != '\0')){
#ifdef STEP_THROUGH_EXT_NS
            cout << "\nSearching beginning of sequence:\n";
            temp = structures[s][indStruct];
            structures[s][indStruct] = 'X';
            cout << sequence << endl << structures[s] << endl;
            structures[s][indStruct] = temp;
            waitukp();
#endif
            indStruct++;
        }

        // first search nucleated region and set start/end
        while (structures[s][indStruct] == ' ' || !(strchr(amino_acids, sequence[indStruct]))) {
#ifdef STEP_THROUGH_EXT_NS
            cout << "\nSearching nucleation site:\n";
            temp = structures[s][indStruct];
            structures[s][indStruct] = 'X';
            cout << sequence << endl << structures[s] << endl;
            structures[s][indStruct] = temp;
            waitukp();
#endif
            indStruct++;
        }
        start = indStruct;
        if (start >= length) {
#ifdef SHOW_PROGRESS
            cout << structures[s] << endl;
#endif
#ifdef STEP_THROUGH_EXT_NS
            waitukp();
#endif
            return;
        }
        while (structures[s][indStruct] == c || !(strchr(amino_acids, sequence[indStruct]))) {
            indStruct++;
        }
        end = indStruct - 1;

        // then extend nucleated region in both directions
        // left side:
#ifndef START_EXTENSION_INSIDE
        do {
            start--;
//#ifdef STEP_THROUGH_EXT_NS
//            cout << "\nFinding next amino acid::\n";
//            temp = structures[s][start];
//            structures[s][start] = 'X';
//            cout << sequence << endl << structures[s] << endl;
//            structures[s][start] = temp;
//            waitukp();
//#endif
        } while ((start > 0) && !(strchr(amino_acids, sequence[start])));
        while (contStruct && (start > 1) && (structures[s][start] != c)) {
#else
            do {
#endif
#ifdef STEP_THROUGH_EXT_NS
                cout << "\nExtending left side:\n";
                temp = structures[s][start];
                structures[s][start] = 'X';
                cout << sequence << endl << structures[s] << endl;
                structures[s][start] = temp;
                waitukp();
#endif
                structBreak[0] = strchr(breakers, sequence[start]) ? true : false;
                neighbour = start - 1;
                while (!(neighbour < 0) && !(strchr(amino_acids, sequence[neighbour]))) {
#ifdef STEP_THROUGH_EXT_NS
                    cout << "\nSearching neighbour:\n";
                    temp = structures[s][neighbour];
                    structures[s][neighbour] = 'X';
                    cout << sequence << endl << structures[s] << endl;
                    structures[s][neighbour] = temp;
                    waitukp();
#endif
                    neighbour--;
                }
                if (neighbour < 0) {
                    break;
                }
                structBreak[1] = strchr(breakers, sequence[neighbour]) ? true : false;
                structIndiff = strchr(indiff, sequence[neighbour]) ? true : false;
                breakStruct = (structBreak[0] && structBreak[1]) || (structBreak[0] && structIndiff);

                switch (s) {
                    case helix:
                        contStruct = !breakStruct && (sequence[start] != 'P');
                        break;
                    case sheet:
                        contStruct = !breakStruct && (sequence[start] != 'P') && (sequence[start] != 'E');
                        break;
                    default:
                        break;
                }
                if (contStruct) {
                    structures[s][start] = c;
                    start = neighbour;
                }
#ifdef START_EXTENSION_INSIDE
                else {
                    structures[s][start] = ' ';
                }

            } while (contStruct && (start > 0) && (structures[s][start] != c));
#else
        }
#endif

        contStruct = true;

        // right side:
#ifndef START_EXTENSION_INSIDE
        end++;
        while (contStruct && (end < (length - 2))) {
#else
            do {
#endif
#ifdef STEP_THROUGH_EXT_NS
                cout << "\nExtending right side:\n";
                temp = structures[s][end];
                structures[s][end] = 'X';
                cout << sequence << endl << structures[s] << endl;
                structures[s][end] = temp;
                waitukp();
#endif
                strchr(breakers, sequence[end]) ? structBreak[0] = true : structBreak[0] = false;
                neighbour = end + 1;
                while (!(strchr(amino_acids, sequence[neighbour]))) {
#ifdef STEP_THROUGH_EXT_NS
                    cout << "\nSearching neighbour:\n";
                    temp = structures[s][neighbour];
                    structures[s][neighbour] = 'X';
                    cout << sequence << endl << structures[s] << endl;
                    structures[s][neighbour] = temp;
                    waitukp();
#endif
                    neighbour++;
                    if (neighbour >= (length - 1)) {
#ifdef SHOW_PROGRESS
                        cout << structures[s] << endl;
#endif
#ifdef STEP_THROUGH_EXT_NS
                        waitukp();
#endif
                        return;
                    }
                }
                strchr(breakers, sequence[neighbour]) ? structBreak[1] = true : structBreak[1] = false;
                strchr(indiff, sequence[neighbour]) ? structIndiff = true : structIndiff = false;
                breakStruct = (structBreak[0] && structBreak[1]) || (structBreak[0] && structIndiff);

                switch (s) {
                    case helix:
                        contStruct = !breakStruct && (sequence[end] != 'P');
                        break;
                    case sheet:
                        contStruct = !breakStruct && (sequence[end] != 'P') && (sequence[end] != 'E');
                        break;
                    default:
                        break;
                }
                if (contStruct) {
                    structures[s][end] = c;
                    end = neighbour;
                    if (structures[s][end] == c) {
                        while (structures[s][end] == c || !(strchr(amino_acids, sequence[end]))) {
#ifdef STEP_THROUGH_EXT_NS
                            cout << "\nSearching end of nucleation site:\n";
                            temp = structures[s][end];
                            structures[s][end] = 'X';
                            cout << sequence << endl << structures[s] << endl;
                            structures[s][end] = temp;
                            waitukp();
#endif
                            end++;
                        }
#ifdef START_EXTENSION_INSIDE
                        end--;
#endif
                    }
                }
#ifdef START_EXTENSION_INSIDE
                else {
                    structures[s][end] = ' ';
                }
                indStruct = end;
            } while (contStruct && (end < (length - 1)));
#else
            indStruct = end;
        }
#endif
        contStruct = true;
    }

#ifdef SHOW_PROGRESS
    cout << structures[s] << endl;
#endif

#ifdef STEP_THROUGH_EXT_NS
    waitukp();
#endif
}


void ED4_pfold_find_turns() {
#ifdef SHOW_PROGRESS
    cout << endl << "Searching for beta-turns: " << endl;
#endif

    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string();
    const char c = 'T';
    const int windowSize = 4;
    double P_a = 0, P_b = 0, P_turn = 0;
    double p_t = 1;
    int pos;
    int count;
    int aa;

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
            
            // compute probabilities
            P_a    += cf_parameters_norm[aa][0];
            P_b    += cf_parameters_norm[aa][1];
            P_turn += cf_parameters_norm[aa][2];
            p_t    *= cf_parameters_norm[aa][3 + count];
        }
        if (count != 0) {
//        if (count == 4) {
            P_a /= count;
            P_b /= count;
            P_turn /= count;
            if ((p_t > 0.000075) && (P_turn > 100) && (P_turn > P_a) && (P_turn > P_b)) {
                for (int j = i; j < (pos + count); j++) {
                    if (char2AA[sequence[j]] != -1) structures[beta_turn][j] = c;
                }
            }
        }
        if (aa == -1) i = pos + count; // skip unknown character
        p_t = 1, P_a = 0, P_b = 0, P_turn = 0;
    }

    free(gap_chars);
#ifdef SHOW_PROGRESS
    cout << structures[beta_turn] << endl;
#endif
}


//TODO: check this functions again. something seems to be wrong!
void ED4_pfold_resolve_overlaps() {
#ifdef SHOW_PROGRESS
    cout << endl << "Resolving overlaps: " << endl;
#endif

    // index of first amino acid included in overlap, end = index of last amino acid included in overlap
    int start = -1, end = -1;
    // P_a and P_b specify probabilities of alpha-helix resp. beta-sheet in overlapping regions
    double P_a = 0, P_b = 0;
    // character representing the structure
    char c = '?';
    //
    structure s;
    // probability of the predicted structure
    double prob;
    // character representing the probability
    char p = '?';
    // gap characters
    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string();

    // scan structures for overlaps
    for (int pos = 0; pos < length; pos++) {

        // if beta-turn is found at position pos
        if (structures[beta_turn][pos] == 'T') {
            // summary at position pos is beta-turn
            //TODO: compare probabilities for turn, helix and sheet instead of just taking the turn? 
            structures[summary][pos] = 'T';

        // if helix and sheet are overlapping and no beta-turn is found at position pos
        } else if ((structures[helix][pos] == 'H') && (structures[sheet][pos] == 'E')) {

            // search start and end of overlap (as long as no beta-turn is found)
            start = pos;
            end = pos;
            while ((structures[helix][end] == 'H') && (structures[sheet][end] == 'E') && (structures[beta_turn][end] != 'T')) {
                end++;
            }
            end--;

            // calculate P_a and P_b for overlap
            for (int i = start; i <= end; i++) {
                // skip gaps
                while (i < end && strchr(gap_chars, sequence[i])) {
                    i++;
                }
                int aa = char2AA[sequence[i]];
                if (aa != -1) {
                    P_a += cf_parameters[aa][helix];
                    P_b += cf_parameters[aa][sheet];
                }
            }

            // check which structure is more likely and define variables appropriately:
            if (P_a > P_b) {
                c = 'H';
                s = helix;
            } else if (P_b > P_a) {
                c = 'E';
                s = sheet;
            } else {
                c = '?';
            }

//            //TODO: compute probability of structure
//            prob = (P_a - P_b) / (end - start + 1);
//            if (prob > 0) {
//                prob /= 1.42;
//            } else
//                if (prob < 0) {
//                    prob /= (-1.40);
//                } else {
//                    prob = 0;
//                }
//
//            // round probability and convert result to character
//            prob = round_sym(prob * 10);
//            p = (prob == 10) ? 'X' : char((int)prob + 48);
//
//            // set structure and probability for region containing the overlap
//            for (int i = start; i <= end; i++) {
//                structures[summary][i] = c;
//                probabilities[0][i] = p;
//                probabilities[1][i] = (round_sym((parameters[s][i] / maxProb[s]) * 10) == 10) ? 'X' : (char(round_sym((parameters[s][i] / maxProb[s]) * 10) + 48));
//            }

            // set variables for next pass of loop resp. end of loop
            P_a = 0, P_b = 0;
            pos = end;

        // if helix and sheet are not overlapping and no beta-turn is found at position pos
        } else {
            // summary at position pos is helix resp. sheet
            if (structures[helix][pos] == 'H') {
                structures[summary][pos] = 'H';
            } else if (structures[sheet][pos] == 'E'){
                structures[summary][pos] = 'E';
            }
        }

    }
    
    free(gap_chars);
#ifdef SHOW_PROGRESS
    cout << structures[summary] << endl;
#endif
}
