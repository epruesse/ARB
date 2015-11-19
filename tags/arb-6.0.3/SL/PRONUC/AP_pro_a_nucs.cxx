// =============================================================== //
//                                                                 //
//   File      : AP_pro_a_nucs.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AP_pro_a_nucs.hxx"

#include <AP_codon_table.hxx>
#include <arbdbt.h>
#include <ad_cb.h>
#include <arb_str.h>

#define pn_assert(cond) arb_assert(cond)

char *AP_create_dna_to_ap_bases() {
    int       i;
    AP_BASES  val;
    char     *table = new char[256];

    for (i=0; i<256; i++) {
        switch ((char)i) {
            case 'a': case 'A': val = AP_A; break;
            case 'g': case 'G': val = AP_G; break;
            case 'c': case 'C': val = AP_C; break;
            case 't': case 'T':
            case 'u': case 'U': val = AP_T; break;
            case 'n': case 'N': val = (AP_BASES)(AP_A + AP_G + AP_C + AP_T); break;
            case '?': case '.': val = (AP_BASES)(AP_A + AP_G + AP_C + AP_T + AP_S); break;
            case '-': val = AP_S; break;
            case 'm': case 'M': val = (AP_BASES)(AP_A+AP_C); break;
            case 'r': case 'R': val = (AP_BASES)(AP_A+AP_G); break;
            case 'w': case 'W': val = (AP_BASES)(AP_A+AP_T); break;
            case 's': case 'S': val = (AP_BASES)(AP_C+AP_G); break;
            case 'y': case 'Y': val = (AP_BASES)(AP_C+AP_T); break;
            case 'k': case 'K': val = (AP_BASES)(AP_G+AP_T); break;
            case 'v': case 'V': val = (AP_BASES)(AP_A+AP_C+AP_G); break;
            case 'h': case 'H': val = (AP_BASES)(AP_A+AP_C+AP_T); break;
            case 'd': case 'D': val = (AP_BASES)(AP_A+AP_G+AP_T); break;
            case 'b': case 'B': val = (AP_BASES)(AP_C+AP_G+AP_T); break;
            default: val = AP_N; break;
        }
        table[i] = (char)val;
    }
    return table;
}

long *AWT_translator::create_pro_to_bits() const {
    int i;
    int j;
    long *table = (long *)GB_calloc(sizeof(long), 256);
    for (i = 0;   i < max_aa; i++) {
        j = index_2_spro[i];
        if (j == '.') {
            table[i] = -1;
            continue;
        }
        j = s2str[j]->index;
        table[i] = 1<<j;
    }
    return table;
}

void AWT_translator::build_table(unsigned char pbase, const char *tri_pro, const char *nuc) {
    struct arb_r2a_pro_2_nuc  *str = s2str[pbase];

    // search for existing protein, else generate new
    if (!str) {
        str                               = new arb_r2a_pro_2_nuc;
        s2str[pbase]          = str;
        s2str[tolower(pbase)] = str;

        str->index      = max_aa++;
        str->single_pro = pbase;
        str->tri_pro[0] = tri_pro[0];
        str->tri_pro[1] = tri_pro[1];
        str->tri_pro[2] = tri_pro[2];

        index_2_spro[str->index] = pbase;
    }
    // fast hash table
    GBS_write_hash(t2i_hash, nuc, pbase);

    int n0 = nuc_2_bitset[safeCharIndex(nuc[0])];
    int n1 = nuc_2_bitset[safeCharIndex(nuc[1])];
    int n2 = nuc_2_bitset[safeCharIndex(nuc[2])];

    struct arb_r2a_pro_2_nucs *nucs;
    for (nucs = str->nucs; nucs; nucs = nucs->next) {
        if ((!(nucs->nucbits[0] & ~n0)) &&      // search superset
            (!(nucs->nucbits[1] & ~n1)) &&
            (!(nucs->nucbits[2] & ~n2))) break;

        int c = 0;
        if (nucs->nucbits[0] != n0) c++;
        if (nucs->nucbits[1] != n1) c++;
        if (nucs->nucbits[2] != n2) c++;
        if (c <= 1) break;
    }
    if (!nucs) {
        nucs       = new arb_r2a_pro_2_nucs;
        nucs->next = str->nucs;
        str->nucs  = nucs;
    }
    nucs->nucbits[0] |= n0;
    nucs->nucbits[1] |= n1;
    nucs->nucbits[2] |= n2;
}

