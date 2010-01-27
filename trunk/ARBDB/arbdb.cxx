// =============================================================== //
//                                                                 //
//   File      : arbdb.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#if defined(DARWIN)
#include <cstdio>
#endif // DARWIN

#include <rpc/xdr.h>

#include "gb_cb.h"
#include "gb_comm.h"
#include "gb_compress.h"
#include "gb_localdata.h"
#include "gb_ta.h"
#include "gb_ts.h"


struct gb_local_data *gb_local = 0;

#ifdef ARBDB_SIZEDEBUG
long *arbdb_stat;
#endif

#if defined(DEVEL_RALF)
#warning convert the following macros to inline functions (and rename them)
#endif // DEVEL_RALF


#define GB_TEST_READ(gbd, ty, error)                                    \
    do {                                                                \
        GB_TEST_TRANSACTION(gbd);                                       \
        if (GB_ARRAY_FLAGS(gbd).changed == GB_DELETED) {                \
            GB_internal_errorf("%s: %s", error, "Entry is deleted !!"); \
            return 0; }                                                 \
        if (GB_TYPE(gbd) != ty && (ty != GB_STRING || GB_TYPE(gbd) == GB_LINK)) { \
            GB_internal_errorf("%s: %s", error, "wrong type");          \
            return 0; }                                                 \
    } while (0)

#define GB_TEST_WRITE(gbd, ty, gerror)                                  \
    do {                                                                \
        GB_TEST_TRANSACTION(gbd);                                       \
        if (GB_ARRAY_FLAGS(gbd).changed == GB_DELETED) {                \
            GB_internal_errorf("%s: %s", gerror, "Entry is deleted !!"); \
            return 0; }                                                 \
        if (GB_TYPE(gbd) != ty && (ty != GB_STRING || GB_TYPE(gbd) != GB_LINK)) { \
            GB_internal_errorf("%s: %s", gerror, "type conflict !!");   \
            return 0; }                                                 \
        if (GB_GET_SECURITY_WRITE(gbd)>GB_MAIN(gbd)->security_level)    \
            return gb_security_error(gbd);                              \
    } while (0)

#define GB_TEST_NON_BUFFER(x, gerror)                                   \
    do {                                                                \
        if (GB_is_in_buffer(x)) {                                       \
            GBK_terminatef("%s: you are not allowed to write any data, which you get by pntr", gerror); \
        }                                                               \
    } while (0)


char *GB_rel(void *struct_address, long rel_address)
{
    if (!rel_address) return NULL;
    return (char*)struct_address+rel_address;
}

NOT4PERL GB_ERROR GB_safe_atof(const char *str, double *res) {
    GB_ERROR  error = NULL;
    char     *end;
    *res            = strtod(str, &end);
    if (end == str || end[0] != 0) {
        error = GBS_global_string("cannot convert '%s' to double", str);
    }
    return error;
}

double GB_atof(const char *str) {
    // convert ASCII to double

    double res     = 0;
#if defined(DEBUG)
    GB_ERROR error = 
#endif // DEBUG
        GB_safe_atof(str, &res);

    gb_assert(!error); // expected double in 'str' - better use GB_safe_atof()

    return res;
}

// ---------------------------
//      compression tables

const int gb_convert_type_2_compression_flags[] = {
    GB_COMPRESSION_NONE,                                                                 // GB_NONE  0
    GB_COMPRESSION_NONE,                                                                 // GB_BIT   1
    GB_COMPRESSION_NONE,                                                                 // GB_BYTE  2
    GB_COMPRESSION_NONE,                                                                 // GB_INT   3
    GB_COMPRESSION_NONE,                                                                 // GB_FLOAT 4
    GB_COMPRESSION_NONE,                                                                 // GB_??    5
    GB_COMPRESSION_BITS,                                                                 // GB_BITS  6
    GB_COMPRESSION_NONE,                                                                 // GB_??    7
    GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN,                                  // GB_BYTES 8
    GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN | GB_COMPRESSION_SORTBYTES,       // GB_INTS  9
    GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN | GB_COMPRESSION_SORTBYTES,       // GB_FLTS 10
    GB_COMPRESSION_NONE,                                                                 // GB_LINK 11
    GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN | GB_COMPRESSION_DICTIONARY,      // GB_STR  12
    GB_COMPRESSION_NONE,                                                                 // GB_STRS 13
    GB_COMPRESSION_NONE,                                                                 // GB??    14
    GB_COMPRESSION_NONE                                                                  // GB_DB   15
};

int gb_convert_type_2_sizeof[] = { /* contains the unit-size of data stored in DB,
                                    * i.e. realsize = unit_size * GB_GETSIZE()
                                    */
    0,                                              // GB_NONE  0
    0,                                              // GB_BIT   1
    sizeof(char),                                   // GB_BYTE  2
    sizeof(int),                                    // GB_INT   3
    sizeof(float),                                  // GB_FLOAT 4
    0,                                              // GB_??    5
    0,                                              // GB_BITS  6
    0,                                              // GB_??    7
    sizeof(char),                                   // GB_BYTES 8
    sizeof(int),                                    // GB_INTS  9
    sizeof(float),                                  // GB_FLTS 10
    sizeof(char),                                   // GB_LINK 11
    sizeof(char),                                   // GB_STR  12
    sizeof(char),                                   // GB_STRS 13
    0,                                              // GB_??   14
    0,                                              // GB_DB   15
};

int gb_convert_type_2_appendix_size[] = { /* contains the size of the suffix (aka terminator element)
                                           * size is in bytes
                                           */

    0,                                              // GB_NONE  0
    0,                                              // GB_BIT   1
    0,                                              // GB_BYTE  2
    0,                                              // GB_INT   3
    0,                                              // GB_FLOAT 4
    0,                                              // GB_??    5
    0,                                              // GB_BITS  6
    0,                                              // GB_??    7
    0,                                              // GB_BYTES 8
    0,                                              // GB_INTS  9
    0,                                              // GB_FLTS 10
    1,                                              // GB_LINK 11 (zero terminated)
    1,                                              // GB_STR  12 (zero terminated)
    1,                                              // GB_STRS 13 (zero terminated)
    0,                                              // GB_??   14
    0,                                              // GB_DB   15
};


// ---------------------------------
//      local buffer management

static void init_buffer(struct gb_buffer *buf, size_t initial_size) {
    buf->size = initial_size;
    buf->mem  = buf->size ? (char*)malloc(buf->size) : NULL;
}

static char *check_out_buffer(struct gb_buffer *buf) {
    char *checkOut = buf->mem;

    buf->mem  = 0;
    buf->size = 0;

    return checkOut;
}

static void alloc_buffer(struct gb_buffer *buf, size_t size) {
    free(buf->mem);
    buf->size = size;
#if (MEMORY_TEST==1)
    buf->mem  = (char *)malloc(buf->size);
#else
    buf->mem  = (char *)GB_calloc(buf->size, 1);
#endif
}

static GB_BUFFER give_buffer(struct gb_buffer *buf, size_t size) {
#if (MEMORY_TEST==1)
    alloc_buffer(buf, size); // do NOT reuse buffer if testing memory
#else
    if (size >= buf->size) {
        alloc_buffer(buf, size);
    }
#endif
    return buf->mem;
}

static int is_in_buffer(struct gb_buffer *buf, GB_CBUFFER ptr) {
    return ptr >= buf->mem && ptr < buf->mem+buf->size;
}

// ------------------------------

GB_BUFFER GB_give_buffer(size_t size) {
    // return a pointer to a static piece of memory at least size bytes long
    return give_buffer(&gb_local->buf1, size);
}

GB_BUFFER GB_increase_buffer(size_t size) {
    if (size < gb_local->buf1.size) {
        char   *old_buffer = gb_local->buf1.mem;
        size_t  old_size   = gb_local->buf1.size;

        gb_local->buf1.mem = NULL;
        alloc_buffer(&gb_local->buf1, size);
        memcpy(gb_local->buf1.mem, old_buffer, old_size);

        free(old_buffer);
    }
    return gb_local->buf1.mem;
}

NOT4PERL int GB_give_buffer_size() {
    return gb_local->buf1.size;
}

GB_BUFFER GB_give_buffer2(long size) {
    return give_buffer(&gb_local->buf2, size);
}

int GB_is_in_buffer(GB_CBUFFER ptr) {
    /* returns 1 or 2 if 'ptr' points to gb_local->buf1/buf2
     * returns 0 otherwise
     */
    int buffer = 0;

    if (is_in_buffer(&gb_local->buf1, ptr)) buffer = 1;
    else if (is_in_buffer(&gb_local->buf2, ptr)) buffer = 2;

    return buffer;
}

char *GB_check_out_buffer(GB_CBUFFER buffer) {
    /* Check a piece of memory out of the buffer management
     * after it is checked out, the user has the full control to use and free it
     * Returns a pointer to the start of the buffer (even if 'buffer' points inside the buffer!)
     */
    char *old = 0;

    if (is_in_buffer(&gb_local->buf1, buffer)) old = check_out_buffer(&gb_local->buf1);
    else if (is_in_buffer(&gb_local->buf2, buffer)) old = check_out_buffer(&gb_local->buf2);

    return old;
}

GB_BUFFER GB_give_other_buffer(GB_CBUFFER buffer, long size) {
    return is_in_buffer(&gb_local->buf1, buffer)
        ? GB_give_buffer2(size)
        : GB_give_buffer(size);
}

unsigned char GB_BIT_compress_data[] = {
    0x1d, GB_CS_OK,  0, 0,
    0x04, GB_CS_OK,  0, 1,
    0x0a, GB_CS_OK,  0, 2,
    0x0b, GB_CS_OK,  0, 3,
    0x0c, GB_CS_OK,  0, 4,
    0x1a, GB_CS_OK,  0, 5,
    0x1b, GB_CS_OK,  0, 6,
    0x1c, GB_CS_OK,  0, 7,
    0xf0, GB_CS_OK,  0, 8,
    0xf1, GB_CS_OK,  0, 9,
    0xf2, GB_CS_OK,  0, 10,
    0xf3, GB_CS_OK,  0, 11,
    0xf4, GB_CS_OK,  0, 12,
    0xf5, GB_CS_OK,  0, 13,
    0xf6, GB_CS_OK,  0, 14,
    0xf7, GB_CS_OK,  0, 15,
    0xf8, GB_CS_SUB, 0, 16,
    0xf9, GB_CS_SUB, 0, 32,
    0xfa, GB_CS_SUB, 0, 48,
    0xfb, GB_CS_SUB, 0, 64,
    0xfc, GB_CS_SUB, 0, 128,
    0xfd, GB_CS_SUB, 1, 0,
    0xfe, GB_CS_SUB, 2, 0,
    0xff, GB_CS_SUB, 4, 0,
    0
};

