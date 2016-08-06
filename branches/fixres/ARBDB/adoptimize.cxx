// =============================================================== //
//                                                                 //
//   File      : adoptimize.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <climits>
#include <netinet/in.h>

#include <arb_file.h>
#include <arb_diff.h>

#include <arbdbt.h>

#include "gb_key.h"
#include "gb_compress.h"
#include "gb_dict.h"

#include "arb_progress.h"

#if defined(DEBUG)
// #define TEST_DICT
#endif // DEBUG

typedef unsigned char        unsigned_char;
typedef unsigned char       *u_str;
typedef const unsigned char *cu_str;

static int gbdByKey_cnt;
struct O_gbdByKey { // one for each diff. keyQuark
    int      cnt;
    GBDATA **gbds;              // gbdoff
};

struct FullDictTree;
struct SingleDictTree;

union DictTree {
    FullDictTree   *full;
    SingleDictTree *single;
    void           *exists;

};

enum DictNodeType { SINGLE_NODE, FULL_NODE };

struct FullDictTree {
    DictNodeType typ;           // always FULL_NODE
    int          usedSons;
    int          count[256];
    DictTree     son[256];      // index == character
};

struct SingleDictTree {
    DictNodeType  typ;          // always SINGLE_NODE
    unsigned_char ch;           // the character
    int           count;        // no of occurrences of this branch
    DictTree      son;
    DictTree      brother;

};

// **************************************************

#define COMPRESSIBLE(type) ((type) >= GB_BYTES && (type)<=GB_STRING)
#define DICT_MEM_WEIGHT    4

#define WORD_HELPFUL(wordlen, occurrences)      ((long)((occurrences)*3 + DICT_MEM_WEIGHT*(2*sizeof(GB_NINT)+(wordlen))) \
                                                 < \
                                                 (long)((occurrences)*(wordlen)))
/* (occurrences)*4                      compressed size
 * 2*sizeof(GB_NINT)+(wordlen)          size in dictionary
 * (occurrences)*(wordlen)              uncompressed size
 */

// **************************************************

#define MIN_WORD_LEN    8       // minimum length of words in dictionary
#define MAX_WORD_LEN    50      // maximum length of words in dictionary
#define MAX_BROTHERS    10      /* maximum no of brothers linked with SingleDictTree
                                 * above we use FullDictTree */
#define MAX_DIFFER       2      /* percentage of difference (of occurrences of strings) below which two
                                 * consecutive parts are treated as EQUAL # of occurrences */
#define INCR_DIFFER      1      // the above percentage is incremented from 0 to MAX_DIFFER by INCR_DIFFER per step

#define DICT_STRING_INCR 1024   // dictionary string will be incremented by this size

// ******************* Tool functions ******************

inline cu_str get_data_n_size(GBDATA *gbd, size_t *size) {
    GB_CSTR data;
    *size = 0;

    switch (gbd->type()) {
        case GB_STRING: data = GB_read_char_pntr(gbd); break;
        case GB_LINK:   data = GB_read_link_pntr(gbd); break;
        case GB_BYTES:  data = GB_read_bytes_pntr(gbd); break;
        case GB_INTS:   data = (char*)GB_read_ints_pntr(gbd); break;
        case GB_FLOATS: data = (char*)GB_read_floats_pntr(gbd); break;
        default:
            data = 0;
            gb_assert(0);
            break;
    }

    if (data) *size = gbd->as_entry()->uncompressed_size();
    return (cu_str)data;
}

static inline long min(long a, long b) {
    return a<b ? a : b;
}

// **************************************************

static void g_b_opti_scanGbdByKey(GB_MAIN_TYPE *Main, GBDATA *gbd, O_gbdByKey *gbk) {
    if (gbd->is_container()) {
        GBCONTAINER *gbc = gbd->as_container();
        for (int idx=0; idx < gbc->d.nheader; idx++) {
            GBDATA *gbd2 = GBCONTAINER_ELEM(gbc, idx);
            if (gbd2) g_b_opti_scanGbdByKey(Main, gbd2, gbk);
        }
    }

    GBQUARK quark = GB_KEY_QUARK(gbd);
    if (quark)
    {
        gb_assert(gbk[quark].cnt < Main->keys[quark].nref || quark==0);
        gb_assert(gbk[quark].gbds != 0);

        gbk[quark].gbds[gbk[quark].cnt] = gbd;
        gbk[quark].cnt++;
    }
}

static O_gbdByKey *g_b_opti_createGbdByKey(GB_MAIN_TYPE *Main)
{
    int idx;
    O_gbdByKey *gbk = (O_gbdByKey *)ARB_calloc(Main->keycnt, sizeof(O_gbdByKey));

    gbdByKey_cnt = Main->keycnt; // always use gbdByKey_cnt instead of Main->keycnt cause Main->keycnt can change

    for (idx=1; idx<gbdByKey_cnt; idx++) {
        gbk[idx].cnt = 0;

        gb_Key& KEY = Main->keys[idx];
        if (KEY.key && KEY.nref>0) {
            gbk[idx].gbds  = (GBDATA **)ARB_calloc(KEY.nref, sizeof(GBDATA*));
        }
        else {
            gbk[idx].gbds = NULL;
        }
    }

    gbk[0].cnt  = 0;
    gbk[0].gbds = (GBDATA **)ARB_calloc(1, sizeof(GBDATA*));

    g_b_opti_scanGbdByKey(Main, Main->gb_main(), gbk);

    for (idx=0; idx<gbdByKey_cnt; idx++) {
        if (gbk[idx].cnt != Main->keys[idx].nref && idx)
        {
            printf("idx=%i gbk[idx].cnt=%i Main->keys[idx].nref=%li\n",             // Main->keys[].nref ist falsch
                   idx, gbk[idx].cnt, Main->keys[idx].nref);

            Main->keys[idx].nref = gbk[idx].cnt;
        }
    }
    return gbk;
}

static void g_b_opti_freeGbdByKey(O_gbdByKey *gbk) {
    for (int idx=0; idx<gbdByKey_cnt; idx++) free(gbk[idx].gbds);
    free(gbk);
}

// ******************* Convert old compression style to new style ******************

static GB_ERROR gb_convert_compression(GBDATA *gbd) {
    GB_ERROR error = 0;

    if (gbd->is_container()) {
        for (GBDATA *gb_child = GB_child(gbd); gb_child; gb_child = GB_nextChild(gb_child)) {
            if (gb_child->flags.compressed_data || gb_child->is_container()) {
                error = gb_convert_compression(gb_child);
                if (error) break;
            }
        }
    }
    else {
        char    *str        = 0;
        GBENTRY *gbe        = gbd->as_entry();
        long     elems      = gbe->size();
        size_t   data_size  = gbe->uncompressed_size();
        size_t   new_size   = -1;
        int      expectData = 1;

        switch (gbd->type()) {
            case GB_STRING:
            case GB_LINK:
            case GB_BYTES:
                str = gb_uncompress_bytes(gbe->data(), data_size, &new_size);
                if (str) {
                    gb_assert(new_size == data_size);
                    str = GB_memdup(str, data_size);
                }
                break;

            case GB_INTS:
            case GB_FLOATS:
                str = gb_uncompress_longs_old(gbe->data(), elems, &new_size);
                if (str) {
                    gb_assert(new_size == data_size);
                    str = GB_memdup(str, data_size);
                }
                break;

            default:
                expectData = 0;
                break;
        }

        if (!str) {
            if (expectData) {
                error = GBS_global_string("Can't read old data to convert compression (Reason: %s)", GB_await_error());
            }
        }
        else {
            switch (gbd->type()) {
                case GB_STRING:
                    error             = GB_write_string(gbe, "");
                    if (!error) error = GB_write_string(gbe, str);
                    break;

                case GB_LINK:
                    error             = GB_write_link(gbe, "");
                    if (!error) error = GB_write_link(gbe, str);
                    break;

                case GB_BYTES:
                    error             = GB_write_bytes(gbe, "", 0);
                    if (!error) error = GB_write_bytes(gbe, str, data_size);
                    break;

                case GB_INTS:
                case GB_FLOATS:
                    error = GB_write_pntr(gbe, str, data_size, elems);
                    break;

                default:
                    gb_assert(0);
                    break;
            }

            free(str);
        }
    }
    return error;
}

GB_ERROR gb_convert_V2_to_V3(GBDATA *gb_main) {
    GB_ERROR  error     = 0;
    GBDATA   *gb_system = GB_search(gb_main, GB_SYSTEM_FOLDER, GB_FIND);

    if (!gb_system) {
        GB_create_container(gb_main, GB_SYSTEM_FOLDER);
        if (GB_entry(gb_main, "extended_data")) {
            GB_warning("Converting data from old V2.0 to V2.1 Format:\n"
                       " Please Wait (may take some time)");
        }
        error = gb_convert_compression(gb_main);
        GB_disable_quicksave(gb_main, "Database converted to new format");
    }
    return error;
}


// ********************* Compress by dictionary ********************


