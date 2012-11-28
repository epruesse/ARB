// =============================================================== //
//                                                                 //
//   File      : PT_prefixtree.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "probe.h"
#include "probe_tree.h"
#include "pt_prototypes.h"
#include "PT_mem.h"

#include <arb_file.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <climits>

struct pt_global PT_GLOBAL;

inline bool locs_in_chain_order(const DataLoc& loc1, const DataLoc& loc2) { 
    pt_assert_stage(STAGE1); // this order is only valid in STAGE1 (STAGE3 has reverse order)

    if (loc1.get_name() < loc2.get_name()) {
#if defined(DEBUG)
        fprintf(stderr, "invalid chain: loc1.name<loc2.name (%i<%i)\n", loc1.get_name(), loc2.get_name());
#endif
        return false;
    }
    if (loc1.get_name() == loc2.get_name()) {
        if (loc1.get_rel_pos() >= loc2.get_rel_pos()) {
#if defined(DEBUG)
            fprintf(stderr, "invalid chain: loc1.rpos>=loc2.rpos (%i>=%i)\n", loc1.get_rel_pos(), loc2.get_rel_pos());
#endif
            return false;
        }
        if (loc1.get_abs_pos() >= loc2.get_abs_pos()) {
#if defined(DEBUG)
            fprintf(stderr, "invalid chain: loc1.apos>=loc2.apos (%i>=%i)\n", loc1.get_abs_pos(), loc2.get_abs_pos());
#endif
            return false;
        }
    }
    return true;
}

#if defined(PTM_DEBUG_VALIDATE_CHAINS)
inline bool PT_is_empty_chain(const POS_TREE * const node) { 
    pt_assert(PT_read_type(node) == PT_NT_CHAIN);
    return !ChainIterator(node);
}
#endif

bool PT_chain_has_valid_entries(const POS_TREE * const node) {
    pt_assert(PT_read_type(node) == PT_NT_CHAIN);

    bool ok = true;

    ChainIterator entry(node);
    if (!entry) return false;

    DataLoc last(entry.at());

    ++entry;
    while (entry && ok) {
        ok   = locs_in_chain_order(last, entry.at());
        last = entry.at();
        ++entry;
    }

    return ok;
}

PT_data::PT_data(Stage stage_)
    : stage(stage_),
      data_offset(stage == STAGE1 ? sizeof(PT_PNTR) : 0),
      data_start(0)
{}

static void init_PT_GLOBAL() {
    for (int i=0; i<256; i++) {
        if      ((i&0xe0) == 0x20) PT_GLOBAL.flag_2_type[i] = PT_NT_SAVED;
        else if ((i&0xe0) == 0x00) PT_GLOBAL.flag_2_type[i] = PT_NT_LEAF;
        else if ((i&0x80) == 0x80) PT_GLOBAL.flag_2_type[i] = PT_NT_NODE;
        else if ((i&0xe0) == 0x40) PT_GLOBAL.flag_2_type[i] = PT_NT_CHAIN;
        else                       PT_GLOBAL.flag_2_type[i] = PT_NT_UNDEF;
    }

    for (unsigned base = PT_QU; base <= PT_BASES; base++) {
        for (unsigned i=0; i<256; i++) {
            unsigned h = 0xff >> (8-base);
            h &= i;
            unsigned count = 0;
            for (unsigned j=0; j<8; j++) {
                if (h&1) count++;
                h = h>>1;
            }
            PT_GLOBAL.count_bits[base][i] = count;
        }
    }
}

PT_data *PT_init(Stage stage) {
    PT_data *ptdata = new PT_data(stage);
    init_PT_GLOBAL();
    pt_assert(MEM.is_clear());
    return ptdata;
}

// ------------------------------
//      functions for stage 1

static void PT_change_link_in_father(POS_TREE *father, POS_TREE *old_link, POS_TREE *new_link) {
    pt_assert_stage(STAGE1);
    long i = PT_GLOBAL.count_bits[PT_BASES][father->flags];
    for (; i>0; i--) {
        POS_TREE *link = PT_read_pointer<POS_TREE>(&father->data+sizeof(PT_PNTR)*i);
        if (link==old_link) {
            PT_write_pointer(&father->data+sizeof(PT_PNTR)*i, new_link);
            return;
        }
    }
    pt_assert(0); // father did not contain 'old_link'
}

void PT_add_to_chain(POS_TREE *node, const DataLoc& loc) {
    pt_assert_stage(STAGE1);
#if defined(PTM_DEBUG_VALIDATE_CHAINS)
    pt_assert(PT_is_empty_chain(node) || PT_chain_has_valid_entries(node));
#endif

    // insert at the beginning of list

    char *data      = node_data_start(node) + ((node->flags&1) ? 4 : 2);
    void *old_first = PT_read_void_pointer(data);

    const int MAX_CHAIN_ENTRY_SIZE = sizeof(PT_PNTR)+3*sizeof(int);

    static char  buffer[MAX_CHAIN_ENTRY_SIZE];
    char        *p = buffer;

    PT_write_pointer(p, old_first);
    p += sizeof(PT_PNTR);

    PT_WRITE_NAT(p, loc.get_name());
    PT_WRITE_NAT(p, loc.get_rel_pos());
    PT_WRITE_NAT(p, loc.get_abs_pos());

    int size = p - buffer;

    pt_assert(size <= MAX_CHAIN_ENTRY_SIZE);

    p = (char*)MEM.get(size);
    memcpy(p, buffer, size);
    PT_write_pointer(data, p);
    psg.stat.cut_offs ++;

    pt_assert_valid_chain(node);
}

