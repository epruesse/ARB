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

#include "gb_key.h"
#include "gb_t_prot.h"
#include "gb_compress.h"
#include "gb_localdata.h"

#if defined(DEBUG)
// #define TEST_HUFFMAN_CODE
#endif // DEBUG

#define GBTUM_COMPRESS_TREE_SIZE 32


#define GB_READ_BIT(p, c, bp, result)    if (!bp) { c = *(p++); bp = 8; }; result = (c>>7); result &= 1; c <<= 1; bp--

#define GB_INIT_WRITE_BITS(p, bp) *p = 0; bp = 8

#define GB_WRITE_BITS(p, bp, bitc, bits, i)     \
    if (bp<=0) { bp += 8; p++;  *p = 0; }       \
    if ((i=bp-bitc)<0) {                        \
        *p |=  bits>>(-i);                      \
        i += 8;                                 \
        bp += 8; p++; *p = 0;                   \
    }                                           \
    *p |=  bits<<i;                             \
    bp-=bitc;

#define GB_READ_BITS(p, c, bp, bitc, bits) {    \
        long _i;                                \
        int _r;                                 \
        bits = 0;                               \
        for (_i=bitc-1; _i>=0; _i--) {          \
            GB_READ_BIT(p, c, bp, _r);          \
            bits = (bits<<1) + _r;              \
        } }


