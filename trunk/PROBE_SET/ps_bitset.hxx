#ifndef PS_BITSET_HXX
#define PS_BITSET_HXX

#ifndef __SET__
#include <set>
#endif
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
    long max_index;                                              // max. used index
    long capacity;                                               // number of allocated bits

    PS_BitSet( const PS_BitSet& );
    PS_BitSet();
    PS_BitSet( const bool _bias, const long _max_index, const long _capacity ) {
        bias      = _bias;
        max_index = _max_index;
        capacity  = _capacity;
    }

public:
    typedef set<long> IndexSet;

    virtual long getTrueIndices(  IndexSet &_index_set, const long _fill_index );
    virtual long getFalseIndices( IndexSet &_index_set, const long _fill_index );
    long         getMaxUsedIndex() { return max_index; }

    virtual bool get( const long _index );
    virtual bool set( const long _index, const bool _value );
    virtual void setTrue(  const long _index );
    virtual void setFalse( const long _index );

    virtual bool reserve( const long _capacity );
    virtual void invert();
    virtual void xor( const PS_BitSet *_other_set );
    
    virtual void print( const bool _header, const long _fill_index );
    virtual bool save( PS_FileBuffer *_file );
    virtual bool load( PS_FileBuffer *_file );

    explicit PS_BitSet( const bool _bias, const long _capacity ) : bias(_bias), max_index(-1), capacity(0) {
        data = 0;
        reserve( _capacity );
    }

    explicit PS_BitSet( PS_FileBuffer *_file ) : bias(false), max_index(-1), capacity(0) {
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

    PS_BitSet_Fast( const PS_BitSet_Fast& );
    PS_BitSet_Fast();

public:

    bool get( const long _index );
    bool set( const long _index, const bool _value );
    void setTrue(  const long _index );
    void setFalse( const long _index );

    explicit PS_BitSet_Fast( bool _bias, long _capacity ) : PS_BitSet( _bias,_capacity ) {
        // we just altered member functions so we dont do anything here
        // but call correct base class constructor to prevent
        // the call of PS_BitSet() without parameters
    }
};


// ************************************************************
// * PS_BitSet::getFalseIndices( _index_set, _fill_index  )
// ************************************************************
long PS_BitSet::getFalseIndices( PS_BitSet::IndexSet &_index_set, const long _fill_index = -1 ) {
    _index_set.clear();
    // get indices of falses from bitset up to max_index
    long index      = 0;
    long byte_index = 0;
    while (true) {
        unsigned char byte  = data[ byte_index++ ];     // get a data byte
        if (!(byte &   1)) _index_set.insert( index ); if (index++ >= max_index) break;
        if (!(byte &   2)) _index_set.insert( index ); if (index++ >= max_index) break;
        if (!(byte &   4)) _index_set.insert( index ); if (index++ >= max_index) break;
        if (!(byte &   8)) _index_set.insert( index ); if (index++ >= max_index) break;
        if (!(byte &  16)) _index_set.insert( index ); if (index++ >= max_index) break;
        if (!(byte &  32)) _index_set.insert( index ); if (index++ >= max_index) break;
        if (!(byte &  64)) _index_set.insert( index ); if (index++ >= max_index) break;
        if (!(byte & 128)) _index_set.insert( index ); if (index++ >= max_index) break;
    }
    // append indices [max_index+1 .. _fill_index] if bias is set to false
    if (!bias) {
        for (; (index <= _fill_index); ++index) {
            _index_set.insert( index );
        }
    }
    return _index_set.size();
}


