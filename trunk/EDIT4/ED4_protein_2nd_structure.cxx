#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include <iostream>
#include <fstream>
#include <iomanip>

#include <malloc.h>

#include "arbdb.h"
#include "ed4_protein_2nd_structure.hxx"
#include "ed4_class.hxx"
#include "ed4_awars.hxx"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define e4_assert(bed) arb_assert(bed)

using namespace std;

// GB_export_error


GB_ERROR ED4_predict_summary(const char *seq, char *result_buffer, int length_) {
    
    GB_ERROR error = 0;

    // assign sequence
    sequence = seq;

    // assign length of sequence
    length = length_;
    e4_assert(int(strlen(seq))==length);

    // specify the characters used for amino acid one letter code
    for (int i = 0; i < 255; i++) {
        isAA[i] = false;
    }
    for (int i = 0; amino_acids[i]; ++i) {
        isAA[(unsigned char)amino_acids[i]] = true;
    }

    // allocate memory for structures[] and initialize them
    for (int i = 0; i < 4; i++) {
        structures[i] = new char [length + 1];
        if (structures[i]) {
            for (int j = 0; j < length; j++) {
                // if (!(strchr(amino_acids, sequence[j]))) {
                if (!isAA[(unsigned char)sequence[j]]) {
                    structures[i][j] = sequence[j];
                } else {
                    structures[i][j] = ' ';
                }
            }
            structures[i][length] = '\0';
        }
        else {
            error = "Out of memory";
            return error;
        }
    }

    // allocate memory for parameters[]
    for (int i = 0; i < 7; i++) {
        parameters[i] = new double [length];
        if (!parameters[i]) {
            error = "Out of memory";
            return error;
        } /*else {
            for (int j = 0; j < length; j++) {
            parameters[i][j] = 0.0;
            }
            } */
    }

    // allocate memory for probabilities[]
    for (int i = 0; i < 2; i++) {
        probabilities[i] = new char [length + 1];
        if (!probabilities[i]) {
            error = "Out of memory";
            return error;
        } else {
            for (int j = 0; j < length; j++) {
                probabilities[i][j] = ' ';
            }
            probabilities[i][length] = '\0';
        }
    }

    // predict structure
    //setDir("../lib/protein_2nd_structure/");
    getParameters("lib/protein_2nd_structure/CFParamNorm.dat");
    findNucSites(helix);
    findNucSites(sheet);
    extNucSites(helix);
    extNucSites(sheet);
    getParameters("lib/protein_2nd_structure/CFParam.dat");
    findTurns();
    resOverlaps();

    // write predicted summary to result_buffer
    if (result_buffer) {
        e4_assert(int(strlen(result_buffer)) >= length);
        for (int i = 0; i < length; i++) {
            result_buffer[i] = structures[summary][i];
        }
        result_buffer[length] = '\0';
    }

    return error;
}


