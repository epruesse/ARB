// =============================================================== //
//                                                                 //
//   File      : MultiProbe.hxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef MULTIPROBE_HXX
#define MULTIPROBE_HXX

#ifndef SOTL_HXX
#include "SoTl.hxx"
#endif
#ifndef MPDEFS_H
#include "mpdefs.h"
#endif
#ifndef AW_AWAR_HXX
#include <aw_awar.hxx>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define mp_assert(x) arb_assert(x)

class MP_Main;
class MP_Window;
class AW_window;
class AW_root;
class AW_window_simple;
class AWT_canvas;
class ProbeValuation;

struct ST_Container;
class  Bitvector;
class  Sonde;

#define MP_AWAR_SEQIN               "tmp/mp/seqin"
#define MP_AWAR_SELECTEDPROBES      "mp/selectedprobes"
#define MP_AWAR_PROBELIST           "mp/probelist"
#define MP_AWAR_WEIGHTEDMISMATCHES  "mp/weightedmismatches"
#define MP_AWAR_COMPLEMENT          "mp/complement"
#define MP_AWAR_MISMATCHES          "mp/mismatches"
#define MP_AWAR_PTSERVER            AWAR_PT_SERVER
#define MP_AWAR_LOADLIST            "mp/loadlist"
#define MP_AWAR_RESULTPROBES        "mp/resultprobes"
#define MP_AWAR_RESULTPROBESCOMMENT "mp/resultprobescomment"
#define MP_AWAR_NOOFPROBES          "mp/noofprobes"
#define MP_AWAR_QUALITY             "mp/quality"
#define MP_AWAR_SINGLEMISMATCHES    "mp/singlemismatches"
#define MP_AWAR_OUTSIDEMISMATCHES   "mp/outsidemismatches"
#define MP_AWAR_QUALITYBORDER1      "mp/qualityborder1"
#define MP_AWAR_GREYZONE            "mp/greyzone"
#define MP_AWAR_EMPHASIS            "mp/emphasis"
#define MP_AWAR_ECOLIPOS            "mp/ecolipos"
#define MP_AWAR_AUTOADVANCE         "mp/auto_advance"

#define QUALITYDEFAULT     3
#define MAXPROBECOMBIS     6    // Maximale Anzahl der Sondenkombinationen
#define MAXMISMATCHES      6    // von 0 bis 5 !!!!
#define FITNESSSCALEFACTOR 10   // wird benutzt um intern die hammingtabelle feiner zu granulieren
// siehe probe_combi_statistic::calc_fitness

#define MULTROULETTEFACTOR    10    // macht aus z.B. 4,231 42
#define MAXINITPOPULATION     50    // Ausgangsgroesse der Population
#define MAXPOPULATION         MAXINITPOPULATION
#define CROSSOVER_WS          60    // Crossoverwahrscheinlichkeit liegt bei CROSSOVER_WS Prozent !!!
#define MUTATION_WS           33    // Mutationswahrscheinlichkeit liegt bei 1/MUTATION_WS Prozent!!!
#define SIGMATRUNCATION_CONST 2     // die Standardwerte liegen zwischen 1 und 3
#define C_MULT                1.4   // Werte zwischen 1.2 und 2.0

// #define USE_SIGMATRUNCATION       // definieren, wenn sigma_truncation in der linearen Skal. (=fitness skalierung) verwendet werden soll
// #define USE_LINEARSCALING         // definieren, wenn lineare_skalierung (=fitness skalierung) verwendet werden soll
// #define USE_DUP_TREE              // definieren, wenn eine Sondenkombination nur einmal pro generation vorkommen soll

#define LIST(TYP) ((List<Sonde>*) TYP)

#define MAXSONDENHASHSIZE   1000        // max 1000 Sonden koennen gecached werden, bei Bedarf aendern !!!!


struct awar_vars {
    char    *selected_probes;
    char    *probelist;
    char    *result_probes;
    char    *result_probes_comment;
    float   outside_mismatches_difference;
    long    weightedmismatches;
    long    complement;
    long    no_of_mismatches;
    long    no_of_probes;
    long    singlemismatches;
    long    ptserver;
    long    probe_quality;
    long    qualityborder_best;
    long    emphasis;
    long    ecolipos;
    float   min_weight;
    float   max_weight;
    float   greyzone;
};

class AW_selection_list;

extern AW_selection_list  *selected_list;
extern AW_selection_list  *probelist;
extern char                MP_probe_tab[256];
extern AW_selection_list  *result_probes_list;
extern int                 remembered_mismatches;
extern int                 anz_elem_marked;
extern int                 anz_elem_unmarked;
extern unsigned char     **hamming_tab;
extern int               **system3_tab;
extern bool                pt_server_different;
extern bool                new_pt_server;

