// ============================================================= //
//                                                               //
//   File      : probe_tree.h                                    //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef PROBE_TREE_H
#define PROBE_TREE_H

#if defined(DARWIN)
#include <krb5.h>
#else
#include <bits/wordsize.h>
#endif // DARWIN

#ifndef STATIC_ASSERT_H
#include <static_assert.h>
#endif
#ifndef PROBE_H
#include "probe.h"
#endif

#define PT_CHAIN_NTERM     250
#define PT_SHORT_SIZE      0xffff
#define PT_INIT_CHAIN_SIZE 20

enum PT_NODE_TYPE {
    PT_NT_LEAF  = 0,
    PT_NT_CHAIN = 1,
    PT_NT_NODE  = 2,
    PT_NT_SAVED = 3, // stage 1 only
};
const PT_NODE_TYPE PT_NT_UNDEF = PT_NODE_TYPE(4); // logically also belongs to PT_NODE_TYPE, but does not fit into bits reserved in node.flags

struct pt_global {
    PT_NODE_TYPE flag_2_type[256];
    char         count_bits[PT_BASES+1][256]; // returns how many bits are set (e.g. PT_count_bits[3][n] is the number of the 3 lsb bits)

    void init(Stage stage);
};

extern pt_global PT_GLOBAL;

#if 0
/*

/ ****************
    Their are 3 stages of data format:
        1st:        Creation of the tree
        2nd:        find equal or nearly equal subtrees
        3rd:        minimum sized tree

    The generic pointer (father):
        1st create:     Pointer to the father
        1st save:
        2nd load        rel ptr of 'this' in the output file
                    (==0 indicates not saved)

**************** /
/ * data format: * /

/ ********************* Written object *********************** /
    byte    =32         bit[7] = bit[6] = 0; bit[5] = 1
                    bit[4] free
                    bit[0-3] size of former entry -4
                    if ==0 then size follows
    PT_PNTR     rel start of the real object       // actually it's a pointer to the objects father
    [int        size]       if bit[0-3] == 0;

/ ********************* tip / leaf (7-13   +4)  *********************** /
    byte    <32         bit[7] = bit[6] = bit[5] = 0
                        bit[3-4] free
    [PT_PNTR    father]     if main->mode
    short/int   name        int if bit[0]
    short/int   rel pos     int if bit[1]
    short/int   abs pos     int if bit[2]

/ ********************* inner node (1 + 4*[1-6] +4) (stage 1 + 2) *********************** /
    byte    >128        bit[7] = 1  bit[6] = 0
    [PT_PNTR father]        if main->mode (STAGE1)
    [PT_PNTR son0]          if bit[0] 
    ...                     ...
    [PT_PNTR son5]          if bit[5] 

/ ********************* inner node (3-22    +4) (stage 3 only) *********************** /
    byte                bit[7] = 1  bit[6] = 0
    byte2               bit2[7] = 0  bit2[6] = 0  -->  short/char
                        bit2[7] = 1  bit2[6] = 0  -->  int/short
                        bit2[7] = 0  bit2[6] = 1  -->  long/int         // atm only if ARB_64 is set
                        bit2[7] = 1  bit2[6] = 1  -->  undefined        // atm only if ARB_64 is set
    [char/short/int/long son0]  if bit[0]   left (bigger) type if bit2[0] else right (smaller) type
    [char/short/int/long son1]  if bit[1]   left (bigger) type if bit2[1] else right (smaller) type
    ...
    [char/short/int/long son5]  if bit[5]   left (bigger) type if bit2[5] else right (smaller) type

    example1:   byte  = 0x8d    --> inner node; son0, son2 and son3 are available
                byte2 = 0x05    --> son0 and son2 are shorts; son3 is a char

    example2:   byte  = 0x8d    --> inner node; son0, son2 and son3 are available
                byte2 = 0x81    --> son0 is a int; son2 and son3 are shorts

// example3 atm only if ARB_64 is set
    example3:   byte  = 0x8d    --> inner node; son0, son2 and son3 are available
                byte2 = 0x44    --> son2 is a long; son0 and son3 are ints

/ ********************* inner nodesingle (1)    (stage3 only) *********************** /
    byte                bit[7] = 1  bit[6] = 1
                    bit[0-2]    base
                    bit[3-5]    offset 0->1 1->3 2->4 3->5 ....


/ ********************* chain (8-n +4) stage 1 *********************** /
    byte =64/65         bit[7] = 0, bit[6] = 1  bit[5] = 0
    [PT_PNTR    father]     if main->mode
    short/int   ref abs pos int if bit[0]
    PT_PNTR     firstelem

    / **** chain elems *** /
        PT_PNTR     next element
        short/int   name
                if bit[15] then integer
                -1 not allowed
        short/int   rel pos
                if bit[15] then integer
        short/int   apos short if bit[15] = 0]
]


/ ********************* chain (8-n +4) stage 2/3 *********************** /

    byte =64/65         bit[7] = 0, bit[6] = 1  bit[5] = 0
    [PT_PNTR    father]     if main->mode
    short/int   ref abs pos int if bit[0]
[   char/short/int      rel name [ to last name eg. rel names 10 30 20 50 -> abs names = 10 40 60 110
                if bit[7] then short
                if bit[7] and bit[6] then integer
    short/int       rel pos
                if bit[15] -> the next bytes are the apos else use ref_abs_pos
                if bit[14] -> this(==rel_pos) is integer
    [short/int]     [apos short if bit[15] = 0]
]
    char -1         end flag
*/
#endif

