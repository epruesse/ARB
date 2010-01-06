// =============================================================== //
//                                                                 //
//   File      : adcompr.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <climits>

#include <arbdbt.h>

#include "gb_t_prot.h"
#include "gb_compress.h"
#include "gb_localdata.h"


#if defined(DEBUG)
/* #define TEST_HUFFMAN_CODE */
#endif /* DEBUG */

#define GBTUM_COMPRESS_TREE_SIZE 32

/********************************************************************************************
                                        GB uncompress procedures
********************************************************************************************/
#define GB_READ_BIT(p,c,bp,result)       if (!bp) { c= *(p++);bp = 8;}; result = (c>>7); result &= 1; c <<=1; bp--

#define GB_INIT_WRITE_BITS(p,bp) *p = 0; bp = 8

#define GB_WRITE_BITS(p,bp,bitc,bits,i)         \
    if (bp<=0) { bp += 8; p++ ; *p = 0; }       \
    if ((i=bp-bitc)<0) {                        \
        *p |=  bits>>(-i);                      \
        i += 8;                                 \
        bp += 8; p++; *p = 0;                   \
    }                                           \
    *p |=  bits<<i;                             \
    bp-=bitc;

#define GB_READ_BITS(p,c,bp,bitc,bits)  {       \
        long _i;                                \
        int _r;                                 \
        bits = 0;                               \
        for (_i=bitc-1;_i>=0;_i--) {            \
            GB_READ_BIT(p,c,bp,_r);             \
            bits = (bits<<1) + _r;              \
        }}


ATTRIBUTED(__ATTR__USERESULT, static GB_ERROR gb_check_huffmann_tree(struct gb_compress_tree *t)) {
    if (t->leave)
        return 0;
    if (!t->son[0])
        return GB_export_error("Database entry corrupt (zero left son)");
    if (!t->son[1])
        return GB_export_error("Database entry corrupt (zero right son)");

    {
        GB_ERROR err = gb_check_huffmann_tree(t->son[0]);
        if (err) return err;
    }
    return gb_check_huffmann_tree(t->son[1]);
}

#if defined(TEST_HUFFMAN_CODE)
static void gb_dump_huffmann_tree(struct gb_compress_tree *t, const char *prefix) {
    if (t->leave) {
        long command = (long)t->son[1];
        printf("%s", prefix);

        switch (command) {
            case gb_cs_end: printf(" [gb_cs_end]\n"); break;
            case gb_cs_id: printf(" [gb_cs_id]\n"); break;
            case gb_cs_ok:  {
                long val = (long)t->son[0];
                printf(": value=%li (='%c')\n", val, (char)val);
                break;
            }
            default:  {
                long val = (long)t->son[0];
                printf(" other command (%li) value=%li (='%c')\n", command, val, (char)val);
                break;
            }
        }



        /*         printf("%s %lx %lx\n", prefix, (long)(t->son[0]), (long)(t->son[1])); */
    }
    else {
        int   len        = strlen(prefix);
        char *my_prefix  = malloc(len+2);
        strcpy(my_prefix, prefix);
        my_prefix[len+1] = 0;
        my_prefix[len]   = '0';
        gb_dump_huffmann_tree(t->son[0], my_prefix);
        my_prefix[len]   = '1';
        gb_dump_huffmann_tree(t->son[1], my_prefix);
        free(my_prefix);
    }
}
static void gb_dump_huffmann_list(struct gb_compress_list *bc, const char *prefix) {
    if (bc->command == gb_cd_node) {
        int   len        = strlen(prefix);
        char *my_prefix  = malloc(len+2);
        strcpy(my_prefix, prefix);
        my_prefix[len+1] = 0;
        my_prefix[len]   = '0';
        gb_dump_huffmann_list(bc->son[0], my_prefix);
        my_prefix[len]   = '1';
        gb_dump_huffmann_list(bc->son[1], my_prefix);
        free(my_prefix);        
    }
    else {
        /*         printf("%s  value=%i (='%c') bitcnt=%i bits=%x mask=%x count=%li\n", */
        /*                prefix, bc->value, (char)bc->value, bc->bitcnt, bc->bits, bc->mask, bc->count); */
        printf("%s  value=%i (='%c') count=%li\n",
               prefix, bc->value, (char)bc->value, bc->count);
    }
}

#endif /* TEST_HUFFMAN_CODE */

