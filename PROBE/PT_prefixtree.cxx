#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
// #include <malloc.h>
#include <memory.h>
#include <PT_server.h>
#define IMPLEMENT_PROBE_TREE
#include "probe.h"

#include "probe_tree.hxx"


struct PTM_struct PTM;
char PT_count_bits[PT_B_MAX+1][256]; // returns how many bits are set

/********************* build a fast conversation table ***********************/

void PT_init_count_bits(void){
    unsigned int base;
    unsigned int count;
    unsigned int i,h,j;
    for (base = PT_QU; base <= PT_B_MAX; base++) {
        for (i=0;i<256;i++){
            h = 0xff >> (8-base);
            h &= i;
            count = 0;
            for (j=0;j<8;j++) {
                if (h&1) count++;
                h = h>>1;
            }
            PT_count_bits[base][i] = count;
        }
    }
}

/********************* Self build malloc free procedures ***********************/

char *PTM_get_mem(int size)
{
    register int    nsize, pos;
    long i;
    char           *erg;
    nsize = (size + (PTM_ALIGNED - 1)) & (-PTM_ALIGNED);
    if (nsize > PTM_MAX_SIZE) {
        return (char *) calloc(1, size);
    }
    pos = nsize >> PTM_LD_ALIGNED;

#ifdef PTM_DEBUG
    PTM.debug[pos]++;
#endif

    if ( (erg = PTM.tables[pos]) ) {
        PT_READ_PNTR( ((char *)PTM.tables[pos]),i);
        PTM.tables[pos] = (char *)i;
    } else {
        if (PTM.size < nsize) {
            PTM.data         = (char *) calloc(1, PTM_TABLE_SIZE);
            PTM.size         = PTM_TABLE_SIZE;
            PTM.allsize     += PTM_TABLE_SIZE;
#ifdef PTM_DEBUG
            static int less  = 0;
            if ((less%10) == 0) {
                printf("Memory usage: %i byte\n",PTM.allsize);
            }
            ++less;
#endif
        }
        erg = PTM.data;
        PTM.data += nsize;
        PTM.size -= nsize;
    }
    memset(erg, 0, nsize);
    return erg;
}

/********************* malloc all small free blocks ***********************/
int
PTM_destroy_mem(void){  /* destroys all left memory sources */
    register int    pos,i;
    int sum;
    sum = 0;
    for (pos=0;pos<=PTM_MAX_TABLES;pos++) {
        while (PTM.tables[pos]) {
            sum += pos;
            PT_READ_PNTR( ((char *)PTM.tables[pos]),i);
            PTM.tables[pos] = (char *)i;
        }
    }
    return sum;
}
void PTM_free_mem(char *data, int size)
{
    register int    nsize, pos;
    long        i;
    nsize = (size + (PTM_ALIGNED - 1)) & (-PTM_ALIGNED);
    if (nsize > PTM_MAX_SIZE) {
        free(data);
    } else {
        /* if (data[4] == PTM_magic) PT_CORE */
        pos = nsize >> PTM_LD_ALIGNED;

#ifdef PTM_DEBUG
        PTM.debug[pos]--;
#endif
        i = (long)PTM.tables[pos];
        PT_WRITE_PNTR(data,i);
        data[sizeof(PT_PNTR)] = PTM_magic;
        PTM.tables[pos] = data;
    }
}

void PTM_debug_mem(void)
{
#ifdef PTM_DEBUG
    int i;
    for (i=1;i<(PTM_MAX_SIZE>>PTM_LD_ALIGNED);i++){
        if (PTM.debug[i]){
            printf("Size %5i used %5li times\n",i,PTM.debug[i]);
        }
    }
#endif
}

struct PT_local_struct {
    int base_count;
} *PTL = 0;

