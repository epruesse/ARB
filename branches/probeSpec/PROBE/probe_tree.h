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

#define PTM_magic             0xf4
#define PTM_TABLE_SIZE        (1024*256)
#define PTM_ALIGNED           1
#define PTM_LD_ALIGNED        0
#define PTM_MAX_TABLES        256 // -- ralf testing
#define PTM_MAX_SIZE          (PTM_MAX_TABLES*PTM_ALIGNED)
#define PT_CHAIN_END          0xff
#define PT_CHAIN_NTERM        250
#define PT_SHORT_SIZE         0xffff
#define PT_BLOCK_SIZE         0x800
#define PT_INIT_CHAIN_SIZE    20

typedef void * PT_PNTR;

extern struct PTM_struct {
    char         *data;
    int           size;
    long          allsize;
    char         *tables[PTM_MAX_TABLES+1];
#ifdef PTM_DEBUG
    long          debug[PTM_MAX_TABLES+1];
#endif
    PT_NODE_TYPE  flag_2_type[256];
    //
    void **alloc_ptr;
    unsigned long alloc_counter;
    unsigned long alloc_array_size;
} PTM;

extern char PT_count_bits[PT_B_MAX+1][256]; // returns how many bits are set
        // e.g. PT_count_bits[3][n] is the number of the 3 lsb bits

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
    [PT_PNTR father]        if main->mode
    [PT_PNTR son0]          if bit[0]
...
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
#define PT_LEAF_SIZE(leaf)       (1+sizeof(PT_PNTR)+6+2*PT_count_bits[3][leaf->flags])
#define PT_EMPTY_CHAIN_SIZE      (1+sizeof(PT_PNTR)+2+sizeof(PT_PNTR)) // tag father apos first_elem
#define PT_EMPTY_NODE_SIZE       (1+sizeof(PT_PNTR)) // tag father
#define PT_NODE_COUNT_SONS(leaf) PT_count_bits[3][leaf->flags];
#define PT_NODE_SIZE(node, size) size = PT_EMPTY_NODE_SIZE + sizeof(PT_PNTR)*PT_count_bits[PT_B_MAX][node->flags]

// ----------------------------
//      Read and write type

#define PT_GET_TYPE(pt)     (PTM.flag_2_type[pt->flags])
#define PT_SET_TYPE(pt, i, j) (pt->flags = (i<<6)+j)

// ----------------------
//      bswap for OSX

#if defined(DARWIN)

static inline unsigned short bswap_16(unsigned short x) {
    return (x>>8) | (x<<8);
}

static inline unsigned int bswap_32(unsigned int x) {
    return (bswap_16(x&0xffff)<<16) | (bswap_16(x>>16));
}

static inline unsigned long long bswap_64(unsigned long long x) {
    return (((unsigned long long)bswap_32(x&0xffffffffull))<<32) | (bswap_32(x>>32));
}

#else
#include <byteswap.h>
#endif // DARWIN

// ------------------------------------------------------------
// Note about bswap as used here:
//
// * MSB has to be at start of written byte-chain, cause the most significant bit is used to separate
//   between INT and SHORT
//
// * To use PT-server on a big-endian system it has to be skipped

// ---------------------------------
//      Read and write to memory

#define PT_READ_INT(ptr, my_int_i)                                      \
    do {                                                                \
        unsigned int *uiptr = (unsigned int*)(ptr);                     \
        (my_int_i)=(unsigned int)bswap_32(*uiptr);                      \
    } while (0)

#define PT_WRITE_INT(ptr, my_int_i)                                     \
    do {                                                                \
        unsigned int *uiptr = (unsigned int*)(ptr);                     \
        *uiptr              = bswap_32((unsigned int)(my_int_i));       \
    } while (0)

#define PT_READ_SHORT(ptr, my_int_i)                    \
    do {                                                \
        (my_int_i) = bswap_16(*(unsigned short*)(ptr)); \
    } while (0)