struct gb_compress_tree *gb_build_uncompress_tree(const unsigned char *data,long short_flag, char **end)
{
    struct gb_compress_tree *Main,*t;
    long                     bits,mask,i;
    const unsigned  char    *p;
    GB_ERROR                 error;
    
    Main = (struct gb_compress_tree *)gbm_get_mem(sizeof(struct gb_compress_tree),GBM_CB_INDEX);
    for (p=data;*p;p+=3+short_flag) {
        bits = p[0];
        mask = 0x80;
        for (i=7;i;i--,mask=mask>>1)  if (mask & bits) break;   /* find first bit */
        if (!i){
            GB_internal_error("Data corrupt");
            return 0;
        }
        t = Main;
        for (;i;i--) {
            if (t->leave) {
                GB_export_error("Corrupt data !!!");
                return 0;
            }
            mask = mask>>1;
            if (mask & bits) {
                if (!t->son[1]) {
                    t->son[1] = (struct gb_compress_tree *)gbm_get_mem(sizeof(struct gb_compress_tree),GBM_CB_INDEX);
                }
                t=t->son[1];
            }else{
                if (!t->son[0]) {
                    t->son[0] = (struct gb_compress_tree *)gbm_get_mem(sizeof(struct gb_compress_tree),GBM_CB_INDEX);
                }
                t=t->son[0];
            }
        }
        if (t->leave) {
            GB_export_error("Corrupt data !!!");
            return 0;
        }
        t->leave = 1;
        if (short_flag) {
            t->son[0] = (struct gb_compress_tree *)(long)((p[2]<<8)+p[3]);
        }else{
            t->son[0] = (struct gb_compress_tree *)(long)(p[2]);
        }
        t->son[1] = (struct gb_compress_tree *)(long)p[1]; /* command */
    }
    if (end) *end = ((char *)p)+1;
    if ( (error = gb_check_huffmann_tree(Main)) ) {
        GB_internal_errorf("%s",error);
        gb_free_compress_tree(Main);
        return 0;
    }

#if defined(TEST_HUFFMAN_CODE)
    printf("Huffman tree:\n");
    gb_dump_huffmann_tree(Main, "");
#endif /* TEST_HUFFMAN_CODE */

    return Main;
}

void gb_free_compress_tree(struct gb_compress_tree *tree) {
    if (tree) {
        if (!tree->leave) {
            if (tree->son[0]) gb_free_compress_tree(tree->son[0]);
            if (tree->son[1])gb_free_compress_tree(tree->son[1]);
        }
    }
    gbm_free_mem((char *)tree,sizeof(struct gb_compress_tree),GBM_CB_INDEX);
}

/********************************************************************************************
                                        GB compress procedures
********************************************************************************************/
struct gb_compress_list *gb_build_compress_list(const unsigned char *data,long short_flag,long *size)
{
    struct gb_compress_list *list;
    int                      i,maxi,bitc;
    int                      val,bits,mask,value;
    const unsigned  char    *p;
    
    enum gb_compress_list_commands command = (enum gb_compress_list_commands)0;

    maxi = 0;
    val  = bits = mask = value = bitc = 0;
    for (p=data;*p;p+=3+short_flag) {
        i = (p[2]);
        if (short_flag) {
            i = (i<<8)+p[3];
        }
        if (i>maxi) maxi = i;
    }
    *size = maxi;

    list = (struct gb_compress_list *)GB_calloc(sizeof(struct gb_compress_list),(size_t)maxi+1);

    maxi = 0;
    val = -1;

    for (p=data;*p;p+=3+short_flag) {
        val = (p[2]);
        if (short_flag) val = (val<<8)+p[3];

        for (i=maxi;i<val;i++) {
            list[i].command = command;
            list[i].mask    = mask;
            list[i].bitcnt  = bitc;
            list[i].bits    = bits;
            list[i].value   = value;
        }
        maxi = val;

        command = (enum gb_compress_list_commands)(p[1]);
        bits = p[0];
        mask = 0x80;
        for (bitc=7;bitc;bitc--,mask=mask>>1)  if (mask & bits) break;  /* find first bit */
        mask = 0xff>>(8-bitc);
        bits = bits & mask;
        value = val;
    }
    i = val;
    list[i].command = command;
    list[i].mask    = mask;
    list[i].bitcnt  = bitc;
    list[i].bits    = bits;
    list[i].value   = value;
    return list;
}


/********************************************************************************************
                                        Compress and uncompress bits
********************************************************************************************/

