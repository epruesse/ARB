
#define PTM_magic             0xf4
#define PTM_TABLE_SIZE        (1024*256)
#define PTM_ALIGNED           1
#define PTM_LD_ALIGNED        0
// #define PTM_MAX_TABLES        64
#define PTM_MAX_TABLES        256 // -- ralf testing
#define PTM_MAX_SIZE          (PTM_MAX_TABLES*PTM_ALIGNED)
#define PT_CHAIN_END          0xff
#define PT_CHAIN_NTERM        250
#define PT_SHORT_SIZE         0xffff
#define PT_BLOCK_SIZE         0x800
#define PT_INIT_CHAIN_SIZE    20
// #define PT_NEXT_CHAIN_SIZE(x) = (x*3/2);

typedef void * PT_PNTR;

extern struct PTM_struct {
    char         *data;
    int           size;
    int           allsize;
    char         *tables[PTM_MAX_TABLES+1];
#ifdef PTM_DEBUG
    long          debug[PTM_MAX_TABLES+1];
#endif
    PT_NODE_TYPE  flag_2_type[256];
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
    int     rel start of the real object
    [int        size]       if bit[0-3] == 0;

/ ********************* tip (7-13   +4) *********************** /
    byte    <32         bit[7] = bit[6] = bit[5] = 0
                    bit[3-4] free
    [PT_PNTR    father]     if main->mode
    short/int   name        int if bit[0]
    short/int   rel pos     int if bit[1]
    short/int   abs pos     int if bit[2]

/ ********************* inner node (1 + 4*[1-6] +4) (stage 1 + 2) *********************** /
    byte    >128            bit[7] =  1
                    bit[6] = 0
    [PT_PNTR father]        if main->mode
    [PT_PNTR son0]          if bit[0]
...
    [PT_PNTR son5]          if bit[5]

/ ********************* inner node (3-22    +4) (stage 3 only) *********************** /
    byte                bit[7] = 1 bit[6] = 0
    byte2               if bit2[7]  then int/short else short/char
    [char/short/int son0]       if bit[0] short/int if bit2[0] else char/short
    [char/short/int son1]       if bit[1]
    [char/short/int son5]       if bit[5]

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
                if bit[15] than integer
                -1 not allowed
        short/int   rel pos
                if bit[15] than integer
        short/int   apos short if bit[15] = 0]
]


/ ********************* chain (8-n +4) stage 2/3 *********************** /

    byte =64/65         bit[7] = 0, bit[6] = 1  bit[5] = 0
    [int        father]     if main->mode
    short/int   ref abs pos int if bit[0]
[   char/short/int      rel name [ to last name eg. rel names 10 30 20 50 -> abs names = 10 40 60 110
                if bit[7] than short
                if bit[7] and bit[6] than integer
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
    PT_read_chain
*/
#endif

#define IS_SINGLE_BRANCH_NODE 0x40
#define LONG_SONS             0x80

/********************* Get the size of entries (stage 1) only***********************/

#define PT_EMPTY_LEAF_SIZE       (1+sizeof(PT_PNTR)+6) /* tag father name rel apos */
#define PT_LEAF_SIZE(leaf)       (1+sizeof(PT_PNTR)+6+2*PT_count_bits[3][leaf->flags])
#define PT_EMPTY_CHAIN_SIZE      (1+sizeof(PT_PNTR)+2+sizeof(PT_PNTR)) // tag father apos first_elem
#define PT_EMPTY_NODE_SIZE       (1+sizeof(PT_PNTR)) // tag father
#define PT_NODE_COUNT_SONS(leaf) PT_count_bits[3][leaf->flags];
#define PT_NODE_SIZE(node,size)  size = PT_EMPTY_NODE_SIZE + sizeof(PT_PNTR)*PT_count_bits[PT_B_MAX][node->flags]

/********************* Read and write type ***********************/

#define PT_GET_TYPE(pt)     (PTM.flag_2_type[pt->flags])
#define PT_SET_TYPE(pt,i,j) (pt->flags = (i<<6)+j)

/********************* Read and write to memory ***********************/

#define PT_READ_INT(ptr,my_int_i) do {                                      \
    unsigned char *mycharp=(unsigned char *)ptr;                            \
    my_int_i= mycharp[0]<<24 | mycharp[1]<<16 | mycharp[2]<<8 | mycharp[3]; \
} while(0)

#define PT_WRITE_INT(ptr,my_int_i) do {                 \
    unsigned char *mycharp=(unsigned char *)(ptr);      \
    unsigned long myinti = (unsigned long)(my_int_i);   \
    mycharp[0]= (unsigned char)(myinti>>24);            \
    mycharp[1]= (unsigned char)(myinti>>16);            \
    mycharp[2]= (unsigned char)(myinti>>8);             \
    mycharp[3]= (unsigned char)myinti;                  \
} while(0)

