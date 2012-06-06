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
#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef NUMERICTYPE_H
#include <NumericType.h>
#endif


#define mp_assert(x) arb_assert(x)

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

// ********************************************************

class AWT_canvas;

class MP_Window : virtual Noncopyable {
    AW_window *aw;
    AW_window *result_window;

    void build_pt_server_list();
    // zeigt auf naechstes Zeichen

public:
    AW_window *get_window() { return aw; }
    AW_window *get_result_window() { return result_window; }

    AW_window *create_result_window(AW_root *aw_root);

    MP_Window(AW_root *aw_root, GBDATA *gb_main);
    ~MP_Window();
};

class MP_Main : virtual Noncopyable {
    MP_Window  *mp_window;
    AW_root    *aw_root;
    AWT_canvas *scr;

    void    create_awars();

public:
    MP_Window  *get_mp_window() { return mp_window; }
    AW_root    *get_aw_root()   { return aw_root; }
    AWT_canvas *get_canvas()    { return scr; }

    MP_Main(AW_root *awr, AWT_canvas *canvas);
    ~MP_Main();
};

class SpeciesName {
    char* name;

public:
    const char *get_name() const { return name; }

    SpeciesName() : name(NULL) {}
    SpeciesName(const char* n) : name(strdup(n)) {}
    SpeciesName(const SpeciesName& other) : name(strdup(other.get_name())) {}
    DECLARE_ASSIGNMENT_OPERATOR(SpeciesName);
    ~SpeciesName() { free(name); }
};

typedef NumericType<int, 1> SpeciesID;
typedef NumericType<int, 2> TargetID; // also references species, but in different Group

template <typename ID>
class Group : virtual Noncopyable {
    long         size;      // size of array-2 (+1 for unused index [0]; +1 for sentinel)
    SpeciesName *member;    // used indices [1..SIZE]; has sentinel at end of array
    long         elems;     // number of elements in member
    GB_HASH*     hashptr;   // hash (name->index)

    bool is_valid_index(ID id) const { return id > 0 && id <= elems; }

    void allocate(long wantedElements) {
        mp_assert(!member);
        mp_assert(!elems);

        size     = wantedElements;
        member = new SpeciesName[size+2];
        memset(member, 0, (size+2)*sizeof(*member));

        hashptr = GBS_create_hash(size+1, GB_IGNORE_CASE);
    }

public:
    void insert_species(const GB_HASH *species_hash);
    void insert_species_knownBy_PTserver();

    long elements() const { return elems; }

    bool hasEntryAt(SpeciesID id) const { return is_valid_index(id); }
    bool hasEntry(const char *name) const { return get_index_by_name(name); }
    
    void put_entry(const char* name) {
        if (!hasEntry(name)) {
            member[++elems] = SpeciesName(name);
            GBS_write_hash(hashptr, name, elems);
        }
    }

    const char *get_name_by_index(ID id) const { return is_valid_index(id) ? member[id].get_name() : NULL; }
    ID get_index_by_name(const char* key) const { return ID(key ? GBS_read_hash(hashptr, key) : 0); }

    Group()
        : size(0),
          member(NULL),
          elems(0),
          hashptr(NULL)
    {
        // partial ctor - needs to be filled using insert_species() or insert_species_knownBy_PTserver()
    }

    ~Group() {
        delete [] member;
        if (hashptr) GBS_free_hash(hashptr);
    }
};

enum GroupMembership {
    GM_IN_GROUP,
    GM_OUT_GROUP,
    GM_UNKNOWN,
};

class TargetGroup {
    Group<SpeciesID> known;    // all known species
    Group<TargetID>  targeted; // species for which multiprobes get calculated

public:

    TargetGroup(const GB_HASH *targeted_species);

    size_t known_species_count() const { return known.elements(); }
    size_t targeted_species_count() const { return targeted.elements(); }
    size_t outgroup_species_count() const { return known_species_count()-targeted_species_count(); }

    SpeciesID name2id(const char *name) const { return known.get_index_by_name(name); }
    const char *id2name(SpeciesID id) const { return known.get_name_by_index(id); }

    GroupMembership getMembership(SpeciesID id) const {
        return known.hasEntryAt(id)
            ? (targeted.hasEntry(known.get_name_by_index(id))
               ? GM_IN_GROUP
               : GM_OUT_GROUP)
            : GM_UNKNOWN;
    }
};

class Sondentopf : virtual Noncopyable {
    const TargetGroup&  targetGroup;
    GB_HASH            *color_hash;
    List<void*>        *Listenliste; // @@@ change type to List<Sonde> ?

    void gen_color_hash();
    
public:
    class probe_tabs* fill_Stat_Arrays();
    double**          gen_Mergefeld();

    void put_Sonde(char *name, int allowed_mis, double outside_mis, GB_ERROR& error);
    long get_length_hitliste();

    const GB_HASH *get_color_hash() {
        if (!color_hash) gen_color_hash();
        mp_assert(color_hash);
        return color_hash;
    }

    Sondentopf(const TargetGroup& targetGroup_);
    ~Sondentopf();
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

    char*   get_vector() { return vector; }
    int     get_num_of_bits() { return num_of_bits; }
    int     get_len() { return len; }

    void    set_vector(char* back) { vector = back; }

    Bitvector(int bits);
    ~Bitvector();
};

class Hit : virtual Noncopyable {
    double    *mismatch;
    SpeciesID  id;

public:
    double *get_mismatch() { return mismatch; }
    