char *gb_compress_bits(const char *source, long size, const unsigned char *c_0, long *msize)
{
    const unsigned char *s      = (const unsigned char *)source;
    char                *buffer = GB_give_other_buffer(source,size);
    char                *dest   = buffer;

    long  i, j;
    int   bitptr,bits, bitc;
    int   zo_flag = 0;
    long  len;
    int   h_i,command;
    int   isNull[256];

    for (i = 0; i<256; ++i) isNull[i] = 0;
    for (i = 0; c_0[i]; ++i) isNull[c_0[i]] = 1;

    GB_INIT_WRITE_BITS(dest,bitptr);
    for (len = size, i = 0; len; len--) {
        if (zo_flag == isNull[*(s++)]) {
            zo_flag = 1 - zo_flag;
            for (command = gb_cs_sub; command != gb_cs_ok;) {
                if (i>gb_local->bc_size) j = gb_local->bc_size; else j = i;
                bits = gb_local->bitcompress[j].bits;
                bitc = gb_local->bitcompress[j].bitcnt;
                command = gb_local->bitcompress[j].command;
                i -= gb_local->bitcompress[j].value;
                GB_WRITE_BITS(dest, bitptr, bitc, bits, h_i);
            }
            i = 0;
        }
        i++;
    }
    for (command = gb_cs_sub; command != gb_cs_ok;) {
        if (i>gb_local->bc_size) j = gb_local->bc_size; else j = i;
        bits = gb_local->bitcompress[j].bits;
        bitc = gb_local->bitcompress[j].bitcnt;
        command = gb_local->bitcompress[j].command;
        i -= gb_local->bitcompress[j].value;
        GB_WRITE_BITS(dest, bitptr, bitc, bits, h_i);
    }
    *msize = dest - buffer + 1;
    return buffer;
}


GB_BUFFER gb_uncompress_bits(const char *source,long size, char c_0, char c_1)
{
    const char              *s;
    char                    *p,*buffer,ch = 0,outc;
    long                     bitp,lastpos,pos;
    struct gb_compress_tree *Main,*t;
    long                     command;

    Main = gb_local->bituncompress;
    bitp = 0;
    s = source;
    buffer = p = GB_give_other_buffer(source,size+1);
    outc = c_0;

    for (pos = 0;pos<size;) {
        lastpos = pos;
        for (command = gb_cs_sub; command != gb_cs_ok; ) {
            for (t=Main;!t->leave;) {
                int bit;
                GB_READ_BIT(s,ch,bitp,bit);
                t = t->son[bit];
            }
            command = (long) t->son[1];
            pos += (long)t->son[0];
        }
        for (;lastpos<pos;lastpos++) *(p++) = outc;
        if (outc==c_0) outc=c_1; else outc=c_0;
    }
    *p = 0;
    return buffer;
}

/********************************************************************************************
                                        Compress  bytes
********************************************************************************************/
/** runlength encoding @@@
    source: string
    dest:   command data command data
    0< command < 120 ->     command == ndata follows
    -120 < command < 0 ->   next byte is duplicated - command times
    -122 lowbyte highbyte   next byte is duplicated lowbyte + 256*highbyte
    0                       end
*/

char *g_b_write_run(char *dest, int scount, int lastbyte){
    while (scount> 0xffff) {
        *(dest++) = 0x100-122 ;
        *(dest++) = 0xff;
        *(dest++) = 0xff;
        *(dest++) = lastbyte;
        scount -= 0xffff;
    }
    if (scount >250){
        *(dest++) = 0x100-122;
        *(dest++) = scount & 0xff;
        *(dest++) = (scount >>8) & 0xff;
        *(dest++) = lastbyte;
        return dest;
    }

    while (scount>120){         /* 120 blocks */
        *(dest++) = 0x100 - 120;
        *(dest++) = lastbyte;
        scount -= 120;
    }

    if (scount) {                       /* and rest */
        *(dest++) = -scount;
        *(dest++) = lastbyte;
    }
    return dest;
}

#define GB_COPY_NONRUN(dest,source,len)         \
    while (len > 120) {                         \
        int _i = 120;                           \
        char *_p;                               \
        len -= _i;                              \
        *(dest++) = _i;                         \
        _p = dest; dest+=_i;                    \
        while (_i--){                           \
            *(_p++) = *(source++);              \
        }                                       \
    }                                           \
    if  (len >0 ) {                             \
        int _i = len;                           \
        char *_p;                               \
        len = 0;                                \
        *(dest++) = _i;                         \
        _p = dest; dest+=_i;                    \
        while (_i--){                           \
            *(_p++) = *(source++);              \
        }                                       \
    }

