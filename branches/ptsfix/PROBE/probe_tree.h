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

#define PT_CHAIN_END          0xff
#define PT_CHAIN_NTERM        250
#define PT_SHORT_SIZE         0xffff
#define PT_BLOCK_SIZE         0x800
#define PT_INIT_CHAIN_SIZE    20

struct pt_global {
    PT_NODE_TYPE flag_2_type[256];
    char         count_bits[PT_B_MAX+1][256]; // returns how many bits are set (e.g. PT_count_bits[3][n] is the number of the 3 lsb bits)
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
                -1 not allowed
    short/int       rel pos
                if bit[15] -> the next bytes are the apos else use ref_abs_pos
                if bit[14] -> this(==rel_pos) is integer
    [short/int]     [apos short if bit[15] = 0]
]
    char -1         end flag

only few functions can be used, when the tree is reloaded (stage 3):
    PT_read_type
    PT_read_son
    PT_read_xpos
    PT_read_name
    PT_forwhole_chain
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

#define PT_EMPTY_LEAF_SIZE       (1+sizeof(PT_PNTR)+6) // tag father name rel apos
#define PT_LEAF_SIZE(leaf)       (1+sizeof(PT_PNTR)+6+2*PT_GLOBAL.count_bits[3][leaf->flags])
#define PT_SHORT_CHAIN_HEAD_SIZE (1+sizeof(PT_PNTR)+2+sizeof(PT_PNTR)) // tag father apos first_elem
#define PT_LONG_CHAIN_HEAD_SIZE  (PT_SHORT_CHAIN_HEAD_SIZE+2) // apos uses 4 byte here
#define PT_EMPTY_NODE_SIZE       (1+sizeof(PT_PNTR))   // tag father

#define PT_MIN_CHAIN_ENTRY_SIZE  (sizeof(PT_PNTR)+3*sizeof(short)) // depends on PT_WRITE_NAT
#define PT_MAX_CHAIN_ENTRY_SIZE  (sizeof(PT_PNTR)+3*sizeof(int))

#define PT_NODE_WITHSONS_SIZE(sons) (PT_EMPTY_NODE_SIZE+sizeof(PT_PNTR)*(sons))

#define PT_NODE_SON_COUNT(node) (PT_GLOBAL.count_bits[PT_B_MAX][node->flags])
#define PT_NODE_SIZE(node)      PT_NODE_WITHSONS_SIZE(PT_NODE_SON_COUNT(node))

// ----------------------------
//      Read and write type

#define FLAG_TYPE_BITS 2
#define FLAG_FREE_BITS (8-FLAG_TYPE_BITS)

inline int checked_lower_bits(int bits) {
    pt_assert(bits >= 0 && bits<(1<<FLAG_FREE_BITS));
    return bits;
}

#define PT_GET_TYPE(pt)            (PT_GLOBAL.flag_2_type[(pt)->flags])
#define PT_SET_TYPE(pt,type,lbits) ((pt)->flags = ((type)<<FLAG_FREE_BITS)+checked_lower_bits(lbits))

inline const char *PT_READ_CHAIN_ENTRY_stage_1(const char *entry, int *name, int *apos, int *rpos, int *entry_size) {
    pt_assert_stage(STAGE1);

    if (entry) {
        const char *rp = entry + sizeof(PT_PNTR);

        PT_READ_NAT(rp, *name);
        PT_READ_NAT(rp, *rpos);
        PT_READ_NAT(rp, *apos);

        *entry_size = rp-entry;

        long next;
        PT_READ_PNTR(entry, next);
        return (char*)next;
    }

    *name = -1;
    return NULL;
}

inline const char *PT_READ_CHAIN_ENTRY_stage_3(const char *ptr, int mainapos, int *name, int *apos, int *rpos) {
    // Caution: 'name' has to be initialized before first call and shall not be modified between calls

    pt_assert_stage(STAGE3);

    *apos = 0;
    *rpos = 0;

    unsigned char *rcep = (unsigned char*)ptr;
    unsigned int   rcei = (*rcep);

    if (rcei==PT_CHAIN_END) {
        *name = -1;
        ptr++;
    }
    else {
        if (rcei&0x80) {
            if (rcei&0x40) {
                PT_READ_INT(rcep, rcei); rcep+=4; rcei &= 0x3fffffff;
            }
            else {
                PT_READ_SHORT(rcep, rcei); rcep+=2; rcei &= 0x3fff;
            }
        }
        else {
            rcei &= 0x7f; rcep++;
        }
        *name += rcei;
        rcei   = (*rcep);

        bool isapos = rcei&0x80;

        if (rcei&0x40) {
            PT_READ_INT(rcep, rcei); rcep+=4; rcei &= 0x3fffffff;
        }
        else {
            PT_READ_SHORT(rcep, rcei); rcep+=2; rcei &= 0x3fff;
        }
        *rpos = (int)rcei;
        if (isapos) {
            rcei = (*rcep);
            if (rcei&0x80) {
                PT_READ_INT(rcep, rcei); rcep+=4; rcei &= 0x7fffffff;
            }
            else {
                PT_READ_SHORT(rcep, rcei); rcep+=2; rcei &= 0x7fff;
            }
            *apos = (int)rcei;
        }
        else {
            *apos = (int)mainapos;
        }
        ptr = (char *)rcep;
    }

    return ptr;
}