inline void PT_set_father(POS_TREE *son, const POS_TREE *father) {
    pt_assert_stage(STAGE1); // otherwise there are no fathers
    pt_assert(!father || PT_read_type(father) == PT_NT_NODE);

    PT_write_pointer(&son->data, father);

    pt_assert(PT_read_father(son) == father);
}

POS_TREE *PT_change_leaf_to_node(POS_TREE *node) {
    pt_assert_stage(STAGE1);
    if (PT_GET_TYPE(node) != PT_NT_LEAF) PT_CORE;

    POS_TREE *father = PT_read_father(node);

    POS_TREE *new_elem = (POS_TREE *)MEM.get(PT_EMPTY_NODE_SIZE);
    if (father) PT_change_link_in_father(father, node, new_elem);
    MEM.put(node, PT_LEAF_SIZE(node));
    PT_SET_TYPE(new_elem, PT_NT_NODE, 0);
    PT_set_father(new_elem, father);

    return new_elem;
}

POS_TREE *PT_leaf_to_chain(POS_TREE *node) {
    pt_assert_stage(STAGE1);
    if (PT_GET_TYPE(node) != PT_NT_LEAF) PT_CORE;

    POS_TREE      *father = PT_read_father(node);
    const DataLoc  loc(node);

    int       chain_size = (loc.get_abs_pos()>PT_SHORT_SIZE) ? PT_LONG_CHAIN_HEAD_SIZE : PT_SHORT_CHAIN_HEAD_SIZE;
    POS_TREE *new_elem   = (POS_TREE *)MEM.get(chain_size);

    PT_change_link_in_father(father, node, new_elem);
    MEM.put(node, PT_LEAF_SIZE(node));
    PT_SET_TYPE(new_elem, PT_NT_CHAIN, 0);
    PT_set_father(new_elem, father);

    char *data = (&new_elem->data)+sizeof(PT_PNTR);
    if (loc.get_abs_pos()>PT_SHORT_SIZE) {
        PT_write_int(data, loc.get_abs_pos());
        data+=4;
        new_elem->flags|=1;
    }
    else {                                                      
        PT_write_short(data, loc.get_abs_pos());
        data+=2;
    }
    PT_write_pointer(data, NULL);
    PT_add_to_chain(new_elem, loc);

    return new_elem;
}

POS_TREE *PT_create_leaf(POS_TREE **pfather, PT_base base, const DataLoc& loc) {
    pt_assert_stage(STAGE1);
    if (base >= PT_BASES) PT_CORE;

    int leafsize = PT_EMPTY_LEAF_SIZE;

    if (loc.get_rel_pos()>PT_SHORT_SIZE) leafsize += 2;
    if (loc.get_abs_pos()>PT_SHORT_SIZE) leafsize += 2;
    if (loc.get_name()   >PT_SHORT_SIZE) leafsize += 2;

    POS_TREE *node = (POS_TREE *)MEM.get(leafsize);

    pt_assert(PT_read_father(node) == NULL); // cause memory is cleared

    if (pfather) {
        POS_TREE *father = *pfather;

        int       oldfathersize  = PT_NODE_SIZE(father);
        POS_TREE *new_elemfather = (POS_TREE *)MEM.get(oldfathersize + sizeof(PT_PNTR));
        PT_SET_TYPE(new_elemfather, PT_NT_NODE, 0);

        POS_TREE *gfather = PT_read_father(father);
        if (gfather) {
            PT_change_link_in_father(gfather, father, new_elemfather);
            PT_set_father(new_elemfather, gfather);
        }
        else {
            pt_assert(PT_read_father(father) == NULL);
        }

        long sbase   = sizeof(PT_PNTR);
        long dbase   = sizeof(PT_PNTR);
        int  basebit = 1 << base;

        pt_assert((father->flags&basebit) == 0); // son already exists!

        for (int i = 1; i < (1<<FLAG_FREE_BITS); i = i << 1) {
            if (i & father->flags) { // existing son
                POS_TREE *son     = PT_read_pointer<POS_TREE>(&father->data+sbase);
                int       sontype = PT_GET_TYPE(son);

                if (sontype != PT_NT_SAVED) {
                    PT_set_father(son, new_elemfather);
                }

                PT_write_pointer(&new_elemfather->data+dbase, son);

                sbase += sizeof(PT_PNTR);
                dbase += sizeof(PT_PNTR);
            }
            else if (i & basebit) { // newly created leaf
                PT_write_pointer(&new_elemfather->data+dbase, node);
                PT_set_father(node, new_elemfather);
                dbase += sizeof(PT_PNTR);
            }
        }

        new_elemfather->flags = father->flags | basebit;
        MEM.put(father, oldfathersize);
        PT_SET_TYPE(node, PT_NT_LEAF, 0); // @@@ dupped below
        *pfather = new_elemfather;
    }
    PT_SET_TYPE(node, PT_NT_LEAF, 0);

    char *dest = (&node->data) + sizeof(PT_PNTR);
    if (loc.get_name()>PT_SHORT_SIZE) {
        PT_write_int(dest, loc.get_name());
        node->flags |= 1;
        dest += 4;
    }
    else {
        PT_write_short(dest, loc.get_name());
        dest += 2;
    }
    if (loc.get_rel_pos()>PT_SHORT_SIZE) {
        PT_write_int(dest, loc.get_rel_pos());
        node->flags |= 2;
        dest += 4;
    }
    else {
        PT_write_short(dest, loc.get_rel_pos());
        dest += 2;
    }
    if (loc.get_abs_pos()>PT_SHORT_SIZE) {
        PT_write_int(dest, loc.get_abs_pos());
        node->flags |= 4;
        dest += 4;
    }
    else {
        PT_write_short(dest, loc.get_abs_pos());
        dest += 2;
    }

    return base == PT_QU ? PT_leaf_to_chain(node) : node;
}