void gb_compress_equal_bytes_2(const char *source, long size, long *msize, char *dest){
    long        i;              /* length counter */
    int         last,rbyte;     /* next and akt value */
    long        scount;         /* same count; count equal bytes */
    char       *buffer = dest;
    const char *sourcenequal;   /* begin of non equal section */
    long        hi;             /* to temporary store i */
    int         hsize;

    sourcenequal = source;
    rbyte = *(source ++);
    last = -1;
    for (i=size-1; i;) {
        if (rbyte!=last){               /* Search an equal pair */
            last = rbyte;
            rbyte = *(source ++);i--;
            continue;
        }                               /* last rbyte *(source) */
        hi = i+1;
        while (i) {     /* get equal bytes */
            rbyte = *(source ++);i--;
            if (rbyte!=last) break;
        }
        scount = hi-i;                  /* number of equal bytes ; at end e.b.-1*/
        if (scount <= GB_RUNLENGTH_SIZE ) continue;     /* less than 4-> no runlength compression */

        /* compression !!!
           1. copy unequal bytes
           2 fill rest */

        hsize = source - sourcenequal - scount -1;/* hsize of non equal bytes */
        source = sourcenequal;
        hi = i;                         /* i=unequal length */
        GB_COPY_NONRUN(dest,source,hsize);
        dest = g_b_write_run(dest,scount,last);
        source += scount;               /* now equal bytes */

        sourcenequal = source++;        /* no rbyte written yet */
        last = rbyte;
        i = hi;
        if (i){
            rbyte = *(source++);i--;
        }
    }       /* for i */
    /* and now rest which is not compressed */
    hsize = source - sourcenequal;
    source = sourcenequal;
    GB_COPY_NONRUN(dest,source,hsize);

    *(dest++) = 0;                  /* end of data */
    *msize = dest-buffer;
    if (*msize >size*9/8) printf("ssize %d, dsize %d\n",(int)size,(int)*msize);
}

GB_BUFFER gb_compress_equal_bytes(const char *source, long size, long *msize, int last_flag){
    char *dest;                     /* destination pointer */
    char *buffer;

    dest  = buffer = GB_give_other_buffer(source,size*9/8);
    *(dest++) = GB_COMPRESSION_RUNLENGTH | last_flag;
    gb_compress_equal_bytes_2(source,size,msize,dest);
    (*msize) ++;            /* Tag byte */
    return buffer;
}

struct gb_compress_huffmann_struct {
    struct gb_compress_huffmann_struct      *next;
    long                                    val;
    struct gb_compress_list                 *element;
} *gb_compress_huffmann_list = 0;

void gb_compress_huffmann_add_to_list(long val, struct gb_compress_list *element)
{
    struct gb_compress_huffmann_struct *dat,*search,*searchlast;

    dat          = (struct gb_compress_huffmann_struct *)gbm_get_mem(sizeof(struct gb_compress_huffmann_struct),GBM_CB_INDEX);
    dat->val     = val;
    dat->element = element;

    searchlast = 0;
    for (search = gb_compress_huffmann_list; search; search = search->next){
        if (val<search->val) break;
        searchlast = search;
    }
    if (gb_compress_huffmann_list){
        if (searchlast) {
            dat->next = searchlast->next;
            searchlast->next = dat;
        }else{
            dat->next = gb_compress_huffmann_list;
            gb_compress_huffmann_list = dat;
        }
    }else{
        gb_compress_huffmann_list = dat;
    }
}

long gb_compress_huffmann_pop(long *val,struct gb_compress_list **element)
{
    struct gb_compress_huffmann_struct *dat;
    if ( (dat = gb_compress_huffmann_list) ) {
        gb_compress_huffmann_list = dat->next;
        *val = dat->val;
        *element = dat->element;
        gbm_free_mem((char *)dat,sizeof(struct gb_compress_huffmann_struct),GBM_CB_INDEX);
        return 1;
    }else{
        GB_internal_error("huffman compression failed");
        return 0;
    }
}

