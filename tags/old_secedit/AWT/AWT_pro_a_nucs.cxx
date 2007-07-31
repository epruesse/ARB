#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
// #include <malloc.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt_pro_a_nucs.hxx>
#include <awt_codon_table.hxx>

struct arb_r2a_struct *awt_pro_a_nucs = 0;

char *AP_create_dna_to_ap_bases(void){
    int i;
    AP_BASES    val;
    char *table = new char[256];
    for (i=0;i<256;i++) {
        switch ((char)i) {
            case 'a':
            case 'A':
                val = AP_A;
                break;
            case 'g':
            case 'G':
                val = AP_G;
                break;
            case 'c':
            case 'C':
                val = AP_C;
                break;
            case 't':
            case 'T':
            case 'u':
            case 'U':
                val = AP_T;
                break;
            case 'n':
            case 'N':
                val = (AP_BASES)(AP_A + AP_G + AP_C + AP_T);
                break;
            case '?':
            case '.':
                val = (AP_BASES)(AP_A + AP_G + AP_C + AP_T + AP_S);
                break;
            case '-':
                val = AP_S;
                break;
            case 'm':
            case 'M':
                val = (AP_BASES)(AP_A+AP_C);
                break;
            case 'r':
            case 'R':
                val = (AP_BASES)(AP_A+AP_G);
                break;
            case 'w':
            case 'W':
                val = (AP_BASES)(AP_A+AP_T);
                break;
            case 's':
            case 'S':
                val = (AP_BASES)(AP_C+AP_G);
                break;
            case 'y':
            case 'Y':
                val = (AP_BASES)(AP_C+AP_T);
                break;
            case 'k':
            case 'K':
                val = (AP_BASES)(AP_G+AP_T);
                break;
            case 'v':
            case 'V':
                val = (AP_BASES)(AP_A+AP_C+AP_G);
                break;
            case 'h':
            case 'H':
                val = (AP_BASES)(AP_A+AP_C+AP_T);
                break;
            case 'd':
            case 'D':
                val = (AP_BASES)(AP_A+AP_G+AP_T);
                break;
            case 'b':
            case 'B':
                val = (AP_BASES)(AP_C+AP_G+AP_T);
                break;
            default:
                val = AP_N;
                break;
        }
        table[i] = (char)val;
    } /*for*/
    return table;
}


long *awt_nuc_create_pro_to_bits(void){
    int i;
    int j;
    long *table = (long *)GB_calloc(sizeof(long),256);
    for ( i = 0 ; i < awt_pro_a_nucs->max_aa; i++) {

        j = awt_pro_a_nucs->index_2_spro[i];
        if (j== '.')  {
            table[i] = -1;
            continue;
        }
        j = awt_pro_a_nucs->s2str[j]->index;
        table[i] = 1<<j;
    }
    return table;
}

static void awt_pro_a_nucs_build_table(unsigned char pbase, const char *tri_pro, const char *nuc) {
    struct arb_r2a_pro_2_nuc *str;
    struct arb_r2a_pro_2_nucs *nucs;
    // search for existing protein, else generate new
    if ( !(str = awt_pro_a_nucs->s2str[pbase] )) {
        //         str                                      = (struct arb_r2a_pro_2_nuc *) GB_calloc(sizeof(struct arb_r2a_pro_2_nuc),1);
        str                                      = new arb_r2a_pro_2_nuc;
        awt_pro_a_nucs->s2str[pbase]             = str;
        awt_pro_a_nucs->s2str[tolower(pbase)]    = str;
        str->index                               = awt_pro_a_nucs->max_aa++;
        str->single_pro                          = pbase;
        str->tri_pro[0]                          = tri_pro[0];
        str->tri_pro[1]                          = tri_pro[1];
        str->tri_pro[2]                          = tri_pro[2];
        awt_pro_a_nucs->index_2_spro[str->index] = pbase;
    }
    // fast hash table
    GBS_write_hash(awt_pro_a_nucs->t2i_hash, nuc, pbase);

    int n0 = awt_pro_a_nucs->nuc_2_bitset[nuc[0]];
    int n1 = awt_pro_a_nucs->nuc_2_bitset[nuc[1]];
    int n2 = awt_pro_a_nucs->nuc_2_bitset[nuc[2]];

    for (nucs = str->nucs; nucs; nucs = nucs->next) {
        if (    (!(nucs->nucbits[0] & ~n0)) &&      // search superset
                (!(nucs->nucbits[1] & ~n1)) &&
                (!(nucs->nucbits[2] & ~n2)) ) break;
        int c = 0;
        if ( nucs->nucbits[0] != n0 ) c++;
        if ( nucs->nucbits[1] != n1 ) c++;
        if ( nucs->nucbits[2] != n2 ) c++;
        if (c <= 1) break;
    }
    if (!nucs) {
        //         nucs       = (struct arb_r2a_pro_2_nucs *)GB_calloc(sizeof(struct arb_r2a_pro_2_nucs),1);
        nucs       = new arb_r2a_pro_2_nucs;
        nucs->next = str->nucs;
        str->nucs  = nucs;
    }
    nucs->nucbits[0] |= n0;
    nucs->nucbits[1] |= n1;
    nucs->nucbits[2] |= n2;
}