#define PT_READ_SHORT(ptr,my_int_i)                 \
do {                                                \
    unsigned char *mycharp=(unsigned char *)ptr;    \
    my_int_i= mycharp[0]<<8 | mycharp[1];           \
} while(0)

#define PT_WRITE_SHORT(ptr,my_int_i) do {           \
    unsigned char *mycharp=(unsigned char *)ptr;    \
    unsigned long myinti = (unsigned long)my_int_i; \
    mycharp[0]= (unsigned char)(myinti>>8);         \
    mycharp[1]= (unsigned char)(myinti);            \
} while(0)

#define PT_WRITE_CHAR(ptr,my_int_i) do { *(unsigned char *)(ptr) = my_int_i; } while(0)

#define PT_READ_CHAR(ptr,my_int_i)  do { my_int_i = *(unsigned char *)(ptr); } while(0)



#ifdef ARB_64

# define PT_READ_PNTR(ptr,my_int_i)                                                 \
do {                                                                                \
    if (sizeof(my_int_i)==4) GB_CORE;                                               \
    unsigned char *mycharp=(unsigned char *)ptr;                                    \
    my_int_i= (long)mycharp[0]<<56 | (long)mycharp[1]<<48 | (long)mycharp[2]<<40 | (long)mycharp[3] << 32 | \
        (long)mycharp[4]<<24 | (long)mycharp[5]<<16 | (long)mycharp[6]<<8 | (long)mycharp[7];               \
} while(0)

# define PT_WRITE_PNTR(ptr,my_int_i)                                                                \
do {                                                                                                \
    unsigned char *mycharp=(unsigned char *)(ptr);unsigned long myinti = (unsigned long)(my_int_i); \
    mycharp[0]= (unsigned char)(myinti>>56);                                                        \
    mycharp[1]= (unsigned char)(myinti>>48);                                                        \
    mycharp[2]= (unsigned char)(myinti>>40);                                                        \
    mycharp[3]= (unsigned char)(myinti>>32);                                                        \
    mycharp[4]= (unsigned char)(myinti>>24);                                                        \
    mycharp[5]= (unsigned char)(myinti>>16);                                                        \
    mycharp[6]= (unsigned char)(myinti>>8);                                                         \
    mycharp[7]= (unsigned char)myinti;                                                              \
} while(0)


#else
/* not ARB_64 */

# define PT_READ_PNTR(ptr,my_int_i)  PT_READ_INT(ptr, my_int_i)
# define PT_WRITE_PNTR(ptr,my_int_i) PT_WRITE_INT(ptr, my_int_i)

#endif



#define PT_WRITE_NAT(ptr,i) do {                \
    if (i<0) GB_CORE;                           \
    if (i>= 0x7FFE)                             \
    {                                           \
        PT_WRITE_INT(ptr,i|0x80000000);         \
        ptr += sizeof(int);                     \
    }                                           \
    else                                        \
    {                                           \
        PT_WRITE_SHORT(ptr,i);                  \
        ptr += sizeof(short);                   \
    }                                           \
} while (0)