#define PT_WRITE_SHORT(ptr, my_int_i)                                   \
    do {                                                                \
        unsigned short *usptr = (unsigned short*)(ptr);                 \
        *usptr                = bswap_16((unsigned short)(my_int_i));   \
    } while (0)

#define PT_WRITE_CHAR(ptr, my_int_i) do { *(unsigned char *)(ptr) = my_int_i; } while (0)

#define PT_READ_CHAR(ptr, my_int_i) do { my_int_i = *(unsigned char *)(ptr); } while (0)



#ifdef ARB_64

STATIC_ASSERT(sizeof(void*) == sizeof(unsigned long));

# define PT_READ_PNTR(ptr, my_int_i)                            \
    do {                                                        \
        pt_assert(sizeof(my_int_i) == 8);                       \
        unsigned long *ulptr = (unsigned long*)(ptr);           \
        (my_int_i)           = (unsigned long)bswap_64(*ulptr); \
    } while (0)


# define PT_WRITE_PNTR(ptr, my_int_i)                                   \
    do {                                                                \
        unsigned long *ulptr = (unsigned long*)(ptr);                   \
        *ulptr               = bswap_64((unsigned long)(my_int_i));     \
    } while (0)


#else
// not ARB_64

STATIC_ASSERT(sizeof(void*) == sizeof(unsigned int));

# define PT_READ_PNTR(ptr, my_int_i) PT_READ_INT(ptr, my_int_i)
# define PT_WRITE_PNTR(ptr, my_int_i) PT_WRITE_INT(ptr, my_int_i)

#endif



#define PT_WRITE_NAT(ptr, i)                    \
    do {                                        \
        pt_assert(i >= 0);                      \
        if (i >= 0x7FFE)                        \
        {                                       \
            PT_WRITE_INT(ptr, i|0x80000000);    \
            ptr += sizeof(int);                 \
        }                                       \
        else                                    \
        {                                       \
            PT_WRITE_SHORT(ptr, i);             \
            ptr += sizeof(short);               \
        }                                       \
    } while (0)

#define PT_READ_NAT(ptr, i)                                             \
    do {                                                                \
        if (*ptr & 0x80) {                                              \
            PT_READ_INT(ptr, i); ptr += sizeof(int); i &= 0x7fffffff;   \
        }                                                               \
        else {                                                          \
            PT_READ_SHORT(ptr, i); ptr += sizeof(short);                \
        }                                                               \
    } while (0)


