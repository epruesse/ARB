/* #include <stdio.h> */
#include <stdlib.h>
#include <string.h>
/* #include <malloc.h> */
#include <sys/types.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "adlocal.h"
/* #include "arbdb.h" */

struct gb_local_data *gb_local = 0;
#ifdef ARBDB_SIZEDEBUG
long *arbdb_stat;
#endif


long    GB_NOVICE  = 0;


char *GB_rel(void *struct_adress,long rel_adress)
{
    if (!rel_adress) return NULL;
    return (char*)struct_adress+rel_adress;
}
/********************************************************************************************
                    GB local data
********************************************************************************************/

double GB_atof(const char *str)
{
    double res = 0.0;
    double val = 1.0;
    register long neg = 0;
    register char c;
    register char *p = (char *)str;
    while ( (c= *(p++)) ){
        if (c == '.'){
            val = 0.1;
            continue;
        }
        if (c == 'e' || c == 'E' ) {
            return atof(str);
        }
        if (c== '-') {
            neg = 1;
            continue;
        }
        if (val == 1.0) {
            res*= 10.0;
            res += c-'0';
            continue;
        }
        res += (c-'0')*val;
        val *= .1;
    }
    if (neg) return -res;
    return res;
}


/********************************************************************************************
                    compression tables
********************************************************************************************/
int gb_convert_type_2_compression_flags[] = {
    /* GB_NONE 0 */ GB_COMPRESSION_NONE,
    /* GB_BIT  1 */ GB_COMPRESSION_NONE,
    /* GB_BYTE 2 */ GB_COMPRESSION_NONE,
    /* GB_INT  3 */ GB_COMPRESSION_NONE,
    /* GB_FLOAT4 */ GB_COMPRESSION_NONE,
    /* GB_??   5 */ GB_COMPRESSION_NONE,
    /* GB_BITS 6 */ GB_COMPRESSION_BITS,
    /* GB_??   7 */ GB_COMPRESSION_NONE,
    /* GB_BYTES8 */ GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN,
    /* GB_INTS 9 */ GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN | GB_COMPRESSION_SORTBYTES,
    /* GB_FLTS 10 */    GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN | GB_COMPRESSION_SORTBYTES,
    /* GB_??   11 */    GB_COMPRESSION_NONE,
    /* GB_STR  12 */    GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN | GB_COMPRESSION_DICTIONARY,
    /* GB_STRS 13 */    GB_COMPRESSION_NONE,
    /* GB??    14 */    GB_COMPRESSION_NONE,
    /* GB_DB   15 */    GB_COMPRESSION_NONE
};

int gb_convert_type_2_sizeof[] = {
    /* GB_NONE 0 */ 0,
    /* GB_BIT  1 */ 0,
    /* GB_BYTE 2 */ sizeof(char),
    /* GB_INT  3 */ sizeof(int),
    /* GB_FLOAT4 */ sizeof(float),
    /* GB_??   5 */ 0,
    /* GB_BITS 6 */ 0,
    /* GB_??   7 */ 0,
    /* GB_BYTES8 */ sizeof(char),
    /* GB_INTS 9 */ sizeof(int),
    /* GB_FLTS 10 */    sizeof(float),
    /* GB_??   11 */    0,
    /* GB_STR  12 */    sizeof(char),
    /* GB_STRS 13 */    sizeof(char),
    /* GB??    14 */    0,
    /* GB_DB   15 */    0,
};

int gb_convert_type_2_appendix_size[] = {
    /* GB_NONE 0 */ 0,
    /* GB_BIT  1 */ 0,
    /* GB_BYTE 2 */ 0,
    /* GB_INT  3 */ 0,
    /* GB_FLOAT4 */ 0,
    /* GB_??   5 */ 0,
    /* GB_BITS 6 */ 0,
    /* GB_??   7 */ 0,
    /* GB_BYTES8 */ 0,
    /* GB_INTS 9 */ 0,
    /* GB_FLTS 10 */    0,
    /* GB_??   11 */    0,
    /* GB_STR  12 */    1,      /* '\0' termination */
    /* GB_STRS 13 */    1,
    /* GB??    14 */    0,
    /* GB_DB   15 */    0,
};


/********************************************************************************************
                    GB local data
********************************************************************************************/

unsigned char GB_BIT_compress_data[] = {
    0x1d,gb_cs_ok,0,0,
    0x04,gb_cs_ok,0,1,
    0x0a,gb_cs_ok,0,2,
    0x0b,gb_cs_ok,0,3,
    0x0c,gb_cs_ok,0,4,
    0x1a,gb_cs_ok,0,5,
    0x1b,gb_cs_ok,0,6,
    0x1c,gb_cs_ok,0,7,
    0xf0,gb_cs_ok,0,8,
    0xf1,gb_cs_ok,0,9,
    0xf2,gb_cs_ok,0,10,
    0xf3,gb_cs_ok,0,11,
    0xf4,gb_cs_ok,0,12,
    0xf5,gb_cs_ok,0,13,
    0xf6,gb_cs_ok,0,14,
    0xf7,gb_cs_ok,0,15,
    0xf8,gb_cs_sub,0,16,
    0xf9,gb_cs_sub,0,32,
    0xfa,gb_cs_sub,0,48,
    0xfb,gb_cs_sub,0,64,
    0xfc,gb_cs_sub,0,128,
    0xfd,gb_cs_sub,1,0,
    0xfe,gb_cs_sub,2,0,
    0xff,gb_cs_sub,4,0,
    0
};

void gb_init_gb(void)
{

    if (gb_local) return;
    gb_local = (struct gb_local_data *)gbm_get_mem(sizeof(struct gb_local_data),0);
    gb_local->bufsize = 4000;
    gb_local->buffer2 = (char *)malloc((size_t)gb_local->bufsize);
    gb_local->bufsize2 = 4000;
    gb_local->buffer = (char *)malloc((size_t)gb_local->bufsize);
    gb_local->write_bufsize = GBCM_BUFFER;
    gb_local->write_buffer = (char *)malloc((size_t)gb_local->write_bufsize);
    gb_local->write_ptr = gb_local->write_buffer;
    gb_local->write_free = gb_local->write_bufsize;
    gb_local->bituncompress = gb_build_uncompress_tree(GB_BIT_compress_data,1,0);
    gb_local->bitcompress = gb_build_compress_list(GB_BIT_compress_data,1,&(gb_local->bc_size));
#ifdef ARBDB_SIZEDEBUG
    arbdb_stat = (long *)GB_calloc(sizeof(long),1000);
#endif
}

/********************************************************************************************
                    local buffer management
********************************************************************************************/
/* return a pointer to a static piece of memory at least size bytes long */
GB_CPNTR GB_give_buffer(long size)
{
#if (MEMORY_TEST==1)
    if (gb_local->buffer) free(gb_local->buffer);
    gb_local->bufsize = size;
    gb_local->buffer = (char *)malloc((size_t)gb_local->bufsize);
#else
    if (size < gb_local->bufsize) return    gb_local->buffer;
    if (gb_local->buffer) free(gb_local->buffer);
    gb_local->bufsize = size;
    gb_local->buffer = (char *)GB_calloc((size_t)gb_local->bufsize,1);
#endif
    return  gb_local->buffer;
}

GB_CPNTR gb_increase_buffer(long size){
    char *old_buffer;
    if (size < gb_local->bufsize) return    gb_local->buffer;
    old_buffer = gb_local->buffer;
    gb_local->buffer = (char *)GB_calloc((size_t)size,1);
    GB_MEMCPY(gb_local->buffer,old_buffer, gb_local->bufsize);
    gb_local->bufsize = size;
    free(old_buffer);
    return gb_local->buffer;
}

int GB_give_buffer_size(){
    return gb_local->bufsize;
}

GB_CPNTR GB_give_buffer2(long size)
{
#if (MEMORY_TEST==1)
    if (gb_local->buffer2) free(gb_local->buffer2);
    gb_local->bufsize2 = size;
    gb_local->buffer2 = (char *)malloc((size_t)gb_local->bufsize2);
#else
    if (size < gb_local->bufsize2) return   gb_local->buffer2;
    if (gb_local->buffer2) free(gb_local->buffer2);
    gb_local->bufsize2 = size;
    gb_local->buffer2 = (char *)GB_calloc((size_t)gb_local->bufsize2,1);
#endif
    return  gb_local->buffer2;
}
/* Check a piece of memory out of the buffer management
                 * after it is checked out, the user has the full control to use and free it*/
char *gb_check_out_buffer(const char *buffer){
    char *old;
    if (buffer >= gb_local->buffer && buffer< gb_local->buffer + gb_local->bufsize){
        old = gb_local->buffer;
        gb_local->buffer = 0;
        gb_local->bufsize = 0;
    }else if (buffer >= gb_local->buffer2 && buffer< gb_local->buffer2 + gb_local->bufsize2){
        old = gb_local->buffer2;
        gb_local->buffer2 = 0;
        gb_local->bufsize2 = 0;
    }else{
        return 0;       /* Nothing to check out */
    }
    return old;
}