void GB_init_gb(void) {
    if (!gb_local) {
        gb_local = (struct gb_local_data *)gbm_get_mem(sizeof(struct gb_local_data), 0);

        init_buffer(&gb_local->buf1, 4000);
        init_buffer(&gb_local->buf2, 4000);

        gb_local->write_bufsize = GBCM_BUFFER;
        gb_local->write_buffer  = (char *)malloc((size_t)gb_local->write_bufsize);
        gb_local->write_ptr     = gb_local->write_buffer;
        gb_local->write_free    = gb_local->write_bufsize;
        
        gb_local->bituncompress = gb_build_uncompress_tree(GB_BIT_compress_data, 1, 0);
        gb_local->bitcompress   = gb_build_compress_list(GB_BIT_compress_data, 1, &(gb_local->bc_size));

#ifdef ARBDB_SIZEDEBUG
        arbdb_stat = (long *)GB_calloc(sizeof(long), 1000);
#endif
    }
}

GB_ERROR gb_unfold(GBCONTAINER *gbd, long deep, int index_pos) {
    /*! get data from server.
     *
     * @param deep if != 0, then get subitems too.
     * @param index_pos
     * - >= 0, get indexed item from server
     * - <0, get all items
     *
     * @return error on failure
     */

    GB_ERROR  error;
    GBDATA   *gb2;
    
    struct gb_header_list_struct *header = GB_DATA_LIST_HEADER(gbd->d);

    if (!gbd->flags2.folded_container) return 0;
    if (index_pos> gbd->d.nheader) gb_create_header_array(gbd, index_pos + 1);
    if (index_pos >= 0  && GB_HEADER_LIST_GBD(header[index_pos])) return 0;

    if (GBCONTAINER_MAIN(gbd)->local_mode) {
        GB_internal_error("Cannot unfold local_mode database");
        return 0;
    }

    do {
        if (index_pos<0) break;
        if (index_pos >= gbd->d.nheader) break;
        if (header[index_pos].flags.changed >= GB_DELETED) {
            GB_internal_error("Tried to unfold a deleted item");
            return 0;
        }
        if (GB_HEADER_LIST_GBD(header[index_pos])) return 0;            // already unfolded
    } while (0);

    error = gbcm_unfold_client(gbd, deep, index_pos);
    if (error) {
        GB_print_error();
        return error;
    }

    if (index_pos<0) {
        gb_untouch_children(gbd);
        gbd->flags2.folded_container = 0;
    }
    else {
        if ((gb2 = GBCONTAINER_ELEM(gbd, index_pos))) {
            if (GB_TYPE(gb2) == GB_DB) gb_untouch_children((GBCONTAINER *)gb2);
            gb_untouch_me(gb2);
        }
    }
    return 0;
}

// -----------------------
//      close database

typedef void (*gb_close_callback)(GBDATA *gb_main, void *client_data);

struct gb_close_callback_struct {
    struct gb_close_callback_struct *next;

    gb_close_callback  cb;
    void              *client_data;
};

#if defined(DEBUG)
static bool atclose_cb_exists(struct gb_close_callback_struct *gccs, gb_close_callback cb) {
    return gccs && (gccs->cb == cb || atclose_cb_exists(gccs->next, cb));
}
#endif // DEBUG

void GB_atclose(GBDATA *gbd, void (*fun)(GBDATA *gb_main, void *client_data), void *client_data) {
    /*! Add a callback, which gets called directly before GB_close destroys all data.
     * This is the recommended way to remove all callbacks from DB elements.
     */

    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    gb_assert(!atclose_cb_exists(Main->close_callbacks, fun)); // each close callback should only exist once

    struct gb_close_callback_struct *gccs = (struct gb_close_callback_struct *)malloc(sizeof(*gccs));

    gccs->next        = Main->close_callbacks;
    gccs->cb          = fun;
    gccs->client_data = client_data;

    Main->close_callbacks = gccs;
}

static void run_close_callbacks(GBDATA *gb_main, struct gb_close_callback_struct *gccs) {
    while (gccs) {
        gccs->cb(gb_main, gccs->client_data);
        struct gb_close_callback_struct *next = gccs->next;
        free(gccs);
        gccs = next;
    }
}

void GB_close(GBDATA *gbd) {
    GB_ERROR error = 0;

    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    gb_assert(Main->transaction == 0); // you can't close DB if there is still an open transaction!

    if (!Main->local_mode) {
        long result = gbcmc_close(Main->c_link);
        if (result != 0) error = GBS_global_string("gbcmc_close returns %li", result);
    }

    if (!error) {
        gb_assert((GBDATA*)Main->data == gbd);

        run_close_callbacks(gbd, Main->close_callbacks);
        Main->close_callbacks = 0;

        gb_delete_main_entry(&gbd);

        /* ARBDB applications using awars easily crash in gb_do_callback_list(),
         * if AWARs are still bound to elements in the closed database.
         * 
         * To unlink awars call AW_root::unlink_awars_from_DB().
         * If that doesn't help, test Main->data (often aka as GLOBAL_gb_main)
         */
        Main->data = NULL;
        gb_do_callback_list(Main);                  // do all callbacks

        gb_destroy_main(Main);
    }

    if (error) {
        GB_warningf("Error in GB_close: %s", error);
    }
}


// ------------------
//      read data

long GB_read_int(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_INT, "GB_read_int");
    return gbd->info.i;
}

int GB_read_byte(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_BYTE, "GB_read_byte");
    return gbd->info.i;
}

void *GB_read_pointer(GBDATA *gbd) {
    GB_TEST_READ(gbd, GB_POINTER, "GB_read_pointer");
    return gbd->info.ptr;
}

double GB_read_float(GBDATA *gbd)
{
    XDR xdrs;
    static float f;
    GB_TEST_READ(gbd, GB_FLOAT, "GB_read_float");
    xdrmem_create(&xdrs, &gbd->info.in.data[0], SIZOFINTERN, XDR_DECODE);
    xdr_float(&xdrs, &f);
    xdr_destroy(&xdrs);
    return (double)f;
}

long GB_read_count(GBDATA *gbd) {
    return GB_GETSIZE(gbd);
}

long GB_read_memuse(GBDATA *gbd) {
    return GB_GETMEMSIZE(gbd);
}

GB_CSTR GB_read_pntr(GBDATA *gbd) {
    int   type = GB_TYPE(gbd);
    char *data = GB_GETDATA(gbd);
    
    if (data) {
        if (gbd->flags.compressed_data) {   // uncompressed data return pntr to database entry
            char *ca = gb_read_cache(gbd);

            if (!ca) {
                long        size = GB_UNCOMPRESSED_SIZE(gbd, type);
                const char *da   = gb_uncompress_data(gbd, data, size);
                
                if (da) {
                    ca = gb_alloc_cache_index(gbd, size);
                    memcpy(ca, da, size);
                }
            }
            data = ca;
        }
    }
    return data;
}

int gb_read_nr(GBDATA *gbd) {
    return gbd->index;
}

GB_CSTR GB_read_char_pntr(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_STRING, "GB_read_char_pntr");
    return GB_read_pntr(gbd);
}

char *GB_read_string(GBDATA *gbd)
{
    const char *d;
    GB_TEST_READ(gbd, GB_STRING, "GB_read_string");
    d = GB_read_pntr(gbd);
    if (!d) return NULL;
    return GB_memdup(d, GB_GETSIZE(gbd)+1);
}

long GB_read_string_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_STRING, "GB_read_string_count");
    return GB_GETSIZE(gbd);
}

GB_CSTR GB_read_link_pntr(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_LINK, "GB_read_link_pntr");
    return GB_read_pntr(gbd);
}

char *GB_read_link(GBDATA *gbd)
{
    const char *d;
    GB_TEST_READ(gbd, GB_LINK, "GB_read_link_pntr");
    d = GB_read_pntr(gbd);
    if (!d) return NULL;
    return GB_memdup(d, GB_GETSIZE(gbd)+1);
}

long GB_read_link_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_LINK, "GB_read_link_pntr");
    return GB_GETSIZE(gbd);
}


long GB_read_bits_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_BITS, "GB_read_bits_count");
    return GB_GETSIZE(gbd);
}

GB_CSTR GB_read_bits_pntr(GBDATA *gbd, char c_0, char c_1)
{
    char *data;
    long size;
    GB_TEST_READ(gbd, GB_BITS, "GB_read_bits_pntr");
    data = GB_GETDATA(gbd);
    size = GB_GETSIZE(gbd);
    if (!size) return 0;
    {
        char *ca = gb_read_cache(gbd);
        char *da;
        if (ca) return ca;
        ca = gb_alloc_cache_index(gbd, size+1);
        da = gb_uncompress_bits(data, size, c_0, c_1);
        if (ca) {
            memcpy(ca, da, size+1);
            return ca;
        }
        else {
            return da;
        }
    }
}

char *GB_read_bits(GBDATA *gbd, char c_0, char c_1) {
    GB_CSTR d = GB_read_bits_pntr(gbd, c_0, c_1);
    return d ? GB_memdup(d, GB_GETSIZE(gbd)+1) : 0;
}


GB_CSTR GB_read_bytes_pntr(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_BYTES, "GB_read_bytes_pntr");
    return GB_read_pntr(gbd);
}

long GB_read_bytes_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_BYTES, "GB_read_bytes_count");
    return GB_GETSIZE(gbd);
}

char *GB_read_bytes(GBDATA *gbd) {
    GB_CSTR d = GB_read_bytes_pntr(gbd);
    return d ? GB_memdup(d, GB_GETSIZE(gbd)) : 0;
}

GB_CUINT4 *GB_read_ints_pntr(GBDATA *gbd)
{
    GB_UINT4 *res;
    GB_TEST_READ(gbd, GB_INTS, "GB_read_ints_pntr");

    if (gbd->flags.compressed_data) {
        res = (GB_UINT4 *)GB_read_pntr(gbd);
    }
    else {
        res = (GB_UINT4 *)GB_GETDATA(gbd);
    }
    if (!res) return NULL;

    if (0x01020304 == htonl((u_long)0x01020304)) {
        return res;
    }
    else {
        long      i;
        int       size = GB_GETSIZE(gbd);
        char     *buf2 = GB_give_other_buffer((char *)res, size<<2);
        GB_UINT4 *s    = (GB_UINT4 *)res;
        GB_UINT4 *d    = (GB_UINT4 *)buf2;

        for (i=size; i; i--) {
            *(d++) = htonl(*(s++));
        }
        return (GB_UINT4 *)buf2;
    }
}

long GB_read_ints_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_INTS, "GB_read_ints_count");
    return GB_GETSIZE(gbd);
}

GB_UINT4 *GB_read_ints(GBDATA *gbd)
{
    GB_CUINT4 *i = GB_read_ints_pntr(gbd);
    if (!i) return NULL;
    return  (GB_UINT4 *)GB_memdup((char *)i, GB_GETSIZE(gbd)*sizeof(GB_UINT4));
}

GB_CFLOAT *GB_read_floats_pntr(GBDATA *gbd)
{
    char *buf2;
    char *res;

    GB_TEST_READ(gbd, GB_FLOATS, "GB_read_floats_pntr");
    if (gbd->flags.compressed_data) {
        res = (char *)GB_read_pntr(gbd);
    }
    else {
        res = (char *)GB_GETDATA(gbd);
    }
    if (!res) return NULL;
    {
        XDR    xdrs;
        float *d;
        long   i;
        long   size      = GB_GETSIZE(gbd);
        long   full_size = size*sizeof(float);

        xdrmem_create(&xdrs, res, (int)(full_size), XDR_DECODE);
        buf2 = GB_give_other_buffer(res, full_size);
        d = (float *)buf2;
        for (i=size; i; i--) {
            xdr_float(&xdrs, d);
            d++;
        }
        xdr_destroy(&xdrs);
    }
    return (float *)buf2;
}

