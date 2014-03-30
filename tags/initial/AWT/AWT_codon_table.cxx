#include <malloc.h>
#include <string.h>
#include <ctype.h>

#include "awt_iupac.hxx"
#include "awt_codon_table.hxx"

struct AWT_Codon_Code_Definition AWT_codon_def[AWT_CODON_CODES+1] =
{
    //	"TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG",  base1
    //	"TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG",  base2
    //	"TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG"   base3
    { 
        "Standard Code",
        "FFLLSSSSYY**CC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
        "---M---------------M---------------M----------------------------"
    }, 
    { 
        "Bacterial Code", 
        "FFLLSSSSYY**CC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG", 
        "---M---------------M------------MMMM---------------M------------" 
    }, 
    { 
        "Vertebrate Mitochondrial Code",  
        "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIMMTTTTNNKKSS**VVVVAAAADDEEGGGG", 
        "--------------------------------MMMM---------------M------------"
    }, 
    { 
        "Invertebrate Mitochondrial Code", 
        "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIMMTTTTNNKKSSSSVVVVAAAADDEEGGGG", 
        "---M----------------------------MMMM---------------M------------" 
    }, 
    { 
        "Yeast Mitochondrial Code", 
        "FFLLSSSSYY**CCWWTTTTPPPPHHQQRRRRIIMMTTTTNNKKSSRRVVVVAAAADDEEGGGG", 
        "-----------------------------------M----------------------------" 
    }, 
    { 
        "Mold/Protozoan/Coelenterate Mitochondrial Code", // identical with "Mycoplasma/Spiroplasma Code" (split because of name-length)
        "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG", 
        "--MM---------------M------------MMMM---------------M------------" 
    }, 
    { 
        "Echinoderm Mitochondrial Code", 
        "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNNKSSSSVVVVAAAADDEEGGGG", 
        "-----------------------------------M----------------------------" 
    }, 
    { 
        "Ascidian Mitochondrial Code", 
        "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIMMTTTTNNKKSSGGVVVVAAAADDEEGGGG", 
        "-----------------------------------M----------------------------" 
    }, 
    { 
        "Flatworm Mitochondrial Code", 
        "FFLLSSSSYYY*CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNNKSSSSVVVVAAAADDEEGGGG", 
        "-----------------------------------M----------------------------" 
    }, 
    { 
        "Mycoplasma/Spiroplasma Code", // identical with "Mold/Protozoan/Coelenterate Mitochondrial Code" (split because of name-length)
        "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG", 
        "--MM---------------M------------MMMM---------------M------------" 
    }, 
    { 
        "Ciliate, Dasycladacean and Hexamita Nuclear Code", 
        "FFLLSSSSYYQQCC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG", 
        "-----------------------------------M----------------------------" 
    }, 
    { 
        "Euplotid Nuclear Code", 
        "FFLLSSSSYY**CCCWLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG", 
        "-----------------------------------M----------------------------" 
    }, 
    { 
        "Alternative Yeast Nuclear Code", 
        "FFLLSSSSYY**CC*WLLLSPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG", 
        "-------------------M---------------M----------------------------" 
    }, 
    { 
        "Blepharisma Nuclear Code", 
        "FFLLSSSSYY*QCC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG", 
        "-----------------------------------M----------------------------" 
    }, 
    
    { 0, 0, 0 } // end of table-marker
};

static int codon_tables_initialized = 0;
static char definite_translation[AWT_MAX_CODONS]; // contains 0 if ambiguous, otherwise it contains the definite translation
static char *ambiguous_codons[AWT_MAX_CODONS]; // for each ambiguous codon: contains all translations (each only once)