// ----------------------------
//      arb_r2a_pro_2_nucs

arb_r2a_pro_2_nucs::arb_r2a_pro_2_nucs()
    : next(0)
{
    memset(nucbits, 0, sizeof(nucbits));
}
arb_r2a_pro_2_nucs::~arb_r2a_pro_2_nucs() {
    delete next;
}

// ---------------------------
//      arb_r2a_pro_2_nuc

arb_r2a_pro_2_nuc::arb_r2a_pro_2_nuc()
    : single_pro(0)
    , index(0)
    , nucs(0)
{
    memset(tri_pro, 0, sizeof(tri_pro));
}
arb_r2a_pro_2_nuc::~arb_r2a_pro_2_nuc() {
    delete nucs;
}

// ------------------------
//      AWT_translator

static int codon_defined_in(const char *codon, const char *codons) {
    for (int off=0; codons[off]; off+=3) {
        if (codon[0]==codons[off] && codon[1]==codons[off+1] && codon[2]==codons[off+2]) {
            return 1;
        }
    }
    return 0;
}

// order of tables generated with build_table() by AWT_translator ctor is important:
// must be compatible with DIST/PH_protdist.cxx !!
// except that this table has an 's' insertion !!!

#define T2I_ENTRIES_MAX 190

AWT_translator::AWT_translator(int arb_protein_code_nr) :
    distance_meter(0),
    code_nr(arb_protein_code_nr),
    pro_2_bitset(0),
    realmax_aa(0),
    max_aa(0)
{
    memset(s2str, 0, sizeof(s2str));
    memset(index_2_spro, 0, sizeof(index_2_spro));

    nuc_2_bitset = AP_create_dna_to_ap_bases();
    t2i_hash     = GBS_create_hash(T2I_ENTRIES_MAX, GB_IGNORE_CASE); // case insensitive

    AP_initialize_codon_tables();

    {
        char *D_codons = strdup(AP_get_codons('D', code_nr));
        char *N_codons = strdup(AP_get_codons('N', code_nr));
        char *E_codons = strdup(AP_get_codons('E', code_nr));
        char *Q_codons = strdup(AP_get_codons('Q', code_nr));

        char protein;
        for (protein='*'; protein<='Z'; protein = (protein=='*' ? 'A' : protein+1)) {
            if (protein!='J' && protein!='O' && protein!='U') { // JOU are no aminos
                const char *codons;
                if (protein=='D')   codons = D_codons;
                else if (protein=='N')  codons = N_codons;
                else if (protein=='E')  codons = E_codons;
                else if (protein=='Q')  codons = Q_codons;
                else            codons = AP_get_codons(protein, code_nr);
                // codons now contains a 0-terminated-string containing all possible codons for protein

                const char *protein_name = AP_get_protein_name(protein);

                for (int off=0; codons[off]; off+=3) {
                    char codon[4];
                    memcpy(codon, codons+off, 3);
                    codon[3] = 0;

                    if (protein=='B') {
                        if (!codon_defined_in(codon, D_codons) && !codon_defined_in(codon, N_codons)) {
                            build_table(protein, protein_name, codon);
                        }
                    }
                    else if (protein=='Z') {
                        if (!codon_defined_in(codon, E_codons) && !codon_defined_in(codon, Q_codons)) {
                            build_table(protein, protein_name, codon);
                        }
                    }
                    else {
                        build_table(protein, protein_name, codon);
                    }
                }
            }
        }

        free(Q_codons);
        free(E_codons);
        free(N_codons);
        free(D_codons);
    }

    realmax_aa = max_aa;

    build_table('-', "---", "---");
    build_table('.', "...", "...");
    build_table('.', "???", "???");
    build_table('X', "NNN", "NNN");

    pn_assert(GBS_hash_count_elems(t2i_hash) <= T2I_ENTRIES_MAX);

    pro_2_bitset = create_pro_to_bits();
}