long GB_read_floats_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_FLOATS, "GB_read_floats_count");
    return GB_GETSIZE(gbd);
}

float *GB_read_floats(GBDATA *gbd)
{
    GB_CFLOAT *f;
    f = GB_read_floats_pntr(gbd);
    if (!f) return NULL;
    return  (float *)GB_memdup((char *)f, GB_GETSIZE(gbd)*sizeof(float));
}

char *GB_read_as_string(GBDATA *gbd)
{
    switch (GB_TYPE(gbd)) {
        case GB_STRING: return GB_read_string(gbd);
        case GB_LINK:   return GB_read_link(gbd);
        case GB_BYTE:   return GBS_global_string_copy("%i", GB_read_byte(gbd));
        case GB_INT:    return GBS_global_string_copy("%li", GB_read_int(gbd));
        case GB_FLOAT:  return GBS_global_string_copy("%g", GB_read_float(gbd));
        case GB_BITS:   return GB_read_bits(gbd, '0', '1');
            /* Be careful : When adding new types here, you have to make sure that
             * GB_write_as_string is able to write them back and that this makes sense.
             */
        default:    return NULL;
    }
}

// ------------------------------------------------------------
//      array type access functions (intended for perl use)

long GB_read_from_ints(GBDATA *gbd, long index) {
    static GBDATA    *last_gbd = 0;
    static long       count    = 0;
    static GB_CUINT4 *i        = 0;

    if (gbd != last_gbd) {
        count    = GB_read_ints_count(gbd);
        i        = GB_read_ints_pntr(gbd);
        last_gbd = gbd;
    }

    if (index >= 0 && index < count) {
        return i[index];
    }
    return -1;
}

double GB_read_from_floats(GBDATA *gbd, long index) {
    static GBDATA    *last_gbd = 0;
    static long       count    = 0;
    static GB_CFLOAT *f        = 0;

    if (gbd != last_gbd) {
        count    = GB_read_floats_count(gbd);
        f        = GB_read_floats_pntr(gbd);
        last_gbd = gbd;
    }

    if (index >= 0 && index < count) {
        return f[index];
    }
    return -1;
}

// -------------------
//      write data

static void gb_do_callbacks(GBDATA *gbd) {
    gb_assert(GB_MAIN(gbd)->transaction<0); // only use in "no transaction mode"!

    GBDATA *gbdn, *gbdc;
    for (gbdc = gbd; gbdc; gbdc=gbdn) {
        struct gb_callback *cb, *cbn;
        gbdn = GB_get_father(gbdc);
        for (cb = GB_GET_EXT_CALLBACKS(gbdc); cb; cb = cbn) {
            cbn = cb->next;
            if (cb->type & GB_CB_CHANGED) {
                ++cb->running;
                cb->func(gbdc, cb->clientdata, GB_CB_CHANGED);
                --cb->running;
            }
        }
    }
}

#define GB_DO_CALLBACKS(gbd) do { if (GB_MAIN(gbd)->transaction<0) gb_do_callbacks(gbd); } while (0)

GB_ERROR GB_write_byte(GBDATA *gbd, int i)
{
    GB_TEST_WRITE(gbd, GB_BYTE, "GB_write_byte");
    if (gbd->info.i != i) {
        gb_save_extern_data_in_ts(gbd);
        gbd->info.i = i & 0xff;
        gb_touch_entry(gbd, GB_NORMAL_CHANGE);
        GB_DO_CALLBACKS(gbd);
    }
    return 0;
}

GB_ERROR GB_write_int(GBDATA *gbd, long i) {
#if defined(ARB64)
#if defined(DEVEL_RALF)
#warning GB_write_int should be GB_ERROR GB_write_int(GBDATA *gbd,int32_t i) 
#endif // DEVEL_RALF
#endif // ARB64

    GB_TEST_WRITE(gbd, GB_INT, "GB_write_int");
    if ((long)((int32_t)i) != i) {
        gb_assert(0);
        GB_warningf("Warning: 64bit incompatibility detected\nNo data written to '%s'\n", GB_get_db_path(gbd));
        return "GB_INT out of range (signed, 32bit)";
    }
    if (gbd->info.i != (int32_t)i) {
        gb_save_extern_data_in_ts(gbd);
        gbd->info.i = i;
        gb_touch_entry(gbd, GB_NORMAL_CHANGE);
        GB_DO_CALLBACKS(gbd);
    }
    return 0;
}

GB_ERROR GB_write_pointer(GBDATA *gbd, void *pointer) {
    GB_TEST_WRITE(gbd, GB_POINTER, "GB_write_pointer");
    if (gbd->info.ptr != pointer) {
        gb_save_extern_data_in_ts(gbd);
        gbd->info.ptr = pointer;
        gb_touch_entry(gbd, GB_NORMAL_CHANGE);
        GB_DO_CALLBACKS(gbd);
    }
    return 0;
}

GB_ERROR GB_write_float(GBDATA *gbd, double f)
{
    XDR xdrs;
    static float f2;

    GB_TEST_WRITE(gbd, GB_FLOAT, "GB_write_float");

    GB_TEST_READ(gbd, GB_FLOAT, "GB_read_float");

    xdrmem_create(&xdrs, &gbd->info.in.data[0], SIZOFINTERN, XDR_DECODE);
    xdr_float(&xdrs, &f2);
    xdr_destroy(&xdrs);

    if (f2 != f) {
        f2 = f;
        gb_save_extern_data_in_ts(gbd);
        xdrmem_create(&xdrs, &gbd->info.in.data[0], SIZOFINTERN, XDR_ENCODE);
        xdr_float(&xdrs, &f2);
        xdr_destroy(&xdrs);
        gb_touch_entry(gbd, GB_NORMAL_CHANGE);
        GB_DO_CALLBACKS(gbd);
    }
    xdr_destroy(&xdrs);
    return 0;
}

GB_ERROR gb_write_compressed_pntr(GBDATA *gbd, const char *s, long memsize, long stored_size) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    gb_free_cache(Main, gbd);
    gb_save_extern_data_in_ts(gbd);
    gbd->flags.compressed_data = 1;
    GB_SETSMDMALLOC(gbd, stored_size, (size_t)memsize, (char *)s);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    return 0;
}

int gb_get_compression_mask(GB_MAIN_TYPE *Main, GBQUARK key, int gb_type)
{
    struct gb_key_struct *ks = &Main->keys[key];
    int compression_mask;

    if (ks->gb_key_disabled) {
        compression_mask = 0;
    }
    else {
        if (!ks->gb_key) gb_load_single_key_data((GBDATA*)Main->data, key);
        compression_mask = gb_convert_type_2_compression_flags[gb_type] & ks->compression_mask;
    }

    return compression_mask;
}

GB_ERROR GB_write_pntr(GBDATA *gbd, const char *s, long bytes_size, long stored_size)
{
    // 'bytes_size' is the size of what 's' points to.
    // 'stored_size' is the size-information written into the DB
    // 
    // e.g. for strings : stored_size = bytes_size-1, cause stored_size is string len,
    //                    but bytes_size includes zero byte.


    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GBQUARK       key  = GB_KEY_QUARK(gbd);
    const char   *d;
    int           compression_mask;
    long          memsize;
    GB_TYPES      type = GB_TYPE(gbd);

    gb_assert(type != GB_STRING || (stored_size == bytes_size-1)); // size constraint for strings not fulfilled!

    gb_free_cache(Main, gbd);
    gb_save_extern_data_in_ts(gbd);

    compression_mask = gb_get_compression_mask(Main, key, type);

    if (compression_mask) {
        d = gb_compress_data(gbd, key, s, bytes_size, &memsize, compression_mask, false);
    }
    else {
        d = NULL;
    }
    if (d) {
        gbd->flags.compressed_data = 1;
    }
    else {
        d = s;
        gbd->flags.compressed_data = 0;
        memsize = bytes_size;
    }

    GB_SETSMDMALLOC(gbd, stored_size, memsize, d);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);

    return 0;
}

GB_ERROR GB_write_string(GBDATA *gbd, const char *s)
{
    long size;
    GB_TEST_WRITE(gbd, GB_STRING, "GB_write_string");
    GB_TEST_NON_BUFFER(s, "GB_write_string");        // compress would destroy the other buffer
    
    if (!s) s = "";
    size      = strlen(s);

    // no zero len strings allowed
    if ((GB_GETMEMSIZE(gbd))  && (size == GB_GETSIZE(gbd)))
    {
        if (!strcmp(s, GB_read_pntr(gbd)))
            return 0;
    }
#if defined(DEBUG) && 0
    // check for error (in compression)
    {
        GB_ERROR error = GB_write_pntr(gbd, s, size+1, size);
        if (!error) {
            char *check = GB_read_string(gbd);

            gb_assert(check);
            gb_assert(strcmp(check, s) == 0);

            free(check);
        }
        return error;
    }
#else
    return GB_write_pntr(gbd, s, size+1, size);
#endif // DEBUG
}

GB_ERROR GB_write_link(GBDATA *gbd, const char *s)
{
    long size;
    GB_TEST_WRITE(gbd, GB_STRING, "GB_write_link");
    GB_TEST_NON_BUFFER(s, "GB_write_link");          // compress would destroy the other buffer
    
    if (!s) s = "";
    size      = strlen(s);

    // no zero len strings allowed
    if ((GB_GETMEMSIZE(gbd))  && (size == GB_GETSIZE(gbd)))
    {
        if (!strcmp(s, GB_read_pntr(gbd)))
            return 0;
    }
    return GB_write_pntr(gbd, s, size+1, size);
}


GB_ERROR GB_write_bits(GBDATA *gbd, const char *bits, long size, const char *c_0)
{
    char *d;
    long memsize[2];

    GB_TEST_WRITE(gbd, GB_BITS, "GB_write_bits");
    GB_TEST_NON_BUFFER(bits, "GB_write_bits");       // compress would destroy the other buffer
    gb_save_extern_data_in_ts(gbd);

    d = gb_compress_bits(bits, size, (const unsigned char *)c_0, memsize);
    gbd->flags.compressed_data = 1;
    GB_SETSMDMALLOC(gbd, size, memsize[0], d);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
    return 0;
}

GB_ERROR GB_write_bytes(GBDATA *gbd, const char *s, long size)
{
    GB_TEST_WRITE(gbd, GB_BYTES, "GB_write_bytes");
    return GB_write_pntr(gbd, s, size, size);
}

GB_ERROR GB_write_ints(GBDATA *gbd, const GB_UINT4 *i, long size)
{

    GB_TEST_WRITE(gbd, GB_INTS, "GB_write_ints");
    GB_TEST_NON_BUFFER((char *)i, "GB_write_ints");  // compress would destroy the other buffer

    if (0x01020304 != htonl((GB_UINT4)0x01020304)) {
        long      j;
        char     *buf2 = GB_give_other_buffer((char *)i, size<<2);
        GB_UINT4 *s    = (GB_UINT4 *)i;
        GB_UINT4 *d    = (GB_UINT4 *)buf2;
        
        for (j=size; j; j--) {
            *(d++) = htonl(*(s++));
        }
        i = (GB_UINT4 *)buf2;
    }
    return GB_write_pntr(gbd, (char *)i, size* 4 /* sizeof(long4) */, size);
}

