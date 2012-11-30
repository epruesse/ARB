#ifndef PROBE_H
#define PROBE_H

#ifndef __LIST__
#include <list>
#endif
#ifndef __SET__
#include <set>
#endif

#ifndef ARBDB_H
#include <arbdb.h>
#endif
#ifndef PT_COM_H
#include <PT_com.h>
#endif
#ifndef AISC_GEN_SERVER_INCLUDED
#include <PT_server.h>
#endif
#ifndef PT_TOOLS_H
#include "PT_tools.h"
#endif
#ifndef CACHE_H
#include <cache.h>
#endif

#define PT_SERVER_MAGIC   0x32108765                // pt server identifier
#define PT_SERVER_VERSION 2                         // version of pt server database (no versioning till 2009/05/13)

#if defined(DEBUG)
// # define PTM_DEBUG_NODES
# define PTM_DEBUG_STAGE_ASSERTIONS
// # define PTM_DEBUG_VALIDATE_CHAINS
#endif // DEBUG

#if defined(PTM_DEBUG_STAGE_ASSERTIONS)
#define pt_assert_stage(s) pt_assert(psg.ptdata->get_stage() == (s))
#else // !defined(PTM_DEBUG_STAGE_ASSERTIONS)
#define pt_assert_stage(s) 
#endif

#if defined(PTM_DEBUG_VALIDATE_CHAINS)
#define pt_assert_valid_chain(node) pt_assert(PT_chain_has_valid_entries(node))
#else // !defined(PTM_DEBUG_VALIDATE_CHAINS)
#define pt_assert_valid_chain(node) 
#endif

typedef unsigned long ULONG;
typedef unsigned int  UINT;
typedef unsigned char uchar;

#define PT_CORE *(int *)0 = 0;

#define PT_MAX_PARTITION_DEPTH 4

#define PT_POS_TREE_HEIGHT 20
#define PT_MIN_TREE_HEIGHT PT_MAX_PARTITION_DEPTH

#define PT_POS_SECURITY  10
#define MIN_PROBE_LENGTH 2

enum PT_MATCH_TYPE {
    PT_MATCH_TYPE_INTEGER           = 0,
    PT_MATCH_TYPE_WEIGHTED_PLUS_POS = 1,
    PT_MATCH_TYPE_WEIGHTED          = -1
};



#define MATCHANSWER  50                             // private msg type: match result answer
#define CREATEANSWER 51                             // private msg type: create result answer
#define FINDANSWER   52                             // private msg type: find result answer

extern int gene_flag;           // if 'gene_flag' == 1 -> we are a gene pt server
struct Hs_struct;

enum type_types {
    t_int    = 1,
    t_string = 0,
    t_float  = 2
};

enum PT_base {
    PT_QU = 0,
    PT_N  = 1,
    PT_A,
    PT_C,
    PT_G,
    PT_T,
    PT_BASES,
    PT_B_UNDEF,
};

inline bool is_std_base(char b) { return b >= PT_A && b <= PT_T; }
inline bool is_ambig_base(char b) { return b == PT_QU || b == PT_N; }

inline char base_2_readable(char base) {
    static char table[] = ".NACGU";
    return base<PT_BASES ? table[safeCharIndex(base)] : base;
}

inline char *probe_2_readable(char *id_string, int len) {
    //! translate a string containing PT_base into readable characters.
    // caution if 'id_string' contains PT_QU ( == zero == EOS).
    // (see also: probe_compress_sequence)
    for (int i = 0; i<len; ++i) {
        id_string[i] = base_2_readable(id_string[i]);
    }
    return id_string;
}

inline void reverse_probe(char *seq, int len) {
    int i = 0;
    int j = len-1;

    while (i<j) std::swap(seq[i++], seq[j--]);
}

// -----------------
//      POS TREE

enum PT_NODE_TYPE {
    PT_NT_LEAF  = 0,
    PT_NT_CHAIN = 1,
    PT_NT_NODE  = 2,
    PT_NT_SAVED = 3, // stage 1 only
    PT_NT_UNDEF = 4
};

struct POS_TREE {
    uchar flags;
    char  data;
};

enum Stage { STAGE1, STAGE3 }; // STAGE2 does not exist

class PT_data {
    Stage  stage;
    int    data_offset;
    char  *data_start; // @@@ change to unsigned long (i.e. use offset)

public:
    PT_data(Stage stage_);

    Stage get_stage() const { return stage; }
    int get_offset() const { return data_offset; } // @@@ rename

    void use_rel_pointers(char *relStartAddress) { data_start = relStartAddress; }
    POS_TREE *rel2abs(unsigned long relPtr) const { return (POS_TREE*)(data_start+relPtr); } // @@@ elim (only used in stage1 where it makes no sense)
};

