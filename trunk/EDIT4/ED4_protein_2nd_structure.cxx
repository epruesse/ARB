#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include <iostream>
#include <fstream>
#include <iomanip>

#include "arbdb.h"
#include "ed4_protein_2nd_structure.hxx"


#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define e4_assert(bed) arb_assert(bed)

using namespace std;

/*#define SHOW_PROGRESS
  #define STEP_THROUGH_FIND_NS
  #define STEP_THROUGH_EXT_NS*/
#define NOCREATE_DISABLED
#define NOREPLACE_DISABLED
#define START_EXTENSION_INSIDE

static char *directory;                     // working directory
static const char *sequence;                        // pointer to sequence of protein
static int length;                                // length of sequence
static char *structures[4];                     // predicted structures
static char *probabilities[2];                      // probabilities of prediction
static double *parameters[7];                       // parameters for structure prediction

static char amino_acids[] = "ARNDCQEGHILKMFPSTWYV";  // specifies the characters used for amino acid one letter code
static bool isAA[256];                              // specifies the characters used for amino acid one letter code
enum structure {three_turn = 4, four_turn = 5, five_turn = 6, helix = 0, sheet = 1, beta_turn = 2, summary = 3};

// functions for structure prediction:
static void printSeq();                         // prints sequence to screen
static void getParameters(const char file[]);       // gets parameters for structure prediction specified in table
static void findNucSites(const structure s);        // finds nucleation sites that initiate the specified structure (alpha-helix or beta-sheet)
static void extNucSites(const structure s);     // extends the found nucleation sites in both directions
static void findTurns();                            // finds beta-turns
static void resOverlaps();                          // resolves overlaps of predicted structures and creates summary
//static void getDSSPVal(const char file[]);            // gets sequence and structure from dssp file

// functions for operating with files:
static void setDir(const char *dir);                                    // sets working directory
static int filelen(const char *file);                                   // gets filelength of file
static void readFile(char *content, const char *file);                  // reads file and puts content into string
static void writeFile(const char file[], const char *content);          // writes string to file
static void waitukp();                                                  // waits until any key is pressed

// functions for string manipulation:
static int nol(const char *string);                                     // gets number of lines in string
static void cropln(char *string, const int numberOfLines = 1);          // crops line/lines at the beginning of string
static char* scat(const int num, char *str1, ...);                      // concatenates str1 with arbitrary number of strings (num specifies number of strings to append)
static char* scat(char *str1, ...);                                     // concatenates str1 with arbitrary number of strings (list must be terminated by NULL)


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

    const unsigned char *AA_true = (unsigned char *)"ARNDCQEGHILKMFPSTWYV";
    for (int i = 0; AA_true[i]; ++i) {
        isAA[AA_true[i]] = true;
    }

    // isAA['A'] = true, isAA['R'] = true, isAA['N'] = true, isAA['D'] = true, isAA['C'] = true;
    // isAA['Q'] = true, isAA['E'] = true, isAA['G'] = true, isAA['H'] = true, isAA['I'] = true;
    // isAA['L'] = true, isAA['K'] = true, isAA['M'] = true, isAA['F'] = true, isAA['P'] = true;
    // isAA['S'] = true, isAA['T'] = true, isAA['W'] = true, isAA['Y'] = true, isAA['V'] = true;

    // allocate memory for structures[] and initialize them
    for (int i = 0; i < 4; i++) {
        structures[i] = new char [length + 1];
        if (structures[i]) {
            for (int j = 0; j < length; j++) {
                //          if (!(strchr(amino_acids, sequence[j]))) {
                if (!isAA[(unsigned char)sequence[j]]) {
                    structures[i][j] = sequence[j];
                } else {
                    structures[i][j] = ' ';
                }
            }
            structures[i][length] = '\0';
        }
        else {
//             cerr << "Allocation of memory failed.";
            error = "Out of memory";
//             exit(1);
        }
    }

    if (!error) {

        // allocate memory for parameters[]
        for (int i = 0; i < 7; i++) {
            parameters[i] = new double [length];
            if (!parameters[i]) {
                cerr << "Allocation of memory failed.";
                exit(1);
            } /*else {
                for (int j = 0; j < length; j++) {
                parameters[i][j] = 0.0;
                }
                } */
        }
    }

    // allocate memory for probabilities[]
    for (int i = 0; i < 2; i++) {
        probabilities[i] = new char [length + 1];
        if (!probabilities[i]) {
            cerr << "Allocation of memory failed.";
            exit(1);
        } else {
            for (int j = 0; j < length; j++) {
                probabilities[i][j] = ' ';
            }
            probabilities[i][length] = '\0';
        }
    }

    // predict structure
    setDir("../Markus/protein/");
    getParameters("dat/CFParamNorm.dat");
    findNucSites(helix);
    findNucSites(sheet);
    extNucSites(helix);
    extNucSites(sheet);
    //  getParameters("dat/CFParam.dat");
    //  findTurns();
    resOverlaps();

    // write predicted summary to result_buffer
    for (int i = 0; i < length; i++) {
        result_buffer[i] = structures[summary][i];
    }
    result_buffer[length] = '\0';

    return error;
}