GB_ERROR GB_write_floats(GBDATA *gbd, const float *f, long size)
{
    long fullsize = size * sizeof(float);
    GB_TEST_WRITE(gbd, GB_FLOATS, "GB_write_floats");
    GB_TEST_NON_BUFFER((char *)f, "GB_write_floats"); // compress would destroy the other buffer

    {
        XDR    xdrs;
        long   i;
        char  *buf2 = GB_give_other_buffer((char *)f, fullsize);
        float *s    = (float *)f;

        xdrmem_create(&xdrs, buf2, (int)fullsize, XDR_ENCODE);
        for (i=size; i; i--) {
            xdr_float(&xdrs, s);
            s++;
        }
        xdr_destroy (&xdrs);
        f = (float *)buf2;
    }
    return GB_write_pntr(gbd, (char *)f, size*sizeof(float), size);
}

GB_ERROR GB_write_as_string(GBDATA *gbd, const char *val)
{
    switch (GB_TYPE(gbd)) {
        case GB_STRING: return GB_write_string(gbd, val);
        case GB_LINK:   return GB_write_link(gbd, val);
        case GB_BYTE:   return GB_write_byte(gbd, atoi(val));
        case GB_INT:    return GB_write_int(gbd, atoi(val));
        case GB_FLOAT:  return GB_write_float(gbd, GB_atof(val));
        case GB_BITS:   return GB_write_bits(gbd, val, strlen(val), "0");
        default:    return GB_export_errorf("Error: You cannot use GB_write_as_string on this type of entry (%s)", GB_read_key_pntr(gbd));
    }
}

// ---------------------------
//      security functions

int GB_read_security_write(GBDATA *gbd) {
    GB_TEST_TRANSACTION(gbd);
    return GB_GET_SECURITY_WRITE(gbd);
}
int GB_read_security_read(GBDATA *gbd) {
    GB_TEST_TRANSACTION(gbd);
    return GB_GET_SECURITY_READ(gbd);
}
int GB_read_security_delete(GBDATA *gbd) {
    GB_TEST_TRANSACTION(gbd);
    return GB_GET_SECURITY_DELETE(gbd);
}
int GB_get_my_security(GBDATA *gbd) {
    return GB_MAIN(gbd)->security_level;
}

GB_ERROR gb_security_error(GBDATA *gbd) { // goes to header: __ATTR__USERESULT
    GB_MAIN_TYPE *Main  = GB_MAIN(gbd);
    const char   *error = GBS_global_string("Protection: Attempt to change a level-%i-'%s'-entry,\n"
                                            "but your current security level is only %i",
                                            GB_GET_SECURITY_WRITE(gbd),
                                            GB_read_key_pntr(gbd),
                                            Main->security_level);
#if defined(DEBUG)
    fprintf(stderr, "%s\n", error);
#endif // DEBUG
    return error;
}

GB_ERROR GB_write_security_write(GBDATA *gbd, unsigned long level)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_TEST_TRANSACTION(gbd);

    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    if (GB_GET_SECURITY_WRITE(gbd) == level) return 0;
    GB_PUT_SECURITY_WRITE(gbd, level);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
    return 0;
}
GB_ERROR GB_write_security_read(GBDATA *gbd, unsigned long level)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_TEST_TRANSACTION(gbd);
    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    if (GB_GET_SECURITY_READ(gbd) == level) return 0;
    GB_PUT_SECURITY_READ(gbd, level);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
    return 0;
}

GB_ERROR GB_write_security_delete(GBDATA *gbd, unsigned long level)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_TEST_TRANSACTION(gbd);
    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    if (GB_GET_SECURITY_DELETE(gbd) == level) return 0;
    GB_PUT_SECURITY_DELETE(gbd, level);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
    return 0;
}
GB_ERROR GB_write_security_levels(GBDATA *gbd, unsigned long readlevel, unsigned long writelevel, unsigned long deletelevel)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_TEST_TRANSACTION(gbd);
    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    GB_PUT_SECURITY_WRITE(gbd, writelevel);
    GB_PUT_SECURITY_READ(gbd, readlevel);
    GB_PUT_SECURITY_DELETE(gbd, deletelevel);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
    return 0;
}

GB_ERROR GB_change_my_security(GBDATA *gbd, int level, const char *passwd)
{
    int i;
    i = level;
    if (i<0) i=0;
    if (i>=8) i = 7;
    GB_MAIN(gbd)->security_level = i;
    passwd = passwd;    // dummy for compiler
    return 0;
}

// For internal use only
void GB_push_my_security(GBDATA *gbd)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    Main->pushed_security_level++;
    if (Main->pushed_security_level <= 1) {
        Main->old_security_level = Main->security_level;
        Main->security_level = 7;
    }
}

void GB_pop_my_security(GBDATA *gbd) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    Main->pushed_security_level--;
    if (Main->pushed_security_level <= 0) {
        Main->security_level = Main->old_security_level;
    }
}


// ------------------------
//      Key information

GB_TYPES GB_read_type(GBDATA *gbd) {
    GB_TEST_TRANSACTION(gbd);
    return (GB_TYPES)GB_TYPE(gbd);
}

char *GB_read_key(GBDATA *gbd) {
    return strdup(GB_read_key_pntr(gbd));
}

GB_CSTR GB_read_key_pntr(GBDATA *gbd) {
    GB_CSTR k;
    GB_TEST_TRANSACTION(gbd);
    k         = GB_KEY(gbd);
    if (!k) k = GBS_global_string("<invalid key (quark=%i)>", GB_KEY_QUARK(gbd));
    return k;
}

GB_CSTR gb_read_key_pntr(GBDATA *gbd) {
    return GB_KEY(gbd);
}


GBQUARK GB_key_2_quark(GBDATA *gbd, const char *s) {
    long          index;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    if (!s) return -1;
    index = GBS_read_hash(Main->key_2_index_hash, s);
    if (!index) {   // create new index
        index = gb_create_key(Main, s, true);
    }
    return (GBQUARK)index;
}

GBQUARK GB_get_quark(GBDATA *gbd) {
    return GB_KEY_QUARK(gbd);
}

bool GB_has_key(GBDATA *gbd, const char *key) {
    GBQUARK quark = GB_key_2_quark(gbd, key);
    return (quark == GB_get_quark(gbd));
}

GBQUARK gb_key_2_quark(GB_MAIN_TYPE *Main, const char *s) {
    long index;
    if (!s) return 0;
    index = GBS_read_hash(Main->key_2_index_hash, s);
    if (!index) {   // create new index
        index = gb_create_key(Main, s, true);
    }
    return (GBQUARK)index;
}




long GB_read_clock(GBDATA *gbd)
{
    if (GB_ARRAY_FLAGS(gbd).changed) return GB_MAIN(gbd)->clock;
    return GB_GET_EXT_UPDATE_DATE(gbd);
}

long    GB_read_transaction(GBDATA *gbd)
{
    return GB_MAIN(gbd)->transaction;
}

// ---------------------------------------------
//      Get and check the database hierarchy

GBDATA *GB_get_father(GBDATA *gbd) {
    // Get the father of an entry
    GBDATA *father;

    GB_TEST_TRANSACTION(gbd);
    if (!(father=(GBDATA*)GB_FATHER(gbd)))  return NULL;
    if (!GB_FATHER(father))         return NULL;

    return father;
}

GBDATA *GB_get_grandfather(GBDATA *gbd) {
    GBDATA *gb_grandpa;
    GB_TEST_TRANSACTION(gbd);

    gb_grandpa = (GBDATA*)GB_FATHER(gbd);
    if (gb_grandpa) {
        gb_grandpa = (GBDATA*)GB_FATHER(gb_grandpa);
        if (gb_grandpa && !GB_FATHER(gb_grandpa)) {
            gb_grandpa = 0;
        }
    }
    return gb_grandpa;
}

GBDATA *GB_get_root(GBDATA *gbd) {  // Get the root entry (gb_main)
    return (GBDATA *)GB_MAIN(gbd)->data;
}

bool GB_check_father(GBDATA *gbd, GBDATA *gb_maybefather) {
    // Test whether an entry is a subentry of another
    GBDATA *gbfather;
    for (gbfather = GB_get_father(gbd);
         gbfather;
         gbfather = GB_get_father(gbfather))
    {
        if (gbfather == gb_maybefather) return true;
    }
    return false;
}

// --------------------------
//      create and rename

GBDATA *gb_create(GBDATA *father, const char *key, GB_TYPES type) {
    GBDATA *gbd = gb_make_entry((GBCONTAINER *)father, key, -1, 0, type);
    gb_touch_header(GB_FATHER(gbd));
    gb_touch_entry(gbd, GB_CREATED);
    return (GBDATA *)gbd;
}

GBDATA *gb_create_container(GBDATA *father, const char *key) {
    // Create a container, do not check anything
    GBCONTAINER *gbd = gb_make_container((GBCONTAINER *)father, key, -1, 0);
    gb_touch_header(GB_FATHER(gbd));
    gb_touch_entry((GBDATA *)gbd, GB_CREATED);
    return (GBDATA *)gbd;
}

void gb_rename(GBCONTAINER *gbc, const char *new_key) {
    gb_rename_entry(gbc, new_key);
}

int GB_rename(GBDATA *gbc, const char *new_key) {
    /*! User accessible rename, check everything
     * returns 0 if successful!
     */
    GBCONTAINER *old_father, *new_father;
    if (GB_check_key(new_key)) {
        GB_print_error();
        return -1;
    }

    GB_TEST_TRANSACTION(gbc);
    old_father = GB_FATHER(gbc);

    if (GB_TYPE(gbc) != GB_DB) {
        GB_internal_error("GB_rename has to be called with container");
        return -1;
    }

    gb_rename((GBCONTAINER*)gbc, new_key);

    new_father = GB_FATHER(gbc);
    if (old_father != new_father) {
        GB_internal_error("father changed during rename");
        return -1;
    }

    gb_touch_header(new_father);
    gb_touch_entry(gbc, GB_NORMAL_CHANGE);

    return 0;
}