#define IS_SINGLE_BRANCH_NODE 0x40
#ifdef ARB_64
# define INT_SONS              0x80
# define LONG_SONS             0x40
#else
# define LONG_SONS             0x80
#endif

// -----------------------------------------------
//      Get the size of entries (stage 1) only

#define PT1_BASE_SIZE sizeof(POS_TREE1) // flag + father

#define PT1_EMPTY_LEAF_SIZE       (PT1_BASE_SIZE+6)                 // name rel apos
#define PT1_LEAF_SIZE(leaf)       (PT1_BASE_SIZE+6+2*PT_GLOBAL.count_bits[3][(leaf)->flags])
#define PT1_CHAIN_SHORT_HEAD_SIZE (PT1_BASE_SIZE+2+sizeof(PT_PNTR)) // apos first_elem
#define PT1_CHAIN_LONG_HEAD_SIZE  (PT1_CHAIN_SHORT_HEAD_SIZE+2)     // apos uses 4 byte here
#define PT1_EMPTY_NODE_SIZE       PT1_BASE_SIZE

#define PT1_MIN_CHAIN_ENTRY_SIZE  (sizeof(PT_PNTR)+3*sizeof(char)) // depends on PT_write_compact_nat
#define PT1_MAX_CHAIN_ENTRY_SIZE  (sizeof(PT_PNTR)+3*(sizeof(int)+1))

#define PT1_NODE_WITHSONS_SIZE(sons) (PT1_EMPTY_NODE_SIZE+sizeof(PT_PNTR)*(sons))

#define PT_NODE_SON_COUNT(node) (PT_GLOBAL.count_bits[PT_BASES][node->flags])
#define PT1_NODE_SIZE(node)     PT1_NODE_WITHSONS_SIZE(PT_NODE_SON_COUNT(node))

// -----------------
//      POS TREE

#define FLAG_TYPE_BITS      2
#define FLAG_FREE_BITS      (8-FLAG_TYPE_BITS)
#define FLAG_FREE_BITS_MASK ((1<<FLAG_TYPE_BITS)-1)
#define FLAG_TYPE_BITS_MASK (0xFF^FLAG_FREE_BITS_MASK)

struct POS_TREE1 {
    uchar      flags;
    POS_TREE1 *father;

    const char *udata() const { return ((const char*)this)+sizeof(*this); }
    char *udata() { return ((char*)this)+sizeof(*this); }

    POS_TREE1 *get_father() const {
        pt_assert(!is_saved());
        return father;
    }
    void set_father(POS_TREE1 *new_father) {
        pt_assert(!new_father || new_father->is_node());
        father = new_father;
    }
    void clear_fathers();

