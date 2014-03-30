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

#include <arb_file.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <climits>

struct PTM_struct PTM;
char PT_count_bits[PT_B_MAX+1][256]; // returns how many bits are set

static void PT_init_count_bits() {
    unsigned int base;
    unsigned int count;
    unsigned int i, h, j;
    for (base = PT_QU; base <= PT_B_MAX; base++) {
        for (i=0; i<256; i++) {
            h = 0xff >> (8-base);
            h &= i;
            count = 0;
            for (j=0; j<8; j++) {
                if (h&1) count++;
                h = h>>1;
            }
            PT_count_bits[base][i] = count;
        }
    }
}

static void PTM_add_alloc(void *ptr) {
    if (PTM.alloc_counter == PTM.alloc_array_size) {
        if (PTM.alloc_array_size == 0)
            PTM.alloc_array_size = 1;

        PTM.alloc_array_size = PTM.alloc_array_size * 2;
        void **new_array = (void **) malloc(PTM.alloc_array_size
                * sizeof(void*));

        if (!new_array)
            abort();

        void *old_array = PTM.alloc_ptr;
        memcpy(new_array, old_array, PTM.alloc_counter * sizeof(void*));
        PTM.alloc_ptr = new_array;
        free(old_array);
    }
    PTM.alloc_ptr[PTM.alloc_counter++] = ptr;
}

void PTM_finally_free_all_mem() {
    for (unsigned long i = 0; i < PTM.alloc_counter; ++i)
        free(PTM.alloc_ptr[i]);
    PTM.alloc_counter = 0;
    PTM.alloc_array_size = 0;
    free(PTM.alloc_ptr);
}

static char *PTM_get_mem(int size) {
    //! allocate 'size' bytes

    int   nsize, pos;
    long  i;
    char *erg;

    nsize = (size + (PTM_ALIGNED - 1)) & (-PTM_ALIGNED);
    if (nsize > PTM_MAX_SIZE) {
        void * ptr = calloc(1, size);
        PTM_add_alloc(ptr);
        return (char *) ptr;
    }
    pos = nsize >> PTM_LD_ALIGNED;

#ifdef PTM_DEBUG
    PTM.debug[pos]++;
#endif

    if ((erg = PTM.tables[pos])) {
        PT_READ_PNTR(((char *)PTM.tables[pos]), i);
        PTM.tables[pos] = (char *)i;
    }
    else {
        if (PTM.size < nsize) {
            PTM.data         = (char *) calloc(1, PTM_TABLE_SIZE);
            PTM.size         = PTM_TABLE_SIZE;
            PTM.allsize     += PTM_TABLE_SIZE;
#ifdef PTM_DEBUG
            static int less  = 0;
            if ((less%10) == 0) {
                printf("Memory usage: %li byte\n", PTM.allsize);
            }
            ++less;
#endif
            PTM_add_alloc(PTM.data);
        }
        erg = PTM.data;
        PTM.data += nsize;
        PTM.size -= nsize;
    }
    memset(erg, 0, nsize);
    return erg;
}

static void PTM_free_mem(char *data, int size) {
    //! free memory allocated by PTM_get_mem()

    pt_assert(size > 0);
    int  nsize, pos;
    long i;
    nsize = (size + (PTM_ALIGNED - 1)) & (-PTM_ALIGNED);
    if (nsize > PTM_MAX_SIZE) {
        free(data);
    }
    else {
        // if (data[4] == PTM_magic) PT_CORE
        pos = nsize >> PTM_LD_ALIGNED;

#ifdef PTM_DEBUG
        PTM.debug[pos]--;
#endif
        i = (long)PTM.tables[pos];
        PT_WRITE_PNTR(data, i);
        data[sizeof(PT_PNTR)] = PTM_magic;
        PTM.tables[pos] = data;
    }
}