//TODO: Ralf zeigen
GB_ERROR ED4_calculate_secstruct_sequence_match(const char *secstruct, const char *seq, int start, int end, char *result_buffer, int strictness /*= 1*/) {
    
    e4_assert(start >= 0);
    e4_assert(secstruct);
    e4_assert(seq);
    e4_assert(result_buffer);
    e4_assert(strictness >= 0 && strictness <= 1); //TODO: wird parameter strictness noch benötigt?
    int match_end = min( min(end, (int)strlen(secstruct)), (int)strlen(seq) );
    
    char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string();
    char *pairs[MATCH_COUNT];
    char pair_chars[MATCH_COUNT];

    pair_chars[STRUCT_NONE] = ' ';
    pair_chars[STRUCT_GOOD_MATCH] = ' ';
    pair_chars[STRUCT_MEDIUM_MATCH] = '-';
    pair_chars[STRUCT_BAD_MATCH] = '~';
    pair_chars[STRUCT_NO_MATCH] = '#';
    
    pairs[STRUCT_NONE]=strdup("  ");
    pairs[STRUCT_GOOD_MATCH]=strdup("HH HG HI HS EE EB ES TT TS");
    pairs[STRUCT_MEDIUM_MATCH]=strdup("HT GT IT ET");
    pairs[STRUCT_BAD_MATCH]=strdup("ET BT");
    pairs[STRUCT_NO_MATCH]=strdup("HB EH EG EI");
    
    ED4_predict_summary(seq, 0, (int)strlen(seq));
    
    int i;
    for (i = start; i < match_end; i++) {
        result_buffer[i - start] = '?';
        if ( strchr(gap_chars, secstruct[i]) || strchr(gap_chars, seq[i]) || 
             (structures[helix][i] == ' ' && structures[sheet][i] == ' ' && structures[beta_turn][i] == ' ') ) {
                result_buffer[i - start] = pair_chars[STRUCT_NONE];
        } else {
            // search for good match first
            // if found: stop searching
            // otherwise: continue searching for a less good match
            for (int n_pt = 0; n_pt < MATCH_COUNT; n_pt++) {
                int len = strlen(pairs[n_pt])-1;
                char *p = pairs[n_pt];
                for (int n_struct = 0; n_struct < 3; n_struct++) {
                    for (int j = 0; j < len; j += 3) {
                        if ( (p[j] == structures[n_struct][i] && p[j+1] == secstruct[i]) ||
                                (p[j] == secstruct[i] && p[j+1] == structures[n_struct][i]) ) {
                            result_buffer[i - start] = pair_chars[n_pt];
                            n_struct = 3; // stop searching the structures
                            n_pt = MATCH_COUNT; // stop searching the pair types
                            break; // stop searching the pairs array
                        }
                    }
                }
            }
        }
    }

    // fill the remaining buffer with spaces
    while (i < end) {
        i++;
        result_buffer[i - start] = ' ';
    }
    
    return 0;
}
//TODO: remove old functions if not needed anymore
//GB_ERROR ED4_calculate_secstruct_sequence_match(const char *secstruct, const char *seq, int start, int end, char *result_buffer, int strictness /*= 1*/) {
//    
//    e4_assert(start >= 0);
//    e4_assert(secstruct);
//    e4_assert(seq);
//    e4_assert(result_buffer);
//    int match_end = min( min(end, (int)strlen(secstruct)), (int)strlen(seq) );
//    
//    //char *gap_chars = ED4_ROOT->aw_root->awar(ED4_AWAR_GAP_CHARS)->read_string();
//    char *pairs[MATCH_COUNT];
//    char pair_chars[MATCH_COUNT];
//    char match[4] = {' '};
//
//    pair_chars[STRUCT_NONE] = ' ';
//    pair_chars[STRUCT_GOOD_MATCH] = ' ';
//    pair_chars[STRUCT_MEDIUM_MATCH] = '-';
//    pair_chars[STRUCT_BAD_MATCH] = '~';
//    pair_chars[STRUCT_NO_MATCH] = '#';
//    
//    if (strictness == 0) {
//        pairs[STRUCT_NONE]=strdup("  ");
//        pairs[STRUCT_GOOD_MATCH]=strdup("HH HG HI EE EB H- E- T-");
//        pairs[STRUCT_MEDIUM_MATCH]=strdup("HS HT ET ES");
//        pairs[STRUCT_BAD_MATCH]=strdup("HB");
//        pairs[STRUCT_NO_MATCH]=strdup("EH EG EI");
//    } else if (strictness == 1) {
//        pairs[STRUCT_NONE]=strdup("  ");
//        pairs[STRUCT_GOOD_MATCH]=strdup("HH HG HI EE EB");
//        pairs[STRUCT_MEDIUM_MATCH]=strdup("HS ES H- E- T-");
//        pairs[STRUCT_BAD_MATCH]=strdup("HB HT ET");
//        pairs[STRUCT_NO_MATCH]=strdup("EH EG EI");
//    } else if (strictness == 2) {
//        pairs[STRUCT_NONE]=strdup("    ");
//        pairs[STRUCT_GOOD_MATCH]=strdup("   -|H HH| EEE|HEHH|HEEE");
//        pairs[STRUCT_MEDIUM_MATCH]=strdup("H HG|H HI|H HT| EES|HEHB|HEHG|HEHI|HEHT|HEEB|HEES");
//        pairs[STRUCT_BAD_MATCH]=strdup("   H|   B|   E|   G|   I|   T|   S|H HB|H H-| EEB| EE-|HEHE|HEHS|HEH-|HEEH|HEEG|HEEI|HEET|HEE-");
//        pairs[STRUCT_NO_MATCH]=strdup("H HE|H HS| EEH| EEG| EEI| EET");
//    } else {
//        const char *error = "Unallowed value for parameter strictness";
//        return error;
//    }
//    
//    ED4_predict_summary(seq, 0, (int)strlen(seq));
//    
//    int i = start;
//    if (strictness == 2) {
//        for (i = start; i < match_end; i++) {
//            result_buffer[i - start] = ' ';
//            match[0] = structures[helix][i];
//            match[1] = structures[sheet][i];
//            match[2] = structures[summary][i];
//            match[3] = secstruct[i];
//            for (int n_pt = 0; n_pt < MATCH_COUNT; n_pt++) {
//                if (strstr(pairs[n_pt], match)) {
//                    result_buffer[i - start] = pair_chars[n_pt];
//                    break;
//                }
//            }
//        }
//    } else {
//        for (i = start; i < match_end; i++) {
//            result_buffer[i - start] = ' ';
//            for (int n_pt = 0; n_pt < MATCH_COUNT; n_pt++) {
//                int len = strlen(pairs[n_pt])-1;
//                char *p = pairs[n_pt];
//                for (int n_struct = 0; n_struct < 3; n_struct++) {
//                    for (int j = 0; j < len; j += 3) {
//                        if ( (p[j] == structures[n_struct][i] && p[j+1] == secstruct[i]) ||
//                             (p[j] == secstruct[i] && p[j+1] == structures[n_struct][i]) ) {
//                            result_buffer[i - start] = pair_chars[n_pt];
//                            n_struct = 3; // stop searching the structures
//                            n_pt = MATCH_COUNT; // stop searching the pair types
//                            break; // stop searching the pairs array
//                        }
//                    }
//                }
//            }
//        }
//    }
//    
//    while (i < end) {
//        i++;
//        result_buffer[i - start] = ' ';
//    }
//    
//    return 0;
//}


