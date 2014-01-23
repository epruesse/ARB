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

struct pt_global {
    char count_bits[PT_BASES+1][256];         // returns how many bits are set (e.g. PT_count_bits[3][n] is the number of the 3 lsb bits)

    void init();
};

extern pt_global PT_GLOBAL;

/* --------------------------------------------------------------------------------
 *
 *  Their are 3 stages of data format:
 *      1st:        Creation of the tree
 *      2nd:        Tree saved to disk
 *
 *  In stage 1 every element has a father pointer directly behind
 *  the 1st byte (flags). Exception: Nodes of type PT1_SAVED.
 *
 * --------------------
 * Data format:
 *
 * for 'compactNAT' see PT_write_compact_nat / PT_read_compact_nat
 *
 * ---------- Written object (PT1_SAVED)
 *
 *  flags       bit[7]   = 0
 *              bit[6]   = 0
 *              bit[5]   = 1
 *              bit[4]   = unused
 *              bit[3-0] = size of former entry-4 (if ==0 then size follows)
 *  PT_PNTR     rel start of the object in the saved index
 *  [int        size]      (if flagsbit[3-0] == 0)
 *
 * ---------- leaf (PT1_LEAF and PT2_LEAF)
 *
 *  byte        flags       bit[7]   = 0
 *                          bit[6]   = 0
 *                          bit[5]   = 0
 *                          bit[4-3] = unused
 *                          bit[2-0] = used to indicate sizes of entries below
 *  [PT_PNTR    father]     only if type is PT1_LEAF
 *  short/int   name        int if bit[0]
 *  short/int   relpos      int if bit[1]
 *  short/int   abspos      int if bit[2]
 *
 * ---------- inner node (PT1_NODE)
 *
 *  byte        flags       bit[7]   = 1
 *                          bit[6]   = 0
 *                          bit[5-0] = indicate existing sons
 *  PT_PNTR     father
 *  [PT_PNTR son0]          if bit[0]
 *  ...                     ...
 *  [PT_PNTR son5]          if bit[5]
 *
 * ---------- inner node with more than one child (PT2_NODE)
 *
 *  byte        flags       bit[7]   = 1
 *                          bit[6]   = 0
 *                          bit[5-0] = indicate existing sons
 *  byte2                   bit2[7] = 0  bit2[6] = 0  -->  short/char
 *                          bit2[7] = 1  bit2[6] = 0  -->  int/short
 *                          bit2[7] = 0  bit2[6] = 1  -->  long/int         (only if ARB_64 is set)
 *                          bit2[7] = 1  bit2[6] = 1  -->  undefined        (only if ARB_64 is set)
 *
 *  [char/short/int/long son0]  if bit[0]; uses left (bigger) type if bit2[0] else right (smaller) type
 *  [char/short/int/long son1]  if bit[1]; uses left (bigger) type if bit2[1] else right (smaller) type
 *  ...
 *  [char/short/int/long son5]  if bit[5]; uses left (bigger) type if bit2[5] else right (smaller) type
 *
 *  example1:   byte  = 0x8d    --> inner node; son0, son2 and son3 are available
 *              byte2 = 0x05    --> son0 and son2 are shorts; son3 is a char
 *
 *  example2:   byte  = 0x8d    --> inner node; son0, son2 and son3 are available
 *              byte2 = 0x81    --> son0 is a int; son2 and son3 are shorts
 *
 *  [example3 atm only if ARB_64 is set]
 *  example3:   byte  = 0x8d    --> inner node; son0, son2 and son3 are available
 *              byte2 = 0x44    --> son2 is a long; son0 and son3 are ints
 *
 * ---------- inner node with single child (PT2_NODE)
 *
 *  byte        flags       bit[7]   = 1
 *                          bit[6]   = 1
 *                          bit[5-3] = offset 0->1 1->3 2->4 3->5 ....
 *                          bit[2-0] = base
 *
 * ---------- chain (PT1_CHAIN)
 *
 *  byte        flags       bit[7]   = 0
 *                          bit[6]   = 1
 *                          bit[5]   = 0
 *                          bit[4]   = uses PT_short_chain_header (see SHORT_CHAIN_HEADER_FLAG_BIT)
 *                          bit[3]   = unused
 *                          bit[2-0] = entries in PT_short_chain_header, see SHORT_CHAIN_HEADER_SIZE_MASK (unused otherwise)
 *  PT_PNTR     father
 *  PT_short_chain_header or PT_long_chain_header
 *
 *  if PT_long_chain_header is used, the memory pointed-to by its 'entrymem'
 *  contains for each chain element:
 *
 *  compactNAT  namediff    (1 bit reserved for 'hasrefapos')
 *  [compactNAT abspos]     (if 'hasrefapos'; otherwise entry has refabspos as in PT_long_chain_header)
 *
 * ---------- chain (PT2_CHAIN)
 *
 *  byte        flags       bit[7]   = 0
 *                          bit[6]   = 1
 *                          bit[5-1] = unused
 *                          bit[0]   = flag for refabspos
 *  short/int   refabspos   (int if bit[0])
 *  compactNAT  chainsize
 *
 *  for each chain element:
 *
 *  compactNAT  namediff    (relative to previous name in chain; first bit reserved : 1 -> has refabspos )
 *  [compactNAT abspos]     (only if not has refabspos)
 *
 */

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

