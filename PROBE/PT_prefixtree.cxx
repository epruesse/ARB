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
    if (loc1.get_name() > loc2.get_name()) {
#if defined(DEBUG)
        fprintf(stderr, "invalid chain: loc1.name>loc2.name (%i>%i)\n", loc1.get_name(), loc2.get_name());
#endif
        return false;
    }
    if (loc1.get_name() == loc2.get_name()) {
        if (loc1.get_abs_pos() <= loc2.get_abs_pos()) {
#if defined(DEBUG)
            fprintf(stderr, "invalid chain: loc1.apos<=loc2.apos (%i<=%i)\n", loc1.get_abs_pos(), loc2.get_abs_pos());
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

PT1_TYPE POS_TREE1::flag_2_type[256];
PT2_TYPE POS_TREE2::flag_2_type[256];

void POS_TREE1::init_static() {
    for (int i=0; i<256; i++) {
        if (i&0x80) { // bit 8 marks a node
            flag_2_type[i] = PT1_NODE;
        }
        else if (i&0x20) { // in STAGE1 bit 6 is used to mark PT1_SAVED (in STAGE2 that bit may be used otherwise)
            flag_2_type[i] = (i&0x40) ? PT1_UNDEF : PT1_SAVED;
        }
        else { // bit 7 distinguishes between chain and leaf
            flag_2_type[i] = (i&0x40) ? PT1_CHAIN : PT1_LEAF;
        }
    }
}
void POS_TREE2::init_static() {
    for (int i=0; i<256; i++) {
        if (i&0x80) { // bit 8 marks a node
            flag_2_type[i] = PT2_NODE;
        }
        else { // bit 7 distinguishes between chain and leaf
            flag_2_type[i] = (i&0x40) ? PT2_CHAIN : PT2_LEAF;
        }
    }
}

void pt_global::init() {
    POS_TREE1::init_static();
    POS_TREE2::init_static();

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

void probe_struct_global::enter_stage(Stage stage_) {
    stage = stage_;
    PT_GLOBAL.init();
    pt_assert(MEM.is_clear());
}

// ------------------------------
//      functions for stage 1

static void PT_change_link_in_father(POS_TREE1 *father, POS_TREE1 *old_link, POS_TREE1 *new_link) {
    pt_assert_stage(STAGE1);
    long i = PT_GLOBAL.count_bits[PT_BASES][father->flags]-1;
    for (; i >= 0; i--) {
        POS_TREE1 *link = PT_read_pointer<POS_TREE1>(father->udata()+sizeof(PT_PNTR)*i);
        if (link==old_link) {
            PT_write_pointer(father->udata()+sizeof(PT_PNTR)*i, new_link);
            return;
        }
    }
    pt_assert(0); // father did not contain 'old_link'
}

class ChainEntryBuffer : virtual Noncopyable {
    const size_t size;
    char *const  mem;
    const int    refabspos;

    char *write_pos;
    int   lastname;

    static const int MAXSIZEPERELEMENT = 2*5; // size for 2 compactNAT's
public:
    ChainEntryBuffer(int forElements, int refabspos_, int lastname_)
        : size(forElements*MAXSIZEPERELEMENT),
          mem((char*)MEM.get(size)),
          refabspos(refabspos_),
          write_pos(mem),
          lastname(lastname_)
    {}
    ~ChainEntryBuffer() { MEM.put(mem, size); }

    void add(const AbsLoc& loc) {
        int namediff = loc.get_name()-lastname;
        pt_assert(namediff >= 0);

        uint_8 has_refabspos = (loc.get_abs_pos() == refabspos);
        write_nat_with_reserved_bits<1>(write_pos, namediff, has_refabspos);
        if (!has_refabspos) PT_write_compact_nat(write_pos, loc.get_abs_pos());

        lastname = loc.get_name();
    }

    size_t get_size() const { return write_pos-mem; }
    const char *get_mem() const { return mem; }
    int get_refabspos() const { return refabspos; }
    int get_lastname() const { return lastname; }
};

void PT_add_to_chain(POS_TREE1 *node, const DataLoc& loc) {
    pt_assert_stage(STAGE1);
#if defined(PTM_DEBUG_VALIDATE_CHAINS)
    pt_assert(PT_is_empty_chain<ChainIteratorStage1>(node) ||
              PT_chain_has_valid_entries<ChainIteratorStage1>(node));
#endif

    if (node->flags & SHORT_CHAIN_HEADER_FLAG_BIT) {
        PT_short_chain_header *sheader = reinterpret_cast<PT_short_chain_header*>(node->udata());
        int                    entries = node->flags & SHORT_CHAIN_HEADER_SIZE_MASK;

        if (entries<SHORT_CHAIN_HEADER_ELEMS) { // store entries in header
            sheader->name[entries]   = loc.get_name();
            sheader->abspos[entries] = loc.get_abs_pos();

            node->flags = (node->flags & (~SHORT_CHAIN_HEADER_SIZE_MASK)) | (entries+1);
        }
        else {
            // short header is filled -> convert into long header
            // @@@ detect most used abspos!
            ChainEntryBuffer buf(entries+1, sheader->abspos[0], 0);

            for (int e = 0; e<entries; ++e) {
                buf.add(AbsLoc(sheader->name[e], sheader->abspos[e]));
            }
            buf.add(loc);

            sheader                       = NULL; // no further access possible (is reused as lheader)
            PT_long_chain_header *lheader = reinterpret_cast<PT_long_chain_header*>(node->udata());

            lheader->entries   = entries+1;
            lheader->memused   = buf.get_size();
            lheader->memsize   = std::max(size_t(PTM_MIN_SIZE), buf.get_size());
            lheader->refabspos = buf.get_refabspos();
            lheader->lastname  = buf.get_lastname();
            lheader->entrymem  = (char*)MEM.get(lheader->memsize);

            memcpy(lheader->entrymem, buf.get_mem(), buf.get_size());

            node->flags = (node->flags & ~(SHORT_CHAIN_HEADER_SIZE_MASK|SHORT_CHAIN_HEADER_FLAG_BIT));
        }
    }
    else {
        PT_long_chain_header *header = reinterpret_cast<PT_long_chain_header*>(node->udata());

        ChainEntryBuffer buf(1, header->refabspos, header->lastname);
        buf.add(loc);
        header->lastname = buf.get_lastname();

        unsigned memfree = header->memsize-header->memused;
        if (memfree >= buf.get_size()) {
            memcpy(header->entrymem+header->memused, buf.get_mem(), buf.get_size());
        }
        else {
            unsigned new_memsize = header->memused+buf.get_size();

            if (header->entries>10) new_memsize *= 1.25; // alloctate 25% more memory than needed // @@@ test with diff percentages

            // @@@ check for better refabspos if memsize per entry is too high

            char *new_mem = (char*)MEM.get(new_memsize);
            memcpy(new_mem, header->entrymem, header->memused);
            memcpy(new_mem+header->memused, buf.get_mem(), buf.get_size());

            MEM.put(header->entrymem, header->memsize);

            header->entrymem = new_mem;
            header->memsize  = new_memsize;
        }
        header->memused += buf.get_size();
        header->entries++;
    }

#if defined(PTM_DEBUG_VALIDATE_CHAINS)
    pt_assert(!PT_is_empty_chain<ChainIteratorStage1>(node));
    pt_assert(PT_chain_has_valid_entries<ChainIteratorStage1>(node));
#endif
}

POS_TREE1 *PT_change_leaf_to_node(POS_TREE1 *node) { // @@@ become member
    pt_assert_stage(STAGE1);
    pt_assert(node->is_leaf());

    POS_TREE1 *father = node->get_father();

    POS_TREE1 *new_elem = (POS_TREE1 *)MEM.get(PT1_EMPTY_NODE_SIZE);
    if (father) PT_change_link_in_father(father, node, new_elem);
    MEM.put(node, PT1_LEAF_SIZE(node));
    new_elem->set_type(PT1_NODE);
    new_elem->set_father(father);

    return new_elem;
}

POS_TREE1 *PT_leaf_to_chain(POS_TREE1 *node) { // @@@ become member
    pt_assert_stage(STAGE1);
    pt_assert(node->is_leaf());

    POS_TREE1     *father = node->get_father();
    const DataLoc  loc(node);

    const size_t  chain_size = sizeof(POS_TREE1)+sizeof(PT_short_chain_header);
    POS_TREE1    *new_elem   = (POS_TREE1 *)MEM.get(chain_size);

    PT_change_link_in_father(father, node, new_elem);
    MEM.put(node, PT1_LEAF_SIZE(node));

    new_elem->set_type(PT1_CHAIN);
    new_elem->set_father(father);

    new_elem->flags |= SHORT_CHAIN_HEADER_FLAG_BIT; // "has short header"

    pt_assert((new_elem->flags & SHORT_CHAIN_HEADER_SIZE_MASK) == 0); // zero elements

    PT_add_to_chain(new_elem, loc);

    return new_elem;
}

POS_TREE1 *PT_create_leaf(POS_TREE1 **pfather, PT_base base, const DataLoc& loc) {
    pt_assert_stage(STAGE1);
    pt_assert(base<PT_BASES);

    int leafsize = PT1_EMPTY_LEAF_SIZE;

    if (loc.get_rel_pos()>PT_SHORT_SIZE) leafsize += 2;
    if (loc.get_abs_pos()>PT_SHORT_SIZE) leafsize += 2;
    if (loc.get_name()   >PT_SHORT_SIZE) leafsize += 2;

    POS_TREE1 *node = (POS_TREE1 *)MEM.get(leafsize);

    pt_assert(node->get_father() == NULL); // cause memory is cleared

    if (pfather) {
        POS_TREE1 *father = *pfather;

        int        oldfathersize  = PT1_NODE_SIZE(father);
        POS_TREE1 *new_elemfather = (POS_TREE1 *)MEM.get(oldfathersize + sizeof(PT_PNTR));
        new_elemfather->set_type(PT1_NODE);

        POS_TREE1 *gfather = father->get_father();
        if (gfather) {
            PT_change_link_in_father(gfather, father, new_elemfather);
            new_elemfather->set_father(gfather);
        }

        long sbase   = 0;
        long dbase   = 0;
        int  basebit = 1 << base;

        pt_assert((father->flags&basebit) == 0); // son already exists!

        for (int i = 1; i < (1<<FLAG_FREE_BITS); i = i << 1) {
            if (i & father->flags) { // existing son
                POS_TREE1 *son = PT_read_pointer<POS_TREE1>(father->udata()+sbase);
                if (!son->is_saved()) son->set_father(new_elemfather);

                PT_write_pointer(new_elemfather->udata()+dbase, son);

                sbase += sizeof(PT_PNTR);
                dbase += sizeof(PT_PNTR);
            }
            else if (i & basebit) { // newly created leaf
                PT_write_pointer(new_elemfather->udata()+dbase, node);
                node->set_father(new_elemfather);
                dbase += sizeof(PT_PNTR);
            }
        }

        new_elemfather->flags = father->flags | basebit;
        MEM.put(father, oldfathersize);
        *pfather = new_elemfather;
    }
    node->set_type(PT1_LEAF);

    char *dest = node->udata();
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

void POS_TREE1::clear_fathers() {
    if (!is_saved()) {
        set_father(NULL);
        if (is_node()) {
            for (int i = PT_QU; i < PT_BASES; i++) {
                POS_TREE1 *son = PT_read_son(this, (PT_base)i);
                if (son) son->clear_fathers();
            }
        }
    }
}

#ifdef ARB_64
void PTD_put_longlong(FILE *out, ULONG i) {
    pt_assert(i == (unsigned long) i);
    const size_t SIZE = 8;
    STATIC_ASSERT(sizeof(uint_big) == SIZE); // this function only works and only gets called at 64-bit

    unsigned char buf[SIZE];
    PT_write_long(buf, i);
    ASSERT_RESULT(size_t, SIZE, fwrite(buf, 1, SIZE, out));
}
#endif

void PTD_put_int(FILE *out, ULONG i) {
    pt_assert(i == (unsigned int) i);
    const size_t SIZE = 4;
#ifndef ARB_64
    STATIC_ASSERT(sizeof(PT_PNTR) == SIZE); // in 32bit mode ints are used to store pointers
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

const int BITS_FOR_SIZE = 4; // size is stored inside POS_TREEx->flags, if it fits into lower 4 bits
const int SIZE_MASK     = (1<<BITS_FOR_SIZE)-1;

STATIC_ASSERT(PT1_BASE_SIZE <= PT1_EMPTY_LEAF_SIZE);
STATIC_ASSERT(PT1_BASE_SIZE <= PT1_CHAIN_SHORT_HEAD_SIZE);
STATIC_ASSERT(PT1_BASE_SIZE <= PT1_EMPTY_NODE_SIZE);

const uint_32 MIN_SIZE_IN_FLAGS = PT1_BASE_SIZE;
const uint_32 MAX_SIZE_IN_FLAGS = PT1_BASE_SIZE+SIZE_MASK-1;

const int FLAG_SIZE_REDUCTION = MIN_SIZE_IN_FLAGS-1;

static void PTD_set_object_to_saved_status(POS_TREE1 *node, long pos_start, uint_32 former_size) {
    pt_assert_stage(STAGE1);
    node->flags = 0x20; // sets node type to PT1_SAVED; see PT_prefixtree.cxx@PT1_SAVED

    char *no_father = (char*)(&node->father);

    PT_write_big(no_father, pos_start);

    pt_assert(former_size >= PT1_BASE_SIZE);

    STATIC_ASSERT((MAX_SIZE_IN_FLAGS-MIN_SIZE_IN_FLAGS+1) == SIZE_MASK); // ????0000 means "size not stored in flags"

    if (former_size >= MIN_SIZE_IN_FLAGS && former_size <= MAX_SIZE_IN_FLAGS) {
        pt_assert(former_size >= int(sizeof(uint_big)));
        node->flags |= former_size-FLAG_SIZE_REDUCTION;
    }
    else {
        pt_assert(former_size >= int(sizeof(uint_big)+sizeof(int)));
        PT_write_int(no_father+sizeof(uint_big), former_size);
    }
}

inline int get_memsize_of_saved(const POS_TREE1 *node) {
    int memsize = node->flags & SIZE_MASK;
    if (memsize) {
        memsize += FLAG_SIZE_REDUCTION;
    }
    else {
        memsize = PT_read_int(node->udata());
    }
    return memsize;
}

static long PTD_write_tip_to_disk(FILE *out, POS_TREE1 *node, const long oldpos) {
    fputc(node->flags, out);         // save type
    int size = PT1_LEAF_SIZE(node);
    // write 4 bytes when not in stage 2 save mode

    size_t cnt = size-sizeof(PT_PNTR)-1;               // no father; type already saved
    ASSERT_RESULT(size_t, cnt, fwrite(node->udata(), 1, cnt, out)); // write name rpos apos

    long pos = oldpos+size-sizeof(PT_PNTR);                // no father
    pt_assert(pos >= 0);
    PTD_set_object_to_saved_status(node, oldpos, size);
    return pos;
}

static long PTD_write_chain_to_disk(FILE *out, POS_TREE1 * const node, const long oldpos, ARB_ERROR& error) {
    pt_assert_valid_chain_stage1(node);

    long                pos          = oldpos;
    ChainIteratorStage1 entry(node);
    uint_32             chain_length = 1+entry.get_elements_ahead();

    pt_assert(chain_length>0);

    int    refabspos     = entry.get_refabspos(); // @@@ search for better apos?
    uint_8 has_long_apos = refabspos>0xffff;

    PTD_put_byte(out, (node->flags & 0xc0) | has_long_apos); // save type
    pos++;

    if (has_long_apos) { PTD_put_int  (out, refabspos); pos += 4; }
    else               { PTD_put_short(out, refabspos); pos += 2; }

    pos += PTD_put_compact_nat(out, chain_length);

    ChainEntryBuffer buf(chain_length, refabspos, 0);
    uint_32 entries  = 0;

    while (entry && !error) {
        const AbsLoc& loc = entry.at();
        if (loc.get_name()<buf.get_lastname()) {
            error = GBS_global_string("Chain Error: name order error (%i < %i)", loc.get_name(), buf.get_lastname());
        }
        else {
            buf.add(loc);
            ++entry;
        }
        ++entries;
    }

    if (buf.get_size() != fwrite(buf.get_mem(), 1, buf.get_size(), out)) {
        error = GB_IO_error("writing chains to", "ptserver-index");
    }
    pos += buf.get_size();

    pt_assert(entries == chain_length);
    pt_assert(pos >= 0);

    {
        bool is_short        = node->flags&SHORT_CHAIN_HEADER_FLAG_BIT;
        int  chain_head_size = sizeof(POS_TREE1)+(is_short ? sizeof(PT_short_chain_header) : sizeof(PT_long_chain_header));

        if (!is_short) {
            PT_long_chain_header *header = reinterpret_cast<PT_long_chain_header*>(node->udata());
            MEM.put(header->entrymem, header->memsize);
        }

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

void PTD_delete_saved_node(POS_TREE1*& node) {
    MEM.put(node, get_memsize_of_saved(node));
    node = NULL;
}

static long PTD_write_node_to_disk(FILE *out, POS_TREE1 *node, long *r_poss, const long oldpos) {
    // Save node after all descendends are already saved

    pt_assert_stage(STAGE1);

    long max_diff = 0;
    int  lasti    = 0;
    int  count    = 0;
    int  size     = PT1_EMPTY_NODE_SIZE;
    int  mysize   = PT1_EMPTY_NODE_SIZE;

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
    // returns new position in index-file (unchanged for type PT1_SAVED)
    // *node_pos is set to the start-position of the most recent object written

    pt_assert_stage(STAGE1);

    switch (node->get_type()) {
        case PT1_SAVED:
            *node_pos = PT_read_big(&node->father);
            break;

        case PT1_LEAF:
            *node_pos = pos;
            pos       = PTD_write_tip_to_disk(out, node, pos);
            break;

        case PT1_CHAIN:
            *node_pos = pos;
            pos       = PTD_write_chain_to_disk(out, node, pos, error);
            pt_assert(node->is_saved());
            break;

        case PT1_NODE: {
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
        case PT1_UNDEF:
            pt_assert(0);
            break;
    }

    pt_assert(error || node->is_saved());
    return pos;
}

ARB_ERROR PTD_read_leafs_from_disk(const char *fname, POS_TREE2*& root_ptr) { // __ATTR__USERESULT
    pt_assert_stage(STAGE2);

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
            STATIC_ASSERT(sizeof(PT_PNTR) == 8);  // 0xffffffff is used to signalize to be compatible with older pt-servers
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

        short info_size = PT_read_short(main);

#ifndef ARB_64
        bool info_detected = false;
#endif

        if (info_size>0 && info_size<(main-buffer)) {   // otherwise impossible size
            main -= info_size;

            long magic   = PT_read_int(main); main += 4;
            int  version = PT_read_char(main); main++;

            pt_assert(PT_SERVER_MAGIC>0 && PT_SERVER_MAGIC<INT_MAX);

            if (magic == PT_SERVER_MAGIC) {
#ifndef ARB_64
                info_detected = true;
#endif
                if (version != PT_SERVER_VERSION) {
                    error = "PT-server database has different version (rebuild necessary)";
                }
            }
        }

#ifndef ARB_64
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
            root_ptr  = (POS_TREE2*)(i+buffer);
        }
    }

    return error;
}

// --------------------------------------------------------------------------------

#if defined(PTM_MEM_DUMP_STATS)
const char *get_blocksize_description(int blocksize) {
    const char *known = NULL;
    switch (blocksize) {
        case PT1_EMPTY_NODE_SIZE:       known = "PT1_EMPTY_NODE_SIZE"; break;
        case PT1_MIN_CHAIN_ENTRY_SIZE:  known = "PT1_MIN_CHAIN_ENTRY_SIZE"; break;
        case PT1_MAX_CHAIN_ENTRY_SIZE:  known = "PT1_MAX_CHAIN_ENTRY_SIZE"; break;
#if defined(ARB_64)
        case PT1_EMPTY_LEAF_SIZE:       known = "PT1_EMPTY_LEAF_SIZE"; break;
        case PT1_CHAIN_SHORT_HEAD_SIZE: known = "PT1_CHAIN_SHORT_HEAD_SIZE"; break;
        case PT1_CHAIN_LONG_HEAD_SIZE:  known = "PT1_CHAIN_LONG_HEAD_SIZE"; break;
        case PT1_NODE_WITHSONS_SIZE(2): known = "PT1_NODE_WITHSONS_SIZE(2)"; break;
#else // 32bit:
        case PT1_EMPTY_LEAF_SIZE:       known = "PT1_EMPTY_LEAF_SIZE      and PT1_CHAIN_SHORT_HEAD_SIZE"; break;
        case PT1_NODE_WITHSONS_SIZE(2): known = "PT1_NODE_WITHSONS_SIZE(2) and PT1_CHAIN_LONG_HEAD_SIZE"; break;

        STATIC_ASSERT(PT1_CHAIN_SHORT_HEAD_SIZE == PT1_EMPTY_LEAF_SIZE);
        STATIC_ASSERT(PT1_CHAIN_LONG_HEAD_SIZE == PT1_NODE_WITHSONS_SIZE(2));
#endif
        case PT1_NODE_WITHSONS_SIZE(1): known = "PT1_NODE_WITHSONS_SIZE(1)"; break;
        case PT1_NODE_WITHSONS_SIZE(3): known = "PT1_NODE_WITHSONS_SIZE(3)"; break;
        case PT1_NODE_WITHSONS_SIZE(4): known = "PT1_NODE_WITHSONS_SIZE(4)"; break;
        case PT1_NODE_WITHSONS_SIZE(5): known = "PT1_NODE_WITHSONS_SIZE(5)"; break;
        case PT1_NODE_WITHSONS_SIZE(6): known = "PT1_NODE_WITHSONS_SIZE(6)"; break;
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

    uint_32 unmodified = 0xffffffff;
    PT_write_int(node->udata(), unmodified);

    PTD_set_object_to_saved_status(node, 0, size);

    expectation_group expected;
    expected.add(that(node->get_type()).is_equal_to(PT1_SAVED));
    expected.add(that(get_memsize_of_saved(node)).is_equal_to(size));

    uint_32 data_after = PT_read_int(node->udata());

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
    static bool initialized;
    EnterStage1() {
        if (initialized) PT_exit_psg();
        PT_init_psg();
        psg.enter_stage(STAGE1);
        initialized = true;
    }
    ~EnterStage1() {
        pt_assert(initialized);
        PT_exit_psg();
        initialized = false;
    }
};
bool EnterStage1::initialized = false;

void TEST_saved_state() {
    EnterStage1 env;

    const size_t SIZE = PT1_BASE_SIZE+6;
    STATIC_ASSERT(SIZE >= (1+sizeof(uint_big)+sizeof(int)));

    POS_TREE1 *node = (POS_TREE1*)malloc(SIZE);

    TEST_SIZE_SAVED_IN_FLAGS(node, PT1_BASE_SIZE);

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
    STATIC_ASSERT(PT1_BASE_SIZE == 9);
#else
    TEST_SIZE_SAVED_IN_FLAGS(node, 8);
    TEST_SIZE_SAVED_IN_FLAGS(node, 7);
    TEST_SIZE_SAVED_IN_FLAGS(node, 6);
    TEST_SIZE_SAVED_IN_FLAGS(node, 5);

    STATIC_ASSERT(PT1_BASE_SIZE == 5);
#endif

    free(node);
}

#if defined(ENABLE_CRASH_TESTS)
// # define TEST_BAD_CHAINS // TEST_chains fails in PTD_save_partial_tree if uncommented (as expected)
#endif

#if defined(TEST_BAD_CHAINS)

static POS_TREE1 *theChain = NULL;
static DataLoc   *theLoc   = NULL;

static void bad_add_to_chain() {
    PT_add_to_chain(theChain, *theLoc);
#if !defined(PTM_DEBUG_VALIDATE_CHAINS)
    pt_assert(PT_chain_has_valid_entries<ChainIteratorStage1>(theChain));
#endif
    theChain = NULL;
    theLoc   = NULL;
}
#endif

void TEST_chains() {
    EnterStage1 env;
    psg.data_count = 3;

    POS_TREE1 *root = PT_create_leaf(NULL, PT_N, DataLoc(0, 0, 0));
    TEST_EXPECT_EQUAL(root->get_type(), PT1_LEAF);

    root = PT_change_leaf_to_node(root);
    TEST_EXPECT_EQUAL(root->get_type(), PT1_NODE);

    DataLoc loc0a(0, 500, 500);
    DataLoc loc0b(0, 555, 555);

    DataLoc loc1a(1, 200, 200);
    DataLoc loc1b(1, 500, 500);

    DataLoc loc2a(2, 300, 300);
    DataLoc loc2b(2, 700, 700);
    DataLoc loc2c(2, 701, 701);

    for (int base = PT_A; base <= PT_G; ++base) {
        POS_TREE1 *leaf = PT_create_leaf(&root, PT_base(base), loc0b);
        TEST_EXPECT_EQUAL(leaf->get_type(), PT1_LEAF);

        POS_TREE1 *chain = PT_leaf_to_chain(leaf);
        TEST_EXPECT_EQUAL(chain->get_type(), PT1_CHAIN);
        TEST_EXPECT(PT_chain_has_valid_entries<ChainIteratorStage1>(chain));

        PT_add_to_chain(chain, loc0a); TEST_EXPECT(PT_chain_has_valid_entries<ChainIteratorStage1>(chain));
        PT_add_to_chain(chain, loc1b); TEST_EXPECT(PT_chain_has_valid_entries<ChainIteratorStage1>(chain));
        PT_add_to_chain(chain, loc1a); TEST_EXPECT(PT_chain_has_valid_entries<ChainIteratorStage1>(chain));
        PT_add_to_chain(chain, loc2c); TEST_EXPECT(PT_chain_has_valid_entries<ChainIteratorStage1>(chain));
        PT_add_to_chain(chain, loc2b); TEST_EXPECT(PT_chain_has_valid_entries<ChainIteratorStage1>(chain));
        PT_add_to_chain(chain, loc2a); TEST_EXPECT(PT_chain_has_valid_entries<ChainIteratorStage1>(chain));

        // now chain is 'loc0b,loc0a,loc1b,loc1a,loc2b,loc2a'

        if (base == PT_A) { // test only once
            ChainIteratorStage1 entry(chain);

            TEST_EXPECT_EQUAL(bool(entry), true); TEST_EXPECT(entry.at() == loc0b); ++entry;
            TEST_EXPECT_EQUAL(bool(entry), true); TEST_EXPECT(entry.at() == loc0a); ++entry;
            TEST_EXPECT_EQUAL(bool(entry), true); TEST_EXPECT(entry.at() == loc1b); ++entry;
            TEST_EXPECT_EQUAL(bool(entry), true); TEST_EXPECT(entry.at() == loc1a); ++entry;
            TEST_EXPECT_EQUAL(bool(entry), true); TEST_EXPECT(entry.at() == loc2c); ++entry;
            TEST_EXPECT_EQUAL(bool(entry), true); TEST_EXPECT(entry.at() == loc2b); ++entry;
            TEST_EXPECT_EQUAL(bool(entry), true); TEST_EXPECT(entry.at() == loc2a); ++entry;
            TEST_EXPECT_EQUAL(bool(entry), false);
        }

#if defined(TEST_BAD_CHAINS)
        // out-of-date
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
        TEST_EXPECT_EQUAL(leaf->get_type(), PT1_CHAIN);
    }

    // since there is no explicit code to free POS_TREE-memory, spool it into /dev/null
    {
        FILE      *out = fopen("/dev/null", "wb");
        ARB_ERROR  error;
        long       root_pos;

        long pos = PTD_save_lower_tree(out, root, 0, error);
        pos      = PTD_save_upper_tree(out, root, pos, root_pos, error);

        TEST_EXPECT_NO_ERROR(error.deliver());
        TEST_EXPECTATION(all().of(that(root_pos).is_equal_to(74), that(pos).is_equal_to(79)));
        TEST_EXPECT_NULL(root); // nulled by PTD_save_upper_tree

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