PTM2 *PT_init(int base_count)
{
    PTM2 *ptmain;
    if (!PTL) {
        PTL = (struct PT_local_struct *)
            calloc(sizeof(struct PT_local_struct),1);
    }
    memset(&PTM,0,sizeof(struct PTM_struct));
    PTL->base_count = base_count;
    ptmain = (PTM2 *)calloc(1,sizeof(PTM2));
    ptmain->mode = sizeof(PT_PNTR);
    ptmain->stage1 = 1;
    int i;
    for (i=0;i<256;i++) {
        if ((i&0xe0) == 0x20) {
            PTM.flag_2_type[i] = PT_NT_SAVED;
        }else if ((i&0xe0) == 0x00) {
            PTM.flag_2_type[i] = PT_NT_LEAF;
        }else if ((i&0x80) == 0x80) {
            PTM.flag_2_type[i] = PT_NT_NODE;
        }else if ((i&0xe0) == 0x40) {
            PTM.flag_2_type[i] = PT_NT_CHAIN;
        }else{
            PTM.flag_2_type[i] = PT_NT_UNDEF;
        }
    }
    PT_init_count_bits();
    return ptmain;
}

/******************************** funtions for all stages !!!!! *********************************/


/******************************** Debug Functions all stages *********************************/

int PTD_chain_print(int name, int apos, int rpos, long clientdata)
{
    printf("          name %6i apos %6i  rpos %i\n",name,apos,rpos);
    clientdata = clientdata;
    return 0;
}

int PTD(POS_TREE * node)
{
    long             i;
    PTM2        *ptmain = psg.ptmain;
    if (!node) printf("Zero node\n");
    PT_READ_PNTR(&node->data, i);
    printf("node father 0x%lx\n", i);
    switch (PT_read_type(node)) {
        case PT_NT_LEAF:
            printf("leaf %i:%i,%i\n", PT_read_name(ptmain,node),
                   PT_read_rpos(ptmain,node),PT_read_apos(ptmain,node));
            break;
        case PT_NT_NODE:
            for (i = 0; i < PT_B_MAX; i++) {
                printf("%6li:0x%lx\n", i, (ulong)PT_read_son(ptmain,node, (enum PT_bases_enum)i));
            }
            break;
        case PT_NT_CHAIN:
            printf("chain:\n");
            PT_read_chain(ptmain,node,PTD_chain_print,0);
            break;
        case PT_NT_SAVED:
            printf("saved:\n");
            break;
        default:
            printf("?????\n");
            break;
    }
    return 0;
}


/******************************** functions for stage 1 *********************************/

void PT_change_father(POS_TREE *father, POS_TREE *source, POS_TREE *dest){  /* stage 1 */
    long i,j;
    i = PT_count_bits[PT_B_MAX][father->flags];
    for (;i>0;i--){
        PT_READ_PNTR((&father->data)+sizeof(PT_PNTR)*i,j);
        if (j==(long)source) {
            PT_WRITE_PNTR((&father->data)+sizeof(PT_PNTR)*i,(long)dest);
            return;
        }
    }
    PT_CORE;
}

POS_TREE *PT_add_to_chain(PTM2 *ptmain, POS_TREE *node, int name, int apos, int rpos)   /*stage1*/
{
    static char buffer[100];
    unsigned long old_first;
    char *data;
    int mainapos;
    data = (&node->data) + ptmain->mode;
    if (node->flags&1){
        PT_READ_INT(data,mainapos);
        data += 4;
    }else{
        PT_READ_SHORT(data,mainapos);
        data += 2;
    }
    PT_READ_PNTR(data,old_first);   // create a new list element
    register char *p;
    p = buffer;
    PT_WRITE_PNTR(p,old_first);
    p+= sizeof(PT_PNTR);
    PT_WRITE_NAT(p,name);
    PT_WRITE_NAT(p,rpos);
    PT_WRITE_NAT(p,apos);
    int size = p - buffer;
    p = (char *)PTM_get_mem(size);
    memcpy(p,buffer,size);
    PT_WRITE_PNTR(data,p);
    psg.stat.cut_offs ++;
    return 0;
}


