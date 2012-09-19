
#include "ps_filebuffer.hxx"

using namespace std;

void PS_FileBuffer::put(const void *_data, int _length) {
    if (is_readonly) {
        fprintf(stderr, "sorry, i can't write to files opened readonly\n");
        CRASH();
    }
    if (_length > BUFFER_SIZE) {
        fprintf(stderr, "sorry, i can't write %i bytes at once, only %i\n", _length, BUFFER_SIZE);
        CRASH();
    }
    if (_length == 0) return;
    if (size + _length < BUFFER_SIZE) {
        memcpy(&buffer[size], _data, _length);
        size += _length;
    }
    else {
        flush();
        memcpy(buffer, _data, _length);
        size = _length;
    }
}


void PS_FileBuffer::get(void *_data, int _length) {
    if (_length > BUFFER_SIZE) {
        fprintf(stderr, "sorry, i can't read %i bytes at once, only %i\n", _length, BUFFER_SIZE);
        CRASH();
    }
    if (_length == 0) return;
    if (position + _length <= size) {
        memcpy(_data, &buffer[position], _length);
        position += _length;
    }
    else {
        refill();
        memcpy(_data, buffer, _length);
        position = _length;
    }
}


void PS_FileBuffer::put_ulong(unsigned long int _ul) {
    if (_ul <= 0x7F) {                  // bit7 == 0 -> 1 byte integer
        put_char(_ul);
    }
    else if (_ul <= 0x3FFF) {
        put_char((_ul>>8) | 0x80);      // bit7==1 && bit6==0 -> 2 byte integer
        put_char(_ul & 0xFF);
    }
    else if (_ul <= 0x1FFFFF) {
        put_char((_ul>>16) | 0xC0);     // bit7==1 && bit6==1 && bit5==0 -> 3 byte integer
        put_char((_ul>>8) & 0xFF);
        put_char(_ul & 0xFF);
    }
    else if (_ul <= 0x0FFFFFFF) {
        put_char((_ul>>24) | 0xE0);     // bit7==1 && bit6==1 && bit5==1 && bit4==0 -> 4 byte integer
        put_char((_ul>>16) & 0xFF);
        put_char((_ul>>8) & 0xFF);
        put_char(_ul & 0xFF);
    }
    else {                              // else -> 5 byte integer
        put_char(0xF0);
        put_char((_ul>>24) & 0xFF);
        put_char((_ul>>16) & 0xFF);
        put_char((_ul>>8) & 0xFF);
        put_char(_ul & 0xFF);
    }

}


void PS_FileBuffer::get_ulong(unsigned long int &_ul) {
    unsigned char c;
    get_char(c);

    if ((c & 0x80) == 0) {                   // 1-byte
        _ul = c;
    }
    else {
        if ((c & 0xC0) == 0x80) {            // 2-byte
            _ul = (unsigned long)(c & 0x3F) << 8;
        }
        else {
            if ((c & 0xE0) == 0xC0) {        // 3-byte
                _ul = (unsigned long)(c & 0x1F) << 16;
            }
            else {
                if ((c & 0xF0) == 0xE0) {    // 4-byte
                    _ul = (unsigned long)(c & 0x0F) << 24;
                }
                else {
                    get_char(c);
                    _ul = (unsigned long)c << 24;
                }
                get_char(c);
                _ul = (unsigned long)c << 16;
            }
            get_char(c);
            _ul |= (unsigned long)c << 8;
        }
        get_char(c);
        _ul |= c;
    }
}


void PS_FileBuffer::peek(void *_data, int _length) {
    if (_length > BUFFER_SIZE) {
        fprintf(stderr, "sorry, i can't read %i bytes at once, only %i\n", _length, BUFFER_SIZE);
        CRASH();
    }
    if (position + _length <= size) {
        memcpy(_data, &buffer[position], _length);
    }
    else {
        refill();
        memcpy(_data, buffer, _length);
    }
}


void PS_FileBuffer::flush() {
    ssize_t written = write(file_handle, buffer, size);
    if (written != size) {
        fprintf(stderr, "failed to write %i bytes to file %s (total_write = %lu)\n", size, file_name, total_write);
        CRASH();
    }
    total_write += written;
    size         = 0;
    position     = 0;
}


void PS_FileBuffer::refill() {
    // move unread data to start of buffer
    int unread = size-position;
    memcpy(buffer, &buffer[position], unread);
    // read data from file
    ssize_t readen = read(file_handle, &buffer[size-position], BUFFER_SIZE-unread);
    if (readen < 1) {
        fprintf(stderr, "failed to refill buffer from file %s (total_read = %lu)\n", file_name, total_read);
        CRASH();
    }
    total_read += readen;
    size        = unread+readen;
    position    = 0;
}

void PS_FileBuffer::reinit(const char *_name, bool _readonly) {
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
    }
    else {
        file_flags = O_WRONLY | O_CREAT | O_EXCL;
        file_mode  = S_IRUSR | S_IWUSR;
    }
    file_handle = open(file_name, file_flags, file_mode);
    if (file_handle == -1) {
        if (_readonly) {
            fprintf(stderr, "failed to open file '%s' for reading\n", file_name);
        }
        else {
            fprintf(stderr, "failed to create file '%s' for writing\nmaybe it already exists ?\n", file_name);
        }
        CRASH();
    }

    // init. buffer
    clear();

    // debug
    total_read  = 0;
    total_write = 0;
}

PS_FileBuffer::PS_FileBuffer(const char *_name, bool _readonly)
    :
    file_name(strdup(_name)),
    is_readonly(_readonly), 
    file_pos(-1)
{
    // init. file

    if (is_readonly) {
        file_flags = O_RDONLY;
        file_mode  = 0;
    }
    else {
        file_flags = O_WRONLY | O_CREAT | O_EXCL;
        file_mode  = S_IRUSR | S_IWUSR;
    }
    file_handle = open(file_name, file_flags, file_mode);
    if (file_handle == -1) {
        if (_readonly) {
            fprintf(stderr, "failed to open file '%s' for reading\n", file_name);
        }
        else {
            fprintf(stderr, "failed to create file '%s' for writing\nmaybe it already exists ?\n", file_name);
        }
        CRASH();
    }

    // init. buffer
    size     = 0;
    position = 0;
    buffer   = (unsigned char *)malloc(BUFFER_SIZE);
    if (!buffer) {
        fprintf(stderr, "failed to allocate memory for buffer for file %s\n", file_name);
        CRASH();
    }

    // debug
    total_read  = 0;
    total_write = 0;
}
