#ifndef PS_FILEBUFFER_HXX
#define PS_FILEBUFFER_HXX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

//***********************************************
//* PS_FileBuffer
//***********************************************
#define BUFFER_SIZE 4096
class PS_FileBuffer {
private:
    // filehandling
    int   file_handle;
    char *file_name;
    int   file_flags;
    int   file_mode;
    bool  is_readonly;
    // bufferhandling
    char *buffer;
    int   size;                         // how much is in buffer
    int   position;                     // where am i in the buffer

    PS_FileBuffer();

    void flush();                       // write buffer to file
    void refill();                      // refill buffer from file

public:
    static const bool READONLY  = true;
    static const bool WRITEONLY = false;

    void put ( const void *_data, int _length );
    void get (       void *_data, int _length );
    void peek(       void *_data, int _length ); // read but don't advance position

    bool empty() {
        if (size == 0) return true;
        return (position < size);
    }

    void clear() {                       // 'clear' buffer
        size     = 0;
        position = 0;
    }

    void reinit( const char *name, bool _readonly ); // reinit. with new file

    PS_FileBuffer( const char *_name, bool _readonly );

    ~PS_FileBuffer() {
        if (buffer) free(buffer);
        if (file_name) free(file_name);
        if (file_handle != -1) close(file_handle);
    }

};

#else
#error ps_filebuffer.hxx included twice
#endif