arb_r2a_pro_2_nucs::arb_r2a_pro_2_nucs()
    : next(0)
{
    memset(nucbits, 0, sizeof(nucbits));
}
arb_r2a_pro_2_nucs::~arb_r2a_pro_2_nucs() {
    delete next;
}

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

arb_r2a_struct::arb_r2a_struct()
    : t2i_hash(0)
    , time_stamp(0)
    , pro_2_bitset(0)
    , nuc_2_bitset(0)
    , realmax_aa(0)
    , max_aa(0)
{
    memset(s2str, 0, sizeof(s2str));
    memset(index_2_spro, 0, sizeof(index_2_spro));
    memset(dist, 0, sizeof(dist));
    memset(transform07, 0, sizeof(transform07));
    memset(transform815, 0, sizeof(transform815));
    memset(transform1623, 0, sizeof(transform1623));
}

arb_r2a_struct::~arb_r2a_struct() {
    free(pro_2_bitset);

    delete [] nuc_2_bitset;
    GBS_free_hash(t2i_hash);
    int i;
    for (i=0;i<64; i++){
        delete dist[i];
    }
    for (i=0;i<256;i++){
        if (i!= tolower(i)) continue; // do not delete duplicated entries
        delete s2str[i];
    }
}

static int codon_defined_in(const char *codon, const char *codons) {
    for (int off=0; codons[off]; off+=3) {
        if (codon[0]==codons[off] && codon[1]==codons[off+1] && codon[2]==codons[off+2]) {
            return 1;
        }
    }
    return 0;
}

// order of table generated by awt_pro_a_nucs_convert_init is important:
// must be compatible with DIST/PH_protdist.cxx !!
// except this table has an 's' insertion !!!

#define T(a,b,c)    awt_pro_a_nucs_build_table(a,b,c)

static int awt_pro_a_nucs_convert_init_time_stamp = 0; // counts how often awt_pro_a_nucs_convert_init has been called