// ---------------------
//      Probe search

class probe_input_data : virtual Noncopyable { // every taxa's own data
    int size;   // @@@ misleading (contains 1 if no bases in sequence)

    mutable int *rel2abs; // @@@ cache me

    GBDATA *gb_species;

    bool group;           // probe_design: whether species is in group

    mutable cache::CacheHandle<SmartCharPtr> seq;
    static cache::Cache<SmartCharPtr>        seq_cache;

    GBDATA *get_gbdata() const { return gb_species; } // @@@ elim

    void load_rel2abs() const {
        GB_transaction ta(gb_species);

        GBDATA *gb_baseoff = GB_entry(gb_species, "baseoff");
        pt_assert(gb_baseoff);

        const GB_CUINT4 *baseoff = GB_read_ints_pntr(gb_baseoff);

        pt_assert(!rel2abs);
        rel2abs = new int[size];

        int abs = 0;
        for (int rel = 0; rel<size; ++rel) {
            abs          += baseoff[rel];
            rel2abs[rel]  = abs;
        }
    }

    SmartCharPtr loadSeq() const {
        GB_transaction ta(gb_species);
        GBDATA *gb_compr = GB_entry(gb_species, "compr");
        return SmartCharPtr(GB_read_bytes(gb_compr));
    }

public:

    probe_input_data()
        : size(0),
          rel2abs(0),
          gb_species(0),
          group(false)
    {}
    ~probe_input_data() {
        seq.release(seq_cache);
        flush_rel2abs();
    }

    static void set_cache_size(size_t csize) { seq_cache.resize(csize); }

    GB_ERROR init(GBDATA *gb_species_);

    SmartCharPtr get_dataPtr() const {
        if (!seq.is_cached()) seq.assign(loadSeq(), seq_cache);
        return seq.access(seq_cache);
    }

    // @@@ speed up all DB-searches by directly using GBQUARK
    const char *get_shortname() const {
        GB_transaction ta(gb_species);

        GBDATA *gb_name = GB_entry(gb_species, "name");
        pt_assert(gb_name);
        return GB_read_char_pntr(gb_name);
    }
    const char *get_fullname() const {
        GB_transaction ta(gb_species);

        GBDATA *gb_full = GB_entry(gb_species, "full_name");
        return gb_full ? GB_read_char_pntr(gb_full) : "";
    }
    long get_checksum() const { // @@@ change return-type -> uint32_t
        GB_transaction ta(gb_species);
        GBDATA *gb_cs = GB_entry(gb_species, "cs");
        pt_assert(gb_cs);
        uint32_t csum = uint32_t(GB_read_int(gb_cs));
        return csum;
    }
    int get_size() const { return size; }

    bool valid_rel_pos(int rel_pos) const { // returns true if rel_pos is inside sequence
        return rel_pos >= 0 && rel_pos<get_size();
    }

    bool inside_group() const { return group; }
    bool outside_group() const { return !group; }

    void set_group_state(bool isGroupMember) { group = isGroupMember; }

    long get_geneabspos() const {
        pt_assert(gene_flag); // only legal in gene-ptserver
        GBDATA *gb_pos = GB_entry(get_gbdata(), "abspos");
        if (gb_pos) return GB_read_int(gb_pos);
        return -1;
    }

    void preload_rel2abs() const { if (!rel2abs) load_rel2abs(); }
    void flush_rel2abs() const {
        if (rel2abs) { delete [] rel2abs; rel2abs = NULL; }
    }
    size_t get_abspos(size_t rel_pos) const {
        pt_assert(rel2abs); // you need to call preload_rel2abs
        return rel2abs[rel_pos];
    }

};

struct probe_statistic_struct {
    // common
    int cut_offs;                                   // statistic of chains
    int single_node;
    int short_node;
    int long_node;
    int longs;
    int shorts;
    int shorts2;
    int chars;

#ifdef ARB_64
    // 64bit specials
    int  int_node;
    int  ints;
    int  ints2;
    long maxdiff;
#endif

    void setup();
};

class BI_ecoli_ref;

class MostUsedPos : virtual Noncopyable {
    int pos;
    int used;

    MostUsedPos *next;

    void swapWith(MostUsedPos *other) {
        std::swap(pos, other->pos);
        std::swap(used, other->used);
    }

public:
    MostUsedPos() : pos(0), used(0), next(NULL) { }
    MostUsedPos(int p) : pos(p), used(1), next(NULL) { }
    ~MostUsedPos() { delete next; }

    void clear() {
        pos  = 0;
        used = 0;
        delete next;
        next = NULL;
    }

