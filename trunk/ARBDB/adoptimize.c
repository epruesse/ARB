#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "adlocal.h"
/*#include "arbdb.h"*/
#include "arbdbt.h"

#if defined(DEBUG)
#define TEST_DICT
#endif /* DEBUG */

typedef unsigned char unsigned_char;
typedef unsigned char *u_str;
typedef const unsigned char *cu_str;

struct S_GB_FULL_DICT_TREE;
struct S_GB_SINGLE_DICT_TREE;

static int gbdByKey_cnt;
typedef struct 	/* one for each diff. keyQuark */
{
    int		cnt;
    GBDATA 	**gbds;	/* gbdoff */
} O_gbdByKey;

typedef union U_GB_DICT_TREE
{
    struct S_GB_FULL_DICT_TREE *full;
    struct S_GB_SINGLE_DICT_TREE *single;
    void *exists;

} GB_DICT_TREE;

typedef enum { SINGLE_NODE, FULL_NODE } GB_DICT_NODE_TYPE;

typedef struct S_GB_FULL_DICT_TREE
{
    GB_DICT_NODE_TYPE 	typ;				/* always FULL_NODE */
    int 		usedSons;
    int 		count[256];
    GB_DICT_TREE 	son[256];			/* index == character */

} GB_FULL_DICT_TREE;

typedef struct S_GB_SINGLE_DICT_TREE
{
    GB_DICT_NODE_TYPE	typ;		/* always SINGLE_NODE */
    unsigned_char		ch;		/* the character */
    int 		count;		/* no of occurances of this branch */
    GB_DICT_TREE 	son;
    GB_DICT_TREE 	brother;

} GB_SINGLE_DICT_TREE;

/****************************************************/

#define COMPRESSABLE(type) 			((type)>=GB_BYTES && (type)<=GB_STRING)
#define DICT_MEM_WEIGHT 			4
#define WORD_HELPFUL(wordlen, occurences) 	((long)((occurences)*3 + DICT_MEM_WEIGHT*(2*sizeof(GB_NINT)+(wordlen))) \
						 <								  \
						 (long)((occurences)*(wordlen)))
/* (occurences)*4			compressed size
 * 2*sizeof(GB_NINT)+(wordlen)		size in dictionary
 * (occurences)*(wordlen)		uncompressed size
 */

/****************************************************/

#define MIN_WORD_LEN   	8	/* minimum length of words in dictionary */
#define MAX_WORD_LEN	50	/* maximum length of words in dictionary */
#define MAX_BROTHERS   	10	/* maximum no of brothers linked with GB_SINGLE_DICT_TREE
                             * above we use GB_FULL_DICT_TREE */
#define MAX_DIFFER     	 2	/* percentage of difference (of occurances of strings) below which two
                             * consecutive parts are treated as EQUAL # of occurances */
#define INCR_DIFFER      1      /* the above percentage is incremented from 0 to MAX_DIFFER by INCR_DIFFER per step */

#define DICT_STRING_INCR 1024	/* dictionary string will be incremented by this size */

/******************** Tool functions *******************/

static GB_INLINE u_str get_data_n_size(GBDATA *gbd, long *size)
{
    char *data;
    int type = GB_TYPE(gbd);

    switch (type)
    {
        case GB_STRING:
            data = GB_read_char_pntr(gbd);
            *size = GB_read_string_count(gbd);
            break;
        case GB_LINK:
            data = GB_read_link_pntr(gbd);
            *size = GB_read_link_count(gbd);
            break;
        case GB_BYTES:
            data = GB_read_bytes_pntr(gbd);
            *size = GB_read_bytes_count(gbd);
            break;
        case GB_INTS:
            data = (char*)GB_read_ints_pntr(gbd);
            *size = GB_read_ints_count(gbd);
            break;
        case GB_FLOATS:
            data = (char*)GB_read_floats_pntr(gbd);
            *size = GB_read_floats_count(gbd);
            break;
        default:
            data = 0;
            ad_assert(0);
            break;
    }

    if (!data)
    {
        *size = 0;
        return NULL;
    }

    *size = *size * gb_convert_type_2_sizeof[type] + gb_convert_type_2_appendix_size[type];

#if 0
    if (gbd->flags.compressed_data)
        data = gb_uncompress_data(gbd,(GB_CSTR)data,*size);
#endif

    return (u_str)data;
}

static GB_INLINE long min(long a, long b)
{
    return a<b ? a : b;
}

/****************************************************/

static void g_b_opti_scanGbdByKey(GB_MAIN_TYPE *Main, GBDATA *gbd, O_gbdByKey *gbk)
{
    unsigned int quark;

    if (GB_TYPE(gbd) == GB_DB)	/* CONTAINER */
    {
        int 		idx;
        GBCONTAINER 	*gbc = (GBCONTAINER *)gbd;
        GBDATA 		*gbd2;

        for (idx=0; idx < gbc->d.nheader; idx++)
            if ((gbd2=GBCONTAINER_ELEM(gbc,idx))!=NULL)
                g_b_opti_scanGbdByKey(Main,gbd2,gbk);
    }

    quark = GB_KEY_QUARK(gbd);

    if (quark)
    {
        ad_assert(gbk[quark].cnt < Main->keys[quark].nref || quark==0);
        ad_assert(gbk[quark].gbds != 0);

        gbk[quark].gbds[ gbk[quark].cnt ] = gbd;
        gbk[quark].cnt++;
    }
}

static O_gbdByKey *g_b_opti_createGbdByKey(GB_MAIN_TYPE *Main)
{
    int idx;
    O_gbdByKey *gbk = (O_gbdByKey *)GB_calloc(Main->keycnt, sizeof(O_gbdByKey));

    gbdByKey_cnt = Main->keycnt; /* always use gbdByKey_cnt instead of Main->keycnt cause Main->keycnt can change */

    for (idx=1; idx<gbdByKey_cnt; idx++){
        gbk[idx].cnt = 0;

        if (Main->keys[idx].key && Main->keys[idx].nref>0) {
            gbk[idx].gbds  = (GBDATA **) GB_calloc(Main->keys[idx].nref, sizeof(GBDATA*));
        }
        else {
            gbk[idx].gbds = NULL;
        }
    }

    gbk[0].cnt  = 0;
    gbk[0].gbds = (GBDATA **)GB_calloc(1,sizeof(GBDATA*));

    g_b_opti_scanGbdByKey(Main,(GBDATA*)Main->data,gbk);

    for (idx=0; idx<gbdByKey_cnt; idx++){
        if (gbk[idx].cnt != Main->keys[idx].nref && idx)
        {
            printf("idx=%i gbk[idx].cnt=%i Main->keys[idx].nref=%li\n",		    /* Main->keys[].nref ist falsch */
                   idx,gbk[idx].cnt,Main->keys[idx].nref);

            Main->keys[idx].nref = gbk[idx].cnt;
        }
        /*ad_assert(gbk[idx].cnt == Main->keys[idx].nref || idx==0);*/
    }
    return gbk;
}

static void g_b_opti_freeGbdByKey(GB_MAIN_TYPE *Main, O_gbdByKey *gbk)
{
    int idx;
    GBUSE(Main);
    for (idx=0; idx<gbdByKey_cnt; idx++) GB_FREE(gbk[idx].gbds);
    GB_FREE(gbk);
}

/******************** Convert old compression style to new style *******************/

GB_ERROR gb_convert_compression(GBDATA *source)
{
    long type;
    GB_ERROR error = 0;
    GBDATA *gb_p;
    char *string;
    long	size;

    type = GB_TYPE(source);
    if (type == GB_DB){
        for (	gb_p = GB_find(source,0,0,down_level);
                gb_p;
                gb_p = GB_find(gb_p,0,0,this_level|search_next)){
            if (gb_p->flags.compressed_data || GB_TYPE(gb_p) == GB_DB ){
                error = gb_convert_compression(gb_p);
                if (error) break;
            }
        }
        return error;
    }

    switch (type) {
        case GB_STRING:
            size = GB_GETSIZE(source)+1;
            string = gbs_malloc_copy(gb_uncompress_bytes(GB_GETDATA(source),size),size);
            GB_write_string(source,"");
            GB_write_string(source,string);
            break;
        case GB_LINK:
            size = GB_GETSIZE(source)+1;
            string = gbs_malloc_copy(gb_uncompress_bytes(GB_GETDATA(source),size),size);
            GB_write_link(source,"");
            GB_write_link(source,string);
            break;
        case GB_BYTES:
            size = GB_GETSIZE(source);
            string = gbs_malloc_copy(gb_uncompress_bytes(GB_GETDATA(source),size),size);
            GB_write_bytes(source,"",0);
            GB_write_bytes(source,string,size);
            free(string);
            break;

        case GB_INTS:
        case GB_FLOATS: {
            char zero_string[] = "";
            size = GB_GETSIZE(source);
            string = gb_uncompress_longs(GB_GETDATA(source),size*4);
            if (!string){
                string = zero_string; size = 0;
                GB_warning("Cannot uncompress '%s'",GB_read_key_pntr(source));
            }
            string = gbs_malloc_copy(string,size*4);
            error = GB_write_pntr(source,string,size*4,size);

            free(string);
            break;
        }

        default:
            break;
    }
    return error;
}

GB_ERROR gb_convert_V2_to_V3(GBDATA *gb_main){
    GBDATA *gb_system;
    GB_ERROR error = 0;
    GBDATA *gb_extended_data;
    gb_system = GB_search(gb_main,GB_SYSTEM_FOLDER, GB_FIND);
    if (gb_system){
        return error;
    }
    gb_system = GB_create_container(gb_main, GB_SYSTEM_FOLDER);
    gb_extended_data = GB_find(gb_main,"extended_data",0,down_level);
    if (gb_extended_data){
        GB_warning("Converting data from old V2.0 to V2.1 Format:\n"
                   " Please Wait (may take some time)");
    }
    error = gb_convert_compression(gb_main);
    GB_disable_quicksave(gb_main,"Database converted to new format");
    return error;
}


/* ********************* Compress by dictionary ******************** */


/* compression tag format:

   unsigned int compressed:1;
       if compressed==0:
           unsigned int last:1;		==1 -> this is the last block
	   unsigned int len:6;          length of uncompressable bytes
	   char[len];
       if compressed==1:
           unsigned int idxlen:1;	==0 -> 10-bit index
	                                ==1 -> 18-bit index
	   unsigned int idxhigh:2;      the 2 highest bits of the index
	   unsigned int len:4;          (length of word) - (MIN_COMPR_WORD_LEN-1)
	   if len==0:
	       char extralen;           (length of word) -
	   char[idxlen+1];		index (low,high)

       tag == 64 -> end of dictionary compressed block (if not coded in last uncompressed block)
*/