    SpeciesID target() { return id; }
    void    set_mismatch_at_pos(int pos, double mm) { mismatch[pos] = mm; }
    double  get_mismatch_at_pos(int pos) { return mismatch[pos]; }

    Hit(SpeciesID id_);
    ~Hit();
};

struct MO_Mismatch {
    SpeciesID nummer;
    double    mismatch;

    MO_Mismatch() : nummer(0), mismatch(0.0) {}
};

class Sonde : virtual Noncopyable {
    char*          kennung;         // Nukleinsaeuren, z.B. "atgatgatg"
    Bitvector*     bitkennung;      // Sonde 1 Platz eins, ...zwei..., ... Analog zum Aufbau der Listenliste
    Hit          **hitliste;        // start bei index 0, letztes element enthaelt NULL
    long           length_hitliste;
    SpeciesID      minelem, maxelem;
    positiontype   kombi_far, kombi_mor;
    long          *Allowed_Mismatch;
    double        *Outside_Mismatch;

public:
    double get_Allowed_Mismatch_no(int no) { return ((Allowed_Mismatch) ? Allowed_Mismatch[no] : 100); }
    double get_Outside_Mismatch_no(int no) { return ((Outside_Mismatch) ? Outside_Mismatch[no] : 100); }
    char* get_name() { return kennung; }
    Hit *hit(long index);
    Hit** get_Hitliste() { return hitliste; }
    long get_length_hitliste() { return length_hitliste; }

    SpeciesID get_minelem() { return minelem; }
    SpeciesID get_maxelem() { return maxelem; }

    positiontype get_far() { return kombi_far; }
    positiontype get_mor() { return kombi_mor; }
    Bitvector* get_bitkennung() { return bitkennung; }

    void set_Allowed_Mismatch_no(int pos, int no) { Allowed_Mismatch[pos] = no; }
    void set_Outside_Mismatch_no(int pos, int no) { Outside_Mismatch[pos] = no; }
    void set_bitkennung(Bitvector* bv); // Setzt eine Leere Bitkennung der laenge bits
    void set_name(char* name) { kennung = strdup(name); }
    void set_Hitliste(Hit** hitptr) { hitliste = hitptr; }
    void set_length_hitliste(long lhl) { length_hitliste = lhl; }

    void set_minelem(SpeciesID min) { minelem = min; }
    void set_maxelem(SpeciesID max) { maxelem = max; }

    void set_far(positiontype far) {  kombi_far = far; }
    void set_mor(positiontype mor) {  kombi_mor = mor; }

    void print();
    void sink(long i, long t, MO_Mismatch** A);
    void heapsort(long feldlaenge, MO_Mismatch** Nr_Mm_Feld);
    double check_for_min(long k, MO_Mismatch** probebacts, long laenge);

    MO_Mismatch** get_matching_species(bool match_kompl, int match_weight, int match_mis, char *match_seq, const TargetGroup& targetGroup, int *number_of_species, GB_ERROR& error);
    GB_ERROR gen_Hitliste(const TargetGroup& targetGroup) __ATTR__USERESULT;

    Sonde(char* bezeichner, int allowed_mis, double outside_mis);
    ~Sonde();
};

class ST_Container : virtual Noncopyable {
    // Sondentopf Container

    TargetGroup targetGroup;
public: 
    
    // @@@ make members private
    List<Sondentopf> *ST_Liste;
    int anzahl_basissonden;
    GB_HASH* cachehash;
    List<char> *Sondennamen;

    Sonde* cache_Sonde(char *name, int allowed_mis, double outside_mis, GB_ERROR& error);
    Sonde* get_cached_sonde(char* name);

    const TargetGroup& get_TargetGroup() { return targetGroup; }

    ST_Container(size_t anz_sonden);
    ~ST_Container();
};

class MP_Global : virtual Noncopyable {
    ST_Container *stc;

    GB_HASH *marked_species;
    GB_HASH *unmarked_species;

public:
    MP_Global()
        : stc(NULL),
          marked_species(NULL),
          unmarked_species(NULL)
    {}
    ~MP_Global() {
        delete stc;
        set_marked_species(NULL);
        set_unmarked_species(NULL);
    }

    ST_Container *get_stc() { return stc; } // @@@ make result const
    void set_stc(ST_Container *stopfC) { // @@@ make private
        delete stc;
        stc = stopfC;
    }

    void reinit_stc(int probe_count) { set_stc(new ST_Container(probe_count)); }
    void clear_stc() { set_stc(NULL); }

    void set_marked_species(GB_HASH *new_marked_species) {
        if (marked_species) GBS_free_hash(marked_species);
        marked_species = new_marked_species;
    }
    void set_unmarked_species(GB_HASH *new_unmarked_species) {
        if (unmarked_species) GBS_free_hash(unmarked_species);
        unmarked_species = new_unmarked_species;
    }

    const GB_HASH *get_marked_species() const { return marked_species; }
    const GB_HASH *get_unmarked_species() const { return unmarked_species; }
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

extern MP_Main   *mp_main;
extern MP_Global *mp_global;
extern awar_vars  mp_gl_awars;                      // globale Variable, die manuell eingegebene Sequenz enthaelt

void MP_init_and_calculate_and_display_multiprobes(AW_window *, AW_CL cl_gb_main);

#else
#error MultiProbe.hxx included twice
#endif // MULTIPROBE_HXX