    void set_type(PT_NODE_TYPE type) {
        // sets user bits to zero
        pt_assert(type != PT_NT_UNDEF && type != PT_NT_SAVED); // does not work for saved nodes (done manually)
        flags = type<<FLAG_FREE_BITS;
    }
    PT_NODE_TYPE get_type() const { return (PT_NODE_TYPE)PT_GLOBAL.flag_2_type[flags]; }

    bool is_node() const { return get_type() == PT_NT_NODE; }
    bool is_leaf() const { return get_type() == PT_NT_LEAF; }
    bool is_chain() const { return get_type() == PT_NT_CHAIN; }
    bool is_saved() const { return get_type() == PT_NT_SAVED; }
} __attribute__((packed));

struct POS_TREE3 { // pos-tree (stage 3)
    uchar flags;
    char  data;

    const char *udata() const { return &data; }
    char *udata() { return &data; }

    void set_type(PT_NODE_TYPE type) {
        // sets user bits to zero
        pt_assert(type != PT_NT_UNDEF && type != PT_NT_SAVED); // does not work for saved nodes (done manually)
        flags = type<<FLAG_FREE_BITS;
    }
    PT_NODE_TYPE get_type() const { return (PT_NODE_TYPE)PT_GLOBAL.flag_2_type[flags]; }

    bool is_node() const { return get_type() == PT_NT_NODE; }
    bool is_leaf() const { return get_type() == PT_NT_LEAF; }
    bool is_chain() const { return get_type() == PT_NT_CHAIN; }
} __attribute__((packed));

inline size_t PT_node_size(POS_TREE3 *node) {
    size_t size = 1; // flags
    if ((node->flags & IS_SINGLE_BRANCH_NODE) == 0) {
        UINT sec = (uchar)node->data; // read second byte for charshort/shortlong info
        ++size;

        long i = PT_GLOBAL.count_bits[PT_BASES][node->flags] + PT_GLOBAL.count_bits[PT_BASES][sec];
#ifdef ARB_64
        if (sec & LONG_SONS) {
            size += 4*i;
        }
        else if (sec & INT_SONS) {
            size += 2*i;
        }
        else {
            size += i;
        }
#else
        if (sec & LONG_SONS) {
            size += 2*i;
        }
        else {
            size += i;
        }
#endif
    }
    return size;
}

template <typename PT> inline PT *PT_read_son(PT *node, PT_base base); // @@@ become member

template <> inline POS_TREE3 *PT_read_son<POS_TREE3>(POS_TREE3 *node, PT_base base) { // stage 3 (no father)
    pt_assert_stage(STAGE3);
    pt_assert(node->is_node());

    if (node->flags & IS_SINGLE_BRANCH_NODE) {
        if (base != (node->flags & 0x7)) return NULL; // no son
        long i    = (node->flags >> 3)&0x7;      // this son
        if (!i) i = 1; else i+=2;                // offset mapping
        pt_assert(i >= 0);
        return (POS_TREE3 *)(((char *)node)-i);
    }
    if (!((1<<base) & node->flags)) { // bit not set
        return NULL;
    }

    UINT sec  = (uchar)node->data;   // read second byte for charshort/shortlong info
    long i    = PT_GLOBAL.count_bits[base][node->flags];
    i        += PT_GLOBAL.count_bits[base][sec];

#ifdef ARB_64
    if (sec & LONG_SONS) {
        if (sec & INT_SONS) {                                   // undefined -> error
            GBK_terminate("Your pt-server search tree is corrupt! You can not use it anymore.\n"
                          "Error: ((sec & LONG_SON) && (sec & INT_SONS)) == true\n"
                          "       this combination of both flags is not implemented");
        }
        else {                                                // long/int
#ifdef DEBUG
            printf("Warning: A search tree of this size is not tested.\n");
            printf("         (sec & LONG_SON) == true\n");
#endif
            UINT offset = 4 * i;
            if ((1<<base) & sec) {              // long
                i = PT_read_long(&node->data+1+offset);
            }
            else {                                              // int
                i = PT_read_int(&node->data+1+offset);
            }
        }

    }
    else {
        if (sec & INT_SONS) {                                   // int/short
            UINT offset = i+i;
            if ((1<<base) & sec) {                              // int
                i = PT_read_int(&node->data+1+offset);
            }
            else {                                              // short
                i = PT_read_short(&node->data+1+offset);
            }
        }
        else {                                                  // short/char
            UINT offset = i;
            if ((1<<base) & sec) {                              // short
                i = PT_read_short(&node->data+1+offset);
            }
            else {                                              // char
                i = PT_read_char(&node->data+1+offset);
            }
        }
    }
#else
    if (sec & LONG_SONS) {
        UINT offset = i+i;
        if ((1<<base) & sec) {
            i = PT_read_int(&node->data+1+offset);
        }
        else {
            i = PT_read_short(&node->data+1+offset);
        }
    }
    else {
        UINT offset = i;
        if ((1<<base) & sec) {
            i = PT_read_short(&node->data+1+offset);
        }
        else {
            i = PT_read_char(&node->data+1+offset);
        }
    }
#endif
    pt_assert(i >= 0);
    return (POS_TREE3 *)(((char*)node)-i);
}