GB_INLINE int INDEX_DICT_OFFSET(int idx, GB_DICTIONARY *dict) {
    ad_assert(idx<dict->words);
    return ntohl(dict->offsets[idx]);
}
GB_INLINE int ALPHA_DICT_OFFSET(int idx, GB_DICTIONARY *dict) {
    int realIndex;
    ad_assert(idx<dict->words);
    realIndex = ntohl(dict->resort[idx]);
    return INDEX_DICT_OFFSET(realIndex, dict);
}

/* #define ALPHA_DICT_OFFSET(i) 	ntohl(offset[ntohl(resort[i])]) */
/* #define INDEX_DICT_OFFSET(i) 	ntohl(offset[i]) */

#define LEN_BITS             	4
#define INDEX_BITS		        2
#define INDEX_LEN_BITS		    1

#define LEN_SHIFT           	0
#define INDEX_SHIFT         	(LEN_SHIFT+LEN_BITS)
#define INDEX_LEN_SHIFT		    (INDEX_SHIFT+INDEX_BITS)

#define BITMASK(bits)		    ((1<<(bits))-1)
#define GETVAL(tag,typ)		    (((tag)>>typ##_SHIFT)&BITMASK(typ##_BITS))

#define MIN_SHORTLEN            6
#define MAX_SHORTLEN		    (BITMASK(LEN_BITS)+MIN_SHORTLEN-1)
#define MIN_LONGLEN             (MAX_SHORTLEN+1)
#define MAX_LONGLEN		        (MIN_LONGLEN+255)

#define SHORTLEN_DECR		    (MIN_SHORTLEN-1) /* !! zero is used as flag for long len !! */
#define LONGLEN_DECR    	    MIN_LONGLEN

#define MIN_COMPR_WORD_LEN 	    MIN_SHORTLEN
#define MAX_COMPR_WORD_LEN   	MAX_LONGLEN

#define MAX_SHORT_INDEX 	    BITMASK(INDEX_BITS+8)
#define MAX_LONG_INDEX          BITMASK(INDEX_BITS+16)

#define LAST_COMPRESSED_BIT     64

#ifdef DEBUG
# define DUMP_COMPRESSION_TEST 	0
/*	0 = only compression ratio
	1 = + original/compressed/decompressed
	2 = + words used to compress/uncompress
	3 = + matching words in dictionary
	4 = + search of words in dictionary
 */
#else
# define DUMP_COMPRESSION_TEST 	0
#endif

#ifdef DEBUG
#define COUNT_CHUNKS
long uncompressedBlocks[64];
long compressedBlocks[MAX_LONGLEN];


#if defined(TEST_DICT)
static void clearChunkCounters(void) {
    int i;

    for (i=0; i<64; i++) uncompressedBlocks[i] = 0;
    for (i=0; i<MAX_LONGLEN; i++) compressedBlocks[i] = 0;
}

static void dumpChunkCounters(void) {
    int i;

    printf("------------------------------\n" "Uncompressed blocks used:\n");
    for (i=0; i<64; i++) if (uncompressedBlocks[i]) printf("  size=%i used=%li\n", i, uncompressedBlocks[i]);
    printf("------------------------------\n" "Words used:\n");
    for (i=0; i<MAX_LONGLEN; i++) if (compressedBlocks[i]) printf("  size=%i used=%li\n", i, compressedBlocks[i]);
    printf("------------------------------\n");
}
#endif /* TEST_DICT */

static cu_str lstr(cu_str s, int len) {
#define BUFLEN 10000
    static unsigned_char buf[BUFLEN];

    ad_assert(len<BUFLEN);
    memcpy(buf,s,len);
    buf[len] = 0;

    return buf;
}

#if DUMP_COMPRESSION_TEST>=2

static cu_str dict_word(GB_DICTIONARY *dict, int idx, int len) {
/*     GB_NINT *offset = dict->offsets; */
    return lstr(dict->text+INDEX_DICT_OFFSET(idx, dict), len);
}

#endif

#if DUMP_COMPRESSION_TEST>=1

static void dumpBinary(u_str data, long size) {
#define PER_LINE 12
    int cnt = 0;

    while (size--) {
        unsigned_char c = *data++;
        int bitval = 128;
        int bits = 8;

        while (bits--) {
            putchar(c&bitval?'1':'0');
            bitval>>=1;
        }
        putchar(' ');

        cnt = (cnt+1)%PER_LINE;
        if (!cnt) putchar('\n');
    }

    if (cnt) putchar('\n');
}

#endif

#endif

static GB_INLINE int GB_MEMCMP(const void *vm1, const void *vm2, long size) {
    char *c1 = (char*)vm1,
        *c2 = (char*)vm2;
    int diff = 0;

    while (size-- && !diff) diff = *c1++-*c2++;
    return diff;
}

/* -------------------------------------------------- */

static int searchWord(GB_DICTIONARY *dict, cu_str source, long size, unsigned long *wordIndex, int *wordLen)
{
    int idx = -1;
    int l = 0;
    int h = dict->words-1;
    cu_str text = dict->text;
/*     GB_NINT *offset = dict->offsets; */
    GB_NINT *resort = dict->resort;
    int dsize = dict->textlen;
    int ilen = 0;

    while (l<h-1) {
        int m = (l+h)/2;
        long off = ALPHA_DICT_OFFSET(m, dict);
        cu_str dictword = text+off;
        long msize = min(size,dsize-off);

#if DUMP_COMPRESSION_TEST>=4
        printf("  %s (%i)\n", lstr(dictword,20), m);
#endif

        if (GB_MEMCMP(source, dictword, msize)<=0) 	h = m;
        else 						                l = m;
    }

    while (l<=h) {
        int off = ALPHA_DICT_OFFSET(l, dict);
        cu_str word = text+off;
        int msize = (int)min(size, dsize-off);
        int equal = 0;
        cu_str s = source;

        while (msize-- && *s++==*word++) equal++;

#if DUMP_COMPRESSION_TEST>=3
        if (equal>=MIN_COMPR_WORD_LEN) {
            printf("  EQUAL=%i '%s' (%i->%i, off=%i)", equal, lstr(text+off,equal), l, ntohl(resort[l]), ALPHA_DICT_OFFSET(l, dict));
            printf(" (context=%s)\n", lstr(text+off-min(off,20),min(off,20)+equal+20));
        }
#endif

        if (equal>ilen) {
            ilen = equal;
            idx  = ntohl(resort[l]);
            ad_assert(idx<dict->words);
        }

        l++;
    }

    *wordIndex = idx;
    *wordLen = (int)min(ilen,MAX_COMPR_WORD_LEN);

    return idx!=-1 && ilen>=MIN_COMPR_WORD_LEN;
}

#ifdef DEBUG
int look(GB_DICTIONARY *dict, GB_CSTR source) {
    unsigned long wordIndex;
    int wordLen;
    int wordFound = searchWord(dict, (cu_str)source, strlen(source), &wordIndex, &wordLen);

    if (wordFound) {
        printf("'%s' (idx=%lu, off=%i)\n", lstr(dict->text+ntohl(dict->offsets[wordIndex]), wordLen), wordIndex, ntohl(dict->offsets[wordIndex]));
    }

    return wordFound;
}
#endif



char *gb_uncompress_by_dictionary_internal(GB_DICTIONARY *dict, /*GBDATA *gbd, */GB_CSTR s_source, long size, GB_BOOL append_zero) {
    cu_str source = (cu_str)s_source;
    u_str dest;
    u_str buffer;
    cu_str text = dict->text;
/*     GB_NINT *offset = dict->offsets; */
    int done = 0;

    dest = buffer = (u_str)GB_give_other_buffer(s_source,size+2);

    while (size && !done) {
        register int c;

        if ((c=*source++)&128)	/* compressed data */ {
            int indexLen = GETVAL(c,INDEX_LEN);
            unsigned long idx = GETVAL(c,INDEX);

            c = GETVAL(c,LEN);	/* ==wordLen */
            if (c) 	c += SHORTLEN_DECR;
            else	c = *source+++LONGLEN_DECR;

            ad_assert(indexLen>=0 && indexLen<=1);

            if (indexLen==0) {
                idx = (idx <<8) | *source++;
            }
            else {
                idx = (((idx << 8) | source[1]) << 8) | source[0];
                source += 2;
            }

            ad_assert(idx<(GB_ULONG)dict->words);

            {
                cu_str word = text+INDEX_DICT_OFFSET(idx, dict);

#if DUMP_COMPRESSION_TEST>=2
                printf("  word='%s' (idx=%lu, off=%li, len=%i)\n",
                       lstr(word, c), idx, (long)ntohl(dict->offsets[idx]), c);
#endif

                {
                    register u_str d = dest;
                    while (c--) *d++ = *word++;
                    dest = d;
                }
            }
        }
        else {			/* uncompressed bytes */
            if (c & LAST_COMPRESSED_BIT) {
                done = 1;
                c ^= LAST_COMPRESSED_BIT;
            }

            size -= c;
            {
                u_str d = dest;
                while (c--) *d++ = *source++;
                dest=d;
            }
        }
    }

    if (append_zero == GB_TRUE) *dest = 0;
    /* if (GB_TYPE(gbd)==GB_STRING || GB_TYPE(gbd) == GB_LINK) *dest = 0; */

    return (char *)buffer;
}

char *gb_uncompress_by_dictionary(GBDATA *gbd, GB_CSTR s_source, long size)
{
    GB_DICTIONARY *dict        = gb_get_dictionary(GB_MAIN(gbd), GB_KEY_QUARK(gbd));
    GB_BOOL        append_zero = GB_TYPE(gbd)==GB_STRING || GB_TYPE(gbd) == GB_LINK;

    if (!dict) {
        fprintf(stderr, "Cannot decompress db-entry '%s' (no dictionary found)\n", GB_get_db_path(gbd));
        if (GB_KEY_QUARK(gbd) == 0) { // illegal key
            static char corrupt[] = "<data corrupted>";
            return corrupt;
        }
        GB_CORE;
    }

    return gb_uncompress_by_dictionary_internal(dict, s_source, size, append_zero);
}