void awt_pro_a_nucs_convert_init(GBDATA *gb_main)
{
    GB_transaction dummy(gb_main);
    gb_main = GB_get_root(gb_main);

    int        code_nr                 = GBT_read_int(gb_main,AWAR_PROTEIN_TYPE);
    static int initialized_for_code_nr = -1;

    if (!awt_pro_a_nucs){
        GBDATA *awar = GB_search(gb_main,AWAR_PROTEIN_TYPE,GB_INT);
        GB_add_callback(awar,GB_CB_CHANGED,(GB_CB)awt_pro_a_nucs_convert_init,0);
    }else{
        if (code_nr == initialized_for_code_nr) return; // already initialized correctly

        delete awt_pro_a_nucs;
        awt_pro_a_nucs = 0;
    }
//     awt_pro_a_nucs               = (struct arb_r2a_struct *)GB_calloc(sizeof(struct arb_r2a_struct),1);
    awt_pro_a_nucs               = new arb_r2a_struct;
    awt_pro_a_nucs->time_stamp   = ++awt_pro_a_nucs_convert_init_time_stamp;
    awt_pro_a_nucs->nuc_2_bitset = AP_create_dna_to_ap_bases();
    awt_pro_a_nucs->t2i_hash     = GBS_create_hash(1024,1); // case insensitive

    AWT_initialize_codon_tables();

    {
        char *D_codons = GB_strdup(AWT_get_codons('D', code_nr));
        char *N_codons = GB_strdup(AWT_get_codons('N', code_nr));
        char *E_codons = GB_strdup(AWT_get_codons('E', code_nr));
        char *Q_codons = GB_strdup(AWT_get_codons('Q', code_nr));

        char protein;
        for (protein='*'; protein<='Z'; protein = (protein=='*' ? 'A' : protein+1)) {
            if (protein!='J' && protein!='O' && protein!='U') { // JOU are no aminos
                const char *codons;
                if (protein=='D')   codons = D_codons;
                else if (protein=='N')  codons = N_codons;
                else if (protein=='E')  codons = E_codons;
                else if (protein=='Q')  codons = Q_codons;
                else            codons = AWT_get_codons(protein, code_nr);
                // codons now contains a 0-terminated-string containing all possible codons for protein

                const char *protein_name = AWT_get_protein_name(protein);

                for (int off=0; codons[off]; off+=3) {
                    char codon[4];
                    memcpy(codon, codons+off, 3);
                    codon[3] = 0;

                    if (protein=='B') {
                        if (!codon_defined_in(codon, D_codons) && !codon_defined_in(codon, N_codons)) {
                            awt_pro_a_nucs_build_table(protein, protein_name, codon);
                        }
                    }
                    else if (protein=='Z') {
                        if (!codon_defined_in(codon, E_codons) && !codon_defined_in(codon, Q_codons)) {
                            awt_pro_a_nucs_build_table(protein, protein_name, codon);
                        }
                    }
                    else {
                        awt_pro_a_nucs_build_table(protein, protein_name, codon);
                    }
                }
            }
        }

        free(Q_codons);
        free(E_codons);
        free(N_codons);
        free(D_codons);
    }

    awt_pro_a_nucs->realmax_aa = awt_pro_a_nucs->max_aa;

    T('-', "---", "---");
    T('.', "...", "...");
    T('.', "???", "???");
    T('X', "NNN", "NNN");


    /*
      T("A","Ala","GCT");
      T("A","Ala","GCC");
      T("A","Ala","GCA");
      T("A","Ala","GCG");
      T("A","Ala","GCX");
      T("R","Arg","CGT");
      T("R","Arg","CGC");
      T("R","Arg","CGA");
      T("R","Arg","CGG");               T("R","Arg","CGX");

      T("R","Arg","AGA");
      T("R","Arg","AGG");
      T("R","Arg","AGR");
      T("R","Arg","MGR");
      T("N","Asn","AAT");
      T("N","Asn","AAC");           T("N","Asn","AAY");

      T("D","Asp","GAT");
      T("D","Asp","GAC");           T("D","Asp","GAY");

      T("C","Cys","TGT");
      T("C","Cys","TGC");           T("C","Cys","TGY");

      T("Q","Gln","CAA");
      if ( code_nr == AP_YEASTMITO ) {
      }else{
      T("Q","Gln","CAG");           T("Q","Gln","CAR");
      }

      T("E","Glu","GAA");
      if (code_nr == AP_MITO ||
      code_nr == AP_VERTMITO ||
      code_nr == AP_FLYMITO ||
      code_nr == AP_YEASTMITO){
      }else{
      T("E","Glu","GAG");
      T("E","Glu","GAR");
      }
      T("G","Gly","GGT");
      T("G","Gly","GGC");
      T("G","Gly","GGA");
      T("G","Gly","GGG");
      T("G","Gly","GGX");
      T("H","His","CAT");
      T("H","His","CAC");
      T("H","His","CAY");
      T("I","Ile","ATT");
      T("I","Ile","ATC");
      T("I","Ile","ATA");
      T("I","Ile","ATH");
      T("L","Leu","TTA");
      T("L","Leu","TTG");       T("L","Leu","TTR");
      T("L","Leu","CTT");
      T("L","Leu","CTC");
      T("L","Leu","CTA");
      T("L","Leu","CTG");       T("L","Leu","CTX");
      T("L","Leu","YTR");

      T("K","Lys","AAA");
      T("K","Lys","AAG");       T("K","Lys","AAR");
      if (code_nr == AP_MITO ||
      code_nr == AP_VERTMITO ||
      code_nr == AP_FLYMITO ||
      code_nr == AP_YEASTMITO){
      T("M","Met","GAG");   // @@@ That is wrong for AP_MITO but we need Met
      }else{
      T("M","Met","ATG");
      }

      T("F","Phe","TTT");
      T("F","Phe","TTC");           T("F","Phe","TTY");

      T("P","Pro","CCT");
      T("P","Pro","CCC");
      T("P","Pro","CCA");
      T("P","Pro","CCG");           T("P","Pro","CCX");

      T("s","Ser","TCT");       // Special insertion, why?? see phylip
      T("s","Ser","TCC");
      T("s","Ser","TCG");
      T("s","Ser","TCA");           T("s","Ser","TCX");

      T("S","Ser","AGT");
      T("S","Ser","AGC");           T("S","Ser","AGY");

      T("T","Thr","ACT");
      T("T","Thr","ACC");
      T("T","Thr","ACA");
      T("T","Thr","ACG");           T("T","Thr","ACX");

      if (  code_nr == AP_YEASTMITO ){
      T("T","Thr","CAG");
      }

      T("W","Trp","TGG");
      if (code_nr == AP_MITO ||
      code_nr == AP_VERTMITO ||
      code_nr == AP_FLYMITO ||
      code_nr == AP_YEASTMITO){
      T("W","Trp","ATG");
      }

      T("Y","Tyr","TAT");
      T("Y","Tyr","TAC");           T("Y","Tyr","TAY");


      T("V","Val","GTA");
      T("V","Val","GTC");
      if (  code_nr == AP_VERTMITO ||
      code_nr == AP_FLYMITO ){
      if ( code_nr == AP_VERTMITO ){
      T("V","Val","GTY");
      }else{
      T("V","Val","GTT");       T("V","Val","GTH");
      }
      }else{
      T("V","Val","GTT");
      T("V","Val","GTG");           T("V","Val","GTX");
      }

      T("*","End","TAA");
      T("*","End","TAG");
      T("*","End","TGA");           T("*","End","TAR");
      T("*","End","TRA");
      if (  code_nr == AP_VERTMITO ||
      code_nr == AP_FLYMITO ){
      T("*","End","GTG");
      }

      if (  code_nr == AP_VERTMITO ){
      T("*","End","GTT");
      }

      awt_pro_a_nucs->realmax_aa = awt_pro_a_nucs->max_aa;
      T("-","---","---");


      // T("B","Asx","AAC");
      // T("B","Asx","AAT");
      // T("B","Asx","GAC");
      // T("B","Asx","GAT");
      // T("B","Asx","AAY");
      // T("B","Asx","GAY");
      T("B","Asx","RAC");
      T("B","Asx","RAT");
      T("B","Asx","RAY");
      T("Z","Glx","SAR");

      T(".","...","...");
      T(".","???","???");
      T(".","NNN","NNN");
    */

    awt_pro_a_nucs->pro_2_bitset = awt_nuc_create_pro_to_bits();
    initialized_for_code_nr      = code_nr;
}