char *gb_compress_huffmann_rek(struct gb_compress_list *bc,int bits,int bitcnt,char *dest)
{
    if(bc->command == gb_cd_node) {
        dest = gb_compress_huffmann_rek(bc->son[0],(bits<<1),bitcnt+1,dest);
        dest = gb_compress_huffmann_rek(bc->son[1],(bits<<1)+1,bitcnt+1,dest);
        gbm_free_mem((char *)bc,sizeof(struct gb_compress_list),GBM_CB_INDEX);
        return dest;
    }else{
        *(dest++) = bits;
        *(dest++) = bc->command;
        *(dest++) = bc->value;
        bc->bitcnt = bitcnt;
        bc->mask = 0xff>>(8-bitcnt);
        bc->bits = bits&bc->mask;
        return dest;
    }
}

GB_BUFFER gb_compress_huffmann(GB_CBUFFER source, long size, long *msize, int last_flag)
{
    char          *buffer;
    unsigned char *s;
    char          *dest;
    int            val,h_i, command;
    long           id = 0,end, len;

    struct gb_compress_list  bitcompress[257],*pbc;
    struct gb_compress_list *pbid;

    memset((char *)(&bitcompress[0]), 0, sizeof(struct gb_compress_list)*257);
    end = 256;

    dest = buffer = GB_give_other_buffer(source,size*2+3*GBTUM_COMPRESS_TREE_SIZE+1);
    *(dest++) = GB_COMPRESSION_HUFFMANN | last_flag;

    {
        long level;
        long i;
        long restcount;

        long vali[2] = {0, 0};
        
        struct gb_compress_list *element1 = 0;
        struct gb_compress_list *element2 = 0;
        struct gb_compress_list *bc       = 0;

        s = (unsigned char *)source;
        for (len = size; len; len--) {
            bitcompress[*(s++)].count++;
        }
        level = size/GBTUM_COMPRESS_TREE_SIZE;
        restcount = 0;
        for (i=0;i<256;i++) {
            bitcompress[i].value = (int)i;
            if (bitcompress[i].count>level) {
                gb_compress_huffmann_add_to_list(bitcompress[i].count, &bitcompress[i]);
                bitcompress[i].command = gb_cs_ok;
            }
            else {
                restcount+= bitcompress[i].count;
                bitcompress[i].count = 0;
                id = i;
                bitcompress[i].command = gb_cs_id;
            }
        }
        bitcompress[end].command = gb_cs_end;

        gb_compress_huffmann_add_to_list(restcount,&bitcompress[id]);
        gb_compress_huffmann_add_to_list(1,&bitcompress[end]);
        while (gb_compress_huffmann_list->next) {
            gb_compress_huffmann_pop(&(vali[0]),&element1);
            gb_compress_huffmann_pop(&(vali[1]),&element2);

            bc          = (struct gb_compress_list *)gbm_get_mem(sizeof(struct gb_compress_list),GBM_CB_INDEX);
            bc->command = gb_cd_node;
            bc->son[0]  = element1;
            bc->son[1]  = element2;

            if (element1->command == gb_cd_node) {
                bc->bits = element1->bits+1;
                if (element2->command == gb_cd_node && element2->bits >= bc->bits) bc->bits = element2->bits+1;
            }
            else {
                if (element2->command == gb_cd_node) {
                    bc->bits = element2->bits+1;
                }
                else {
                    bc->bits = 1;
                }
            }

            gb_assert(bc->bits <= 7); // max. 7 bits allowed

            // if already 6 bits used -> put to end of list; otherwise sort in
            gb_compress_huffmann_add_to_list(bc->bits >= 6 ? LONG_MAX : vali[0]+vali[1], bc);
        }
        gb_compress_huffmann_pop(&(vali[0]),&element1);
#if defined(TEST_HUFFMAN_CODE)
        printf("huffman list:\n");
        gb_dump_huffmann_list(bc, "");
#endif /* TEST_HUFFMAN_CODE */
        dest      = gb_compress_huffmann_rek(bc,1,0,dest);
        *(dest++) = 0;
    }
    pbid =  &bitcompress[id];
    s = (unsigned char *)source;
    {
        int bitptr, bits, bitc;

        GB_INIT_WRITE_BITS(dest,bitptr);
        for (len = size; len; len--) {
            val = *(s++);
            command = bitcompress[val].command;
            if (command == gb_cs_ok) {
                pbc = &bitcompress[val];
                bits = pbc->bits;
                bitc = pbc->bitcnt;
                GB_WRITE_BITS(dest, bitptr, bitc, bits, h_i);
            }else{
                bits = pbid->bits;
                bitc = pbid->bitcnt;
                GB_WRITE_BITS(dest, bitptr, bitc, bits, h_i);

                bits = val;
                bitc = 8;
                GB_WRITE_BITS(dest, bitptr, bitc, bits, h_i);
            }
        }
        bits = bitcompress[end].bits;
        bitc = bitcompress[end].bitcnt;
        GB_WRITE_BITS(dest, bitptr, bitc, bits, h_i);
    }
    *msize = dest - buffer + 1;
#if defined(TEST_HUFFMAN_CODE)
    printf("huffman compression %li -> %li (%5.2f %%)\n", size, *msize, (double)((double)(*msize)/size*100));
#endif /* TEST_HUFFMAN_CODE */
    if (*msize >size*2) printf("ssize %d, size %d\n",(int)size,(int)*msize);
    return buffer;
}