void AWT_initialize_codon_tables() {
    if (codon_tables_initialized) return;
    
    int codon_nr; 
    int code_nr;
    
    for (codon_nr=0; codon_nr<AWT_MAX_CODONS; codon_nr++) {
        ambiguous_codons[codon_nr] = 0;
    }
    
    awt_assert(AWT_CODON_CODES>=1);
    memcpy(definite_translation, AWT_codon_def[0].aa, AWT_MAX_CODONS); // only one translation is really definite
    
    awt_assert(AWT_codon_def[AWT_CODON_CODES].aa==NULL); // Error in AWT_codon_def or AWT_CODON_CODES
    
    for (code_nr=1; code_nr<AWT_CODON_CODES; code_nr++) {
        const char *translation = AWT_codon_def[code_nr].aa;
	
        for (codon_nr=0; codon_nr<AWT_MAX_CODONS; codon_nr++) {
            if (definite_translation[codon_nr]!='?') { // is definite till now
                if (definite_translation[codon_nr]!=translation[codon_nr]) { // we found a different translation
                    // create ambiguous_codons:
                    char *amb = (char*)GB_calloc(AWT_MAX_CODONS+1, sizeof(char));
                    amb[0] = definite_translation[0];
                    amb[1] = translation[codon_nr];
		    
                    ambiguous_codons[codon_nr] = amb;
                    definite_translation[codon_nr] = '?';
#if defined(DEBUG) && 0
                    printf("amb[%i]='%s'\n", codon_nr, amb);
#endif
                }
            }
            else { // is ambiguous
                if (strchr(ambiguous_codons[codon_nr], translation[codon_nr])==0) { // not listed in ambiguous codons
                    // append another ambiguous codon:
                    char *amb = ambiguous_codons[codon_nr];
                    amb[strlen(amb)] = translation[codon_nr];
#if defined(DEBUG) && 0
                    printf("amb[%i]='%s'\n", codon_nr, amb);
#endif
                }
            }
        }
    }
    
    codon_tables_initialized = 1;
}

// return 0..3 (ok) or 4 (failure)
inline int dna2idx(char c) {
    switch (c) {
        case 'T': case 't':
        case 'U': case 'u': return 0; 
        case 'C': case 'c': return 1;
        case 'A': case 'a': return 2;
        case 'G': case 'g': return 3;
    }
    return 4;
}

inline char idx2dna(int idx) {
    awt_assert(idx>=0 && idx<4);
    return "TCAG"[idx];
}

inline int calc_codon_nr(const char *dna) {
    int i1 = dna2idx(dna[0]);
    int i2 = dna2idx(dna[1]);
    int i3 = dna2idx(dna[2]);
	
    if (i1==4||i2==4||i3==4) return AWT_MAX_CODONS; // is not a codon
    
    int codon_nr = i1*16 + i2*4 + i3;
    awt_assert(codon_nr>=0 && codon_nr<=AWT_MAX_CODONS);
    return codon_nr;
}

inline void build_codon(int codon_nr, char *to_buffer) {
    awt_assert(codon_nr>=0 && codon_nr<AWT_MAX_CODONS);
    
    to_buffer[0] = idx2dna((codon_nr>>4)&3);
    to_buffer[1] = idx2dna((codon_nr>>2)&3);
    to_buffer[2] = idx2dna(codon_nr&3); 
}

const char* AWT_get_codon_code_name(int code) {
    awt_assert(code>=0 && code<AWT_CODON_CODES);
    return AWT_codon_def[code].name;
}

static const char *protein_name[26+1] = {
    "Ala", // A
        "Asx", // B	
        "Cys", // C
        "Asp", // D
        "Glu", // E
        "Phe", // F
        "Gly", // G
        "His", // H
        "Ile", // I
        0,     // J
        "Lys", // K
        "Leu", // L
        "Met", // M
        "Asn", // N
        0,     // O
        "Pro", // P
        "Gln", // Q
        "Arg", // R
        "Ser", // S
        "Thr", // T
        0,     // U
        "Val", // V
        "Trp", // W
        "Xxx", // X
        "Tyr", // Y
        "Glx", // Z
        0
        };