long            k_aus_n(int k, int n);                      // Berechnung k aus n
extern int      get_random(int min, int max);       // gibt eine Zufallszahl x mit der Eigenschaft : min <= x <= max

// ********************************************************

extern MP_Main   *mp_main;
extern awar_vars  mp_gl_awars;                      // globale Variable, die manuell eingegebene Sequenz enthaelt

// ********************************************************



class MP_Main : virtual Noncopyable {
    MP_Window      *mp_window;
    AW_root        *aw_root;
    AWT_canvas     *scr;
    ST_Container   *stc;
    ProbeValuation *p_eval;

    void    create_awars();

public:
    MP_Window   *get_mp_window()    { return mp_window; };
    AW_root *get_aw_root()      { return aw_root; };
    AWT_canvas  *get_canvas()      { return scr; };
    ProbeValuation *get_p_eval()    { return p_eval; };
    ST_Container *get_stc()         { return stc; };
    void    set_stc(ST_Container *stopfC) { stc = stopfC; }
    void    set_p_eval (ProbeValuation *y) { p_eval = y; };
    ProbeValuation  *new_probe_eval(char **field, int size, int* array, int *mismatches);
    void        destroy_probe_eval();

    MP_Main(AW_root *awr, AWT_canvas *canvas);
    ~MP_Main();
};



class MP_Window : virtual Noncopyable {
    AW_window_simple *aws;
    AW_window_simple *result_window;

public:
    AW_window_simple    *get_window()       { return aws; };
    AW_window_simple    *get_result_window()    { return result_window; };

    AW_window_simple *create_result_window(AW_root *aw_root);

    MP_Window(AW_root *aw_root, GBDATA *gb_main);
    ~MP_Window();
};



// *****************************************************
// Globale Klassenlose Funktionen
// *****************************************************
void MP_compute(AW_window *, AW_CL cl_gb_main);


class Bakt_Info : virtual Noncopyable {
    char* name;
    long  hit_flag;

public:
    char*       get_name() { return name; };
    long        inc_hit_flag() { return (++hit_flag); };
    long        get_hit_flag() { return (hit_flag); };
    void        kill_flag() { hit_flag = 0; };

    Bakt_Info(const char* n);
    ~Bakt_Info();
};

class Hit : virtual Noncopyable {
    double  *mismatch;
    long    baktid;

public:
    double*     get_mismatch()  { return mismatch; };
    long    get_baktid()    { return baktid; };
    void    set_mismatch_at_pos(int pos, double mm) { mismatch[pos] = mm; };
    double  get_mismatch_at_pos(int pos) { return mismatch[pos]; };

    Hit(long baktnummer);
    ~Hit();
};


class probe_tabs;

class Sondentopf : virtual Noncopyable {
    List<void*>     *Listenliste; // @@@ change type to List<Sonde> ? 
    GB_HASH     *color_hash;
    MO_Liste        *BaktList;
    MO_Liste        *Auswahllist;

public:
    probe_tabs*     fill_Stat_Arrays();
    double**        gen_Mergefeld();
    void        put_Sonde(char *name, int allowed_mis, double outside_mis);
    long        get_length_hitliste();
    void        gen_color_hash(positiontype anz_sonden);
    GB_HASH         *get_color_hash() { return color_hash; };

    Sondentopf(MO_Liste *BL, MO_Liste *AL);
    ~Sondentopf();
};

class Sonde : virtual Noncopyable {
    char*       kennung;        // Nukleinsaeuren, z.B. "atgatgatg"
    Bitvector*      bitkennung;     // Sonde 1 Platz eins, ...zwei..., ... Analog zum Aufbau der Listenliste
    Hit         **hitliste;     // start bei index 0, letztes element enthaelt NULL
    long        length_hitliste;
    long        minelem;
    long        maxelem;
    positiontype    kombi_far, kombi_mor;
    long        *Allowed_Mismatch;
    double      *Outside_Mismatch;

public:
    double              get_Allowed_Mismatch_no(int no) { return ((Allowed_Mismatch) ? Allowed_Mismatch[no] : 100); };
    double              get_Outside_Mismatch_no(int no) { return ((Outside_Mismatch) ? Outside_Mismatch[no] : 100); };
    char*       get_name() { return kennung; };
    Hit*        get_hitdata_by_number(long index);
    Hit**       get_Hitliste() {    return hitliste; };
    long        get_length_hitliste() {    return length_hitliste; };
    long        get_minelem() { return minelem; };
    long        get_maxelem() { return maxelem; };
    positiontype    get_far() { return kombi_far; };
    positiontype    get_mor() { return kombi_mor; };
    Bitvector*      get_bitkennung() { return bitkennung; };