/********************************************************************************************
                                        Uncompress  bytes
********************************************************************************************/

static GB_BUFFER gb_uncompress_equal_bytes(GB_CBUFFER s, long size, long *new_size) {
    const signed char *source = (signed char*)s;
    char              *dest;
    unsigned int       c;
    long               j;
    long               i,k;
    char              *buffer;

    dest = buffer = GB_give_other_buffer((char *)source,size);

#if defined(DEBUG) && 0
    printf("gb_uncompress_equal_bytes(size=%li):\n", size);
#endif /* DEBUG */

    for (i=size;i;) {
        j = *(source++);
#if defined(DEBUG) && 0
        printf("size=%li (code=%i)\n", i, (int)j);
#endif /* DEBUG */
        if (j>0) {                      /* uncompressed data block */
            if (j>i) j=i;
            i -= j;
            for (;j;j--) {
                *(dest++) = (char )(*(source++));
            }
        }else{                  /* equal bytes compressed */
            if (!j) break;      /* end symbol */
            if (j== -122) {
                j = *(source++) & 0xff;
                j |= ((*(source++)) <<8) &0xff00;
                j = -j;
            }
            c = *(source++);
            i += j;
            if (i<0) {
                /* GB_internal_error("Internal Error: Missing end in data"); */
                j += -i;
                i = 0;
            }
            if (j<-30){             /* set multiple bytes */
                j = -j;
                if ( ((long)dest) & 1) {
                    *(dest++) = c;
                    j--;
                }
                if ( ((long)dest) &2){
                    *(dest++) = c;
                    *(dest++) = c;
                    j-=2;
                }
                c &= 0xff;      /* copy c to upper bytes */
                c |= (c<<8);
                c |= (c<<16);
                k = j&3;
                j = j>>2;
                for (;j;j--){
                    *((GB_UINT4 *)dest) = c;
                    dest += sizeof(GB_UINT4);
                }
                j = k;
                for (;j;j--) *(dest++) = c;
            }else{
                for (;j;j++) *(dest++) = c;
            }
        }
    }

    *new_size = dest-buffer;
    gb_assert(size >= *new_size); // buffer overflow

    return buffer;
}

static GB_BUFFER gb_uncompress_huffmann(GB_CBUFFER source, long maxsize, long *new_size) {
    struct gb_compress_tree *un_tree, *t;
    
    char *data[1];
    char *p, *buffer;
    long  bitp;
    char  ch = 0, *s;
    long  val,command;

    un_tree = gb_build_uncompress_tree((unsigned char *)source, 0, data);
    if (!un_tree) return 0;

    bitp = 0;
    buffer = p = GB_give_other_buffer(source,maxsize);
    s = data[0];
    while(1) {
        for (t = un_tree; !t->leave;) {
            int bit;
            GB_READ_BIT(s,ch,bitp,bit);
            t = t->son[bit];
        }
        command = (long) t->son[1];
        if (command == gb_cs_end) break;
        if (command == gb_cs_id) {
            GB_READ_BITS(s, ch, bitp, 8, val);
        } else {
            val = (long) t->son[0];
        }
        *(p++) = (int)val;
    }
    gb_free_compress_tree(un_tree);

    *new_size = p-buffer;
    gb_assert(maxsize >= *new_size); // buffer overflow

    return buffer;
}

GB_BUFFER gb_uncompress_bytes(GB_CBUFFER source, long size, long *new_size) {
    char *data = gb_uncompress_huffmann(source, size, new_size);
    if (data) data = gb_uncompress_equal_bytes(data, size, new_size);
    gb_assert(!data || size >= *new_size); // buffer overflow
    return data;
}