POS_TREE *PT_change_leaf_to_node(PTM2 */*ptmain*/, POS_TREE *node)      /*stage 1*/
{
    long i;
    POS_TREE *father,*new_elem;
    if (PT_GET_TYPE(node) != PT_NT_LEAF) PT_CORE;
    PT_READ_PNTR((&node->data),i);
    father=(POS_TREE *)i;
    new_elem = (POS_TREE *)PTM_get_mem(PT_EMPTY_NODE_SIZE);
    if (father) PT_change_father(father,node,new_elem);
    PTM_free_mem((char *)node,PT_LEAF_SIZE(node));
    PT_SET_TYPE(new_elem,PT_NT_NODE,0);
    PT_WRITE_PNTR((&(new_elem->data)),(long)father);
    return new_elem;
}

POS_TREE *PT_leaf_to_chain(PTM2 *ptmain, POS_TREE *node)        /* stage 1*/
{
    long i;
    int apos,rpos,name;
    POS_TREE *father,*new_elem;
    int chain_size;
    char    *data;
    if (PT_GET_TYPE(node) != PT_NT_LEAF) PT_CORE;
    PT_READ_PNTR((&node->data),i);
    father=(POS_TREE *)i;
    name = PT_read_name(ptmain,node);
    apos = PT_read_apos(ptmain,node);
    rpos = PT_read_rpos(ptmain,node);
    chain_size = PT_EMPTY_CHAIN_SIZE;
    if (apos>PT_SHORT_SIZE) chain_size+=2;

    new_elem = (POS_TREE *)PTM_get_mem(chain_size);
    PT_change_father(father,node,new_elem);
    PTM_free_mem((char *)node,PT_LEAF_SIZE(node));
    PT_SET_TYPE(new_elem,PT_NT_CHAIN,0);
    PT_WRITE_PNTR((&new_elem->data),(long)father);
    data = (&new_elem->data)+sizeof(PT_PNTR);
    if (apos>PT_SHORT_SIZE){
        PT_WRITE_INT(data,apos);
        data+=4; new_elem->flags|=1;
    }else{
        PT_WRITE_SHORT(data,apos);
        data+=2;
    }
    PT_WRITE_PNTR(data,0);      // first element
    PT_add_to_chain(ptmain, new_elem,name,apos,rpos);
    return new_elem;
}