#define PT_READ_NAT(ptr,i)                                      \
do {                                                            \
    if (*ptr & 0x80) {                                          \
        PT_READ_INT(ptr,i); ptr+= sizeof(int); i &= 0x7fffffff; \
    }else{                                                      \
        PT_READ_SHORT(ptr,i); ptr+= sizeof(short);              \
    }                                                           \
} while(0)


/********************* PT_READ_CHAIN_ENTRY ***********************/


GB_INLINE const char *PT_READ_CHAIN_ENTRY(const char* ptr,int mainapos,int *name,int *apos,int *rpos) {
    unsigned int rcei;
    unsigned char *rcep = (unsigned char*)ptr;
    int isapos;

    *apos = 0;
    *rpos = 0;
    rcei = (*rcep);

    if (rcei==PT_CHAIN_END) {
        *name = -1;
        ptr++;
    }
    else {
        if (rcei&0x80) {
            if (rcei&0x40) {
                PT_READ_INT(rcep,rcei); rcep+=4; rcei &= 0x3fffffff;
            }
            else {
                PT_READ_SHORT(rcep,rcei); rcep+=2; rcei &= 0x3fff;
            }
        }
        else {
            rcei&= 0x7f;rcep++;
        }
        *name += rcei;
        rcei = (*rcep);
        if (rcei&0x80) isapos = 1;
        else isapos = 0;
        if (rcei&0x40) {
            PT_READ_INT(rcep,rcei); rcep+=4; rcei &= 0x3fffffff;
        }else{
            PT_READ_SHORT(rcep,rcei); rcep+=2; rcei &= 0x3fff;
        }
        *rpos = (int)rcei;
        if (isapos) {
            rcei = (*rcep);
            if (rcei&0x80) {
                PT_READ_INT(rcep,rcei); rcep+=4; rcei &= 0x7fffffff;
            }else{
                PT_READ_SHORT(rcep,rcei); rcep+=2; rcei &= 0x7fff;
            }
            *apos = (int)rcei;
        }else{
            *apos = (int)mainapos;}
        ptr = (char *)rcep;
    }

    return ptr;
}


/* stage 1 */
GB_INLINE char *PT_WRITE_CHAIN_ENTRY(const char * const ptr,const int mainapos,int name,const int apos,const int rpos)
{
    register unsigned char *wcep = (unsigned char *)ptr;
    int  isapos;
    if (name < 0x7f) {      /* write the name */
        *(wcep++) = name;
    } else if (name <0x3fff) {
        name |= 0x8000;
        PT_WRITE_SHORT(wcep,name);
        wcep += 2;
    } else {
        name |= 0xc0000000;
        PT_WRITE_INT(wcep,name);
        wcep += 4;
    }

    if (apos == mainapos) isapos = 0; else isapos = 0x80;

    /* tell about old decompression error */

    /* #if defined(DEBUG)
    if (rpos < 0x7fff && rpos > 0x3fff) {
    printf("REMARK: Was wrong data (name = %d, apos = %d, rpos = %d)\n", name, apos, rpos);
    }
    #endif */

    if (rpos < 0x3fff) {        /* write the rpos */
           /*0x7fff, mit der rpos vorher verglichen wurde war zu groß*/
        PT_WRITE_SHORT(wcep,rpos);
        *wcep |= isapos;
        wcep += 2;
    } else {
        PT_WRITE_INT(wcep,rpos);
        *wcep |= 0x40+isapos;
        wcep += 4;
    }
    if (isapos){            /* write the apos */
        if (apos < 0x7fff) {
            PT_WRITE_SHORT(wcep,apos);
            wcep += 2;
        } else {
            PT_WRITE_INT(wcep,apos);
            *wcep |= 0x80;
            wcep += 4;
        }
    }
    return (char *)wcep;
}
// calculate the index of the pointer in a node