//TODO: Ralf zeigen
GB_ERROR ED4_calculate_secstruct_match(const char *secstruct1, const char *secstruct2, int start, int end, char *result_buffer) {
    
    e4_assert(start >= 0);
    e4_assert(secstruct1);
    e4_assert(secstruct2);
    e4_assert(result_buffer);
    int end_match = min( min(end, (int)strlen(secstruct1)), (int)strlen(secstruct2) );
    
    char *pairs[MATCH_COUNT];
    char pair_chars[MATCH_COUNT];
   
    pairs[STRUCT_NONE]=strdup("  ");
    pair_chars[STRUCT_NONE] = ' ';

    pairs[STRUCT_GOOD_MATCH]=strdup("HH BB EE GG II TT SS --");
    pair_chars[STRUCT_GOOD_MATCH] = ' ';
    
    pairs[STRUCT_MEDIUM_MATCH]=strdup("HG HI HT HS ES GI GT GS IT IS TS");
    pair_chars[STRUCT_MEDIUM_MATCH] = '-';

    pairs[STRUCT_BAD_MATCH]=strdup("BE BT BS ET H- B- E- G- I- T- S-");
    pair_chars[STRUCT_BAD_MATCH] = '~';

    pairs[STRUCT_NO_MATCH]=strdup("HB HE BG BI EG EI");
    pair_chars[STRUCT_NO_MATCH] = '#';

    int i;
    for (i = start; i < end_match; i++) {
        result_buffer[i - start] = ' ';
        for (int n_pt = 0; n_pt < MATCH_COUNT; n_pt++) {
            int len = strlen(pairs[n_pt])-1;
            char *p = pairs[n_pt];
            for (int j = 0; j < len; j += 3) {
                if ( (p[j] == secstruct1[i] && p[j+1] == secstruct2[i]) ||
                     (p[j] == secstruct2[i] && p[j+1] == secstruct1[i]) ) {
                    result_buffer[i - start] = pair_chars[n_pt];
                    n_pt = MATCH_COUNT; // stop searching the pair types 
                    break; // stop searching the pairs array
                }
            }
        }
    }
    
    while (i < end) {
        i++;
        result_buffer[i - start] = ' ';
    }
    
    return 0;
}


void printSeq() {
    cout << "\nSequence:\n" << sequence << endl;
}


inline int round_sym(double d) {
    return int(d + .5);
}


