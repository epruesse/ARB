#ifndef PS_BITSET_HXX
#define PS_BITSET_HXX

//  ############################################################
//  # PS_BitSet
//  ############################################################
class PS_BitSet {
private:

    unsigned char *data;
    long size;                                                   // max. used index
    long capacity;                                               // number of allocated bits
    bool bias;                                                   // preset value for newly allocated memory

    PS_BitSet();
    PS_BitSet( const PS_BitSet& );

public:

    //    PS_BitSet& operator=(const PS_BitSet&){}

    bool get( const long index );
    void set( const long index, const bool value );

    bool reserve( const long _capacity );
    
    void print();

    PS_BitSet( bool _bias, long _capacity ) {
        data     = 0;
        size     = -1;
        capacity = 0;
        bias     = _bias;
        reserve( _capacity );
    }
    explicit PS_BitSet( bool _bias ) {
        data     = 0;
        size     = -1;
        capacity = 0;
        bias     = _bias;
    }

    ~PS_BitSet() {
        //if (data) fprintf( stderr, "~PS_BitSet(%p)   free(%p(%lu))\n", this, data, capacity/8 );
        if (data) free( data );
    }
};


// ************************************************************
// * set( index, value )
// ************************************************************
void PS_BitSet::set( const long index, const bool value ) {
    pg_assert( index >= 0 );
    reserve( index );
//     unsigned char byte   = data[index / 8];
//     unsigned char offset = index % 8;
//     unsigned char mask   = 1 << offset;
//     if (value == true) {
//         byte = byte |  mask; 
//     } else {
//         byte = byte & ~mask;
//     }
//     data[index / 8] = byte;
    if (value) {
        data[ index/8 ] |= 1 << (index % 8);
    } else {
        data[ index/8 ] |= ~(1 << (index % 8));
    }        
    if (index > size) size = index;
}


// ************************************************************
// * get( index )
// ************************************************************
bool PS_BitSet::get( const long index ) {
    pg_assert( index >= 0 );
    reserve( index );
//     unsigned char byte   = data[index / 8];
//     unsigned char offset = index % 8;
//     byte = (byte >> offset);
//     return (byte & 1 == 1);
    return ((data[ index/8 ] >> (index % 8)) & 1 == 1);
}


// ************************************************************
// * reserve( capacity )
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
// * print()
// ************************************************************
void PS_BitSet::print() {
    printf( "PS_BitSet : size(%li) capacity(%li)\n",size,capacity );
    for (long i = 0; i <= size; ++i) {
        printf( get(i) ? "+" : "_" );
    }
    if (size >= 0) printf( "\n" );
}

#else
#error ps_bitset.hxx included twice
#endif