const char *AWT_get_protein_name(char protein) {
    if (protein=='*') return "End";
    if (protein=='-') return "---";
    
    awt_assert(protein>='A' && protein<='Z');
    awt_assert(protein_name[protein-'A']!=0);
    return protein_name[protein-'A'];
}

#ifdef DEBUG

inline char nextBase(char c) {
    switch (c) {
        case 'T': return 'C';
        case 'C': return 'A';
        case 'A': return 'G';
        case 'G': return 0;
        default: awt_assert(0);
    }
    return 0;
}

void AWT_dump_codons() {
    AWT_allowedCode allowed_code;
    
    for (char c='*'; c<='Z'; c++) {
        printf("Codes for '%c': ", c); 
        int first_line = 1;
        int found = 0;
        for (char b1='T'; b1; b1=nextBase(b1)) {
            for (char b2='T'; b2; b2=nextBase(b2)) {
                for (char b3='T'; b3; b3=nextBase(b3)) {
                    char dna[4];
                    dna[0]=b1;
                    dna[1]=b2;
                    dna[2]=b3; 
                    dna[3]=0;
		    
                    AWT_allowedCode allowed_code_left;
                    if (AWT_is_codon(c, dna, allowed_code, allowed_code_left)) {
                        if (!first_line) printf("\n               "); 
                        first_line = 0;
                        printf("%s (", dna);
			
                        int first=1;
                        for (int code=0; code<AWT_CODON_CODES; code++) {
                            if (allowed_code_left.is_allowed(code)) {
                                if (!first) printf(",");
                                first=0;
                                printf("%i",code);
                            }
                        }
                        printf(") "); 
			
                        found = 1;
                    }
                }
            }
        }
        if (!found) printf("none");
        printf("\n");
        if (c=='*') c='A'-1;
    }
}
#endif

// return TRUE if 'dna' contains a codon of 'protein' ('dna' must not contain any gaps)
// allowed_code contains 1 for each allowed code and 0 otherwise
// allowed_code_left contains a copy of allowed_codes with all impossible codes set to zero

int AWT_is_codon(char protein, const char *dna, const AWT_allowedCode& allowed_code, AWT_allowedCode& allowed_code_left) {
    awt_assert(codon_tables_initialized); 
    
    protein = toupper(protein);
    if (protein=='B') { // B is a shortcut for Asp(=D) or Asn(=N)
        return
            AWT_is_codon('D', dna, allowed_code, allowed_code_left) || 
            AWT_is_codon('N', dna, allowed_code, allowed_code_left); 
    }
    else if (protein=='Z') { // Z is a shortcut for Glu(=E) or Gln(=Q) 
        return
            AWT_is_codon('E', dna, allowed_code, allowed_code_left) ||
            AWT_is_codon('Q', dna, allowed_code, allowed_code_left);	
    }
    
    int codon_nr = calc_codon_nr(dna);
    if (codon_nr==AWT_MAX_CODONS) { // dna is not a clean codon (it contains iupac-codes)
        int error_positions = 0;
        int first_error_pos = -1; 
        {
            int iupac_pos;
            for (iupac_pos=0; iupac_pos<3; iupac_pos++) {
                if (strchr("ACGTU", dna[iupac_pos])==0) {
                    if (first_error_pos==-1) first_error_pos = iupac_pos;
                    error_positions++;
                } 
            } 
        } 
        gb_assert(error_positions); 
        if (error_positions==3) { // don't accept codons with 3 errors
            return 0;
        }
	
        const char *decoded_iupac = AWT_decode_iupac(dna[first_error_pos], GB_AT_DNA, 0); 
        char dna_copy[4]; 
        memcpy(dna_copy, dna, 3);
        dna_copy[3] = 0;
	
#if defined(DEBUG) && 0
        printf("Check if '%s' is a codon for '%c'\n", dna_copy, protein);
#endif
	
        int all_are_codons = 1;
        AWT_allowedCode allowed_code_copy;
        allowed_code_copy = allowed_code;
	
        for (int i=0; decoded_iupac[i]; i++) {
            dna_copy[first_error_pos] = decoded_iupac[i];
            if (!AWT_is_codon(protein, dna_copy, allowed_code_copy, allowed_code_left)) {
                all_are_codons = 0;
                break;
            } 
            allowed_code_copy = allowed_code_left;
        }
	
        if (all_are_codons) {
            allowed_code_left = allowed_code_copy;
        }
        else {
            allowed_code_left.forbidAll();
        }
#if defined(DEBUG) && 0
        printf("result 	= %i\n", all_are_codons);
#endif
        return all_are_codons;
    }
    
    if (definite_translation[codon_nr]!='?') {
        int ok = definite_translation[codon_nr]==protein; 
	
        if (ok) allowed_code_left = allowed_code;
        else allowed_code_left.forbidAll(); 
	
        return ok;
    }
    
    if (strchr(ambiguous_codons[codon_nr], protein)==0) {
        allowed_code_left.forbidAll();
        return 0; 
    }
    
    // search for allowed correct translation possibity:
    int found = 0; 
    for (int code_nr=0; code_nr<AWT_CODON_CODES; code_nr++) {
        if (allowed_code.is_allowed(code_nr) &&  // is this code allowed?
            AWT_codon_def[code_nr].aa[codon_nr]==protein) { // and does it translate correct?
            allowed_code_left.allow(code_nr);
            found = 1;
        }
        else {
            allowed_code_left.forbid(code_nr); // otherwise forbid code in future
        }
    }
    
    return found;
}