void getParameters(const char file[]) {

#ifdef SHOW_PROGRESS
    cout << endl << "Getting Parameters: " << endl;
#endif

    // read file and crop string
    char *table = new char [filelen(file) + 1];
    if (table) {
        readFile(table, file);
        //      cout << endl << table << endl;
        cropln(table, 3);
        //      cout << endl << table << endl;
    } else {
        //TODO: remove all exits and establish proper error handling
        cerr << "Out of memory";
        exit(1);
    }

    // scan sequence and get parameters
    char *p;
    char value[7];
    int indVal = 0;
    int indParam = 0;
    for (int i = 0; i < (length); i++) {
        while (!(p = strchr(table, *(sequence + i))) && sequence[i] != '\0') {
            i++;
        }
        while ((*p != '\n') && (*p != '\0')) {
            do {
                p++;
            } while ((*p == ' ') || (*p == '\t'));
            while ((*p != ' ') && (*p != '\t') && (*p != '\n') && (*p != '\0')) {
                value[indVal] = *p;
                p++;
                indVal++;
            }
            value[indVal] = '\0';
            parameters[indParam][i] = atof(value);
            indParam++;
            indVal = 0;
        }
        indParam = 0;
    }

    //  cout << endl << table;
    //  waitukp();

    delete [] table;

    //  for (int i = 0; i < 7; i++) {
    //      for (int j = 0; j < length; j++) {
    //          cout << parameters[i][j] << " ";
    //      }
    //      cout << endl;
    //  }

}


void findNucSites(const structure s) {

#ifdef SHOW_PROGRESS
    cout << endl << "Searching for nucleation sites";
#endif

    // definition of local variables:
    // c specifies the character representing a particular structure
    char c;
    // spcifies window size used for finding nucleation sites
    int windowSize;
    // sum of former resp. breaker values in window
    double sumOfFormVal = 0, sumOfBreakVal = 0;
    // temporary variable for saving position in sequence
    int savePos;
    // just for testing..
#ifdef STEP_THROUGH_FIND_NS
    char temp;
#endif

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
#ifdef SHOW_PROGRESS
            cout << ":" << endl;
#endif
            cerr << "Program aborted.\nIncorrect call of function \"findNucleationSites(const structure s)\".\nTried to call function with invalid structure.\nPossible candidates are 'helix' and 'sheet'." << endl;
            exit(1);
            break;
    }

    for (int i = 0; i < ((length + 1) - windowSize); i++) {
        while ((sequence[i] == '.') || (sequence[i] == '-') && (sequence[i] != '\0')){
            i++;
#ifdef STEP_THROUGH_FIND_NS
            temp = sequence[i];
            sequence[i] = 'X';
            cout << endl << sequence << endl << structures[s] << endl;
            sequence[i] = temp;
            waitukp();
#endif
        }
        savePos = i;
        for (int j = 0; j < windowSize; j++) {
#ifdef STEP_THROUGH_FIND_NS
            temp = sequence[i];
            sequence[i] = 'X';
            cout << endl << sequence << endl << structures[s] << endl;
            sequence[i] = temp;
            waitukp();
#endif
            while ((sequence[i] == '-') && (sequence[i] != '\0')){
                i++;
#ifdef STEP_THROUGH_FIND_NS
                temp = sequence[i];
                sequence[i] = 'X';
                cout << endl << sequence << endl << structures[s] << endl;
                sequence[i] = temp;
                waitukp();
#endif
            }
            if (sequence[i] == '.') {
#ifdef SHOW_PROGRESS
                cout << structures[s] << endl;
#endif
                return;
            }
            //          cout << setw(5) << parameters[s][j];
            //          cout << setw(5) << parameters[s + 2][j];
            sumOfFormVal += parameters[s][i];
            sumOfBreakVal += parameters[s + 2][i];
            i++;
        }
        i = savePos;
        if ((sumOfFormVal > (windowSize - 2)) && (sumOfBreakVal < 2)) {
            for (int j = 0; j < windowSize; j++) {
                while ((sequence[i] == '-') && (sequence[i] != '\0')){
                    i++;
                }
                structures[s][i] = c;
                i++;
            }
            i = savePos;
        }
        //      cout << "    " << sumOfFormVal << "    " << sumOfBreakVal;
        //      cout << endl;
        sumOfFormVal = 0;
        sumOfBreakVal = 0;
    }