/* compression tag format:
 *
 * unsigned int compressed:1;
 *     if compressed==0:
 *         unsigned int last:1;         ==1 -> this is the last block
 *         unsigned int len:6;          length of uncompressible bytes
 *         char[len];
 *     if compressed==1:
 *         unsigned int idxlen:1;       ==0 -> 10-bit index
 *                                      ==1 -> 18-bit index
 *         unsigned int idxhigh:2;      the 2 highest bits of the index
 *         unsigned int len:4;          (length of word) - (MIN_COMPR_WORD_LEN-1)
 *         if len==0:
 *             char extralen;           (length of word) -
 *         char[idxlen+1];              index (low,high)
 *
 *     tag == 64 -> end of dictionary compressed block (if not coded in last uncompressed block)
 */

inline int INDEX_DICT_OFFSET(int idx, GB_DICTIONARY *dict) {
    gb_assert(idx<dict->words);
    return ntohl(dict->offsets[idx]);
}
inline int ALPHA_DICT_OFFSET(int idx, GB_DICTIONARY *dict) {
    int realIndex;
    gb_assert(idx<dict->words);
    realIndex = ntohl(dict->resort[idx]);
    return INDEX_DICT_OFFSET(realIndex, dict);
}

// #define ALPHA_DICT_OFFSET(i)         ntohl(offset[ntohl(resort[i])])
// #define INDEX_DICT_OFFSET(i)         ntohl(offset[i])

#define LEN_BITS       4
#define INDEX_BITS     2
#define INDEX_LEN_BITS 1

#define LEN_SHIFT       0
#define INDEX_SHIFT     (LEN_SHIFT+LEN_BITS)
#define INDEX_LEN_SHIFT (INDEX_SHIFT+INDEX_BITS)

#define BITMASK(bits)   ((1<<(bits))-1)
#define GETVAL(tag,typ) (((tag)>>typ##_SHIFT)&BITMASK(typ##_BITS))

#define MIN_SHORTLEN 6
#define MAX_SHORTLEN (BITMASK(LEN_BITS)+MIN_SHORTLEN-1)
#define MIN_LONGLEN  (MAX_SHORTLEN+1)
#define MAX_LONGLEN  (MIN_LONGLEN+255)

#define SHORTLEN_DECR (MIN_SHORTLEN-1)               // !! zero is used as flag for long len !!
#define LONGLEN_DECR  MIN_LONGLEN

#define MIN_COMPR_WORD_LEN MIN_SHORTLEN
#define MAX_COMPR_WORD_LEN MAX_LONGLEN

#define MAX_SHORT_INDEX BITMASK(INDEX_BITS+8)
#define MAX_LONG_INDEX  BITMASK(INDEX_BITS+16)

#define LAST_COMPRESSED_BIT 64

#ifdef DEBUG
# define DUMP_COMPRESSION_TEST  0
/*      0 = only compression ratio
 *      1 = + original/compressed/decompressed
 *      2 = + words used to compress/uncompress
 *      3 = + matching words in dictionary
 *      4 = + search of words in dictionary
 */
#else
# define DUMP_COMPRESSION_TEST  0
#endif

#ifdef DEBUG
// #define COUNT_CHUNKS

#if defined(COUNT_CHUNKS)

static long uncompressedBlocks[64];
static long compressedBlocks[MAX_LONGLEN];

static void clearChunkCounters() {
    int i;

    for (i=0; i<64; i++) uncompressedBlocks[i] = 0;
    for (i=0; i<MAX_LONGLEN; i++) compressedBlocks[i] = 0;
}

static void dumpChunkCounters() {
    int i;

    printf("------------------------------\n" "Uncompressed blocks used:\n");
    for (i=0; i<64; i++) if (uncompressedBlocks[i]) printf("  size=%i used=%li\n", i, uncompressedBlocks[i]);
    printf("------------------------------\n" "Words used:\n");
    for (i=0; i<MAX_LONGLEN; i++) if (compressedBlocks[i]) printf("  size=%i used=%li\n", i, compressedBlocks[i]);
    printf("------------------------------\n");
}
#endif // COUNT_CHUNKS

static cu_str lstr(cu_str s, int len) {
#define BUFLEN 20000
    static unsigned_char buf[BUFLEN];

    gb_assert(len<BUFLEN);
    memcpy(buf, s, len);
    buf[len] = 0;

    return buf;
}

#if DUMP_COMPRESSION_TEST>=2

