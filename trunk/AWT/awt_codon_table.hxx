#ifndef awt_codon_table_included
#define awt_codon_table_included

#include <awt.hxx>

// --------------------------------------------------------------------------------

struct AWT_Codon_Code_Definition {
    const char *name;
    const char *aa;             // amino-codes
    const char *starts;
    int         embl_feature_transl_table; // number of transl_table-entry in EMBL/GENEBANK features list
};

#define AWT_CODON_TABLES 17     // number of different Amino-Translation-Tables
#define AWT_MAX_CODONS 64       // maximum of possible codon (= 4^3)

extern struct AWT_Codon_Code_Definition AWT_codon_def[AWT_CODON_TABLES+1];

const int AWAR_PROTEIN_TYPE_bacterial_code_index = 8; // contains the index of the bacterial code table

// --------------------------------------------------------------------------------

class AWT_allowedCode {
    char allowed[AWT_CODON_TABLES];

    void copy(const AWT_allowedCode& other) {
        for (int a=0; a<AWT_CODON_TABLES; a++) {
            allowed[a] = other.allowed[a];
        }
    }
    void set(int val) {
        for (int a=0; a<AWT_CODON_TABLES; a++) {
            allowed[a] = val;
        }
    }
    void legal(int nr) const {
        if (nr<0 || nr>=AWT_CODON_TABLES) {
            *((char*)0)=0; // throw exception
        }
    }
public:
    AWT_allowedCode() { set(1); }
    AWT_allowedCode(const AWT_allowedCode& other) { copy(other); }
    AWT_allowedCode& operator=(const AWT_allowedCode& other)  { copy(other); return *this; }

    int is_allowed(int nr) const { legal(nr); return allowed[nr]!=0; }
    void allow(int nr) { legal(nr); allowed[nr]=1; }
    void forbid(int nr) { legal(nr); allowed[nr]=0; }

    void forbidAll() { set(0); }
    void allowAll() { set(1); }

    void forbidAllBut(int nr) {
        legal(nr);
        for (int a=0; a<AWT_CODON_TABLES; a++) {
            if (a != nr) allowed[a] = 0;
        }
    }
};

// --------------------------------------------------------------------------------

void AWT_initialize_codon_tables();

int AWT_embl_transl_table_2_arb_code_nr(int embl_index);
int AWT_arb_code_nr_2_embl_transl_table(int arb_code_nr);

bool        AWT_is_codon(char protein, const char *dna, const AWT_allowedCode& allowed_code, AWT_allowedCode& allowed_code_left, const char **fail_reason = 0);
const char *AWT_get_codons(char protein, int code_nr);

char AWT_is_start_codon(const char *dna, int code_nr);

const char *AWT_get_protein_name(char protein);
const char* AWT_get_codon_code_name(int code);

#ifdef DEBUG
void AWT_dump_codons();
void test_AWT_get_codons();
#endif

// --------------------------------------------------------------------------------

#endif
