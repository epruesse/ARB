#ifndef PS_BITSET_HXX
#define PS_BITSET_HXX

#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif

#ifndef NDEBUG
# define bs_assert(bed) do { if (!(bed)) *(int *)0=0; } while (0)
# ifndef DEBUG
#  error DEBUG is NOT defined - but it has to!
# endif
#else
# ifdef DEBUG
#  error DEBUG is defined - but it should not!
# endif
# define bs_assert(bed)
#endif /* NDEBUG */

//  ############################################################
//  # PS_BitSet
//  ############################################################
class PS_BitSet {
protected:

    unsigned char *data;
    bool bias;                                                   // preset value for newly allocated memory
    long size;                                                   // max. used index
    long capacity;                                               // number of allocated bits

    PS_BitSet( const PS_BitSet& );
    PS_BitSet();
    PS_BitSet( const bool _bias, const long _size, const long _capacity ) {
        bias     = _bias;
        size     = _size;
        capacity = _capacity;
    }

public:

    long getSize() { return size; }

    virtual bool get( const long _index );
    virtual bool set( const long _index, const bool _value );
    virtual void setTrue(  const long _index );
    virtual void setFalse( const long _index );

    virtual bool reserve( const long _capacity );
    virtual void invert();
    virtual void xor( const PS_BitSet *_other_set );
    
    virtual void print( long _fill_index );
    virtual bool save( PS_FileBuffer *_file );
    virtual bool load( PS_FileBuffer *_file );

    explicit PS_BitSet( const bool _bias, const long _capacity ) : bias(_bias), size(-1), capacity(0) {
        data = 0;
        reserve( _capacity );
    }

    explicit PS_BitSet( PS_FileBuffer *_file ) : bias(false), size(-1), capacity(0) {
        data = 0;
        load( _file );
    }

    virtual ~PS_BitSet() {
        //if (data) fprintf( stderr, "~PS_BitSet(%p)   free(%p(%lu))\n", this, data, capacity/8 );
        if (data) free( data );
    }
};

//  ############################################################
//  # PS_BitSet_Fast
//  ############################################################
class PS_BitSet_Fast : public PS_BitSet {
private:

    PS_BitSet_Fast();
    PS_BitSet_Fast( const PS_BitSet_Fast& );

public:

    bool get( const long _index ) { return ((data[ _index/8 ] >> (_index % 8)) & 1 == 1); }
    bool set( const long _index, const bool _value );
    void setTrue(  const long _index );
    void setFalse( const long _index );

    explicit PS_BitSet_Fast( bool _bias, long _capacity ) : PS_BitSet( _bias,_capacity ) {}
};


// ************************************************************
// * PS_BitSet::set( _index, _value )
// ************************************************************
bool PS_BitSet::set( const long _index, const bool _value ) {
    bs_assert( _index >= 0 );
    reserve( _index );
//     unsigned char byte   = data[_index / 8];
//     unsigned char offset = _index % 8;
//     unsigned char mask   = 1 << offset;
//     if (_value == true) {
//         byte = byte |  mask; 
//     } else {
//         byte = byte & ~mask;
//     }
//     data[_index / 8] = byte;
    bool previous_value = ((data[ _index/8 ] >> (_index % 8)) & 1 == 1); 
    if (_value) {
        data[ _index/8 ] |= 1 << (_index % 8);
    } else {
        data[ _index/8 ] |= ~(1 << (_index % 8));
    }        
    if (_index > size) size = _index;
    return previous_value;
}


// ************************************************************
// * PS_BitSet::setTrue( _index )
// ************************************************************
void PS_BitSet::setTrue( const long _index ) {
    bs_assert( _index >= 0 );
    reserve( _index );
//     unsigned char byte   = data[_index / 8];
//     unsigned char offset = _index % 8;
//     unsigned char mask   = 1 << offset;
//     if (_value == true) {
//         byte = byte |  mask; 
//     } else {
//         byte = byte & ~mask;
//     }
//     data[_index / 8] = byte;
    data[ _index/8 ] |= 1 << (_index % 8);
    if (_index > size) size = _index;
}


// ************************************************************
// * PS_BitSet::setFalse( _index )
// ************************************************************
void PS_BitSet::setFalse( const long _index ) {
    bs_assert( _index >= 0 );
    reserve( _index );
//     unsigned char byte   = data[_index / 8];
//     unsigned char offset = _index % 8;
//     unsigned char mask   = 1 << offset;
//     if (_value == true) {
//         byte = byte |  mask; 
//     } else {
//         byte = byte & ~mask;
//     }
//     data[_index / 8] = byte;
    data[ _index/8 ] |= ~(1 << (_index % 8));
    if (_index > size) size = _index;
}


// ************************************************************
// * PS_BitSet::get( _index )
// ************************************************************
bool PS_BitSet::get( const long _index ) {
    bs_assert( _index >= 0 );
    reserve( _index );
//     unsigned char byte   = data[_index / 8];
//     unsigned char offset = _index % 8;
//     byte = (byte >> offset);
//     return (byte & 1 == 1);
    return ((data[ _index/8 ] >> (_index % 8)) & 1 == 1);
}