static cu_str dict_word(GB_DICTIONARY *dict, int idx, int len) {
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
            putchar(c&bitval ? '1' : '0');
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

inline int GB_MEMCMP(const void *vm1, const void *vm2, long size) {
    char *c1 = (char*)vm1,
        *c2 = (char*)vm2;
    int diff = 0;

    while (size-- && !diff) diff = *c1++-*c2++;
    return diff;
}

// --------------------------------------------------

static int searchWord(GB_DICTIONARY *dict, cu_str source, long size, unsigned long *wordIndex, int *wordLen)
{
    int      idx    = -1;
    int      l      = 0;
    int      h      = dict->words-1;
    cu_str   text   = dict->text;
    GB_NINT *resort = dict->resort;
    int      dsize  = dict->textlen;
    int      ilen   = 0;

    while (l<h-1) {
        int    m        = (l+h)/2;
        long   off      = ALPHA_DICT_OFFSET(m, dict);
        cu_str dictword = text+off;
        long   msize    = min(size, dsize-off);

#if DUMP_COMPRESSION_TEST>=4
        printf("  %s (%i)\n", lstr(dictword, 20), m);
#endif

        if (GB_MEMCMP(source, dictword, msize)<=0)      h = m;
        else                                                            l = m;
    }

    while (l<=h) {
        int    off   = ALPHA_DICT_OFFSET(l, dict);
        cu_str word  = text+off;
        int    msize = (int)min(size, dsize-off);
        int    equal = 0;
        cu_str s     = source;

        while (msize-- && *s++==*word++) equal++;

#if DUMP_COMPRESSION_TEST>=3
        if (equal>=MIN_COMPR_WORD_LEN) {
            printf("  EQUAL=%i '%s' (%i->%i, off=%i)", equal, lstr(text+off, equal), l, ntohl(resort[l]), ALPHA_DICT_OFFSET(l, dict));
            printf(" (context=%s)\n", lstr(text+off-min(off, 20), min(off, 20)+equal+20));
        }
#endif

        if (equal>ilen) {
            ilen = equal;
            idx  = ntohl(resort[l]);
            gb_assert(idx<dict->words);
        }

        l++;
    }

    *wordIndex = idx;
    *wordLen = (int)min(ilen, MAX_COMPR_WORD_LEN);

    return idx!=-1 && ilen>=MIN_COMPR_WORD_LEN;
}

#ifdef DEBUG
int lookup_DICTIONARY(GB_DICTIONARY *dict, GB_CSTR source) { // used for debugging
    unsigned long wordIndex;
    int           wordLen;
    int           wordFound = searchWord(dict, (cu_str)source, strlen(source), &wordIndex, &wordLen);

    if (wordFound) {
        printf("'%s' (idx=%lu, off=%i)\n", lstr(dict->text+ntohl(dict->offsets[wordIndex]), wordLen), wordIndex, ntohl(dict->offsets[wordIndex]));
    }

    return wordFound;
}
#endif



static char *gb_uncompress_by_dictionary_internal(GB_DICTIONARY *dict, /* GBDATA *gbd, */ GB_CSTR s_source, const size_t size, bool append_zero, size_t *new_size) {
    cu_str source = (cu_str)s_source;
    u_str  dest;
    u_str  buffer;
    cu_str text   = dict->text;
    int    done   = 0;
    long   left   = size;

    dest = buffer = (u_str)GB_give_other_buffer(s_source, size+2);

    while (left && !done) {
        int c;

        if ((c=*source++)&128) { // compressed data
            int indexLen = GETVAL(c, INDEX_LEN);
            unsigned long idx = GETVAL(c, INDEX);

            c = GETVAL(c, LEN); // ==wordLen
            if (c)      c += SHORTLEN_DECR;
            else        c = *source+++LONGLEN_DECR;

            gb_assert(indexLen>=0 && indexLen<=1);

            if (indexLen==0) {
                idx = (idx << 8) | *source++;
            }
            else {
                idx = (((idx << 8) | source[1]) << 8) | source[0];
                source += 2;
            }

            gb_assert(idx<(GB_ULONG)dict->words);

            {
                cu_str word = text+INDEX_DICT_OFFSET(idx, dict);

#if DUMP_COMPRESSION_TEST>=2
                printf("  word='%s' (idx=%lu, off=%li, len=%i)\n",
                       lstr(word, c), idx, (long)ntohl(dict->offsets[idx]), c);
#endif

                {
                    u_str d = dest;
                    gb_assert(((d + c) <= word) || (d >= (word + c)));
                    while (c--) *d++ = *word++;
                    dest = d;
                }
            }
        }
        else {                  // uncompressed bytes
            if (c & LAST_COMPRESSED_BIT) {
                done = 1;
                c ^= LAST_COMPRESSED_BIT;
            }

            left -= c;
            {
                u_str d = dest;
                gb_assert(((d + c) <= source) || (d >= (source + c)));
                while (c--) *d++ = *source++;
                dest=d;
            }
        }
    }

    if (append_zero) *dest++ = 0;

    *new_size = dest-buffer;
    gb_assert(size >= *new_size); // buffer overflow

    return (char *)buffer;
}

char *gb_uncompress_by_dictionary(GBDATA *gbd, GB_CSTR s_source, size_t size, size_t *new_size)
{
    GB_DICTIONARY *dict        = gb_get_dictionary(GB_MAIN(gbd), GB_KEY_QUARK(gbd));
    bool           append_zero = gbd->is_a_string();

    if (!dict) {
        GB_ERROR error = GBS_global_string("Cannot decompress db-entry '%s' (no dictionary found)\n", GB_get_db_path(gbd));
        GB_export_error(error);
        return 0;
    }

    return gb_uncompress_by_dictionary_internal(dict, s_source, size, append_zero, new_size);
}

char *gb_compress_by_dictionary(GB_DICTIONARY *dict, GB_CSTR s_source, size_t size, size_t *msize, int last_flag, int search_backward, int search_forward)
{
    cu_str source           = (cu_str)s_source;
    u_str  dest;
    u_str  buffer;
    cu_str unknown          = source; // start of uncompressible bytes
    u_str  lastUncompressed = NULL; // ptr to start of last block of uncompressible bytes (in dest)

#if defined(ASSERTION_USED)
    const size_t org_size = size;
#endif                          // ASSERTION_USED

    gb_assert(size>0); // compression of zero-length data fails!

    dest = buffer = (u_str)GB_give_other_buffer((GB_CSTR)source, 1+(size/63+1)+size);
    *dest++ = GB_COMPRESSION_DICTIONARY | last_flag;

    while (size) {
        unsigned long wordIndex;
        int wordLen;
        int wordFound;

        if ((wordFound = searchWord(dict, source, size, &wordIndex, &wordLen))) {
            int length;

        takeRest :
            length = source-unknown;

            if (length) {
                int shift;
                int takeShift = 0;
                int maxShift = (int)min(search_forward, wordLen-1);

                for (shift=1; shift<=maxShift; shift++) {
                    unsigned long wordIndex2;
                    int wordLen2;
                    int wordFound2;

                    if ((wordFound2 = searchWord(dict, source+shift, size-shift, &wordIndex2, &wordLen2))) {
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

            while (length) {    // if there were uncompressible bytes
                int take = (int)min(length, 63);

#ifdef COUNT_CHUNKS
                uncompressedBlocks[take]++;
#endif

                lastUncompressed = dest;

                *dest++ = take; // tag byte
                memcpy(dest, unknown, take);
                dest += take;
                unknown += take;
                length -= take;
            }

            gb_assert(unknown==source);

            while (wordFound) { // as long as we find words in dictionary
                int           indexLen      = wordIndex>MAX_SHORT_INDEX;
                int           indexHighBits = indexLen==0 ? wordIndex>>8 : wordIndex>>16;
                int           nextWordFound;
                int           nextWordLen;
                unsigned long nextWordIndex;

                gb_assert((long)wordIndex<dict->words);
                gb_assert((long)wordIndex <= MAX_LONG_INDEX);
                gb_assert(indexHighBits==(indexHighBits & BITMASK(INDEX_BITS)));
                gb_assert(wordLen>=MIN_SHORTLEN);

                lastUncompressed = NULL;

                {
                    cu_str source2 = source+wordLen;
                    long size2 = size-wordLen;

                    if (!(nextWordFound=searchWord(dict, source+wordLen, size-wordLen, &nextWordIndex, &nextWordLen))) { // no word right afterwards
                        int shift;

                        for (shift=1; shift<=search_backward && shift<(wordLen-MIN_COMPR_WORD_LEN); shift++) {
                            // try to cut end of word to get a better result
                            unsigned long wordIndex2;
                            int wordLen2;
                            int wordFound2;

                            if ((wordFound2=searchWord(dict, source2-shift, size2+shift, &wordIndex2, &wordLen2))) {
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
                    *dest++ = wordLen-LONGLEN_DECR; // extra length byte
                }

                *dest++ = (char)wordIndex; // low index byte
                if (indexLen)
                    *dest++ = (char)(wordIndex >> 8); // high index byte

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

    if (lastUncompressed)       *lastUncompressed |= LAST_COMPRESSED_BIT;
    else                        *dest++ = LAST_COMPRESSED_BIT;

    *msize = dest-buffer;

#if defined(ASSERTION_USED)
    {
        size_t  new_size = -1;
        char   *test     = gb_uncompress_by_dictionary_internal(dict, (GB_CSTR)buffer+1, org_size + GB_COMPRESSION_TAGS_SIZE_MAX, true, &new_size);

        gb_assert(memcmp(test, s_source, org_size) == 0);
        gb_assert((org_size+1) == new_size);
    }
#endif // ASSERTION_USED

    return (char*)buffer;
}


#if defined(TEST_DICT)

static void test_dictionary(GB_DICTIONARY *dict, O_gbdByKey *gbk, long *uncompSum, long *compSum) {
    long uncompressed_sum = 0;
    long compressed_sum   = 0;

    long dict_size = (dict->words*2+1)*sizeof(GB_NINT)+dict->textlen;

    long char_count[256];
    for (int i=0; i<256; i++) char_count[i] = 0;

    printf("  * Testing compression..\n");

#ifdef COUNT_CHUNKS
    clearChunkCounters();
#endif

    for (int cnt=0; cnt<gbk->cnt; cnt++) {
        GBDATA *gbd = gbk->gbds[cnt];

        if (COMPRESSIBLE(gbd->type())) {
            size_t size;
            cu_str data = get_data_n_size(gbd, &size);

            if (gbd->is_a_string()) size--;
            if (size<1) continue;

            u_str copy = (u_str)gbm_get_mem(size, GBM_DICT_INDEX);
            gb_assert(copy!=0);
            memcpy(copy, data, size);

#if DUMP_COMPRESSION_TEST>=1
            printf("----------------------------\n");
            printf("original    : %3li b = '%s'\n", size, data);
#endif

            int    last_flag  = 0;
            size_t compressedSize;
            u_str  compressed = (u_str)gb_compress_by_dictionary(dict, (GB_CSTR)data, size, &compressedSize, last_flag, 9999, 2);

#if DUMP_COMPRESSION_TEST>=1
            printf("compressed  : %3li b = '%s'\n", compressedSize, lstr(compressed, compressedSize));
            dumpBinary(compressed, compressedSize);
#endif

            for (size_t i=0; i<compressedSize; i++) char_count[compressed[i]]++;

            size_t new_size     = -1;
            u_str  uncompressed = (u_str)gb_uncompress_by_dictionary(gbd, (char*)compressed+1, size+GB_COMPRESSION_TAGS_SIZE_MAX, &new_size);

#if DUMP_COMPRESSION_TEST>=1
            printf("copy        : %3li b = '%s'\n", size, lstr(copy, size));
            printf("decompressed: %3li b = '%s'\n", size, lstr(uncompressed, size));
#endif

            if (GB_MEMCMP(copy, uncompressed, size)!=0) {
                int byte = 0;

                while (copy[byte]==uncompressed[byte]) byte++;
                printf("Error in compression (off=%i, '%s'", byte, lstr(copy+byte, 10));
                printf("!='%s'\n", lstr(uncompressed+byte, 10));
            }

            if (compressedSize<size) {
                uncompressed_sum += size;
                compressed_sum += compressedSize;
            }
            else {
                uncompressed_sum += size;
                compressed_sum += size;
            }

            gbm_free_mem(copy, size, GBM_DICT_INDEX);
        }
    }

#ifdef COUNT_CHUNKS
    dumpChunkCounters();
#endif

    {
        long  compressed_plus_dict = compressed_sum+dict_size;
        char *dict_text            = GBS_global_string_copy("+dict %li b", dict_size);
        long  ratio                = (compressed_plus_dict*100)/uncompressed_sum;

        printf("    uncompressed size = %10li b\n"
               "      compressed size = %10li b\n"
               "    %17s = %10li b (Ratio=%li%%)\n",
               uncompressed_sum,
               compressed_sum,
               dict_text, compressed_plus_dict, ratio);

        free(dict_text);
    }

    *uncompSum += uncompressed_sum;
    *compSum   += compressed_sum+dict_size;
}

#endif // TEST_DICT


// ******************* Build dictionary ******************

#ifdef DEBUG
#define TEST                    // test trees?
// #define DUMP_TREE // dump trees?

// #define DUMP_EXPAND
/*
  #define SELECT_WORDS
  #define SELECTED_WORDS "oropl"
*/

# ifdef SELECT_WORDS
static char *strnstr(char *s1, int len, char *s2) {
    char c    = *s2;
    int  len2 = strlen(s2);

    while (len-->=len2) {
        if (*s1==c) {
            if (strncmp(s1, s2, len2)==0) return s1;
        }
        s1++;
    }

    return NULL;
}
# endif

#ifdef DUMP_TREE
static void dump_dtree(int deep, DictTree tree)
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
#endif

#else
#ifdef DUMP_TREE
# define dump_dtree(deep, tree)
#endif
#endif

#ifdef TEST
static int testCounts(DictTree tree) {
    // tests if all inner nodes have correct 'count's
    int cnt = 0;

    if (tree.exists) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                while (tree.exists) {
                    if (tree.single->son.exists) {
                        int son_cnt = testCounts(tree.single->son);
#ifdef COUNT_EQUAL
                        gb_assert(son_cnt==tree.single->count);
#else
                        gb_assert(son_cnt<=tree.single->count);
#endif
                    }

                    gb_assert(tree.single->count>0);
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
                        gb_assert(son_cnt==tree.full->count[idx]);
#else
                        gb_assert(son_cnt<=tree.full->count[idx]);
#endif
                        if (tree.full->usedSons)    gb_assert(tree.full->count[idx]>0);
                        else                        gb_assert(tree.full->count[idx]==0);

                        sons++;
                    }
                    else if (tree.full->count[idx]) {
                        sons++;
                    }

                    cnt += tree.full->count[idx];
                }

                gb_assert(sons==tree.full->usedSons);
                break;
            }
        }
    }

    return cnt;
}

// #define TEST_MAX_OCCUR_COUNT

#ifdef TEST_MAX_OCCUR_COUNT
#define MAX_OCCUR_COUNT 600000
#endif

static DictTree test_dtree(DictTree tree)
// only correct while tree is under contruction (build_dict_tree())
{
    if (tree.exists) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
#if defined(TEST_MAX_OCCUR_COUNT)
                gb_assert(tree.single->count<MAX_OCCUR_COUNT);  // quite improbable
#endif // TEST_MAX_OCCUR_COUNT

                if (tree.single->son.exists) {
                    gb_assert(tree.single->count==0);
                    test_dtree(tree.single->son);
                }
                else {
                    gb_assert(tree.single->count>0);
                }

                if (tree.single->brother.exists) test_dtree(tree.single->brother);
                break;
            }
            case FULL_NODE: {
                int idx;
                int countSons = 0;

                for (idx=0; idx<256; idx++) {
#if defined(TEST_MAX_OCCUR_COUNT)
                    gb_assert(tree.full->count[idx]<MAX_OCCUR_COUNT); // quite improbable
#endif // TEST_MAX_OCCUR_COUNT

                    if (tree.full->son[idx].exists) {
                        gb_assert(tree.full->count[idx]==0);
                        test_dtree(tree.full->son[idx]);
                        countSons++;
                    }
                    else {
                        gb_assert(tree.full->count[idx]>=0);
                        if (tree.full->count[idx]>0)
                            countSons++;
                    }
                }

                gb_assert(countSons==tree.full->usedSons);

                break;
            }
        }
    }

    return tree;
}

#else
# define test_dtree(tree)       // (tree)
# define testCounts(tree)       // 0
#endif


static DictTree new_dtree(cu_str text, long len, long *memcount) {
    // creates a new (sub-)tree from 'text' (which has length 'len')
    DictTree tree;

    if (len) {
        SingleDictTree *tail = NULL;
        SingleDictTree *head = NULL;

        while (len) {
            if (tail)   tail = tail->son.single = (SingleDictTree*)gbm_get_mem(sizeof(*tail), GBM_DICT_INDEX);
            else        tail = head = (SingleDictTree*)gbm_get_mem(sizeof(*tail), GBM_DICT_INDEX);

            (*memcount) += sizeof(*tail);

            tail->typ = SINGLE_NODE;
            tail->ch = *text++;
            len--;

            tail->brother.single = NULL;
            tail->son.single     = NULL;
        }

        tail->count = 1;
        tree.single = head;
    }
    else {
        tree.single = NULL;
    }

    return tree;
}

static DictTree single2full_dtree(DictTree tree, long *memcount) {
    if (tree.exists && tree.single->typ==SINGLE_NODE) {
        FullDictTree *full = (FullDictTree*)gbm_get_mem(sizeof(*full), GBM_DICT_INDEX);
        int idx;

        (*memcount) += sizeof(*full);
        full->typ = FULL_NODE;
        full->usedSons = 0;

        for (idx=0; idx<256; idx++) {
            full->son[idx].exists = NULL;
            full->count[idx] = 0;
        }

        while (tree.exists) {
            SingleDictTree *t = tree.single;

            gb_assert(t->typ==SINGLE_NODE);
            gb_assert(full->son[t->ch].exists==NULL);

            full->son[t->ch] = t->son;
            full->count[t->ch] = t->count;
            full->usedSons++;

            tree.single = t->brother.single;

            gbm_free_mem(t, sizeof(*t), GBM_DICT_INDEX);
            (*memcount) -= sizeof(*t);
        }

        tree.full = full;
    }

    return tree;
}

static void free_dtree(DictTree tree)
{
    if (tree.exists) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                if (tree.single->son.exists) free_dtree(tree.single->son);
                if (tree.single->brother.exists) free_dtree(tree.single->brother);

                gbm_free_mem(tree.single, sizeof(*(tree.single)), GBM_DICT_INDEX);
                break;
            }
            case FULL_NODE: {
                int idx;

                for (idx=0; idx<256; idx++) if (tree.full->son[idx].exists) free_dtree(tree.full->son[idx]);
                gbm_free_mem(tree.full, sizeof(*(tree.full)), GBM_DICT_INDEX);
                break;
            }
        }
    }
}