POS_TREE       *
PT_create_leaf(PTM2 *ptmain, POS_TREE ** pfather, PT_BASES base, int rpos, int apos, int name)  /* stage 1*/
{
    POS_TREE       *father, *node, *new_elemfather;
    int             base2;
    int     leafsize;
    char        *dest;
    leafsize = PT_EMPTY_LEAF_SIZE;
    if (rpos>PT_SHORT_SIZE) leafsize+=2;
    if (apos>PT_SHORT_SIZE) leafsize+=2;
    if (name>PT_SHORT_SIZE) leafsize+=2;
    node = (POS_TREE *) PTM_get_mem(leafsize);
    if (base >= PT_B_MAX)
        *(int *) 0 = 0;
    if (pfather) {
        int             oldfathersize;
        POS_TREE       *gfather, *son;
        long             i,j, sbase, dbase;
        father = *pfather;
        PT_NODE_SIZE(father,oldfathersize);
        new_elemfather = (POS_TREE *)PTM_get_mem(oldfathersize + sizeof(PT_PNTR));
        PT_READ_PNTR(&father->data,j);
        gfather = (POS_TREE *)j;
        if (gfather){
            PT_change_father(gfather, father, new_elemfather);
            PT_WRITE_PNTR(&new_elemfather->data,gfather);
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
    if (name>PT_SHORT_SIZE){
        PT_WRITE_INT(dest,name);
        node->flags |= 1;
        dest += 4;
    }else{
        PT_WRITE_SHORT(dest,name);
        dest += 2;
    }
    if (rpos>PT_SHORT_SIZE){
        PT_WRITE_INT(dest,rpos);
        node->flags |= 2;
        dest += 4;
    }else{
        PT_WRITE_SHORT(dest,rpos);
        dest += 2;
    }
    if (apos>PT_SHORT_SIZE){
        PT_WRITE_INT(dest,apos);
        node->flags |= 4;
        dest += 4;
    }else{
        PT_WRITE_SHORT(dest,apos);
        dest += 2;
    }
    if (base == PT_QU)
        return PT_leaf_to_chain(ptmain, node);
    return node;
}


/******************************** funtions for stage 1: save *********************************/

void PTD_clear_fathers(PTM2 *ptmain, POS_TREE * node)       /* stage 1*/
{
    POS_TREE       *sons;
    int i;
    PT_NODE_TYPE    type = PT_read_type(node);
    if (type == PT_NT_SAVED) return;
    PT_WRITE_PNTR((&node->data), 0);
    if (type == PT_NT_NODE) {
        for (i = PT_QU; i < PT_B_MAX; i++) {
            sons = PT_read_son(ptmain,node, (enum PT_bases_enum)i);
            if (sons)
                PTD_clear_fathers(ptmain,sons);
        }
    }
}

void PTD_put_int(FILE * out, ulong i)
{
    register int io;
    static unsigned char buf[4];
    PT_WRITE_INT(buf,i);
    io = buf[0]; putc(io,out);
    io = buf[1]; putc(io,out);
    io = buf[2]; putc(io,out);
    io = buf[3]; putc(io,out);
}

void PTD_put_short(FILE * out, ulong i)
{
    register int io;
    static unsigned char buf[2];
    PT_WRITE_SHORT(buf,i);
    io = buf[0]; putc(io,out);
    io = buf[1]; putc(io,out);
}

void PTD_set_object_to_saved_status(POS_TREE * node, long pos, int size){
    node->flags = 0x20;
    PT_WRITE_PNTR((&node->data),pos);
    if (size < 20){
        node->flags |= size-sizeof(PT_PNTR);
    }else{
        PT_WRITE_INT((&node->data)+sizeof(PT_PNTR),size);
    }
}

long PTD_write_tip_to_disk(FILE * out, PTM2 */*ptmain*/,POS_TREE * node,long pos)
{
    register int size,i,cnt;
    register char *data;
    putc(node->flags,out);          /* save type */
    size = PT_LEAF_SIZE(node);
    // write 4 bytes when not in stage 2 save mode

    cnt = size-sizeof(PT_PNTR)-1;               /* no father; type already saved */
    for (data = (&node->data)+sizeof(PT_PNTR);cnt;cnt--) { /* write apos rpos name */
        i = (int)(*(data++));
        putc(i,out);
    }
    PTD_set_object_to_saved_status(node,pos,size);
    pos += size-sizeof(PT_PNTR);                /* no father */
    return pos;
}

int ptd_count_chain_entries(char * entry){
    int counter   = 0;
    long next;
    while (entry){
        counter ++;
        PT_READ_PNTR(entry,next);
        entry = (char *)next;
    }
    return counter;
}

void ptd_set_chain_references(char *entry,char **entry_tab){
    int counter   = 0;
    long next;
    while (entry){
        entry_tab[counter] = entry;
        counter ++;
        PT_READ_PNTR(entry,next);
        entry = (char *)next;
    }
}

void ptd_write_chain_entries(FILE * out, long *ppos, PTM2 */*ptmain*/ , char ** entry_tab,  int n_entries, int mainapos){
    static char buffer[100];
    int name;
    int rpos;
    int apos;
    int lastname = 0;
    while (n_entries>0){
        char *entry = entry_tab[n_entries-1];
        n_entries --;
        register char *rp = entry;
        rp+= sizeof(PT_PNTR);

        PT_READ_NAT(rp,name);
        PT_READ_NAT(rp,rpos);
        PT_READ_NAT(rp,apos);
        if (name < lastname) {
            fprintf(stderr, "Chain Error name order error %i < %i\n",name, lastname);
            return;
        }
        char *wp = buffer;
        wp = PT_WRITE_CHAIN_ENTRY(wp,mainapos,name-lastname,apos,rpos);
        int size = wp -buffer;
        if (1 !=fwrite(buffer,size,1,out) ) {
            fprintf(stderr,"Write Error (Disc Full ?\?\?)\n");
            exit(EXIT_FAILURE);
        }
        *ppos += size;
        PTM_free_mem(entry,rp-entry);
        lastname = name;
    }
}


long PTD_write_chain_to_disk(FILE * out, PTM2 *ptmain,POS_TREE * node,long pos) {
    register char *data;
    long oldpos = pos;
    putc(node->flags,out);          /* save type */
    pos++;
    int mainapos;
    data = (&node->data) + ptmain->mode;

    if (node->flags&1){
        PT_READ_INT(data,mainapos);
        PTD_put_int(out,mainapos);
        data += 4;
        pos += 4;
    }else{
        PT_READ_SHORT(data,mainapos);
        PTD_put_short(out,mainapos);
        data += 2;
        pos += 2;
    }
    long first_entry;
    PT_READ_PNTR(data,first_entry);
    int n_entries = ptd_count_chain_entries( (char *)first_entry );
    {
        char **entry_tab = (char **)GB_calloc(sizeof(char *),n_entries);
        ptd_set_chain_references((char *)first_entry, entry_tab);
        ptd_write_chain_entries(out, &pos, ptmain, entry_tab,n_entries,mainapos);
        delete entry_tab;
    }
    putc(PT_CHAIN_END,out);
    pos++;
    PTD_set_object_to_saved_status(node,oldpos,data+sizeof(PT_PNTR)-(char*)node);
    return pos;
}

void PTD_debug_nodes(void)
{
    printf ("Inner Node Statistic:\n");
    printf ("   Single Nodes:   %6i\n",psg.stat.single_node);
    printf ("   Short  Nodes:   %6i\n",psg.stat.short_node);
    printf ("       Chars:      %6i\n",psg.stat.chars);
    printf ("       Shorts:     %6i\n",psg.stat.shorts2);
    printf ("   Long   Nodes:   %6i\n",psg.stat.long_node);
    printf ("       Shorts:     %6i\n",psg.stat.shorts);
    printf ("       Longs:      %6i\n",psg.stat.longs);
}

long PTD_write_node_to_disk(FILE * out, PTM2 *ptmain,POS_TREE * node, long *r_poss,long pos){
    register int i,size;    // Save node after all descendends are already saved
    register POS_TREE *sons;

    ulong   max_diff = 0;
    int lasti = 0;
    int count = 0;
    int mysize;

    size = PT_EMPTY_NODE_SIZE;
    mysize = PT_EMPTY_NODE_SIZE;

    for (i = PT_QU; i < PT_B_MAX; i++) {    /* free all sons */
        sons = PT_read_son(ptmain, node, (enum PT_bases_enum)i);
        if (sons) {
            int memsize;
            ulong   diff = pos - r_poss[i];
            if (diff>max_diff) {
                max_diff = diff;
                lasti = i;
            }
            mysize+= sizeof(PT_PNTR);
            if (PT_GET_TYPE(sons) != PT_NT_SAVED) {
                fprintf(stderr,"Internal Error: Son not saved: There is no help\n");
                GB_CORE;
            }
            if ( (sons->flags & 0xf) == 0) {
                PT_READ_INT((&sons->data)+sizeof(PT_PNTR),memsize);
            }else{
                memsize = (sons->flags &0xf) + sizeof(PT_PNTR);
            }
            PTM_free_mem((char *)sons,memsize);
            count ++;
        }
    }
    if ( (count == 1) && (max_diff<=9) && (max_diff != 2)){ // nodesingle
        if (max_diff>2) max_diff -=2; else max_diff -= 1;
        long flags = 0xc0 | lasti | (max_diff <<3);
        putc((int)flags,out);
        psg.stat.single_node++;
    }else{                          // multinode
        putc(node->flags,out);
        int flags2 = 0;
        int level;
        if (max_diff > 0xffff){
            flags2 |= 0x80;
            level = 0xffff;
            psg.stat.long_node++;
        }else{
            max_diff = 0;
            level = 0xff;
            psg.stat.short_node++;
        }
        for (i = PT_QU; i < PT_B_MAX; i++) {    /* set the flag2 */
            if (r_poss[i]){
                /*u*/ long  diff = pos - r_poss[i];
                if (diff>level) flags2 |= 1<<i;
            }
        }
        putc(flags2,out);
        size++;
        for (i = PT_QU; i < PT_B_MAX; i++) {    /* write the data */
            if (r_poss[i]){
                /*u*/ long  diff = pos - r_poss[i];
                if (max_diff) {
                    if (diff>level) {
                        PTD_put_int(out,diff);
                        size += 4;
                        psg.stat.longs++;
                    }else{
                        PTD_put_short(out,diff);
                        size += 2;
                        psg.stat.shorts++;
                    }
                }else{
                    if (diff>level) {
                        PTD_put_short(out,diff);
                        size += 2;
                        psg.stat.shorts2++;
                    }else{
                        putc((int)diff,out);
                        size += 1;
                        psg.stat.chars++;
                    }
                }
            }
        }
    }

    PTD_set_object_to_saved_status(node,pos,mysize);
    pos += size-sizeof(PT_PNTR);                /* no father */
    return pos;
}
int
PTD_write_leafs_to_disk(FILE * out, PTM2 *ptmain, POS_TREE * node, long pos, long *pnodepos, int *pblock)
{
    // returns new pos when son is written 0 otherwise
    // pnodepos is set to last object

    POS_TREE       *sons;
    long             r_pos,r_poss[PT_B_MAX],son_size[PT_B_MAX],o_pos;
    int     block[10];          /* dummy [10] to force cc not to use register */
    int i;
    PT_NODE_TYPE    type = PT_read_type(node);


    if (type == PT_NT_SAVED) {          // already saved
        long father;
        PT_READ_PNTR((&node->data),father);
        *pnodepos = father;
        return 0;
    }else   if (type == PT_NT_LEAF) {
        *pnodepos = pos;
        pos = (long)PTD_write_tip_to_disk(out,ptmain,node,pos);
    }else if (type == PT_NT_CHAIN) {
        *pnodepos = pos;
        pos = (long)PTD_write_chain_to_disk(out,ptmain,node,pos);
    }else if (type == PT_NT_NODE) {
        block[0] = 0;
        o_pos = pos;
        for (i = PT_QU; i < PT_B_MAX; i++) {    /* save all sons */
            sons = PT_read_son(ptmain, node, (enum PT_bases_enum)i);
            r_poss[i] = 0;
            if (sons) {
                r_pos = PTD_write_leafs_to_disk(out, ptmain, sons,pos,
                                                &(r_poss[i]),&( block[0]));
                if (r_pos>pos){         // really saved ????
                    son_size[i] = r_pos-pos;
                    pos = r_pos;
                }else{
                    son_size[i] = 0;
                }
            }else{
                son_size[i] = 0;
            }
            {
                // ?????????????????????
                //  if (r_poss[0] && r_poss[0] == r_poss[1]){ GB_warning("Scheisse");GB_CORE;}
            }
        }
        if (block[0] ) {    /* son wrote a block */
            *pblock = 1;
        }else if (pos-o_pos > PT_BLOCK_SIZE) {
            /* a block is written */
            *pblock = 1;
        }else{          /* now i can write my data */
            *pnodepos = pos;
            pos = PTD_write_node_to_disk(out,ptmain,node,r_poss,pos);
        }
    }
    return pos;
}

/******************************** funtions for stage 2-3: load *********************************/

void PTD_read_leafs_from_disk(char *fname,PTM2 *ptmain, POS_TREE **pnode)
{
    char *buffer;
    int i;
    char    *main;
    buffer = GB_map_file(fname,0);
    if (!buffer){
        GB_print_error();
        fprintf(stderr,"PT_SERVER: Error Out of Memory: mmap failes\n");
        exit(EXIT_FAILURE);
    }

    long size = GB_size_of_file(fname);

    main = &(buffer[size-4]);
    PT_READ_INT(main, i);
    *pnode = (POS_TREE *)(i+buffer);
    ptmain->mode = 0;
    ptmain->base = buffer;
}