// -------------------------------------------------------------------------------- Codon_Group

class Codon_Group
{
    char codon[64]; // index is calculated with calc_codon_nr
    
public:
    Codon_Group(char protein, int code_nr);
    ~Codon_Group() {}
    
    //    static int idx(int x, int y, int z) const { return (((x<<2)+y)<<2)+z; }
    //    static int is_idx(int idx) { return idx>=0 && idx<AWT_MAX_CODONS; }
    
    //    void add_member(int idx) { awt_assert(is_idx(idx)); codon[idx] = 1; }    
    Codon_Group& operator+=(const Codon_Group& other);
    int expand(char *to_buffer) const;
};

Codon_Group::Codon_Group(char protein, int code_nr) {    
    protein = toupper(protein); 
    awt_assert(protein=='*' || isalpha(protein));
    awt_assert(code_nr>=0 && code_nr<AWT_CODON_CODES);
    
    const char *amino_table = AWT_codon_def[code_nr].aa;
    for (int i=0; i<AWT_MAX_CODONS; i++) {
        codon[i] = amino_table[i]==protein;
    }
}

Codon_Group& Codon_Group::operator+=(const Codon_Group& other) {
    for (int i=0; i<AWT_MAX_CODONS; i++) {
        codon[i] = codon[i] || other.codon[i];
    }
    return *this;
}

inline int legal_dna_no(int i) { return i>=0 && i<4; } 
inline void my_memcpy(char *dest, const char *source, size_t length) { for (size_t l=0; l<length; l++) { dest[l] = source[l]; } }

inline const char *buildMixedCodon(const char *con1, const char *con2) {
    int mismatches = 0;
    int mismatch_index = -1;
    static char buf[4];
    
    for (int i=0; i<3; i++) {
        if (con1[i]!=con2[i]) {
            mismatches++;
            mismatch_index = i; 
        } 
        else {
            buf[i] = con1[i];
        }
    }
    
    if (mismatches==1) { // exactly one position differs between codons 
        awt_assert(mismatch_index!=-1);
        buf[mismatch_index] = AWT_iupac_add(con1[mismatch_index], con2[mismatch_index], GB_AT_DNA);
        buf[3] = 0;
        return buf;
    }
    return 0;
}

