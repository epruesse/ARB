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

    PS_BitSet( const PS_BitSet& );
    PS_BitSet();
    PS_BitSet( const bool _bias, const long _max_index, const long _capacity ) {
        bias      = _bias;
        max_index = _max_index;
        capacity  = _capacity;
    }

public:
    long capacity;                                               // number of allocated bits
    typedef set<long> IndexSet;

    virtual long getTrueIndices(  IndexSet &_index_set, const long _fill_index );
    virtual long getFalseIndices( IndexSet &_index_set, const long _fill_index );
    virtual long getCountOfTrues( const long _fill_index = -1 );
    long         getMaxUsedIndex() { return max_index; }

    virtual bool get( const long _index );
    virtual bool set( const long _index, const bool _value );
    virtual void setTrue(  const long _index );
    virtual void setFalse( const long _index );

    virtual void invert();
    virtual void x_or( const PS_BitSet *_other_set );
    
    virtual void print( const bool _header, const long _fill_index );
    virtual bool save( PS_FileBuffer *_file );
    virtual bool load( PS_FileBuffer *_file, const long _fill_index );

    virtual bool copy( const PS_BitSet *_other_bitset );
    virtual bool reserve( const long _capacity );

    explicit PS_BitSet( const bool _bias, const long _capacity ) : bias(_bias), max_index(-1), capacity(0) {
        data = 0;
        reserve( _capacity );
    }

    explicit PS_BitSet( PS_FileBuffer *_file, const long _fill_index = -1 ) : bias(false), max_index(-1), capacity(0) {
        data = 0;
        load( _file, _fill_index );
    }

    virtual ~PS_BitSet() {
//         if (data) printf( "~PS_BitSet(%p) free(%p(%lu))\n", this, data, capacity/8 );
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

    virtual bool get( const long _index );
    virtual bool set( const long _index, const bool _value );
    virtual void setTrue(  const long _index );
    virtual void setFalse( const long _index );

    virtual bool reserve( const long _capacity );

    explicit PS_BitSet_Fast( PS_FileBuffer *_file, const long _fill_index = -1 ) : PS_BitSet(false,-1,0) {
        //printf( "PS_BitSet_Fast(%p)\n\t", this );
        data = 0;
        load( _file, _fill_index );
        //printf( "\tbias(%1i) max_index(%li) capacity(%li)\n", bias, max_index, capacity );
    }

    explicit PS_BitSet_Fast( bool _bias, long _capacity ) : PS_BitSet( _bias,_capacity ) {
        // we just altered member functions so we dont do anything here
        // but call correct base class constructor to prevent
        // the call of PS_BitSet() without parameters
        //printf( "PS_BitSet_Fast(%p) ", this );
    }

//     virtual ~PS_BitSet_Fast() {
//         printf( "~PS_BitSet_Fast(%p) ", this );
//     }
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
    // get indices of trues from bitset up to max_index
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
// * PS_BitSet::getCountOfTrues()
// ************************************************************
long PS_BitSet::getCountOfTrues( const long _fill_index) {
    long count = 0;
    // get indices of trues from bitset up to max_index
    long index      = 0;
    long byte_index = 0;
    while (true) {
        unsigned char byte  = data[ byte_index++ ];     // get a data byte
        if (byte &   1) ++count; if (index++ >= max_index) break;
        if (byte &   2) ++count; if (index++ >= max_index) break;
        if (byte &   4) ++count; if (index++ >= max_index) break;
        if (byte &   8) ++count; if (index++ >= max_index) break;
        if (byte &  16) ++count; if (index++ >= max_index) break;
        if (byte &  32) ++count; if (index++ >= max_index) break;
        if (byte &  64) ++count; if (index++ >= max_index) break;
        if (byte & 128) ++count; if (index++ >= max_index) break;
    }
    // append indices [max_index+1 .. _max_index] if bias is set to true
    if (bias && (_fill_index > max_index)) {
        count += _fill_index-max_index +1;
    }
    return count;
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
// * PS_BitSet::copy( _other_bitset )
// ************************************************************
bool PS_BitSet::copy( const PS_BitSet *_other_bitset ) {
    bias = _other_bitset->bias;
    if (!reserve( _other_bitset->capacity )) return false;
    memcpy( data, _other_bitset->data, (capacity>>3) );
    max_index = _other_bitset->max_index;
    return true;
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
    //printf( "PS_BitSet::reserve(%p) new_data = malloc(%li) = %p\n", this, new_capacity_bytes, new_data );
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
// * PS_BitSet::x_or( _other_set )
// ************************************************************
void PS_BitSet::x_or( const PS_BitSet *_other_set ) {
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
// * PS_BitSet::load( PS_FileBuffer *_file, const long _fill_index )
// ************************************************************
bool PS_BitSet::load( PS_FileBuffer *_file, const long _fill_index = -1 ) {
    // load max_index
    _file->get_long( max_index );
    // load bias
    bias = (_file->get_char() == '1');
    // initialize bitset
    capacity = 0;
    if (!reserve( (max_index > _fill_index) ? max_index : _fill_index )) return false;
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
    if (_index >= capacity) printf( "PS_BitSet_Fast::get( %li ) exceeds capacity %li\n", _index, capacity );
    if (_index > max_index) max_index = _index;
    return ((data[ _index/8 ] >> (_index % 8)) & 1 == 1);
}


// ************************************************************
// * PS_BitSet_Fast::set( _index, _value )
// ************************************************************
bool PS_BitSet_Fast::set( const long _index, const bool _value ) {
    if (_index >= capacity) printf( "PS_BitSet_Fast::set( %li,%1i ) exceeds capacity %li\n", _index, _value, capacity );
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
    if (_index >= capacity) printf( "PS_BitSet_Fast::setTrue( %li ) exceeds capacity %li\n", _index, capacity );
    data[ _index/8 ] |= 1 << (_index % 8);
    if (_index > max_index) max_index = _index;
}


// ************************************************************
// * PS_BitSet_Fast::setFalse( _index )
// ************************************************************
void PS_BitSet_Fast::setFalse( const long _index ) {
    if (_index >= capacity) printf( "PS_BitSet_Fast::setFalse( %li ) exceeds capacity %li\n", _index, capacity );
    data[ _index/8 ] &= ~(1 << (_index % 8));
    if (_index > max_index) max_index = _index;
}


// ************************************************************
// * PS_BitSet_Fast::reserve( _capacity )
// ************************************************************
//   as i assume that a user of the _FAST variant knows how much
//   data will be stored in the set we dont adjust the given
//   capacity to bigger chunks as PS_BitSet::reserve does
bool PS_BitSet_Fast::reserve( const long _capacity ) {
    unsigned char *new_data;
    long new_capacity_bytes = (_capacity/8)+1;
    long old_capacity_bytes = ( capacity/8)+1;
    if (capacity > 0) {
        if (new_capacity_bytes <= old_capacity_bytes) return true;      // smaller or same size requested ?
    }
    new_data = (unsigned char *)malloc( new_capacity_bytes );           // get new memory
    //printf( "PS_BitSet::reserve(%p) new_data = malloc(%li) = %p\n", this, new_capacity_bytes, new_data );
    if (new_data == 0) return false;
    memset( new_data,bias ? 0xFF : 0,new_capacity_bytes );              // set memory to bias value
    if (capacity > 0) memcpy( new_data,data,old_capacity_bytes );       // copy old values
    if (data) free( data );                                             // free old memory
    data     = new_data;                                                // store new pointer
    capacity = new_capacity_bytes*8;                                    // store new capacity
    return true;
}
#else
#error ps_bitset.hxx included twice
#endif
