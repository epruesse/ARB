#ifndef awt_codon_table_included
#define awt_codon_table_included

#include <awt.hxx>

// --------------------------------------------------------------------------------

struct AWT_Codon_Code_Definition {
    const char *name;
    const char *aa; // amino-codes
    const char *starts;
};

#define AWT_CODON_CODES 14 // number of different Amino-Translation-Codes
#define AWT_MAX_CODONS 64  // maximum of possible codon (= 4^3)

extern struct AWT_Codon_Code_Definition AWT_codon_def[AWT_CODON_CODES+1];

// --------------------------------------------------------------------------------

class AWT_allowedCode {
    char allowed[AWT_CODON_CODES];
    
    void copy(const AWT_allowedCode& other) {
	for (int a=0; a<AWT_CODON_CODES; a++) {
	    allowed[a] = other.allowed[a];
	}
    }
    void set(int val) {
	for (int a=0; a<AWT_CODON_CODES; a++) {
	    allowed[a] = val;
	}
    }
    void legal(int nr) const {
	if (nr<0 || nr>=AWT_CODON_CODES) {
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
};

// --------------------------------------------------------------------------------

void AWT_initialize_codon_tables();

int AWT_is_codon(char protein, const char *dna, const AWT_allowedCode& allowed_code, AWT_allowedCode& allowed_code_left);
const char *AWT_get_codons(char protein, int code_nr);

const char *AWT_get_protein_name(char protein);
const char* AWT_get_codon_code_name(int code);

#ifdef DEBUG
void AWT_dump_codons();
void test_AWT_get_codons();
#endif

// --------------------------------------------------------------------------------

#endif