static DictTree cut_dtree(DictTree tree, int cut_count, long *memcount, long *leafcount)
/* removes all branches from 'tree' which are referenced less/equal than cut_count
 * returns: the reduced tree */
{
    if (tree.exists) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                if (tree.single->son.exists) tree.single->son = cut_dtree(tree.single->son, cut_count, memcount, leafcount);

                if (!tree.single->son.exists) { // leaf
                    if (tree.single->count<=cut_count) { // leaf with less/equal references
                        DictTree brother = tree.single->brother;

                        gbm_free_mem(tree.single, sizeof(*tree.single), GBM_DICT_INDEX);
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

                        if (tree.full->son[idx].exists)         count++;
                        else                                                tree.full->count[idx] = 0;
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

                if (!count) {           // no more sons
                    gbm_free_mem(tree.full, sizeof(*(tree.full)), GBM_DICT_INDEX);
                    (*memcount) -= sizeof(*(tree.full));
                    tree.exists = NULL;
                }

                break;
            }
        }
    }

    return tree;
}
static DictTree cut_useless_words(DictTree tree, int deep, long *removed)
/* removes/shortens all branches of 'tree' which are not useful for compression
 * 'deep' should be zero (incremented by cut_useless_words)
 * 'removed' will be set to the # of removed occurrences
 * returns: the reduced tree
 */
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

                if (!tree.single->son.exists && !WORD_HELPFUL(deep, tree.single->count)) {
                    DictTree brother = tree.single->brother;

                    *removed += tree.single->count;
                    gbm_free_mem(tree.single, sizeof(*tree.single), GBM_DICT_INDEX);

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
                        if (!WORD_HELPFUL(deep, tree.full->count[idx])) {       // useless!
                            *removed += tree.full->count[idx];
                            tree.full->count[idx] = 0;
                        }
                        else {
                            count++;
                        }
                    }
                }

                tree.full->usedSons = count;

                if (!count) {           // no more sons
                    gbm_free_mem(tree.full, sizeof(*(tree.full)), GBM_DICT_INDEX);
                    tree.exists = NULL;
                }

                break;
            }
        }
    }

    return tree;
}

static DictTree add_dtree_to_dtree(DictTree toAdd, DictTree to, long *memcount)
/* adds 'toAdd' as brother of 'to' (must be leftmost of all SINGLE_NODEs or a FULL_NODE)
 * returns: the leftmost of all SINGLE_NODEs or a FULL_NODE
 */
{
    DictTree tree = toAdd;

    gb_assert(toAdd.single->typ==SINGLE_NODE);

    if (to.exists) {
        switch (to.full->typ) {
            case SINGLE_NODE: {
                SingleDictTree *left = to.single;

                gb_assert(left!=0);

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

                gb_assert(to.full->son[ch].exists==NULL);
                gb_assert(to.full->count[ch]==0); // if this fails, count must be added & tested
                gb_assert(toAdd.single->brother.exists==NULL);

                to.full->son[ch] = toAdd.single->son;
                to.full->count[ch] = toAdd.single->count;
                to.full->usedSons++;

                tree = to;

                gbm_free_mem(toAdd.single, sizeof(*(toAdd.single)), GBM_DICT_INDEX);
                (*memcount) -= sizeof(toAdd.single);

                break;
            }
        }
    }

    return tree;
}

static DictTree add_to_dtree(DictTree tree, cu_str text, long len, long *memcount)
/* adds the string 'text' (which has length 'len') to 'tree'
 * returns: new tree
 */
{
    if (tree.exists) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                SingleDictTree *t = tree.single;
                int count = 0;

                do {
                    count++;
                    if (t->ch==text[0]) {                               // we found an existing subtree
                        if (len>1) {
                            t->son = add_to_dtree(t->son, text+1, len-1, memcount);     // add rest of text to subtree
                        }
                        else {
                            gb_assert(len==1);
                            gb_assert(t->son.exists==NULL);
                            t->count++;
                        }

                        return count>MAX_BROTHERS ? single2full_dtree(tree, memcount) : tree;
                    }
                    else if (t->ch > text[0]) {
                        break;
                    }
                }
                while ((t=t->brother.single)!=NULL);

                tree = add_dtree_to_dtree(new_dtree(text, len, memcount),               // otherwise we create a new subtree
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

    return new_dtree(text, len, memcount);
}

static long calcCounts(DictTree tree)
{
    long cnt = 0;

    gb_assert(tree.exists!=0);

    switch (tree.full->typ) {
        case SINGLE_NODE: {
            while (tree.exists) {
                if (tree.single->son.exists) tree.single->count = calcCounts(tree.single->son);
                gb_assert(tree.single->count>0);
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
                    gb_assert(tree.full->count[idx]>0);
                }
                else {
                    gb_assert(tree.full->count[idx]>=0);
                }
                cnt += tree.full->count[idx];
            }
            break;
        }
    }

    return cnt;
}