char *gb_compress_by_dictionary(GB_DICTIONARY *dict, GB_CSTR s_source, long size, long *msize, int last_flag, int search_backward, int search_forward)
{
    cu_str source           = (cu_str)s_source;
    u_str  dest;
    u_str  buffer;
    cu_str unknown          = source; /* start of uncompressable bytes */
    u_str  lastUncompressed = NULL; /* ptr to start of last block of uncompressable bytes (in dest) */
#if defined(ASSERTION_USED)
    long   org_size         = size;
#endif /* ASSERTION_USED */

    ad_assert(size>0); /* compression of zero-length data fails! */

    dest = buffer = (u_str)GB_give_other_buffer((GB_CSTR)source, 1+(size/63+1)+size);
    *dest++ = GB_COMPRESSION_DICTIONARY | last_flag;

    while (size) {
        unsigned long wordIndex;
        int wordLen;
        int wordFound;

        if ( (wordFound = searchWord(dict, source, size, &wordIndex, &wordLen)) ) {
            int length;

        takeRest:
            length = source-unknown;

            if (length) {
                int shift;
                int takeShift = 0;
                int maxShift = (int)min(search_forward, wordLen-1);

                for (shift=1; shift<=maxShift; shift++) {
                    unsigned long wordIndex2;
                    int wordLen2;
                    int wordFound2;

                    if ( (wordFound2 = searchWord(dict, source+shift, size-shift, &wordIndex2, &wordLen2))) {
                        if (wordLen2>(wordLen+shift)) {
                            wordIndex = wordIndex2;
                            wordLen = wordLen2;
                            takeShift = shift;
                        }
                    }
                }

                if (takeShift) {
                    source += takeShift;
                    size -= takeShift;
                    length = source-unknown;
                }
            }

            while (length) {	/* if there were uncompressable bytes */
                int take = (int)min(length,63);

#ifdef COUNT_CHUNKS
                uncompressedBlocks[take]++;
#endif

                lastUncompressed = dest;

                *dest++ = take;	/* tag byte */
                memcpy(dest, unknown, take);
                dest += take;
                unknown += take;
                length -= take;
            }

            ad_assert(unknown==source);

            while (wordFound) {	/* as long as we find words in dictionary */
                int           indexLen      = wordIndex>MAX_SHORT_INDEX;
                int           indexHighBits = indexLen==0 ? wordIndex>>8 : wordIndex>>16;
                int           nextWordFound;
                int           nextWordLen;
                unsigned long nextWordIndex;

                ad_assert((long)wordIndex<dict->words);
                ad_assert((long)wordIndex <= MAX_LONG_INDEX);
                ad_assert(indexHighBits==(indexHighBits & BITMASK(INDEX_BITS)));
                ad_assert(wordLen>=MIN_SHORTLEN);

                lastUncompressed = NULL;

                {
                    cu_str source2 = source+wordLen;
                    long size2 = size-wordLen;

                    if (!(nextWordFound=searchWord(dict, source+wordLen, size-wordLen, &nextWordIndex, &nextWordLen))) { /* no word right afterwards */
                        int shift;

                        for (shift=1; shift<=search_backward && shift<(wordLen-MIN_COMPR_WORD_LEN); shift++) {
                            /* try to cut end of word to get a better result */
                            unsigned long wordIndex2;
                            int wordLen2;
                            int wordFound2;

                            if ( (wordFound2=searchWord(dict, source2-shift, size2+shift, &wordIndex2, &wordLen2))) {
                                if (wordLen2>(shift+1)) {
                                    wordLen -= shift;

                                    nextWordFound = 1;
                                    nextWordIndex = wordIndex2;
                                    nextWordLen = wordLen2;
                                    break;
                                }
                            }
                        }
                    }
                }

#ifdef COUNT_CHUNKS
                compressedBlocks[wordLen]++;
#endif

#if DUMP_COMPRESSION_TEST>=2
                printf("  word='%s' (idx=%li, off=%i, len=%i)\n",
                       dict_word(dict, wordIndex, wordLen), wordIndex, (int)ntohl(dict->offsets[wordIndex]), wordLen);
#endif

                if (wordLen<=MAX_SHORTLEN) {
                    *dest++ = 128 |
                        (indexLen << INDEX_LEN_SHIFT) |
                        (indexHighBits << INDEX_SHIFT) |
                        ((wordLen-SHORTLEN_DECR) << LEN_SHIFT);
                }
                else {
                    *dest++ = 128 |
                        (indexLen << INDEX_LEN_SHIFT) |
                        (indexHighBits << INDEX_SHIFT);
                    *dest++ = wordLen-LONGLEN_DECR; /* extra length byte */
                }

                *dest++ = (char)wordIndex; /* low index byte */
                if (indexLen)
                    *dest++ = (char)(wordIndex >> 8); /* high index byte */

                unknown = source += wordLen;
                size -= wordLen;

                wordFound = nextWordFound;
                wordIndex = nextWordIndex;
                wordLen = nextWordLen;
            }
        }
        else {
            source++;
            if (--size==0) goto takeRest;
        }
    }

    if (lastUncompressed) 	*lastUncompressed |= LAST_COMPRESSED_BIT;
    else			*dest++ = LAST_COMPRESSED_BIT;

    *msize = dest-buffer;

#if defined(ASSERTION_USED)
    {
        char *test = gb_uncompress_by_dictionary_internal(dict, (GB_CSTR)buffer+1, org_size + GB_COMPRESSION_TAGS_SIZE_MAX, GB_TRUE);

        ad_assert(memcmp(test, s_source, org_size) == 0);
    }
#endif /* ASSERTION_USED */

    ad_assert(1);
    return (char*)buffer;
}


#if defined(TEST_DICT)

static void test_dictionary(GB_DICTIONARY *dict, O_gbdByKey *gbk, long *uncompSum, long *compSum)
{
    int cnt;
    long uncompressed_sum = 0;
    long compressed_sum = 0;
    long dict_size = (dict->words*2+1)*sizeof(GB_NINT)+dict->textlen;
    int i;
    long char_count[256];

    for (i=0; i<256; i++) char_count[i] = 0;

    printf("  Test compression..\n");

#ifdef COUNT_CHUNKS
    clearChunkCounters();
#endif

    for (cnt=0; cnt<gbk->cnt; cnt++) {
        GBDATA *gbd = gbk->gbds[cnt];
        int type = GB_TYPE(gbd);

        if (COMPRESSABLE(type)) {
            long size;
            cu_str data = get_data_n_size(gbd, &size);
            u_str copy;
            long compressedSize;
            int last_flag = 0;
            u_str compressed;
            u_str uncompressed;

            if (type==GB_STRING || type == GB_LINK) size--;

            if (size<1) continue;

#ifndef	NDEBUG
            copy = (u_str)gbm_get_mem(size, GBM_DICT_INDEX);
            ad_assert(copy!=0);
            memcpy(copy, data, size);
#endif

#if DUMP_COMPRESSION_TEST>=1
            printf("----------------------------\n");
            printf("original    : %3li b = '%s'\n", size, data);
#endif

            compressed = (u_str)gb_compress_by_dictionary(dict, (GB_CSTR)data, size, &compressedSize, last_flag, 9999, 2);

#if DUMP_COMPRESSION_TEST>=1
            printf("compressed  : %3li b = '%s'\n", compressedSize, lstr(compressed,compressedSize));
            dumpBinary(compressed, compressedSize);
#endif

            for (i=0; i<compressedSize; i++) char_count[compressed[i]]++;

            uncompressed = (u_str)gb_uncompress_by_dictionary(gbd, (char*)compressed+1, size);

#if DUMP_COMPRESSION_TEST>=1
            printf("copy        : %3li b = '%s'\n", size, lstr(copy,size));
            printf("decompressed: %3li b = '%s'\n", size, lstr(uncompressed,size));
#endif

            if (GB_MEMCMP(copy, uncompressed, size)!=0) {
                int byte = 0;

                while (copy[byte]==uncompressed[byte]) byte++;
                printf("Error in compression (off=%i, '%s'", byte, lstr(copy+byte,10));
                printf("!='%s'\n", lstr(uncompressed+byte,10));
            }

            if (compressedSize<size) {
                uncompressed_sum += size;
                compressed_sum += compressedSize;
            }
            else {
                uncompressed_sum += size;
                compressed_sum += size;
            }

            gbm_free_mem((char*)copy, size, GBM_DICT_INDEX);
        }
    }

#ifdef COUNT_CHUNKS
    dumpChunkCounters();
#endif

    /*
          printf("\nVerteilung der Characters:\n");
          for (i=0; i<256; i++)
          {
          if (char_count[i])
          printf("  char=%c (=%3i)  used=%4li (=%2li.%02li%%)\n",
          (char)i, i,
          char_count[i],
          (char_count[i]*100L)/compressed_sum,
          ((char_count[i]*10000L)/compressed_sum)%100
          );
          }
    */

    printf("    uncompressed size = %li b\n"
           "      compressed size = %li b\n"
           "    +%li (dict)     = %li b       (Ratio=%li%%)\n",
           uncompressed_sum, compressed_sum,
           dict_size, dict_size+compressed_sum,
           ((dict_size+compressed_sum)*100)/uncompressed_sum
           );

    *uncompSum += uncompressed_sum;
    *compSum += compressed_sum+dict_size;
}

#endif /* TEST_DICT */


/******************** Build dictionary *******************/

#ifdef DEBUG
#define TEST 		/* test trees? */

/* #define DUMP_EXPAND */
/*
  #define SELECT_WORDS
  #define SELECTED_WORDS "oropl"
*/

# ifdef SELECT_WORDS
static char *strnstr(char *s1, int len, char *s2) {
    char c = *s2;
    int len2 = strlen(s2);

    while (len-->=len2) {
        if (*s1==c) {
            if (strncmp(s1,s2,len2)==0) return s1;
        }
        s1++;
    }

    return NULL;
}
# endif

static void dump_dtree(int deep, GB_DICT_TREE tree)
{
    static unsigned_char buffer[1024];

    if (tree.full) {
        switch (tree.full->typ) {
            case FULL_NODE: {
                int idx;

                for (idx=0; idx<256; idx++) {
                    buffer[deep] = idx;
                    buffer[deep+1] = 0;

                    if (tree.full->son[idx].exists) dump_dtree(deep+1, tree.full->son[idx]);
                    else if (tree.full->count[idx]>0) printf(" '%s' (%i) [array]\n", buffer, tree.full->count[idx]);
                }
                break;
            }
            case SINGLE_NODE: {
                buffer[deep] = tree.single->ch;
                buffer[deep+1] = 0;

                if (tree.single->son.exists) dump_dtree(deep+1, tree.single->son);
                else printf(" '%s' (%i) [single]\n", buffer, tree.single->count);

                if (tree.single->brother.exists) dump_dtree(deep, tree.single->brother);
                break;
            }
        }
    }
}

