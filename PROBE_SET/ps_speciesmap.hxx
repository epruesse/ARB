#ifndef PS_SPECIESMAP_HXX
#define PS_SPECIESMAP_HXX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __MAP__
#include <map>
#endif

#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif

using namespace std;

//***********************************************
//* PS_SpeciesMap
//***********************************************
class PS_SpeciesMap {
private:

    map<int,char*> id2str;
    map<char*,int> str2id;

public:
    //
    // access funtions
    //
    const char *getName( int _id );
    int         getID( const char *_name );
    void set( int _id, const char *_name ) {
        char *newStr = strdup(_name);
        id2str[_id] = newStr;
        str2id[newStr] = _id;
    }

    //
    // utility-functions
    //
    bool empty() { return id2str.empty(); }
    long size()  { return id2str.size();  }

    void clear() {
        id2str.clear();
        str2id.clear();
    }

    void save( PS_FileBuffer *_fb ) {
        unsigned int size;
        _fb->put_uint( id2str.size() );
        for (map<int,char*>::iterator it = id2str.begin();
             it != id2str.end();
             ++it) {
            size = strlen(it->second);
            _fb->put_int( it->first );
            _fb->put_uint( size );
            _fb->put( it->second, size );
        }
    }

    void saveASCII( PS_FileBuffer *_fb, char *_buffer ) {
        int count;
        count = sprintf( _buffer,"mapsize = %d\n", id2str.size()+1 );
        _fb->put( _buffer, count );
        for (map<int,char*>::iterator it = id2str.begin();
             it != id2str.end();
             ++it) {
            count = sprintf( _buffer,"[%6d] '%s'\n", it->first, it->second );
            _fb->put( _buffer,count );
        }
    }

    //
    // initialization-functions
    //
    explicit PS_SpeciesMap() {}
    explicit PS_SpeciesMap( PS_FileBuffer *_fb ) {
        unsigned int  mapSize;
        unsigned int  size;
        char         *buffer = (char*)malloc(100);
        int           id;
        if (!buffer) return;
        _fb->get_uint( mapSize );
        for (unsigned int i = 0; i < mapSize; ++i) {
            _fb->get_int( id );         // read id
            _fb->get_uint( size );      // read length of name
            _fb->get( buffer,size );    // read name
            buffer[size+1] = '\0';      // terminate name
            set( id,buffer );           // insert in maps
        }
        if (buffer) free(buffer);
    }
    
    ~PS_SpeciesMap() {
        char *s;
        for (map<int,char*>::iterator it = id2str.begin(); it != id2str.end(); ++it) {
            s = it->second;
            id2str.erase(it);
            if (s) free(s);
        }
        str2id.clear();
//         for (map<char*,int>::iterator it = str2id.begin(); it != str2id.end(); ++it) {
//             s = it->first;
//             str2id.erase(it);
//             if (s) free(s);
//         }
    }

};

#else
#error ps_speciesmap.hxx included twice
#endif