template<> inline POS_TREE1 *PT_read_son<POS_TREE1>(POS_TREE1 *node, PT_base base) {
    pt_assert_stage(STAGE1);
    if (!((1<<base) & node->flags)) return NULL;   // bit not set
    base = (PT_base)PT_GLOBAL.count_bits[base][node->flags];
    return PT_read_pointer<POS_TREE1>(node->udata() + sizeof(PT_PNTR)*base);
}

inline size_t PT_leaf_size(POS_TREE3 *node) {
    size_t size = 1;  // flags
    size += (PT_GLOBAL.count_bits[PT_BASES][node->flags]+3)*2;
    return size;
}

// --------------------------------------
//      Different types of locations

class AbsLoc {
    int name; // index into psg.data[], aka species id
    int apos; // absolute position in alignment
protected:
    void set_abs_pos(int abs_pos) { apos = abs_pos; }
    void set_name(int name_) {
        pt_assert(name == -1); // only allowed if instance has been default constructed
        name = name_;
        pt_assert(has_valid_name());
    }
public:
    AbsLoc() : name(-1), apos(0) {}
    AbsLoc(int name_, int abs_pos) : name(name_), apos(abs_pos) {}
    virtual ~AbsLoc() {}

    bool has_valid_name() const { return name >= 0 && name < psg.data_count; }

    int get_name() const { return name; }
    int get_abs_pos() const { return apos; }

    const probe_input_data& get_pid() const { pt_assert(has_valid_name()); return psg.data[name]; }

    bool is_equal(const AbsLoc& other) const { return apos == other.apos && name == other.name; }

#if defined(DEBUG)
    void dump(FILE *fp) const {
        fprintf(fp, "          apos=%6i  name=%6i='%s'\n",
                get_abs_pos(), get_name(), has_valid_name() ? get_pid().get_shortname() : "<invalid>");
    }
#endif
};

class DataLoc : public AbsLoc {
    int rpos; // relative position in data

public:
    bool has_valid_positions() const { return get_abs_pos() >= 0 && rpos >= 0 && get_abs_pos() >= rpos; }

    DataLoc() : rpos(0) {}
    DataLoc(int name_, int apos_, int rpos_) : AbsLoc(name_, apos_), rpos(rpos_) { pt_assert(has_valid_positions()); }
    template <typename PT> explicit DataLoc(const PT *node) {
        pt_assert(node->is_leaf());
        const char *data = node->udata();
        if (node->flags&1) { set_name(PT_read_int(data)); data += 4; } else { set_name(PT_read_short(data)); data += 2; }
        if (node->flags&2) { rpos = PT_read_int(data); data += 4; } else { rpos = PT_read_short(data); data += 2; }
        if (node->flags&4) { set_abs_pos(PT_read_int(data)); data += 4; } else { set_abs_pos(PT_read_short(data)); /*data += 2;*/ }

        pt_assert(has_valid_positions());
    }
    explicit DataLoc(const AbsLoc& aloc) // expensive!
        : AbsLoc(aloc),
          rpos(get_pid().calc_relpos(get_abs_pos()))
    {
#if 0 // @@@ fix expensive conversions later
        const DataLoc *dloc = dynamic_cast<const DataLoc*>(&aloc);
        if (dloc) {
            GBK_dump_backtrace(stderr, "expensive conversion");
            // pt_assert(!dloc); // expensive conversion (DataLoc->AbsLoc->DataLoc)
        }
#endif
    }