#else
# define dump_dtree(deep,tree)
#endif

#ifdef TEST
static int testCounts(GB_DICT_TREE tree)
     /*
 * tests if all inner nodes have correct 'count's
 */
{
    int cnt = 0;

    if (tree.exists) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                while (tree.exists) {
                    if (tree.single->son.exists) {
                        int son_cnt = testCounts(tree.single->son);
#ifdef COUNT_EQUAL
                        ad_assert(son_cnt==tree.single->count);
#else
                        ad_assert(son_cnt<=tree.single->count);
#endif
                    }

                    ad_assert(tree.single->count>0);
                    cnt += tree.single->count;
                    tree = tree.single->brother;
                }
                break;
            }
            case FULL_NODE: {
                int idx,
                    sons = 0;

                for (idx=0; idx<256; idx++) {
                    if (tree.full->son[idx].exists) {
                        int son_cnt = testCounts(tree.full->son[idx]);
#ifdef COUNT_EQUAL
                        ad_assert(son_cnt==tree.full->count[idx]);
#else
                        ad_assert(son_cnt<=tree.full->count[idx]);
#endif
                        if (tree.full->usedSons)    ad_assert(tree.full->count[idx]>0);
                        else                        ad_assert(tree.full->count[idx]==0);

                        sons++;
                    }
                    else if (tree.full->count[idx]) {
                        sons++;
                    }

                    cnt += tree.full->count[idx];
                }

                ad_assert(sons==tree.full->usedSons);
                break;
            }
        }
    }

    return cnt;
}

/* #define TEST_MAX_OCCUR_COUNT */

#ifdef TEST_MAX_OCCUR_COUNT
#define MAX_OCCUR_COUNT 600000
#endif

static GB_DICT_TREE test_dtree(GB_DICT_TREE tree)
     /* only correct while tree is under contruction (build_dict_tree()) */
{
    if (tree.exists) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
#if defined(TEST_MAX_OCCUR_COUNT)
                ad_assert(tree.single->count<MAX_OCCUR_COUNT);	/* eher unwahrscheinlich */
#endif // TEST_MAX_OCCUR_COUNT

                if (tree.single->son.exists) {
                    ad_assert(tree.single->count==0);
                    test_dtree(tree.single->son);
                }
                else {
                    ad_assert(tree.single->count>0);
                }

                if (tree.single->brother.exists) test_dtree(tree.single->brother);
                break;
            }
            case FULL_NODE: {
                int idx;
                int countSons = 0;

                for (idx=0; idx<256; idx++) {
#if defined(TEST_MAX_OCCUR_COUNT)
                    ad_assert(tree.full->count[idx]<MAX_OCCUR_COUNT); /* eher unwahrscheinlich */
#endif // TEST_MAX_OCCUR_COUNT

                    if (tree.full->son[idx].exists) {
                        ad_assert(tree.full->count[idx]==0);
                        test_dtree(tree.full->son[idx]);
                        countSons++;
                    }
                    else {
                        ad_assert(tree.full->count[idx]>=0);
                        if (tree.full->count[idx]>0)
                            countSons++;
                    }
                }

                ad_assert(countSons==tree.full->usedSons);

                break;
            }
        }
    }

    return tree;
}

#else
# define test_dtree(tree)	/* (tree) */
# define testCounts(tree)	/* 0 */
#endif


static GB_DICT_TREE new_dtree(cu_str text, long len, long *memcount)
     /* creates a new (sub-)tree from 'text' (which has length 'len') */
{
    GB_DICT_TREE tree;

    if (len) {
        GB_SINGLE_DICT_TREE *tail = NULL,
            *head = NULL;

        while (len) {
            if (tail) 	tail = tail->son.single = (GB_SINGLE_DICT_TREE*)gbm_get_mem(sizeof(*tail),GBM_DICT_INDEX);
            else 	tail = head = (GB_SINGLE_DICT_TREE*)gbm_get_mem(sizeof(*tail),GBM_DICT_INDEX);

            (*memcount) += sizeof(*tail);

            tail->typ = SINGLE_NODE;
            tail->ch = *text++;
            len--;

            tail->brother.single = NULL;
            tail->son.single 	 = NULL;
        }

        tail->count = 1;
        tree.single = head;
    }
    else {
        tree.single = NULL;
    }

    return tree;
}

GB_DICT_TREE single2full_dtree(GB_DICT_TREE tree, long *memcount)
{
    if (tree.exists && tree.single->typ==SINGLE_NODE) {
        GB_FULL_DICT_TREE *full = (GB_FULL_DICT_TREE*)gbm_get_mem(sizeof(*full), GBM_DICT_INDEX);
        int idx;

        (*memcount) += sizeof(*full);
        full->typ = FULL_NODE;
        full->usedSons = 0;

        for (idx=0; idx<256; idx++) {
            full->son[idx].exists = NULL;
            full->count[idx] = 0;
        }

        while (tree.exists) {
            GB_SINGLE_DICT_TREE *t = tree.single;

            ad_assert(t->typ==SINGLE_NODE);
            ad_assert(full->son[t->ch].exists==NULL);

            full->son[t->ch] = t->son;
            full->count[t->ch] = t->count;
            full->usedSons++;

            tree.single = t->brother.single;

            gbm_free_mem((char*)t, sizeof(*t), GBM_DICT_INDEX);
            (*memcount) -= sizeof(*t);
        }

        tree.full = full;
    }

    return tree;
}

static void free_dtree(GB_DICT_TREE tree)
{
    if (tree.exists) {
        switch(tree.full->typ) {
            case SINGLE_NODE: {
                if (tree.single->son.exists) free_dtree(tree.single->son);
                if (tree.single->brother.exists) free_dtree(tree.single->brother);

                gbm_free_mem((char*)tree.single, sizeof(*(tree.single)), GBM_DICT_INDEX);
                break;
            }
            case FULL_NODE: {
                int idx;

                for (idx=0; idx<256; idx++) if (tree.full->son[idx].exists) free_dtree(tree.full->son[idx]);
                gbm_free_mem((char*)tree.full, sizeof(*(tree.full)), GBM_DICT_INDEX);
                break;
            }
        }
    }
}



static GB_DICT_TREE cut_dtree(GB_DICT_TREE tree, int cut_count, long *memcount, long *leafcount)
     /* removes all branches from 'tree' which are referenced less/equal than cut_count
      * returns: the reduced tree */
{
    if (tree.exists) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                if (tree.single->son.exists) tree.single->son = cut_dtree(tree.single->son, cut_count, memcount, leafcount);

                if (!tree.single->son.exists) { /* leaf */
                    if (tree.single->count<=cut_count) { /* leaf with less/equal references */
                        GB_DICT_TREE brother = tree.single->brother;

                        gbm_free_mem((char*)tree.single, sizeof(*tree.single), GBM_DICT_INDEX);
                        (*memcount) -= sizeof(*tree.single);
                        if (brother.exists) return cut_dtree(brother, cut_count, memcount, leafcount);

                        tree.single = NULL;
                        break;
                    }
                    else {
                        (*leafcount)++;
                    }
                }

                if (tree.single->brother.exists) tree.single->brother = cut_dtree(tree.single->brother, cut_count, memcount, leafcount);
                break;
            }
            case FULL_NODE: {
                int idx;
                int count = 0;

                for (idx=0; idx<256; idx++) {
                    if (tree.full->son[idx].exists) {
                        tree.full->son[idx] = cut_dtree(tree.full->son[idx], cut_count, memcount, leafcount);

                        if (tree.full->son[idx].exists) 	count++;
                        else 					            tree.full->count[idx] = 0;
                    }
                    else if (tree.full->count[idx]>0) {
                        if (tree.full->count[idx]<=cut_count) {
                            tree.full->count[idx] = 0;
                        }
                        else {
                            count++;
                            (*leafcount)++;
                        }
                    }
                }

                tree.full->usedSons = count;

                if (!count)	{	/* no more sons */
                    gbm_free_mem((char*)tree.full, sizeof(*(tree.full)), GBM_DICT_INDEX);
                    (*memcount) -= sizeof(*(tree.full));
                    tree.exists = NULL;
                }

                break;
            }
        }
    }

    return tree;
}
static GB_DICT_TREE cut_useless_words(GB_DICT_TREE tree, int deep, long *removed)
     /* removes/shortens all branches of 'tree' which are not useful for compression
 * 'deep' should be zero (incremented by cut_useless_words)
 * 'removed' will be set to the # of removed occurances
 * returns: the reduced tree */
{
    *removed = 0;

    if (tree.exists) {
        deep++;

        switch (tree.full->typ) {
            long removed_single;

            case SINGLE_NODE: {
                if (tree.single->son.exists) {
                    tree.single->son = cut_useless_words(tree.single->son, deep, &removed_single);
                    tree.single->count -= removed_single;
                    *removed += removed_single;
                }

                if (!tree.single->son.exists && !WORD_HELPFUL(deep,tree.single->count)) {
                    GB_DICT_TREE brother = tree.single->brother;

                    *removed += tree.single->count;
                    gbm_free_mem((char*)tree.single, sizeof(*tree.single), GBM_DICT_INDEX);

                    if (brother.exists) {
                        tree = cut_useless_words(brother, deep-1, &removed_single);
                        *removed += removed_single;
                    }
                    else {
                        tree.exists = NULL;
                    }

                    break;
                }

                if (tree.single->brother.exists) {
                    tree.single->brother = cut_useless_words(tree.single->brother, deep-1, &removed_single);
                    *removed += removed_single;
                }

                break;
            }
            case FULL_NODE: {
                int idx;
                int count = 0;

                for (idx=0; idx<256; idx++) {
                    if (tree.full->son[idx].exists) {
                        tree.full->son[idx] = cut_useless_words(tree.full->son[idx], deep, &removed_single);
                        tree.full->count[idx] -= removed_single;
                        *removed += removed_single;
                    }

                    if (tree.full->son[idx].exists) {
                        count++;
                    }
                    else if (tree.full->count[idx]) {
                        if (!WORD_HELPFUL(deep, tree.full->count[idx]))	{	/* useless! */
                            *removed += tree.full->count[idx];
                            tree.full->count[idx] = 0;
                        }
                        else {
                            count++;
                        }
                    }
                }

                tree.full->usedSons = count;

                if (!count)	{	/* no more sons */
                    gbm_free_mem((char*)tree.full, sizeof(*(tree.full)), GBM_DICT_INDEX);
                    tree.exists = NULL;
                }

                break;
            }
        }
    }

    return tree;
}