GBDATA *GB_create(GBDATA *father, const char *key, GB_TYPES type) {
    /*! Create a DB entry
     *
     * @param father container to create DB field in
     * @param key name of field
     * @param type field type
     *
     * @return
     * - created DB entry
     * - NULL on failure (error is exported then)
     * 
     * @see GB_create_container()
     */
    GBDATA *gbd;

    if (GB_check_key(key)) {
        GB_print_error();
        return NULL;
    }

    if (type == GB_DB) {
        gb_assert(type != GB_DB); // you like to use GB_create_container!
        GB_export_error("GB_create error: can't create containers");
        return NULL;
    }

    if (!father) {
        GB_internal_errorf("GB_create error in GB_create:\nno father (key = '%s')", key);
        return NULL;
    }
    GB_TEST_TRANSACTION(father);
    if (GB_TYPE(father)!=GB_DB) {
        GB_export_errorf("GB_create: father (%s) is not of GB_DB type (%i) (creating '%s')",
                         GB_read_key_pntr(father), GB_TYPE(father), key);
        return NULL;
    };

    if (type == GB_POINTER) {
        if (!GB_in_temporary_branch(father)) {
            GB_export_error("GB_create: pointers only allowed in temporary branches");
            return NULL;
        }
    }

    gbd = gb_make_entry((GBCONTAINER *)father, key, -1, 0, type);
    gb_touch_header(GB_FATHER(gbd));
    gb_touch_entry(gbd, GB_CREATED);
    
    gb_assert(GB_ARRAY_FLAGS(gbd).changed < GB_DELETED); // happens sometimes -> needs debugging
    
    return gbd;
}

GBDATA *GB_create_container(GBDATA *father, const char *key) {
    /*! Create a new DB container
     *
     * @param father parent container
     * @param key name of created container
     *
     * @return
     * - created container
     * - NULL on failure (error is exported then)
     *
     * @see GB_create()
     */
    
    GBCONTAINER *gbd;
    if (GB_check_key(key)) {
        GB_print_error();
        return NULL;
    }

    if ((*key == '\0')) {
        GB_export_error("GB_create error: empty key");
        return NULL;
    }
    if (!father) {
        GB_internal_errorf("GB_create error in GB_create:\nno father (key = '%s')", key);
        return NULL;
    }
    GB_TEST_TRANSACTION(father);
    if (GB_TYPE(father)!=GB_DB) {
        GB_export_errorf("GB_create: father (%s) is not of GB_DB type (%i) (creating '%s')",
                         GB_read_key_pntr(father), GB_TYPE(father), key);
        return NULL;
    };
    gbd = gb_make_container((GBCONTAINER *)father, key, -1, 0);
    gb_touch_header(GB_FATHER(gbd));
    gb_touch_entry((GBDATA *)gbd, GB_CREATED);
    return (GBDATA *)gbd;
}

#if defined(DEVEL_RALF)
#warning change param for GB_delete to GBDATA **
#endif // DEVEL_RALF

GB_ERROR GB_delete(GBDATA *source) {
    GBDATA *gb_main;

    GB_TEST_TRANSACTION(source);
    if (GB_GET_SECURITY_DELETE(source)>GB_MAIN(source)->security_level) {
        return GB_export_errorf("Security error in GB_delete: %s", GB_read_key_pntr(source));
    }

    gb_main = GB_get_root(source);

    if (source->flags.compressed_data) {
        gb_set_compression_mask(gb_main, 0); // disable compression
        gb_set_compression(source); // write data w/o compression (otherwise GB_read_old_value... won't work)
        gb_set_compression_mask(gb_main, -1); // allow all types of compressions
    }

    {
        GB_MAIN_TYPE *Main = GB_MAIN(source);
        if (Main->transaction<0) {
            gb_delete_entry(&source);
            gb_do_callback_list(Main);
        }
        else {
            gb_touch_entry(source, GB_DELETED);
        }
    }
    return 0;
}

GB_ERROR gb_delete_force(GBDATA *source)    // delete always
{
    gb_touch_entry(source, GB_DELETED);
    return 0;
}


// ------------------
//      Copy data

#if defined(DEVEL_RALF)
#warning replace GB_copy with GB_copy_with_protection after release 
#endif // DEVEL_RALF

GB_ERROR GB_copy(GBDATA *dest, GBDATA *source) {
    return GB_copy_with_protection(dest, source, false);
}

GB_ERROR GB_copy_with_protection(GBDATA *dest, GBDATA *source, bool copy_all_protections) {
    GB_TYPES type;
    GB_ERROR error = 0;
    GBDATA *gb_p;
    GBDATA *gb_d;
    GBCONTAINER *destc, *sourcec;
    const char *key;

    GB_TEST_TRANSACTION(source);
    type = GB_TYPE(source);
    if (GB_TYPE(dest) != type)
    {
        return GB_export_errorf("incompatible types in GB_copy (source %s:%u != %s:%u",
                                GB_read_key_pntr(source), type, GB_read_key_pntr(dest), GB_TYPE(dest));
    }

    switch (type)
    {
        case GB_INT:
            error = GB_write_int(dest, GB_read_int(source));
            break;
        case GB_FLOAT:
            error = GB_write_float(dest, GB_read_float(source));
            break;
        case GB_BYTE:
            error = GB_write_byte(dest, GB_read_byte(source));
            break;
        case GB_STRING:     // No local compression
            error = GB_write_string(dest, GB_read_char_pntr(source));
            break;
        case GB_LINK:       // No local compression
            error = GB_write_link(dest, GB_read_link_pntr(source));
            break;
        case GB_BITS:       // only local compressions for the following types
        case GB_BYTES:
        case GB_INTS:
        case GB_FLOATS:
            gb_save_extern_data_in_ts(dest);
            GB_SETSMDMALLOC(dest,   GB_GETSIZE(source),
                            GB_GETMEMSIZE(source),
                            GB_GETDATA(source));
            dest->flags.compressed_data = source->flags.compressed_data;

            break;
        case GB_DB:

            destc = (GBCONTAINER *)dest;
            sourcec = (GBCONTAINER *)source;

            if (GB_TYPE(destc) != GB_DB)
            {
                GB_ERROR err = GB_export_errorf("GB_COPY Type conflict %s:%i != %s:%i",
                                                GB_read_key_pntr(dest), GB_TYPE(dest), GB_read_key_pntr(source), GB_DB);
                GB_internal_error(err);
                return err;
            }

            if (source->flags2.folded_container)    gb_unfold((GBCONTAINER *)source, -1, -1);
            if (dest->flags2.folded_container)  gb_unfold((GBCONTAINER *)dest, 0, -1);

            for (gb_p = GB_child(source); gb_p; gb_p = GB_nextChild(gb_p)) {
                GB_TYPES type2 = (GB_TYPES)GB_TYPE(gb_p);

                key = GB_read_key_pntr(gb_p);
                if (type2 == GB_DB)
                {
                    gb_d = GB_create_container(dest, key);
                    gb_create_header_array((GBCONTAINER *)gb_d, ((GBCONTAINER *)gb_p)->d.size);
                }
                else
                {
                    gb_d = GB_create(dest, key, type2);
                }

                if (!gb_d) error = GB_await_error();
                else error       = GB_copy_with_protection(gb_d, gb_p, copy_all_protections);
                
                if (error) break;
            }

            destc->flags3 = sourcec->flags3;
            break;

        default:
            error = GB_export_error("GB_copy error unknown type");
    }
    if (error) return error;

    gb_touch_entry(dest, GB_NORMAL_CHANGE);

    dest->flags.security_read = source->flags.security_read;
    if (copy_all_protections) {
        dest->flags.security_write  = source->flags.security_write;
        dest->flags.security_delete = source->flags.security_delete;
    }

    return 0;
}


static char *gb_stpcpy(char *dest, const char *source)
{
    while ((*dest++=*source++)) ;
    return dest-1; // return pointer to last copied character (which is \0)
}

char* GB_get_subfields(GBDATA *gbd) {
    /*! Get all subfield names 
     *
     * @return all subfields of 'gbd' as ';'-separated heap-copy
     * (first and last char of result is a ';')
     */
    long type;
    char *result = 0;

    GB_TEST_TRANSACTION(gbd);
    type = GB_TYPE(gbd);

    if (type==GB_DB) { // we are a container
        GBCONTAINER *gbc = (GBCONTAINER*)gbd;
        GBDATA *gbp;
        int result_length = 0;

        if (gbc->flags2.folded_container) {
            gb_unfold(gbc, -1, -1);
        }

        for (gbp = GB_child(gbd); gbp; gbp = GB_nextChild(gbp)) {
            const char *key = GB_read_key_pntr(gbp);
            int keylen = strlen(key);

            if (result) {
                char *neu_result = (char*)malloc(result_length+keylen+1+1);

                if (neu_result) {
                    char *p = gb_stpcpy(neu_result, result);
                    p = gb_stpcpy(p, key);
                    *p++ = ';';
                    p[0] = 0;

                    freeset(result, neu_result);
                    result_length += keylen+1;
                }
                else {
                    gb_assert(0);
                }
            }
            else {
                result = (char*)malloc(1+keylen+1+1);
                result[0] = ';';
                strcpy(result+1, key);
                result[keylen+1] = ';';
                result[keylen+2] = 0;
                result_length = keylen+2;
            }
        }
    }
    else {
        result = strdup(";");
    }

    return result;
}

// ----------------------
//      recompression

#if defined(DEVEL_RALF)
#warning rename gb_set_compression into gb_recompress (misleading name)
#endif // DEVEL_RALF

GB_ERROR gb_set_compression(GBDATA *source)
{
    long type;
    GB_ERROR error = 0;
    GBDATA *gb_p;
    char *string;

    GB_TEST_TRANSACTION(source);
    type = GB_TYPE(source);

    switch (type) {
        case GB_STRING:
            string = GB_read_string(source);
            GB_write_string(source, "");
            GB_write_string(source, string);
            free(string);
            break;
        case GB_BITS:
        case GB_BYTES:
        case GB_INTS:
        case GB_FLOATS:
            break;
        case GB_DB:
            for (gb_p = GB_child(source); gb_p; gb_p = GB_nextChild(gb_p)) {
                error = gb_set_compression(gb_p);
                if (error) break;
            }
            break;
        default:
            break;
    }
    if (error) return error;
    return 0;
}

void gb_set_compression_mask(GBDATA *gb_main, GB_COMPRESSION_MASK disable_compression) {
    GB_MAIN_TYPE *Main     = GB_MAIN(gb_main);
    Main->compression_mask = disable_compression;
}

// --------------------------
//      temporary entries

GB_ERROR GB_set_temporary(GBDATA *gbd) {
    /*! if the temporary flag is set, then that entry (including all subentries) will not be saved
     * @see GB_clear_temporary() and GB_is_temporary()
     */

    GB_TEST_TRANSACTION(gbd);
    if (GB_GET_SECURITY_DELETE(gbd)>GB_MAIN(gbd)->security_level)
        return GB_export_errorf("Security error in GB_set_temporary: %s", GB_read_key_pntr(gbd));
    gbd->flags.temporary = 1;
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    return 0;
}

GB_ERROR GB_clear_temporary(GBDATA *gbd) {
    //! undo effect of GB_set_temporary()

    GB_TEST_TRANSACTION(gbd);
    gbd->flags.temporary = 0;
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    return 0;
}

bool GB_is_temporary(GBDATA *gbd) {
    //! @see GB_set_temporary() and GB_in_temporary_branch()
    GB_TEST_TRANSACTION(gbd);
    return (long)gbd->flags.temporary;
}

bool GB_in_temporary_branch(GBDATA *gbd) {
    /*! @return true, if 'gbd' is member of a temporary subtree,
     * i.e. if GB_is_temporary(itself or any parent)  
     */

    if (GB_is_temporary(gbd)) return true;

    GBDATA *gb_parent = GB_get_father(gbd);
    if (!gb_parent) return false;

    return GB_in_temporary_branch(gb_parent);
}