GB_CPNTR GB_give_other_buffer(const char *buffer, long size)
{
    if (buffer >= gb_local->buffer && buffer< gb_local->buffer + gb_local->bufsize){
        return GB_give_buffer2(size);
    }
    return GB_give_buffer(size);
}


/********************************************************************************************
            unfold -> load data from disk or server
********************************************************************************************/

GB_ERROR gb_unfold(GBCONTAINER *gbd, long deep, int index_pos)
{
    /* get data from server if deep than get subitems too
       if index_pos >=0 than get indexed item from server */
    /*  if index_pos <0 get all items           */
    GB_ERROR error;
    GBDATA *gb2;
    struct gb_header_list_struct *header = GB_DATA_LIST_HEADER(gbd->d);

    if (!gbd->flags2.folded_container) return 0;
    if (index_pos> gbd->d.nheader) gb_create_header_array(gbd,index_pos + 1);
    if (index_pos >= 0  && GB_HEADER_LIST_GBD(header[index_pos])) return 0;

    if (GBCONTAINER_MAIN(gbd)->local_mode == GB_TRUE) {
        GB_internal_error("Cannot unfold local_mode database");
        return 0;
    }

    do {
        if (index_pos<0 ) break;
        if (index_pos >= gbd->d.nheader) break;
        if ( (int)header[index_pos].flags.changed >= gb_deleted){
            GB_internal_error("Tried to unfold a deleted item");
            return 0;
        }
        if (GB_HEADER_LIST_GBD(header[index_pos])) return 0;            /* already unfolded */
    } while (0);

    error= gbcm_unfold_client(gbd,deep,index_pos);
    if (error) {
        GB_print_error();
        return error;
    }

    if ( index_pos<0 ) {
        gb_untouch_children(gbd);
        gbd->flags2.folded_container = 0;
    }else{
        if ( (gb2 = GBCONTAINER_ELEM(gbd,index_pos)) ) {
            if (GB_TYPE(gb2) == GB_DB) gb_untouch_children((GBCONTAINER *)gb2);
            gb_untouch_me(gb2);
        }
    }
    return 0;
}

/********************************************************************************************
                    CLOSE DATABASE
********************************************************************************************/
GB_ERROR GB_close(GBDATA *gbd)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (!Main->local_mode){
        gbcmc_close(Main->c_link);
    }

#if defined(DEVEL_RALF)
#warning fix that error!    
#endif /* DEVEL_RALF */
    return 0;                   /* @@@ there's an error below (return to avoid crash) */

    gb_delete_entry(gbd);
    gb_do_callback_list(gbd);   /* do all callbacks */
    gb_destroy_main(Main);
    return 0;
}

GB_ERROR GB_exit(GBDATA *gbd)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (!Main->local_mode){
        gbcmc_close(Main->c_link);
    }
    return 0;
}



/********************************************************************************************
                    READ DATA
********************************************************************************************/


long GB_read_int(GBDATA *gbd)
{
    GB_TEST_READ(gbd,GB_INT,"GB_read_int");
    return  gbd->info.i;
}

int GB_read_byte(GBDATA *gbd)
{
    GB_TEST_READ(gbd,GB_BYTE,"GB_read_byte");
    return gbd->info.i;
}

double GB_read_float(GBDATA *gbd)
{
    XDR xdrs;
    static float f;
    GB_TEST_READ(gbd,GB_FLOAT,"GB_read_float");
    xdrmem_create(&xdrs, &gbd->info.in.data[0],SIZOFINTERN,XDR_DECODE);
    xdr_float(&xdrs,&f);
    xdr_destroy(&xdrs);
    return (double)f;
}

long GB_read_count(GBDATA *gbd) {
    return GB_GETSIZE(gbd);
}

long GB_read_memuse(GBDATA *gbd) {
    return GB_GETMEMSIZE(gbd);
}

GB_CPNTR GB_read_pntr(GBDATA *gbd){
    char *data;
    long size;
    int type;
    type = GB_TYPE(gbd);
    data = GB_GETDATA(gbd);
    if (!data) return 0;
    if (gbd->flags.compressed_data) {   /* uncompressed data return pntr to
                                           database entry   */
        char *ca = gb_read_cache(gbd);
        char *da;

        if (ca) return ca;

        size = GB_GETSIZE(gbd) * gb_convert_type_2_sizeof[type] + gb_convert_type_2_appendix_size[type];
        ca = gb_alloc_cache_index(gbd,size);
        da = gb_uncompress_data(gbd,data,size);
        GB_MEMCPY(ca,da,size);
        return ca;
    }else{
        return data;
    }
}

int gb_read_nr(GBDATA *gbd){
    return gbd->index;
}

GB_CPNTR GB_read_char_pntr(GBDATA *gbd) /* @@@ change return type to const char * */
{
    GB_TEST_READ(gbd,GB_STRING,"GB_read_char_pntr");
    return GB_read_pntr(gbd);
}

char *GB_read_string(GBDATA *gbd)
{
    const char *d;
    GB_TEST_READ(gbd,GB_STRING,"GB_read_string");
    d = GB_read_pntr(gbd);
    if (!d) return 0;
    return gbs_malloc_copy(d,GB_GETSIZE(gbd)+1);
}

long GB_read_string_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd,GB_STRING,"GB_read_string_count");
    return GB_GETSIZE(gbd);
}

GB_CPNTR GB_read_link_pntr(GBDATA *gbd)
{
    GB_TEST_READ(gbd,GB_LINK,"GB_read_link_pntr");
    return GB_read_pntr(gbd);
}

char *GB_read_link(GBDATA *gbd)
{
    const char *d;
    GB_TEST_READ(gbd,GB_LINK,"GB_read_link_pntr");
    d = GB_read_pntr(gbd);
    if (!d) return 0;
    return gbs_malloc_copy(d,GB_GETSIZE(gbd)+1);
}

long GB_read_link_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd,GB_LINK,"GB_read_link_pntr");
    return GB_GETSIZE(gbd);
}


long GB_read_bits_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd,GB_BITS,"GB_read_bits_count");
    return GB_GETSIZE(gbd);
}

GB_CPNTR GB_read_bits_pntr(GBDATA *gbd,char c_0, char c_1)
{
    char *data;
    long size;
    GB_TEST_READ(gbd,GB_BITS,"GB_read_bits_pntr");
    data = GB_GETDATA(gbd);
    size = GB_GETSIZE(gbd);
    if (!size) return 0;
    {
        char *ca = gb_read_cache(gbd);
        char *da;
        if (ca) return ca;
        ca = gb_alloc_cache_index(gbd,size+1);
        da = gb_uncompress_bits(data,size,c_0,c_1);
        if (ca) {
            GB_MEMCPY(ca,da,size+1);
            return ca;
        }else{
            return da;
        }
    }
}

char *GB_read_bits(GBDATA *gbd, char c_0, char c_1)
{
    char *d;
    d = GB_read_bits_pntr(gbd,c_0,c_1);
    if (!d) return 0;
    d = gbs_malloc_copy(d,GB_GETSIZE(gbd)+1);
    return d;
}


GB_CPNTR GB_read_bytes_pntr(GBDATA *gbd)
{
    GB_TEST_READ(gbd,GB_BYTES,"GB_read_bytes_pntr");
    return GB_read_pntr(gbd);
}

long GB_read_bytes_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd,GB_BYTES,"GB_read_bytes_count");
    return GB_GETSIZE(gbd);
}

char *GB_read_bytes(GBDATA *gbd)
{
    char *d;
    d = GB_read_bytes_pntr(gbd);
    if (!d) return 0;
    d = gbs_malloc_copy(d,GB_GETSIZE(gbd));
    return d;
}

GB_CUINT4 *GB_read_ints_pntr(GBDATA *gbd)
{
    GB_UINT4 *res;
    GB_TEST_READ(gbd,GB_INTS,"GB_read_ints_pntr");

    if (gbd->flags.compressed_data) {
        res = (GB_UINT4 *)GB_read_pntr(gbd);
    }else{
        res = (GB_UINT4 *)GB_GETDATA(gbd);
    }
    if (!res) return 0;

    if ( 0x01020304 == htonl((u_long)0x01020304) ) {
        return res;
    }else{
        register long i;
        int size = GB_GETSIZE(gbd);
        char *buf2 = GB_give_other_buffer((char *)res,size<<2);
        register GB_UINT4 *s = (GB_UINT4 *)res;
        register GB_UINT4 *d = (GB_UINT4 *)buf2;
        for (i=size;i;i--) {
            *(d++) = htonl(*(s++));
        }
        return (GB_UINT4 *)buf2;
    }
}