static GB_DICT_TREE add_dtree_to_dtree(GB_DICT_TREE toAdd, GB_DICT_TREE to, long *memcount)
     /* adds 'toAdd' as brother of 'to' (must be leftmost of all SINGLE_NODEs or a FULL_NODE)
 * returns: the leftmost of all SINGLE_NODEs or a FULL_NODE */
{
    GB_DICT_TREE tree = toAdd;

    ad_assert(toAdd.single->typ==SINGLE_NODE);

    if (to.exists) {
        switch (to.full->typ) {
            case SINGLE_NODE: {
                GB_SINGLE_DICT_TREE *left = to.single;

                ad_assert(left!=0);

                if (toAdd.single->ch < to.single->ch) {
                    toAdd.single->brother = to;
                    return toAdd;
                }

                while (to.single->brother.exists) {
                    if (toAdd.single->ch < to.single->brother.single->ch) {
                        toAdd.single->brother = to.single->brother;
                        to.single->brother = toAdd;

                        tree.single = left;
                        return tree;
                    }
                    to = to.single->brother;
                }

                to.single->brother = toAdd;
                tree.single = left;
                break;
            }
            case FULL_NODE: {
                unsigned_char ch = toAdd.single->ch;

                ad_assert(to.full->son[ch].exists==NULL);
                ad_assert(to.full->count[ch]==0); /* if this fails, count must be added & tested */
                ad_assert(toAdd.single->brother.exists==NULL);

                to.full->son[ch] = toAdd.single->son;
                to.full->count[ch] = toAdd.single->count;
                to.full->usedSons++;

                tree = to;

                gbm_free_mem((char*)toAdd.single, sizeof(*(toAdd.single)), GBM_DICT_INDEX);
                (*memcount) -= sizeof(toAdd.single);

                break;
            }
        }
    }

    return tree;
}

static GB_DICT_TREE add_to_dtree(GB_DICT_TREE tree, cu_str text, long len, long *memcount)
     /* adds the string 'text' (which has length 'len') to 'tree'
 * returns: new tree */
{
    if (tree.exists) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                GB_SINGLE_DICT_TREE *t = tree.single;
                int count = 0;

                do {
                    count++;
                    if (t->ch==text[0]) {				/* we found an existing subtree */
                        if (len>1) {
                            t->son = add_to_dtree(t->son, text+1, len-1, memcount);	/* add rest of text to subtree */
                        }
                        else {
                            ad_assert(len==1);
                            ad_assert(t->son.exists==NULL);
                            t->count++;
                        }

                        return count>MAX_BROTHERS ? single2full_dtree(tree, memcount) : tree;
                    }
                    else if (t->ch > text[0]) {
                        break;
                    }
                }
                while ((t=t->brother.single)!=NULL);

                tree = add_dtree_to_dtree(new_dtree(text,len,memcount),			/* otherwise we create a new subtree */
                                          count>MAX_BROTHERS ? single2full_dtree(tree, memcount) : tree,
                                          memcount);
                break;
            }
            case FULL_NODE: {
                unsigned_char ch = text[0];

                if (tree.full->son[ch].exists) {
                    tree.full->son[ch] = add_to_dtree(tree.full->son[ch], text+1, len-1, memcount);
                }
                else {
                    tree.full->son[ch] = new_dtree(text+1, len-1, memcount);
                    if (!tree.full->son[ch].exists) {
                        if (tree.full->count[ch]==0) tree.full->usedSons++;
                        tree.full->count[ch]++;
                    }
                    else {
                        tree.full->usedSons++;
                    }
                }
                break;
            }
        }

        return tree;
    }

    return new_dtree(text,len,memcount);
}

static long calcCounts(GB_DICT_TREE tree)
{
    long cnt = 0;

    ad_assert(tree.exists!=0);

    switch (tree.full->typ) {
        case SINGLE_NODE: {
            while (tree.exists) {
                if (tree.single->son.exists) tree.single->count = calcCounts(tree.single->son);
                ad_assert(tree.single->count>0);
                cnt += tree.single->count;
                tree = tree.single->brother;
            }
            break;
        }
        case FULL_NODE: {
            int idx;

            for (idx=0; idx<256; idx++) {
                if (tree.full->son[idx].exists) {
                    tree.full->count[idx] = calcCounts(tree.full->son[idx]);
                    ad_assert(tree.full->count[idx]>0);
                }
                else {
                    ad_assert(tree.full->count[idx]>=0);
                }
                cnt += tree.full->count[idx];
            }
            break;
        }
    }

    return cnt;
}

static int count_dtree_leafs(GB_DICT_TREE tree, int deep, int *maxdeep) {
    /* returns # of leafs and max. depth of tree */
    int leafs = 0;

    ad_assert(tree.exists!=0);

    if (++deep>*maxdeep) *maxdeep = deep;

    switch(tree.full->typ) {
        case SINGLE_NODE: {
            if (tree.single->son.exists) 	leafs += count_dtree_leafs(tree.single->son, deep, maxdeep);
            else				            leafs++;
            if (tree.single->brother.exists) 	leafs += count_dtree_leafs(tree.single->brother, deep, maxdeep);
            break;
        }
        case FULL_NODE: {
            int idx;

            for (idx=0; idx<256; idx++) {
                if (tree.full->son[idx].exists) 	leafs += count_dtree_leafs(tree.full->son[idx], deep, maxdeep);
                else if (tree.full->count[idx])		leafs++;
            }
            break;
        }
    }

    return leafs;
}

static int COUNT(GB_DICT_TREE tree) {
    /* counts sum of # of occurencies of tree */
    int cnt = 0;

    switch(tree.single->typ) {
        case SINGLE_NODE: {
            while (tree.exists) {
                cnt += tree.single->count;
                tree = tree.single->brother;
            }
            break;
        }
        case FULL_NODE: {
            int idx;

            for (idx=0; idx<256; idx++) cnt += tree.full->count[idx];
            break;
        }
    }

    return cnt;
}

static GB_DICT_TREE removeSubsequentString(GB_DICT_TREE *tree_pntr, cu_str buffer, int len, int max_occur) {
    /*
     * searches tree for 'buffer' (length='len')
     *
     * returns   	- rest below found string
     * 		  (if found and if the # of occurances of the string is less/equal than 'max_occur')
     *		- NULL otherwise
     *
     * removes the whole found string from the tree (not only the rest!)
     */
    GB_DICT_TREE tree = *tree_pntr, rest;
    static int restCount;

    rest.exists = NULL;

    ad_assert(tree.exists!=0);
    ad_assert(len>0);

    switch (tree.full->typ) {
        case SINGLE_NODE: {
            while (tree.single->ch <= buffer[0]) {
                if (tree.single->ch == buffer[0]) {	/* found wanted character */
                    if (tree.single->son.exists) {
                        if (len==1) {
                            if (tree.single->count <= max_occur) {
                                rest = tree.single->son;
                                restCount = COUNT(rest);
                                tree.single->son.exists = NULL;
                            }
                        }
                        else {
                            rest = removeSubsequentString(&tree.single->son, buffer+1, len-1, max_occur);
                        }
                    }

                    if (rest.exists) { /* the string was found */
                        tree.single->count -= restCount;
                        ad_assert(tree.single->count >= 0);

                        if (!tree.single->count) { /* empty subtree -> delete myself */
                            GB_DICT_TREE brother = tree.single->brother;

                            tree.single->brother.exists = NULL; /* elsewise it would be free'ed by free_dtree */
                            free_dtree(tree);
                            *tree_pntr = tree = brother;
                        }
                    }

                    break;
                }

                tree_pntr = &(tree.single->brother);
                if (!(tree = tree.single->brother).exists) break;
            }

            break;
        }
        case FULL_NODE: {
            unsigned_char ch;

            if (tree.full->son[ch=buffer[0]].exists) {
                if (len==1) {
                    if (tree.full->count[ch] <= max_occur) {
                        rest = tree.full->son[ch];
                        restCount = COUNT(rest);
                        tree.full->son[ch].exists = NULL;
                    }
                }
                else {
                    rest = removeSubsequentString(&tree.full->son[ch], buffer+1, len-1, max_occur);
                }

                if (rest.exists) {
                    ad_assert(restCount>0);
                    tree.full->count[ch] -= restCount;
                    ad_assert(tree.full->count[ch]>=0);
                    if (tree.full->count[ch]==0) {
                        ad_assert(tree.full->son[ch].exists==NULL);

                        if (--tree.full->usedSons==0) { /* last son deleted -> delete myself */
                            free_dtree(tree);
                            tree.exists = NULL;
                            *tree_pntr = tree;
                        }
                    }
                }
            }

            break;
        }
    }

    return rest;
}

static cu_str memstr(cu_str stringStart, int stringStartLen, cu_str inString, int inStringLen) {
    if (!inStringLen) return stringStart; /* string of length zero is found everywhere */

    while (stringStartLen) {
        cu_str found = (cu_str)memchr(stringStart, inString[0], stringStartLen);

        if (!found) break;

        stringStartLen -= found-stringStart;
        stringStart = found;

        if (stringStartLen<inStringLen) break;

        if (GB_MEMCMP(stringStart,inString,inStringLen)==0) return stringStart;

        stringStart++;
        stringStartLen--;
    }

    return NULL;
}