// ************************************************************
// * PS_BitSet::getTrueIndices( _index_set, _fill_index  )
// ************************************************************
long PS_BitSet::getTrueIndices( PS_BitSet::IndexSet &_index_set, const long _fill_index = -1 ) {
    _index_set.clear();
    // get indices of falses from bitset up to max_index
    long index      = 0;
    long byte_index = 0;
    while (true) {
        unsigned char byte  = data[ byte_index++ ];     // get a data byte
        if (byte &   1) _index_set.insert( index ); if (index++ >= max_index) break;
        if (byte &   2) _index_set.insert( index ); if (index++ >= max_index) break;
        if (byte &   4) _index_set.insert( index ); if (index++ >= max_index) break;
        if (byte &   8) _index_set.insert( index ); if (index++ >= max_index) break;
        if (byte &  16) _index_set.insert( index ); if (index++ >= max_index) break;
        if (byte &  32) _index_set.insert( index ); if (index++ >= max_index) break;
        if (byte &  64) _index_set.insert( index ); if (index++ >= max_index) break;
        if (byte & 128) _index_set.insert( index ); if (index++ >= max_index) break;
    }
    // append indices [max_index+1 .. _max_index] if bias is set to true
    if (bias) {
        for (; (index <= _fill_index); ++index) {
            _index_set.insert( index );
        }
    }
    return _index_set.size();
}


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
        data[ _index/8 ] &= ~(1 << (_index % 8));
    }        
    if (_index > max_index) max_index = _index;
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
    data[ _index/8 ] &= 1 << (_index % 8);
    if (_index > max_index) max_index = _index;
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
    data[ _index/8 ] &= ~(1 << (_index % 8));
    if (_index > max_index) max_index = _index;
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
    long new_capacity_bytes = (_capacity/8)+1;
    long old_capacity_bytes = ( capacity/8)+1;
    if (capacity > 0) {
        if (new_capacity_bytes <= old_capacity_bytes) return true;      // smaller or same size requested ?
    }
    new_capacity_bytes = ((new_capacity_bytes / 32)+1)*32;              // adjust requested size to bigger chunks
    new_data = (unsigned char *)malloc( new_capacity_bytes );           // get new memory
    if (new_data == 0) return false;
    memset( new_data,bias ? 0xFF : 0,new_capacity_bytes );              // set memory to bias value
    if (capacity > 0) memcpy( new_data,data,old_capacity_bytes );       // copy old values
    if (data) free( data );                                             // free old memory
    data     = new_data;                                                // store new pointer
    capacity = new_capacity_bytes*8;                                    // store new capacity
    return true;
}


// ************************************************************
// * PS_BitSet::invert()
// ************************************************************
void PS_BitSet::invert() {
    for (long i = 0; i < capacity/8; ++i) {
        data[i] = ~data[i];
    }
}


// ************************************************************
// * PS_BitSet::xor( _other_set )
// ************************************************************
void PS_BitSet::xor( const PS_BitSet *_other_set ) {
    for (long i = 0; i < capacity/8; ++i) {
        data[i] ^= _other_set->data[i];
    }
}


// ************************************************************
// * PS_BitSet::print( _header, _fill_index )
// ************************************************************
void PS_BitSet::print( const bool _header = true, const long _fill_index = -1 ) {
    if (_header) printf( "PS_BitSet: bias(%1i) max_index(%6li) capacity(%6li) ", bias, max_index, capacity );
    for (long i = 0; i <= max_index; ++i) {
        printf( get(i) ? "+" : "_" );
    }
    for (long i = max_index+1; i <= _fill_index; ++i) {
        printf( "." );
    }
    printf( " %li\n", max_index );
}


// ************************************************************
// * PS_BitSet::save( PS_FileBuffer *_file )
// ************************************************************
bool PS_BitSet::save( PS_FileBuffer *_file ) {
    if (_file->isReadonly()) return false;
    // save max_index
    _file->put_long( max_index );
    // save bias
    _file->put_char( (bias) ? '1' : '0' );
    // save bitset
    long bytes = (max_index / 8)+1;
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
    // load max_index
    _file->get_long( max_index );
    // load bias
    bias = (_file->get_char() == '1');
    // initialize bitset
    capacity = 0;
    if (!reserve( max_index )) return false;
    // load bitset
    long bytes = (max_index / 8)+1;
    long i = 0;
    while ( (bytes-i) >= PS_FileBuffer::BUFFER_SIZE ) {
        _file->get( &(data[i]), PS_FileBuffer::BUFFER_SIZE );
        i += PS_FileBuffer::BUFFER_SIZE;
    }
    _file->get( &(data[i]), (bytes-i) );
    return true;
}


// ************************************************************
// * PS_BitSet_Fast::get( _index )
// ************************************************************
bool PS_BitSet_Fast::get( const long _index ) {
    if (_index > max_index) max_index = _index;
    return ((data[ _index/8 ] >> (_index % 8)) & 1 == 1);
}


// ************************************************************
// * PS_BitSet_Fast::set( _index, _value )
// ************************************************************
bool PS_BitSet_Fast::set( const long _index, const bool _value ) {
    bool previous_value = ((data[ _index/8 ] >> (_index % 8)) & 1 == 1); 
    if (_value) {
        data[ _index/8 ] |= 1 << (_index % 8);
    } else {
        data[ _index/8 ] &= ~(1 << (_index % 8));
    }        
    if (_index > max_index) max_index = _index;
    return previous_value;
}


// ************************************************************
// * PS_BitSet_Fast::setTrue( _index )
// ************************************************************
void PS_BitSet_Fast::setTrue( const long _index ) {
    data[ _index/8 ] |= 1 << (_index % 8);
    if (_index > max_index) max_index = _index;
}


// ************************************************************
// * PS_BitSet_Fast::setFalse( _index )
// ************************************************************
void PS_BitSet_Fast::setFalse( const long _index ) {
    data[ _index/8 ] &= ~(1 << (_index % 8));
    if (_index > max_index) max_index = _index;
}
#else
#error ps_bitset.hxx included twice
#endif