    int get_rel_pos() const { return rpos; }

    void set_position(int abs_pos, int rel_pos) {
        set_abs_pos(abs_pos);
        rpos = rel_pos;
        pt_assert(has_valid_positions());
    }

    int restlength() const { return get_pid().get_size()-rpos; }
    bool is_shorther_than(int offset) const { return offset >= restlength(); }

    bool is_equal(const DataLoc& other) const { return rpos == other.rpos && AbsLoc::is_equal(other); }

#if defined(DEBUG)
    void dump(FILE *fp) const {
        fprintf(fp, "          apos=%6i  rpos=%6i  name=%6i='%s'\n",
                get_abs_pos(), rpos, get_name(), has_valid_name() ? get_pid().get_shortname() : "<invalid>");
    }
#endif
};

inline bool operator==(const AbsLoc& loc1, const AbsLoc& loc2) { return loc1.is_equal(loc2); }

// -------------------------
//      ReadableDataLoc

class ReadableDataLoc : public DataLoc {
    const probe_input_data& pid;
    mutable SmartCharPtr    seq;
    const char&             qseq;

public:
    ReadableDataLoc(int name_, int apos_, int rpos_)
        : DataLoc(name_, apos_, rpos_),
          pid(DataLoc::get_pid()),
          seq(pid.get_dataPtr()),
          qseq(*seq)
    {}
    explicit ReadableDataLoc(const DataLoc& loc)
        : DataLoc(loc),
          pid(DataLoc::get_pid()),
          seq(pid.get_dataPtr()),
          qseq(*seq)
    {}
    template <typename PT> explicit ReadableDataLoc(const PT *node)
        : DataLoc(node),
          pid(DataLoc::get_pid()),
          seq(pid.get_dataPtr()),
          qseq(*seq)
    {}

    const probe_input_data& get_pid() const { return pid; }

    PT_base operator[](int offset) const {
        int ro_pos = get_rel_pos()+offset;
        return pid.valid_rel_pos(ro_pos) ? PT_base((&qseq)[ro_pos]) : PT_QU;
    }
};

// -----------------------

inline const char *PT_READ_CHAIN_ENTRY_stage_1(const char *entry, AbsLoc& loc, int *entry_size) {
    pt_assert_stage(STAGE1);

    if (entry) {
        const char *rp = entry + sizeof(PT_PNTR);

        int name = PT_read_compact_nat(rp);
        int rpos = PT_read_compact_nat(rp);
        int apos = PT_read_compact_nat(rp);

        loc = DataLoc(name, apos, rpos);

        *entry_size = rp-entry;

        return PT_read_pointer<char>(entry);
    }

    loc = DataLoc(-1, 0, 0);
    return NULL;
}

inline const char *PT_READ_CHAIN_ENTRY_stage_3(const char *ptr, int mainapos, AbsLoc& loc) {
    // Caution: 'name' (of AbsLoc) has to be initialized before first call and shall not be modified between calls

    pt_assert_stage(STAGE3);

    uint_8  has_main_apos;
    uint_32 name = loc.get_name() + read_nat_with_reserved_bits<1>(ptr, has_main_apos);

    loc = AbsLoc(name, has_main_apos ? mainapos : PT_read_compact_nat(ptr));

    return ptr;
}

inline char *PT_WRITE_CHAIN_ENTRY(char *ptr, int mainapos, int name, const int apos) {
    pt_assert_stage(STAGE1);
    bool has_main_apos = apos == mainapos;
    write_nat_with_reserved_bits<1>(ptr, name, has_main_apos);
    if (!has_main_apos) PT_write_compact_nat(ptr, apos);
    return ptr;
}