GB_INLINE POS_TREE *PT_read_son(PTM2 *ptmain, POS_TREE *node, PT_BASES base)
{
    register long i;
    register uint sec;
    register uint offset;
    if (ptmain->stage3) {       // stage 3  no father
        if (node->flags & IS_SINGLE_BRANCH_NODE){
            if (base != (node->flags & 0x7)) return 0;  // no son
            i = (node->flags >> 3)&0x7;         // this son
            if (!i) i = 1; else i+=2;           // offset mapping
            return (POS_TREE *)(((char *)node)-i);
        }
        if (!( (1<<base) & node->flags)) return 0;  /* bit not set */
        sec = (uchar)node->data;    // read second byte for charshort/shortlong info
        i = PT_count_bits[base][node->flags];
        i+= PT_count_bits[base][sec];
        if (sec & LONG_SONS) {
            offset = i+i;
            if ( (1<<base) & sec) {
                PT_READ_INT((&node->data+1)+offset,i);
            }else{
                PT_READ_SHORT((&node->data+1)+offset,i);
            }
        }else{
            offset = i;
            if ( (1<<base) & sec) {
                PT_READ_SHORT((&node->data+1)+offset,i);
            }else{
                PT_READ_CHAR((&node->data+1)+offset,i);
            }
        }
        return (POS_TREE *)(((ulong)node)-i);

    }else{          // stage 1 or 2 ->father
        if (!( (1<<base) & node->flags)) return 0;  /* bit not set */
        base = (PT_BASES)PT_count_bits[base][node->flags];
        PT_READ_PNTR((&node->data)+sizeof(PT_PNTR)*base+ptmain->mode,i);
        return (POS_TREE *)(i+ptmain->base);
    }
}

GB_INLINE POS_TREE *PT_read_son_stage_1(PTM2 *ptmain, POS_TREE *node, PT_BASES base)
{
    register long i;
    if (!( (1<<base) & node->flags)) return 0;  /* bit not set */
    base = (PT_BASES)PT_count_bits[base][node->flags];
    PT_READ_PNTR((&node->data)+sizeof(PT_PNTR)*base+ptmain->mode,i);
    return (POS_TREE *)(i+ptmain->base);
}

GB_INLINE PT_NODE_TYPE PT_read_type(POS_TREE *node)
{
    return (PT_NODE_TYPE)PT_GET_TYPE(node);
}

GB_INLINE int PT_read_name(PTM2 *ptmain,POS_TREE *node)
{
    int i;
    if (node->flags&1){
        PT_READ_INT((&node->data)+ptmain->mode,i);
    }else{
        PT_READ_SHORT((&node->data)+ptmain->mode,i);
    }
    return i;
}

GB_INLINE int PT_read_rpos(PTM2 *ptmain,POS_TREE *node)
{
    int i;
    char *data = (&node->data)+2+ptmain->mode;
    if (node->flags&1) data+=2;
    if (node->flags&2){
        PT_READ_INT(data,i);
    }else{
        PT_READ_SHORT(data,i);
    }
    return i;
}

GB_INLINE int PT_read_apos(PTM2 *ptmain,POS_TREE *node)
{
    int i;
    char *data = (&node->data)+ptmain->mode+4;  /* father 4 name 2 rpos 2 */
    if (node->flags&1) data+=2;
    if (node->flags&2) data+=2;
    if (node->flags&4){
        PT_READ_INT(data,i);
    }else{
        PT_READ_SHORT(data,i);
    }
    return i;
}

GB_INLINE int PT_read_chain(PTM2 *ptmain,POS_TREE *node, int func(int name,int apos,int rpos, long clientdata), long clientdata)
{
    int pos, apos, rpos;
    int cname;
    int error;
    const char *data;

    data = (&node->data) + ptmain->mode;
    if (node->flags&1){
        PT_READ_INT(data,pos);
        data += 4;
    }else{
        PT_READ_SHORT(data,pos);
        data += 2;
    }
    error = 0;
    cname = 0;
    while (cname>=0){
        data = PT_READ_CHAIN_ENTRY(data,pos,&cname,&apos,&rpos);
        if (cname>=0){
            error = func(cname,apos,rpos,clientdata);
            if (error) return error;
        }
    }
    return error;
}