// ************************************************************
// * PS_BitSet::reserve( _capacity )
// ************************************************************
bool PS_BitSet::reserve( const long _capacity ) {
    unsigned char *new_data;
    long _capacity_bytes = (_capacity/8)+1;
    long  capacity_bytes = ( capacity/8)+1;
    if (capacity > 0) {
        if (_capacity_bytes <= capacity_bytes) return true;      // smaller or same size requested ?
    }
    _capacity_bytes = ((_capacity_bytes / 128)+1)*128;           // adjust requested size to bigger chunks
    new_data = (unsigned char *)malloc( _capacity_bytes );       // get new memory
    if (new_data == 0) return false;
    //fprintf( stderr, "PS_BitSet(%p) malloc(%lu) = %p\n", this, _capacity_bytes, new_data );
    memset( new_data,bias ? 0xFF : 0,_capacity_bytes );          // set memory to bias value
    if (capacity > 0) memcpy( new_data,data,capacity_bytes );    // copy old values
    //if (data) fprintf( stderr, "PS_BitSet(%p)   free(%p(%lu))\n", this, data, capacity_bytes );
    if (data) free( data );                                      // free old memory
    data = new_data;                                             // store new pointer
    capacity = _capacity_bytes*8;                                // store new capacity
    return true;
}


// ************************************************************
// * PS_BitSet::invert()
// ************************************************************
void PS_BitSet::invert() {
    bias = (bias == false);
    for (long i = 0; i < capacity/8; ++i) {
        data[i] = ~data[i];
    }
}


// ************************************************************
// * PS_BitSet::xor( _other_set )
// ************************************************************
void PS_BitSet::xor( const PS_BitSet *_other_set ) {
    for (long i = 0; i < capacity/8; ++i) {
	printf( "PS_BitSet : xor %li\n",i );
        data[i] ^= _other_set->data[i];
    }
}


// ************************************************************
// * PS_BitSet::print( _fill_index )
// ************************************************************
void PS_BitSet::print( long _fill_index = -1 ) {
    printf( "PS_BitSet : bias (%5s) size(%6li) capacity(%6li) ",(bias) ? "true" : "false", size,capacity );
    for (long i = 0; i <= size; ++i) {
        printf( get(i) ? "+" : "_" );
    }
    if (bias) {
        for (long i = (size > 0) ? size+1 : 0; i <= _fill_index; ++i) {
            printf( "+" );
        }
    } else {
        for (long i = (size > 0) ? size+1 : 0; i <= _fill_index; ++i) {
            printf( "_" );
        }
    }
    printf( "\n" );
}


// ************************************************************
// * PS_BitSet::save( PS_FileBuffer *_file )
// ************************************************************
bool PS_BitSet::save( PS_FileBuffer *_file ) {
    if (_file->isReadonly()) return false;
    // save size
    _file->put_long( size );
    // save bias
    _file->put_char( (bias) ? '1' : '0' );
    // save bitset
    long bytes = (size / 8)+1;
    long i = 0;
    while ( (bytes-i) >= PS_FileBuffer::BUFFER_SIZE ) {
        _file->put( &(data[i]), PS_FileBuffer::BUFFER_SIZE );
        i += PS_FileBuffer::BUFFER_SIZE;
    }
    _file->put( &(data[i]), (bytes-i) );
    return true;
}


// ************************************************************
// * PS_BitSet::load( PS_FileBuffer *_file )
// ************************************************************
bool PS_BitSet::load( PS_FileBuffer *_file ) {
    // load size
    _file->get_long( size );
    // load bias
    bias = (_file->get_char() == '1');
    // initialize bitset
    capacity = 0;
    if (!reserve( size )) return false;
    // load bitset
    long bytes = (size / 8)+1;
    long i = 0;
    while ( (bytes-i) >= PS_FileBuffer::BUFFER_SIZE ) {
        _file->get( &(data[i]), PS_FileBuffer::BUFFER_SIZE );
        i += PS_FileBuffer::BUFFER_SIZE;
    }
    _file->get( &(data[i]), (bytes-i) );
    return true;
}


// ************************************************************
// * PS_BitSet_Fast::setTrue( _index )
// ************************************************************
void PS_BitSet_Fast::setTrue( const long _index ) {
//     unsigned char byte   = data[_index / 8];
//     unsigned char offset = _index % 8;
//     unsigned char mask   = 1 << offset;
//     if (_value == true) {
//         byte = byte |  mask; 
//     } else {
//         byte = byte & ~mask;
//     }
//     data[_index / 8] = byte;
    data[ _index/8 ] |= 1 << (_index % 8);
    if (_index > size) size = _index;
}


// ************************************************************
// * PS_BitSet_Fast::setFalse( _index )
// ************************************************************
void PS_BitSet_Fast::setFalse( const long _index ) {
//     unsigned char byte   = data[_index / 8];
//     unsigned char offset = _index % 8;
//     unsigned char mask   = 1 << offset;
//     if (_value == true) {
//         byte = byte |  mask; 
//     } else {
//         byte = byte & ~mask;
//     }
//     data[_index / 8] = byte;
    data[ _index/8 ] |= ~(1 << (_index % 8));
    if (_index > size) size = _index;
}


// ************************************************************
// * PS_BitSet_Fast::set( _index, _value )
// ************************************************************
bool PS_BitSet_Fast::set( const long _index, const bool _value ) {
//     unsigned char byte   = data[_index / 8];
//     unsigned char offset = _index % 8;
//     unsigned char mask   = 1 << offset;
//     if (_value == true) {
//         byte = byte |  mask; 
//     } else {
//         byte = byte & ~mask;
//     }
//     data[_index / 8] = byte;
    bool previous_value = ((data[ _index/8 ] >> (_index % 8)) & 1 == 1); 
    if (_value) {
        data[ _index/8 ] |= 1 << (_index % 8);
    } else {
        data[ _index/8 ] |= ~(1 << (_index % 8));
    }        
    if (_index > size) size = _index;
    return previous_value;
}


#else
#error ps_bitset.hxx included twice
#endif