long GB_read_ints_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd,GB_INTS,"GB_read_ints_count");
    return GB_GETSIZE(gbd);
}

GB_UINT4 *GB_read_ints(GBDATA *gbd)
{
    GB_CUINT4 *i;
    i = GB_read_ints_pntr(gbd);
    if (!i) return 0;
    return  (GB_UINT4 *)gbs_malloc_copy((char *)i,GB_GETSIZE(gbd)*sizeof(GB_UINT4));
}

GB_CFLOAT *GB_read_floats_pntr(GBDATA *gbd)
{
    char *buf2;
    char *res;

    GB_TEST_READ(gbd,GB_FLOATS,"GB_read_floats_pntr");
    if (gbd->flags.compressed_data) {
        res = (char *)GB_read_pntr(gbd);
    }else{
        res = (char *)GB_GETDATA(gbd);
    }
    if (!res) return 0;
    {
        XDR xdrs;
        register float *d;
        register long i;
        long size = GB_GETSIZE(gbd);
        long full_size = size*sizeof(float);

        xdrmem_create(&xdrs, res,(int)(full_size),XDR_DECODE);
        buf2 = GB_give_other_buffer(res,full_size);
        d = (float *)buf2;
        for (i=size;i;i--) {
            xdr_float(&xdrs,d);
            d++;
        }
        xdr_destroy(&xdrs);
    }
    return (float *)buf2;
}

long GB_read_floats_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd,GB_FLOATS,"GB_read_floats_count");
    return GB_GETSIZE(gbd);
}

float *GB_read_floats(GBDATA *gbd)
{
    GB_CFLOAT *f;
    f = GB_read_floats_pntr(gbd);
    if (!f) return 0;
    return  (float *)gbs_malloc_copy((char *)f,GB_GETSIZE(gbd)*sizeof(float));
}

char *GB_read_as_string(GBDATA *gbd)
{
    char buffer[256];
    switch (GB_TYPE(gbd)) {
        case GB_STRING: return GB_read_string(gbd);
        case GB_LINK:   return GB_read_link(gbd);
        case GB_BYTE:   sprintf(buffer,"%i",GB_read_byte(gbd)); return GB_STRDUP(buffer);
        case GB_INT:    sprintf(buffer,"%li",GB_read_int(gbd)); return GB_STRDUP(buffer);
        case GB_FLOAT:  sprintf(buffer,"%.4g",GB_read_float(gbd)); return GB_STRDUP(buffer);
        case GB_BITS:   return GB_read_bits(gbd,'0','1');
            /* Be careful : When adding new types here, you have to make sure that
             * GB_write_as_string is able to write them back and that this makes sense.
             */
        default:    return 0;
    }
}


/********************************************************************************************
                    WRITE DATA
********************************************************************************************/


GB_ERROR GB_write_byte(GBDATA *gbd,int i)
{
    GB_TEST_WRITE(gbd,GB_BYTE,"GB_write_byte");
    if (gbd->info.i != i){
        gb_save_extern_data_in_ts(gbd);
        gbd->info.i = i && 0xff;
        gb_touch_entry(gbd,gb_changed);
        GB_DO_CALLBACKS(gbd);
    }
    return 0;
}

GB_ERROR GB_write_int(GBDATA *gbd,long i)
{
    GB_TEST_WRITE(gbd,GB_INT,"GB_write_int");
    if (gbd->info.i != i){
        gb_save_extern_data_in_ts(gbd);
        gbd->info.i = i;
        gb_touch_entry(gbd,gb_changed);
        GB_DO_CALLBACKS(gbd);
    }
    return 0;
}

GB_ERROR GB_write_float(GBDATA *gbd,double f)
{
    XDR xdrs;
    static float f2;

    GB_TEST_WRITE(gbd,GB_FLOAT,"GB_write_float");

    GB_TEST_READ(gbd,GB_FLOAT,"GB_read_float");

    xdrmem_create(&xdrs, &gbd->info.in.data[0],SIZOFINTERN,XDR_DECODE);
    xdr_float(&xdrs,&f2);
    xdr_destroy(&xdrs);

    if (f2 != f) {
        f2 = f;
        gb_save_extern_data_in_ts(gbd);
        xdrmem_create(&xdrs, &gbd->info.in.data[0],SIZOFINTERN,XDR_ENCODE);
        xdr_float(&xdrs,&f2);
        xdr_destroy(&xdrs);
        gb_touch_entry(gbd,gb_changed);
        GB_DO_CALLBACKS(gbd);
    }
    xdr_destroy(&xdrs);
    return 0;
}

GB_ERROR gb_write_compressed_pntr(GBDATA *gbd,const char *s, long memsize , long stored_size){
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    gb_free_cache(Main,gbd);
    gb_save_extern_data_in_ts(gbd);
    gbd->flags.compressed_data = 1;
    GB_SETSMDMALLOC(gbd,stored_size,(size_t)memsize,(char *)s);
    gb_touch_entry(gbd,gb_changed);
    return 0;
}

int gb_get_compression_mask(GB_MAIN_TYPE *Main, GBQUARK key, int gb_type)
{
    struct gb_key_struct *ks = &Main->keys[key];
    int compression_mask;

    if (ks->gb_key_disabled) {
        compression_mask = 0;
    } else {
        if (!ks->gb_key) gb_load_single_key_data((GBDATA*)Main->data, key);
        compression_mask = gb_convert_type_2_compression_flags[gb_type] & ks->compression_mask;
    }

    return compression_mask;
}

GB_ERROR GB_write_pntr(GBDATA *gbd,const char *s, long bytes_size, long stored_size)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GBQUARK key = GB_KEY_QUARK(gbd);
    char *d;
    int compression_mask;
    long memsize;

    gb_free_cache(Main,gbd);
    gb_save_extern_data_in_ts(gbd);

    compression_mask = gb_get_compression_mask(Main, key, GB_TYPE(gbd));

    if (compression_mask){
        d = gb_compress_data(gbd, key, s, bytes_size, &memsize, compression_mask, GB_FALSE);
    }else{
        d = NULL;
    }
    if (d){
        gbd->flags.compressed_data = 1;
    }else{
        d = (char *)s;
        gbd->flags.compressed_data = 0;
        memsize = bytes_size;
    }

    GB_SETSMDMALLOC(gbd,stored_size,memsize,d);
    gb_touch_entry(gbd,gb_changed);
    GB_DO_CALLBACKS(gbd);

    return 0;
}

GB_ERROR GB_write_string(GBDATA *gbd,const char *s)
{
    long size;
    /*fprintf(stderr, "GB_write_string(%p, %s);\n", gbd, s);*/
    GB_TEST_WRITE(gbd,GB_STRING,"GB_write_string");
    GB_TEST_NON_BUFFER(s,"GB_write_string");
    if (!s) s="";
    size = strlen(s);

    /* no zero len strings allowed */
    if ((GB_GETMEMSIZE(gbd))  && (size == GB_GETSIZE(gbd)))
    {
        if (!strcmp(s,GB_read_pntr(gbd)))
            return 0;
    }
#if defined(DEBUG) && 0
    // check for error (in compression)
    {
        GB_ERROR error = GB_write_pntr(gbd,s,size+1,size);
        if (!error) {
            char *check = GB_read_string(gbd);

            gb_assert(check);
            gb_assert(strcmp(check, s) == 0);

            free(check);
        }
        return error;
    }
#else
    return GB_write_pntr(gbd,s,size+1,size);
#endif /* DEBUG */
}

GB_ERROR GB_write_link(GBDATA *gbd,const char *s)
{
    long size;
    GB_TEST_WRITE(gbd,GB_STRING,"GB_write_link");
    GB_TEST_NON_BUFFER(s,"GB_write_link");
    if (!s) s="";
    size = strlen(s);

    /* no zero len strings allowed */
    if ((GB_GETMEMSIZE(gbd))  && (size == GB_GETSIZE(gbd)))
    {
        if (!strcmp(s,GB_read_pntr(gbd)))
            return 0;
    }
    return GB_write_pntr(gbd,s,size+1,size);
}


GB_ERROR GB_write_bits(GBDATA *gbd,const char *bits,long size,char c_0)
{
    char *d;
    long memsize[2];

    GB_TEST_WRITE(gbd,GB_BITS,"GB_write_bits");
    GB_TEST_NON_BUFFER(bits,"GB_write_bits");
    gb_save_extern_data_in_ts(gbd);

    d = gb_compress_bits(bits,size,c_0,memsize);
    gbd->flags.compressed_data = 1;
    GB_SETSMDMALLOC(gbd,size,memsize[0],d);
    gb_touch_entry(gbd,gb_changed);
    GB_DO_CALLBACKS(gbd);
    return 0;
}

GB_ERROR GB_write_bytes(GBDATA *gbd,const char *s,long size)
{
    GB_TEST_WRITE(gbd,GB_BYTES,"GB_write_bytes");
    return GB_write_pntr(gbd,s,size,size);
}