    void        set_Allowed_Mismatch_no(int pos, int no) { Allowed_Mismatch[pos] = no; };
    void        set_Outside_Mismatch_no(int pos, int no) { Outside_Mismatch[pos] = no; };
    void        set_bitkennung(Bitvector* bv);  // Setzt eine Leere Bitkennung der laenge bits
    void        set_name(char* name) {  kennung = strdup(name); };
    void        set_Hitliste(Hit** hitptr) {     hitliste = hitptr; };
    void        set_length_hitliste(long lhl) { length_hitliste = lhl; };
    void        set_minelem(long min) { minelem = min; };
    void        set_maxelem(long max) { maxelem = max; };
    void        set_far(positiontype far) {  kombi_far = far; };
    void        set_mor(positiontype mor) {  kombi_mor = mor; };

    void        print();
    void        sink(long i, long t, MO_Mismatch** A);
    void        heapsort(long feldlaenge, MO_Mismatch** Nr_Mm_Feld);
    double      check_for_min(long k, MO_Mismatch** probebacts, long laenge);

    MO_Mismatch**   get_matching_species(bool match_kompl,
                                         int match_weight,
                                         int match_mis,
                                         char *match_seq,
                                         MO_Liste *convert,
                                         long *number_of_species);
    int         gen_Hitliste(MO_Liste *Bakterienliste);

    Sonde(char* bezeichner, int allowed_mis, double outside_mis);
    ~Sonde();
};
// ##################################### Sondentopf Container  ###########################

struct ST_Container : virtual Noncopyable {
    MO_Liste        *Bakterienliste;
    MO_Liste        *Auswahlliste;
    Sondentopf      *sondentopf; // Wird einmal eine Sondentopfliste
    List<Sondentopf>    *ST_Liste;
    int         anzahl_basissonden;
    GB_HASH*        cachehash;
    List<char>      *Sondennamen;

    Sonde*      cache_Sonde(char *name, int allowed_mis, double outside_mis);
    Sonde*      get_cached_sonde(char* name);
    ST_Container(int anz_sonden);
    ~ST_Container();
};

// ##################################### MO_Liste #####################################
/* Die Namen werden in die Hashtabelle mit fortlaufender Nummer abgespeichert
 * Die Nummer bezeichnet das Feld in einem Array, indem momentan der B.-Name steht,
 * spaeter
 * aber auch z.B. ein Pointer auf eine Klasse mit mehr Informationen stehen kann
 ACHTUNG:: Es kann passieren, dass eine Sonde mehrmals an einen Bakter bindet. Dann wird der Bakter nur einmal in die
 Liste eingetragen, und damit ist die laenge nicht mehr aussagekraeftig. current-1 enthaelt das letzte element der liste, die
 Liste beginnt bei 1
*/

class MO_Liste : virtual Noncopyable {
    Bakt_Info** mo_liste;
    long        laenge;
    long        current;                            // zeigt auf den ersten freien eintrag
    GB_HASH*    hashptr;

    static GBDATA *gb_main;

public:
    positiontype fill_marked_bakts();
    void         get_all_species();
    long         debug_get_current();
    long         get_laenge();
    long         put_entry(const char* name);
    char*        get_entry_by_index(long index);
    long         get_index_by_entry(const char* key);
    Bakt_Info*   get_bakt_info_by_index(long index);

    Bakt_Info** get_mo_liste() { return mo_liste; }

    static void set_gb_main(GBDATA *gb_main_) {
        mp_assert(implicated(gb_main, gb_main == gb_main_));
        gb_main = gb_main_;
    }

    MO_Liste();
    ~MO_Liste();
};


class Bitvector : virtual Noncopyable {
    // Bitpositionen sind 0 bis 7 
    char*   vector;
    int     len;
    int     num_of_bits;
public:
    int     gen_id();
    Bitvector*  merge(Bitvector* x);
    int     subset(Bitvector* Obermenge);
    int     readbit(int pos);
    int     setbit(int pos);
    int     delbit(int pos);
    void    rshift();
    void    print();

    char*   get_vector() { return vector; };
    int     get_num_of_bits() { return num_of_bits; };
    int     get_len() { return len; };

    void    set_vector(char* back) { vector = back; };

    Bitvector(int bits);
    ~Bitvector();
};

#else
#error MultiProbe.hxx included twice
#endif // MULTIPROBE_HXX
