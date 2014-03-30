// =============================================================== //
//                                                                 //
//   File      : ps_filebuffer.hxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in October 2002                     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PS_FILEBUFFER_HXX
#define PS_FILEBUFFER_HXX

#ifndef PS_ASSERT_HXX
#include "ps_assert.hxx"
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

class PS_FileBuffer : virtual Noncopyable {
    // filehandling
    int   file_handle;
    char *file_name;
    int   file_flags;
    int   file_mode;
    bool  is_readonly;
    long  file_pos;
    // bufferhandling
    unsigned char *buffer;
    int            size;                // how much is in buffer
    int            position;            // where am i in the buffer
    // debug
    unsigned long int total_read;
    unsigned long int total_write;

    PS_FileBuffer();

    void refill();                      // refill buffer from file

public:
    static const bool READONLY    = true;
    static const bool WRITEONLY   = false;
    static const int  BUFFER_SIZE = 4096;

    //
    // IO-functions
    //

    // raw
    void put (const void *_data, int _length);
    void get (void *_data, int _length);
    void peek(void *_data, int _length);         // read but don't advance position

    // char
    void put_char(unsigned char  _c) {
        if (is_readonly) {
            fprintf(stderr, "sorry, i can't write to files opened readonly\n");
            CRASH();
        }
        if (size+1 < BUFFER_SIZE) {
            buffer[size] = _c;
            ++size;
        }
        else {
            flush();
            buffer[0] = _c;
            size      = 1;
        }
    }
    void get_char(unsigned char &_c) {
        if (position < size) {
            _c = buffer[position];
            ++position;
        }
        else {
            refill();
            _c       = buffer[0];
            position = 1;
        }
    }
    unsigned char get_char() {
        if (position < size) {
            return buffer[position++];
        }
        else {
            refill();
            position = 1;
            return buffer[0];
        }
    }

    // long
    void put_ulong(unsigned long int  _ul);
    void get_ulong(unsigned long int &_ul);
    void put_long(long int  _l) {
        unsigned long int ul;
        if (_l < 0) {
            ul = (-_l << 1) | 1;
        }
        else {
            ul = _l << 1;
        }
        put_ulong(ul);
    }
    void get_long(long int &_l) {
        unsigned long int ul;
        get_ulong(ul);
        if (ul & 1) {
            _l = -(long)(ul >> 1);
        }
        else {
            _l = ul >> 1;
        }
    }

    // int
    void put_uint(unsigned int  _ui) {
        put_ulong(_ui);
    }
    void get_uint(unsigned int &_ui) {
        unsigned long int ul;
        get_ulong(ul);
        _ui=(unsigned int)ul;
    }
    void put_int(int  _i) {
        put_long(_i);
    }
    void get_int(int &_i) {
        long int l;
        get_long(l);
        _i=(int)l;
    }


    //
    // utility-functions
    //
    bool store_pos() {
        if (!is_readonly) return false; // cannot jump in a writeable file
        file_pos = lseek(file_handle, 0, SEEK_CUR);
        return (file_pos >= 0);
    }

    bool restore_pos() {
        if (!is_readonly) return false; // cannot jump in a writeable file
        if (file_pos < 0) return false;
        if (lseek(file_handle, file_pos, SEEK_SET) < 0) return false;
        size     = 0;
        position = 0;
        refill();
        return true;
    }

    bool empty() {
        if (size == 0) return true;
        return (position < size);
    }

    void clear() {                       // 'clear' buffer
        size     = 0;
        position = 0;
    }

    bool isReadonly() {
        return is_readonly;
    }

    void flush();                       // write buffer to file

    //
    // initialization-functions
    //
    void reinit(const char *name, bool _readonly);   // reinit. with new file

    PS_FileBuffer(const char *_name, bool _readonly);

    ~PS_FileBuffer() {
        // finish file
        if (!is_readonly) flush();
        if (file_name) free(file_name);
        if (file_handle != -1) close(file_handle);
        // finish buffer
        if (buffer) free(buffer);
    }

};

#else
#error ps_filebuffer.hxx included twice
#endif // PS_FILEBUFFER_HXX