// ------------------------------------
//      functions for stage 1: save

void PTD_clear_fathers(POS_TREE *node) {
    pt_assert_stage(STAGE1);
    PT_NODE_TYPE type = PT_read_type(node);
    if (type != PT_NT_SAVED) {
        PT_set_father(node, NULL);
        if (type == PT_NT_NODE) {
            for (int i = PT_QU; i < PT_BASES; i++) {
                POS_TREE *son = PT_read_son_stage_1(node, (PT_base)i);
                if (son) PTD_clear_fathers(son);
            }
        }
    }
}

#ifdef ARB_64
void PTD_put_longlong(FILE *out, ULONG i) {
    pt_assert(i == (unsigned long) i);
    const size_t SIZE = 8;
    COMPILE_ASSERT(sizeof(uint_big) == SIZE); // this function only works and only gets called at 64-bit

    unsigned char buf[SIZE];
    PT_write_long(buf, i);
    ASSERT_RESULT(size_t, SIZE, fwrite(buf, 1, SIZE, out));
}
#endif

void PTD_put_int(FILE *out, ULONG i) {
    pt_assert(i == (unsigned int) i);
    const size_t SIZE = 4;
#ifndef ARB_64
    COMPILE_ASSERT(sizeof(PT_PNTR) == SIZE); // in 32bit mode ints are used to store pointers
#endif
    unsigned char buf[SIZE];
    PT_write_int(buf, i);
    ASSERT_RESULT(size_t, SIZE, fwrite(buf, 1, SIZE, out));
}

void PTD_put_short(FILE *out, ULONG i) {
    pt_assert(i == (unsigned short) i);
    const size_t SIZE = 2;
    unsigned char buf[SIZE];
    PT_write_short(buf, i);
    ASSERT_RESULT(size_t, SIZE, fwrite(buf, 1, SIZE, out));
}

void PTD_put_byte(FILE *out, ULONG i) {
    pt_assert(i == (unsigned char) i);
    unsigned char ch;
    PT_write_char(&ch, i);
    fputc(ch, out);
}

const int    BITS_FOR_SIZE = 4;        // size is stored inside POS_TREE->flags, if it fits into lower 4 bits
const int    SIZE_MASK     = (1<<BITS_FOR_SIZE)-1;
const size_t MIN_NODE_SIZE = PT_EMPTY_NODE_SIZE;

COMPILE_ASSERT(MIN_NODE_SIZE <= PT_EMPTY_LEAF_SIZE);
COMPILE_ASSERT(MIN_NODE_SIZE <= PT_SHORT_CHAIN_HEAD_SIZE);
COMPILE_ASSERT(MIN_NODE_SIZE <= PT_EMPTY_NODE_SIZE);

const int MIN_SIZE_IN_FLAGS = MIN_NODE_SIZE;
const int MAX_SIZE_IN_FLAGS = MIN_NODE_SIZE+SIZE_MASK-1;

const int FLAG_SIZE_REDUCTION = MIN_SIZE_IN_FLAGS-1;

static void PTD_set_object_to_saved_status(POS_TREE *node, long pos_start, int former_size) {
    pt_assert_stage(STAGE1);
    node->flags = 0x20; // sets node type to PT_NT_SAVED; see PT_prefixtree.cxx@PT_NT_SAVED

    PT_write_big(&node->data, pos_start);

    pt_assert(former_size >= int(MIN_NODE_SIZE));

    COMPILE_ASSERT((MAX_SIZE_IN_FLAGS-MIN_SIZE_IN_FLAGS+1) == SIZE_MASK); // ????0000 means "size not stored in flags"

    if (former_size >= MIN_SIZE_IN_FLAGS && former_size <= MAX_SIZE_IN_FLAGS) {
        pt_assert(former_size >= int(sizeof(uint_big)));
        node->flags |= former_size-FLAG_SIZE_REDUCTION;
    }
    else {
        pt_assert(former_size >= int(sizeof(uint_big)+sizeof(int)));
        PT_write_int((&node->data)+sizeof(uint_big), former_size);
    }
}