inline const char *PT_READ_CHAIN_ENTRY(const char* ptr, int mainapos, int *name, int *apos, int *rpos) {
    // Caution: 'name' has to be initialized before first call and shall not be modified between calls
 
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
    unsigned char *wcep = (unsigned char *)ptr;
    int  isapos;
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

    if (apos == mainapos) isapos = 0; else isapos = 0x80;

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
// calculate the index of the pointer in a node

inline POS_TREE *PT_read_son(POS_TREE *node, PT_BASES base)
{
    long  i;
    UINT  sec;
    PTM2 *ptmain = psg.ptmain;
    if (ptmain->stage3) {       // stage 3  no father
        if (node->flags & IS_SINGLE_BRANCH_NODE) {
            if (base != (node->flags & 0x7)) return NULL;  // no son
            i = (node->flags >> 3)&0x7;         // this son
            if (!i) i = 1; else i+=2;           // offset mapping
            pt_assert(i >= 0);
            return (POS_TREE *)(((char *)node)-i);
        }
        if (!((1<<base) & node->flags)) return NULL;   // bit not set
        sec = (uchar)node->data;    // read second byte for charshort/shortlong info
        i = PT_count_bits[base][node->flags];
        i += PT_count_bits[base][sec];
#ifdef ARB_64
        if (sec & LONG_SONS) {
            if (sec & INT_SONS) {                                   // undefined -> error
                GBK_terminate("Your pt-server search tree is corrupt! You can not use it anymore.\n"
                              "Error: ((sec & LONG_SON) && (sec & INT_SONS)) == true\n"
                              "       this combination of both flags is not implemented\n");
            }
            else {                                                // long/int
#ifdef DEBUG
                printf("Warning: A search tree of this size is not tested.\n");
                printf("         (sec & LONG_SON) == true\n");
#endif
                UINT offset = 4 * i;
                if ((1<<base) & sec) {              // long
                    STATIC_ASSERT(sizeof(PT_PNTR) == 8); // 64-bit necessary
                    PT_READ_PNTR((&node->data+1)+offset, i);
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
    else {          // stage 1 or 2 ->father
        if (!((1<<base) & node->flags)) return NULL;   // bit not set
        base = (PT_BASES)PT_count_bits[base][node->flags];
        PT_READ_PNTR((&node->data)+sizeof(PT_PNTR)*base+ptmain->mode, i);
        return (POS_TREE *)(i+ptmain->data_start); // ptmain->data_start == 0x00 in stage 1
    }
}

inline POS_TREE *PT_read_son_stage_1(POS_TREE *node, PT_BASES base) {
    if (!((1<<base) & node->flags)) return NULL;   // bit not set
    base = (PT_BASES)PT_count_bits[base][node->flags];
    long i;
    PT_READ_PNTR((&node->data)+sizeof(PT_PNTR)*base+psg.ptmain->mode, i);
    return (POS_TREE *)(i+psg.ptmain->data_start); // psg.ptmain->data_start == 0x00 in stage 1
}

inline PT_NODE_TYPE PT_read_type(POS_TREE *node)
{
    return (PT_NODE_TYPE)PT_GET_TYPE(node);
}

struct DataLoc {
    int name; // index into psg.data[], aka as species id
    int apos;
    int rpos; // position in data

    void init(const char ** data, int pos) {
        *data = PT_READ_CHAIN_ENTRY(*data, pos, &name, &apos, &rpos);
    }
    void init(POS_TREE *node) {
        pt_assert(PT_read_type(node) == PT_NT_LEAF);
        char *data = (&node->data)+psg.ptmain->mode;
        if (node->flags&1) { PT_READ_INT(data, name); data += 4; } else { PT_READ_SHORT(data, name); data += 2; }
        if (node->flags&2) { PT_READ_INT(data, rpos); data += 4; } else { PT_READ_SHORT(data, rpos); data += 2; }
        if (node->flags&4) { PT_READ_INT(data, apos); data += 4; } else { PT_READ_SHORT(data, apos); /*data += 2;*/ }

        pt_assert(name >= 0);
        pt_assert(apos >= 0);
        pt_assert(rpos >= 0);
    }

    DataLoc(int name_, int apos_, int rpos_) : name(name_), apos(apos_), rpos(rpos_) {}
    DataLoc(POS_TREE *pt) { init(pt); }
    DataLoc(const char ** data, int pos) { name = 0; init(data, pos); }

    const probe_input_data& get_pid() const { pt_assert(name >= 0 && name<psg.data_count); return psg.data[name]; }
    const char *get_data() const { return get_pid().get_data(); }
    PT_BASES operator[](int offset) const { return PT_BASES(get_data()[rpos+offset]); }

    int restlength() const { return get_pid().get_size()-rpos; }
    bool is_shorther_than(int offset) const { return offset >= restlength(); }

#if defined(DEBUG)
    void dump(FILE *fp) const {
        fprintf(fp, "          apos=%6i  rpos=%6i  name=%6i='%s'\n", apos, rpos, name, psg.data[name].get_name());
    }
#endif
};

template<typename T>
int PT_forwhole_chain(POS_TREE *node, T func) {
    pt_assert(PT_read_type(node) == PT_NT_CHAIN);

    const char *data = (&node->data) + psg.ptmain->mode;
    int         pos;

    if (node->flags&1) {
        PT_READ_INT(data, pos);
        data += 4;
    }
    else {
        PT_READ_SHORT(data, pos);
        data += 2;
    }

    int     error = 0;
    DataLoc location(&data, pos);
    while (location.name>=0) {
        error = func(location);
        if (error) break;
        location.init(&data, pos);
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