/********************************************************************************************
                                Compress  long and floats (4 byte values)
********************************************************************************************/

GB_BUFFER gb_uncompress_longs_old(GB_CBUFFER source, long size, long *new_size)
{                               /* size is byte value */
    char *res  = 0;
    char *data = gb_uncompress_huffmann(source, (size*9)/8, new_size);
    if (data) {
        char *p, *s0, *s1, *s2, *s3;
        GB_UINT4  mi, i;

        data = gb_uncompress_equal_bytes(data, size, new_size);

        gb_assert(*new_size == size);
        res = p = GB_give_other_buffer(data, size);
        
        gb_assert(sizeof(GB_UINT4) == 4);

        mi = (GB_UINT4)(size / 4);
        s0 = data + 0 * mi;
        s1 = data + 1 * mi;
        s2 = data + 2 * mi;
        s3 = data + 3 * mi;
        
        for (i = 0; i < mi; i++) {
            *(p++) = *(s0++);
            *(p++) = *(s1++);
            *(p++) = *(s2++);
            *(p++) = *(s3++);
        }

        *new_size = mi*4;
    }
    return res;
}


static GB_BUFFER gb_uncompress_longs(GB_CBUFFER data, long size, long *new_size) {
    const char *s0, *s1, *s2, *s3;
    char       *p,*res;
    long        mi, i;

    res = p = GB_give_other_buffer(data, size);

    gb_assert(sizeof(GB_UINT4) == 4);

    mi = size / 4;
    s0 = data + 0 * mi;
    s1 = data + 1 * mi;
    s2 = data + 2 * mi;
    s3 = data + 3 * mi;
    
    for (i = 0; i < mi; i++) {
        *(p++) = *(s0++);
        *(p++) = *(s1++);
        *(p++) = *(s2++);
        *(p++) = *(s3++);
    }

    *new_size = mi*4;
    return res;
}

GB_BUFFER gb_compress_longs(GB_CBUFFER source, long size, int last_flag){
    long        mi,i;
    const char *p;
    char       *s0,*s1,*s2,*s3;
    char       *dest = GB_give_other_buffer(source,size+1);

    mi = size/4;
    p = source;
    *(dest) = GB_COMPRESSION_SORTBYTES | last_flag;
    s0 = dest + 0*mi + 1;
    s1 = dest + 1*mi + 1;
    s2 = dest + 2*mi + 1;
    s3 = dest + 3*mi + 1;
    for (i=0;i<mi;i++) {
        *(s0++) = *(p++);
        *(s1++) = *(p++);
        *(s2++) = *(p++);
        *(s3++) = *(p++);
    }
    return dest;
}


/****************************************************************************************************
 *                      Dictionary Functions
 ****************************************************************************************************/

/*  Get Dictionary */
GB_DICTIONARY * gb_get_dictionary(GB_MAIN_TYPE *Main, GBQUARK key){
    struct gb_key_struct *ks = &Main->keys[key];
    if (ks->gb_key_disabled) return 0;
    if (!ks->gb_key){
        gb_load_single_key_data((GBDATA *)Main->data, key);
        if (Main->gb_key_data && !ks->gb_key){
            GB_internal_error("Couldn't load gb_key");
        }
    }
    return Main->keys[key].dictionary;
}



/********************** Overall Compression Algorithms **************************/
/**** Compresses a data string, returns 0 if no compression makes sense ****/