AWT_translator::~AWT_translator() {
    free(pro_2_bitset);

    delete [] nuc_2_bitset;
    GBS_free_hash(t2i_hash);
    for (int i=0; i<256; i++) {
        if (i != tolower(i)) continue; // do not delete duplicated entries (a-z == A-Z!)
        delete s2str[i];
    }

    delete distance_meter;
}

const AWT_distance_meter *AWT_translator::getDistanceMeter() const {
    if (!distance_meter) distance_meter = new AWT_distance_meter(this);
    return distance_meter;
}


// ----------------------------
//      Distance functions

static int nuc_dist(const AWT_translator *translator, unsigned char p1, unsigned char p2) {
    // calculate minimum necessary nucleotide-mutations for a given amino-acid-mutation

    const struct arb_r2a_pro_2_nuc *s1, *s2;
    s1                                      = translator->S2str(p1);
    s2                                      = translator->S2str(p2);
    if ((!s1) || (!s2)) return -1;
    struct arb_r2a_pro_2_nucs      *n1, *n2;
    long                            mindist = 3;
    // Check all combinations, if any combination is valid -> zero distance
    for (n1 = s1->nucs; n1; n1=n1->next) {
        for (n2 = s2->nucs; n2; n2=n2->next) {
            int dist = 0;
            int i;
            for (i=0; i<3; i++) {
                if (n1->nucbits[i] & n2->nucbits[i]) continue;
                dist++;
            }
            if (dist< mindist) mindist = dist;
        }
    }
    return mindist;
}

#if defined(DEBUG)
static void awt_pro_a_nucs_debug(const AWT_translator *translator, const AWT_distance_meter *distmeter) {
    int max_aa     = translator->MaxAA();
    int realmax_aa = translator->RealmaxAA();

    for (int s = 0; s< max_aa; s++) {
        const AWT_PDP *dist = distmeter->getDistance(s);

        // check bits should not be present in distpad
        if (s<realmax_aa) {
            for (int i=0; i<2; i++) {
                // assertion: bit is set in patd[i] -> bit is clear in patd[i+1]
                assert_or_exit((dist->patd[i] & ~dist->patd[i+1]) == 0);
            }
        }
        printf("Base %c[%i]: Dist to ", translator->Index2Spro(s), s);
        for (int d = 0; d< max_aa; d++) {
            int i;
            for (i=0; i<3; i++) {
                if (dist->patd[i] & (1<<d)) break;
            }
            printf ("%c%i ", translator->Index2Spro(d), i);
        }
        printf ("\n");
    }
}
#endif // DEBUG

// ----------------------------
//      AWT_distance_meter

AWT_distance_meter::AWT_distance_meter(const AWT_translator *translator) {
    memset(dist_, 0, sizeof(dist_));
    memset(transform07, 0, sizeof(transform07));
    memset(transform815, 0, sizeof(transform815));
    memset(transform1623, 0, sizeof(transform1623));

    int s;
    int i;
    int max_aa     = translator->MaxAA();
    int realmax_aa = translator->RealmaxAA();

    for (s = 0; s< max_aa; s++) {
        dist_[s] = (AWT_PDP *)calloc(sizeof(AWT_PDP), max_aa);

        const arb_r2a_pro_2_nuc *s2str = translator->S2str(translator->Index2Spro(s));
        for (i=0; i<3; i++) {
            dist_[s]->nucbits[i] = s2str->nucs->nucbits[i];
        }
    }

    for (s = 0; s< max_aa; s++) {
        for (int d = 0; d< realmax_aa; d++) {
            int dist = nuc_dist(translator, translator->Index2Spro(s), translator->Index2Spro(d));

            if (dist==0) dist_[s]->patd[0] |= 1<<d; // distance == 0
            if (dist<=1) dist_[s]->patd[1] |= 1<<d; // distance <= 1
        }
        dist_[s]->patd[2] |= dist_[s]->patd[1]; // (distance <= 1) => (distance <= 2)
        dist_[s]->patd[0] |= 1<<s; // set 'distance to self' == 0
    }

    for (s = 0; s< max_aa; s++) {
        long sum = 0;
        for (int d = 0; d< realmax_aa; d++) {
            if ((1 << d) & dist_[s]->patd[1]) {   // if distance(s, d) <= 1
                sum |= dist_[d]->patd[1]; // collect all proteins which have 'distance <= 1' to 'd'
            }
        }
        dist_[s]->patd[2] |= sum; // and store them in 'distance <= 2'
    }

    for (i=0; i<256; i++) {
        for (s = 0; s<8; s++) {
            if (i & (1<<s)) {
                transform07[i]   |= dist_[s]->patd[1];
                transform815[i]  |= dist_[s+8]->patd[1];
                transform1623[i] |= dist_[s+16]->patd[1];
            }
        }
    }
#ifdef DEBUG
    awt_pro_a_nucs_debug(translator, this);
#endif
}