inline int get_memsize_of_saved(const POS_TREE *node) {
    int memsize = node->flags & SIZE_MASK;
    if (memsize) {
        memsize += FLAG_SIZE_REDUCTION;
    }
    else {
        memsize = PT_read_int(&node->data+sizeof(PT_PNTR));
    }
    return memsize;
}

static long PTD_write_tip_to_disk(FILE *out, POS_TREE *node, const long oldpos) {
    fputc(node->flags, out);         // save type
    int size = PT_LEAF_SIZE(node);
    // write 4 bytes when not in stage 2 save mode

    size_t cnt = size-sizeof(PT_PNTR)-1;               // no father; type already saved
    ASSERT_RESULT(size_t, cnt, fwrite(&node->data + sizeof(PT_PNTR), 1, cnt, out)); // write name rpos apos

    long pos = oldpos+size-sizeof(PT_PNTR);                // no father
    pt_assert(pos >= 0);
    PTD_set_object_to_saved_status(node, oldpos, size);
    return pos;
}

static char *reverse_chain(char *entry, char *successor) {
    while (1) {
        char *nextEntry = PT_read_pointer<char>(entry);
        PT_write_pointer(entry, successor);

        if (!nextEntry) break;

        successor = entry;
        entry     = nextEntry;
    }
    return entry;
}

static void reverse_chain(POS_TREE * const node) {
    pt_assert_stage(STAGE1);
    pt_assert(PT_read_type(node) == PT_NT_CHAIN);

    char *data       = node_data_start(node) + ((node->flags&1) ? 4 : 2);
    char *entry      = PT_read_pointer<char>(data);
    char *last_entry = reverse_chain(entry, NULL);

    PT_write_pointer(data, last_entry);
}

static long PTD_write_chain_to_disk(FILE *out, POS_TREE * const node, const long oldpos, ARB_ERROR& error) {
    pt_assert_valid_chain(node);

    long pos = oldpos;

    PTD_put_byte(out, node->flags);         // save type
    pos++;

    const char *data = node_data_start(node);
    int         mainapos;

    if (node->flags&1) {
        mainapos = PT_read_int(data);
        PTD_put_int(out, mainapos);

        data += 4;
        pos  += 4;
    }
    else {
        mainapos = PT_read_short(data);
        PTD_put_short(out, mainapos);

        data += 2;
        pos  += 2;
    }

    reverse_chain(node);

    ChainIterator entry(node);
    int           lastname = 0;
    while (entry && !error) {
        const DataLoc& loc = entry.at();
        if (loc.get_name()<lastname) {
            error = GBS_global_string("Chain Error: name order error (%i < %i)", loc.get_name(), lastname);
        }
        else {
            static char buffer[100];

            char *wp = buffer;
            wp       = PT_WRITE_CHAIN_ENTRY(wp, mainapos, loc.get_name()-lastname, loc.get_abs_pos(), loc.get_rel_pos());

            int size = wp -buffer;
            if (1 != fwrite(buffer, size, 1, out)) {
                error = GB_IO_error("writing chains to", "ptserver-index");
            }

            pos      += size;
            lastname  = loc.get_name();

            MEM.put((char*)entry.memory(), entry.memsize());
            ++entry;
        }
    }

    fputc(PT_CHAIN_END, out);
    pos++;
    pt_assert(pos >= 0);
    PTD_set_object_to_saved_status(node, oldpos, data+sizeof(PT_PNTR)-(char*)node);
    pt_assert(PT_read_type(node) == PT_NT_SAVED);
    return pos;
}

void PTD_debug_nodes() {
    printf ("Inner Node Statistic:\n");
    printf ("   Single Nodes:   %6i\n", psg.stat.single_node);
    printf ("   Short  Nodes:   %6i\n", psg.stat.short_node);
    printf ("       Chars:      %6i\n", psg.stat.chars);
    printf ("       Shorts:     %6i\n", psg.stat.shorts2);
    
#ifdef ARB_64
    printf ("   Int    Nodes:   %6i\n", psg.stat.int_node);
    printf ("       Shorts:     %6i\n", psg.stat.shorts);
    printf ("       Ints:       %6i\n", psg.stat.ints2);
#endif

    printf ("   Long   Nodes:   %6i\n", psg.stat.long_node);
#ifdef ARB_64
    printf ("       Ints:       %6i\n", psg.stat.ints);
#else    
    printf ("       Shorts:     %6i\n", psg.stat.shorts);
#endif
    printf ("       Longs:      %6i\n", psg.stat.longs);

#ifdef ARB_64
    printf ("   maxdiff:        %6li\n", psg.stat.maxdiff);
#endif
}