enum PT1_TYPE {
    PT1_LEAF  = 0,
    PT1_CHAIN = 1,
    PT1_NODE  = 2,
    PT1_SAVED = 3,
    PT1_UNDEF = 4,
};

enum PT2_TYPE {
    PT2_LEAF  = PT1_LEAF,
    PT2_CHAIN = PT1_CHAIN,
    PT2_NODE  = PT1_NODE,
};

struct POS_TREE1 { // pos-tree (stage 1)
    uchar      flags;
    POS_TREE1 *father;

    typedef PT1_TYPE TYPE;

    static TYPE flag_2_type[256];
    static void init_static();

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

    void set_type(TYPE type) {
        // sets user bits to zero
        pt_assert(type != PT1_UNDEF && type != PT1_SAVED); // does not work for saved nodes (done manually)
        flags = type<<FLAG_FREE_BITS;
    }
    TYPE get_type() const { return flag_2_type[flags]; }

    bool is_node() const { return get_type() == PT1_NODE; }
    bool is_leaf() const { return get_type() == PT1_LEAF; }
    bool is_chain() const { return get_type() == PT1_CHAIN; }
    bool is_saved() const { return get_type() == PT1_SAVED; }
} __attribute__((packed));

struct POS_TREE2 { // pos-tree (stage 2)
    uchar flags;

    typedef PT2_TYPE TYPE;

    static TYPE flag_2_type[256];
    static void init_static();

    const char *udata() const { return ((const char*)this)+sizeof(*this); }
    char *udata() { return ((char*)this)+sizeof(*this); }

    void set_type(TYPE type) { flags = type<<FLAG_FREE_BITS; } // sets user bits to zero
    TYPE get_type() const { return flag_2_type[flags]; }

    bool is_node() const { return get_type() == PT2_NODE; }
    bool is_leaf() const { return get_type() == PT2_LEAF; }
    bool is_chain() const { return get_type() == PT2_CHAIN; }
} __attribute__((packed));


#if defined(ARB_64)
#define SHORT_CHAIN_HEADER_ELEMS 4
#else // !defined(ARB_64)
#define SHORT_CHAIN_HEADER_ELEMS 3
#endif

#define SHORT_CHAIN_HEADER_FLAG_BIT  (1<<4)
#define SHORT_CHAIN_HEADER_SIZE_MASK 0x07

STATIC_ASSERT(SHORT_CHAIN_HEADER_SIZE_MASK >= SHORT_CHAIN_HEADER_ELEMS);
STATIC_ASSERT((SHORT_CHAIN_HEADER_SIZE_MASK & SHORT_CHAIN_HEADER_FLAG_BIT) == 0);

struct PT_short_chain_header {
    unsigned name[SHORT_CHAIN_HEADER_ELEMS];
    unsigned abspos[SHORT_CHAIN_HEADER_ELEMS];
};

struct PT_long_chain_header {
    unsigned  entries;
    unsigned  memsize;
    unsigned  memused;
    unsigned  refabspos;
    unsigned  lastname;
    char     *entrymem;
};