AWT_distance_meter::~AWT_distance_meter() {
    for (int i=0; i<64; i++) {
        delete dist_[i];
    }

}

// --------------------------------------------------------------------------------
// Translator factory:

static int current_user_code_nr = -1;                // always contain same value as AWAR_PROTEIN_TYPE (after calling AWT_default_protein_type once)

static void user_code_nr_changed_cb(GBDATA *gb_awar) {
    // this callback keeps 'current_user_code_nr' synced with AWAR_PROTEIN_TYPE
    GBDATA         *gb_main = GB_get_root(gb_awar);
    GB_transaction  ta(gb_main);
    current_user_code_nr    = GB_read_int(gb_awar);
}

#define CACHED_TRANSLATORS 4

#if defined(DEBUG)
// #define DUMP_TRANSLATOR_ALLOC
#endif // DEBUG


AWT_translator *AWT_get_translator(int code_nr) {
    static AWT_translator *cached[CACHED_TRANSLATORS];

    if (!cached[0] || cached[0]->CodeNr() != code_nr) { // most recent != requested
        AWT_translator *translator = 0;
        int             i;

        for (i = 1; i<CACHED_TRANSLATORS; i++) {
            if (cached[i] && cached[i]->CodeNr() == code_nr) {
                // found existing translator
                translator = cached[i];
                cached[i]  = 0;
                break;
            }
        }

        if (!translator) {
            translator = new AWT_translator(code_nr);

#if defined(DUMP_TRANSLATOR_ALLOC)
            static int allocCount = 0;
            allocCount++;
            printf("Alloc translator for code_nr=%i (allocCount=%i)\n", translator->CodeNr(), allocCount);
#endif // DUMP_TRANSLATOR_ALLOC

        }

        // insert new or found translator at front and shift existing to higher indices:
        for (i = 0; i<CACHED_TRANSLATORS && translator; i++) {
            AWT_translator *move = cached[i];
            cached[i]  = translator;
            translator = move;
        }

        // delete oldest translator,  if no empty array position was found:
        if (translator) {
#if defined(DUMP_TRANSLATOR_ALLOC)
            static int freeCount = 0;
            freeCount++;
            printf("Free translator for code_nr=%i (freeCount=%i)\n", translator->CodeNr(), freeCount);
#endif // DUMP_TRANSLATOR_ALLOC

            delete translator;
        }
    }

    pn_assert(cached[0]->CodeNr() == code_nr);
    return cached[0];
}

int AWT_default_protein_type(GBDATA *gb_main) {
    // returns protein code selected in AWAR_PROTEIN_TYPE

    if (current_user_code_nr == -1) { // user protein code in AWAR_PROTEIN_TYPE not traced yet
        pn_assert(gb_main != 0); // either pass gb_main here or call once with valid gb_main at program startup

        {
            GB_transaction ta(gb_main);
            GBDATA *awar = GB_search(gb_main, AWAR_PROTEIN_TYPE, GB_INT);
            GB_add_callback(awar, GB_CB_CHANGED, makeDatabaseCallback(user_code_nr_changed_cb)); // bind a callback that traces AWAR_PROTEIN_TYPE
            user_code_nr_changed_cb(awar);
        }

        pn_assert(current_user_code_nr != -1);
    }

    return current_user_code_nr;
}

AWT_translator *AWT_get_user_translator(GBDATA *gb_main) {
    return AWT_get_translator(AWT_default_protein_type(gb_main));
}