static long PTD_write_node_to_disk(FILE *out, POS_TREE *node, long *r_poss, const long oldpos) {
    // Save node after all descendends are already saved

    pt_assert_stage(STAGE1);
    
    long max_diff = 0;
    int  lasti    = 0;
    int  count    = 0;
    int  size     = PT_EMPTY_NODE_SIZE;
    int  mysize   = PT_EMPTY_NODE_SIZE;

    for (int i = PT_QU; i < PT_BASES; i++) {    // free all sons
        POS_TREE *son = PT_read_son_stage_1(node, (PT_base)i);
        if (son) {
            long diff = oldpos - r_poss[i];
            pt_assert(diff >= 0);
            if (diff>max_diff) {
                max_diff = diff;
                lasti = i;
#ifdef ARB_64
                if (max_diff > psg.stat.maxdiff) {
                    psg.stat.maxdiff = max_diff;
                }
#endif
            }
            mysize += sizeof(PT_PNTR);
            if (PT_GET_TYPE(son) != PT_NT_SAVED) GBK_terminate("Internal Error: Son not saved");
            MEM.put(son, get_memsize_of_saved(son));
            count ++;
        }
    }
    if ((count == 1) && (max_diff<=9) && (max_diff != 2)) { // nodesingle
        if (max_diff>2) max_diff -= 2; else max_diff -= 1;
        long flags = 0xc0 | lasti | (max_diff << 3);
        fputc((int)flags, out);
        psg.stat.single_node++;
    }
    else {                          // multinode
        fputc(node->flags, out);
        int flags2 = 0;
        int level;
#ifdef ARB_64
        if (max_diff > 0xffffffff) {        // long node
            printf("Warning: max_diff > 0xffffffff is not tested.\n");
            flags2 |= 0x40;
            level = 0xffffffff;
            psg.stat.long_node++;
        }
        else if (max_diff > 0xffff) {       // int node
            flags2 |= 0x80;
            level = 0xffff;
            psg.stat.int_node++;
        }
        else {                              // short node
            level = 0xff;
            psg.stat.short_node++;
        }
#else
        if (max_diff > 0xffff) {
            flags2 |= 0x80;
            level = 0xffff;
            psg.stat.long_node++;
        }
        else {
            max_diff = 0;
            level = 0xff;
            psg.stat.short_node++;
        }
#endif
        for (int i = PT_QU; i < PT_BASES; i++) {    // set the flag2
            if (r_poss[i]) {
                long diff = oldpos - r_poss[i];
                pt_assert(diff >= 0);

                if (diff>level) flags2 |= 1<<i;
            }
        }
        fputc(flags2, out);
        size++;
        for (int i = PT_QU; i < PT_BASES; i++) {    // write the data
            if (r_poss[i]) {
                long diff = oldpos - r_poss[i];
                pt_assert(diff >= 0);
#ifdef ARB_64
                if (max_diff > 0xffffffff) {        // long long / int  (bit[6] in flags2 is set; bit[7] is unset)
                    printf("Warning: max_diff > 0xffffffff is not tested.\n");
                    if (diff>level) {               // long long (64 bit)  (bit[i] in flags2 is set)
                        PTD_put_longlong(out, diff);
                        size += 8;
                        psg.stat.longs++;
                    }
                    else {                          // int              (bit[i] in flags2 is unset)
                        PTD_put_int(out, diff);
                        size += 4;
                        psg.stat.ints++;
                    }
                }
                else if (max_diff > 0xffff) {       // int/short        (bit[6] in flags2 is unset; bit[7] is set)
                    if (diff>level) {               // int              (bit[i] in flags2 is set)
                        PTD_put_int(out, diff);
                        size += 4;
                        psg.stat.ints2++;
                    }
                    else {                          // short            (bit[i] in flags2 is unset)
                        PTD_put_short(out, diff);
                        size += 2;
                        psg.stat.shorts++;
                    }
                }
                else {                              // short/char       (bit[6] in flags2 is unset; bit[7] is unset)
                    if (diff>level) {               // short            (bit[i] in flags2 is set)
                        PTD_put_short(out, diff);
                        size += 2;
                        psg.stat.shorts2++;
                    }
                    else {                          // char             (bit[i] in flags2 is unset)
                        PTD_put_byte(out, diff);
                        size += 1;
                        psg.stat.chars++;
                    }
                }
#else
                if (max_diff) {                     // int/short (bit[7] in flags2 set)
                    if (diff>level) {               // int
                        PTD_put_int(out, diff);
                        size += 4;
                        psg.stat.longs++;
                    }
                    else {                          // short
                        PTD_put_short(out, diff);
                        size += 2;
                        psg.stat.shorts++;
                    }
                }
                else {                              // short/char  (bit[7] in flags2 not set)
                    if (diff>level) {               // short
                        PTD_put_short(out, diff);
                        size += 2;
                        psg.stat.shorts2++;
                    }
                    else {                          // char
                        PTD_put_byte(out, diff);
                        size += 1;
                        psg.stat.chars++;
                    }
                }
#endif
            }
        }
    }

    long pos = oldpos+size-sizeof(PT_PNTR);                // no father
    pt_assert(pos >= 0);
    PTD_set_object_to_saved_status(node, oldpos, mysize);
    return pos;
}