PTM2 *PT_init() {
    PTM2 *ptmain;
    memset(&PTM, 0, sizeof(struct PTM_struct));
    ptmain = (PTM2 *)calloc(1, sizeof(PTM2));
    ptmain->mode = sizeof(PT_PNTR);
    ptmain->stage1 = 1;
    int i;
    for (i=0; i<256; i++) {
        if      ((i&0xe0) == 0x20) PTM.flag_2_type[i] = PT_NT_SAVED;
        else if ((i&0xe0) == 0x00) PTM.flag_2_type[i] = PT_NT_LEAF;
        else if ((i&0x80) == 0x80) PTM.flag_2_type[i] = PT_NT_NODE;
        else if ((i&0xe0) == 0x40) PTM.flag_2_type[i] = PT_NT_CHAIN;
        else                       PTM.flag_2_type[i] = PT_NT_UNDEF;
    }
    PT_init_count_bits();

    PTM.alloc_ptr = NULL;
    PTM.alloc_counter = 0;
    PTM.alloc_array_size = 0;

    return ptmain;
}

// ------------------------------
//      functions for stage 1

static void PT_change_father(POS_TREE *father, POS_TREE *source, POS_TREE *dest) { // stage 1
    long i, j;
    i = PT_count_bits[PT_B_MAX][father->flags];
    for (; i>0; i--) {
        PT_READ_PNTR((&father->data)+sizeof(PT_PNTR)*i, j);
        if (j==(long)source) {
            PT_WRITE_PNTR((&father->data)+sizeof(PT_PNTR)*i, (long)dest);
            return;
        }
    }
    PT_CORE;
}

POS_TREE *PT_add_to_chain(POS_TREE *node, const DataLoc& loc) { // stage1
    // insert at the beginning of list

    char *data  = (&node->data) + psg.ptmain->mode;
    data       += (node->flags&1) ? 4 : 2;

    unsigned long old_first;
    PT_READ_PNTR(data, old_first);  // create a new list element

    static char  buffer[100];
    char        *p = buffer;

    PT_WRITE_PNTR(p, old_first);
    p += sizeof(PT_PNTR);

    PT_WRITE_NAT(p, loc.name);
    PT_WRITE_NAT(p, loc.rpos);
    PT_WRITE_NAT(p, loc.apos);
    
    int size = p - buffer;
    p        = (char *)PTM_get_mem(size);
    memcpy(p, buffer, size);
    PT_WRITE_PNTR(data, p);
    psg.stat.cut_offs ++;
    return NULL;
}

POS_TREE *PT_change_leaf_to_node(POS_TREE *node) { // stage 1
    if (PT_GET_TYPE(node) != PT_NT_LEAF) PT_CORE;
    
    long i;
    PT_READ_PNTR((&node->data), i);

    POS_TREE *father   = (POS_TREE *)i;
    POS_TREE *new_elem = (POS_TREE *)PTM_get_mem(PT_EMPTY_NODE_SIZE);
    if (father) PT_change_father(father, node, new_elem);
    PTM_free_mem((char *)node, PT_LEAF_SIZE(node));
    PT_SET_TYPE(new_elem, PT_NT_NODE, 0);
    PT_WRITE_PNTR((&(new_elem->data)), (long)father);

    return new_elem;
}

POS_TREE *PT_leaf_to_chain(POS_TREE *node) { // stage 1
    if (PT_GET_TYPE(node) != PT_NT_LEAF) PT_CORE;

    long i;
    PT_READ_PNTR((&node->data), i);

    POS_TREE      *father = (POS_TREE *)i;
    const DataLoc  loc(node);

    int chain_size                          = PT_EMPTY_CHAIN_SIZE;
    if (loc.apos>PT_SHORT_SIZE) chain_size += 2;

    POS_TREE *new_elem = (POS_TREE *)PTM_get_mem(chain_size);
    PT_change_father(father, node, new_elem);
    PTM_free_mem((char *)node, PT_LEAF_SIZE(node));
    PT_SET_TYPE(new_elem, PT_NT_CHAIN, 0);
    PT_WRITE_PNTR((&new_elem->data), (long)father);
    
    char *data = (&new_elem->data)+sizeof(PT_PNTR);
    if (loc.apos>PT_SHORT_SIZE) {                                   
        PT_WRITE_INT(data, loc.apos);                               
        data+=4;
        new_elem->flags|=1;
    }
    else {                                                      
        PT_WRITE_SHORT(data, loc.apos);
        data+=2;
    }
    PT_WRITE_PNTR(data, NULL);
    PT_add_to_chain(new_elem, loc);

    return new_elem;
}