STATIC_ASSERT(sizeof(PT_long_chain_header) >= sizeof(PT_short_chain_header)); // SHORT_CHAIN_HEADER_ELEMS is too big
STATIC_ASSERT((sizeof(PT_short_chain_header)/SHORT_CHAIN_HEADER_ELEMS*(SHORT_CHAIN_HEADER_ELEMS+1)) > sizeof(PT_long_chain_header)); // SHORT_CHAIN_HEADER_ELEMS is too small

inline size_t PT_node_size(POS_TREE2 *node) { // @@@ become member
    size_t size = 1; // flags
    if ((node->flags & IS_SINGLE_BRANCH_NODE) == 0) {
        UINT sec = (uchar)*node->udata(); // read second byte for charshort/shortlong info
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

template <> inline POS_TREE2 *PT_read_son<POS_TREE2>(POS_TREE2 *node, PT_base base) { // stage 2 (no father)
    pt_assert_stage(STAGE2);
    pt_assert(node->is_node());

    if (node->flags & IS_SINGLE_BRANCH_NODE) {
        if (base != (node->flags & 0x7)) return NULL; // no son
        long i    = (node->flags >> 3)&0x7;      // this son
        if (!i) i = 1; else i+=2;                // offset mapping
        pt_assert(i >= 0);
        return (POS_TREE2 *)(((char *)node)-i);
    }
    if (!((1<<base) & node->flags)) { // bit not set
        return NULL;
    }

    UINT sec  = (uchar)*node->udata();   // read second byte for charshort/shortlong info
    long i    = PT_GLOBAL.count_bits[base][node->flags];
    i        += PT_GLOBAL.count_bits[base][sec];

    char *sons = node->udata()+1;

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
                i = PT_read_long(sons+offset);
            }
            else {                                              // int
                i = PT_read_int(sons+offset);
            }
        }

    }
    else {
        if (sec & INT_SONS) {                                   // int/short
            UINT offset = i+i;
            if ((1<<base) & sec) {                              // int
                i = PT_read_int(sons+offset);
            }
            else {                                              // short
                i = PT_read_short(sons+offset);
            }
        }
        else {                                                  // short/char
            UINT offset = i;
            if ((1<<base) & sec) {                              // short
                i = PT_read_short(sons+offset);
            }
            else {                                              // char
                i = PT_read_char(sons+offset);
            }
        }
    }
#else
    if (sec & LONG_SONS) {
        UINT offset = i+i;
        if ((1<<base) & sec) {
            i = PT_read_int(sons+offset);
        }
        else {
            i = PT_read_short(sons+offset);
        }
    }
    else {
        UINT offset = i;
        if ((1<<base) & sec) {
            i = PT_read_short(sons+offset);
        }
        else {
            i = PT_read_char(sons+offset);
        }
    }
#endif
    pt_assert(i >= 0);
    return (POS_TREE2 *)(((char*)node)-i);
}

template<> inline POS_TREE1 *PT_read_son<POS_TREE1>(POS_TREE1 *node, PT_base base) {
    pt_assert_stage(STAGE1);
    if (!((1<<base) & node->flags)) return NULL;   // bit not set
    base = (PT_base)PT_GLOBAL.count_bits[base][node->flags];
    return PT_read_pointer<POS_TREE1>(node->udata() + sizeof(PT_PNTR)*base);
}

