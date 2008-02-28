
#ifndef ED4_PROTEIN_2ND_STRUCTURE_HXX
#define ED4_PROTEIN_2ND_STRUCTURE_HXX

//#define SHOW_PROGRESS
//#define STEP_THROUGH_FIND_NS
//#define STEP_THROUGH_EXT_NS
#define NOCREATE_DISABLED
#define NOREPLACE_DISABLED
#define START_EXTENSION_INSIDE

static char *directory;         /// Working directory
static const char *sequence;    /// Pointer to sequence of protein
static int length;              /// Length of sequence
static char *structures[4];     /// Predicted structures
static char *probabilities[2];  /// Probabilities of prediction
static double *parameters[7];   /// Parameters for structure prediction

static const char *amino_acids = "ARNDCQEGHILKMFPSTWYV"; /// Specifies the characters used for amino acid one letter code
static bool isAA[256]; /// Specifies the characters used for amino acid one letter code
enum structure {three_turn = 4, four_turn = 5, five_turn = 6, helix = 0, sheet = 1, beta_turn = 2, summary = 3}; /// Defines the structure types

typedef enum {
    STRUCT_NONE,
    STRUCT_GOOD_MATCH,
    STRUCT_MEDIUM_MATCH,
    STRUCT_BAD_MATCH,
    STRUCT_NO_MATCH,
    MATCH_COUNT
} PROTEIN_2ND_STRUCT_MATCH;

//TODO: check which functions aren't needed anymore

// ED4 functions
GB_ERROR ED4_predict_summary(const char *seq, char *result_buffer, int result_buffer_size); /// Predicts protein secondary structure from its primary structure (amino acid sequence)
GB_ERROR ED4_calculate_secstruct_match(const char *secstruct1, const char *secstruct2, int start, int end, char *result_buffer); /// Compares two secondary structures
GB_ERROR ED4_calculate_secstruct_sequence_match(const char *secstruct, const char *sequence, int start, int end, char *result_buffer, int strictness = 1); /// Compares a given protein secondary structure with the predicted secondary structure of a given primary structure (amino acid sequence)

// functions for structure prediction:
void printSeq();                         /// Prints sequence to screen
void getParameters(const char file[]);   /// Gets parameters for structure prediction specified in table
void findNucSites(const structure s);    /// Finds nucleation sites that initiate the specified structure (alpha-helix or beta-sheet)
void extNucSites(const structure s);     /// Extends the found nucleation sites in both directions
void findTurns();                        /// Finds beta-turns
void resOverlaps();                      /// Resolves overlaps of predicted structures and creates summary
//void getDSSPVal(const char file[]);    /// Gets sequence and structure from dssp file

// functions for file operations:
void setDir(const char *dir);                            /// Sets working directory
int filelen(const char *file);                           /// Gets filelength of file
void readFile(char *content, const char *file);          /// Reads file and puts content into string
void writeFile(const char file[], const char *content);  /// Writes string to file
void waitukp();                                          /// Waits until any key is pressed

// functions for string manipulation:
int nol(const char *string);                             /// Gets number of lines in string
void cropln(char *string, const int numberOfLines = 1);  /// Crops line/lines at the beginning of string
char* scat(const int num, char *str1, ...);              /// Concatenates str1 with arbitrary number of strings (num specifies number of strings to append)
char* scat(char *str1, ...) __ATTR__SENTINEL;            /// Concatenates str1 with arbitrary number of strings (list must be terminated by NULL)

// helper functions
inline int round_sym(double d); /// Symmetric arithmetic rounding of a double value to an integer value

#endif