// ---------------------
//      transactions


GB_ERROR GB_push_local_transaction(GBDATA *gbd) {
    /*! Starts a read only transaction.
     * Be sure that all data is cached. Be extremely careful!
     */
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->transaction>0) {
        return GB_push_transaction(gbd);
    }
    Main->transaction --;
    return 0;
}

GB_ERROR GB_pop_local_transaction(GBDATA *gbd) {
    /*! Stops a read only transaction.
     * Be sure that all data is cached!
     */
     
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->transaction>0) {
        return GB_pop_transaction(gbd);
    }
    Main->transaction ++;
    return 0;
}

GB_ERROR GB_push_transaction(GBDATA *gbd) {
    /*! start a transaction if no transaction is running.
     * (otherwise only trace nested transactions)
     *
     * recommended transaction usage:
     *
     * \code
     * GB_ERROR myFunc() {
     *     GB_ERROR error = GB_push_transaction(gbd);
     *     if (!error) {
     *         error = ...;
     *     }
     *     return GB_end_transaction(gbd, error);
     * }
     *       
     * void myFunc() {
     *     GB_ERROR error = GB_push_transaction(gbd);
     *     if (!error) {
     *         error = ...;
     *     }
     *     GB_end_transaction_show_error(gbd, error, aw_message);
     * }
     * \endcode
     *
     * @see GB_pop_transaction(), GB_end_transaction(), GB_begin_transaction()
     */

    GB_MAIN_TYPE *Main  = GB_MAIN(gbd);
    GB_ERROR      error = 0;

    if (Main->transaction == 0) error = GB_begin_transaction(gbd);
    else if (Main->transaction>0) Main->transaction++;
    // Main->transaction<0 is "no transaction mode"

    return error;
}

GB_ERROR GB_pop_transaction(GBDATA *gbd) {
    //! commit a transaction started with GB_push_transaction()

    GB_ERROR      error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->transaction==0) {
        error = GB_export_error("Pop without push");
        GB_internal_error(error);
        return  error;
    }
    if (Main->transaction<0) return 0;  // no transaction mode
    if (Main->transaction==1) {
        return GB_commit_transaction(gbd);
    }
    else {
        Main->transaction--;
    }
    return 0;
}

GB_ERROR GB_begin_transaction(GBDATA *gbd) {
    /*! like GB_push_transaction(),
     * but fails if there is already an transaction running.
     * @see GB_commit_transaction() and GB_abort_transaction()
     */
    
    GB_ERROR      error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    gbd                = (GBDATA *)Main->data;
    if (Main->transaction>0) {
        error = GB_export_errorf("GB_begin_transaction called %i !!!",
                                 Main->transaction);
        GB_internal_error(error);
        return GB_push_transaction(gbd);
    }
    if (Main->transaction<0) return 0;
    Main->transaction = 1;
    Main->aborted_transaction = 0;
    if (!Main->local_mode) {
        error = gbcmc_begin_transaction(gbd);
        if (error) return error;
        error = gb_commit_transaction_local_rek(gbd, 0, 0); // init structures
        gb_untouch_children((GBCONTAINER *)gbd);
        gb_untouch_me(gbd);
        if (error) return error;
    }

    /* do all callbacks
     * cb that change the db are no problem, because it's the beginning of a ta
     */
    gb_do_callback_list(Main);
    
    Main->clock ++;
    return 0;
}

GB_ERROR gb_init_transaction(GBCONTAINER *gbd)      // the first transaction ever
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_ERROR      error;

    Main->transaction = 1;
    
    error = gbcmc_init_transaction(Main->data);
    if (!error) Main->clock ++;

    return error;
}

GB_ERROR GB_no_transaction(GBDATA *gbd)
{
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (!Main->local_mode) {
        error = GB_export_error("Tried to disable transactions in a client");
        GB_internal_error(error);
        return 0;
    }
    Main->transaction = -1;
    return 0;
}

GB_ERROR GB_abort_transaction(GBDATA *gbd)
{
    /*! abort a running transaction,
     * i.e. forget all changes made to DB inside the current transaction.
     *
     * May be called instead of GB_pop_transaction() or GB_commit_transaction()
     *
     * If a nested transactions got aborted,
     * committing a surrounding transaction will silently abort it as well.
     */
    
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    gbd = (GBDATA *)Main->data;
    if (Main->transaction<=0) {
        GB_internal_error("No running Transaction");
        return GB_export_error("GB_abort_transaction: No running Transaction");
    }
    if (Main->transaction>1) {
        Main->aborted_transaction = 1;
        return GB_pop_transaction(gbd);
    }

    gb_abort_transaction_local_rek(gbd, 0);
    if (!Main->local_mode) {
        error = gbcmc_abort_transaction(gbd);
        if (error) return error;
    }
    Main->clock--;
    gb_do_callback_list(Main);       // do all callbacks
    Main->transaction = 0;
    gb_untouch_children((GBCONTAINER *)gbd);
    gb_untouch_me(gbd);
    return 0;
}

GB_ERROR GB_commit_transaction(GBDATA *gbd) {
    /*! commit a transaction started with GB_begin_transaction()
     *
     * commit changes made to DB.
     *
     * in case of nested transactions, this is equal to GB_pop_transaction()
     */
    
    GB_ERROR      error = 0;
    GB_MAIN_TYPE *Main  = GB_MAIN(gbd);
    GB_CHANGE     flag;

    gbd = (GBDATA *)Main->data;
    if (!Main->transaction) {
        error = GB_export_error("GB_commit_transaction: No running Transaction");
        GB_internal_error(error);
        return error;
    }
    if (Main->transaction>1) {
        GB_internal_error("Running GB_commit_transaction not at root transaction level");
        return GB_pop_transaction(gbd);
    }
    if (Main->aborted_transaction) {
        Main->aborted_transaction = 0;
        return      GB_abort_transaction(gbd);
    }
    if (Main->local_mode) {
        char *error1 = gb_set_undo_sync(gbd);
        while (1) {
            flag = (GB_CHANGE)GB_ARRAY_FLAGS(gbd).changed;
            if (!flag) break;           // nothing to do
            error = gb_commit_transaction_local_rek(gbd, 0, 0);
            gb_untouch_children((GBCONTAINER *)gbd);
            gb_untouch_me(gbd);
            if (error) break;
            gb_do_callback_list(Main);       // do all callbacks
        }
        gb_disable_undo(gbd);
        if (error1) {
            Main->transaction = 0;
            return error;
        }
    }
    else {
        gb_disable_undo(gbd);
        while (1) {
            flag = (GB_CHANGE)GB_ARRAY_FLAGS(gbd).changed;
            if (!flag) break;           // nothing to do

            error = gbcmc_begin_sendupdate(gbd);        if (error) break;
            error = gb_commit_transaction_local_rek(gbd, 1, 0); if (error) break;
            error = gbcmc_end_sendupdate(gbd);      if (error) break;

            gb_untouch_children((GBCONTAINER *)gbd);
            gb_untouch_me(gbd);
            gb_do_callback_list(Main);       // do all callbacks
        }
        if (!error) error = gbcmc_commit_transaction(gbd);

    }
    Main->transaction = 0;
    if (error) return error;
    return 0;
}

GB_ERROR GB_end_transaction(GBDATA *gbd, GB_ERROR error) {
    /*! abort or commit transaction 
     *
     * @ param error
     * - if NULL commit transaction
     * - else abort transaction
     *
     * @return error or transaction error
     * @see GB_push_transaction() for example
     */
    if (error) GB_abort_transaction(gbd);
    else error = GB_pop_transaction(gbd);
    return error;
}

void GB_end_transaction_show_error(GBDATA *gbd, GB_ERROR error, void (*error_handler)(GB_ERROR)) {
    //! like GB_end_transaction(), but show error using 'error_handler'
    error = GB_end_transaction(gbd, error);
    if (error) error_handler(error);
}

int GB_get_transaction_level(GBDATA *gbd) {
    /*! @return transaction level
     * - 0 -> not in transaction
     * - 1 -> one single transaction
     * - 2, ... -> nested transactions
     */
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    return Main->transaction;
}

#if defined(DEVEL_RALF)
#warning GB_update_server should be ARBDB-local!
#endif // DEVEL_RALF

GB_ERROR GB_update_server(GBDATA *gbd) {
    //! Send updated data to server
    GB_ERROR error;
    GB_MAIN_TYPE    *Main = GB_MAIN(gbd);
    GBDATA *gb_main = (GBDATA *)Main->data;
    struct gb_callback_list *cbl_old = Main->cbl_last;
    if (!Main->transaction) {
        error = GB_export_error("GB_update_server: No running Transaction");
        GB_internal_error(error);
        return error;
    }
    if (Main->local_mode) {
        return GB_export_error("You cannot update the server as you are the server yourself");
    }

    error = gbcmc_begin_sendupdate(gb_main);
    if (error) return error;
    error = gb_commit_transaction_local_rek(gbd, 2, 0);
    if (error) return error;
    error = gbcmc_end_sendupdate(gb_main);
    if (error) return error;
    if (cbl_old != Main->cbl_last) {
        GB_internal_error("GB_update_server produced a callback, this is not allowed");
    }
    // gb_do_callback_list(gbd);         do all callbacks
    return 0;
}

// ------------------
//      callbacks

GB_ERROR gb_add_changed_callback_list(GBDATA *gbd, struct gb_transaction_save *old, GB_CB_TYPE gbtype, GB_CB func, int *clientdata)
{
    struct gb_callback_list *cbl;
    GB_MAIN_TYPE    *Main = GB_MAIN(gbd);
    cbl = (struct gb_callback_list *)gbm_get_mem(sizeof(struct gb_callback_list), GBM_CB_INDEX);
    if (Main->cbl) {
        Main->cbl_last->next = cbl;
    }
    else {
        Main->cbl = cbl;
    }
    Main->cbl_last = cbl;
    cbl->clientdata = clientdata;
    cbl->func = func;
    cbl->gbd = gbd;
    cbl->type = gbtype;
    gb_add_ref_gb_transaction_save(old);
    cbl->old = old;

    return 0;
}

GB_ERROR gb_add_delete_callback_list(GBDATA *gbd, struct gb_transaction_save *old, GB_CB func, int *clientdata)
{
    struct gb_callback_list *cbl;
    GB_MAIN_TYPE    *Main = GB_MAIN(gbd);
    cbl = (struct gb_callback_list *)gbm_get_mem(sizeof(struct gb_callback_list), GBM_CB_INDEX);
    if (Main->cbld) {
        Main->cbld_last->next = cbl;
    }
    else {
        Main->cbld = cbl;
    }
    Main->cbld_last = cbl;
    cbl->clientdata = clientdata;
    cbl->func = func;
    cbl->gbd = gbd;
    cbl->type = GB_CB_DELETE;
    if (old) gb_add_ref_gb_transaction_save(old);
    cbl->old = old;
    return 0;
}

static struct gb_callback_list *g_b_old_callback_list = NULL; // points to callback during callback (NULL otherwise)
static GB_MAIN_TYPE            *g_b_old_main          = NULL; // points to DB root during callback (NULL otherwise)