POS_TREE *PT_create_leaf(POS_TREE ** pfather, PT_BASES base, const DataLoc& loc) { // stage 1
    POS_TREE *father, *node, *new_elemfather;
    int       base2;
    int       leafsize;
    char     *dest;
    leafsize = PT_EMPTY_LEAF_SIZE;

    if (loc.rpos>PT_SHORT_SIZE) leafsize += 2;
    if (loc.apos>PT_SHORT_SIZE) leafsize += 2;
    if (loc.name>PT_SHORT_SIZE) leafsize += 2;
    node                              = (POS_TREE *) PTM_get_mem(leafsize);
    if (base >= PT_B_MAX)
        *(int *) 0                    = 0;
    if (pfather) {
        int             oldfathersize;
        POS_TREE       *gfather, *son;
        long             i, j, sbase, dbase;
        father = *pfather;
        PT_NODE_SIZE(father, oldfathersize);
        new_elemfather = (POS_TREE *)PTM_get_mem(oldfathersize + sizeof(PT_PNTR));
        PT_READ_PNTR(&father->data, j);
        gfather = (POS_TREE *)j;
        if (gfather) {
            PT_change_father(gfather, father, new_elemfather);
            PT_WRITE_PNTR(&new_elemfather->data, gfather);
        }
        sbase = dbase = sizeof(PT_PNTR);
        base2 = 1 << base;
        for (i = 1; i < 0x40; i = i << 1) {
            if (i & father->flags) {
                PT_READ_PNTR(((char *) &father->data) + sbase, j);
                son = (POS_TREE *)j;
                int sontype = PT_GET_TYPE(son);
                if (sontype != PT_NT_SAVED) {
                    PT_WRITE_PNTR((char *) &son->data, new_elemfather);
                }
                PT_WRITE_PNTR(((char *) &new_elemfather->data) + dbase, son);
                sbase += sizeof(PT_PNTR);
                dbase += sizeof(PT_PNTR);
                continue;
            }
            if (i & base2) {
                PT_WRITE_PNTR(((char *) &new_elemfather->data) + dbase, node);
                PT_WRITE_PNTR((&node->data), (PT_PNTR)new_elemfather);
                dbase += sizeof(PT_PNTR);
            }
        }
        new_elemfather->flags = father->flags | base2;
        PTM_free_mem((char *)father, oldfathersize);
        PT_SET_TYPE(node, PT_NT_LEAF, 0);
        *pfather = new_elemfather;
    }
    PT_SET_TYPE(node, PT_NT_LEAF, 0);
    dest = (&node->data) + sizeof(PT_PNTR);
    if (loc.name>PT_SHORT_SIZE) {
        PT_WRITE_INT(dest, loc.name);
        node->flags |= 1;
        dest += 4;
    }
    else {
        PT_WRITE_SHORT(dest, loc.name);
        dest += 2;
    }
    if (loc.rpos>PT_SHORT_SIZE) {
        PT_WRITE_INT(dest, loc.rpos);
        node->flags |= 2;
        dest += 4;
    }
    else {
        PT_WRITE_SHORT(dest, loc.rpos);
        dest += 2;
    }
    if (loc.apos>PT_SHORT_SIZE) {
        PT_WRITE_INT(dest, loc.apos);
        node->flags |= 4;
        dest += 4;
    }
    else {
        PT_WRITE_SHORT(dest, loc.apos);
        dest += 2;
    }
    if (base == PT_QU)
        return PT_leaf_to_chain(node);
    return node;
}

// ------------------------------------
//      functions for stage 1: save

void PTD_clear_fathers(POS_TREE * node) { // stage 1
    PT_NODE_TYPE type = PT_read_type(node);
    if (type != PT_NT_SAVED) {
        PT_WRITE_PNTR((&node->data), NULL);
        if (type == PT_NT_NODE) {
            for (int i = PT_QU; i < PT_B_MAX; i++) {
                POS_TREE *sons = PT_read_son(node, (PT_BASES)i);
                if (sons) PTD_clear_fathers(sons);
            }
        }
    }
}

