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

#define PT_SERVER_MAGIC   0x32108765                // pt server identifier
#define PT_SERVER_VERSION 2                         // version of pt server database (no versioning till 2009/05/13)

#define pt_assert(bed) arb_assert(bed)

#if defined(DEBUG)
// # define PTM_DEBUG
// # define PTM_DUMP_PTREE
#endif // DEBUG

typedef unsigned long ULONG;
typedef unsigned int  UINT;
typedef unsigned char uchar;

#define PT_CORE *(int *)0 = 0;

#define PT_POS_TREE_HEIGHT 20
#define PT_POS_SECURITY    10
#define MIN_PROBE_LENGTH   6

enum PT_MATCH_TYPE {
    PT_MATCH_TYPE_INTEGER           = 0,
    PT_MATCH_TYPE_WEIGHTED_PLUS_POS = 1,
    PT_MATCH_TYPE_WEIGHTED          = -1
};



#define MATCHANSWER  50                             // private msg type: match result answer
#define CREATEANSWER 51                             // private msg type: create result answer
#define FINDANSWER   52                             // private msg type: find result answer

extern int gene_flag;           // if 'gene_flag' == 1 -> we are a gene pt server
extern ULONG physical_memory;
struct Hs_struct;

enum type_types {
    t_int    = 1,
    t_string = 0,
    t_float  = 2
};

enum PT_BASES {
    PT_QU = 0,
    PT_N  = 1,
    PT_A,
    PT_C,
    PT_G,
    PT_T,
    PT_B_MAX,
    PT_B_UNDEF,
};

inline char PT_base_2_char(PT_BASES base) {
    static char table[] = "0NACGU";
    return base<PT_B_MAX ? table[base] : (char)base;
}

inline void PT_base_2_string(char *id_string) {
    //! get a string with readable bases from a string with PT_?
    for (int i = 0; id_string[i]; ++i) {
        id_string[i] = PT_base_2_char(PT_BASES(id_string[i]));
    }
}


// -----------------
//      POS TREE

enum PT_NODE_TYPE {
    PT_NT_LEAF        = 0,
    PT_NT_CHAIN       = 1,
    PT_NT_NODE        = 2,
    PT_NT_SINGLE_NODE = 3,                          // stage 3
    PT_NT_SAVED       = 3,                          // stage 1+2
    PT_NT_UNDEF       = 4
};

struct POS_TREE {
    uchar flags;
    char  data;
};

struct PTM2 {
    char *data_start;                               // points to start of data
    int   stage1;
    int   stage2;
    int   stage3;
    int   mode;
};



// ---------------------
//      Probe search

class probe_input_data : virtual Noncopyable {      // every taxa's own data

    char *data;       // sequence
    long  checksum;   // checksum of sequence
    int   size; // @@@ misleading (contains 1 if no bases in sequence)

    char   *name;
    char   *fullname;
    GBDATA *gbd;

    bool group;           // probe_design: whether species is in group

    // obsolete methods below @@@ remove them
    GBDATA *get_gbdata() const { return gbd; }
    void set_data(char *assign, int size_) { pt_assert(!data); data = assign; size = size_; }
    void set_checksum(long cs) { checksum = cs; }

public:

    probe_input_data()
        : data(0),
          checksum(0),
          size(0),
          name(0),
          fullname(0),
          gbd(0),
          group(false)
    {}
    ~probe_input_data() {
        free(data);
        free(name);
        free(fullname);
    }

    GB_ERROR init(GBDATA *gbd_);

    const char *get_data() const { return data; }
    char *read_alignment(int *psize) const;

    const char *get_name() const { return name; }
    const char *get_fullname() const { return fullname; }
    long get_checksum() const { return checksum; }
    int get_size() const { return size; }

    bool inside_group() const { return group; }
    bool outside_group() const { return !group; }

    void set_group_state(bool isGroupMember) { group = isGroupMember; }

    long get_abspos() const {
        pt_assert(gene_flag); // only legal in gene-ptserver
        GBDATA *gb_pos = GB_entry(get_gbdata(), "abspos");
        if (gb_pos) return GB_read_int(gb_pos);
        return -1;
    }

private:
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

extern struct probe_struct_global {
    GB_shell *gb_shell;
    GBDATA   *gb_main;                              // ARBDB interface
    GBDATA   *gb_species_data;
    GBDATA   *gb_sai_data;
    char     *alignment_name;
    GB_HASH  *namehash;                             // name to int

    int                      data_count;
    struct probe_input_data *data;                  // the internal database

    char         *ecoli;                            // the ecoli sequenz
    BI_ecoli_ref *bi_ecoli;

    int  max_size;                                  // maximum sequence len
    long char_count;                                // number of all 'acgtuACGTU'

    int    mismatches;                              // chain handle in match
    double wmismatches;
    int    N_mismatches;
    int*   w_N_mismatches;
    int    w_N_mismatches_Size;

    int reversed;                                   // tell the matcher whether probe is reversed

    double *pos_to_weight;                          // position to weight
    char    complement[256];                        // complement

    int deep;                                       // for probe matching
    int height;
    int length;

    MostUsedPos abs_pos;

    int sort_by;

    char *probe;                                    // probe design + chains
    char *main_probe;

    char      *server_name;                         // name of this server
    aisc_com  *link;
    T_PT_MAIN  main;
    Hs_struct *com_so;                              // the communication socket
    POS_TREE  *pt;
    PTM2      *ptmain;

    probe_statistic_struct stat;

    void setup();
    void cleanup();

} psg;

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

#define PT_base_string_counter_eof(str) (*(unsigned char *)(str) == 255)

inline void fflush_all() { // @@@ will become a duplicate (when merging PT_tools.h later)
    fflush(stderr);
    fflush(stdout);
}

#else
#error probe.h included twice
#endif

