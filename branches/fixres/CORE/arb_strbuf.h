// ============================================================= //
//                                                               //
//   File      : arb_strbuf.h                                    //
//   Purpose   : "unlimited" output buffer                       //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef ARB_STRBUF_H
#define ARB_STRBUF_H

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARB_MEM_H
#include "arb_mem.h"
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif


// -----------------------
//      String streams

class GBS_strstruct : virtual Noncopyable {
    char   *data;
    size_t  buffer_size;
    size_t  pos;

    void set_pos(size_t toPos) {
        pos = toPos;
        if (data) data[pos] = 0;
    }
    void inc_pos(size_t inc) { set_pos(pos+inc); }

public:

    GBS_strstruct()
        : data(NULL)
        , buffer_size(0)
        , pos(0)
    {}
    GBS_strstruct(size_t buffersize)
        : data(NULL)
        , buffer_size(0)
        , pos(0)
    {
        alloc_mem(buffersize);
    }
    ~GBS_strstruct() { free(data); }

    size_t get_buffer_size() const { return buffer_size; }
    size_t get_position() const { return pos; }

    bool filled() const { return get_position()>0; }
    bool empty() const { return !filled(); }

    const char *get_data() const { return data; }

    char *release_mem(size_t& size) {
        char *result = data;
        size         = buffer_size;
        buffer_size  = 0;
        data         = 0;
        return result;
    }
    char *release() { size_t s; return release_mem(s); }

    void erase() { set_pos(0); }

    void assign_mem(char *block, size_t blocksize) {
        free(data);

        arb_assert(block && blocksize>0);

        data      = block;
        buffer_size = blocksize;

        erase();
    }
    void reassign_mem(GBS_strstruct& from) {
        size_t  size;
        char   *block = from.release_mem(size);

        assign_mem(block, size);
    }

    void alloc_mem(size_t blocksize) {
        arb_assert(blocksize>0);
        arb_assert(!data);

        assign_mem((char*)ARB_alloc(blocksize), blocksize);
    }
    void realloc_mem(size_t newsize) {
        if (!data) alloc_mem(newsize);
        else {
            // cppcheck-suppress memleakOnRealloc
            ARB_realloc(data, newsize);
            buffer_size = newsize;

            arb_assert(pos<newsize);
        }
    }

    void ensure_mem(size_t needed_size) {
        // ensures insertion of 'needed_size' bytes is ok
        size_t whole_needed_size = pos+needed_size+1;
        if (buffer_size<whole_needed_size) {
            size_t next_size = (whole_needed_size * 3) >> 1;
            realloc_mem(next_size);
        }
    }

    // --------------------

    void cut_tail(size_t byte_count) {
        set_pos(pos<byte_count ? 0 : pos-byte_count);
    }

    void put(char c) {
        ensure_mem(1);
        data[pos] = c;
        inc_pos(1);
    }
    void nput(char c, size_t count) {
        ensure_mem(count);
        memset(data+pos, c, count);
        inc_pos(count);
    }

    void ncat(const char *from, size_t count) {
        if (count) {
            ensure_mem(count);
            memcpy(data+pos, from, count);
            inc_pos(count);
        }
    }
    void cat(const char *from) { ncat(from, strlen(from)); }

    void vnprintf(size_t maxlen, const char *templat, va_list& parg) __ATTR__VFORMAT_MEMBER(2);
    void nprintf(size_t maxlen, const char *templat, ...) __ATTR__FORMAT_MEMBER(2);

    void putlong(long l) { nprintf(100, "%li", l); }
    void putfloat(float f) { nprintf(100, "%f", f); }
};

// old interface

GBS_strstruct *GBS_stropen(long init_size);
char          *GBS_strclose(GBS_strstruct *strstr);
void           GBS_strforget(GBS_strstruct *strstr);
char          *GBS_mempntr(GBS_strstruct *strstr);
long           GBS_memoffset(GBS_strstruct *strstr);
void           GBS_str_cut_tail(GBS_strstruct *strstr, size_t byte_count);
void           GBS_strncat(GBS_strstruct *strstr, const char *ptr, size_t len);
void           GBS_strcat(GBS_strstruct *strstr, const char *ptr);
void           GBS_strnprintf(GBS_strstruct *strstr, long maxlen, const char *templat, ...) __ATTR__FORMAT(3);
void           GBS_chrcat(GBS_strstruct *strstr, char ch);
void           GBS_chrncat(GBS_strstruct *strstr, char ch, size_t n);
void           GBS_intcat(GBS_strstruct *strstr, long val);
void           GBS_floatcat(GBS_strstruct *strstr, double val);

#else
#error arb_strbuf.h included twice
#endif // ARB_STRBUF_H