#ifdef SHOW_PROGRESS
    cout << structures[s] << endl;
#endif

}


void extNucSites(const structure s) {

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
            cerr << "Program aborted.\nIncorrect call of function \"extNucSites(const structure s)\".\nTried to call function with invalid structure.\nPossible candidates are 'helix' and 'sheet'." << endl;
            exit(1);
            break;
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
            //          cout << start << ", " << length << endl;
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
        //      cout << start << ", " << end << " | ";

        // then extend nucleated region in both directions
        // left side:
#ifndef START_EXTENSION_INSIDE
        do {
            start--;
            //#ifdef STEP_THROUGH_EXT_NS
            //          cout << "\nFinding next amino acid::\n";
            //          temp = structures[s][start];
            //          structures[s][start] = 'X';
            //          cout << sequence << endl << structures[s] << endl;
            //          structures[s][start] = temp;
            //          waitukp();
            //#endif
        } while (!(strchr(amino_acids, sequence[start])) && (start > 0));
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
                while (!(strchr(amino_acids, sequence[neighbour])) && !(neighbour < 0)) {
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
                //          cout << structBreak[0] << structBreak[1] << structIndiff;
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
                    //          cout << " " << contStruct << " - ";
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
                //          cout << structBreak[0] << structBreak[1] << structIndiff;
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
        //          cout << " " << contStruct << " - ";

        contStruct = true;
        //      cout << endl;
    }

#ifdef SHOW_PROGRESS
    cout << structures[s] << endl;
#endif
    //  cout << start << ", " << length << endl;

#ifdef STEP_THROUGH_EXT_NS
    waitukp();
#endif

}


void findTurns() {

#ifdef SHOW_PROGRESS
    cout << endl << "Searching for beta-turns: " << endl;
#endif

    const char c = 'T';
    const int windowSize = 4;
    double P_a = 0, P_b = 0, P_turn = 0;
    double p_t = 1;

    for (int indParam = 0; indParam < ((length + 1) - windowSize); indParam++) {
        for (int i = 0; i < windowSize; i++) {
            P_a += parameters[0][indParam + i];
            P_b += parameters[1][indParam + i];
            P_turn += parameters[2][indParam + i];
            p_t *= parameters[3 + i][indParam + i];
        }
        P_a /= windowSize;
        P_b /= windowSize;
        P_turn /= windowSize;
        if ((p_t > 0.000075) && (P_turn > 100) && (P_turn > P_a) && (P_turn > P_b)) {
            for (int i = indParam; i < (indParam + windowSize); i++) {
                structures[beta_turn][i] = c;
            }
        }
        p_t = 1, P_a = 0, P_b = 0, P_turn = 0;
    }

#ifdef SHOW_PROGRESS
    cout << structures[beta_turn] << endl;
#endif

}