#ifdef ARB_64
void PTD_put_longlong(FILE * out, ULONG i)
{
    pt_assert(i == (unsigned long) i);
    COMPILE_ASSERT(sizeof(PT_PNTR) == 8);       // this function only work and only get called at 64-bit
    int io;
    static unsigned char buf[8];
    PT_WRITE_PNTR(buf, i);

    io = buf[0]; putc(io, out);             // TODO: replace with fwrite
    io = buf[1]; putc(io, out);
    io = buf[2]; putc(io, out);
    io = buf[3]; putc(io, out);
    io = buf[4]; putc(io, out);
    io = buf[5]; putc(io, out);
    io = buf[6]; putc(io, out);
    io = buf[7]; putc(io, out);
}
#endif
void PTD_put_int(FILE * out, ULONG i)
{
    pt_assert(i == (unsigned int) i);
    int io;
    static unsigned char buf[4];
    PT_WRITE_INT(buf, i);
    io = buf[0]; putc(io, out);             // TODO: replace with fwrite
    io = buf[1]; putc(io, out);
    io = buf[2]; putc(io, out);
    io = buf[3]; putc(io, out);
}

void PTD_put_short(FILE * out, ULONG i)
{
    pt_assert(i == (unsigned short) i);
    int io;
    static unsigned char buf[2];
    PT_WRITE_SHORT(buf, i);
    io = buf[0]; putc(io, out);             // TODO: replace with fwrite
    io = buf[1]; putc(io, out);
}

static void PTD_set_object_to_saved_status(POS_TREE * node, long pos, int size) {
    node->flags = 0x20;
    PT_WRITE_PNTR((&node->data), pos);
    if (size < 20) {
        node->flags |= size-sizeof(PT_PNTR);
    }
    else {
        PT_WRITE_INT((&node->data)+sizeof(PT_PNTR), size);
    }
}

static long PTD_write_tip_to_disk(FILE * out, POS_TREE * node, long pos)
{
    int size, cnt;
    putc(node->flags, out);         // save type
    size = PT_LEAF_SIZE(node);
    // write 4 bytes when not in stage 2 save mode

    cnt = size-sizeof(PT_PNTR)-1;               // no father; type already saved
#ifdef ARB_64
    fwrite(&node->data + sizeof(PT_PNTR), 0x01, cnt, out);   // write name rpos apos
#else
    for (char *data = (&node->data)+sizeof(PT_PNTR); cnt; cnt--) { // write apos rpos name
        int i = (int)(*(data++));
        putc(i, out);
    }
#endif
    PTD_set_object_to_saved_status(node, pos, size);
    pos += size-sizeof(PT_PNTR);                // no father
    pt_assert(pos >= 0);
    return pos;
}

static int ptd_count_chain_entries(char * entry) {
    int counter   = 0;
    long next;
    while (entry) {
        counter ++;
        PT_READ_PNTR(entry, next);
        entry = (char *)next;
    }
    pt_assert(counter >= 0);
    return counter;
}

static void ptd_set_chain_references(char *entry, char **entry_tab) {
    int counter   = 0;
    long next;
    while (entry) {
        entry_tab[counter] = entry;
        counter ++;
        PT_READ_PNTR(entry, next);
        entry = (char *)next;
    }
}

static ARB_ERROR ptd_write_chain_entries(FILE * out, long *ppos, char ** entry_tab,  int n_entries, int mainapos) { // __ATTR__USERESULT
    ARB_ERROR   error;
    int         lastname = 0;
    
    while (n_entries>0 && !error) {
        char *entry = entry_tab[n_entries-1];
        n_entries --;
        char *rp = entry;
        rp += sizeof(PT_PNTR);

        int name;
        int rpos;
        int apos;
        PT_READ_NAT(rp, name);
        PT_READ_NAT(rp, rpos);
        PT_READ_NAT(rp, apos);
        if (name < lastname) {
            error = GBS_global_string("Chain Error: name order error %i < %i\n", name, lastname);
        }
        else {
            static char  buffer[100];
            char        *wp   = buffer;
            wp                = PT_WRITE_CHAIN_ENTRY(wp, mainapos, name-lastname, apos, rpos);
            int          size = wp -buffer;

            if (1 != fwrite(buffer, size, 1, out)) {
                error = GB_IO_error("writing chains to", "ptserver-index");
            }
            else {
                *ppos    += size;
                PTM_free_mem(entry, rp-entry);
                lastname  = name;
            }
        }
    }

    return error;
}


