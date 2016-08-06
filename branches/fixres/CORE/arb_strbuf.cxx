// ============================================================= //
//                                                               //
//   File      : arb_strbuf.cxx                                  //
//   Purpose   : "unlimited" output buffer                       //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "arb_strbuf.h"
#include "arb_string.h"

void GBS_strstruct::vnprintf(size_t maxlen, const char *templat, va_list& parg) {
    ensure_mem(maxlen+1);

    char *buffer = data+pos;
    int   printed;

#ifdef LINUX
    printed = vsnprintf(buffer, maxlen+1, templat, parg);
#else
    printed = vsprintf(buffer, templat, parg);
#endif

    assert_or_exit(printed >= 0 && (size_t)printed <= maxlen);
    inc_pos(printed);
}

void GBS_strstruct::nprintf(size_t maxlen, const char *templat, ...) {
    va_list parg;
    va_start(parg, templat);
    vnprintf(maxlen, templat, parg);
}

// old interface

static GBS_strstruct last_used;

GBS_strstruct *GBS_stropen(long init_size) {
    /*! create a new memory file
     * @param init_size  estimated used size
     */
    
    GBS_strstruct *strstr = new GBS_strstruct;

    arb_assert(init_size>0);

    if (last_used.get_buffer_size() >= (size_t)init_size) {
        strstr->reassign_mem(last_used);

        static short oversized_counter = 0;

        if ((size_t)init_size*10 < strstr->get_buffer_size()) oversized_counter++;
        else oversized_counter = 0;

        if (oversized_counter>10) {                 // was oversized more than 10 times -> allocate smaller block
            size_t dummy;
            free(strstr->release_mem(dummy));
            strstr->alloc_mem(init_size);
        }
    }
    else {
        strstr->alloc_mem(init_size);
    }

    return strstr;
}

char *GBS_strclose(GBS_strstruct *strstr) {
    // returns a char* copy of the memory file
    char *str = GB_strndup(strstr->get_data(), strstr->get_position());
    GBS_strforget(strstr);
    return str;
}

void GBS_strforget(GBS_strstruct *strstr) {
    size_t last_bsize = last_used.get_buffer_size();
    size_t curr_bsize = strstr->get_buffer_size();

    if (last_bsize < curr_bsize) { // last_used is smaller -> keep this
        last_used.reassign_mem(*strstr);
    }
    delete strstr;
}

char *GBS_mempntr(GBS_strstruct *strstr) {
    // returns the memory file (with write access)
    return (char*)strstr->get_data(); 
}

long GBS_memoffset(GBS_strstruct *strstr) {
    // returns the offset into the memory file
    return strstr->get_position();
}

void GBS_str_cut_tail(GBS_strstruct *strstr, size_t byte_count) {
    // Removes byte_count characters at the tail of a memfile
    strstr->cut_tail(byte_count);
}

void GBS_strncat(GBS_strstruct *strstr, const char *ptr, size_t len) {
    /* append some bytes string to strstruct
     * (caution : copies zero byte and mem behind if used with wrong len!)
     */
    strstr->ncat(ptr, len);
}

void GBS_strcat(GBS_strstruct *strstr, const char *ptr) {
    // append string to strstruct
    strstr->cat(ptr);
}

void GBS_strnprintf(GBS_strstruct *strstr, long maxlen, const char *templat, ...) {
    va_list parg;
    va_start(parg, templat);
    strstr->vnprintf(maxlen+2, templat, parg);
}

void GBS_chrcat(GBS_strstruct *strstr, char ch) {
    strstr->put(ch);
}

void GBS_chrncat(GBS_strstruct *strstr, char ch, size_t n) {
    strstr->nput(ch, n);
}

void GBS_intcat(GBS_strstruct *strstr, long val) {
    char buffer[100];
    long len = sprintf(buffer, "%li", val);
    GBS_strncat(strstr, buffer, len);
}

void GBS_floatcat(GBS_strstruct *strstr, double val) {
    char buffer[100];
    long len = sprintf(buffer, "%f", val);
    GBS_strncat(strstr, buffer, len);
}