static int expandBranches(u_str buffer, int deep, int minwordlen, int maxdeep, GB_DICT_TREE tree, GB_DICT_TREE root, int max_percent) {
    /*
     * expands all branches in 'tree'
     *
     * this is done by searching every of these branches in 'root' and moving any subsequent parts from there to 'tree'
     * (this is only done, if the # of occurances of the found part does not exceed the # of occurances of 'tree' more than 'max_percent' percent)
     *
     *	'buffer'	strings are rebuild here while descending the tree (length of buffer==MAX_WORD_LEN)
     *	'deep'		recursion level
     *	'maxdeep'	maximum recursion level
     * 	'minwordlen'	is the length of the words to search (usually equal to MIN_WORD_LEN-1)
     *
     * returns the # of occurances which were added to 'tree'
     */
    int expand = 0;		/* calculate count-sum of added subsequent parts */

    ad_assert(tree.exists!=0);

    if (deep<maxdeep) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                while (tree.exists) {
                    buffer[deep] = tree.single->ch;

                    if (!tree.single->son.exists) {
                        GB_DICT_TREE rest;
                        u_str buf = buffer+1;
                        int len = deep;

                        if (len>minwordlen)	{	/* do not search more than MIN_WORD_LEN-1 chars */
                            buf += len-minwordlen;
                            len = minwordlen;
                        }

                        if (len==minwordlen) {
                            cu_str self = memstr(buffer, deep+1, buf, len);

                            ad_assert(self!=0);
                            if (self==buf)  rest = removeSubsequentString(&root, buf, len, ((100+max_percent)*tree.single->count)/100);
                            else            rest.exists = NULL;
                        }
                        else {
                            rest.exists = NULL;
                        }

                        if (rest.exists) {
                            int cnt = COUNT(rest);

                            tree.single->son = rest;
                            tree.single->count += cnt;
                            expand += cnt;
#ifdef DUMP_EXPAND
#define DUMP_MORE 1
                            printf("expanding '%s'", lstr(buffer, deep+1+DUMP_MORE));
                            printf(" (searching for '%s') -> found %i nodes\n", lstr(buf, len+DUMP_MORE), cnt);
#endif
                        }
                    }

                    if (tree.single->son.exists) {
                        int added = expandBranches(buffer, deep+1, minwordlen, maxdeep, tree.single->son, root, max_percent);

                        expand += added;
                        tree.single->count += added;
                    }

                    tree = tree.single->brother;
                }

                break;
            }
            case FULL_NODE: {
                int idx;

                for (idx=0; idx<256; idx++) {
                    buffer[deep] = idx;

                    if (!tree.full->son[idx].exists && tree.full->count[idx])	/* leaf */ {
                        GB_DICT_TREE rest;
                        u_str buf = buffer+1;
                        int len = deep;

                        if (len>minwordlen)	{ /* do not search more than MIN_WORD_LEN-1 chars */
                            buf += len-minwordlen;
                            len = minwordlen;
                        }

                        if (len==minwordlen) {
                            cu_str self = memstr(buffer, deep+1, buf, len);

                            ad_assert(self!=0);
                            if (self==buf)
                                rest = removeSubsequentString(&root, buf, len, ((100+max_percent)*tree.full->count[idx])/100);
                            else
                                rest.exists = NULL;
                        }
                        else {
                            rest.exists = NULL;
                        }

                        if (rest.exists) {	/* substring found! */
                            int cnt = COUNT(rest);

                            if (tree.full->count[idx]==0) tree.full->usedSons++;
                            tree.full->son[idx] = rest;
                            tree.full->count[idx] += cnt;

                            expand += cnt;
#ifdef DUMP_EXPAND
                            printf("expanding '%s'", lstr(buffer, deep+1+DUMP_MORE));
                            printf(" (searching for '%s') -> found %i nodes\n", lstr(buf, len+DUMP_MORE), cnt);
#endif
                        }
                    }

                    if (tree.full->son[idx].exists) {
                        int added = expandBranches(buffer, deep+1, minwordlen, maxdeep, tree.full->son[idx], root, max_percent);

                        expand += added;
                        tree.full->count[idx] += added;
                    }
                }

                break;
            }
        }
    }

    return expand;
}

static GB_DICT_TREE build_dict_tree(O_gbdByKey *gbk, long maxmem, long maxdeep, long minwordlen, long *data_sum)
     /* builds a tree of the most used words
 *
 * 'maxmem' is the amount of memory that will be allocated
 * 'maxdeep' is the maximum length of the _returned_ words
 * 'minwordlen' is the minimum length a word needs to get into the tree
 *		this is used in the first pass as maximum tree depth
 * 'data_sum' will be set to the overall-size of data of which the tree was built
 */
{
    GB_DICT_TREE tree;
    long memcount = 0;
    long leafs = 0;

    *data_sum = 0;

    {
        int cnt;
        long lowmem = (maxmem*9)/10;
        int cut_count = 1;

        /* Build 8-level-deep tree of all existing words */

        tree.exists = NULL;		/* empty tree */

        for (cnt=0; cnt<gbk->cnt; cnt++) {
            GBDATA *gbd = gbk->gbds[cnt];
            int type =  GB_TYPE(gbd);

            if (COMPRESSABLE(type)) {
                long size;
                u_str data = get_data_n_size(gbd, &size);
                u_str lastWord;

                if (type==GB_STRING || type == GB_LINK) size--;
                if (size<minwordlen) continue;

                *data_sum += size;
                lastWord = data+size-minwordlen;

#ifdef SELECT_WORDS
                if (strnstr(data,size,SELECTED_WORDS)) /* test some words only */
#endif
                    {

                    for ( ; data<=lastWord; data++) {
                        tree = add_to_dtree(tree, data, minwordlen, &memcount);

                        while (memcount>maxmem) {
                            leafs = 0;
                            tree  = cut_dtree(tree, cut_count, &memcount, &leafs);
                            if (memcount<=lowmem) break;
                            cut_count++;
                        }
                    }
                }
            }
        }
    }

    /*test_dtree(tree); */
    {
        int cutoff = 1;

        leafs = 0;
        tree  = cut_dtree(tree, cutoff, &memcount, &leafs); /* cut all single elements */
        test_dtree(tree);

#if defined(DEBUG)
        if (tree.exists) {
            int  maxdeep2 = 0;
            long counted  = count_dtree_leafs(tree, 0, &maxdeep2);
            ad_assert(leafs == counted);
        }
#endif // DEBUG

        /* avoid directory overflow (max. 18bit) */
        while (leafs >= MAX_LONG_INDEX) {
            leafs = 0;
            ++cutoff;
#if defined(DEBUG)
            printf("Directory overflow (%li) -- reducing size (cutoff = %i)\n", leafs, cutoff);
#endif // DEBUG
            tree  = cut_dtree(tree, cutoff, &memcount, &leafs);
        }
    }
#if 0
    printf("----------------------- tree with short branches:\n");
    dump_dtree(0,tree);
    printf("---------------------------\n");
#endif

    /* Try to create longer branches */

    if (tree.exists) {
        int add_count;
        u_str buffer = (u_str)gbm_get_mem(maxdeep, GBM_DICT_INDEX);
        int max_differ;
        long dummy;

        if (tree.full->typ != FULL_NODE) tree = single2full_dtree(tree,&memcount);	/* ensure root is FULL_NODE */

        test_dtree(tree);
        calcCounts(tree);	/* calculate counters of inner nodes */
        testCounts(tree);

        for (max_differ=0; max_differ<=MAX_DIFFER; max_differ+=INCR_DIFFER) { /* percent of allowed difference for concatenating tree branches */
            do {
                int idx;
                add_count = 0;

                for (idx=0; idx<256; idx++) {
                    if (tree.full->son[idx].exists) {
                        int added;

                        buffer[0] = idx;
                        added = expandBranches(buffer, 1, minwordlen-1, maxdeep, tree.full->son[idx], tree, max_differ);
                        tree.full->count[idx] += added;
                        add_count += added;
                    }
                }
            }
            while (add_count);
        }

        gbm_free_mem((char*)buffer, maxdeep, GBM_DICT_INDEX);

        tree = cut_useless_words(tree,0,&dummy);
    }

#if 0
    printf("----------------------- tree with expanded branches:\n");
    dump_dtree(0,tree);
    printf("-----------------------\n");
#endif
    testCounts(tree);

    return tree;
}