    void announce(int p) {
        if (p == pos) used++;
        else {
            if (next) next->announce(p);
            else next = new MostUsedPos(p);
            if (next->used>used) swapWith(next);
        }
    }


    int get_most_used() const { return pos; }
};

class probe_struct_global {
    char complement[256];                           // complement

public:
    GB_shell *gb_shell;
    GBDATA   *gb_main;                              // ARBDB interface
    GBDATA   *gb_species_data;
    GBDATA   *gb_sai_data; // @@@ elim (used once)
    char     *alignment_name;
    GB_HASH  *namehash;                             // name to int

    int                      data_count;
    struct probe_input_data *data;                  // the internal database

    char         *ecoli;                            // the ecoli sequenz
    BI_ecoli_ref *bi_ecoli;

    int  max_size;                                  // maximum sequence len
    long char_count;                                // number of all 'acgtuACGTU'

    int reversed;                                   // tell the matcher whether probe is reversed

    double *pos_to_weight;                          // position to weight

    int height;                                     // used in PT_forwhole_chain callbacks (design+debug)

    MostUsedPos abs_pos;

    int sort_by;

    const char *probe;                              // used in PT_forwhole_chain callbacks (design)
    char       *main_probe;

    char      *server_name;                         // name of this server
    aisc_com  *link;
    T_PT_MAIN  main;
    Hs_struct *com_so;                              // the communication socket
    POS_TREE  *pt;
    PT_data   *ptdata;

    probe_statistic_struct stat;

    void setup();
    void cleanup();

    int get_complement(int base) {
        pt_assert(base >= 0 && base <= 256);
        return complement[base];
    }

    void complement_probe(char *Probe, int len) {
        for (int i = 0; i<len; i++) {
            Probe[i] = get_complement(Probe[i]);
        }
    }

};

extern probe_struct_global psg;

class gene_struct {
    char       *gene_name;
    const char *arb_species_name; // pointers into 'gene_name'
    const char *arb_gene_name;

    void init(const char *gene_name_, const char *arb_species_name_, const char *arb_gene_name_) {
        int gene_name_len        = strlen(gene_name_);
        int arb_species_name_len = strlen(arb_species_name_);
        int arb_gene_name_len    = strlen(arb_gene_name_);

        int fulllen      = gene_name_len+1+arb_species_name_len+1+arb_gene_name_len+1;
        gene_name        = new char[fulllen];
        strcpy(gene_name, gene_name_);
        arb_species_name = gene_name+(gene_name_len+1);
        strcpy((char*)arb_species_name, arb_species_name_);
        arb_gene_name    = arb_species_name+(arb_species_name_len+1);
        strcpy((char*)arb_gene_name, arb_gene_name_);
    }

public:
    gene_struct(const char *gene_name_, const char *arb_species_name_, const char *arb_gene_name_) {
        init(gene_name_, arb_species_name_, arb_gene_name_);
    }
    gene_struct(const gene_struct& other) {
        if (&other != this) {
            init(other.get_internal_gene_name(), other.get_arb_species_name(), other.get_arb_gene_name());
        }
    }
    gene_struct& operator = (const gene_struct& other) {
        if (&other != this) {
            delete [] gene_name;
            init(other.get_internal_gene_name(), other.get_arb_species_name(), other.get_arb_gene_name());
        }
        return *this;
    }

    ~gene_struct() {
        delete [] gene_name;
    }

    const char *get_internal_gene_name() const { return gene_name; }
    const char *get_arb_species_name() const { return arb_species_name; }
    const char *get_arb_gene_name() const { return arb_gene_name; }
};

struct ltByArbName {
    bool operator()(const gene_struct *gs1, const gene_struct *gs2) const {
        int cmp           = strcmp(gs1->get_arb_species_name(), gs2->get_arb_species_name());
        if (cmp == 0) { cmp = strcmp(gs1->get_arb_gene_name(), gs2->get_arb_gene_name()); }
        return cmp<0;
    }
};
struct ltByInternalName {
    bool operator()(const gene_struct *gs1, const gene_struct *gs2) const {
        int cmp = strcmp(gs1->get_internal_gene_name(), gs2->get_internal_gene_name());
        return cmp<0;
    }
};

typedef std::list<gene_struct>                          gene_struct_list;
typedef std::set<const gene_struct *, ltByInternalName> gene_struct_index_internal;
typedef std::set<const gene_struct *, ltByArbName>      gene_struct_index_arb;

extern gene_struct_index_arb      gene_struct_arb2internal; // sorted by arb species+gene name
extern gene_struct_index_internal gene_struct_internal2arb; // sorted by internal name

#else
#error probe.h included twice
#endif