inline char *PT_WRITE_CHAIN_ENTRY(const char * const ptr, const int mainapos, int name, const int apos, const int rpos) { // stage 1
    pt_assert_stage(STAGE1);
    unsigned char *wcep = (unsigned char *)ptr;
    if (name < 0x7f) {      // write the name
        *(wcep++) = name;
    }
    else if (name <0x3fff) {
        name |= 0x8000;
        PT_WRITE_SHORT(wcep, name);
        wcep += 2;
    }
    else {
        name |= 0xc0000000;
        PT_WRITE_INT(wcep, name);
        wcep += 4;
    }

    int isapos = (apos == mainapos) ? 0 : 0x80;
    if (rpos < 0x3fff) {        // write the rpos
        // 0x7fff, mit der rpos vorher verglichen wurde war zu gross
        PT_WRITE_SHORT(wcep, rpos);
        *wcep |= isapos;
        wcep += 2;
    }
    else {
        PT_WRITE_INT(wcep, rpos);
        *wcep |= 0x40+isapos;
        wcep += 4;
    }

    if (isapos) {           // write the apos
        if (apos < 0x7fff) {
            PT_WRITE_SHORT(wcep, apos);
            wcep += 2;
        }
        else {
            PT_WRITE_INT(wcep, apos);
            *wcep |= 0x80;
            wcep += 4;
        }
    }
    return (char *)wcep;
}

inline const char *node_data_start(const POS_TREE *node) { return &node->data + psg.ptdata->get_offset(); }
inline char *node_data_start(POS_TREE *node) { return const_cast<char*>(node_data_start(const_cast<const POS_TREE*>(node))); }

inline POS_TREE *PT_read_son_stage_3(POS_TREE *node, PT_BASES base) { // stage 3 (no father)
    pt_assert_stage(STAGE3);
    
    if (node->flags & IS_SINGLE_BRANCH_NODE) {
        if (base != (node->flags & 0x7)) return NULL;  // no son
        long i = (node->flags >> 3)&0x7;         // this son
        if (!i) i = 1; else i+=2;           // offset mapping
        pt_assert(i >= 0);
        return (POS_TREE *)(((char *)node)-i);
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
                COMPILE_ASSERT(sizeof(PT_PNTR) == 8); // 64-bit necessary
                PT_READ_LONG((&node->data+1)+offset, i);
            }
            else {                                              // int
                PT_READ_INT((&node->data+1)+offset, i);
            }
        }

    }
    else {
        if (sec & INT_SONS) {                                   // int/short
            UINT offset = i+i;
            if ((1<<base) & sec) {                              // int
                PT_READ_INT((&node->data+1)+offset, i);
            }
            else {                                              // short
                PT_READ_SHORT((&node->data+1)+offset, i);
            }
        }
        else {                                                  // short/char
            UINT offset = i;
            if ((1<<base) & sec) {                              // short
                PT_READ_SHORT((&node->data+1)+offset, i);
            }
            else {                                              // char
                PT_READ_CHAR((&node->data+1)+offset, i);
            }
        }
    }
#else
    if (sec & LONG_SONS) {
        UINT offset = i+i;
        if ((1<<base) & sec) {
            PT_READ_INT((&node->data+1)+offset, i);
        }
        else {
            PT_READ_SHORT((&node->data+1)+offset, i);
        }
    }
    else {
        UINT offset = i;
        if ((1<<base) & sec) {
            PT_READ_SHORT((&node->data+1)+offset, i);
        }
        else {
            PT_READ_CHAR((&node->data+1)+offset, i);
        }
    }
#endif
    pt_assert(i >= 0);
    return (POS_TREE *)(((char*)node)-i);
}

inline POS_TREE *PT_read_son_stage_1(POS_TREE *node, PT_BASES base) {
    pt_assert_stage(STAGE1);
    if (!((1<<base) & node->flags)) return NULL;   // bit not set
    base = (PT_BASES)PT_GLOBAL.count_bits[base][node->flags];
    long i;
    PT_READ_PNTR(node_data_start(node) + sizeof(PT_PNTR)*base, i);
    return psg.ptdata->rel2abs(i);
}

inline POS_TREE *PT_read_son(POS_TREE *node, PT_BASES base) {
    PT_data *ptdata = psg.ptdata;
    if (ptdata->get_stage() == STAGE3) {
        return PT_read_son_stage_3(node, base);
    }
    else {
        return PT_read_son_stage_1(node, base);
    }
}

inline PT_NODE_TYPE PT_read_type(const POS_TREE *node) {
    return (PT_NODE_TYPE)PT_GET_TYPE(node);
}