static long PTD_write_chain_to_disk(FILE * out, POS_TREE * node, long pos, ARB_ERROR& error) {
    char *data;
    long oldpos = pos;
    putc(node->flags, out);         // save type
    pos++;
    int mainapos;
    data = (&node->data) + psg.ptmain->mode;

    if (node->flags&1) {
        PT_READ_INT(data, mainapos);
        PTD_put_int(out, mainapos);
        data += 4;
        pos += 4;
    }
    else {
        PT_READ_SHORT(data, mainapos);
        PTD_put_short(out, mainapos);
        data += 2;
        pos += 2;
    }
    long first_entry;
    PT_READ_PNTR(data, first_entry);
    int n_entries = ptd_count_chain_entries((char *)first_entry);
    {
        char **entry_tab = (char **)GB_calloc(sizeof(char *), n_entries);
        ptd_set_chain_references((char *)first_entry, entry_tab);
        error = ptd_write_chain_entries(out, &pos, entry_tab, n_entries, mainapos);
        free(entry_tab);
    }
    putc(PT_CHAIN_END, out);
    pos++;
    PTD_set_object_to_saved_status(node, oldpos, data+sizeof(PT_PNTR)-(char*)node);
    pt_assert(pos >= 0);
    return pos;
}

void PTD_debug_nodes() {
#ifdef ARB_64
    printf ("Inner Node Statistic:\n");
    printf ("   Single Nodes:   %6i\n", psg.stat.single_node);
    printf ("   Short  Nodes:   %6i\n", psg.stat.short_node);
    printf ("       Chars:      %6i\n", psg.stat.chars);
    printf ("       Shorts:     %6i\n", psg.stat.shorts2);
    printf ("   Int    Nodes:   %6i\n", psg.stat.int_node);
    printf ("       Shorts:     %6i\n", psg.stat.shorts);
    printf ("       Ints:       %6i\n", psg.stat.ints2);
    printf ("   Long   Nodes:   %6i\n", psg.stat.long_node);
    printf ("       Ints:       %6i\n", psg.stat.ints);
    printf ("       Longs:      %6i\n", psg.stat.longs);
    printf ("   maxdiff:        %6li\n", psg.stat.maxdiff);
#else
    printf ("Inner Node Statistic:\n");
    printf ("   Single Nodes:   %6i\n", psg.stat.single_node);
    printf ("   Short  Nodes:   %6i\n", psg.stat.short_node);
    printf ("       Chars:      %6i\n", psg.stat.chars);
    printf ("       Shorts:     %6i\n", psg.stat.shorts2);
    printf ("   Long   Nodes:   %6i\n", psg.stat.long_node);
    printf ("       Shorts:     %6i\n", psg.stat.shorts);
    printf ("       Longs:      %6i\n", psg.stat.longs);        // "longs" are actually 32 bit ints
#endif
}

static long PTD_write_node_to_disk(FILE * out, POS_TREE * node, long *r_poss, long pos) {
    int i, size;   // Save node after all descendends are already saved
    POS_TREE *sons;

    long max_diff = 0;
    int  lasti = 0;
    int  count = 0;
    int  mysize;

    size = PT_EMPTY_NODE_SIZE;
    mysize = PT_EMPTY_NODE_SIZE;

    for (i = PT_QU; i < PT_B_MAX; i++) {    // free all sons
        sons = PT_read_son(node, (PT_BASES)i);
        if (sons) {
            int memsize;
            long   diff = pos - r_poss[i];
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
            if (PT_GET_TYPE(sons) != PT_NT_SAVED) {
                GBK_terminate("Internal Error: Son not saved");
            }
            if ((sons->flags & 0xf) == 0) {
                PT_READ_INT((&sons->data)+sizeof(PT_PNTR), memsize);
            }
            else {
                memsize = (sons->flags &0xf) + sizeof(PT_PNTR);
            }
            PTM_free_mem((char *)sons, memsize);
            count ++;
        }
    }
    if ((count == 1) && (max_diff<=9) && (max_diff != 2)) { // nodesingle
        if (max_diff>2) max_diff -= 2; else max_diff -= 1;
        long flags = 0xc0 | lasti | (max_diff << 3);
        putc((int)flags, out);
        psg.stat.single_node++;
    }
    else {                          // multinode
        putc(node->flags, out);
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
        for (i = PT_QU; i < PT_B_MAX; i++) {    // set the flag2
            if (r_poss[i]) {
                /* u */ long  diff = pos - r_poss[i];
                pt_assert(diff >= 0);
                if (diff>level) flags2 |= 1<<i;
            }
        }
        putc(flags2, out);
        size++;
        for (i = PT_QU; i < PT_B_MAX; i++) {    // write the data
            if (r_poss[i]) {
                /* u */ long  diff = pos - r_poss[i];
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
                        putc((int)diff, out);
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
                        putc((int)diff, out);
                        size += 1;
                        psg.stat.chars++;
                    }
                }
#endif
            }
        }
    }

    PTD_set_object_to_saved_status(node, pos, mysize);
    pos += size-sizeof(PT_PNTR);                // no father
    pt_assert(pos >= 0);
    return pos;
}