static GB_DICT_TREE remove_word_from_dtree(GB_DICT_TREE tree, cu_str wordStart, int wordLen, u_str resultBuffer, int *resultLen, long *resultFrequency, long *removed) {
    /* searches 'tree' for a word starting with 'wordStart' an removes it from the tree
     * if there are more than one possibilities, the returned word will be the one with the most occurances
     * if there was no possibility -> resultLen==0, tree unchanged
     * otherwise: resultBuffer contains the word, returns new tree with word removed
     */
    long removed_single = 0;
    ad_assert(tree.exists!=0);
    *removed = 0;

    if (wordLen) {			/* search wanted path into tree */
        switch(tree.full->typ) {
            case SINGLE_NODE: {
                if (tree.single->ch==*wordStart) {
                    *resultBuffer = *wordStart;

                    if (tree.single->son.exists) {
                        ad_assert(tree.single->count>0);
                        tree.single->son = remove_word_from_dtree(tree.single->son, wordStart+1, wordLen-1,
                                                                  resultBuffer+1, resultLen, resultFrequency,
                                                                  &removed_single);
                        if (*resultLen)	{ /* word removed */
                            ad_assert(tree.single->count>=removed_single);
                            tree.single->count -= removed_single;
                            *removed += removed_single;
                            (*resultLen)++;
                        }
                    }
                    else {
                        *resultLen = wordLen==1;	/* if wordLen==1 -> fully overlapping word found */
                        *resultFrequency = tree.single->count;
                    }

                    if (!tree.single->son.exists && *resultLen)	{	/* if no son and a word was found -> remove branch */
                        GB_DICT_TREE brother = tree.single->brother;

                        *removed += tree.single->count;
                        gbm_free_mem((char*)tree.single, sizeof(*tree.single), GBM_DICT_INDEX);

                        if (brother.exists) 	tree = brother;
                        else 			        tree.exists = NULL;
                    }
                }
                else if (tree.single->ch < *wordStart  &&  tree.single->brother.exists) {
                    tree.single->brother = remove_word_from_dtree(tree.single->brother, wordStart, wordLen,
                                                                  resultBuffer, resultLen, resultFrequency,
                                                                  &removed_single);
                    if (*resultLen) *removed += removed_single;
                }
                else {
                    *resultLen = 0; /* not found */
                }

                break;
            }
            case FULL_NODE: {
                unsigned_char ch = *wordStart;
                *resultBuffer = ch;

                if (tree.full->son[ch].exists) {
                    tree.full->son[ch] = remove_word_from_dtree(tree.full->son[ch], wordStart+1, wordLen-1,
                                                                resultBuffer+1, resultLen, resultFrequency,
                                                                &removed_single);
                    if (*resultLen) {
                        if (tree.full->son[ch].exists) { /* another son? */
                            tree.full->count[ch] -= removed_single;
                        }
                        else {	/* last son -> remove whole branch */
                            removed_single = tree.full->count[ch];
                            tree.full->count[ch] = 0;
                            tree.full->usedSons--;
                        }

                        *removed += removed_single;
                        (*resultLen)++;
                    }
                }
                else if (tree.full->count[ch]) {
                    *resultLen = (wordLen==1);

                    if (*resultLen) {
                        *removed += removed_single = *resultFrequency = tree.full->count[ch];
                        tree.full->count[ch] = 0;
                        tree.full->usedSons--;
                    }
                }
                else {
                    *resultLen = 0; /* not found */
                }

                if (!tree.full->usedSons) {
                    free_dtree(tree);
                    tree.exists = NULL;
                }

                break;
            }
        }
    }
    else {	/* take any word */
        switch(tree.full->typ) {
            case SINGLE_NODE: {
                *resultBuffer = tree.single->ch;
                ad_assert(tree.single->count>0);

                if (tree.single->son.exists) {
                    tree.single->son = remove_word_from_dtree(tree.single->son, wordStart, wordLen,
                                                              resultBuffer+1, resultLen, resultFrequency,
                                                              &removed_single);
                    ad_assert(*resultLen);
                    (*resultLen)++;
                }
                else {
                    *resultLen = 1;
                    *resultFrequency = tree.single->count;
                    removed_single = tree.single->count;
                }

                ad_assert(*resultFrequency>0);

                if (tree.single->son.exists) {
                    ad_assert(tree.single->count>=removed_single);
                    tree.single->count -= removed_single;
                    *removed += removed_single;
                }
                else {
                    GB_DICT_TREE brother = tree.single->brother;

                    *removed += tree.single->count;
                    gbm_free_mem((char*)tree.single, sizeof(*tree.single), GBM_DICT_INDEX);

                    if (brother.exists)	tree = brother;
                    else		tree.exists = NULL;
                }

                break;
            }
            case FULL_NODE: {
                int idx;

                for (idx=0; idx<256; idx++) {
                    if (tree.full->son[idx].exists) {
                        *resultBuffer = idx;
                        tree.full->son[idx] = remove_word_from_dtree(tree.full->son[idx], wordStart, wordLen,
                                                                     resultBuffer+1, resultLen, resultFrequency,
                                                                     &removed_single);
                        ad_assert(*resultLen);
                        (*resultLen)++;

                        if (!tree.full->son[idx].exists) { /* son branch removed -> zero count */
                            removed_single = tree.full->count[idx];
                            tree.full->count[idx] = 0;
                            tree.full->usedSons--;
                        }
                        else {
                            tree.full->count[idx] -= removed_single;
                            ad_assert(tree.full->count[idx]>0);
                        }

                        break;
                    }
                    else if (tree.full->count[idx]) {
                        *resultBuffer = idx;
                        *resultLen = 1;
                        *resultFrequency = tree.full->count[idx];
                        removed_single = tree.full->count[idx];
                        tree.full->count[idx] = 0;
                        tree.full->usedSons--;
                        break;
                    }
                }

                ad_assert(idx<256); /* ad_assert break was used to exit loop (== node had a son) */

                *removed += removed_single;

                if (!tree.full->usedSons) {
                    free_dtree(tree);
                    tree.exists = NULL;
                }

                break;
            }
        }
    }

#ifdef DEBUG
    if (*resultLen) {
        ad_assert(*resultLen>0);
        ad_assert(*resultFrequency>0);
        ad_assert(*resultLen>=wordLen);
    }
#endif

    return tree;
}

#if 0
#  define TEST_SORTED
static void dump_dictionary(GB_DICTIONARY *dict)
{
    int idx;

    printf("dictionary\n"
           "  words = %i\n"
           "  textlen = %i\n",
           dict->words,
           dict->textlen);

    for (idx=0; idx<dict->words; idx++) {
        u_str word = dict->text+ALPHA_DICT_OFFSET(idx, dict);
        /*dict->offsets[dict->resort[idx]];*/ /* @@@@ ntoh */

        printf("    word%03i='%s' (%i)\n", idx, lstr(word,30), ntohl(dict->resort[idx]));
    }
}
#endif


#define cmp(i1,i2) 	(heap2[i1]-heap2[i2])
#define swap(i1,i2) do                          \
{                                               \
    int s = heap[i1];                           \
    heap[i1] = heap[i2];                        \
    heap[i2] = s;                               \
                                                \
    s = heap2[i1];                              \
    heap2[i1] = heap2[i2];                      \
    heap2[i2] = s;                              \
}                                               \
while (0)

static void downheap(int *heap, int *heap2, int me, int num) {
    int lson = me*2;
    int rson = lson+1;

    ad_assert(me>=1);
    if (lson>num) return;

    if (cmp(lson,me)<0)	{	/* left son smaller than me?  (we sort in descending order!!!) */
        if (rson<=num && cmp(lson,rson)>0) { /* right son smaller than left son? */
            swap(me, rson);
            downheap(heap, heap2, rson, num);
        }
        else {
            swap(me, lson);
            downheap(heap, heap2, lson, num);
        }
    }
    else if (rson<=num && cmp(me, rson)>0) { /* right son smaller than me? */
        swap(me, rson);
        downheap(heap, heap2, rson, num);
    }
}

#undef cmp
#undef swap



#define cmp(i1,i2)	GB_MEMCMP(dict->text+dict->offsets[heap[i1]], dict->text+dict->offsets[heap[i2]], dict->textlen)
#define swap(i1,i2)	do { int s = heap[i1]; heap[i1] = heap[i2]; heap[i2] = s; } while(0)

static void downheap2(int *heap, GB_DICTIONARY *dict, int me, int num) {
    int lson = me*2;
    int rson = lson+1;

    ad_assert(me>=1);
    if (lson>num) return;

    if (cmp(lson,me)>0) {		/* left son bigger than me? */
        if (rson<=num && cmp(lson,rson)<0) { /* right son bigger than left son? */
            swap(me, rson);
            downheap2(heap, dict, rson, num);
        }
        else {
            swap(me, lson);
            downheap2(heap, dict, lson, num);
        }
    }
    else if (rson<=num && cmp(me, rson)<0) { /* right son bigger than me? */
        swap(me, rson);
        downheap2(heap, dict, rson, num);
    }
}

#undef cmp
#undef swap

static void sort_dict_offsets(GB_DICTIONARY *dict) {
    /* 1. sorts the 'dict->offsets' by frequency
     *    (frequency of each offset is stored in the 'dict->resort' with the same index)
     * 2. initializes & sorts 'dict->resort' in alphabethical order
     */
    int i;
    int num = dict->words;
    int *heap = dict->offsets-1;
    int *heap2 = dict->resort-1;

    /* sort offsets */

    for (i=num/2; i>=1; i--) downheap(heap, heap2, i, num);	/* make heap */

    while (num>1) { /* sort heap */
        int big = heap[1];
        int big2 = heap2[1];

        heap[1] = heap[num];
        heap2[1] = heap2[num];

        downheap(heap, heap2, 1, num-1);

        heap[num] = big;
        heap2[num] = big2;

        num--;
    }

#ifdef TEST_SORTED
    for (i=2,num=dict->words; i<=num; i++) ad_assert(heap2[i-1]>=heap2[i]); /* test if sorted correctly */
#endif

    /* initialize dict->resort */

    for (i=0, num=dict->words; i<num; i++) dict->resort[i] = i;

    /* sort dictionary alphabetically */

    for (i=num/2; i>=1; i--) downheap2(heap2, dict, i, num);	/* make heap */

    while (num>1) {
        int big = heap2[1];

        heap2[1] = heap2[num];
        downheap2(heap2, dict, 1, num-1);
        heap2[num] = big;
        num--;
    }

#ifdef TEST_SORTED
    for (i=1,num=dict->words; i<num; i++) {
        u_str word1 = dict->text+dict->offsets[dict->resort[i-1]];
        u_str word2 = dict->text+dict->offsets[dict->resort[i]];
        ad_assert(GB_MEMCMP(word1,word2,dict->textlen)<=0);
    }
#endif
}

/* Warning dictionary is not in network byte order !!!! */
static GB_DICTIONARY *gb_create_dictionary(O_gbdByKey *gbk, long maxmem) {
    long data_sum;
    GB_DICT_TREE  tree = build_dict_tree(gbk, maxmem, MAX_WORD_LEN, MIN_WORD_LEN, &data_sum);

    if (tree.exists) {
        GB_DICTIONARY *dict        = (GB_DICTIONARY*)gbm_get_mem(sizeof(*dict), GBM_DICT_INDEX);
        int            maxdeep     = 0;
        int            words       = count_dtree_leafs(tree, 0, &maxdeep);
        u_str          word;
        int            wordLen;
        long           wordFrequency;
        int            offset      = 0; /* next free position in dict->text */
        int            overlap     = 0; /* # of bytes overlapping with last word */
        u_str          buffer;
        long           dummy;
        long           word_sum    = 0;
        long           overlap_sum = 0;
        long           max_overlap = 0;

        /* reduce tree as long as it has to many leafs (>MAX_LONG_INDEX) */
        while (words >= MAX_LONG_INDEX) {

            words = count_dtree_leafs(tree, 0, &maxdeep);
        }

        buffer = (u_str)gbm_get_mem(maxdeep, GBM_DICT_INDEX);

        calcCounts(tree);
        testCounts(tree);

#if DEBUG
        printf("    examined data was %li bytes\n", data_sum);
        printf("    tree contains %i words *** maximum tree depth = %i\n", words, maxdeep);
#endif

        dict->words = 0;
        dict->textlen = DICT_STRING_INCR;
        dict->text = (u_str)gbm_get_mem(DICT_STRING_INCR, GBM_DICT_INDEX);
        dict->offsets = (GB_NINT*)gbm_get_mem(sizeof(*(dict->offsets))*words, GBM_DICT_INDEX);
        dict->resort = (GB_NINT*)gbm_get_mem(sizeof(*(dict->resort))*words, GBM_DICT_INDEX);

        memset(buffer, '*', maxdeep);
        tree = remove_word_from_dtree(tree, NULL, 0, buffer, &wordLen, &wordFrequency, &dummy);
        testCounts(tree);

        while (1) {
            int nextWordLen = 0;
            int len;

#if DUMP_COMPRESSION_TEST>=4
            printf("word='%s' (occur=%li overlap=%i)\n", lstr(buffer, wordLen), wordFrequency, overlap);
#endif

            overlap_sum += overlap;
            if (overlap>max_overlap) max_overlap = overlap;
            word_sum += wordLen;

            if (offset-overlap+wordLen > dict->textlen) { /* if not enough space allocated -> realloc dictionary string */
                u_str ntext = (u_str)gbm_get_mem(dict->textlen+DICT_STRING_INCR, GBM_DICT_INDEX);

                memcpy(ntext, dict->text, dict->textlen);
                gbm_free_mem((char*)dict->text, dict->textlen, GBM_DICT_INDEX);

                dict->text = ntext;
                dict->textlen += DICT_STRING_INCR;
            }

            dict->offsets[dict->words] = offset-overlap;
            dict->resort[dict->words] = wordFrequency;			/* temporarily miss-use this to store frequency */
            dict->words++;

    	    word = dict->text+offset-overlap;
            ad_assert(overlap==0 || GB_MEMCMP(word,buffer,overlap)==0);	/* test overlapping string-part */
            memcpy(word, buffer, wordLen);				/* word -> dictionary string */
            offset += wordLen-overlap;

            if (!tree.exists) break;

            for (len=min(10,wordLen-1); len>=0 && nextWordLen==0; len--) {
                memset(buffer, '*', maxdeep);
                tree = remove_word_from_dtree(tree, word+wordLen-len, len, buffer, &nextWordLen, &wordFrequency, &dummy);
                overlap = len;
            }

            wordLen = nextWordLen;
        }

        ad_assert(dict->words <= MAX_LONG_INDEX);
        ad_assert(dict->words==words);	/* dict->words == # of words stored in dictionary string
                                         * words   	   == # of words pre-calculated */

#if DEBUG
        printf("    word_sum=%li overlap_sum=%li (%li%%) max_overlap=%li\n",
               word_sum, overlap_sum, (overlap_sum*100)/word_sum, max_overlap);
#endif

        if (offset<dict->textlen) { /* reallocate dict->text if it was allocated too large */
            u_str ntext = (u_str)gbm_get_mem(offset, GBM_DICT_INDEX);

            memcpy(ntext, dict->text, offset);
            gbm_free_mem((char*)dict->text, dict->textlen, GBM_DICT_INDEX);

            dict->text = ntext;
            dict->textlen = offset;
        }

        sort_dict_offsets(dict);

        gbm_free_mem((char*)buffer, maxdeep, GBM_DICT_INDEX);
        free_dtree(tree);

        return dict;
    }

    return NULL;
}