GB_ERROR GB_write_ints(GBDATA *gbd,const GB_UINT4 *i,long size)
{

    GB_TEST_WRITE(gbd,GB_INTS,"GB_write_ints");
    GB_TEST_NON_BUFFER((char *)i,"GB_write_ints");      /* compress will destroy the other buffer */

    if ( 0x01020304 != htonl((GB_UINT4)0x01020304) ) {
        register long j;
        char *buf2 = GB_give_other_buffer((char *)i,size<<2);
        register GB_UINT4 *s = (GB_UINT4 *)i;
        register GB_UINT4 *d = (GB_UINT4 *)buf2;
        for (j=size;j;j--) {
            *(d++) = htonl(*(s++));
        }
        i = (GB_UINT4 *)buf2;
    }
    return GB_write_pntr(gbd,(char *)i,size* 4 /*sizeof(long4)*/,size);
}

GB_ERROR GB_write_floats(GBDATA *gbd,const float *f,long size)
{
    long fullsize = size * sizeof(float);
    GB_TEST_WRITE(gbd,GB_FLOATS,"GB_write_floats");
    GB_TEST_NON_BUFFER((char *)f,"GB_write_floats");

    {
        XDR xdrs;
        register long i;
        char *buf2 = GB_give_other_buffer((char *)f,fullsize);
        register float *s = (float *)f;

        xdrmem_create(&xdrs, buf2 ,(int)fullsize ,XDR_ENCODE);
        for (i=size;i;i--) {
            xdr_float(&xdrs,s);
            s++;
        }
        xdr_destroy (&xdrs);
        f = (float *)buf2;
    }
    return GB_write_pntr(gbd,(char *)f,size*sizeof(float),size);
}

GB_ERROR GB_write_as_string(GBDATA *gbd,const char *val)
{
    switch (GB_TYPE(gbd)) {
        case GB_STRING: return GB_write_string(gbd,val);
        case GB_LINK:   return GB_write_link(gbd,val);
        case GB_BYTE:   return GB_write_byte(gbd,atoi(val));
        case GB_INT:    return GB_write_int(gbd,atoi(val));
        case GB_FLOAT:  return GB_write_float(gbd,GB_atof(val));
        case GB_BITS:   return GB_write_bits(gbd,val,strlen(val),'0');
        default:    return GB_export_error("Error: You cannot use GB_write_as_string on this type of entry (%s)",GB_read_key_pntr(gbd));
    }
}

/********************************************************************************************
                    Key Information
********************************************************************************************/
int GB_read_security_write(GBDATA *gbd)
{
    GB_TEST_TRANSACTION(gbd);
    return GB_GET_SECURITY_WRITE(gbd);}
int GB_read_security_read(GBDATA *gbd)
{
    GB_TEST_TRANSACTION(gbd);
    return GB_GET_SECURITY_READ(gbd);}
int GB_read_security_delete(GBDATA *gbd)
{
    GB_TEST_TRANSACTION(gbd);
    return GB_GET_SECURITY_DELETE(gbd);}

int GB_get_my_security(GBDATA *gbd)
{
    return GB_MAIN(gbd)->security_level;
}

GB_ERROR gb_security_error(GBDATA *gbd){
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    return GB_export_error("Security Error: You tried to change an entry '%s' with security level %i\n"
                           "    and your current security level is %i",
                           GB_read_key_pntr(gbd),
                           GB_GET_SECURITY_WRITE(gbd),
                           Main->security_level);
}

GB_ERROR GB_write_security_write(GBDATA *gbd,unsigned long level)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_TEST_TRANSACTION(gbd);

    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    if (GB_GET_SECURITY_WRITE(gbd) == level) return 0;
    GB_PUT_SECURITY_WRITE(gbd,level);
    gb_touch_entry(gbd,gb_changed);
    GB_DO_CALLBACKS(gbd);
    return 0;
}
GB_ERROR GB_write_security_read(GBDATA *gbd,unsigned long level)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_TEST_TRANSACTION(gbd);
    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    if (GB_GET_SECURITY_READ(gbd) == level) return 0;
    GB_PUT_SECURITY_READ(gbd,level);
    gb_touch_entry(gbd,gb_changed);
    GB_DO_CALLBACKS(gbd);
    return 0;
}

GB_ERROR GB_write_security_delete(GBDATA *gbd,unsigned long level)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_TEST_TRANSACTION(gbd);
    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    if (GB_GET_SECURITY_DELETE(gbd) == level) return 0;
    GB_PUT_SECURITY_DELETE(gbd,level);
    gb_touch_entry(gbd,gb_changed);
    GB_DO_CALLBACKS(gbd);
    return 0;
}
GB_ERROR GB_write_security_levels(GBDATA *gbd,unsigned long readlevel,unsigned long writelevel,unsigned long deletelevel)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_TEST_TRANSACTION(gbd);
    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    GB_PUT_SECURITY_WRITE(gbd,writelevel);
    GB_PUT_SECURITY_READ(gbd,readlevel);
    GB_PUT_SECURITY_DELETE(gbd,deletelevel);
    gb_touch_entry(gbd,gb_changed);
    GB_DO_CALLBACKS(gbd);
    return 0;
}

GB_ERROR GB_change_my_security(GBDATA *gbd,int level,const char *passwd)
{
    int i;
    i = level;
    if (i<0) i=0;
    if (i>=8) i = 7;
    GB_MAIN(gbd)->security_level = i;
    passwd = passwd;    /*dummy for compiler */
    return 0;
}

/* For internal use only */
GB_ERROR GB_push_my_security(GBDATA *gbd)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    Main->pushed_security_level++;
    if (Main->pushed_security_level>1){
        return 0;
    }

    Main->old_security_level = Main->security_level;
    Main->security_level = 7;
    return 0;
}

GB_ERROR GB_pop_my_security(GBDATA *gbd)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    Main->pushed_security_level--;
    if (Main->pushed_security_level>0){
        return 0;
    }
    Main->security_level = Main->old_security_level;
    return 0;
}

GB_TYPES GB_read_type(GBDATA *gbd)
{
    GB_TEST_TRANSACTION(gbd);
    return (GB_TYPES)GB_TYPE(gbd);
}

char *GB_read_key(GBDATA *gbd){
    char *k;
    GB_TEST_TRANSACTION(gbd);
    k = GB_KEY(gbd);
    if (!k) return GB_STRDUP("_null_");
    return GB_STRDUP(k);
}

GB_CSTR GB_read_key_pntr(GBDATA *gbd){
    GB_TEST_TRANSACTION(gbd);
    return GB_KEY(gbd);
}

GB_CSTR gb_read_key_pntr(GBDATA *gbd){
    return GB_KEY(gbd);
}


GBQUARK GB_key_2_quark(GBDATA *gbd, const char *s) {
    register long index;
    register GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (!s) return -1;
    index = GBS_read_hash(Main->key_2_index_hash,s);
    if (!index) {   /* create new index */
        index = gb_create_key(Main,s,GB_TRUE);
    }
    return (GBQUARK)index;
}

GBQUARK GB_get_quark(GBDATA *gbd) {
    return GB_KEY_QUARK(gbd);
}