void resOverlaps() {

#ifdef SHOW_PROGRESS
    cout << endl << "Resolving overlaps: " << endl;
#endif

    // definition of lokal variables:
    // start = index of first amino acid included in overlap, end = index of last amino acid included in overlap
    int start = -1, end = -1;
    // P_a and P_b specify probabilities of alpha-helix resp. beta-sheet in overlapping regions
    double P_a = 0, P_b = 0;
    // c specifies the character representing a particular structure
    char c = '?';
    //
    structure s;
    // prob is the probability of the predicted structure
    double prob;
    // p specifies the character representing a probability
    char p = '?';
    //
    double maxProb;

    // scan structures for overlaps
    for (int pos = 0; pos < length; pos++) {

//        cout << sequence << endl << structures[helix] << endl << structures[sheet] << endl << structures[beta_turn] << endl << structures[summary];
//        waitukp();

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
//            cout << start << " " << end << endl;

            // calculate P_a and P_b for overlap:
            for (int i = start; i <= end; i++) {
                P_a += parameters[helix][i];
                P_b += parameters[sheet][i];
            }

//            cout << endl << P_a << "   " << P_b << endl;

            // check which structure is more likely and define variables appropriately:
            if (P_a > P_b) {
                c = 'H';
                s = helix;
                maxProb = 1.42;
            } else if (P_b > P_a) {
                c = 'E';
                s = sheet;
                maxProb = 1.62;
            } else {
                c = '?';
            }

            // compute probability of structure
            prob = (P_a - P_b) / (end - start + 1);
            if (prob > 0) {
                prob /= 1.42;
            } else
                if (prob < 0) {
                    prob /= (-1.40);
                } else {
                    prob = 0;
                }

            // round probability and convert result to character
            prob = round_sym(prob * 10);
            p = (prob == 10) ? 'X' : char((int)prob + 48);

            // set structure and probability for region containing the overlap
            for (int i = start; i <= end; i++) {
                structures[summary][i] = c;
                probabilities[0][i] = p;
                probabilities[1][i] = (round_sym((parameters[s][i] / maxProb) * 10) == 10) ? 'X' : (char(round_sym((parameters[s][i] / maxProb) * 10) + 48));

            }

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

#ifdef SHOW_PROGRESS
    cout << structures[summary] << endl;
#endif

}


void setDir(const char *dir) {

#ifdef SHOW_PROGRESS
    cout << "Setting working directory... ";
#endif

    if (directory) {
        delete [] directory;
    }
    directory = new char [strlen(dir) + 1];
    if (directory)
        strcpy(directory, dir);
    else {
#ifdef SHOW_PROGRESS
        cout << "aborted." << endl;
#endif
        cerr << "Allocation of memory failed." << endl;
        exit(1);
    }

#ifdef SHOW_PROGRESS
    cout << "done.\nCurrent working directory: \"" << directory << "\"." << endl;
#endif

}


int filelen(const char file[]) {

#ifdef SHOW_PROGRESS
    cout << "Getting filelength... ";
#endif

    // concatenate file with working directory
    if (!directory) {
        directory = new char [1];
        directory[0] = '\0';
    }
    char dir[(strlen(directory) + strlen(file))];
    dir[0] = '\0';
    strcat(dir, directory);
    strcat(dir, file);

    // open file
    ifstream ifl(dir, ios::in);

    // if creating ifstream fails, print error message and exit
    if (!ifl) {
#ifdef SHOW_PROGRESS
        cout << "failed." << endl;
#endif
        cerr << "Couldn't open file \"" << dir << "\". (in)" << endl;
        exit(1);
    }

    //  get filelength
    int filelength = 0;
    while (ifl.get() != EOF) {
        filelength++;
    }
    ifl.close();

#ifdef SHOW_PROGRESS
    cout << "done.\nFilelength = " << filelength << "." << endl;
#endif

    return (filelength);

}


void readFile(char *content, const char file[]) {

    // concatenate file with working directory
    if (!directory) {
        *directory = '\0';
    }
    char dir[(strlen(directory) + strlen(file))];
    dir[0] = '\0';
    strcat(dir, directory);
    strcat(dir, file);

#ifdef SHOW_PROGRESS
    cout << "Reading file \"" << dir << "\"... ";
#endif

    // open file
    //  ifstream ifl(dir, ios::in|ios::nocreate); nocreate nicht erkannt???
    ifstream ifl(dir, ios::in);

    // if creating ifstream fails, print error message and exit
    if (!ifl) {
#ifdef SHOW_PROGRESS
        cout << "aborted." << endl;
#endif
        cerr << "\nOpening file \"" << dir << "\" failed. (in)" << endl;
        exit(1);
    }

    //  put content into string
    char c;
    while (ifl.get(c)) {
        *content = c;
        content++;
    }
    ifl.close();
    *content = '\0';

#ifdef SHOW_PROGRESS
    cout << "done." << endl;
#endif

    return;

}


void writeFile(const char file[], const char *content) {

    // definition of variables
    char c;
    int indCont = 0;
    int inputCounter = 0;

    // concatenate file with working directory
    if (!directory) {
        *directory = '\0';
    }
    char dir[(strlen(directory) + strlen(file))];
    dir[0] = '\0';
    strcat(dir, directory);
    strcat(dir, file);

#ifdef SHOW_PROGRESS
    cout << "\nWriting to file \"" << dir << "\"... ";
#endif

#ifdef NOCREATE_DISABLED
    ofstream ofl(dir, ios::out);
    ofl.close();
#else
    ofstream ofl(dir, ios::out|ios::nocreate);
    ofl.close();
#endif

    if (!ofl) {
        cout << "aborted.\nCouldn't find file \"" << dir << "\".\nCreate file? ('y' = yes, 'n' = no)\n";
        cin.get(c);
        while (cin.get() != '\n') {
            inputCounter++;
        }
        if (inputCounter > 0) {
            c = 'f';
        }
        inputCounter = 0;
        while ((c != 'n') || (c != 'y')) {
            if (c == 'n') {
                cout << "\nFile was not created.\n";
                return;
            } else if (c =='y') {

#ifdef SHOW_PROGRESS
                cout << "\nCreating file... ";
#endif
                // write content to file and close stream
                ofstream ofl2(dir, ios::out);
                while ((c = content[indCont]) != '\0') {
                    ofl2 << c;
                    indCont++;
                }
                ofl2.close();

#ifdef SHOW_PROGRESS
                cout << "done." << endl;
#endif
                return;

            } else {
                cout << "\nWrong input. Please type 'n' or 'y'.\n";
                cin.get(c);
                while (cin.get() != '\n') {
                    inputCounter++;
                }
                if (inputCounter > 0) {
                    c = 'f';
                }
                inputCounter = 0;
            }
        }

#ifndef NOREPLACE_DISABLED
    } else {
        cout << "aborted.\nFile \"" << dir << "\" already exists.\nOverwrite file? ('y' = yes, 'n' = no)\n";
        cin.get(c);
        while (cin.get() != '\n') {
            inputCounter++;
        }
        if (inputCounter > 0) {
            c = 'f';
        }
        inputCounter = 0;
        while ((c != 'n') || (c != 'y')) {
            if (c == 'n') {
                cout << "\nFile was not overwritten.\n";
                return;
            } else if (c =='y') {

#ifdef SHOW_PROGRESS
                cout << "\nWriting to file... ";
#endif
                // write content to file and close stream
                ofstream ofl2(dir, ios::out);
                while ((c = content[indCont]) != '\0') {
                    ofl2 << c;
                    indCont++;
                }
                ofl2.close();

#ifdef SHOW_PROGRESS
                cout << "done." << endl;
#endif
                return;

            } else {
                cout << "\nWrong input. Please type 'n' or 'y'.\n";
                cin.get(c);
                while (cin.get() != '\n') {
                    inputCounter++;
                }
                if (inputCounter > 0) {
                    c = 'f';
                }
                inputCounter = 0;
            }
        }
    }

#else
    } else {
        // write content to file and close stream
        ofstream ofl2(dir, ios::out);
        while ((c = content[indCont]) != '\0') {
            ofl2 << c;
            indCont++;
        }
        ofl2.close();
#ifdef SHOW_PROGRESS
    cout << "done." << endl;
#endif
    }
#endif // NOREPLACE_DISABLED
}