long PTD_write_leafs_to_disk(FILE *out, POS_TREE * const node, long pos, long *node_pos, ARB_ERROR& error) { 
    // returns new position in index-file (unchanged for type PT_NT_SAVED)
    // *node_pos is set to the start-position of the most recent object written

    pt_assert_stage(STAGE1);

    PT_NODE_TYPE type = PT_read_type(node);

    if (type == PT_NT_SAVED) { // already saved
        *node_pos = PT_read_big(&node->data);
    }
    else if (type == PT_NT_LEAF) {
        *node_pos = pos;
        pos = PTD_write_tip_to_disk(out, node, pos);
    }
    else if (type == PT_NT_CHAIN) {
        *node_pos = pos;
        pos = PTD_write_chain_to_disk(out, node, pos, error);
        pt_assert(PT_read_type(node) == PT_NT_SAVED);
    }
    else if (type == PT_NT_NODE) {
        long son_pos[PT_BASES];
        for (int i = PT_QU; i < PT_BASES && !error; i++) { // save all sons
            POS_TREE *son = PT_read_son_stage_1(node, (PT_base)i);
            son_pos[i] = 0;
            if (son) {
                pos = PTD_write_leafs_to_disk(out, son, pos, &(son_pos[i]), error);
                pt_assert(PT_read_type(son) == PT_NT_SAVED);
            }
        }

        if (!error) {
            *node_pos = pos;
            pos = PTD_write_node_to_disk(out, node, son_pos, pos);
        }
    }
#if defined(ASSERTION_USED)
    else pt_assert(0);
#endif
    pt_assert(error || PT_read_type(node) == PT_NT_SAVED);
    return pos;
}

// --------------------------------------
//      functions for stage 2-3: load


ARB_ERROR PTD_read_leafs_from_disk(const char *fname, POS_TREE **pnode) { // __ATTR__USERESULT
    pt_assert_stage(STAGE3);

    GB_ERROR  error  = NULL;
    char     *buffer = GB_map_file(fname, 0);

    if (!buffer) {
        error = GBS_global_string("mmap failed (%s)", GB_await_error());
    }
    else {
        long  size = GB_size_of_file(fname);
        char *main = &(buffer[size-4]);

        bool big_db = false;

        long i = PT_read_int(main);
#ifdef ARB_64
        if (i == 0xffffffff) {                    // 0xffffffff signalizes that "last_obj" is stored
            main -= 8;                            // in the previous 8 byte (in a long long)
            COMPILE_ASSERT(sizeof(PT_PNTR) == 8); // 0xffffffff is used to signalize to be compatible with older pt-servers
            i      = PT_read_big(main);           // this search tree can only be loaded at 64 Bit
            big_db = true;
        }
#else
        if (i<0) {
            pt_assert(i == -1);                     // aka 0xffffffff
            big_db = true;                          // not usable in 32-bit version (fails below)
        }
        pt_assert(i <= INT_MAX);
#endif

        // try to find info_area
        main -= 2;

        short info_size     = PT_read_short(main);
        bool  info_detected = false;

        if (info_size>0 && info_size<(main-buffer)) {   // otherwise impossible size
            main -= info_size;

            long magic   = PT_read_int(main); main += 4;
            int  version = PT_read_char(main); main++;

            pt_assert(PT_SERVER_MAGIC>0 && PT_SERVER_MAGIC<INT_MAX);

            if (magic == PT_SERVER_MAGIC) {
                info_detected = true;
                if (version>PT_SERVER_VERSION) {
                    error = "PT-server database was built with a newer version of PT-Server";
                }
            }
        }

#ifdef ARB_64
        // 64bit version:
        big_db        = big_db; // only used in 32bit
        info_detected = info_detected;
#else
        // 32bit version:
        if (!error && big_db) {
            error = "PT-server database can only be used with 64bit-PT-Server";
        }
        if (!error && !info_detected) {
            printf("Warning: ptserver DB has old format (no problem)\n");
        }
#endif

        if (!error) {
            pt_assert(i >= 0);

            *pnode = (POS_TREE *)(i+buffer);

            pt_assert(psg.ptdata->get_offset() == 0);
            psg.ptdata->use_rel_pointers(buffer);
        }
    }

    return error;
}

// --------------------------------------------------------------------------------