GB_ERROR ED4_calculate_summary_match(const char *summary, const char *global_summary, int start, int end, char *result_buffer) {
    for (int i = start; i < end; i++) {
        if (summary[i] == global_summary[i]) {
            result_buffer[i] = ' ';
        } else {
            result_buffer[i] = '~';
        }
    }
    return 0;
}


static void printSeq() {

    cout << "\nSequence:\n" << sequence << endl;

}


static void getParameters(const char file[]) {

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
        cerr << "Allocation of memory failed.";
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


static void findNucSites(const structure s) {

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


static void extNucSites(const structure s) {

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


static void findTurns() {

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

inline double round(double d) {
    return int(d+.5);
}


static void resOverlaps() {

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

    // scan structures for overlaps:
    // for lenght of sequence
    for (int pos = 0; pos < length; pos++) {

        //      cout << sequence << endl << structures[helix] << endl << structures[sheet] << endl << structures[beta_turn] << endl << structures[summary];
        //      waitukp();

        // if beta-turn is found at position pos
        if (structures[beta_turn][pos] == 'T') {
            // summary at position pos is beta-turn
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
            //          cout << start << " " << end << endl;

            // calculate P_a and P_b for overlap:
            for (int i = start; i <= end; i++) {
                P_a += parameters[helix][i];
                P_b += parameters[sheet][i];
            }

            //          cout << endl << P_a << "   " << P_b << endl;

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
            prob = round(prob * 10);
            if (prob == 10) {
                p = 'X';
            } else {
                p = char((int)prob + 48);
            }

            // set structure and probability for region containing the overlap:
            for (int i = start; i <= end; i++) {
                structures[summary][i] = c;
                probabilities[0][i] = p;
                probabilities[1][i] = ((int)round((parameters[s][i] / maxProb) * 10) == 10) ? 'X' : (char((int)round((parameters[s][i] / maxProb) * 10) + 48));

            }

            // set variables for next pass of loop resp. end of loop
            P_a = 0, P_b = 0;
            pos = end;

            // if helix and sheet are not overlapping and no beta-turn is found at position pos
        } else {
            // summary at position pos is helix resp. sheet
            if (structures[0][pos] == 'H') {
                structures[summary][pos] = 'H';
            } else if (structures[1][pos] == 'E'){
                structures[summary][pos] = 'E';
            }
        }

    }

#ifdef SHOW_PROGRESS
    cout << structures[summary] << endl;
#endif

}


static void setDir(const char *dir) {

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


static int filelen(const char file[]) {

#ifdef SHOW_PROGRESS
    cout << "Getting filelength... ";
#endif

    // concatenate file with working directory
    if (!directory) {
        *directory = '\0';
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


static void readFile(char *content, const char file[]) {

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


static void writeFile(const char file[], const char *content) {

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

#endif

}


static void waitukp(){

#ifdef SHOW_PROGRESS
cout << endl << "Program stopped. Please press [ENTER] to continue. ";
#endif

getchar();
cout << endl;

}


static int nol(const char *string) {

//  get number of lines
int nol = 0;
while (*string != '\0') {
string++;
while ((*string != '\n') && (*string != '\0')) {
string++;
}
nol++;
}
//  while (*string != '\0') {
//      while ((*string != '\n') && (*string != '\0')) {
//          cout << *string;
//          string++;
//      }
//      string++;
//      nol++;
//  }

return nol;

}


static void cropln(char *string, int numberOfLines) {

//  crop lines
int length = strlen(string) + 1;
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
int l = length - (adr - string);
for (int i = 0; i < l; i++) {
*(string + i) = *adr;
adr++;
}
for (int i = l; i < length; i++) {
*(string + i) = '\0';
}

}


static char* scat(int num, char *str1, ...) {

va_list ap;
va_start(ap, str1);

for (int i = 0; i < num; i++) {
strcat(str1, (va_arg(ap, char*)));
}

va_end(ap);
return (str1);

}


static char* scat(char *str1, ...) {

va_list ap;
va_start(ap, str1);
char *p;

while ((p = va_arg(ap, char*))) {
strcat(str1, p);
//      cout << "app ";
}

//  cout << endl;
va_end(ap);
return (str1);

}
