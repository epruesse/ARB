// =============================================================== //
//                                                                 //
//   File      : AP_codon_table.hxx                                //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de)                   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_CODON_TABLE_HXX
#define AP_CODON_TABLE_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define pn_assert(cond) arb_assert(cond)

// --------------------------------------------------------------------------------

struct AWT_Codon_Code_Definition {
    const char *name;
    const char *aa;             // amino-codes
    const char *starts;
    int         embl_feature_transl_table; // number of transl_table-entry in EMBL/GENEBANK features list
};

#define AWT_CODON_TABLES 17     // number of different Amino-Translation-Tables
#define AWT_MAX_CODONS 64       // maximum of possible codon (= 4^3)

const int AWAR_PROTEIN_TYPE_bacterial_code_index = 8; // contains the index of the bacterial code table

// --------------------------------------------------------------------------------

class TransTables {
    char allowed[AWT_CODON_TABLES];

    void copy(const TransTables& other) {
        for (int a=0; a<AWT_CODON_TABLES; a++) {
            allowed[a] = other.allowed[a];
        }
    }

    void legal(int IF_ASSERTION_USED(nr)) const { pn_assert(nr >= 0 && nr<AWT_CODON_TABLES); }

public:
    TransTables() { allowAll(true); }
    TransTables(const TransTables& other) { copy(other); }
    TransTables& operator=(const TransTables& other)  { copy(other); return *this; }

    int is_allowed(int nr) const { legal(nr); return allowed[nr] != 0; }
    bool any() const {
        int a = 0;
        while (a<AWT_CODON_TABLES && !allowed[a]) ++a;
        return a<AWT_CODON_TABLES;
    }
    bool none() const { return !any(); }

    void allow(int nr) { legal(nr); allowed[nr] = 1; }
    void forbid(int nr) { legal(nr); allowed[nr]=0; }
    void forbid(const TransTables& other) {
        for (int a=0; a<AWT_CODON_TABLES; a++) {
            if (other.is_allowed(a)) forbid(a);
        }
    }

    void allowAll(bool Allow = true) {
        for (int a=0; a<AWT_CODON_TABLES; a++) {
            allowed[a] = Allow;
        }
    }
    void forbidAll() { allowAll(false); }

    void forbidAllBut(int nr) {
        legal(nr);
        for (int a=0; a<AWT_CODON_TABLES; a++) {
            if (a != nr) allowed[a] = 0;
        }
    }

    int explicit_table() const {
        // return explicit table number (or -1 if not exactly 1 table is allowed)
        int table = -1;
        for (int i = 0; i<AWT_CODON_TABLES; ++i) {
            if (allowed[i]) {
                if (table != -1) return -1;
                table = i;
            }
        }
        return table;
    }

    bool is_subset_of(const TransTables& other) const {
        for (int i = 0; i<AWT_CODON_TABLES; ++i) {
            if (is_allowed(i) && !other.is_allowed(i)) return false;
        }
        return true;
    }
};

// --------------------------------------------------------------------------------

void AP_initialize_codon_tables();

int AWT_embl_transl_table_2_arb_code_nr(int embl_code_nr);
int AWT_arb_code_nr_2_embl_transl_table(int arb_code_nr);

bool        AWT_is_codon(char protein, const char *const dna, const TransTables& allowed, TransTables& allowed_left, const char **fail_reason_ptr = 0);
const char *AP_get_codons(char protein, int code_nr);

char AWT_is_start_codon(const char *dna, int arb_code_nr);

const char *AP_get_protein_name(char protein);
const char* AWT_get_codon_code_name(int code);

#ifdef DEBUG
void AWT_dump_codons();
void test_AWT_get_codons();
#endif

// --------------------------------------------------------------------------------

#else
#error AP_codon_table.hxx included twice
#endif // AP_CODON_TABLE_HXX
