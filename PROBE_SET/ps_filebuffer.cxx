
#include "ps_filebuffer.hxx"

using namespace std;

// ************************************************************
// * PS_FileBuffer( _name,_readonly )
// ************************************************************
PS_FileBuffer::PS_FileBuffer( const char *_name, bool _readonly ) {
    // init. file
    file_name   = strdup(_name);
    is_readonly = _readonly;
    if (is_readonly) {
        file_flags = O_RDONLY;
        file_mode  = 0;
    } else {
        file_flags = O_WRONLY | O_CREAT | O_EXCL;
        file_mode  = S_IRUSR | S_IWUSR;
    }
    file_handle = open( file_name, file_flags, file_mode );
    if (file_handle == -1) {
        fprintf( stderr, "failed to open file %s\n",file_name );
        *(int *)0 = 0;
    }

    // init. buffer
    size     = 0;
    position = 0;
    buffer   = (char *)malloc(BUFFER_SIZE);
    if (!buffer) {
        fprintf( stderr, "failed to allocate memory for buffer for file %s\n",file_name );
        *(int *)0 = 0;
    }

    // debug
    total_read  = 0;
    total_write = 0;
}

// ************************************************************
// * reinit( _name,_readonly )
// ************************************************************
void PS_FileBuffer::reinit( const char *_name, bool _readonly ) {
    // finish old file
    if (!is_readonly) flush();
    if (file_name) free(file_name);
    if (file_handle != -1) close(file_handle);

    // init. file
    file_name   = strdup(_name);
    is_readonly = _readonly;
    if (is_readonly) {
        file_flags = O_RDONLY;
        file_mode  = 0;
    } else {
        file_flags = O_WRONLY | O_CREAT | O_EXCL;
        file_mode  = S_IRUSR | S_IWUSR;
    }
    file_handle = open( file_name, file_flags, file_mode );
    if (file_handle == -1) {
        fprintf( stderr, "failed to open file %s\n",file_name );
        *(int *)0 = 0;
    }    

    // init. buffer
    clear();

    // debug
    total_read  = 0;
    total_write = 0;
}

// ************************************************************
// * put( _data,_length )
// ************************************************************
void PS_FileBuffer::put( const void *_data, int _length ) {
    if (_length > BUFFER_SIZE) {
        fprintf( stderr, "sorry, i can't write %i bytes at once, only %i\n",_length,BUFFER_SIZE );
        *(int *)0 = 0;
    }
    if (is_readonly) {
        fprintf( stderr, "sorry, i can't write to files opened readonly\n" );
        *(int *)0 = 0;
    }
    if (size + _length < BUFFER_SIZE) {
        memcpy( &buffer[size], _data, _length );
        size += _length;
    } else {
        flush();
        memcpy( buffer,_data,_length );
        size = _length;
    }
}


// ************************************************************
// * get( _data,_length )
// ************************************************************
void PS_FileBuffer::get( void *_data, int _length ) {
    if (_length > BUFFER_SIZE) {
        fprintf( stderr, "sorry, i can't read %i bytes at once, only %i\n",_length,BUFFER_SIZE );
        *(int *)0 = 0;
    }
    if (position + _length <= size) {
        memcpy( _data, &buffer[position], _length );
        position += _length;
    } else {
        refill();
        memcpy( _data,buffer,_length );
        position = _length;
    }
}


// ************************************************************
// * peek( _data,_length )
// ************************************************************
void PS_FileBuffer::peek( void *_data, int _length ) {
    if (_length > BUFFER_SIZE) {
        fprintf( stderr, "sorry, i can't read %i bytes at once, only %i\n",_length,BUFFER_SIZE );
        *(int *)0 = 0;
    }
    if (position + _length <= size) {
        memcpy( _data, &buffer[position], _length );
    } else {
        refill();
        memcpy( _data,buffer,_length );
    }
}


// ************************************************************
// * flush()
// ************************************************************
void PS_FileBuffer::flush() {
    ssize_t written = write( file_handle, buffer, size );
    if (written != size) {
        fprintf( stderr, "failed to write %i bytes to file %s\n",size,file_name );
        *(int *)0 = 0;
    }
    total_write += written;
    size         = 0;
    position     = 0;
}


// ************************************************************
// * refill()
// ************************************************************
void PS_FileBuffer::refill() {
    // move unread data to start of buffer
    int unread = size-position;
    memcpy( buffer, &buffer[position], unread );
    // read data from file
    ssize_t readen = read( file_handle, &buffer[size-position], BUFFER_SIZE-unread );
    total_read += readen;
    size        = unread+readen;
    position    = 0;
}