inline size_t PT_leaf_size(POS_TREE2 *node) { // @@@ become member
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
    {}

    int get_rel_pos() const { return rpos; }

    void set_position(int abs_pos, int rel_pos) {
        set_abs_pos(abs_pos);
        rpos = rel_pos;
        pt_assert(has_valid_positions());
    }

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

inline const char *PT_READ_CHAIN_ENTRY_stage_2(const char *ptr, int refabspos, AbsLoc& loc) {
    // Caution: 'name' (of AbsLoc) has to be initialized before first call and shall not be modified between calls

    pt_assert_stage(STAGE2);

    uint_8  has_main_apos;
    uint_32 name = loc.get_name() + read_nat_with_reserved_bits<1>(ptr, has_main_apos);

    loc = AbsLoc(name, has_main_apos ? refabspos : PT_read_compact_nat(ptr));

    return ptr;
}

inline char *PT_WRITE_CHAIN_ENTRY(char *ptr, int refabspos, int name, const int apos) {
    pt_assert_stage(STAGE1);
    bool has_main_apos = (apos == refabspos);
    write_nat_with_reserved_bits<1>(ptr, name, has_main_apos);
    if (!has_main_apos) PT_write_compact_nat(ptr, apos);
    return ptr;
}

// -----------------------
//      ChainIterators

class ChainIteratorStage1 : virtual Noncopyable {
    bool is_short;
    union _U {
        struct _S {
            const PT_short_chain_header *header;
            int                          next_entry;

            void set_loc(AbsLoc& location) {
                pt_assert(next_entry<SHORT_CHAIN_HEADER_ELEMS);
                location = AbsLoc(header->name[next_entry], header->abspos[next_entry]);
                ++next_entry;
            }
        } S;
        struct _L {
            int         refabspos;
            const char *read_pos;

            void set_loc(AbsLoc& location) {
                uint_8  has_refabspos;
                uint_32 name   = location.get_name()+read_nat_with_reserved_bits<1>(read_pos, has_refabspos);
                uint_32 abspos = has_refabspos ? refabspos : PT_read_compact_nat(read_pos);
                location = AbsLoc(name, abspos);
            }
        } L;
    } iter;

    AbsLoc loc;
    int    elements_ahead;

    bool at_end_of_chain() const { return elements_ahead<0; }
    void set_loc_from_chain() {
        pt_assert(!at_end_of_chain());
        if (elements_ahead>0) {
            if (is_short) iter.S.set_loc(loc);
            else          iter.L.set_loc(loc);
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
    typedef POS_TREE1 POS_TREE_TYPE;

    ChainIteratorStage1(const POS_TREE1 *node)
        : loc(0, 0)
    {
        pt_assert_stage(STAGE1);
        pt_assert(node->is_chain());

        is_short = node->flags & SHORT_CHAIN_HEADER_FLAG_BIT;
        if (is_short) {
            elements_ahead    = node->flags & SHORT_CHAIN_HEADER_SIZE_MASK;
            iter.S.next_entry = 0;
            iter.S.header     = reinterpret_cast<const PT_short_chain_header*>(node->udata());
        }
        else {
            const PT_long_chain_header *header = reinterpret_cast<const PT_long_chain_header*>(node->udata());

            elements_ahead = header->entries;

            iter.L.refabspos = header->refabspos;
            iter.L.read_pos  = header->entrymem;
        }

        set_loc_from_chain();
    }

    operator bool() const { return !at_end_of_chain(); }
    const AbsLoc& at() const { return loc; }
    const ChainIteratorStage1& operator++() { inc(); return *this; } // prefix-inc

    int get_elements_ahead() const { return elements_ahead; }

    int get_refabspos() const {
        return is_short
            ? iter.S.header->abspos[0] // @@@ returns any abspos, optimize for SHORT_CHAIN_HEADER_ELEMS >= 3 only
            : iter.L.refabspos;
    }
};

class ChainIteratorStage2 : virtual Noncopyable {
    const char *data;
    AbsLoc      loc;

    int refabspos;
    int elements_ahead;

    bool at_end_of_chain() const { return elements_ahead<0; }
    void set_loc_from_chain() {
        if (elements_ahead>0) {
            data = PT_READ_CHAIN_ENTRY_stage_2(data, refabspos, loc);
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
    typedef POS_TREE2 POS_TREE_TYPE;

    ChainIteratorStage2(const POS_TREE2 *node)
        : data(node->udata()),
          loc(0, 0) // init name with 0 (needed for chain reading in STAGE2)
    {
        pt_assert_stage(STAGE2);
        pt_assert(node->is_chain());

        if (node->flags&1) { refabspos = PT_read_int(data);   data += 4; }
        else               { refabspos = PT_read_short(data); data += 2; }

        elements_ahead = PT_read_compact_nat(data);

        pt_assert(elements_ahead>0); // chain cant be empty
        set_loc_from_chain();
    }

    operator bool() const { return !at_end_of_chain(); }
    const AbsLoc& at() const { return loc; }
    const ChainIteratorStage2& operator++() { inc(); return *this; } // prefix-inc

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

template<typename T> int PT_forwhole_chain(POS_TREE2 *node, T& func) {
    pt_assert_stage(STAGE2);
    int error = 0;
    for (ChainIteratorStage2 entry(node); entry && !error; ++entry) {
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