GB_ERROR gb_do_callback_list(GB_MAIN_TYPE *Main) {
    struct gb_callback_list *cbl, *cbl_next;
    g_b_old_main = Main;

    // first all delete callbacks:
    for (cbl = Main->cbld; cbl;  cbl = cbl_next) {
        g_b_old_callback_list = cbl;
        cbl->func(cbl->gbd, cbl->clientdata, GB_CB_DELETE);
        cbl_next = cbl->next;
        g_b_old_callback_list = NULL;
        gb_del_ref_gb_transaction_save(cbl->old);
        gbm_free_mem((char *)cbl, sizeof(struct gb_callback_list), GBM_CB_INDEX);
    }
    
    Main->cbld_last = NULL;
    Main->cbld      = NULL;

    // then all update callbacks:
    for (cbl = Main->cbl; cbl;  cbl = cbl_next) {
        g_b_old_callback_list = cbl;
        cbl->func(cbl->gbd, cbl->clientdata, cbl->type);
        cbl_next = cbl->next;
        g_b_old_callback_list = NULL;
        gb_del_ref_gb_transaction_save(cbl->old);
        gbm_free_mem((char *)cbl, sizeof(struct gb_callback_list), GBM_CB_INDEX);
    }

    g_b_old_main   = NULL;
    Main->cbl_last = NULL;
    Main->cbl      = NULL;
    
    return 0;
}

GB_MAIN_TYPE *gb_get_main_during_cb() {
    /* if inside a callback, return the DB root of the DB element, the callback was called for.
     * if not inside a callback, return NULL.
     */
    return g_b_old_main;
}

NOT4PERL bool GB_inside_callback(GBDATA *of_gbd, GB_CB_TYPE cbtype) {
    GB_MAIN_TYPE *Main   = gb_get_main_during_cb();
    bool          inside = false;

    if (Main) {                 // inside a callback
        gb_assert(g_b_old_callback_list);
        if (g_b_old_callback_list->gbd == of_gbd) {
            GB_CB_TYPE curr_cbtype;
            if (Main->cbld) {       // delete callbacks were not all performed yet
                                    // -> current callback is a delete callback
                curr_cbtype = GB_CB_TYPE(g_b_old_callback_list->type & GB_CB_DELETE);
            }
            else {
                gb_assert(Main->cbl); // change callback
                curr_cbtype = GB_CB_TYPE(g_b_old_callback_list->type & (GB_CB_ALL-GB_CB_DELETE));
            }
            gb_assert(curr_cbtype != GB_CB_NONE); // wtf!? are we inside callback or not?

            if ((cbtype&curr_cbtype) != GB_CB_NONE) {
                inside = true;
            }
        }
    }

    return inside;
}

GBDATA *GB_get_gb_main_during_cb() {
    GBDATA       *gb_main = NULL;
    GB_MAIN_TYPE *Main    = gb_get_main_during_cb();

    if (Main) {                 // inside callback
        if (!GB_inside_callback((GBDATA*)Main->data, GB_CB_DELETE)) { // main is not deleted
            gb_main = (GBDATA*)Main->data;
        }
    }
    return gb_main;
}



GB_CSTR gb_read_pntr_ts(GBDATA *gbd, struct gb_transaction_save *ts) {
    int         type = GB_TYPE_TS(ts);
    const char *data = GB_GETDATA_TS(ts);
    if (data) {
        if (ts->flags.compressed_data) {    // uncompressed data return pntr to database entry
            long size = GB_GETSIZE_TS(ts) * gb_convert_type_2_sizeof[type] + gb_convert_type_2_appendix_size[type];
            data = gb_uncompress_data(gbd, data, size);
        }
    }
    return data;
}

// get last array value in callbacks
NOT4PERL const void *GB_read_old_value() {
    char *data;

    if (!g_b_old_callback_list) {
        GB_export_error("You cannot call GB_read_old_value outside a ARBDB callback");
        return NULL;
    }
    if (!g_b_old_callback_list->old) {
        GB_export_error("No old value available in GB_read_old_value");
        return NULL;
    }
    data = GB_GETDATA_TS(g_b_old_callback_list->old);
    if (!data) return NULL;

    return gb_read_pntr_ts(g_b_old_callback_list->gbd, g_b_old_callback_list->old);
}
// same for size
long GB_read_old_size() {
    if (!g_b_old_callback_list) {
        GB_export_error("You cannot call GB_read_old_size outside a ARBDB callback");
        return -1;
    }
    if (!g_b_old_callback_list->old) {
        GB_export_error("No old value available in GB_read_old_size");
        return -1;
    }
    return GB_GETSIZE_TS(g_b_old_callback_list->old);
}

char *GB_get_callback_info(GBDATA *gbd) {
    // returns human-readable information about callbacks of 'gbd' or 0
    char *result = 0;
    if (gbd->ext) {
        struct gb_callback *cb = gbd->ext->callback;
        while (cb) {
            char *cb_info = GBS_global_string_copy("func=%p type=%i clientdata=%p priority=%i",
                                                   (void*)cb->func, cb->type, cb->clientdata, cb->priority);
            if (result) {
                char *new_result = GBS_global_string_copy("%s\n%s", result, cb_info);
                free(result);
                free(cb_info);
                result = new_result;
            }
            else {
                result = cb_info;
            }
            cb = cb->next;
        }
    }

    return result;
}

GB_ERROR GB_add_priority_callback(GBDATA *gbd, GB_CB_TYPE type, GB_CB func, int *clientdata, int priority) {
    /* Adds a callback to a DB entry.
     * 
     * Callbacks with smaller priority values get executed before bigger priority values.
     *
     * Be careful when writing GB_CB_DELETE callbacks, there is a severe restriction:
     * 
     * - the DB element may already be freed. The pointer is still pointing to the original
     *   location, so you can use it to identify the DB element, but you cannot dereference
     *   it under all circumstances.
     *
     * ARBDB internal delete-callbacks may use gb_get_main_during_cb() to access the DB root.
     * See also: GB_get_gb_main_during_cb()
     */

    struct gb_callback *cb;

#if defined(DEBUG)
    if (GB_inside_callback(gbd, GB_CB_DELETE)) {
        printf("Warning: GB_add_priority_callback called inside delete-callback of gbd (gbd may already be freed)\n");
#if defined(DEVEL_RALF)
        gb_assert(0); // fix callback-handling (never modify callbacks from inside delete callbacks)
#endif // DEVEL_RALF
    }
#endif // DEBUG

    GB_TEST_TRANSACTION(gbd); // may return error
    GB_CREATE_EXT(gbd);
    cb = (struct gb_callback *)gbm_get_mem(sizeof(struct gb_callback), GB_GBM_INDEX(gbd));

    if (gbd->ext->callback) {
        struct gb_callback *prev = 0;
        struct gb_callback *curr = gbd->ext->callback;

        while (curr) {
            if (priority <= curr->priority) {
                // wanted priority is lower -> insert here
                break;
            }

#if defined(DEVEL_RALF)
            // test if callback already was added (every callback shall only exist once). see below.
            gb_assert((curr->func != func) || (curr->clientdata != clientdata) || (curr->type != type));
#endif // DEVEL_RALF

            prev = curr;
            curr = curr->next;
        }

        if (prev) { prev->next = cb; }
        else { gbd->ext->callback = cb; }

        cb->next = curr;
    }
    else {
        cb->next           = 0;
        gbd->ext->callback = cb;
    }

    cb->type       = type;
    cb->clientdata = clientdata;
    cb->func       = func;
    cb->priority   = priority;

#if defined(DEVEL_RALF)
#if defined(DEBUG)
    // test if callback already was added (every callback shall only exist once)
    // maybe you like to use GB_ensure_callback instead of GB_add_callback
    while (cb->next) {
        cb = cb->next;
        gb_assert((cb->func != func) || (cb->clientdata != clientdata) || (cb->type != type));
    }
#endif // DEBUG
#endif // DEVEL_RALF

    return 0;
}

GB_ERROR GB_add_callback(GBDATA *gbd, GB_CB_TYPE type, GB_CB func, int *clientdata) {
    return GB_add_priority_callback(gbd, type, func, clientdata, 5); // use default priority 5
}

static void gb_remove_callback(GBDATA *gbd, GB_CB_TYPE type, GB_CB func, int *clientdata, bool cd_should_match) {
    bool removed     = false;
    bool exactly_one = cd_should_match; // remove exactly one callback

#if defined(DEBUG)
    if (GB_inside_callback(gbd, GB_CB_DELETE)) {
        printf("Warning: gb_remove_callback called inside delete-callback of gbd (gbd may already be freed)\n");
#if defined(DEVEL_RALF)
        gb_assert(0); // fix callback-handling (never modify callbacks from inside delete callbacks)
#endif // DEVEL_RALF
    }
#endif // DEBUG

    if (gbd->ext) {
        struct gb_callback **cb_ptr       = &gbd->ext->callback;
        struct gb_callback  *cb;
        short                prev_running = 0;

        for (cb = *cb_ptr; cb; cb = *cb_ptr) {
            short this_running = cb->running;

            if ((cb->func == func)  &&
                (cb->type == type) &&
                (cb->clientdata == clientdata || !cd_should_match))
            {
                if (prev_running || cb->running) {
                    // if the previous callback in list or the callback itself is running (in "no transaction mode")
                    // the callback cannot be removed (see gb_do_callbacks)
                    GBK_terminate("gb_remove_callback: tried to remove currently running callback");
                }

                *cb_ptr = cb->next;
                gbm_free_mem((char *)cb, sizeof(struct gb_callback), GB_GBM_INDEX(gbd));
                removed = true;
                if (exactly_one) break;
            }
            else {
                cb_ptr = &cb->next;
            }
            prev_running = this_running;
        }
    }
}

void GB_remove_callback(GBDATA *gbd, GB_CB_TYPE type, GB_CB func, int *clientdata) {
    // remove specific callback (type, func and clientdata must match)
    gb_remove_callback(gbd, type, func, clientdata, true);
}

void GB_remove_all_callbacks_to(GBDATA *gbd, GB_CB_TYPE type, GB_CB func) {
    // removes all callbacks 'func' bound to 'gbd' with 'type'
    gb_remove_callback(gbd, type, func, 0, false);
}

GB_ERROR GB_ensure_callback(GBDATA *gbd, GB_CB_TYPE type, GB_CB func, int *clientdata) {
    struct gb_callback *cb;
    for (cb = GB_GET_EXT_CALLBACKS(gbd); cb; cb = cb->next) {
        if ((cb->func == func) && (cb->clientdata == clientdata) && (cb->type == type)) {
            return NULL;        // already in cb list
        }
    }
    return GB_add_callback(gbd, type, func, clientdata);
}