GB_BUFFER gb_compress_data(GBDATA *gbd, int key, GB_CBUFFER source, long size, long *msize, GB_COMPRESSION_MASK max_compr, GB_BOOL pre_compressed){
    char *data;
    int last_flag = GB_COMPRESSION_LAST;

    if (pre_compressed){
        last_flag = 0;
    }

    gb_assert(1);

    if (max_compr & GB_COMPRESSION_SORTBYTES){
        source = gb_compress_longs(source,size,last_flag);
        last_flag = 0;
        size++;                 /* @@@ OLI */
    }else if (max_compr & GB_COMPRESSION_DICTIONARY){
        GB_MAIN_TYPE *Main = GB_MAIN(gbd);
        GB_DICTIONARY *dict;
        if (!key){
            key = GB_KEY_QUARK(gbd);
        }
        dict = gb_get_dictionary(Main,key);
        if (dict) {
            long real_size = size-(GB_TYPE(gbd)==GB_STRING); /* for strings w/o trailing zero */

            if (real_size) {
                data = gb_compress_by_dictionary(dict, source, real_size, msize,last_flag,9999,3);

                if ( (*msize<=10 && size>10) || *msize < size*7/8) { /* successfull compression */
                    source = data;
                    size = *msize;
                    last_flag = 0;
                }
            }
        }

    }
    if (max_compr & GB_COMPRESSION_RUNLENGTH && size > GB_RUNLENGTH_MIN_SIZE) {
        data = gb_compress_equal_bytes(source,size,msize,last_flag);
        if (*msize < size-10 && *msize < size*7/8){             /* successfull compression */
            source = data;
            size = *msize;
            last_flag = 0;
        }
    }

    if (max_compr & GB_COMPRESSION_HUFFMANN && size > GB_HUFFMAN_MIN_SIZE) {
        data = gb_compress_huffmann(source,size,msize,last_flag);
        if (*msize < size-10 && *msize < size*7/8){             /* successfull compression */
            source = data;
            size = *msize;
            last_flag = 0;
        }
    }

    *msize = size;

    if (last_flag) return 0;                                    /* no compression */
    return (char *)source;
}

GB_CBUFFER gb_uncompress_data(GBDATA *gbd, GB_CBUFFER source, long size){
    int         last     = 0;
    const char *data     = (char *)source;
    long        new_size = -1;
    GB_ERROR    error    = 0;

    while (!last){
        int c = *((unsigned char *)(data++));
        if (c & GB_COMPRESSION_LAST) {
            last = 1;
            c &= ~GB_COMPRESSION_LAST;
        }
        if (c == GB_COMPRESSION_HUFFMANN) {
            data = gb_uncompress_huffmann(data,size + GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);
        }else if (c == GB_COMPRESSION_RUNLENGTH) {
            data = gb_uncompress_equal_bytes(data,size + GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);
        }else if (c == GB_COMPRESSION_DICTIONARY) {
            data = gb_uncompress_by_dictionary(gbd, data, size + GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);
        }else if (c == GB_COMPRESSION_SEQUENCE) {
            data = gb_uncompress_by_sequence(gbd,data,size,&error, &new_size);
        }else if (c == GB_COMPRESSION_SORTBYTES) {
            data = gb_uncompress_longs(data,size, &new_size);
        }else{
            error = GBS_global_string("Internal Error: Cannot uncompress data of field '%s'",GB_read_key_pntr(gbd));
        }

        if (!data && !error) error = GB_await_error();
        if (error) last = 1; // sth went wrong, stop
    }

    if (!error && new_size != size) {
        error = GBS_global_string("Wrong decompressed size (expected=%li, got=%li)", size, new_size);
    }

    if (error) {
        GB_export_error(error);
        data = 0;
    }

    return data;
}

GB_BOOL GB_is_directory_compressed(GBDATA *gbd) {
    int         type = GB_TYPE(gbd);
    const char *data = GB_GETDATA(gbd);

    if (data) {
        if (gbd->flags.compressed_data) {
            long     size     = GB_UNCOMPRESSED_SIZE(gbd, type);
            int      last     = 0;
            GB_ERROR error    = 0;
            long     new_size = -1; // dummy

            while (!last){
                int c = *((unsigned char *)(data++));

                if (c & GB_COMPRESSION_LAST) {
                    last = 1;
                    c &= ~GB_COMPRESSION_LAST;
                }

                if (c == GB_COMPRESSION_DICTIONARY) {
                    return GB_TRUE;
                }
                
                if (c == GB_COMPRESSION_HUFFMANN) {
                    data = gb_uncompress_huffmann(data,size + GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);
                }
                else if (c == GB_COMPRESSION_RUNLENGTH) {
                    data = gb_uncompress_equal_bytes(data,size + GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);
                }
                else if (c == GB_COMPRESSION_SEQUENCE) {
                    data = gb_uncompress_by_sequence(gbd, data, size, &error, &new_size);
                }
                else if (c == GB_COMPRESSION_SORTBYTES) {
                    data = gb_uncompress_longs(data, size, &new_size);
                }
                else{
                    error = GB_export_errorf("Internal Error: Cannot uncompress data of field '%s'",GB_read_key_pntr(gbd));
                }

                if (error) {
                    GB_internal_error(error);
                    break;
                }
            }
        }
    }

    return GB_FALSE;
}