void waitukp() {

#ifdef SHOW_PROGRESS
    cout << endl << "Program stopped. Please press [ENTER] to continue. ";
#endif
    getchar();
    cout << endl;
}


int nol(const char *string) {

    //  get number of lines
    int nol = 0;
    while (*string != '\0') {
        string++;
        while ((*string != '\n') && (*string != '\0')) {
            string++;
        }
        nol++;
    }
//    while (*string != '\0') {
//        while ((*string != '\n') && (*string != '\0')) {
//            cout << *string;
//            string++;
//        }
//        string++;
//        nol++;
//    }

    return nol;
}


void cropln(char *string, int numberOfLines /*= 1*/) {

    // crop lines
    int len = strlen(string) + 1;
    char *adr = string;
    char c = *adr;
    for (int i = 0; i < numberOfLines; i++) {
        while (c != '\n') {
            adr++;
            c = *adr;
        }
        adr++;
        c = *adr;
    }
    int l = len - (adr - string);
    for (int i = 0; i < l; i++) {
        *(string + i) = *adr;
        adr++;
    }
    for (int i = l; i < len; i++) {
        *(string + i) = '\0';
    }
}


char* scat(int num, char *str1, ...) {

    va_list ap;
    va_start(ap, str1);

    for (int i = 0; i < num; i++) {
        strcat(str1, (va_arg(ap, char*)));
    }

    va_end(ap);
    return (str1);
}


char* scat(char *str1, ...) {

    va_list ap;
    va_start(ap, str1);
    char *p;

    while ((p = va_arg(ap, char*))) {
        strcat(str1, p);
//        cout << "app ";
    }

//    cout << endl;
    va_end(ap);
    return (str1);
}