GBQUARK gb_key_2_quark(GB_MAIN_TYPE *Main, const char *s) {
    register long index;
    if (!s) return 0;
    index = GBS_read_hash(Main->key_2_index_hash,s);
    if (!index) {   /* create new index */
        index = gb_create_key(Main,s,GB_TRUE);
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

/********************************************************************************************
                    Get and check the database hierarchy
********************************************************************************************/

GBDATA *GB_get_father(GBDATA *gbd) /* Get the father of an entry */
{
    GBDATA *father;

    GB_TEST_TRANSACTION(gbd);
    if (!(father=(GBDATA*)GB_FATHER(gbd)))  return NULL;
    if (!GB_FATHER(father))         return NULL;

    return father;
}

GBDATA *GB_get_root(GBDATA *gbd) {  /* Get the root entry (gb_main) */
    return (GBDATA *)GB_MAIN(gbd)->data;
}

GB_BOOL GB_check_father(GBDATA *gbd, GBDATA *gb_maybefather) {
    /* Test whether an entry is a subentry of another */
    GBDATA *gbfather;
    for (   gbfather = GB_get_father(gbd);
            gbfather;
            gbfather = GB_get_father(gbfather)){
        if (gbfather ==     gb_maybefather) return GB_TRUE;
    }
    return GB_FALSE;
}

GBDATA *gb_create(GBDATA *father,const char *key, GB_TYPES type){
    GBDATA *gbd = gb_make_entry((GBCONTAINER *)father, key, -1,0,type);
    gb_touch_header(GB_FATHER(gbd));
    gb_touch_entry(gbd,gb_created);
    return (GBDATA *)gbd;
}

/* Create a container, do not check anything */
GBDATA *gb_create_container(GBDATA *father, const char *key){
    GBCONTAINER *gbd = gb_make_container((GBCONTAINER *)father,key, -1, 0);
    gb_touch_header(GB_FATHER(gbd));
    gb_touch_entry((GBDATA *)gbd,gb_created);
    return (GBDATA *)gbd;
}

void gb_rename(GBCONTAINER *gbc, const char *new_key) {
    gb_rename_entry(gbc, new_key);
}

/* User accessible rename, check everything
 * returns 0 if successful!
 */
int GB_rename(GBDATA *gbc, const char *new_key) {
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
    gb_touch_entry(gbc, gb_changed);

    return 0;
}

/* User accessible create, check everything */
GBDATA *GB_create(GBDATA *father,const char *key, GB_TYPES type)
{
    GBDATA *gbd;

    if (GB_check_key(key)) {
        GB_print_error();
        return 0;
    }

    if (type == GB_DB) {
        gb_assert(type != GB_DB); // you like to use GB_create_container!
        GB_export_error("GB_create error: can't create containers");
        return NULL;
    }

    /*     now checked by GB_check_key */
    /*     if ( (*key == '\0')) { */
    /*         GB_export_error("GB_create error: empty key"); */
    /*         return NULL; */
    /*     } */

    if ( !father ) {
        GB_internal_error("GB_create error in GB_create:\nno father (key = '%s')",key);
        return NULL;
    }
    GB_TEST_TRANSACTION(father);
    if ( GB_TYPE(father)!=GB_DB) {
        GB_export_error("GB_create: father (%s) is not of GB_DB type (%i) (creating '%s')",
                        GB_read_key_pntr(father),GB_TYPE(father),key);
        return NULL;
    };
    gbd = gb_make_entry((GBCONTAINER *)father, key, -1,0,type);
    gb_touch_header(GB_FATHER(gbd));
    gb_touch_entry(gbd,gb_created);
    return gbd;
}

GBDATA *GB_create_container(GBDATA *father,const char *key)
{
    GBCONTAINER *gbd;
    if (GB_check_key(key)) {
        GB_print_error();
        return 0;
    }

    if ( (*key == '\0')) {
        GB_export_error("GB_create error: empty key");
        return NULL;
    }
    if ( !father ) {
        GB_internal_error("GB_create error in GB_create:\nno father (key = '%s')",key);
        return NULL;
    }
    GB_TEST_TRANSACTION(father);
    if ( GB_TYPE(father)!=GB_DB) {
        GB_export_error("GB_create: father (%s) is not of GB_DB type (%i) (creating '%s')",
                        GB_read_key_pntr(father),GB_TYPE(father),key);
        return NULL;
    };
    gbd = gb_make_container((GBCONTAINER *)father,key, -1, 0);
    gb_touch_header(GB_FATHER(gbd));
    gb_touch_entry((GBDATA *)gbd,gb_created);
    return (GBDATA *)gbd;
}



GB_ERROR GB_delete(GBDATA *source)
{
    GBDATA *gb_main;

    GB_TEST_TRANSACTION(source);
    if (GB_GET_SECURITY_DELETE(source)>GB_MAIN(source)->security_level) {
        return GB_export_error("Security error in GB_delete: %s",GB_read_key_pntr(source));
    }

    gb_main = GB_get_root(source);

    if (source->flags.compressed_data) {
        GB_set_compression(gb_main, 0); /* disable compression */
        gb_set_compression(source); /* write data w/o compression (otherwise GB_read_old_value... won't work) */
        GB_set_compression(gb_main, -1); /* allow all types of compressions */
    }

    if (GB_MAIN(source)->transaction<0){
        gb_delete_entry(source);
        gb_do_callback_list(gb_main);
    }else{
        gb_touch_entry(source,gb_deleted);
    }
    return 0;
}

GB_ERROR gb_delete_force(GBDATA *source)    /* delete always */
{
    gb_touch_entry(source,gb_deleted);
    return 0;
}


/********************************************************************************************
                    Copy Data
********************************************************************************************/
GB_ERROR GB_copy(GBDATA *dest, GBDATA *source)
{
    long type;
    GB_ERROR error = 0;
    GBDATA *gb_p;
    GBDATA *gb_d;
    GBCONTAINER *destc,*sourcec;
    const char *key;

    GB_TEST_TRANSACTION(source);
    type = GB_TYPE(source);
    if (GB_TYPE(dest) != type)
    {
        return GB_export_error("incompatible types in GB_copy (source %s:%li != %s:%i",
                               GB_read_key_pntr(source),type, GB_read_key_pntr(dest), GB_TYPE(dest));
    }

    switch (type)
    {
        case GB_INT:
            error = GB_write_int(dest,GB_read_int(source));
            break;
        case GB_FLOAT:
            error = GB_write_float(dest,GB_read_float(source));
            break;
        case GB_BYTE:
            error = GB_write_byte(dest,GB_read_byte(source));
            break;
        case GB_STRING:     /* No local compression */
            error = GB_write_string(dest,GB_read_char_pntr(source));
            break;
        case GB_LINK:       /* No local compression */
            error = GB_write_link(dest,GB_read_link_pntr(source));
            break;
        case GB_BITS:       /* only local compressions for the following types */
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
                GB_ERROR err = GB_export_error("GB_COPY Type conflict %s:%i != %s:%i",
                                               GB_read_key_pntr(dest), GB_TYPE(dest), GB_read_key_pntr(source), GB_DB);
                GB_internal_error("%s",err);
                return err;
            }

            if (source->flags2.folded_container)    gb_unfold((GBCONTAINER *)source,-1,-1);
            if (dest->flags2.folded_container)  gb_unfold((GBCONTAINER *)dest,0,-1);

            for (gb_p = GB_find(source,0,0,down_level);
                 gb_p;
                 gb_p = GB_find(gb_p,0,0,this_level|search_next))
            {
                GB_TYPES type2 = (GB_TYPES)GB_TYPE(gb_p);

                key = GB_read_key_pntr(gb_p);
                if (type2 == GB_DB)
                {
                    gb_d = GB_create_container(dest,key);
                    gb_create_header_array((GBCONTAINER *)gb_d, ((GBCONTAINER *)gb_p)->d.size);
                }
                else
                {
                    gb_d = GB_create(dest,key,type2);
                }

                if (!gb_d) return GB_get_error();
                error = GB_copy(gb_d, gb_p);
                if (error) break;
            }

            destc->flags3 = sourcec->flags3;
            break;

        default:
            error = GB_export_error("GB_copy error unknown type");
    }
    if (error) return error;

    gb_touch_entry(dest,gb_changed);
    dest->flags.security_read   = source->flags.security_read;
    return 0;
}

/********************************************************************************************
                                        Get all subfield names
********************************************************************************************/

static char *stpcpy(char *dest, const char *source)
{
    while ((*dest++=*source++)) ;
    return dest-1; /* return pointer to last copied character (which is \0) */
}

char* GB_get_subfields(GBDATA *gbd)
{
    long type;
    char *result = 0;

    GB_TEST_TRANSACTION(gbd);
    type = GB_TYPE(gbd);

    if (type==GB_DB) { /* we are a container */
        GBCONTAINER *gbc = (GBCONTAINER*)gbd;
        GBDATA *gbp;
        int result_length = 0;

        if (gbc->flags2.folded_container) {
            gb_unfold(gbc, -1, -1);
        }

        for (gbp = GB_find(gbd, 0, 0, down_level);
             gbp;
             gbp = GB_find(gbp, 0, 0, this_level|search_next))
        {
            const char *key = GB_read_key_pntr(gbp);
            int keylen = strlen(key);

            if (result) {
                char *neu_result = (char*)malloc(result_length+keylen+1+1);

                if (neu_result) {
                    char *p = stpcpy(neu_result, result);
                    p = stpcpy(p, key);
                    *p++ = ';';
                    p[0] = 0;

                    free(result);
                    result = neu_result;
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
        result = GB_strdup(";");
    }

    return result;
}

/********************************************************************************************
                    Copy Data
********************************************************************************************/
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
            GB_write_string(source,"");
            GB_write_string(source,string);
            free(string);
            break;
        case GB_BITS:
        case GB_BYTES:
        case GB_INTS:
        case GB_FLOATS:
            break;
        case GB_DB:
            for (   gb_p = GB_find(source,0,0,down_level);
                    gb_p;
                    gb_p = GB_find(gb_p,0,0,this_level|search_next)){
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

GB_ERROR GB_set_compression(GBDATA *gb_main, GB_COMPRESSION_MASK disable_compression){
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    GB_ERROR error = 0;
    if (Main->compression_mask == disable_compression) return 0;
    Main->compression_mask = disable_compression;
#if 0
    GB_push_my_security(gb_main);
    error = gb_set_compression(gb_main);
    GB_pop_my_security(gb_main);
#endif
    return error;
}



/********************************************************************************************
                    TEMPORARY
********************************************************************************************/


/** if the temporary flag is set, than that entry (including all subentries) will not be saved*/
GB_ERROR GB_set_temporary(GBDATA *gbd)
{
    GB_TEST_TRANSACTION(gbd);
    if (GB_GET_SECURITY_DELETE(gbd)>GB_MAIN(gbd)->security_level)
        return GB_export_error("Security error in GB_set_temporary: %s",GB_read_key_pntr(gbd));
    gbd->flags.temporary = 1;
    gb_touch_entry(gbd,gb_changed);
    return 0;
}

/** enable save */
GB_ERROR GB_clear_temporary(GBDATA *gbd)
{
    GB_TEST_TRANSACTION(gbd);
    gbd->flags.temporary = 0;
    gb_touch_entry(gbd,gb_changed);
    return 0;
}

long    GB_read_temporary(GBDATA *gbd)
{
    GB_TEST_TRANSACTION(gbd);
    return (long)gbd->flags.temporary;
}

/********************************************************************************************
                    TRANSACTIONS
********************************************************************************************/

GB_ERROR GB_push_local_transaction(GBDATA *gbd){ /* Starts a read only transaction !!;
                                                    be shure that all data is cached
                                                    be extremely carefull !!!!! */
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->transaction>0) {
        return GB_push_transaction(gbd);
    }
    Main->transaction --;
    return 0;
}

GB_ERROR GB_pop_local_transaction(GBDATA *gbd){ /* Stops a read only transaction !!;
                                                   be shure that all data is cached !!!!! */
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->transaction>0){
        return GB_pop_transaction(gbd);
    }
    Main->transaction ++;
    return 0;
}


GB_ERROR GB_push_transaction(GBDATA *gbd){
    /* start a transaction if no transaction is running */
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->transaction<0) return 0;  /* no transaction mode */
    if (Main->transaction){
        Main->transaction++;
    }else{
        return GB_begin_transaction(gbd);
    }
    return 0;
}

GB_ERROR GB_pop_transaction(GBDATA *gbd)
{
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->transaction==0) {
        error = GB_export_error("Pop without push");
        GB_internal_error("%s",error);
        return  error;
    }
    if (Main->transaction<0) return 0;  /* no transaction mode */
    if (Main->transaction==1){
        return GB_commit_transaction(gbd);
    }else{
        Main->transaction--;
    }
    return 0;
}

GB_ERROR GB_begin_transaction(GBDATA *gbd)
{
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    gbd = (GBDATA *)Main->data;
    if (Main->transaction>0) {
        error = GB_export_error("GB_begin_transaction called %i !!!",
                                Main->transaction);
        GB_internal_error("%s",error);
        return GB_push_transaction(gbd);
    }
    if (Main->transaction<0) return 0;
    Main->transaction = 1;
    Main->aborted_transaction = 0;
    if (!Main->local_mode){
        error = gbcmc_begin_transaction(gbd);
        if (error) return error;
        error = gb_commit_transaction_local_rek(gbd,0,0);   /* init structures */
        gb_untouch_children((GBCONTAINER *)gbd);
        gb_untouch_me(gbd);
        if (error) return error;
    }
    gb_do_callback_list(gbd);       /* do all callbacks
                                       cb that change the db are no problem
                                       'cause it's the beginning of a ta */
    Main->clock ++;
    return 0;
}

GB_ERROR gb_init_transaction(GBCONTAINER *gbd)      /* the first transaction ever */
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_ERROR error;
    Main->transaction = 1;
    error = gbcmc_init_transaction(Main->data);
    if (error) return error;
    Main->clock ++;
    return 0;
}