long PTD_write_leafs_to_disk(FILE * out, POS_TREE * node, long pos, long *pnodepos, int *pblock, ARB_ERROR& error) {
    // returns new pos when son is written 0 otherwise
    // pnodepos is set to last object

    POS_TREE     *sons;
    long          r_pos, r_poss[PT_B_MAX], o_pos;
    int           block[10];            // TODO: check why we allocate 10 ints when only block[0] is used
    int           i;
    PT_NODE_TYPE  type = PT_read_type(node);


    if (type == PT_NT_SAVED) {          // already saved
        long father;
        PT_READ_PNTR((&node->data), father);
        *pnodepos = father;
        return 0;
    }
    else if (type == PT_NT_LEAF) {
        *pnodepos = pos;
        pos = PTD_write_tip_to_disk(out, node, pos);
    }
    else if (type == PT_NT_CHAIN) {
        *pnodepos = pos;
        pos = PTD_write_chain_to_disk(out, node, pos, error);
    }
    else if (type == PT_NT_NODE) {
        block[0] = 0;
        o_pos = pos;
        for (i = PT_QU; i < PT_B_MAX && !error; i++) {    // save all sons
            sons = PT_read_son(node, (PT_BASES)i);
            r_poss[i] = 0;
            if (sons) {
                r_pos = PTD_write_leafs_to_disk(out, sons, pos, &(r_poss[i]), &(block[0]), error);
                if (r_pos>pos) {        // really saved ????
                    pos = r_pos;
                }
            }
        }
        if (block[0]) {     // son wrote a block
            *pblock = 1;
        }
        else if (pos-o_pos > PT_BLOCK_SIZE) {
            // a block is written
            *pblock = 1;
        }
        else {          // now i can write my data
            *pnodepos = pos;
            if (!error) pos = PTD_write_node_to_disk(out, node, r_poss, pos);
        }
    }
    pt_assert(pos >= 0 || error);
    return pos;
}

// --------------------------------------
//      functions for stage 2-3: load


ARB_ERROR PTD_read_leafs_from_disk(const char *fname, POS_TREE **pnode) { // __ATTR__USERESULT
    GB_ERROR  error  = NULL;
    char     *buffer = GB_map_file(fname, 0);

    if (!buffer) {
        error = GBS_global_string("mmap failed (%s)", GB_await_error());
    }
    else {
        long  size = GB_size_of_file(fname);
        char *main = &(buffer[size-4]);

        long i;
        bool big_db = false;
        PT_READ_INT(main, i);
#ifdef ARB_64
        if (i == 0xffffffff) {                      // 0xffffffff signalizes that "last_obj" is stored
            main -= 8;                              // in the previous 8 byte (in a long long)
            COMPILE_ASSERT(sizeof(PT_PNTR) == 8);   // 0xffffffff is used to signalize to be compatible with older pt-servers
            PT_READ_PNTR(main, i);                  // this search tree can only be loaded at 64 Bit

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
        short info_size;
        PT_READ_SHORT(main, info_size);

        bool info_detected = false;
        if (info_size>0 && info_size<(main-buffer)) {   // otherwise impossible size
            main -= info_size;

            long magic;
            int  version;

            PT_READ_INT(main, magic); main += 4;
            PT_READ_CHAR(main, version); main++;

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

            psg.ptmain->mode       = 0;
            psg.ptmain->data_start = buffer;
        }
    }

    return error;
}