// -----------------------
//      ChainIterators

class ChainIteratorStage1 : virtual Noncopyable {
    const char *data;
    AbsLoc      loc;
    int         mainapos;

    const char *last_data;
    int         last_size;

    bool at_end_of_chain() const { return loc.get_name() == -1; }
    void set_loc_from_chain() {
        last_data = data;
        data      = PT_READ_CHAIN_ENTRY_stage_1(data, loc, &last_size);
        pt_assert(at_end_of_chain() || loc.has_valid_name());
    }
    void inc() {
        pt_assert(!at_end_of_chain());
        set_loc_from_chain();
    }

public:
    typedef POS_TREE1 POS_TREE_TYPE;

    ChainIteratorStage1(const POS_TREE1 *node)
        : data(node->udata()),
          last_size(-1)
    {
        pt_assert_stage(STAGE1);
        pt_assert(node->is_chain());

        if (node->flags&1) {
            mainapos  = PT_read_int(data);
            data     += 4;
        }
        else {
            mainapos  = PT_read_short(data);
            data     += 2;
        }

        data = PT_read_pointer<char>(data);
        set_loc_from_chain();
    }

    operator bool() const { return !at_end_of_chain(); }
    const AbsLoc& at() const { return loc; }
    const ChainIteratorStage1& operator++() { inc(); return *this; } // prefix-inc

    const char *memory() const { return last_data; }
    int memsize() const { return last_size; }

    int get_mainapos() const { return mainapos; }
};

class ChainIteratorStage3 : virtual Noncopyable {
    const char *data;
    AbsLoc      loc;

    int mainapos;
    int elements_ahead;

    bool at_end_of_chain() const { return elements_ahead<0; }
    void set_loc_from_chain() {
        if (elements_ahead>0) {
            data = PT_READ_CHAIN_ENTRY_stage_3(data, mainapos, loc);
        }
        else {
            pt_assert(elements_ahead == 0);
            loc = AbsLoc();
        }
        --elements_ahead;
        pt_assert(at_end_of_chain() || loc.has_valid_name());
    }
    void inc() {
        pt_assert(!at_end_of_chain());
        set_loc_from_chain();
    }

public:
    typedef POS_TREE3 POS_TREE_TYPE;

    ChainIteratorStage3(const POS_TREE3 *node)
        : data(node->udata()),
          loc(0, 0) // init name with 0 (needed for chain reading in STAGE3)
    {
        pt_assert_stage(STAGE3);
        pt_assert(node->is_chain());

        if (node->flags&1) {
            mainapos  = PT_read_int(data);
            data     += 4;
        }
        else {
            mainapos  = PT_read_short(data);
            data     += 2;
        }

        elements_ahead = PT_read_compact_nat(data);

        pt_assert(elements_ahead>0); // chain cant be empty
        set_loc_from_chain();
    }

    operator bool() const { return !at_end_of_chain(); }
    const AbsLoc& at() const { return loc; }
    const ChainIteratorStage3& operator++() { inc(); return *this; } // prefix-inc

    const char *memptr() const { return data; }
};

template<typename PT, typename T> int PT_forwhole_chain(PT *node, T& func);

template<typename T> int PT_forwhole_chain(POS_TREE1 *node, T& func) {
    pt_assert_stage(STAGE1);
    int error = 0;
    for (ChainIteratorStage1 entry(node); entry && !error; ++entry) {
        error = func(entry.at());
    }
    return error;
}

template<typename T> int PT_forwhole_chain(POS_TREE3 *node, T& func) {
    pt_assert_stage(STAGE3);
    int error = 0;
    for (ChainIteratorStage3 entry(node); entry && !error; ++entry) {
        error = func(entry.at());
    }
    return error;
}

#if defined(DEBUG)
struct PTD_chain_print {
    int operator()(const AbsLoc& loc) { loc.dump(stdout); return 0; }
};
#endif

#else
#error probe_tree.h included twice
#endif // PROBE_TREE_H