GB_ERROR GB_release(GBDATA *gbd) {
    /*! free cached data in client.
     *
     * Warning: pointers into the freed region(s) will get invalid!
     */
    GBCONTAINER  *gbc;
    GBDATA       *gb;
    int           index;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    GB_TEST_TRANSACTION(gbd);
    if (Main->local_mode) return 0;
    if (GB_ARRAY_FLAGS(gbd).changed && !gbd->flags2.update_in_server) {
        GB_update_server(gbd);
    }
    if (GB_TYPE(gbd) != GB_DB) {
        GB_ERROR error = GB_export_errorf("You cannot release non container (%s)",
                                          GB_read_key_pntr(gbd));
        GB_internal_error(error);
        return error;
    }
    if (gbd->flags2.folded_container) return 0;
    gbc = (GBCONTAINER *)gbd;

    for (index = 0; index < gbc->d.nheader; index++) {
        if ((gb = GBCONTAINER_ELEM(gbc, index))) {
            gb_delete_entry(&gb);
        }
    }

    gbc->flags2.folded_container = 1;
    gb_do_callback_list(Main);       // do all callbacks
    return 0;
}

int GB_testlocal(GBDATA *gbd) {
    /*! test whether data is available as local data
     *
     * @return 1 if data is available
     *
     * important for callbacks, because only testlocal tested data is available
     */
    if (GB_TYPE(gbd) != GB_DB) return 1;            // all non-containers are available
    if (GB_MAIN(gbd)->local_mode) return 1;         // everything is available in server
    if (gbd->flags2.folded_container) return 0; 
    return 1;
}

int GB_nsons(GBDATA *gbd) {
    /*! return number of child entries
     *
     * @@@ does this work in clients ? 
     */
    if (GB_TYPE(gbd) != GB_DB) return 0;
    return ((GBCONTAINER *)gbd)->d.size;
}

void GB_disable_quicksave(GBDATA *gbd, const char *reason) {
    /*! Disable quicksaving database
     * @param reason why quicksaving is not allowed
     */
    freedup(GB_MAIN(gbd)->qs.quick_save_disabled, reason);
}

GB_ERROR GB_resort_data_base(GBDATA *gb_main, GBDATA **new_order_list, long listsize) {
    long new_index;
    GBCONTAINER *father;
    struct gb_header_list_struct *hl, h;

    if (GB_read_clients(gb_main)<0)
        return GB_export_error("Sorry: this program is not the arbdb server, you cannot resort your data");

    if (GB_read_clients(gb_main)>0)
        return GB_export_errorf("There are %li clients (editors, tree programs) connected to this server,\n"
                                "please close clients and rerun operation",
                                GB_read_clients(gb_main));

    if (listsize <= 0) return 0;

    father = GB_FATHER(new_order_list[0]);
    GB_disable_quicksave(gb_main, "some entries in the database got a new order");
    hl = GB_DATA_LIST_HEADER(father->d);

    for (new_index = 0; new_index< listsize; new_index++)
    {
        long old_index = new_order_list[new_index]->index;

        if (old_index < new_index)
            GB_warningf("Warning at resort database: entry exists twice: %li and %li",
                        old_index, new_index);
        else
        {
            GBDATA *ngb;
            GBDATA *ogb;
            ogb = GB_HEADER_LIST_GBD(hl[old_index]);
            ngb = GB_HEADER_LIST_GBD(hl[new_index]);

            h = hl[new_index];
            hl[new_index] = hl[old_index];
            hl[old_index] = h;              // Warning: Relative Pointers are incorrect !!!

            SET_GB_HEADER_LIST_GBD(hl[old_index], ngb);
            SET_GB_HEADER_LIST_GBD(hl[new_index], ogb);

            if (ngb)    ngb->index = old_index;
            if (ogb)    ogb->index = new_index;
        }
    }

    gb_touch_entry((GBDATA *)father, GB_NORMAL_CHANGE);
    return 0;
}

GB_ERROR GB_resort_system_folder_to_top(GBDATA *gb_main) {
    GBDATA *gb_system = GB_entry(gb_main, GB_SYSTEM_FOLDER);
    GBDATA *gb_first = GB_child(gb_main);
    GBDATA **new_order_list;
    GB_ERROR error = 0;
    int i, len;
    if (GB_read_clients(gb_main)<0) return 0; // we are not server
    if (!gb_system) {
        return GB_export_error("System databaseentry does not exist");
    }
    if (gb_first == gb_system) return 0;
    len = GB_number_of_subentries(gb_main);
    new_order_list = (GBDATA **)GB_calloc(sizeof(GBDATA *), len);
    new_order_list[0] = gb_system;
    for (i=1; i<len; i++) {
        new_order_list[i] = gb_first;
        do {
            gb_first = GB_nextChild(gb_first);
        } while (gb_first == gb_system);
    }
    error = GB_resort_data_base(gb_main, new_order_list, len);
    free(new_order_list);
    return error;
}

// -------------------
//      user flags

GB_ERROR GB_write_usr_public(GBDATA *gbd, long flags) {
    GB_TEST_TRANSACTION(gbd);
    if (GB_GET_SECURITY_WRITE(gbd) > GB_MAIN(gbd)->security_level)
        return gb_security_error(gbd);
    gbd->flags.user_flags = flags;
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    return 0;
}

long GB_read_usr_public(GBDATA *gbd)
{
    GB_TEST_TRANSACTION(gbd);
    return (long)gbd->flags.user_flags;
}

// ------------------------------
//      private(?) user flags

long GB_read_usr_private(GBDATA *gbd) {
    GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    if (GB_TYPE(gbc) != GB_DB) {
        GB_ERROR error = GB_export_errorf("GB_write_usr_private: not a container (%s)", GB_read_key_pntr(gbd));
        GB_internal_error(error);
        return 0;
    }
    return gbc->flags2.usr_ref;
}

GB_ERROR GB_write_usr_private(GBDATA *gbd, long ref) {
    GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    if (GB_TYPE(gbc) != GB_DB) {
        GB_ERROR error = GB_export_errorf("GB_write_usr_private: not a container (%s)", GB_read_key_pntr(gbd));
        GB_internal_error(error);
        return 0;
    }
    gbc->flags2.usr_ref = ref;
    return 0;
}

// ------------------------
//      mark DB entries

void GB_write_flag(GBDATA *gbd, long flag) {
    GBCONTAINER *gbc  = (GBCONTAINER *)gbd;
    int          prev;
    int          ubit = GB_MAIN(gbd)->users[0]->userbit;

    GB_TEST_TRANSACTION(gbd);

    prev = GB_ARRAY_FLAGS(gbc).flags;
    gbd->flags.saved_flags = prev;

    if (flag) {
        GB_ARRAY_FLAGS(gbc).flags |= ubit;
    }
    else {
        GB_ARRAY_FLAGS(gbc).flags &= ~ubit;
    }
    if (prev != (int)GB_ARRAY_FLAGS(gbc).flags) {
        gb_touch_entry(gbd, GB_NORMAL_CHANGE);
        gb_touch_header(GB_FATHER(gbd));
        GB_DO_CALLBACKS(gbd);
    }
}

int GB_read_flag(GBDATA *gbd) {
    GB_TEST_TRANSACTION(gbd);
    if (GB_ARRAY_FLAGS(gbd).flags & GB_MAIN(gbd)->users[0]->userbit) return 1;
    else return 0;
}

void GB_touch(GBDATA *gbd) {
    GB_TEST_TRANSACTION(gbd);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
}


char GB_type_2_char(GB_TYPES type) {
    const char *type2char = "-bcif-B-CIFlSS-%";
    return type2char[type];
}

static void dump(const char *data, int size) {
    int x = 0;

    printf("\nDump %p (%i Byte):\n", data, size);

    while (size--)
    {
        const char *hex = "0123456789abcdef";
        char c = *data++;

        printf("%c%c ", hex[(c&0xf0)>>4], hex[c&0x0f]);

        if (++x==32)
        {
            x = 0;
            printf("\n");
        }
    }

    printf("\n");
}

GB_ERROR GB_print_debug_information(void *dummy, GBDATA *gb_main) {
    int i;
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    GB_push_transaction(gb_main);
    dummy = dummy;
    for (i=0; i<Main->keycnt; i++) {
        if (Main->keys[i].key) {
            printf("%3i %20s    nref %i\n", i, Main->keys[i].key, (int)Main->keys[i].nref);
        }
        else {
            printf("    %3i unused key, next free key = %li\n", i, Main->keys[i].next_free_key);
        }
    }
    gbm_debug_mem(Main);
    GB_pop_transaction(gb_main);
    return 0;
}

int GB_info_deep = 15;


int gb_info(GBDATA *gbd, int deep) {
    GBCONTAINER *gbc;
    GB_TYPES type;
    char    *data;
    int     size;
    GB_MAIN_TYPE *Main;

    if (gbd==NULL) { printf("NULL\n"); return -1; }
    GB_push_transaction(gbd);
    type = (GB_TYPES)GB_TYPE(gbd);

    if (deep) {
        printf("    ");
    }

    printf("(GBDATA*)0x%lx (GBCONTAINER*)0x%lx ", (long)gbd, (long)gbd);

    if (gbd->rel_father==0) { printf("father=NULL\n"); return -1; }

    if (type==GB_DB)    { gbc = (GBCONTAINER*) gbd; Main = GBCONTAINER_MAIN(gbc); }
    else        { gbc = NULL; Main = GB_MAIN(gbd); }

    if (!Main)                  { printf("Oops - I have no main entry!!!\n"); return -1; }
    if (gbd==(GBDATA*)(Main->dummy_father))     { printf("dummy_father!\n"); return -1; }

    printf("%10s Type '%c'  ", GB_read_key_pntr(gbd), GB_type_2_char(type));

    switch (type)
    {
        case GB_DB:
            gbc = (GBCONTAINER *)gbd;
            size = gbc->d.size;
            printf("Size %i nheader %i hmemsize %i", gbc->d.size, gbc->d.nheader, gbc->d.headermemsize);
            printf(" father=(GBDATA*)0x%lx\n", (long)GB_FATHER(gbd));
            if (size < GB_info_deep) {
                int index;
                struct gb_header_list_struct *header;

                header = GB_DATA_LIST_HEADER(gbc->d);
                for (index = 0; index < gbc->d.nheader; index++) {
                    GBDATA *gb_sub = GB_HEADER_LIST_GBD(header[index]);
                    printf("\t\t%10s (GBDATA*)0x%lx (GBCONTAINER*)0x%lx\n", Main->keys[header[index].flags.key_quark].key, (long)gb_sub, (long)gb_sub);
                }
            }
            break;
        default:
            data = GB_read_as_string(gbd);
            if (data) { printf("%s", data); free(data); }
            printf(" father=(GBDATA*)0x%lx\n", (long)GB_FATHER(gbd));
    }


    GB_pop_transaction(gbd);

    return 0;
}


int GB_info(GBDATA *gbd)
{
    return gb_info(gbd, 0);
}

long GB_number_of_subentries(GBDATA *gbd)
{
    long subentries = -1; 

    if (GB_TYPE(gbd) == GB_DB) {
        GBCONTAINER *gbc = (GBCONTAINER*)gbd;

        if (GB_is_server(gbd)) {
            subentries = gbc->d.size;
        }
        else { // client really needs to count entries in header
            int                           end    = gbc->d.nheader;
            struct gb_header_list_struct *header = GB_DATA_LIST_HEADER(gbc->d);
            int                           index;

            subentries = 0;
            for (index = 0; index<end; index++) {
                if ((int)header[index].flags.changed < GB_DELETED) subentries++;
            }
        }
    }

    return subentries;
}