GB_ERROR GB_no_transaction(GBDATA *gbd)
{
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (!Main->local_mode) {
        error = GB_export_error("Tried to disable transactions in a client");
        GB_internal_error("%s",error);
        return 0;
    }
    Main->transaction = -1;
    return 0;
}

GB_ERROR GB_abort_transaction(GBDATA *gbd)
{
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

    gb_abort_transaction_local_rek(gbd,0);
    if (!Main->local_mode){
        error = gbcmc_abort_transaction(gbd);
        if (error) return error;
    }
    Main->clock--;
    gb_do_callback_list(gbd);       /* do all callbacks */
    Main->transaction = 0;
    gb_untouch_children((GBCONTAINER *)gbd);
    gb_untouch_me(gbd);
    return 0;
}


GB_ERROR GB_commit_transaction(GBDATA *gbd)
{
    GB_ERROR error =0;
    GB_MAIN_TYPE    *Main = GB_MAIN(gbd);
    GB_CHANGED  flag;
    gbd = (GBDATA *)Main->data;
    if (!Main->transaction) {
        error = GB_export_error("GB_commit_transaction: No running Transaction");
        GB_internal_error("%s",error);
        return error;
    }
    if (Main->transaction>1){
        GB_internal_error("Running GB_commit_transaction not at root transaction level");
        return GB_pop_transaction(gbd);
    }
    if (Main->aborted_transaction) {
        Main->aborted_transaction = 0;
        return      GB_abort_transaction(gbd);
    }
    if (Main->local_mode) {
        char *error1 = gb_set_undo_sync(gbd);
        while(1){
            flag = (GB_CHANGED)GB_ARRAY_FLAGS(gbd).changed;
            if (!flag) break;           /* nothing to do */
            error = gb_commit_transaction_local_rek(gbd,0,0);
            gb_untouch_children((GBCONTAINER *)gbd);
            gb_untouch_me(gbd);
            if (error) break;
            gb_do_callback_list(gbd);       /* do all callbacks */
        }
        gb_disable_undo(gbd);
        if(error1){
            Main->transaction = 0;
            return error;
        }
    }else{
        gb_disable_undo(gbd);
        while(1){
            flag = (GB_CHANGED)GB_ARRAY_FLAGS(gbd).changed;
            if (!flag) break;           /* nothing to do */

            error = gbcmc_begin_sendupdate(gbd);        if (error) break;
            error = gb_commit_transaction_local_rek(gbd,1,0);   if (error) break;
            error = gbcmc_end_sendupdate(gbd);      if (error) break;

            gb_untouch_children((GBCONTAINER *)gbd);
            gb_untouch_me(gbd);
            gb_do_callback_list(gbd);       /* do all callbacks */
        }
        if (!error)         error = gbcmc_commit_transaction(gbd);

    }
    Main->transaction = 0;
    if (error) return error;
    return 0;
}
/********************************************************************************************
            Send updated data to server (for GB_release)
********************************************************************************************/
GB_ERROR GB_update_server(GBDATA *gbd)
{
    GB_ERROR error;
    GB_MAIN_TYPE    *Main = GB_MAIN(gbd);
    GBDATA *gb_main = (GBDATA *)Main->data;
    struct gb_callback_list *cbl_old = Main->cbl_last;
    if (!Main->transaction) {
        error = GB_export_error("GB_update_server: No running Transaction");
        GB_internal_error("%s",error);
        return error;
    }
    if (Main->local_mode){
        return GB_export_error("You cannot update the server as you are the server yourself");
    }

    error = gbcmc_begin_sendupdate(gb_main);
    if (error) return error;
    error = gb_commit_transaction_local_rek(gbd,2,0);
    if (error) return error;
    error = gbcmc_end_sendupdate(gb_main);
    if (error) return error;
    if (cbl_old != Main->cbl_last){
        GB_internal_error("GB_update_server produced a callback, this is not allowed");
    }
    /* gb_do_callback_list(gbd);         do all callbacks  */
    return 0;
}
/********************************************************************************************
                    CALLBACKS
********************************************************************************************/