static int AP_nuc_dist(unsigned char p1, unsigned char p2) {  // nucleotid changes between two aa
    struct arb_r2a_pro_2_nuc *s1,*s2;
    s1 = awt_pro_a_nucs->s2str[p1];
    s2 = awt_pro_a_nucs->s2str[p2];
    if ( (!s1) || (!s2) ) return -1;
    struct arb_r2a_pro_2_nucs *n1,*n2;
    long mindist = 3;
    // Check all combinations, if any combination is valid -> zero distance
    for (   n1 = s1->nucs; n1; n1=n1->next){
        for (   n2 = s2->nucs; n2; n2=n2->next){
            int dist = 0;
            int i;
            for (i=0;i<3;i++) {
                if ( n1->nucbits[i] & n2->nucbits[i] ) continue;
                dist++;
            }
            if (dist< mindist) mindist = dist;
        }
    }
    return mindist;
}

void awt_pro_a_nucs_debug(void) {
    int s;
    int d;
    int i;
    for (s = 0; s< awt_pro_a_nucs->max_aa; s++){
        // check bits should not be present in distpad
        if (s<awt_pro_a_nucs->realmax_aa) {
            for (i=0;i<2;i++) {
                // assertion: bit is set in patd[i] => bit is clear in patd[i+1]
                if ( awt_pro_a_nucs->dist[s]->patd[i] & ~awt_pro_a_nucs->dist[s]->patd[i+1]) {
                    GB_CORE;
                }
            }
        }
        printf("Base %c[%i]: Dist to ",awt_pro_a_nucs->index_2_spro[s],s);
        for (d = 0; d< awt_pro_a_nucs->max_aa; d++){
            for (i=0;i<3;i++) {
                if (awt_pro_a_nucs->dist[s]->patd[i] & (1<<d)) break;
            }
            printf ("%c%i ",awt_pro_a_nucs->index_2_spro[d],i);
        }
        printf ("\n");
    }
}


