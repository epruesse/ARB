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

inline bool locs_in_chain_order(const AbsLoc& loc1, const AbsLoc& loc2) { 
    pt_assert_stage(STAGE1); // this order is only valid in STAGE1 (STAGE3 has reverse order)

    if (loc1.get_name() < loc2.get_name()) {
#if defined(DEBUG)
        fprintf(stderr, "invalid chain: loc1.name<loc2.name (%i<%i)\n", loc1.get_name(), loc2.get_name());
#endif
        return false;
    }
    if (loc1.get_name() == loc2.get_name()) {
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
template <typename CHAINITER>
inline bool PT_is_empty_chain(const typename CHAINITER::POS_TREE_TYPE * const node) { 
    pt_assert(node->is_chain());
    return !CHAINITER(node);
}
#endif

template <typename CHAINITER>
bool PT_chain_has_valid_entries(const typename CHAINITER::POS_TREE_TYPE * const node) {
    pt_assert(node->is_chain());

    bool ok = true;

    CHAINITER entry(node);
    if (!entry) return false;

    AbsLoc last(entry.at());

    ++entry;
    while (entry && ok) {
        ok   = locs_in_chain_order(last, entry.at());
        last = entry.at();
        ++entry;
    }

    return ok;
}

void pt_global::init(Stage stage) {
    for (int i=0; i<256; i++) {
        if (i&0x80) { // bit 8 marks a node
            flag_2_type[i] = PT_NT_NODE;
        }
        else if (stage == STAGE1 && i&0x20) { // in STAGE1 bit 6 is used to mark PT_NT_SAVED (in STAGE3 that bit may be used otherwise)
            flag_2_type[i] = (i&0x40) ? PT_NT_UNDEF : PT_NT_SAVED;
        }
        else { // bit 7 distinguishes between chain and leaf
            flag_2_type[i] = (i&0x40) ? PT_NT_CHAIN : PT_NT_LEAF;
        }
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
            count_bits[base][i] = count;
        }
    }
}

void PT_init_cache_sizes(Stage stage) {
    size_t species_count = psg.data_count;
    pt_assert(species_count>0);

#define CACHE_SEQ_PERCENT 50 // @@@ make global def and use in mem est.

    if (stage == STAGE1) {
        probe_input_data::set_cache_sizes(species_count*CACHE_SEQ_PERCENT/100+5, // cache about CACHE_SEQ_PERCENT% of seq data
                                          1);                                    // don't cache - only need abspos of currently inserted species
    }
    else {
        probe_input_data::set_cache_sizes(species_count, species_count); // cache all seq and positional data
    }
}

PT_data *PT_init(Stage stage) {
    PT_data *ptdata = new PT_data(stage);
    PT_GLOBAL.init(stage);
    pt_assert(MEM.is_clear());
    return ptdata;
}

// ------------------------------
//      functions for stage 1

static void PT_change_link_in_father(POS_TREE1 *father, POS_TREE1 *old_link, POS_TREE1 *new_link) {
    pt_assert_stage(STAGE1);
    long i = PT_GLOBAL.count_bits[PT_BASES][father->flags];
    for (; i>0; i--) {
        POS_TREE1 *link = PT_read_pointer<POS_TREE1>(&father->data+sizeof(PT_PNTR)*i);
        if (link==old_link) {
            PT_write_pointer(&father->data+sizeof(PT_PNTR)*i, new_link);
            return;
        }
    }
    pt_assert(0); // father did not contain 'old_link'
}

void PT_add_to_chain(POS_TREE1 *node, const DataLoc& loc) {
    pt_assert_stage(STAGE1);
#if defined(PTM_DEBUG_VALIDATE_CHAINS)
    pt_assert(PT_is_empty_chain<ChainIteratorStage1>(node) ||
              PT_chain_has_valid_entries<ChainIteratorStage1>(node));
#endif

    // insert at the beginning of list

    char *data      = node_data_start(node) + ((node->flags&1) ? 4 : 2);
    void *old_first = PT_read_void_pointer(data);

    const int MAX_CHAIN_ENTRY_SIZE = sizeof(PT_PNTR)+3*sizeof(int);

    static char  buffer[MAX_CHAIN_ENTRY_SIZE];
    char        *p = buffer;

    PT_write_pointer(p, old_first);
    p += sizeof(PT_PNTR);

    PT_write_compact_nat(p, loc.get_name());
    PT_write_compact_nat(p, loc.get_rel_pos());
    PT_write_compact_nat(p, loc.get_abs_pos());

    int size = p - buffer;

    pt_assert(size <= MAX_CHAIN_ENTRY_SIZE);

    p = (char*)MEM.get(size);
    memcpy(p, buffer, size);
    PT_write_pointer(data, p);
    psg.stat.cut_offs ++;

    pt_assert_valid_chain_stage1(node);
}

inline void PT_set_father(POS_TREE1 *son, const POS_TREE1 *father) {
    pt_assert_stage(STAGE1); // otherwise there are no fathers
    pt_assert(!father || father->is_node());

    PT_write_pointer(&son->data, father);

    pt_assert(PT_read_father(son) == father);
}

POS_TREE1 *PT_change_leaf_to_node(POS_TREE1 *node) {
    pt_assert_stage(STAGE1);
    pt_assert(node->is_leaf());

    POS_TREE1 *father = PT_read_father(node);

    POS_TREE1 *new_elem = (POS_TREE1 *)MEM.get(PT_EMPTY_NODE_SIZE);
    if (father) PT_change_link_in_father(father, node, new_elem);
    MEM.put(node, PT_LEAF_SIZE(node));
    new_elem->set_type(PT_NT_NODE);
    PT_set_father(new_elem, father);

    return new_elem;
}

POS_TREE1 *PT_leaf_to_chain(POS_TREE1 *node) {
    pt_assert_stage(STAGE1);
    pt_assert(node->is_leaf());

    POS_TREE1     *father = PT_read_father(node);
    const DataLoc  loc(node);

    int        chain_size = (loc.get_abs_pos()>PT_SHORT_SIZE) ? PT_LONG_CHAIN_HEAD_SIZE : PT_SHORT_CHAIN_HEAD_SIZE;
    POS_TREE1 *new_elem   = (POS_TREE1 *)MEM.get(chain_size);

    PT_change_link_in_father(father, node, new_elem);
    MEM.put(node, PT_LEAF_SIZE(node));
    new_elem->set_type(PT_NT_CHAIN);
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

POS_TREE1 *PT_create_leaf(POS_TREE1 **pfather, PT_base base, const DataLoc& loc) {
    pt_assert_stage(STAGE1);
    if (base >= PT_BASES) PT_CORE;

    int leafsize = PT_EMPTY_LEAF_SIZE;

    if (loc.get_rel_pos()>PT_SHORT_SIZE) leafsize += 2;
    if (loc.get_abs_pos()>PT_SHORT_SIZE) leafsize += 2;
    if (loc.get_name()   >PT_SHORT_SIZE) leafsize += 2;

    POS_TREE1 *node = (POS_TREE1 *)MEM.get(leafsize);

    pt_assert(PT_read_father(node) == NULL); // cause memory is cleared

    if (pfather) {
        POS_TREE1 *father = *pfather;

        int       oldfathersize  = PT_NODE_SIZE(father);
        POS_TREE1 *new_elemfather = (POS_TREE1 *)MEM.get(oldfathersize + sizeof(PT_PNTR));
        new_elemfather->set_type(PT_NT_NODE);

        POS_TREE1 *gfather = PT_read_father(father);
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
                POS_TREE1 *son = PT_read_pointer<POS_TREE1>(&father->data+sbase);
                if (!son->is_saved()) PT_set_father(son, new_elemfather);

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
        *pfather = new_elemfather;
    }
    node->set_type(PT_NT_LEAF);

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

void PTD_clear_fathers(POS_TREE1 *node) {
    pt_assert_stage(STAGE1);
    PT_NODE_TYPE type = node->get_type();
    if (type != PT_NT_SAVED) {
        PT_set_father(node, NULL);
        if (type == PT_NT_NODE) {
            for (int i = PT_QU; i < PT_BASES; i++) {
                POS_TREE1 *son = PT_read_son(node, (PT_base)i);
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

int PTD_put_compact_nat(FILE *out, uint_32 nat) {
    // returns number of bytes written
    const size_t MAXSIZE = 5;

    char  buf[MAXSIZE];
    char *bp = buf;

    PT_write_compact_nat(bp, nat);
    size_t size = bp-buf;
    pt_assert(size <= MAXSIZE);
    ASSERT_RESULT(size_t, size, fwrite(buf, 1, size, out));
    return size;
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

static void PTD_set_object_to_saved_status(POS_TREE1 *node, long pos_start, int former_size) {
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

inline int get_memsize_of_saved(const POS_TREE1 *node) {
    int memsize = node->flags & SIZE_MASK;
    if (memsize) {
        memsize += FLAG_SIZE_REDUCTION;
    }
    else {
        memsize = PT_read_int(&node->data+sizeof(PT_PNTR));
    }
    return memsize;
}

static long PTD_write_tip_to_disk(FILE *out, POS_TREE1 *node, const long oldpos) {
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

static char *reverse_chain(char *entry, char *successor, uint_32& chain_length) {
    while (1) {
        char *nextEntry = PT_read_pointer<char>(entry);
        PT_write_pointer(entry, successor);
        ++chain_length;

        if (!nextEntry) break;

        successor = entry;
        entry     = nextEntry;
    }
    return entry;
}

static uint_32 reverse_chain(POS_TREE1 * const node) {
    pt_assert_stage(STAGE1);
    pt_assert(node->is_chain());

    char *data  = node_data_start(node) + ((node->flags&1) ? 4 : 2);
    char *entry = PT_read_pointer<char>(data);

    uint_32  length     = 0;
    char    *last_entry = reverse_chain(entry, NULL, length);

    PT_write_pointer(data, last_entry);
    return length;
}

static long PTD_write_chain_to_disk(FILE *out, POS_TREE1 * const node, const long oldpos, ARB_ERROR& error) {
    pt_assert_valid_chain_stage1(node);

    long pos = oldpos;

    PTD_put_byte(out, node->flags); // save type
    pos++;

    uint_32 chain_length = reverse_chain(node);
    pt_assert(chain_length>0);

    ChainIteratorStage1 entry(node);

    int mainapos = entry.get_mainapos();

    if (node->flags&1) {
        PTD_put_int(out, mainapos);
        pos  += 4;
    }
    else {
        PTD_put_short(out, mainapos);
        pos  += 2;
    }

    pos += PTD_put_compact_nat(out, chain_length);

    uint_32 entries  = 0;
    int     lastname = 0;
    while (entry && !error) {
        const AbsLoc& loc = entry.at();
        if (loc.get_name()<lastname) {
            error = GBS_global_string("Chain Error: name order error (%i < %i)", loc.get_name(), lastname);
        }
        else {
            static char buffer[100];

            char *wp = PT_WRITE_CHAIN_ENTRY(buffer, mainapos, loc.get_name()-lastname, loc.get_abs_pos());

            int size = wp -buffer;
            if (1 != fwrite(buffer, size, 1, out)) {
                error = GB_IO_error("writing chains to", "ptserver-index");
            }

            pos      += size;
            lastname  = loc.get_name();

            MEM.put((char*)entry.memory(), entry.memsize());
            ++entry;
        }
        ++entries;
    }

    pt_assert(entries == chain_length);
    pt_assert(pos >= 0);

    {
        int chain_head_size = node->flags&1 ? PT_LONG_CHAIN_HEAD_SIZE : PT_SHORT_CHAIN_HEAD_SIZE;
        PTD_set_object_to_saved_status(node, oldpos, chain_head_size);
        pt_assert(node->is_saved());
    }

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

static long PTD_write_node_to_disk(FILE *out, POS_TREE1 *node, long *r_poss, const long oldpos) {
    // Save node after all descendends are already saved

    pt_assert_stage(STAGE1);
    
    long max_diff = 0;
    int  lasti    = 0;
    int  count    = 0;
    int  size     = PT_EMPTY_NODE_SIZE;
    int  mysize   = PT_EMPTY_NODE_SIZE;

    for (int i = PT_QU; i < PT_BASES; i++) {    // free all sons
        POS_TREE1 *son = PT_read_son(node, (PT_base)i);
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
            if (!son->is_saved()) GBK_terminate("Internal Error: Son not saved");
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

long PTD_write_leafs_to_disk(FILE *out, POS_TREE1 * const node, long pos, long *node_pos, ARB_ERROR& error) { 
    // returns new position in index-file (unchanged for type PT_NT_SAVED)
    // *node_pos is set to the start-position of the most recent object written

    pt_assert_stage(STAGE1);

    switch (node->get_type()) {
        case PT_NT_SAVED:
            *node_pos = PT_read_big(&node->data);
            break;

        case PT_NT_LEAF:
            *node_pos = pos;
            pos       = PTD_write_tip_to_disk(out, node, pos);
            break;

        case PT_NT_CHAIN:
            *node_pos = pos;
            pos       = PTD_write_chain_to_disk(out, node, pos, error);
            pt_assert(node->is_saved());
            break;

        case PT_NT_NODE: {
            long son_pos[PT_BASES];
            for (int i = PT_QU; i < PT_BASES && !error; i++) { // save all sons
                POS_TREE1 *son = PT_read_son(node, (PT_base)i);
                son_pos[i] = 0;
                if (son) {
                    pos = PTD_write_leafs_to_disk(out, son, pos, &(son_pos[i]), error);
                    pt_assert(son->is_saved());
                }
            }

            if (!error) {
                *node_pos = pos;
                pos = PTD_write_node_to_disk(out, node, son_pos, pos);
            }
            break;
        }
    }

    pt_assert(error || node->is_saved());
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

        psg.big_db = false;

        long i = PT_read_int(main);
#ifdef ARB_64
        if (i == 0xffffffff) {                    // 0xffffffff signalizes that "last_obj" is stored
            main -= 8;                            // in the previous 8 byte (in a long long)
            COMPILE_ASSERT(sizeof(PT_PNTR) == 8); // 0xffffffff is used to signalize to be compatible with older pt-servers
            i      = PT_read_big(main);           // this search tree can only be loaded at 64 Bit
            psg.big_db = true;
        }
#else
        if (i<0) {
            pt_assert(i == -1);                     // aka 0xffffffff
            psg.big_db = true;                          // not usable in 32-bit version (fails below)
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
        // not used in 64bit version:
        info_detected = info_detected;
#else
        // 32bit version:
        if (!error && psg.big_db) {
            error = "PT-server database can only be used with 64bit-PT-Server";
        }
        if (!error && !info_detected) {
            printf("Warning: ptserver DB has old format (no problem)\n");
        }
#endif

        if (!error) {
            pt_assert(i >= 0);
            *pnode = (POS_TREE *)(i+buffer);
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

static arb_test::match_expectation has_proper_saved_state(POS_TREE1 *node, int size, bool expect_in_flags) {
    using namespace arb_test;

    int unmodified = 0xffffffff;
    PT_write_int((&node->data)+sizeof(PT_PNTR), unmodified);

    PTD_set_object_to_saved_status(node, 0, size);

    expectation_group expected;
    expected.add(that(node->get_type()).is_equal_to(PT_NT_SAVED));
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

    POS_TREE1 *node = (POS_TREE1*)malloc(sizeof(POS_TREE1)+sizeof(PT_PNTR)+5);

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

    POS_TREE1 *root = PT_create_leaf(NULL, PT_N, DataLoc(0, 0, 0));
    TEST_EXPECT_EQUAL(root->get_type(), PT_NT_LEAF);

    root = PT_change_leaf_to_node(root);
    TEST_EXPECT_EQUAL(root->get_type(), PT_NT_NODE);

    DataLoc loc1a(1, 200, 200);
    DataLoc loc1b(1, 500, 500);

    DataLoc loc2a(2, 300, 300);
    DataLoc loc2b(2, 700, 700);

    for (int base = PT_A; base <= PT_G; ++base) {
        POS_TREE1 *leaf = PT_create_leaf(&root, PT_base(base), loc1a);
        TEST_EXPECT_EQUAL(leaf->get_type(), PT_NT_LEAF);

        POS_TREE1 *chain = PT_leaf_to_chain(leaf);
        TEST_EXPECT_EQUAL(chain->get_type(), PT_NT_CHAIN);
        TEST_EXPECT(PT_chain_has_valid_entries<ChainIteratorStage1>(chain));

        PT_add_to_chain(chain, loc2a);
        TEST_EXPECT(PT_chain_has_valid_entries<ChainIteratorStage1>(chain));

        if (base == PT_A) { // test only once
            ChainIteratorStage1 entry(chain);

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
        POS_TREE1 *leaf = PT_create_leaf(&root, PT_QU, loc1a); // PT_QU always produces chain
        TEST_EXPECT_EQUAL(leaf->get_type(), PT_NT_CHAIN);
    }

    // since there is no explicit code to free POS_TREE-memory, spool it into /dev/null
    {
        FILE      *out = fopen("/dev/null", "wb");
        ARB_ERROR  error;
        long       root_pos;

        long pos = PTD_save_lower_tree(out, root, 0, error);
        pos      = PTD_save_upper_tree(out, root, pos, root_pos, error);

        TEST_EXPECT_NO_ERROR(error.deliver());
        TEST_EXPECTATION(all().of(that(root_pos).is_equal_to(29), that(pos).is_equal_to(34)));

        TEST_EXPECT_EQUAL(root->get_type(), PT_NT_SAVED);
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

template<int R>
static arb_test::match_expectation nat_stored_as_expected(uint_32 written_nat, int expected_bytes) {
    char buffer[5];

    static uint_8 reserved_bits = 0;

    reserved_bits = (reserved_bits+1) & BitsBelowBit<R>::value;
    uint_8 reserved_bits_read;

    char       *wp = buffer;
    const char *rp = buffer;

    write_nat_with_reserved_bits<R>(wp, written_nat, reserved_bits);
    uint_32 read_nat = read_nat_with_reserved_bits<R>(rp, reserved_bits_read);

    int written_bytes = wp-buffer;
    int read_bytes    = rp-buffer;

    using namespace   arb_test;
    expectation_group expected;
    expected.add(that(read_nat).is_equal_to(written_nat));
    expected.add(that(written_bytes).is_equal_to(expected_bytes));
    expected.add(that(read_bytes).is_equal_to(written_bytes));
    expected.add(that(reserved_bits_read).is_equal_to(reserved_bits));

    return all().ofgroup(expected);
}

template<int R>
static arb_test::match_expectation int_stored_as_expected(int32_t written_int, int expected_bytes) {
    char buffer[5];

    static uint_8 reserved_bits = 0;

    reserved_bits = (reserved_bits+1) & BitsBelowBit<R>::value;
    uint_8 reserved_bits_read;

    char       *wp = buffer;
    const char *rp = buffer;

    write_int_with_reserved_bits<R>(wp, written_int, reserved_bits);
    int32_t read_int = read_int_with_reserved_bits<R>(rp, reserved_bits_read);

    int written_bytes = wp-buffer;
    int read_bytes    = rp-buffer;

    using namespace   arb_test;
    expectation_group expected;
    expected.add(that(read_int).is_equal_to(written_int));
    expected.add(that(written_bytes).is_equal_to(expected_bytes));
    expected.add(that(read_bytes).is_equal_to(written_bytes));
    expected.add(that(reserved_bits_read).is_equal_to(reserved_bits));

    return all().ofgroup(expected);
}


#define TEST_COMPACT_NAT_STORAGE(nat,inBytes)          TEST_EXPECTATION(nat_stored_as_expected<0>(nat, inBytes))
#define TEST_COMPACT_NAT_STORAGE_RESERVE1(nat,inBytes) TEST_EXPECTATION(nat_stored_as_expected<1>(nat, inBytes))
#define TEST_COMPACT_NAT_STORAGE_RESERVE2(nat,inBytes) TEST_EXPECTATION(nat_stored_as_expected<2>(nat, inBytes))
#define TEST_COMPACT_NAT_STORAGE_RESERVE3(nat,inBytes) TEST_EXPECTATION(nat_stored_as_expected<3>(nat, inBytes))
#define TEST_COMPACT_NAT_STORAGE_RESERVE4(nat,inBytes) TEST_EXPECTATION(nat_stored_as_expected<4>(nat, inBytes))
#define TEST_COMPACT_NAT_STORAGE_RESERVE5(nat,inBytes) TEST_EXPECTATION(nat_stored_as_expected<5>(nat, inBytes))

#define TEST_COMPACT_INT_STORAGE(i,inBytes)          TEST_EXPECTATION(int_stored_as_expected<0>(i, inBytes))
#define TEST_COMPACT_INT_STORAGE_RESERVE3(i,inBytes) TEST_EXPECTATION(int_stored_as_expected<3>(i, inBytes))

void TEST_compact_storage() {
    // test natural numbers (w/o reserved bits)

    TEST_COMPACT_NAT_STORAGE(0, 1);
    TEST_COMPACT_NAT_STORAGE(0x7f, 1);

    TEST_COMPACT_NAT_STORAGE(0x80, 2);
    TEST_COMPACT_NAT_STORAGE(0x407F, 2);

    TEST_COMPACT_NAT_STORAGE(0x4080, 3);
    TEST_COMPACT_NAT_STORAGE(0x20407F, 3);

    TEST_COMPACT_NAT_STORAGE(0x204080, 4);
    TEST_COMPACT_NAT_STORAGE(0x1020407F, 4);

    TEST_COMPACT_NAT_STORAGE(0x10204080, 5);
    TEST_COMPACT_NAT_STORAGE(0xffffffff, 5);

    // test 1 reserved bit (bit7 is reserved)
    TEST_COMPACT_NAT_STORAGE_RESERVE1(0, 1);
    TEST_COMPACT_NAT_STORAGE_RESERVE1(0x3f, 1);

    TEST_COMPACT_NAT_STORAGE_RESERVE1(0x40, 2);
    TEST_COMPACT_NAT_STORAGE_RESERVE1(0x203f, 2);

    TEST_COMPACT_NAT_STORAGE_RESERVE1(0x2040, 3);
    TEST_COMPACT_NAT_STORAGE_RESERVE1(0x10203f, 3);

    TEST_COMPACT_NAT_STORAGE_RESERVE1(0x102040, 4);
    TEST_COMPACT_NAT_STORAGE_RESERVE1(0x0810203f, 4);

    TEST_COMPACT_NAT_STORAGE_RESERVE1(0x08102040, 5);
    TEST_COMPACT_NAT_STORAGE_RESERVE1(0xffffffff, 5);

    // test integers (same as natural numbers with 1 bit reserved for sign)
    TEST_COMPACT_INT_STORAGE( 0, 1);
    TEST_COMPACT_INT_STORAGE( 0x3f, 1);
    TEST_COMPACT_INT_STORAGE(-0x40, 1);

    TEST_COMPACT_INT_STORAGE( 0x40, 2);
    TEST_COMPACT_INT_STORAGE(-0x41, 2);
    TEST_COMPACT_INT_STORAGE( 0x203f, 2);
    TEST_COMPACT_INT_STORAGE(-0x2040, 2);

    TEST_COMPACT_INT_STORAGE( 0x2040, 3);
    TEST_COMPACT_INT_STORAGE(-0x2041, 3);
    TEST_COMPACT_INT_STORAGE( 0x10203f, 3);
    TEST_COMPACT_INT_STORAGE(-0x102040, 3);

    TEST_COMPACT_INT_STORAGE( 0x102040, 4);
    TEST_COMPACT_INT_STORAGE(-0x102041, 4);
    TEST_COMPACT_INT_STORAGE( 0x0810203f, 4);
    TEST_COMPACT_INT_STORAGE(-0x08102040, 4);

    TEST_COMPACT_INT_STORAGE( 0x08102040, 5);
    TEST_COMPACT_INT_STORAGE(-0x08102041, 5);
    TEST_COMPACT_INT_STORAGE(INT_MAX, 5);
    TEST_COMPACT_INT_STORAGE(INT_MIN, 5);

    // test 2 reserved bits (bit7 and bit6 are reserved)
    TEST_COMPACT_NAT_STORAGE_RESERVE2(0, 1);
    TEST_COMPACT_NAT_STORAGE_RESERVE2(0x1f, 1);

    TEST_COMPACT_NAT_STORAGE_RESERVE2(0x20, 2);
    TEST_COMPACT_NAT_STORAGE_RESERVE2(0x101f, 2);

    TEST_COMPACT_NAT_STORAGE_RESERVE2(0x1020, 3);
    TEST_COMPACT_NAT_STORAGE_RESERVE2(0x08101f, 3);

    TEST_COMPACT_NAT_STORAGE_RESERVE2(0x081020, 4);
    TEST_COMPACT_NAT_STORAGE_RESERVE2(0x0408101f, 4);

    TEST_COMPACT_NAT_STORAGE_RESERVE2(0x04081020, 5);
    TEST_COMPACT_NAT_STORAGE_RESERVE2(0xffffffff, 5);

    // test 3 reserved bits (bit7 .. bit5 are reserved)
    TEST_COMPACT_NAT_STORAGE_RESERVE3(0, 1);
    TEST_COMPACT_NAT_STORAGE_RESERVE3(0xf, 1);

    TEST_COMPACT_NAT_STORAGE_RESERVE3(0x10, 2);
    TEST_COMPACT_NAT_STORAGE_RESERVE3(0x080f, 2);

    TEST_COMPACT_NAT_STORAGE_RESERVE3(0x0810, 3);
    TEST_COMPACT_NAT_STORAGE_RESERVE3(0x04080f, 3);

    TEST_COMPACT_NAT_STORAGE_RESERVE3(0x040810, 4);
    TEST_COMPACT_NAT_STORAGE_RESERVE3(0x0204080f, 4);

    TEST_COMPACT_NAT_STORAGE_RESERVE3(0x02040810, 5);
    TEST_COMPACT_NAT_STORAGE_RESERVE3(0xffffffff, 5);

    // test 4 reserved bits (bit7 .. bit4 are reserved)
    TEST_COMPACT_NAT_STORAGE_RESERVE4(0, 1);
    TEST_COMPACT_NAT_STORAGE_RESERVE4(0x07, 1);

    TEST_COMPACT_NAT_STORAGE_RESERVE4(0x08, 2);
    TEST_COMPACT_NAT_STORAGE_RESERVE4(0x0407, 2);

    TEST_COMPACT_NAT_STORAGE_RESERVE4(0x0408, 3);
    TEST_COMPACT_NAT_STORAGE_RESERVE4(0x020407, 3);

    TEST_COMPACT_NAT_STORAGE_RESERVE4(0x020408, 4);
    TEST_COMPACT_NAT_STORAGE_RESERVE4(0x01020407, 4);

    TEST_COMPACT_NAT_STORAGE_RESERVE4(0x01020408, 5);
    TEST_COMPACT_NAT_STORAGE_RESERVE4(0xffffffff, 5);

    // test integers with 3 reserved bits
    TEST_COMPACT_INT_STORAGE_RESERVE3( 0, 1);
    TEST_COMPACT_INT_STORAGE_RESERVE3( 0x07, 1);
    TEST_COMPACT_INT_STORAGE_RESERVE3(-0x08, 1);

    TEST_COMPACT_INT_STORAGE_RESERVE3( 0x08, 2);
    TEST_COMPACT_INT_STORAGE_RESERVE3(-0x09, 2);
    TEST_COMPACT_INT_STORAGE_RESERVE3( 0x0407, 2);
    TEST_COMPACT_INT_STORAGE_RESERVE3(-0x0408, 2);

    TEST_COMPACT_INT_STORAGE_RESERVE3( 0x0408, 3);
    TEST_COMPACT_INT_STORAGE_RESERVE3(-0x0409, 3);
    TEST_COMPACT_INT_STORAGE_RESERVE3( 0x020407, 3);
    TEST_COMPACT_INT_STORAGE_RESERVE3(-0x020408, 3);

    TEST_COMPACT_INT_STORAGE_RESERVE3( 0x020408, 4);
    TEST_COMPACT_INT_STORAGE_RESERVE3(-0x020409, 4);
    TEST_COMPACT_INT_STORAGE_RESERVE3( 0x01020407, 4);
    TEST_COMPACT_INT_STORAGE_RESERVE3(-0x01020408, 4);

    TEST_COMPACT_INT_STORAGE_RESERVE3( 0x01020408, 5);
    TEST_COMPACT_INT_STORAGE_RESERVE3(-0x01020409, 5);
    TEST_COMPACT_INT_STORAGE_RESERVE3(INT_MAX, 5);
    TEST_COMPACT_INT_STORAGE_RESERVE3(INT_MIN, 5);

#if 0
    // reserving more than 4 bits does not work (compacting also uses 4 bits)
    // (compiles, but spits warnings)
    TEST_COMPACT_NAT_STORAGE_RESERVE5(0, 1);
#endif
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