static int count_dtree_leafs(DictTree tree, int deep, int *maxdeep) {
    // returns # of leafs and max. depth of tree
    int leafs = 0;

    gb_assert(tree.exists!=0);

    if (++deep>*maxdeep) *maxdeep = deep;

    switch (tree.full->typ) {
        case SINGLE_NODE: {
            if (tree.single->son.exists)        leafs += count_dtree_leafs(tree.single->son, deep, maxdeep);
            else                                            leafs++;
            if (tree.single->brother.exists)    leafs += count_dtree_leafs(tree.single->brother, deep, maxdeep);
            break;
        }
        case FULL_NODE: {
            int idx;

            for (idx=0; idx<256; idx++) {
                if (tree.full->son[idx].exists)         leafs += count_dtree_leafs(tree.full->son[idx], deep, maxdeep);
                else if (tree.full->count[idx])         leafs++;
            }
            break;
        }
    }

    return leafs;
}

static int COUNT(DictTree tree) {
    // counts sum of # of occurrences of tree
    int cnt = 0;

    switch (tree.single->typ) {
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

static DictTree removeSubsequentString(DictTree *tree_pntr, cu_str buffer, int len, int max_occur) {
    /* searches tree for 'buffer' (length='len')
     *
     * returns  - rest below found string
     *            (if found and if the # of occurrences of the string is less/equal than 'max_occur')
     *          - NULL otherwise
     *
     * removes the whole found string from the tree (not only the rest!)
     */
    DictTree tree = *tree_pntr, rest;
    static int restCount;

    rest.exists = NULL;

    gb_assert(tree.exists!=0);
    gb_assert(len>0);

    switch (tree.full->typ) {
        case SINGLE_NODE: {
            while (tree.single->ch <= buffer[0]) {
                if (tree.single->ch == buffer[0]) {     // found wanted character
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

                    if (rest.exists) { // the string was found
                        tree.single->count -= restCount;
                        gb_assert(tree.single->count >= 0);

                        if (!tree.single->count) { // empty subtree -> delete myself
                            DictTree brother = tree.single->brother;

                            tree.single->brother.exists = NULL; // elsewise it would be freed by free_dtree
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
                    gb_assert(restCount>0);
                    tree.full->count[ch] -= restCount;
                    gb_assert(tree.full->count[ch]>=0);
                    if (tree.full->count[ch]==0) {
                        gb_assert(tree.full->son[ch].exists==NULL);

                        if (--tree.full->usedSons==0) { // last son deleted -> delete myself
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
    if (!inStringLen) return stringStart; // string of length zero is found everywhere

    while (stringStartLen) {
        cu_str found = (cu_str)memchr(stringStart, inString[0], stringStartLen);

        if (!found) break;

        stringStartLen -= found-stringStart;
        stringStart = found;

        if (stringStartLen<inStringLen) break;

        if (GB_MEMCMP(stringStart, inString, inStringLen)==0) return stringStart;

        stringStart++;
        stringStartLen--;
    }

    return NULL;
}


static int expandBranches(u_str buffer, int deep, int minwordlen, int maxdeep, DictTree tree, DictTree root, int max_percent) {
    /* expands all branches in 'tree'
     *
     * this is done by searching every of these branches in 'root' and moving any subsequent parts from there to 'tree'
     * (this is only done, if the # of occurrences of the found part does not exceed the # of occurrences of 'tree' more than 'max_percent' percent)
     *
     *  'buffer'        strings are rebuild here while descending the tree (length of buffer==MAX_WORD_LEN)
     *  'deep'          recursion level
     *  'maxdeep'       maximum recursion level
     *  'minwordlen'    is the length of the words to search (usually equal to MIN_WORD_LEN-1)
     *
     * returns the # of occurrences which were added to 'tree'
     */
    int expand = 0;             // calculate count-sum of added subsequent parts

    gb_assert(tree.exists!=0);

    if (deep<maxdeep) {
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                while (tree.exists) {
                    buffer[deep] = tree.single->ch;

                    if (!tree.single->son.exists) {
                        DictTree rest;
                        u_str buf = buffer+1;
                        int len = deep;

                        if (len>minwordlen) {           // do not search more than MIN_WORD_LEN-1 chars
                            buf += len-minwordlen;
                            len = minwordlen;
                        }

                        if (len==minwordlen) {
                            cu_str self = memstr(buffer, deep+1, buf, len);

                            gb_assert(self!=0);
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

                    if (!tree.full->son[idx].exists && tree.full->count[idx]) {  // leaf
                        DictTree rest;
                        u_str buf = buffer+1;
                        int len = deep;

                        if (len>minwordlen) {     // do not search more than MIN_WORD_LEN-1 chars
                            buf += len-minwordlen;
                            len = minwordlen;
                        }

                        if (len==minwordlen) {
                            cu_str self = memstr(buffer, deep+1, buf, len);

                            gb_assert(self!=0);
                            if (self==buf)
                                rest = removeSubsequentString(&root, buf, len, ((100+max_percent)*tree.full->count[idx])/100);
                            else
                                rest.exists = NULL;
                        }
                        else {
                            rest.exists = NULL;
                        }

                        if (rest.exists) {      // substring found!
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

static DictTree build_dict_tree(O_gbdByKey *gbk, long maxmem, long maxdeep, size_t minwordlen, long *data_sum)
/* builds a tree of the most used words
 *
 * 'maxmem' is the amount of memory that will be allocated
 * 'maxdeep' is the maximum length of the _returned_ words
 * 'minwordlen' is the minimum length a word needs to get into the tree
 *              this is used in the first pass as maximum tree depth
 * 'data_sum' will be set to the overall-size of data of which the tree was built
 */
{
    DictTree tree;
    long memcount = 0;
    long leafs = 0;

    *data_sum = 0;

    {
        int cnt;
        long lowmem = (maxmem*9)/10;
        int cut_count = 1;

        // Build 8-level-deep tree of all existing words

        tree.exists = NULL;             // empty tree

        for (cnt=0; cnt<gbk->cnt; cnt++) {
            GBDATA *gbd = gbk->gbds[cnt];

            if (COMPRESSIBLE(gbd->type())) {
                size_t size;
                cu_str data = get_data_n_size(gbd, &size);
                cu_str lastWord;

                if (gbd->is_a_string()) size--;
                if (size<minwordlen) continue;

                *data_sum += size;
                lastWord = data+size-minwordlen;

#ifdef SELECT_WORDS
                if (strnstr(data, size, SELECTED_WORDS)) // test some words only
#endif
                {

                    for (; data<=lastWord; data++) {
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

    {
        int cutoff = 1;

        leafs = 0;
        tree  = cut_dtree(tree, cutoff, &memcount, &leafs); // cut all single elements
        test_dtree(tree);

#if defined(DEBUG)
        if (tree.exists) {
            int  maxdeep2 = 0;
            long counted  = count_dtree_leafs(tree, 0, &maxdeep2);
            gb_assert(leafs == counted);
        }
#endif // DEBUG

        // avoid directory overflow (max. 18bit)
        while (leafs >= MAX_LONG_INDEX) {
            leafs = 0;
            ++cutoff;
#if defined(DEBUG)
            printf("Directory overflow (%li) -- reducing size (cutoff = %i)\n", leafs, cutoff);
#endif // DEBUG
            tree  = cut_dtree(tree, cutoff, &memcount, &leafs);
        }
    }
#ifdef DUMP_TREE
    printf("----------------------- tree with short branches:\n");
    dump_dtree(0, tree);
    printf("---------------------------\n");
#endif

    // Try to create longer branches

    if (tree.exists) {
        int add_count;
        u_str buffer = (u_str)gbm_get_mem(maxdeep, GBM_DICT_INDEX);
        int max_differ;
        long dummy;

        if (tree.full->typ != FULL_NODE) tree = single2full_dtree(tree, &memcount);     // ensure root is FULL_NODE

        test_dtree(tree);
        calcCounts(tree);       // calculate counters of inner nodes
        testCounts(tree);

        for (max_differ=0; max_differ<=MAX_DIFFER; max_differ+=INCR_DIFFER) { // percent of allowed difference for concatenating tree branches
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

        gbm_free_mem(buffer, maxdeep, GBM_DICT_INDEX);

        tree = cut_useless_words(tree, 0, &dummy);
    }

#ifdef DUMP_TREE
    printf("----------------------- tree with expanded branches:\n");
    dump_dtree(0, tree);
    printf("-----------------------\n");
#endif
    testCounts(tree);

    return tree;
}

static DictTree remove_word_from_dtree(DictTree tree, cu_str wordStart, int wordLen, u_str resultBuffer, int *resultLen, long *resultFrequency, long *removed) {
    /* searches 'tree' for a word starting with 'wordStart' an removes it from the tree
     * if there are more than one possibilities, the returned word will be the one with the most occurrences
     * if there was no possibility -> resultLen==0, tree unchanged
     * otherwise: resultBuffer contains the word, returns new tree with word removed
     */
    long removed_single = 0;
    gb_assert(tree.exists!=0);
    *removed = 0;

    if (wordLen) {                      // search wanted path into tree
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                if (tree.single->ch==*wordStart) {
                    *resultBuffer = *wordStart;

                    if (tree.single->son.exists) {
                        gb_assert(tree.single->count>0);
                        tree.single->son = remove_word_from_dtree(tree.single->son, wordStart+1, wordLen-1,
                                                                  resultBuffer+1, resultLen, resultFrequency,
                                                                  &removed_single);
                        if (*resultLen) { // word removed
                            gb_assert(tree.single->count>=removed_single);
                            tree.single->count -= removed_single;
                            *removed += removed_single;
                            (*resultLen)++;
                        }
                    }
                    else {
                        *resultLen = wordLen==1;        // if wordLen==1 -> fully overlapping word found
                        *resultFrequency = tree.single->count;
                    }

                    if (!tree.single->son.exists && *resultLen) {       // if no son and a word was found -> remove branch
                        DictTree brother = tree.single->brother;

                        *removed += tree.single->count;
                        gbm_free_mem(tree.single, sizeof(*tree.single), GBM_DICT_INDEX);

                        if (brother.exists)     tree = brother;
                        else                            tree.exists = NULL;
                    }
                }
                else if (tree.single->ch < *wordStart  &&  tree.single->brother.exists) {
                    tree.single->brother = remove_word_from_dtree(tree.single->brother, wordStart, wordLen,
                                                                  resultBuffer, resultLen, resultFrequency,
                                                                  &removed_single);
                    if (*resultLen) *removed += removed_single;
                }
                else {
                    *resultLen = 0; // not found
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
                        if (tree.full->son[ch].exists) { // another son?
                            tree.full->count[ch] -= removed_single;
                        }
                        else {  // last son -> remove whole branch
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
                    *resultLen = 0; // not found
                }

                if (!tree.full->usedSons) {
                    free_dtree(tree);
                    tree.exists = NULL;
                }

                break;
            }
        }
    }
    else {      // take any word
        switch (tree.full->typ) {
            case SINGLE_NODE: {
                *resultBuffer = tree.single->ch;
                gb_assert(tree.single->count>0);

                if (tree.single->son.exists) {
                    tree.single->son = remove_word_from_dtree(tree.single->son, wordStart, wordLen,
                                                              resultBuffer+1, resultLen, resultFrequency,
                                                              &removed_single);
                    gb_assert(*resultLen);
                    (*resultLen)++;
                }
                else {
                    *resultLen = 1;
                    *resultFrequency = tree.single->count;
                    removed_single = tree.single->count;
                }

                gb_assert(*resultFrequency>0);

                if (tree.single->son.exists) {
                    gb_assert(tree.single->count>=removed_single);
                    tree.single->count -= removed_single;
                    *removed += removed_single;
                }
                else {
                    DictTree brother = tree.single->brother;

                    *removed += tree.single->count;
                    gbm_free_mem(tree.single, sizeof(*tree.single), GBM_DICT_INDEX);

                    if (brother.exists) tree = brother;
                    else                tree.exists = NULL;
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
                        gb_assert(*resultLen);
                        (*resultLen)++;

                        if (!tree.full->son[idx].exists) { // son branch removed -> zero count
                            removed_single = tree.full->count[idx];
                            tree.full->count[idx] = 0;
                            tree.full->usedSons--;
                        }
                        else {
                            tree.full->count[idx] -= removed_single;
                            gb_assert(tree.full->count[idx]>0);
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

                gb_assert(idx<256); // gb_assert break was used to exit loop (== node had a son)

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
        gb_assert(*resultLen>0);
        gb_assert(*resultFrequency>0);
        gb_assert(*resultLen>=wordLen);
    }
#endif

    return tree;
}

#define cmp(i1, i2)     (heap2[i1]-heap2[i2])
#define swap(i1, i2) do                         \
    {                                           \
        int s = heap[i1];                       \
        heap[i1] = heap[i2];                    \
        heap[i2] = s;                           \
                                                \
        s = heap2[i1];                          \
        heap2[i1] = heap2[i2];                  \
        heap2[i2] = s;                          \
    }                                           \
    while (0)

static void downheap(int *heap, int *heap2, int me, int num) {
    int lson = me*2;
    int rson = lson+1;

    gb_assert(me>=1);
    if (lson>num) return;

    if (cmp(lson, me)<0) {      // left son smaller than me?  (we sort in descending order!!!)
        if (rson<=num && cmp(lson, rson)>0) { // right son smaller than left son?
            swap(me, rson);
            downheap(heap, heap2, rson, num);
        }
        else {
            swap(me, lson);
            downheap(heap, heap2, lson, num);
        }
    }
    else if (rson<=num && cmp(me, rson)>0) { // right son smaller than me?
        swap(me, rson);
        downheap(heap, heap2, rson, num);
    }
}

#undef cmp
#undef swap



#define cmp(i1, i2)     GB_MEMCMP(dict->text+dict->offsets[heap[i1]], dict->text+dict->offsets[heap[i2]], dict->textlen)
#define swap(i1, i2)    do { int s = heap[i1]; heap[i1] = heap[i2]; heap[i2] = s; } while (0)

static void downheap2(int *heap, GB_DICTIONARY *dict, int me, int num) {
    int lson = me*2;
    int rson = lson+1;

    gb_assert(me>=1);
    if (lson>num) return;

    if (cmp(lson, me)>0) {              // left son bigger than me?
        if (rson<=num && cmp(lson, rson)<0) { // right son bigger than left son?
            swap(me, rson);
            downheap2(heap, dict, rson, num);
        }
        else {
            swap(me, lson);
            downheap2(heap, dict, lson, num);
        }
    }
    else if (rson<=num && cmp(me, rson)<0) { // right son bigger than me?
        swap(me, rson);
        downheap2(heap, dict, rson, num);
    }
}

#undef cmp
#undef swap

static void sort_dict_offsets(GB_DICTIONARY *dict) {
    /* 1. sorts the 'dict->offsets' by frequency
     *    (frequency of each offset is stored in the 'dict->resort' with the same index)
     * 2. initializes & sorts 'dict->resort' in alphabetic order
     */
    int  i;
    int  num   = dict->words;
    int *heap  = dict->offsets-1;
    int *heap2 = dict->resort-1;

    // sort offsets

    for (i=num/2; i>=1; i--) downheap(heap, heap2, i, num);     // make heap

    while (num>1) { // sort heap
        int big = heap[1];
        int big2 = heap2[1];

        heap[1] = heap[num];
        heap2[1] = heap2[num];

        downheap(heap, heap2, 1, num-1);

        heap[num] = big;
        heap2[num] = big2;

        num--;
    }

    // initialize dict->resort

    for (i=0, num=dict->words; i<num; i++) dict->resort[i] = i;

    // sort dictionary alphabetically

    for (i=num/2; i>=1; i--) downheap2(heap2, dict, i, num);    // make heap

    while (num>1) {
        int big = heap2[1];

        heap2[1] = heap2[num];
        downheap2(heap2, dict, 1, num-1);
        heap2[num] = big;
        num--;
    }
}

// Warning dictionary is not in network byte order !!!!
static GB_DICTIONARY *gb_create_dictionary(O_gbdByKey *gbk, long maxmem) {
    long     data_sum;
    DictTree tree = build_dict_tree(gbk, maxmem, MAX_WORD_LEN, MIN_WORD_LEN, &data_sum);

    if (tree.exists) {
        GB_DICTIONARY *dict    = (GB_DICTIONARY*)gbm_get_mem(sizeof(*dict), GBM_DICT_INDEX);
        int            maxdeep = 0;
        int            words   = count_dtree_leafs(tree, 0, &maxdeep);
        u_str          word;

        int   wordLen;
        long  wordFrequency;
        int   offset      = 0;          // next free position in dict->text
        int   overlap     = 0;          // # of bytes overlapping with last word
        u_str buffer;
        long  dummy;
#if defined(DEBUG)
        long  word_sum    = 0;
        long  overlap_sum = 0;
        long  max_overlap = 0;
#endif

        // reduce tree as long as it has to many leafs (>MAX_LONG_INDEX)
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

        dict->words   = 0;
        dict->textlen = DICT_STRING_INCR;

        dict->text    = (u_str)gbm_get_mem(DICT_STRING_INCR, GBM_DICT_INDEX);
        dict->offsets = (GB_NINT*)gbm_get_mem(sizeof(*(dict->offsets))*words, GBM_DICT_INDEX);
        dict->resort  = (GB_NINT*)gbm_get_mem(sizeof(*(dict->resort))*words, GBM_DICT_INDEX);

        memset(buffer, '*', maxdeep);
        tree = remove_word_from_dtree(tree, NULL, 0, buffer, &wordLen, &wordFrequency, &dummy);
        testCounts(tree);

        while (1) {
            int nextWordLen = 0;
            int len;

#if DUMP_COMPRESSION_TEST>=4
            printf("word='%s' (occur=%li overlap=%i)\n", lstr(buffer, wordLen), wordFrequency, overlap);
#endif

#if defined(DEBUG)
            overlap_sum += overlap;
            if (overlap>max_overlap) max_overlap = overlap;
            word_sum += wordLen;
#endif

            if (offset-overlap+wordLen > dict->textlen) { // if not enough space allocated -> reallocate dictionary string
                u_str ntext = (u_str)gbm_get_mem(dict->textlen+DICT_STRING_INCR, GBM_DICT_INDEX);

                memcpy(ntext, dict->text, dict->textlen);
                gbm_free_mem(dict->text, dict->textlen, GBM_DICT_INDEX);

                dict->text = ntext;
                dict->textlen += DICT_STRING_INCR;
            }

            dict->offsets[dict->words] = offset-overlap;
            dict->resort[dict->words] = wordFrequency;                  // temporarily miss-use this to store frequency
            dict->words++;

            word = dict->text+offset-overlap;
            gb_assert(overlap==0 || GB_MEMCMP(word, buffer, overlap)==0); // test overlapping string-part
            memcpy(word, buffer, wordLen);                              // word -> dictionary string
            offset += wordLen-overlap;

            if (!tree.exists) break;

            for (len=min(10, wordLen-1); len>=0 && nextWordLen==0; len--) {
                memset(buffer, '*', maxdeep);
                tree = remove_word_from_dtree(tree, word+wordLen-len, len, buffer, &nextWordLen, &wordFrequency, &dummy);
                overlap = len;
            }

            wordLen = nextWordLen;
        }

        gb_assert(dict->words <= MAX_LONG_INDEX);
        gb_assert(dict->words==words);  /* dict->words == # of words stored in dictionary string
                                         * words           == # of words pre-calculated */

#if DEBUG
        printf("    word_sum=%li overlap_sum=%li (%li%%) max_overlap=%li\n",
               word_sum, overlap_sum, (overlap_sum*100)/word_sum, max_overlap);
#endif

        if (offset<dict->textlen) { // reallocate dict->text if it was allocated too large
            u_str ntext = (u_str)gbm_get_mem(offset, GBM_DICT_INDEX);

            memcpy(ntext, dict->text, offset);
            gbm_free_mem(dict->text, dict->textlen, GBM_DICT_INDEX);

            dict->text = ntext;
            dict->textlen = offset;
        }

        sort_dict_offsets(dict);

        gbm_free_mem(buffer, maxdeep, GBM_DICT_INDEX);
        free_dtree(tree);

        return dict;
    }

    return NULL;
}

static void gb_free_dictionary(GB_DICTIONARY*& dict) {
    gbm_free_mem(dict->text, dict->textlen, GBM_DICT_INDEX);
    gbm_free_mem(dict->offsets, sizeof(*(dict->offsets))*dict->words, GBM_DICT_INDEX);
    gbm_free_mem(dict->resort, sizeof(*(dict->resort))*dict->words, GBM_DICT_INDEX);

    gbm_free_mem(dict, sizeof(*dict), GBM_DICT_INDEX);
    dict = NULL;
}

static GB_ERROR readAndWrite(O_gbdByKey *gbkp, size_t& old_size, size_t& new_size) {
    GB_ERROR error = 0;

    old_size = 0;
    new_size = 0;

    for (int i=0; i<gbkp->cnt && !error; i++) {
        GBDATA *gbd = gbkp->gbds[i];

        if (COMPRESSIBLE(gbd->type())) {
            size_t  size;
            char   *data;

            {
                cu_str d  = (cu_str)get_data_n_size(gbd, &size);
                old_size += gbd->as_entry()->memsize();

                data = (char*)gbm_get_mem(size, GBM_DICT_INDEX);
                memcpy(data, d, size);
                gb_assert(data[size-1] == 0);
            }

            switch (gbd->type()) {
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
                    if (!error) error = GB_write_floats(gbd, (float*)(void*)data, size);
                    break;
                default:
                    gb_assert(0);
                    break;
            }

            new_size += gbd->as_entry()->memsize();

            gbm_free_mem(data, size, GBM_DICT_INDEX);
        }
    }
    return error;
}

static GB_ERROR gb_create_dictionaries(GB_MAIN_TYPE *Main, long maxmem) {
    GB_ERROR error = NULL;
#if defined(TEST_DICT)
    long uncompressed_sum = 0;
    long compressed_sum = 0;
#endif // TEST_DICT

    printf("Creating GBDATA-Arrays..\n");

    if (!error) {
        O_gbdByKey *gbk = g_b_opti_createGbdByKey(Main);
        int         idx = -1;

        printf("Creating dictionaries..\n");

#ifdef DEBUG
        // #define TEST_ONE // test only key specified below
        // #define TEST_SOME // test only some keys specified below
#if defined(TEST_ONE)
        // select wanted index
        for (idx=0; idx<gbdByKey_cnt; idx++) { // title author dew_author ebi_journal name ua_tax date full_name ua_title
            if (gbk[idx].cnt && strcmp(quark2key(Main, idx), "tree")==0) break;
        }
        gb_assert(idx<gbdByKey_cnt);
#endif
#endif

#ifdef TEST_ONE
        // only create dictionary for index selected above (no loop)
#else
        // create dictionaries for all indices (this is the normal operation)
        arb_progress progress("Optimizing key data", gbdByKey_cnt-1);
        for (idx = gbdByKey_cnt-1; idx >= 1 && !error; --idx, progress.inc_and_check_user_abort(error))
#endif

        {
            GB_DICTIONARY *dict;

            GB_CSTR  key_name = quark2key(Main, idx);
            GBDATA  *gb_main  = Main->gb_main();

#ifdef TEST_SOME
            if (!( // add all wanted keys here
                  strcmp(key_name, "REF") == 0 ||
                  strcmp(key_name, "ref") == 0
                  )) continue;
#endif                          // TEST_SOME

#ifndef TEST_ONE
            if (!gbk[idx].cnt) continue; // there are no entries with this quark

            GB_TYPES type = gbk[idx].gbds[0]->type();

            GB_begin_transaction(gb_main);
            int compression_mask = gb_get_compression_mask(Main, idx, type);
            GB_commit_transaction(gb_main);

            if ((compression_mask & GB_COMPRESSION_DICTIONARY) == 0) continue; // compression with dictionary is not allowed
            if (strcmp(key_name, "data")                       == 0) continue;
            if (strcmp(key_name, "quality")                    == 0) continue;
#endif

            printf("- dictionary for '%s' (idx=%i)\n", key_name, idx);
            GB_begin_transaction(gb_main);
            dict = gb_create_dictionary(&(gbk[idx]), maxmem);

            if (dict) {
                /* decompress with old dictionary and write
                   all data of actual type without compression: */

                printf("  * Uncompressing all with old dictionary ...\n");

                size_t old_compressed_size;

                {
                    int& compr_mask     = Main->keys[idx].compression_mask;
                    int  old_compr_mask = compr_mask;

                    size_t new_size;

                    compr_mask &= ~GB_COMPRESSION_DICTIONARY;
                    error       = readAndWrite(&gbk[idx], old_compressed_size, new_size);
                    compr_mask  = old_compr_mask;
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
                    char *dict_buffer      = (char*)gbm_get_mem(dict_buffer_size, GBM_DICT_INDEX);
                    long  old_dict_buffer_size;
                    char *old_dict_buffer;

                    {
                        GB_NINT *nint = (GB_NINT*)dict_buffer;
                        int n;

                        *nint++ = htonl(dict->words);
                        for (n=0; n<dict->words; n++) *nint++ = htonl(dict->offsets[n]);
                        for (n=0; n<dict->words; n++) *nint++ = htonl(dict->resort[n]);

                        memcpy(nint, dict->text, dict->textlen);
                    }

                    const char *key = Main->keys[idx].key;

                    error = gb_load_dictionary_data(gb_main, key, &old_dict_buffer, &old_dict_buffer_size);
                    if (!error) {
                        gb_save_dictionary_data(gb_main, key, dict_buffer, dict_buffer_size);

                        // compress all data with new dictionary
                        printf("  * Compressing all with new dictionary ...\n");

                        size_t old_size, new_compressed_size;
                        error = readAndWrite(&gbk[idx], old_size, new_compressed_size);

                        if (!error) {
                            printf("    (compressed size: old=%zu new=%zu ratio=%.1f%%)\n",
                                   old_compressed_size, new_compressed_size, (new_compressed_size*100.0)/old_compressed_size);

                            // @@@ for some keys compression fails (e.g. 'ref');
                            // need to find out why dictionary is so bad
                            // gb_assert(new_compressed_size <= old_compressed_size); // @@@ enable this (fails in unit-test)
                        }

                        if (error) {
                            /* critical state: new dictionary has been written, but transaction will be aborted below.
                             * Solution: Write back old dictionary.
                             */
                            gb_save_dictionary_data(gb_main, key, old_dict_buffer, old_dict_buffer_size);
                        }
                    }

                    gbm_free_mem(dict_buffer, dict_buffer_size, GBM_DICT_INDEX);
                    if (old_dict_buffer) gbm_free_mem(old_dict_buffer, old_dict_buffer_size, GBM_DICT_INDEX);

#if defined(TEST_DICT)
                    if (!error) {
                        GB_DICTIONARY *dict_reloaded = gb_get_dictionary(Main, idx);
                        test_dictionary(dict_reloaded, &(gbk[idx]), &uncompressed_sum, &compressed_sum);
                    }
#endif // TEST_DICT
                }

                gb_free_dictionary(dict);
            }

            error = GB_end_transaction(gb_main, error);
        }

#ifdef TEST_DICT
        if (!error) {
            printf("    overall uncompressed size = %li b\n"
                   "    overall   compressed size = %li b (Ratio=%li%%)\n",
                   uncompressed_sum, compressed_sum,
                   (compressed_sum*100)/uncompressed_sum);
        }
#endif // TEST_DICT

        printf("Done.\n");

        g_b_opti_freeGbdByKey(gbk);
    }

    return error;
}

GB_ERROR GB_optimize(GBDATA *gb_main) {
    unsigned long maxKB          = GB_get_usable_memory();
    long          maxMem;
    GB_ERROR      error          = 0;
    GB_UNDO_TYPE  prev_undo_type = GB_get_requested_undo_type(gb_main);

#ifdef DEBUG
    maxKB /= 2;
#endif

    if (maxKB<=(LONG_MAX/1024)) maxMem = maxKB*1024;
    else                        maxMem = LONG_MAX;

    error = GB_request_undo_type(gb_main, GB_UNDO_KILL);
    if (!error) {
        error = gb_create_dictionaries(GB_MAIN(gb_main), maxMem);
        if (!error) GB_disable_quicksave(gb_main, "Database optimized");
        ASSERT_NO_ERROR(GB_request_undo_type(gb_main, prev_undo_type));
    }
    return error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

// #define TEST_AUTO_UPDATE // uncomment to auto-update binary result of DB optimization
// #define TEST_AUTO_UPDATE_ASCII // uncomment to auto-update ascii-input from output (be careful with this!)

void TEST_SLOW_optimize() {
    // test DB optimization (optimizes compression)
    // [current coverage ~89%]
    //
    // Note: a test for sequence compression is in adseqcompr.cxx@TEST_SLOW_sequence_compression

    GB_shell shell;

    const char *source_ascii = "TEST_opti_ascii_in.arb";
    const char *target_ascii = "TEST_opti_ascii_out.arb";

    const char *nonopti     = "TEST_opti_none.arb";
    const char *optimized   = "TEST_opti_initial.arb";
    const char *reoptimized = "TEST_opti_again.arb";
    const char *expected    = "TEST_opti_expected.arb"; // expected result after optimize

    // initial optimization of ASCII-DB
    {
        GBDATA *gb_main = GB_open(source_ascii, "rw");

        TEST_EXPECT_NO_ERROR(GB_save_as(gb_main, nonopti, "b"));

        GB_push_my_security(gb_main);
        TEST_EXPECT_NO_ERROR(GB_optimize(gb_main));
        GB_pop_my_security(gb_main);

        GB_flush_cache(gb_main);

        TEST_EXPECT_NO_ERROR(GB_save_as(gb_main, optimized, "b"));
        TEST_EXPECT_NO_ERROR(GB_save_as(gb_main, target_ascii, "a"));

#if defined(TEST_AUTO_UPDATE)
        TEST_COPY_FILE(optimized, expected);
#endif
#if defined(TEST_AUTO_UPDATE_ASCII)
        TEST_COPY_FILE(target_ascii, source_ascii);
#endif

        TEST_EXPECT_TEXTFILE_DIFFLINES(source_ascii, target_ascii, 0);
        TEST_EXPECT_FILES_EQUAL(optimized, expected);

        // check that optimization made sense:
        long nonopti_size   = GB_size_of_file(nonopti);
        long optimized_size = GB_size_of_file(optimized);
        TEST_EXPECT_LESS(optimized_size, nonopti_size);         // did file shrink?
        TEST_EXPECT_EQUAL(optimized_size*100/nonopti_size, 74); // document compression ratio (in percent)

        GB_close(gb_main);
    }

    // re-optimize DB (which is already compressed)
    {
        GBDATA *gb_main = GB_open(optimized, "rw");

        GB_push_my_security(gb_main);
        TEST_EXPECT_NO_ERROR(GB_optimize(gb_main));
        GB_pop_my_security(gb_main);

        TEST_EXPECT_NO_ERROR(GB_save_as(gb_main, reoptimized, "b"));
        TEST_EXPECT_NO_ERROR(GB_save_as(gb_main, target_ascii, "a"));

        TEST_EXPECT_TEXTFILE_DIFFLINES(source_ascii, target_ascii, 0);
        TEST_EXPECT_FILES_EQUAL(reoptimized, expected); // reoptimize should produce same result as initial optimize

        GB_close(gb_main);
    }

    TEST_EXPECT_NO_ERROR(GBK_system(GBS_global_string("rm %s %s %s %s", target_ascii, nonopti, optimized, reoptimized)));
}

void TEST_streamed_ascii_save_asUsedBy_silva_pipeline() {
    GB_shell shell;

    const char *loadname = "TEST_loadsave_ascii.arb";
    const char *savename = "TEST_streamsaved.arb";

    {
        GBDATA *gb_main1 = GB_open(loadname, "r");  TEST_REJECT_NULL(gb_main1);
        GBDATA *gb_main2 = GB_open(loadname, "rw"); TEST_REJECT_NULL(gb_main2);

        // delete all species from DB2
        GBDATA *gb_species_data2 = NULL;
        {
            GB_transaction ta(gb_main2);
            GB_push_my_security(gb_main2);
            gb_species_data2 = GBT_get_species_data(gb_main2);
            for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data2);
                 gb_species;
                 gb_species = GBT_next_species(gb_species))
            {
                TEST_EXPECT_NO_ERROR(GB_delete(gb_species));
            }
            GB_pop_my_security(gb_main2);
        }

        for (int saveWhileTransactionOpen = 0; saveWhileTransactionOpen<=1; ++saveWhileTransactionOpen) {
            {
                ArbDBWriter *writer = NULL;
                TEST_EXPECT_NO_ERROR(GB_start_streamed_save_as(gb_main2, savename, "a", writer));
                TEST_EXPECT_NO_ERROR(GB_stream_save_part(writer, gb_main2, gb_species_data2));

                {
                    GB_transaction ta1(gb_main1);
                    GB_transaction ta2(gb_main2);

                    for (GBDATA *gb_species1 = GBT_first_species(gb_main1);
                         gb_species1;
                         gb_species1 = GBT_next_species(gb_species1))
                    {
                        GBDATA *gb_species2 = GB_create_container(gb_species_data2, "species");
                        if (!gb_species2) {
                            TEST_EXPECT_NO_ERROR(GB_await_error());
                        }
                        else {
                            TEST_EXPECT_NO_ERROR(GB_copy_with_protection(gb_species2, gb_species1, true));
                            GB_write_flag(gb_species2, GB_read_flag(gb_species1)); // copy marked flag
                        }

                        if (!saveWhileTransactionOpen) GB_commit_transaction(gb_main2);
                        TEST_EXPECT_NO_ERROR(GB_stream_save_part(writer, gb_species2, gb_species2));
                        if (!saveWhileTransactionOpen) GB_begin_transaction(gb_main2);

                        GB_push_my_security(gb_main2);
                        TEST_EXPECT_NO_ERROR(GB_delete(gb_species2));
                        GB_pop_my_security(gb_main2);
                    }
                }

                TEST_EXPECT_NO_ERROR(GB_stream_save_part(writer, gb_species_data2, gb_main2));
                TEST_EXPECT_NO_ERROR(GB_finish_stream_save(writer));
            }

            // test file content
            TEST_EXPECT_TEXTFILES_EQUAL(savename, loadname);
            TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(savename));
        }

        // test some error cases:
        {
            // 1. stream binary (not supported)
            ArbDBWriter *writer = NULL;
            TEST_EXPECT_NO_ERROR(GB_start_streamed_save_as(gb_main2, savename, "b", writer));
            TEST_EXPECT_ERROR_CONTAINS(GB_stream_save_part(writer, gb_main2, gb_species_data2), "only supported for ascii");
            GB_finish_stream_save(writer);
            TEST_EXPECT(!GB_is_regularfile(savename)); // no partial file remains

            // 2. invalid use of GB_stream_save_part (not ancestors)
            TEST_EXPECT_NO_ERROR(GB_start_streamed_save_as(gb_main2, savename, "a", writer));
            TEST_EXPECT_NO_ERROR(GB_stream_save_part(writer, gb_main2, gb_species_data2));
            GBDATA *gb_extended_data2;
            {
                GB_transaction ta(gb_main2);
                gb_extended_data2 = GBT_get_SAI_data(gb_main2);
            }
            TEST_EXPECT_ERROR_CONTAINS(GB_stream_save_part(writer, gb_species_data2, gb_extended_data2), "has to be an ancestor");
            GB_finish_stream_save(writer);
            TEST_EXPECT(!GB_is_regularfile(savename)); // no partial file remains

        }
        // TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(savename));

        GB_close(gb_main2);
        GB_close(gb_main1);
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