GB_ERROR gb_add_changed_callback_list(GBDATA *gbd,struct gb_transaction_save *old, GB_CB_TYPE gbtype, GB_CB func, int *clientdata)
{
    struct gb_callback_list *cbl;
    GB_MAIN_TYPE    *Main = GB_MAIN(gbd);
    cbl = (struct gb_callback_list *)gbm_get_mem(sizeof(struct gb_callback_list),GBM_CB_INDEX);
    if (Main->cbl){
        Main->cbl_last->next = cbl;
    }else{
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

GB_ERROR gb_add_delete_callback_list(GBDATA *gbd,struct gb_transaction_save *old, GB_CB func, int *clientdata)
{
    struct gb_callback_list *cbl;
    GB_MAIN_TYPE    *Main = GB_MAIN(gbd);
    cbl = (struct gb_callback_list *)gbm_get_mem(sizeof(struct gb_callback_list),GBM_CB_INDEX);
    if (Main->cbld){
        Main->cbld_last->next = cbl;
    }else{
        Main->cbld = cbl;
    }
    Main->cbld_last = cbl;
    cbl->clientdata = clientdata;
    cbl->func = func;
    cbl->gbd = gbd;
    if (old) gb_add_ref_gb_transaction_save(old);
    cbl->old = old;
    return 0;
}

struct gb_callback_list *g_b_old_callback_list = 0;
GB_MAIN_TYPE        *g_b_old_main = 0;

GB_ERROR gb_do_callback_list(GBDATA *gbd)
{
    struct gb_callback_list *cbl,*cbl_next; /* first all delete callbacks */
    GB_MAIN_TYPE    *Main = GB_MAIN(gbd);
    g_b_old_main    = Main;

    for (cbl = Main->cbld; cbl ; cbl = cbl_next){
        g_b_old_callback_list = cbl;
        cbl->func(cbl->gbd,cbl->clientdata, GB_CB_DELETE);
        cbl_next = cbl->next;
        g_b_old_callback_list = 0;
        gb_del_ref_gb_transaction_save(cbl->old);
        gbm_free_mem((char *)cbl,sizeof(struct gb_callback_list),GBM_CB_INDEX);
    }
    Main->cbld_last = 0;
    Main->cbld = 0;
    /* then all updates */
    for (cbl = Main->cbl; cbl ; cbl = cbl_next){
        g_b_old_callback_list = cbl;
        cbl->func(cbl->gbd,cbl->clientdata, cbl->type);
        cbl_next = cbl->next;
        g_b_old_callback_list = 0;
        gb_del_ref_gb_transaction_save(cbl->old);
        gbm_free_mem((char *)cbl,sizeof(struct gb_callback_list),GBM_CB_INDEX);
    }
    Main->cbl_last = 0;
    Main->cbl = 0;
    return 0;
}

GB_MAIN_TYPE *gb_get_main_during_cb(){
    return g_b_old_main;
}

GB_CPNTR gb_read_pntr_ts(GBDATA *gbd, struct gb_transaction_save *ts){
    char *data;
    long size;
    int type;
    type = GB_TYPE_TS(ts);
    data = GB_GETDATA_TS(ts);
    if (!data) return 0;
    if (ts->flags.compressed_data) {    /* uncompressed data return pntr to
                                           database entry   */
        size = GB_GETSIZE_TS(ts) * gb_convert_type_2_sizeof[type] + gb_convert_type_2_appendix_size[type];
        data = gb_uncompress_data(gbd,data,size);
    }
    return data;
}

/* get last array value in callbacks */
void *GB_read_old_value(){
    char *data;

    if (!g_b_old_callback_list) {
        GB_export_error("You cannot call GB_read_old_value outside a ARBDB callback");
        return 0;
    }
    if (!g_b_old_callback_list->old) {
        GB_export_error("No old value available in GB_read_old_value");
        return 0;
    }
    data = GB_GETDATA_TS(g_b_old_callback_list->old);
    if (!data) return 0;

    return gb_read_pntr_ts(g_b_old_callback_list->gbd, g_b_old_callback_list->old);
}
/* same for size */
long GB_read_old_size(){
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

/********************************************************************************************
                    CALLBACKS
********************************************************************************************/

char *GB_get_callback_info(GBDATA *gbd) {
    /* returns human-readable information about callbacks of 'gbd' or 0 */
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

GB_ERROR GB_add_priority_callback(GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata, int priority) {
    /* smaller priority values get executed before bigger priority values */

    struct gb_callback *cb;

    GB_TEST_TRANSACTION(gbd);
    GB_CREATE_EXT(gbd);
    cb = (struct gb_callback *)gbm_get_mem(sizeof(struct gb_callback),GB_GBM_INDEX(gbd));

    if (gbd->ext->callback) {
        struct gb_callback *prev = 0;
        struct gb_callback *curr = gbd->ext->callback;

        while (curr) {
            if (priority <= curr->priority) {
                // wanted priority is lower -> insert here
                break;
            }
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

    /* cb->next = gbd->ext->callback; */
    /* gbd->ext->callback = cb; */

    cb->type       = type;
    cb->clientdata = clientdata;
    cb->func       = func;
    cb->priority   = priority;

    return 0;
}

GB_ERROR GB_add_callback(GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata)
{
    return GB_add_priority_callback(gbd, type, func, clientdata, 5); // use default priority 5
}

GB_ERROR GB_remove_callback(GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata)
{
    struct gb_callback *cb,*cblast;
    /*GB_TEST_TRANSACTION(gbd);*/
    cblast = 0;
    GB_CREATE_EXT(gbd);
    for (cb = gbd->ext->callback; cb; cb=cb->next){
        if ((cb->func == func) &&
            ( (clientdata==0) || (clientdata==cb->clientdata)) &&
            (cb->type == type )) break;
        cblast = cb;
    }
    if (cb) {
        if (cblast) {
            cblast->next = cb->next;
        }else{
            gbd->ext->callback = cb->next;
        }
        gbm_free_mem((char *)cb,sizeof(struct gb_callback),GB_GBM_INDEX(gbd));
    }else{
        return "Callback not found";
    }
    return 0;
}

GB_ERROR GB_ensure_callback(GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata) {
    /*GB_TEST_TRANSACTION(gbd);*/
    struct gb_callback *cb;
    GB_CREATE_EXT(gbd);
    for (cb = gbd->ext->callback;cb;cb=cb->next){
        if ((cb->func == func) &&
            ((clientdata==0) || (clientdata==cb->clientdata)) &&
            (cb->type == type ))
        {
            return NULL;        /* already in cb list */
        }
    }
    return GB_add_callback(gbd,type,func,clientdata);
}
/********************************************************************************************
                    RELEASE
    free cached data in a client, no pointers in the freed region are allowed
********************************************************************************************/
GB_ERROR GB_release(GBDATA *gbd){
    GBCONTAINER *gbc;
    GBDATA *gb;
    int index;
    GB_TEST_TRANSACTION(gbd);
    if (GB_MAIN(gbd)->local_mode) return 0;
    if (GB_ARRAY_FLAGS(gbd).changed &&!gbd->flags2.update_in_server) {
        GB_update_server(gbd);
    }
    if (GB_TYPE(gbd) != GB_DB) {
        GB_ERROR error=GB_export_error("You cannot release non container (%s)",
                                       GB_read_key_pntr(gbd));
        GB_internal_error("%s",error);
        return error;
    }
    if (gbd->flags2.folded_container) return 0;
    gbc = (GBCONTAINER *)gbd;

    for (index = 0; index < gbc->d.nheader; index++) {
        if ( (gb = GBCONTAINER_ELEM(gbc,index)) ) {
            gb_delete_entry(gb);
        }
    }

    gbc->flags2.folded_container = 1;
    gb_do_callback_list(gbd);       /* do all callbacks */
    return 0;
}
/********************************************************************************************
                    test local
    test whether data is available in local data
    !!! important for callbacks, because only testlocal tested data is available
********************************************************************************************/

int GB_testlocal(GBDATA *gbd)
{
    if (GB_TYPE(gbd) != GB_DB) {
        return 1;           /* all non containers are available */
    }
    if (GB_MAIN(gbd)->local_mode) return 1;
    if (gbd->flags2.folded_container) return 0;
    return 1;
}
/********************************************************************************************
                    some information about sons
********************************************************************************************/

int GB_nsons(GBDATA *gbd) {
    if (GB_TYPE(gbd) != GB_DB) {
        return 0;           /* all non containers are available */
    }
    return ((GBCONTAINER *)gbd)->d.size;
}

GB_ERROR GB_disable_quicksave(GBDATA *gbd,const char *reason){
    GB_MAIN_TYPE * Main = GB_MAIN(gbd);
    if (Main->qs.quick_save_disabled) free(Main->qs.quick_save_disabled);
    Main->qs.quick_save_disabled = strdup(reason);
    return 0;
}
/********************************************************************************************
                    Resort data base
********************************************************************************************/

GB_ERROR GB_resort_data_base(GBDATA *gb_main, GBDATA **new_order_list, long listsize)
{
    long new_index;
    GBCONTAINER *father;
    struct gb_header_list_struct *hl, h;

    if (GB_read_clients(gb_main)<0)
        return GB_export_error("Sorry: this program is not the arbdb server, you cannot resort your data");

    if (GB_read_clients(gb_main)>0)
        return GB_export_error("There are %li clients (editors, tree programms) connected to this server,\n"
                               "please close clients and rerun operation",
                               GB_read_clients(gb_main));

    if (listsize <=0) return 0;

    father = GB_FATHER(new_order_list[0]);
    GB_disable_quicksave(gb_main,"some entries in the database got a new order");
    hl = GB_DATA_LIST_HEADER(father->d);

    for (new_index= 0 ; new_index< listsize; new_index++ )
    {
        long old_index = new_order_list[new_index]->index;

        if (old_index < new_index)
            GB_warning("Warning at resort database: entry exists twice: %li and %li",
                       old_index, new_index);
        else
        {
            GBDATA *ngb;
            GBDATA *ogb;
            ogb = GB_HEADER_LIST_GBD(hl[old_index]);
            ngb = GB_HEADER_LIST_GBD(hl[new_index]);

            h = hl[new_index];
            hl[new_index] = hl[old_index];
            hl[old_index] = h;              /* Warning: Relative Pointers are incorrect !!! */

            SET_GB_HEADER_LIST_GBD(hl[old_index], ngb );
            SET_GB_HEADER_LIST_GBD(hl[new_index], ogb );

            if ( ngb )  ngb->index = old_index;
            if ( ogb )  ogb->index = new_index;
        }
    }

    gb_touch_entry((GBDATA *)father,gb_changed);
    return 0;
}

GB_ERROR GB_resort_system_folder_to_top(GBDATA *gb_main){
    GBDATA *gb_system = GB_find(gb_main,GB_SYSTEM_FOLDER,0,down_level);
    GBDATA *gb_first = GB_find(gb_main,0,0,down_level);
    GBDATA **new_order_list;
    GB_ERROR error = 0;
    int i,len;
    if (GB_read_clients(gb_main)<0) return 0; /* we are not server */
    if (!gb_system){
        return GB_export_error("System databaseentry does not exist");
    }
    if (gb_first == gb_system) return 0;
    len = GB_number_of_subentries(gb_main);
    new_order_list = (GBDATA **)GB_calloc(sizeof(GBDATA *),len);
    new_order_list[0] = gb_system;
    for (i=1;i<len;i++){
        new_order_list[i] = gb_first;
        do {
            gb_first = GB_find(gb_first,0,0,this_level | search_next);
        } while(gb_first == gb_system);
    }
    error = GB_resort_data_base(gb_main,new_order_list,len);
    GB_FREE(new_order_list);
    return error;
}

/********************************************************************************************
                    USER FLAGS
********************************************************************************************/
GB_ERROR GB_write_usr_public(GBDATA *gbd, long flags)
{
    GB_TEST_TRANSACTION(gbd);
    if (GB_GET_SECURITY_WRITE(gbd) > GB_MAIN(gbd)->security_level)
        return gb_security_error(gbd);
    gbd->flags.user_flags = flags;
    gb_touch_entry(gbd,gb_changed);
    return 0;
}

long GB_read_usr_public(GBDATA *gbd)
{
    GB_TEST_TRANSACTION(gbd);
    return (long)gbd->flags.user_flags;
}

/********************************************************************************************
    private user access
********************************************************************************************/

long GB_read_usr_private(GBDATA *gbd) {
    register GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    if (GB_TYPE(gbc) != GB_DB) {
        GB_ERROR error =
            GB_export_error("GB_write_usr_private: not a container (%s)",GB_read_key_pntr(gbd));
        GB_internal_error("%s",error);
        return 0;
    }
    return gbc->flags2.usr_ref;
}

GB_ERROR GB_write_usr_private(GBDATA *gbd,long ref) {
    register GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    if (GB_TYPE(gbc) != GB_DB) {
        GB_ERROR error =
            GB_export_error("GB_write_usr_private: not a container (%s)",GB_read_key_pntr(gbd));
        GB_internal_error("%s",error);
        return 0;
    }
    gbc->flags2.usr_ref = ref;
    return 0;
}

/********************************************************************************************
            flag access
********************************************************************************************/

GB_ERROR GB_write_flag(GBDATA *gbd,long flag)
{
    register GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    int prev;
    int ubit = GB_MAIN(gbd)->users[0]->userbit;
    GB_TEST_TRANSACTION(gbd);

    prev = GB_ARRAY_FLAGS(gbc).flags;
    gbd->flags.saved_flags = prev;

    if (flag){
        GB_ARRAY_FLAGS(gbc).flags |= ubit;
    }else{
        GB_ARRAY_FLAGS(gbc).flags &= ~ubit;
    }
    if (prev != (int)GB_ARRAY_FLAGS(gbc).flags) {
        gb_touch_entry(gbd,gb_changed);
        gb_touch_header(GB_FATHER(gbd));
        GB_DO_CALLBACKS(gbd);
    }
    return 0;
}

int GB_read_flag(GBDATA *gbd)
{
    GB_TEST_TRANSACTION(gbd);
    if (GB_ARRAY_FLAGS(gbd).flags & GB_MAIN(gbd)->users[0]->userbit) return 1;
    else return 0;
}

/********************************************************************************************
            touch entry
********************************************************************************************/


GB_ERROR GB_touch(GBDATA *gbd) {
    GB_TEST_TRANSACTION(gbd);
    gb_touch_entry(gbd,gb_changed);
    GB_DO_CALLBACKS(gbd);
    return 0;
}


/********************************************************************************************
            debug data
********************************************************************************************/

void dump(const char *data, int size)
{
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

GB_ERROR GB_print_debug_information(void *dummy, GBDATA *gb_main){
    int i;
    GB_MAIN_TYPE *Main = GB_MAIN( gb_main );
    GB_push_transaction(gb_main);
    dummy = dummy;
    for (i=0;i<Main->keycnt;i++) {
        if (Main->keys[i].key) {
            printf("%3i %20s    nref %i\n", i, Main->keys[i].key, (int)Main->keys[i].nref);
        }else{
            printf("    %3i unused key, next free key = %li\n", i, Main->keys[i].next_free_key);
        }
    }
    gbm_debug_mem(Main);
    GB_pop_transaction(gb_main);
    return 0;
}

int GB_info_deep = 15;


int gb_info(GBDATA *gbd, int deep){
    GBCONTAINER *gbc;
    GB_TYPES type;
    char    *data;
    int     size ;
    GB_MAIN_TYPE *Main;

    if (gbd==NULL) { printf("NULL\n"); return -1; }
    GB_push_transaction(gbd);
    type = (GB_TYPES)GB_TYPE(gbd);

    if (deep) {
        printf("    ");
    }

    printf("(GBDATA*)0x%lx (GBCONTAINER*)0x%lx ",(long)gbd,(long)gbd);

    if (gbd->rel_father==0) { printf("father=NULL\n"); return -1; }

    if (type==GB_DB)    {gbc = (GBCONTAINER*) gbd; Main = GBCONTAINER_MAIN(gbc);}
    else        {gbc = NULL; Main = GB_MAIN(gbd);}

    if (!Main)                  { printf("Oops - I have no main entry!!!\n"); return -1;}
    if (gbd==(GBDATA*)(Main->dummy_father))     { printf("dummy_father!\n"); return -1; }

    printf("%10s Type '%c'  ", GB_read_key_pntr(gbd), GB_TYPE_2_CHAR[type]);

    switch(type)
    {
        case GB_DB:
            gbc = (GBCONTAINER *)gbd;
            size = gbc->d.size;
            printf("Size %i nheader %i hmemsize %i", gbc->d.size, gbc->d.nheader, gbc->d.headermemsize);
            printf(" father=(GBDATA*)0x%lx\n", (long)GB_FATHER(gbd));
            if (size < GB_info_deep){
                int index;
                struct gb_header_list_struct *header;

                header = GB_DATA_LIST_HEADER(gbc->d);
                for (index = 0; index < gbc->d.nheader; index++) {
                    GBDATA *gb_sub = GB_HEADER_LIST_GBD(header[index]);
                    printf("\t\t%10s (GBDATA*)0x%lx (GBCONTAINER*)0x%lx\n",Main->keys[header[index].flags.key_quark].key,(long)gb_sub,(long)gb_sub);
                }
            }
            break;
        default:
            data = GB_read_as_string(gbd);
            if (data) {printf("%s",data); free(data);}
            printf(" father=(GBDATA*)0x%lx\n", (long)GB_FATHER(gbd));
    }


    GB_pop_transaction(gbd);

    return 0;
}


int GB_info(GBDATA *gbd)
{
    return gb_info(gbd,0);
}

long GB_number_of_subentries(GBDATA *gbd)
{
    GBCONTAINER *gbc;
    GB_TYPES type = (GB_TYPES)GB_TYPE(gbd);

    switch(type)
    {
        case GB_DB:         /* @@@ client size < actual size!!! => use GB_rescan_number_of_subentries() from client */
            gbc = (GBCONTAINER *)gbd;
            return gbc->d.size;
        default:
            return -1;
    }
}

long GB_rescan_number_of_subentries(GBDATA *gbd) {
    /* this is just a workaround for the above function, cause GB_number_of_subentries does not work in clients; it's used in ARB_EDIT4 */

    GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    /*    int userbit = GBCONTAINER_MAIN(gbc)->users[0]->userbit; */
    int index;
    int end = gbc->d.nheader;
    struct gb_header_list_struct *header;
    long count = 0;

    header = GB_DATA_LIST_HEADER(gbc->d);
    for (index = 0; index<end; index++) {
        if ((int)header[index].flags.changed >= gb_deleted) continue;
        count++;
    }
    return count;
}