static int expandMore(const char *bufferStart, int no_of_condons, char*&to_buffer) {
    int i, j;
    const char *con1, *con2;
    int added = 0;
    
    for (i=0; i<no_of_condons; i++) {
        con1 = bufferStart+3*i;
	
        for (j=i+1; j<no_of_condons; j++) {
            con2 = bufferStart+3*j;
            const char *result = buildMixedCodon(con1, con2);
            if (result) {
                to_buffer[0] = 0; 
                // do we already have this codon?
                const char *found;
                const char *startSearch = bufferStart;
                for (;;) {
                    found = strstr(startSearch, result);
                    if (!found) break;
                    int pos = (found-bufferStart);
                    if ((pos%3)==0) break; // yes aready here!
                    startSearch = found+1; // was misaligned -> try behind
                }

                if (!found) { 
                    my_memcpy(to_buffer, result, 3); to_buffer+=3;
                    added++;
                }
            }
        }
    }
    return no_of_condons+added;
}

int Codon_Group::expand(char *to_buffer) const {
    int count = 0;
    int i;
    char *org_to_buffer = to_buffer;
    
    for (i=0; i<AWT_MAX_CODONS; i++) {
        if (codon[i]) {
            build_codon(i, to_buffer);
            to_buffer += 3;
            count++;
        }
    }
    
#if defined(DEBUG) && 0
    to_buffer[0] = 0;
    printf("codons = '%s'\n", org_to_buffer);
#endif
    
    for (;;) {
        int new_count = expandMore(org_to_buffer, count, to_buffer);
        if (new_count==count) break; // nothing expanded -> done
        count = new_count;
#if defined(DEBUG) && 0
        to_buffer[0] = 0;
        printf("codons (expandedMore) = '%s'\n", org_to_buffer);
#endif 
    }
    
    awt_assert(count==(int(to_buffer-org_to_buffer)/3)); 
    
    return count;
}
	

// --------------------------------------------------------------------------------

#define MAX_CODON_LIST_LENGTH 200

// get a list of all codons ("xyzxyzxyz...") encoding 'protein' in case we use Codon-Code 'code_nr'
// (includes all completely contained IUPAC-encoded codons at the end of list)
const char *AWT_get_codons(char protein, int code_nr) {
    awt_assert(code_nr>=0 && code_nr<AWT_CODON_CODES);
    protein = toupper(protein);
    awt_assert(isalpha(protein) || protein=='*'); 
    awt_assert(codon_tables_initialized);    
    
    Codon_Group *cgroup = 0;
    
    if (protein=='B') {
        cgroup = new Codon_Group('D', code_nr);
        Codon_Group N('N', code_nr);
        *cgroup += N;
    }
    else if (protein=='Z') {
        cgroup = new Codon_Group('E', code_nr);
        Codon_Group Q('Q', code_nr);
        *cgroup += Q;	
    }
    else {
        cgroup = new Codon_Group(protein, code_nr);
    }
    
    awt_assert(cgroup);    
    
    static char buffer[MAX_CODON_LIST_LENGTH]; 
    int offset = 3*cgroup->expand(buffer);
    awt_assert(offset<MAX_CODON_LIST_LENGTH);    
    buffer[offset] = 0; 
    
    delete cgroup; 
    
    return buffer;
}

#if defined(DEBUG) && 1
void test_AWT_get_codons() {
    AWT_initialize_codon_tables();
    for (int code_nr=0; code_nr<1; /*AWT_CODON_CODES;*/ code_nr++) {
        printf("--------------------- Code = %i\n", code_nr);
        for (char c='*'; c<='Z'; c++) {
            const char *got_codons = AWT_get_codons(c, code_nr);
            printf("%c='%s'\n", c, got_codons);
            if (c=='*') c='A'-1;
        }
    }
}
#endif