#if defined(PTM_MEM_DUMP_STATS)
const char *get_blocksize_description(int blocksize) {
#if defined(ARB_64)
#else // 32bit:
    COMPILE_ASSERT(PT_SHORT_CHAIN_HEAD_SIZE == PT_EMPTY_LEAF_SIZE);
    COMPILE_ASSERT(PT_LONG_CHAIN_HEAD_SIZE == PT_NODE_WITHSONS_SIZE(2));
#endif
    const char *known = NULL;
    switch (blocksize) {
        case PT_EMPTY_NODE_SIZE:       known = "PT_EMPTY_NODE_SIZE"; break;
#if defined(ARB_64)
        case PT_EMPTY_LEAF_SIZE:       known = "PT_EMPTY_LEAF_SIZE"; break;
        case PT_SHORT_CHAIN_HEAD_SIZE: known = "PT_SHORT_CHAIN_HEAD_SIZE"; break;
        case PT_LONG_CHAIN_HEAD_SIZE:  known = "PT_LONG_CHAIN_HEAD_SIZE"; break;
        case PT_MIN_CHAIN_ENTRY_SIZE:  known = "PT_MIN_CHAIN_ENTRY_SIZE"; break;
        case PT_MAX_CHAIN_ENTRY_SIZE:  known = "PT_MAX_CHAIN_ENTRY_SIZE"; break;
        case PT_NODE_WITHSONS_SIZE(2): known = "PT_NODE_WITHSONS_SIZE(2)"; break;
#else // 32bit:
        case PT_EMPTY_LEAF_SIZE:       known = "PT_EMPTY_LEAF_SIZE       and PT_SHORT_CHAIN_HEAD_SIZE"; break;
        case PT_MIN_CHAIN_ENTRY_SIZE:  known = "PT_MIN_CHAIN_ENTRY_SIZE"; break;
        case PT_MAX_CHAIN_ENTRY_SIZE:  known = "PT_MAX_CHAIN_ENTRY_SIZE"; break;
        case PT_NODE_WITHSONS_SIZE(2): known = "PT_NODE_WITHSONS_SIZE(2) and PT_LONG_CHAIN_HEAD_SIZE"; break;
#endif
        case PT_NODE_WITHSONS_SIZE(1): known = "PT_NODE_WITHSONS_SIZE(1)"; break;
        case PT_NODE_WITHSONS_SIZE(3): known = "PT_NODE_WITHSONS_SIZE(3)"; break;
        case PT_NODE_WITHSONS_SIZE(4): known = "PT_NODE_WITHSONS_SIZE(4)"; break;
        case PT_NODE_WITHSONS_SIZE(5): known = "PT_NODE_WITHSONS_SIZE(5)"; break;
        case PT_NODE_WITHSONS_SIZE(6): known = "PT_NODE_WITHSONS_SIZE(6)"; break;
    }
    return known;
}
#endif

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static arb_test::match_expectation has_proper_saved_state(POS_TREE *node, int size, bool expect_in_flags) {
    using namespace arb_test;

    int unmodified = 0xffffffff;
    PT_write_int((&node->data)+sizeof(PT_PNTR), unmodified);

    PTD_set_object_to_saved_status(node, 0, size);

    expectation_group expected;
    expected.add(that(PT_read_type(node)).is_equal_to(PT_NT_SAVED));
    expected.add(that(get_memsize_of_saved(node)).is_equal_to(size));

    int data_after = PT_read_int(&node->data+sizeof(PT_PNTR));

    if (expect_in_flags) {
        expected.add(that(data_after).is_equal_to(unmodified));
    }
    else {
        expected.add(that(data_after).does_differ_from(unmodified));
    }

    return all().ofgroup(expected);
}

#define TEST_SIZE_SAVED_IN_FLAGS(pt,size)         TEST_EXPECTATION(has_proper_saved_state(pt,size,true))
#define TEST_SIZE_SAVED_EXTERNAL(pt,size)         TEST_EXPECTATION(has_proper_saved_state(pt,size,false))
#define TEST_SIZE_SAVED_IN_FLAGS__BROKEN(pt,size) TEST_EXPECTATION__BROKEN(has_proper_saved_state(pt,size,true))
#define TEST_SIZE_SAVED_EXTERNAL__BROKEN(pt,size) TEST_EXPECTATION__BROKEN(has_proper_saved_state(pt,size,false))

struct EnterStage1 {
    EnterStage1() {
        PT_init_psg();
        psg.ptdata = PT_init(STAGE1);
    }
    ~EnterStage1() {
        PT_exit_psg();
    }
};

void TEST_saved_state() {
    EnterStage1 env;

    POS_TREE *node = (POS_TREE*)malloc(sizeof(POS_TREE)+sizeof(PT_PNTR)+5);

    TEST_SIZE_SAVED_IN_FLAGS(node, MIN_NODE_SIZE);

    TEST_SIZE_SAVED_IN_FLAGS(node, MIN_SIZE_IN_FLAGS);
    TEST_SIZE_SAVED_IN_FLAGS(node, MAX_SIZE_IN_FLAGS);

    TEST_SIZE_SAVED_EXTERNAL(node, 5000);
    TEST_SIZE_SAVED_EXTERNAL(node, 40);

#ifdef ARB_64
    TEST_SIZE_SAVED_EXTERNAL(node, 24);
    TEST_SIZE_SAVED_IN_FLAGS(node, 23);
#else
    TEST_SIZE_SAVED_EXTERNAL(node, 20);
    TEST_SIZE_SAVED_IN_FLAGS(node, 19);
#endif

    TEST_SIZE_SAVED_IN_FLAGS(node, 10);
    TEST_SIZE_SAVED_IN_FLAGS(node, 9);

#ifdef ARB_64
    COMPILE_ASSERT(MIN_NODE_SIZE == 9);
#else
    TEST_SIZE_SAVED_IN_FLAGS(node, 8);
    TEST_SIZE_SAVED_IN_FLAGS(node, 7);
    TEST_SIZE_SAVED_IN_FLAGS(node, 6);
    TEST_SIZE_SAVED_IN_FLAGS(node, 5);

    COMPILE_ASSERT(MIN_NODE_SIZE == 5);
#endif

    free(node);
}

