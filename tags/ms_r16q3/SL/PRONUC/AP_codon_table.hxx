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
#ifndef STATIC_ASSERT_H
#include <static_assert.h>
#endif
#ifndef _STDINT_H
#include <stdint.h>
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
#define AWT_MAX_CODONS   64     // maximum of possible codon ( = 4^3)

const int AWAR_PROTEIN_TYPE_bacterial_code_index = 8; // contains the index of the bacterial code table

// --------------------------------------------------------------------------------

#define ALL_TABLES_MASK ((1<<AWT_CODON_TABLES)-1)

class TransTables {
    uint32_t allowed;

    static uint32_t bitmask(int nr) { pn_assert(nr >= 0 && nr<AWT_CODON_TABLES); return 1<<nr; }

public:
    TransTables() { allowAll(); }

    bool is_allowed(int nr) const { return (allowed & bitmask(nr)) != 0; }
    bool any()  const { return  allowed; }
    bool none() const { return !allowed; }
    bool all() const { return allowed == ALL_TABLES_MASK; }

    void allow(int nr) { allowed |= bitmask(nr); }
    void forbid(int nr) { allowed &= ~bitmask(nr); }
    void forbid(const TransTables& other) { allowed &= ~other.allowed; }

    void allowAll() {
        STATIC_ASSERT(sizeof(allowed)*8 > AWT_CODON_TABLES); // one bit wasted
        allowed = ALL_TABLES_MASK;
    }
    void forbidAll() { allowed = 0; }
    void forbidAllBut(int nr) { allowed &= bitmask(nr); }

    int explicit_table() const {
        // return explicit table number (or -1 if not exactly one table is allowed)
        if (any()) {
            uint32_t tabs = allowed;
            for (int t = 0; t<AWT_CODON_TABLES; ++t) {
                if (tabs&1) return (tabs == 1) ? t : -1;
                tabs >>= 1;
            }
            pn_assert(0);
        }
        return -1;
    }

    bool is_subset_of(const TransTables& other) const { return (allowed&other.allowed) == allowed; }

    const char *to_string() const {
        const int    MAX_LEN = 42;
        static char  buffer[MAX_LEN];
        char        *out     = buffer;

        for (int a = 0; a<AWT_CODON_TABLES; ++a) {
            if (is_allowed(a)) {
                out += sprintf(out, ",%i", a);
            }
        }
        pn_assert((out-buffer)<MAX_LEN);
        out[0] = 0;
        return buffer+1;
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