inline POS_TREE *PT_read_father(POS_TREE *node) {
    pt_assert_stage(STAGE1); // in STAGE3 POS_TREE has no father
    pt_assert(PT_read_type(node) != PT_NT_SAVED); // saved nodes do not know their father
    long p;
    PT_READ_PNTR(&node->data, p);

    POS_TREE *father = (POS_TREE*)p;
#if defined(ASSERTION_USED)
    if (father) pt_assert(PT_read_type(father) == PT_NT_NODE);
#endif
    return father;
}


struct DataLoc {
    int name; // index into psg.data[], aka as species id
    int apos;
    int rpos; // position in data

#if defined(ASSERTION_USED)
    bool has_valid_name() const { return name >= 0 && name < psg.data_count; }
#endif

    void init_from_leaf(POS_TREE *node) {
        pt_assert(PT_read_type(node) == PT_NT_LEAF);
        const char *data = node_data_start(node);
        if (node->flags&1) { PT_READ_INT(data, name); data += 4; } else { PT_READ_SHORT(data, name); data += 2; }
        if (node->flags&2) { PT_READ_INT(data, rpos); data += 4; } else { PT_READ_SHORT(data, rpos); data += 2; }
        if (node->flags&4) { PT_READ_INT(data, apos); data += 4; } else { PT_READ_SHORT(data, apos); /*data += 2;*/ }

        pt_assert(has_valid_name());
        pt_assert(apos >= 0);
        pt_assert(rpos >= 0);
    }

    explicit DataLoc() : name(0), apos(0), rpos(0) {}
    DataLoc(int name_, int apos_, int rpos_) : name(name_), apos(apos_), rpos(rpos_) {}
    DataLoc(POS_TREE *pt) { init_from_leaf(pt); }

    const probe_input_data& get_pid() const { pt_assert(has_valid_name()); return psg.data[name]; }
    PT_BASES operator[](int offset) const { return PT_BASES(get_pid().base_at(rpos+offset)); }

    int restlength() const { return get_pid().get_size()-rpos; }
    bool is_shorther_than(int offset) const { return offset >= restlength(); }

#if defined(DEBUG)
    void dump(FILE *fp) const {
        fprintf(fp, "          apos=%6i  rpos=%6i  name=%6i='%s'\n", apos, rpos, name, psg.data[name].get_name());
        fflush(fp);
    }
#endif
};

inline bool operator == (const DataLoc& loc1, const DataLoc& loc2) {
    return loc1.name == loc2.name && loc1.apos == loc2.apos && loc1.rpos == loc2.rpos;
}

class ChainIterator : virtual Noncopyable {
    const char *data;
    DataLoc     loc;

    const char *last_data; // @@@ only used in STAGE1 - split class (one for each stage)? 
    int         last_size; // @@@ only used in STAGE1
    int         mainapos;  // @@@ only used in STAGE3

    bool at_end_of_chain() const { return loc.name == -1; }
    void set_loc_from_chain() {
        last_data = data;
        if (psg.ptdata->get_stage() == STAGE1) {
            data = PT_READ_CHAIN_ENTRY_stage_1(data, &loc.name, &loc.apos, &loc.rpos, &last_size);
        }
        else {
            data = PT_READ_CHAIN_ENTRY_stage_3(data, mainapos, &loc.name, &loc.apos, &loc.rpos);
        }
        pt_assert(at_end_of_chain() || loc.has_valid_name());
    }
    void inc() {
        pt_assert(!at_end_of_chain());
        set_loc_from_chain();
    }

public:
    ChainIterator(const POS_TREE *node)
        : data(node_data_start(node)),
          loc(),
          last_size(-1)
    {
        pt_assert(PT_read_type(node) == PT_NT_CHAIN);

        if (node->flags&1) {
            PT_READ_INT(data, mainapos);
            data += 4;
        }
        else {
            PT_READ_SHORT(data, mainapos);
            data += 2;
        }

        if (psg.ptdata->get_stage() == STAGE1) {
            long first_entry;
            PT_READ_PNTR(data, first_entry);
            data = (char*)first_entry;
        }
        set_loc_from_chain();
    }

    operator bool() const { return !at_end_of_chain(); }
    const DataLoc& at() const { return loc; }
    const ChainIterator& operator++() { inc(); return *this; } // prefix-inc

    const char *memory() const { return last_data; }
    int memsize() const { return last_size; }
};

template<typename T>
int PT_forwhole_chain(POS_TREE *node, T func) {
    int error = 0;
    for (ChainIterator entry(node); entry && !error; ++entry) {
        error = func(entry.at());
    }
    return error;
}

template<typename T>
int PT_withall_tips(POS_TREE *node, T func) {
    // like PT_forwhole_chain, but also can handle leafs
    PT_NODE_TYPE type = PT_read_type(node);
    if (type == PT_NT_LEAF) {
        return func(DataLoc(node));
    }

    pt_assert(type == PT_NT_CHAIN);
    return PT_forwhole_chain(node, func);
}

#if defined(DEBUG)
struct PTD_chain_print { int operator()(const DataLoc& loc) { loc.dump(stdout); return 0; } };
#endif

#else
#error probe_tree.h included twice
#endif // PROBE_TREE_H