#if defined(ENABLE_CRASH_TESTS)
// # define TEST_BAD_CHAINS // TEST_chains fails in PTD_save_partial_tree if uncommented (as expected)
#endif

#if defined(TEST_BAD_CHAINS)

static POS_TREE *theChain = NULL;
static DataLoc  *theLoc   = NULL;

static void bad_add_to_chain() {
    PT_add_to_chain(theChain, *theLoc);
#if !defined(PTM_DEBUG_VALIDATE_CHAINS)
    pt_assert(PT_chain_has_valid_entries(theChain));
#endif
    theChain = NULL;
    theLoc   = NULL;
}
#endif

void TEST_chains() {
    EnterStage1 env;
    psg.data_count = 3;

    POS_TREE *root = PT_create_leaf(NULL, PT_N, DataLoc(0, 0, 0));
    TEST_EXPECT_EQUAL(PT_read_type(root), PT_NT_LEAF);

    root = PT_change_leaf_to_node(root);
    TEST_EXPECT_EQUAL(PT_read_type(root), PT_NT_NODE);

    DataLoc loc1a(1, 200, 200);
    DataLoc loc1b(1, 500, 500);

    DataLoc loc2a(2, 300, 300);
    DataLoc loc2b(2, 700, 700);

    for (int base = PT_A; base <= PT_G; ++base) {
        POS_TREE *leaf = PT_create_leaf(&root, PT_base(base), loc1a);
        TEST_EXPECT_EQUAL(PT_read_type(leaf), PT_NT_LEAF);

        POS_TREE *chain = PT_leaf_to_chain(leaf);
        TEST_EXPECT_EQUAL(PT_read_type(chain), PT_NT_CHAIN);
        TEST_EXPECT(PT_chain_has_valid_entries(chain));

        PT_add_to_chain(chain, loc2a);
        TEST_EXPECT(PT_chain_has_valid_entries(chain));

        if (base == PT_A) { // test only once
            ChainIterator entry(chain);

            TEST_EXPECT_EQUAL(bool(entry), true);
            TEST_EXPECT(entry.at() == loc2a);
            ++entry;
            TEST_EXPECT_EQUAL(bool(entry), true);
            TEST_EXPECT(entry.at() == loc1a);
        }

        // now chain is 'loc1a,loc2a'

#if defined(TEST_BAD_CHAINS)
        switch (base) {
            case PT_A: theChain = chain; theLoc = &loc2a; TEST_EXPECT_CODE_ASSERTION_FAILS(bad_add_to_chain); break; // add same location twice -> fail
            case PT_C: theChain = chain; theLoc = &loc1b; TEST_EXPECT_CODE_ASSERTION_FAILS(bad_add_to_chain); break; // add species in wrong order -> fail
            case PT_G: theChain = chain; theLoc = &loc2b; TEST_EXPECT_CODE_ASSERTION_FAILS(bad_add_to_chain); break; // add positions in wrong order (should fail)
            default: TEST_EXPECT(0); break;
        }
#endif
    }
    {
        POS_TREE *leaf = PT_create_leaf(&root, PT_QU, loc1a); // PT_QU always produces chain
        TEST_EXPECT_EQUAL(PT_read_type(leaf), PT_NT_CHAIN);
    }

    // since there is no explicit code to free POS_TREE-memory, spool it into /dev/null
    {
        FILE      *out = fopen("/dev/null", "wb");
        ARB_ERROR  error;
        long       root_pos;

        long pos = PTD_save_lower_tree(out, root, 0, error);
        pos      = PTD_save_upper_tree(out, root, pos, root_pos, error);

        TEST_EXPECT_NO_ERROR(error.deliver());
        TEST_EXPECTATION(all().of(that(root_pos).is_equal_to(43), that(pos).is_equal_to(48)));

        TEST_EXPECT_EQUAL(PT_read_type(root), PT_NT_SAVED);
        MEM.put(root, get_memsize_of_saved(root));
        
        fclose(out);
    }
}

void TEST_mem() {
    EnterStage1 env;

#if defined(PTM_MEM_DUMP_STATS)
    MEM.dump_stats(false);
#endif

    const int  MAXSIZE = 1024;
    char      *ptr[MAXSIZE+1];

    for (int size = PTM_MIN_SIZE; size <= MAXSIZE; ++size) {
        ptr[size] = (char*)MEM.get(size);
    }
    for (int size = PTM_MIN_SIZE; size <= MAXSIZE; ++size) {
#if defined(PTM_MEM_CHECKED_FREE)
        if (size <= PTM_MAX_SIZE) {
            TEST_EXPECT_EQUAL(MEM.block_has_size(ptr[size], size), true);
        }
#endif
        MEM.put(ptr[size], size);
    }

    MEM.clear();
#if defined(PTM_MEM_DUMP_STATS)
    MEM.dump_stats(true);
#endif

}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