__ATTR__USERESULT static GB_ERROR gb_check_huffmann_tree(gb_compress_tree *t) {
    if (t->leaf)
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
static void gb_dump_huffmann_tree(gb_compress_tree *t, const char *prefix) {
    if (t->leaf) {
        long command = (long)t->son[1];
        printf("%s", prefix);

        switch (command) {
            case GB_CS_END: printf(" [GB_CS_END]\n"); break;
            case GB_CS_ID: printf(" [GB_CS_ID]\n"); break;
            case GB_CS_OK: {
                long val = (long)t->son[0];
                printf(": value=%li (='%c')\n", val, (char)val);
                break;
            }
            default: {
                long val = (long)t->son[0];
                printf(" other command (%li) value=%li (='%c')\n", command, val, (char)val);
                break;
            }
        }
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
static void gb_dump_huffmann_list(gb_compress_list *bc, const char *prefix) {
    if (bc->command == GB_CD_NODE) {
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
        printf("%s  value=%i (='%c') count=%li\n", prefix, bc->value, (char)bc->value, bc->count);
    }
}

#endif // TEST_HUFFMAN_CODE

gb_compress_tree *gb_build_uncompress_tree(const unsigned char *data, long short_flag, char **end) {
    gb_compress_tree    *Main, *t;
    long                 bits, mask, i;
    const unsigned char *p;
    GB_ERROR             error;

    Main = (gb_compress_tree *)gbm_get_mem(sizeof(gb_compress_tree), GBM_CB_INDEX);
    for (p=data; *p; p+=3+short_flag) {
        bits = p[0];
        mask = 0x80;
        for (i=7; i; i--, mask=mask>>1) if (mask & bits) break; // find first bit
        if (!i) {
            GB_internal_error("Data corrupt");
            return 0;
        }
        t = Main;
        for (; i; i--) {
            if (t->leaf) {
                GB_export_error("Corrupt data !!!");
                return 0;
            }
            mask = mask>>1;
            if (mask & bits) {
                if (!t->son[1]) {
                    t->son[1] = (gb_compress_tree *)gbm_get_mem(sizeof(gb_compress_tree), GBM_CB_INDEX);
                }
                t=t->son[1];
            }
            else {
                if (!t->son[0]) {
                    t->son[0] = (gb_compress_tree *)gbm_get_mem(sizeof(gb_compress_tree), GBM_CB_INDEX);
                }
                t=t->son[0];
            }
        }
        if (t->leaf) {
            GB_export_error("Corrupt data !!!");
            return 0;
        }
        t->leaf = 1;
        if (short_flag) {
            t->son[0] = (gb_compress_tree *)(long)((p[2]<<8)+p[3]);
        }
        else {
            t->son[0] = (gb_compress_tree *)(long)(p[2]);
        }
        t->son[1] = (gb_compress_tree *)(long)p[1]; // command
    }
    if (end) *end = ((char *)p)+1;
    if ((error = gb_check_huffmann_tree(Main))) {
        GB_internal_errorf("%s", error);
        gb_free_compress_tree(Main);
        return 0;
    }

#if defined(TEST_HUFFMAN_CODE)
    printf("Huffman tree:\n");
    gb_dump_huffmann_tree(Main, "");
#endif // TEST_HUFFMAN_CODE

    return Main;
}

void gb_free_compress_tree(gb_compress_tree *tree) {
    if (tree) {
        if (!tree->leaf) {
            if (tree->son[0]) gb_free_compress_tree(tree->son[0]);
            if (tree->son[1])gb_free_compress_tree(tree->son[1]);
        }
    }
    gbm_free_mem(tree, sizeof(gb_compress_tree), GBM_CB_INDEX);
}

gb_compress_list *gb_build_compress_list(const unsigned char *data, long short_flag, long *size) {
    int                  i, maxi, bitc;
    int                  val, bits, mask, value;
    const unsigned char *p;

    enum gb_compress_list_commands command = (enum gb_compress_list_commands)0;

    maxi = 0;
    val  = bits = mask = value = bitc = 0;
    for (p=data; *p; p+=3+short_flag) {
        i = (p[2]);
        if (short_flag) {
            i = (i<<8)+p[3];
        }
        if (i>maxi) maxi = i;
    }
    *size = maxi;

    gb_compress_list *list = (gb_compress_list *)GB_calloc(sizeof(gb_compress_list), (size_t)maxi+1);

    maxi = 0;
    val = -1;

    for (p=data; *p; p+=3+short_flag) {
        val = (p[2]);
        if (short_flag) val = (val<<8)+p[3];

        for (i=maxi; i<val; i++) {
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
        for (bitc=7; bitc; bitc--, mask=mask>>1) if (mask & bits) break; // find first bit
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

// ------------------------
//      bit compression


char *gb_compress_bits(const char *source, long size, const unsigned char *c_0, long *msize)
{
    const unsigned char *s      = (const unsigned char *)source;
    char                *buffer = GB_give_other_buffer(source, size);
    char                *dest   = buffer;

    long  i, j;
    int   bitptr, bits, bitc;
    int   zo_flag = 0;
    long  len;
    int   h_i, command;
    int   isNull[256];

    for (i = 0; i<256; ++i) isNull[i] = 0;
    for (i = 0; c_0[i]; ++i) isNull[c_0[i]] = 1;

    GB_INIT_WRITE_BITS(dest, bitptr);
    for (len = size, i = 0; len; len--) {
        if (zo_flag == isNull[*(s++)]) {
            zo_flag = 1 - zo_flag;
            for (command = GB_CS_SUB; command != GB_CS_OK;) {
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
    for (command = GB_CS_SUB; command != GB_CS_OK;) {
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


GB_BUFFER gb_uncompress_bits(const char *source, long size, char c_0, char c_1) {
    gb_compress_tree *Main = gb_local->bituncompress;

    char ch   = 0;
    long bitp = 0;
    char outc = c_0;

    const char *s      = source;
    char       *p      = GB_give_other_buffer(source, size+1);
    char       *buffer = p;

    for (long pos = 0; pos<size;) {
        long lastpos = pos;
        for (long command = GB_CS_SUB; command != GB_CS_OK;) {
            gb_compress_tree *t;
            for (t = Main; !t->leaf;) {
                int bit;
                GB_READ_BIT(s, ch, bitp, bit);
                t = t->son[bit];
            }
            command = (long) t->son[1];
            pos += (long)t->son[0];
        }
        for (; lastpos<pos; lastpos++) *(p++) = outc;
        if (outc==c_0) outc=c_1; else outc=c_0;
    }
    *p = 0;
    return buffer;
}



/* Runlength encoding produces..
 *
 * from source: string
 * dest:   [command]+
 *
 * command     data                     meaning
 *
 * -122        low high byte            'byte' was repeated 'low'+256*'high' times
 * -119..-1    byte                     'byte' was repeated -'command' times
 * 0           ---                      end
 * 1..119      [byte]+                 'command' data bytes follow
 */

static char *g_b_write_run(char *dest, int scount, int lastbyte) {
    while (scount> 0xffff) {
        *(dest++) = 0x100-122;
        *(dest++) = 0xff;
        *(dest++) = 0xff;
        *(dest++) = lastbyte;
        scount -= 0xffff;
    }
    if (scount >250) {
        *(dest++) = 0x100-122;
        *(dest++) = scount & 0xff;
        *(dest++) = (scount >> 8) & 0xff;
        *(dest++) = lastbyte;
        return dest;
    }

    while (scount>120) {                            // 120 blocks
        *(dest++) = 0x100 - 120;
        *(dest++) = lastbyte;
        scount -= 120;
    }

    if (scount) {                       // and rest
        *(dest++) = -scount;
        *(dest++) = lastbyte;
    }
    return dest;
}

#define GB_COPY_NONRUN(dest, source, len)       \
    while (len > 120) {                         \
        int _i = 120;                           \
        char *_p;                               \
        len -= _i;                              \
        *(dest++) = _i;                         \
        _p = dest; dest+=_i;                    \
        while (_i--) {                          \
            *(_p++) = *(source++);              \
        }                                       \
    }                                           \
    if  (len >0) {                              \
        int _i = len;                           \
        char *_p;                               \
        len = 0;                                \
        *(dest++) = _i;                         \
        _p = dest; dest+=_i;                    \
        while (_i--) {                          \
            *(_p++) = *(source++);              \
        }                                       \
    }

void gb_compress_equal_bytes_2(const char *source, size_t size, size_t *msize, char *dest) {
    long        i;                                  // length counter
    int         last, rbyte;                        // next and akt value
    long        scount;                             // same count; count equal bytes
    char       *buffer = dest;
    const char *sourcenequal;                       // begin of non equal section
    long        hi;                                 // to temporary store i
    int         hsize;

    sourcenequal = source;
    rbyte = *(source ++);
    last = -1;
    for (i=size-1; i;) {
        if (rbyte!=last) {                          // Search an equal pair
            last = rbyte;
            rbyte = *(source ++); i--;
            continue;
        }                                           // last rbyte *(source)
        hi = i+1;
        while (i) {                                 // get equal bytes
            rbyte = *(source ++); i--;
            if (rbyte!=last) break;
        }
        scount = hi-i;                              // number of equal bytes ; at end e.b.-1
        if (scount <= GB_RUNLENGTH_SIZE) continue;  // less than 4-> no runlength compression

        /* compression !!!
         * 1. copy unequal bytes
         * 2. fill rest
         */

        hsize   = source - sourcenequal - scount -1; // hsize of non equal bytes
        source  = sourcenequal;
        hi      = i;                                // i=unequal length
        GB_COPY_NONRUN(dest, source, hsize);
        dest    = g_b_write_run(dest, scount, last);
        source += scount;                           // now equal bytes

        sourcenequal = source++;                    // no rbyte written yet
        last         = rbyte;
        i            = hi;
        if (i) {
            rbyte = *(source++); i--;
        }
    }
    // and now rest which is not compressed
    hsize = source - sourcenequal;
    source = sourcenequal;
    GB_COPY_NONRUN(dest, source, hsize);

    *(dest++) = 0;                  // end of data
    *msize = dest-buffer;
    if (*msize >size*9/8) printf("ssize %d, dsize %d\n", (int)size, (int)*msize);
}

static GB_BUFFER gb_compress_equal_bytes(const char *source, size_t size, size_t *msize, int last_flag) {
    char *dest;
    char *buffer;

    dest  = buffer = GB_give_other_buffer(source, size*9/8);
    *(dest++) = GB_COMPRESSION_RUNLENGTH | last_flag;
    gb_compress_equal_bytes_2(source, size, msize, dest);
    (*msize) ++;            // Tag byte
    return buffer;
}

struct huffmann_list {
    huffmann_list    *next;
    long              val;
    gb_compress_list *element;
};

static huffmann_list *huffmann_listhead = NULL;

static void gb_compress_huffmann_add_to_list(long val, gb_compress_list *element) {
    huffmann_list *dat, *search, *searchlast;

    dat          = (huffmann_list *)gbm_get_mem(sizeof(huffmann_list), GBM_CB_INDEX);
    dat->val     = val;
    dat->element = element;

    searchlast = 0;
    for (search = huffmann_listhead; search; search = search->next) {
        if (val<search->val) break;
        searchlast = search;
    }
    if (huffmann_listhead) {
        if (searchlast) {
            dat->next = searchlast->next;
            searchlast->next = dat;
        }
        else {
            dat->next = huffmann_listhead;
            huffmann_listhead = dat;
        }
    }
    else {
        huffmann_listhead = dat;
    }
}

static long gb_compress_huffmann_pop(long *val, gb_compress_list **element) {
    huffmann_list *dat;
    if ((dat = huffmann_listhead)) {
        huffmann_listhead = dat->next;
        *val = dat->val;
        *element = dat->element;
        gbm_free_mem(dat, sizeof(huffmann_list), GBM_CB_INDEX);
        return 1;
    }
    else {
        GB_internal_error("huffman compression failed");
        return 0;
    }
}

static char *gb_compress_huffmann_rek(gb_compress_list *bc, int bits, int bitcnt, char *dest) {
    if (bc->command == GB_CD_NODE) {
        dest = gb_compress_huffmann_rek(bc->son[0], (bits<<1), bitcnt+1, dest);
        dest = gb_compress_huffmann_rek(bc->son[1], (bits<<1)+1, bitcnt+1, dest);
        gbm_free_mem(bc, sizeof(gb_compress_list), GBM_CB_INDEX);
        return dest;
    }
    else {
        *(dest++) = bits;
        *(dest++) = bc->command;
        *(dest++) = bc->value;
        bc->bitcnt = bitcnt;
        bc->mask = 0xff>>(8-bitcnt);
        bc->bits = bits&bc->mask;
        return dest;
    }
}

static GB_BUFFER gb_compress_huffmann(GB_CBUFFER source, size_t size, size_t *msize, int last_flag) {
    const int        COMPRESS_LIST_SIZE = 257;
    gb_compress_list bitcompress[COMPRESS_LIST_SIZE];

    memset((char *)(&bitcompress[0]), 0, sizeof(gb_compress_list)*COMPRESS_LIST_SIZE);

    char *buffer = GB_give_other_buffer(source, size*2+3*GBTUM_COMPRESS_TREE_SIZE+1);
    char *dest   = buffer;
    *(dest++)    = GB_COMPRESSION_HUFFMANN | last_flag;

    long id = 0;

    {
        long level;
        long i;
        long restcount;

        long vali[2] = { 0, 0 };

        gb_compress_list *element1 = 0;
        gb_compress_list *element2 = 0;
        gb_compress_list *bc       = 0;

        unsigned char *s = (unsigned char *)source;
        for (long len = size; len; len--) {
            bitcompress[*(s++)].count++;
        }
        level = size/GBTUM_COMPRESS_TREE_SIZE;
        restcount = 0;
        for (i=0; i<256; i++) {
            bitcompress[i].value = (int)i;
            if (bitcompress[i].count>level) {
                gb_compress_huffmann_add_to_list(bitcompress[i].count, &bitcompress[i]);
                bitcompress[i].command = GB_CS_OK;
            }
            else {
                restcount += bitcompress[i].count;
                bitcompress[i].count = 0;
                id = i;
                bitcompress[i].command = GB_CS_ID;
            }
        }
        bitcompress[COMPRESS_LIST_SIZE-1].command = GB_CS_END;

        gb_compress_huffmann_add_to_list(restcount, &bitcompress[id]);
        gb_compress_huffmann_add_to_list(1, &bitcompress[COMPRESS_LIST_SIZE-1]);
        while (huffmann_listhead->next) {
            gb_compress_huffmann_pop(&(vali[0]), &element1);
            gb_compress_huffmann_pop(&(vali[1]), &element2);

            bc          = (gb_compress_list *)gbm_get_mem(sizeof(gb_compress_list), GBM_CB_INDEX);
            bc->command = GB_CD_NODE;
            bc->son[0]  = element1;
            bc->son[1]  = element2;

            if (element1->command == GB_CD_NODE) {
                bc->bits = element1->bits+1;
                if (element2->command == GB_CD_NODE && element2->bits >= bc->bits) bc->bits = element2->bits+1;
            }
            else {
                if (element2->command == GB_CD_NODE) {
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
        gb_compress_huffmann_pop(&(vali[0]), &element1);
#if defined(TEST_HUFFMAN_CODE)
        printf("huffman list:\n");
        gb_dump_huffmann_list(bc, "");
#endif // TEST_HUFFMAN_CODE
        dest      = gb_compress_huffmann_rek(bc, 1, 0, dest);
        *(dest++) = 0;
    }

    gb_compress_list *pbid = &bitcompress[id];
    unsigned char    *s    = (unsigned char *)source;
    {
        int bitptr, bits, bitc;

        GB_INIT_WRITE_BITS(dest, bitptr);
        int h_i;
        for (long len = size; len; len--) {
            int val     = *(s++);
            int command = bitcompress[val].command;
            if (command == GB_CS_OK) {
                gb_compress_list *pbc = &bitcompress[val];
                bits = pbc->bits;
                bitc = pbc->bitcnt;
                GB_WRITE_BITS(dest, bitptr, bitc, bits, h_i);
            }
            else {
                bits = pbid->bits;
                bitc = pbid->bitcnt;
                GB_WRITE_BITS(dest, bitptr, bitc, bits, h_i);

                bits = val;
                bitc = 8;
                GB_WRITE_BITS(dest, bitptr, bitc, bits, h_i);
            }
        }
        bits = bitcompress[COMPRESS_LIST_SIZE-1].bits;
        bitc = bitcompress[COMPRESS_LIST_SIZE-1].bitcnt;
        // cppcheck-suppress unreadVariable (bitptr is assigned but never used)
        GB_WRITE_BITS(dest, bitptr, bitc, bits, h_i);
    }
    *msize = dest - buffer + 1;
#if defined(TEST_HUFFMAN_CODE)
    printf("huffman compression %zu -> %zu (%5.2f %%)\n", size, *msize, (double)((double)(*msize)/size*100));
#endif // TEST_HUFFMAN_CODE
    if (*msize >size*2) printf("ssize %d, size %d\n", (int)size, (int)*msize);
    return buffer;
}


static GB_BUFFER gb_uncompress_equal_bytes(GB_CBUFFER s, size_t size, size_t *new_size) {
    const signed char *source = (signed char*)s;
    char              *dest;
    unsigned int       c;
    long               j;
    long               i, k;
    char              *buffer;

    dest = buffer = GB_give_other_buffer((char *)source, size);

#if defined(DEBUG) && 0
    printf("gb_uncompress_equal_bytes(size=%li):\n", size);
#endif // DEBUG

    for (i=size; i;) {
        j = *(source++);
#if defined(DEBUG) && 0
        printf("size=%li (code=%i)\n", i, (int)j);
#endif // DEBUG
        if (j>0) {                                  // uncompressed data block
            if (j>i) j=i;
            i -= j;
            for (; j; j--) {
                *(dest++) = (char)(*(source++));
            }
        }
        else {                                      // equal bytes compressed
            if (!j) break;                          // end symbol
            if (j == -122) {
                j = *(source++) & 0xff;
                j |= ((*(source++)) << 8) &0xff00;
                j = -j;
            }
            c = *(source++);
            i += j;
            if (i<0) {
                j += -i;
                i = 0;
            }
            if (j<-30) {                            // set multiple bytes
                j = -j;
                if (((long)dest) & 1) {
                    *(dest++) = c;
                    j--;
                }
                if (((long)dest) &2) {
                    *(dest++) = c;
                    *(dest++) = c;
                    j-=2;
                }
                c &= 0xff;                          // copy c to upper bytes
                c |= (c<<8);
                c |= (c<<16);
                k  = j&3;
                j  = j>>2;
                for (; j; j--) {
                    *((GB_UINT4 *)dest) = c;
                    dest += sizeof(GB_UINT4);
                }
                j = k;
                for (; j; j--) *(dest++) = c;
            }
            else {
                for (; j; j++) *(dest++) = c;
            }
        }
    }

    *new_size = dest-buffer;
    gb_assert(size >= *new_size);                   // buffer overflow

    return buffer;
}

static GB_BUFFER gb_uncompress_huffmann(GB_CBUFFER source, size_t maxsize, size_t *new_size) {
    gb_compress_tree *un_tree, *t;

    char *data[1];
    char *p, *buffer;
    long  bitp;
    char  ch = 0, *s;
    long  val, command;

    un_tree = gb_build_uncompress_tree((unsigned char *)source, 0, data);
    if (!un_tree) return 0;

    bitp = 0;
    buffer = p = GB_give_other_buffer(source, maxsize);
    s = data[0];
    while (1) {
        for (t = un_tree; !t->leaf;) {
            int bit;
            GB_READ_BIT(s, ch, bitp, bit);
            t = t->son[bit];
        }
        command = (long) t->son[1];
        if (command == GB_CS_END) break;
        if (command == GB_CS_ID) {
            GB_READ_BITS(s, ch, bitp, 8, val);
        }
        else {
            val = (long) t->son[0];
        }
        *(p++) = (int)val;
    }
    gb_free_compress_tree(un_tree);

    *new_size = p-buffer;
    gb_assert(maxsize >= *new_size);                // buffer overflow

    return buffer;
}

GB_BUFFER gb_uncompress_bytes(GB_CBUFFER source, size_t size, size_t *new_size) {
    char *data     = gb_uncompress_huffmann(source, size, new_size);
    if (data) data = gb_uncompress_equal_bytes(data, size, new_size);
    gb_assert(!data || size >= *new_size);          // buffer overflow
    return data;
}

// -------------------------------------------------
//      Compress long and floats (4 byte values)


GB_BUFFER gb_uncompress_longs_old(GB_CBUFFER source, size_t size, size_t *new_size)
{                               // size is byte value
    char *res  = 0;
    char *data = gb_uncompress_huffmann(source, (size*9)/8, new_size);
    if (data) {
        char *p, *s0, *s1, *s2, *s3;
        GB_UINT4  mi, i;

        data = gb_uncompress_equal_bytes(data, size, new_size);

        gb_assert(*new_size == size);
        res = p = GB_give_other_buffer(data, size);

        STATIC_ASSERT(sizeof(GB_UINT4) == 4);

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


static GB_BUFFER gb_uncompress_longs(GB_CBUFFER data, size_t size, size_t *new_size) {
    const char *s0, *s1, *s2, *s3;
    char       *p, *res;
    long        mi, i;

    res = p = GB_give_other_buffer(data, size);

    STATIC_ASSERT(sizeof(GB_UINT4) == 4);

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

static GB_BUFFER gb_compress_longs(GB_CBUFFER source, long size, int last_flag) {
    long        mi, i;
    const char *p;
    char       *s0, *s1, *s2, *s3;
    char       *dest = GB_give_other_buffer(source, size+1);

    mi = size/4;
    p = source;
    *(dest) = GB_COMPRESSION_SORTBYTES | last_flag;
    s0 = dest + 0*mi + 1;
    s1 = dest + 1*mi + 1;
    s2 = dest + 2*mi + 1;
    s3 = dest + 3*mi + 1;
    for (i=0; i<mi; i++) {
        *(s0++) = *(p++);
        *(s1++) = *(p++);
        *(s2++) = *(p++);
        *(s3++) = *(p++);
    }
    return dest;
}

// -----------------------------
//      Dictionary Functions

GB_DICTIONARY * gb_get_dictionary(GB_MAIN_TYPE *Main, GBQUARK key) {
    gb_Key *ks = &Main->keys[key];
    if (ks->gb_key_disabled) return 0;
    if (!ks->gb_key) {
        gb_load_single_key_data(Main->gb_main(), key);
        if (Main->gb_key_data && !ks->gb_key) {
            GB_internal_error("Couldn't load gb_key");
        }
    }
    return Main->keys[key].dictionary;
}

bool GB_is_dictionary_compressed(GBDATA *gbd) {
    if (gbd->is_entry()) {
        GBENTRY    *gbe  = gbd->as_entry();
        const char *data = gbe->data();

        if (data) {
            if (gbe->flags.compressed_data) {
                size_t   size     = gbe->uncompressed_size();
                int      last     = 0;
                GB_ERROR error    = 0;
                size_t   new_size = -1;

                while (!last) {
                    int c = *((unsigned char *)(data++));

                    if (c & GB_COMPRESSION_LAST) {
                        last = 1;
                        c &= ~GB_COMPRESSION_LAST;
                    }

                    if (c == GB_COMPRESSION_DICTIONARY) {
                        return true;
                    }

                    if (c == GB_COMPRESSION_HUFFMANN) {
                        data = gb_uncompress_huffmann(data, size + GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);
                    }
                    else if (c == GB_COMPRESSION_RUNLENGTH) {
                        data = gb_uncompress_equal_bytes(data, size + GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);
                    }
                    else if (c == GB_COMPRESSION_SEQUENCE) {
                        data = gb_uncompress_by_sequence(gbe, data, size, &error, &new_size);
                    }
                    else if (c == GB_COMPRESSION_SORTBYTES) {
                        data = gb_uncompress_longs(data, size, &new_size);
                    }
                    else {
                        error = GB_export_errorf("Internal Error: Cannot uncompress data of field '%s'", GB_read_key_pntr(gbe));
                    }

                    if (error) {
                        GB_internal_error(error);
                        break;
                    }
                }
            }
        }
    }
    return false;
}

// -----------------------------------
//      Top compression algorithms

GB_BUFFER gb_compress_data(GBDATA *gbd, int key, GB_CBUFFER source, size_t size, size_t *msize, GB_COMPRESSION_MASK max_compr, bool pre_compressed) {
    // Compress a data string.
    //
    // Returns:
    // - NULL if no compression makes sense
    // - otherwise returns compressed data and stores its size in 'msize'

    char *data;
    int   last_flag = GB_COMPRESSION_LAST;

    if (pre_compressed) {
        last_flag = 0;
    }

    gb_assert(1);

    if (max_compr & GB_COMPRESSION_SORTBYTES) {
        source = gb_compress_longs(source, size, last_flag);
        last_flag = 0;
        size++;                 // @@@ OLI
    }
    else if (max_compr & GB_COMPRESSION_DICTIONARY) {
        GB_MAIN_TYPE *Main = GB_MAIN(gbd);
        GB_DICTIONARY *dict;
        if (!key) {
            key = GB_KEY_QUARK(gbd);
        }
        dict = gb_get_dictionary(Main, key);
        if (dict) {
            size_t real_size = size-(gbd->type()==GB_STRING); // for strings w/o trailing zero; @@@ or use is_a_string()?

            if (real_size) {
                data = gb_compress_by_dictionary(dict, source, real_size, msize, last_flag, 9999, 3);

                if ((*msize<=10 && size>10) || *msize < size*7/8) {  // successfull compression
                    source = data;
                    size = *msize;
                    last_flag = 0;
                }
            }
        }

    }
    if (max_compr & GB_COMPRESSION_RUNLENGTH && size > GB_RUNLENGTH_MIN_SIZE) {
        data = gb_compress_equal_bytes(source, size, msize, last_flag);
        if (*msize < size-10 && *msize < size*7/8) { // successfull compression
            source = data;
            size = *msize;
            last_flag = 0;
        }
    }

    if (max_compr & GB_COMPRESSION_HUFFMANN && size > GB_HUFFMAN_MIN_SIZE) {
        data = gb_compress_huffmann(source, size, msize, last_flag);
        if (*msize < size-10 && *msize < size*7/8) { // successfull compression
            source = data;
            size = *msize;
            last_flag = 0;
        }
    }

    *msize = size;

    if (last_flag) return 0;                        // no compression
    return (char *)source;
}

GB_CBUFFER gb_uncompress_data(GBDATA *gbd, GB_CBUFFER source, size_t size) {
    int         last     = 0;
    const char *data     = (char *)source;
    size_t      new_size = -1;
    GB_ERROR    error    = 0;

    while (!last) {
        int c = *((unsigned char *)(data++));
        if (c & GB_COMPRESSION_LAST) {
            last = 1;
            c &= ~GB_COMPRESSION_LAST;
        }
        if (c == GB_COMPRESSION_HUFFMANN) {
            data = gb_uncompress_huffmann(data, size + GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);
        }
        else if (c == GB_COMPRESSION_RUNLENGTH) {
            data = gb_uncompress_equal_bytes(data, size + GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);
        }
        else if (c == GB_COMPRESSION_DICTIONARY) {
            data = gb_uncompress_by_dictionary(gbd, data, size + GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);
        }
        else if (c == GB_COMPRESSION_SEQUENCE) {
            data = gb_uncompress_by_sequence(gbd, data, size, &error, &new_size);
        }
        else if (c == GB_COMPRESSION_SORTBYTES) {
            data = gb_uncompress_longs(data, size, &new_size);
        }
        else {
            error = GBS_global_string("Internal Error: Cannot uncompress data of field '%s'", GB_read_key_pntr(gbd));
        }

        if (!data && !error) error = GB_await_error();
        if (error) last = 1;                        // sth went wrong, stop
    }

    if (!error && new_size != size) {
        error = GBS_global_string("Wrong decompressed size (expected=%zi, got=%zi)", size, new_size);
    }

    if (error) {
        GB_export_error(error);
        data = 0;
    }

    return data;
}