static GB_ERROR readAndWrite(O_gbdByKey *gbkp) {
    int      i;
    GB_ERROR error = 0;

    for (i=0; i<gbkp->cnt && !error; i++) {
        GBDATA *gbd  = gbkp->gbds[i];
        int     type = GB_TYPE(gbd);

        if (COMPRESSABLE(type)) {
            long size;
            char *data;

            {
                char *d = (char*)get_data_n_size(gbd, &size);

                data = gbm_get_mem(size, GBM_DICT_INDEX);
                GB_MEMCPY(data, d, size);
                ad_assert(data[size-1] == 0);
            }

            switch(type) {
                case GB_STRING:
                    error             = GB_write_string(gbd, "");
                    if (!error) error = GB_write_string(gbd, data);
                    break;
                case GB_LINK:
                    error             = GB_write_link(gbd, "");
                    if (!error) error = GB_write_link(gbd, data);
                    break;
                case GB_BYTES:
                    error             = GB_write_bytes(gbd, 0, 0);
                    if (!error) error = GB_write_bytes(gbd, data, size);
                    break;
                case GB_INTS:
                    error             = GB_write_ints(gbd, (GB_UINT4 *)0, 0);
                    if (!error) error = GB_write_ints(gbd, (GB_UINT4 *)data, size);
                    break;
                case GB_FLOATS:
                    error             = GB_write_floats(gbd, (float*)0, 0);
                    if (!error) error = GB_write_floats(gbd, (float*)data, size);
                    break;
                default:
                    ad_assert(0);
                    break;
            }

            gbm_free_mem(data, size, GBM_DICT_INDEX);
        }
    }
    return error;
}

GB_ERROR gb_create_dictionaries(GB_MAIN_TYPE *Main, long maxmem) {
    GB_ERROR error = NULL;
#if defined(TEST_DICT)
    long uncompressed_sum = 0;
    long compressed_sum = 0;
#endif /* TEST_DICT */

    /*error = GB_begin_transaction((GBDATA*)Main->data);*/

    printf("Creating GBDATA-Arrays..\n");

    if (!error) {
        O_gbdByKey *gbk = g_b_opti_createGbdByKey(Main);
        int idx;

        printf("Creating dictionaries..\n");

#ifdef DEBUG
        /* #define TEST_ONE */               /* test only key specified below */
        /*#define TEST_SOME*/ 	/* test some keys starting with key specified below */
#if defined(TEST_ONE) || defined(TEST_SOME)
        /* select wanted index */
        for (idx=0; idx<gbdByKey_cnt; idx++) { /* title author dew_author ebi_journal name ua_tax date full_name ua_title */
            if (gbk[idx].cnt && strcmp(Main->keys[idx].key, "tree")==0) break;
        }
        ad_assert(idx<gbdByKey_cnt);
#endif
#endif

#ifdef TEST_ONE
        /* only create dictionary for index selected above (no loop) */
#else
#ifdef TEST_SOME
        /* only create dictionaries for some indices, starting with index selected above */
        for (       ; idx<gbdByKey_cnt && !error; ++idx)
#else
        /* create dictionaries for all indices (this is the normal operation) */
        for (idx = 1; idx<gbdByKey_cnt && !error; ++idx)
#endif
#endif

        {
            GB_DICTIONARY *dict;
            int            compression_mask;
            GB_CSTR        key_name = Main->keys[idx].key;
            int            type;
            GBDATA        *gb_main  = (GBDATA*)Main->data;

#ifndef TEST_ONE
            GB_status(idx/(double)gbdByKey_cnt);
            if (!gbk[idx].cnt) continue; /* there are no entries with this quark */

            type = GB_TYPE(gbk[idx].gbds[0]);
            compression_mask = gb_get_compression_mask(Main, idx, type);
            if ((compression_mask & GB_COMPRESSION_DICTIONARY)==0) continue; /* compression with dictionary is not allowed */
            if (strcmp(key_name,"data")==0) continue;
            if (strcmp(key_name,"quality")==0) continue;
#endif

            printf("dictionary for '%s' (idx=%i):\n", key_name, idx);
            GB_begin_transaction(gb_main);
            dict = gb_create_dictionary(&(gbk[idx]), maxmem);

            if (dict) {
                /* decompress with old dictionary and write
                   all data of actual type without compression: */

                printf("  Uncompressing all with old dictionary ...\n");

                {
                    int old_compression_mask = Main->keys[idx].compression_mask;

                    Main->keys[idx].compression_mask &= ~GB_COMPRESSION_DICTIONARY;
                    error                             = readAndWrite(&gbk[idx]);
                    Main->keys[idx].compression_mask  = old_compression_mask;
                }

                if (!error) {
                    /* dictionary is saved in the following format:
                     *
                     * GB_NINT size
                     * GB_NINT offsets[dict->words]
                     * GB_NINT resort[dict->words]
                     * char *text
                     */

                    int   dict_buffer_size = sizeof(GB_NINT) * (1+dict->words*2) + dict->textlen;
                    char *dict_buffer      = gbm_get_mem(dict_buffer_size, GBM_DICT_INDEX);
                    long  old_dict_buffer_size;
                    char *old_dict_buffer;

                    {
                        GB_NINT *nint = (GB_NINT*)dict_buffer;
                        int n;

                        *nint++ = htonl(dict->words);
                        for (n=0; n<dict->words; n++) *nint++ = htonl(dict->offsets[n]);
                        for (n=0; n<dict->words; n++) *nint++ = htonl(dict->resort[n]);

                        GB_MEMCPY(nint, dict->text, dict->textlen);
                    }

                    error = gb_load_dictionary_data(gb_main, Main->keys[idx].key, &old_dict_buffer, &old_dict_buffer_size);
                    if (!error) {
                        gb_save_dictionary_data(gb_main, Main->keys[idx].key, dict_buffer, dict_buffer_size);

                        /* compress all data with new dictionary */
                        printf("  Compressing all with new dictionary ...\n");
                        error = readAndWrite(&gbk[idx]);
                        if (error) {
                            /* critical state: new dictionary has been written, but transaction will be aborted below.
                             * Solution: Write back old dictionary.
                             */
                            gb_save_dictionary_data(gb_main, Main->keys[idx].key, old_dict_buffer, old_dict_buffer_size);
                        }
                    }

                    gbm_free_mem(dict_buffer, dict_buffer_size, GBM_DICT_INDEX);
                    if (old_dict_buffer) gbm_free_mem(old_dict_buffer, old_dict_buffer_size, GBM_DICT_INDEX);

#if defined(TEST_DICT)
                    if (!error) {
                        GB_DICTIONARY *dict_reloaded = gb_get_dictionary(Main, idx);
                        test_dictionary(dict_reloaded,&(gbk[idx]), &uncompressed_sum, &compressed_sum);
                    }
#endif /* TEST_DICT */
                }
            }

            if (error) GB_abort_transaction(gb_main);
            else GB_commit_transaction(gb_main);
        }

#ifdef DEBUG
        if (!error) {
            printf("    overall uncompressed size = %li b\n"
                   "    overall   compressed size = %li b (Ratio=%li%%)\n",
                   uncompressed_sum, compressed_sum,
                   (compressed_sum*100)/uncompressed_sum);
        }
#endif

        printf("Done.\n");

        g_b_opti_freeGbdByKey(Main,gbk);
        /* error = GB_commit_transaction((GBDATA*)Main->data);*/
    }

    return error;
}

GB_ERROR GB_optimize(GBDATA *gb_main) {
    unsigned long maxKB     = GB_get_physical_memory();
    long          maxMem;
    GB_ERROR      error     = 0;
    GB_UNDO_TYPE  undo_type = GB_get_requested_undo_type(gb_main);

#ifdef DEBUG
    maxKB /= 2;
#endif

    if (maxKB<=(LONG_MAX/1024)) maxMem = maxKB*1024;
    else 			maxMem = LONG_MAX;

    GB_request_undo_type(gb_main,GB_UNDO_KILL);

    error = gb_create_dictionaries(GB_MAIN(gb_main), maxMem);

    GB_disable_quicksave(gb_main,"Database optimized");
    GB_request_undo_type(gb_main,undo_type);

    return error;
}