void awt_pro_a_nucs_gen_dist(GBDATA *gb_main){
    int  dist;
    int  s;
    long sum;
    int  d;
    int  i;

    awt_pro_a_nucs_convert_init(gb_main);

    for (s = 0; s< awt_pro_a_nucs->max_aa; s++) {
        awt_pro_a_nucs->dist[s] = (AWT_PDP *)calloc(sizeof(AWT_PDP), awt_pro_a_nucs->max_aa);
        for (i=0;i<3;i++) {
            awt_pro_a_nucs->dist[s]->nucbits[i] = awt_pro_a_nucs->s2str[awt_pro_a_nucs->index_2_spro[s]]->nucs->nucbits[i];
        }
    }

    for (s = 0; s< awt_pro_a_nucs->max_aa; s++){
        for (d = 0; d< awt_pro_a_nucs->realmax_aa; d++){
            dist = AP_nuc_dist(awt_pro_a_nucs->index_2_spro[s], awt_pro_a_nucs->index_2_spro[d] );

            if (dist==0) awt_pro_a_nucs->dist[s]->patd[0] |= 1<<d; // distance == 0
            if (dist<=1) awt_pro_a_nucs->dist[s]->patd[1] |= 1<<d; // distance <= 1
        }
        awt_pro_a_nucs->dist[s]->patd[2] |= awt_pro_a_nucs->dist[s]->patd[1]; // (distance <= 1) => (distance <= 2)
        awt_pro_a_nucs->dist[s]->patd[0] |= 1<<s; // set 'distance to self' == 0
    }

    for (s = 0; s< awt_pro_a_nucs->max_aa; s++) {
        sum = 0;
        for (d = 0; d< awt_pro_a_nucs->realmax_aa; d++) {
            if ( (1 << d) & awt_pro_a_nucs->dist[s]->patd[1] ) { // if distance(s, d) <= 1
                sum |= awt_pro_a_nucs->dist[d]->patd[1]; // collect all proteins which have 'distance <= 1' to 'd'
            }
        }
        awt_pro_a_nucs->dist[s]->patd[2] |= sum; // and store them in 'distance <= 2'
    }

    for (i=0; i<256; i++) {
        for (s = 0; s<8; s++) {
            if (i & (1<<s) ) {
                awt_pro_a_nucs->transform07[i]   |= awt_pro_a_nucs->dist[s]->patd[1];
                awt_pro_a_nucs->transform815[i]  |= awt_pro_a_nucs->dist[s+8]->patd[1];
                awt_pro_a_nucs->transform1623[i] |= awt_pro_a_nucs->dist[s+16]->patd[1];
            }
        }
    }
#ifdef DEBUG
    awt_pro_a_nucs_debug();
#endif
}
