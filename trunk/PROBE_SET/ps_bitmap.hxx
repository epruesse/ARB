#ifndef PS_BITMAP_HXX
#define PS_BITMAP_HXX

#ifndef PS_BITSET_HXX
#include "ps_bitset.hxx"
#endif

//  ############################################################
//  # PS_BitMap
//  ############################################################
class PS_BitMap {
private:

    PS_BitSet **data;                                            // array of pointers to PS_BitSet
    long size;
    long capacity;
    bool bias;
    bool is_triangular;

    PS_BitMap();
    PS_BitMap( const PS_BitMap& );

public:

    static const bool NORMAL     = false;
    static const bool TRIANGULAR = true;

    bool get( const long _x, const long _y );
    void set( const long _x, const long _y, const bool _value );
    // triangle_ - functions reverse _x and _y if _x is greater than _y
    // the resulting map is only triangular
    bool triangle_get( const long _x, const long _y );
    void triangle_set( const long _x, const long _y, const bool _value );

    bool reserve( const long _capacity );
    void invert();

    void print();

    explicit PS_BitMap( bool _bias, long _capacity, bool _triangular ) {
        size          = 0;
        capacity      = _capacity+1;
        bias          = _bias;
        is_triangular = _triangular;
        // init data
        data     = (PS_BitSet **)malloc(sizeof(PS_BitSet*)*capacity);
        if (_triangular) {
            //fprintf( stderr, "PS_BitMap(%p) malloc(%u) = %p\n", this, sizeof(PS_BitSet*)*8, data );
            for (long i = 0; i < capacity; ++i) {                    // init new requested bitsets
                data[i] = new PS_BitSet( bias,i );                   // init field
                if (data[i] == 0) *(int *)0 = 0;                     // check success
            }
        } else {
            //fprintf( stderr, "PS_BitMap(%p) malloc(%u) = %p\n", this, sizeof(PS_BitSet*)*8, data );
            for (long i = 0; i < capacity; ++i) {                    // init new requested bitsets
                data[i] = new PS_BitSet( bias,capacity );            // init field
                if (data[i] == 0) *(int *)0 = 0;                     // check success
            }
        }
    }

    explicit PS_BitMap( bool _bias, long _capacity ) {
        size          = 0;
        capacity      = _capacity+1;
        bias          = _bias;
        is_triangular = false;
        // init data
        data     = (PS_BitSet **)malloc(sizeof(PS_BitSet*)*8);
        //fprintf( stderr, "PS_BitMap(%p) malloc(%u) = %p\n", this, sizeof(PS_BitSet*)*8, data );
        for (long i = 0; i < capacity; ++i) {                    // init new requested bitsets
            data[i] = new PS_BitSet( bias );                     // init field
            if (data[i] == 0) *(int *)0 = 0;                     // check success
        }
    }

    explicit PS_BitMap( bool _bias ) {
        size          = 0;
        capacity      = 1;
        bias          = _bias;
        is_triangular = false;
        data     = (PS_BitSet **)malloc(sizeof(PS_BitSet*));
        //fprintf( stderr, "PS_BitMap(%p) malloc(%u) = %p\n", this, sizeof(PS_BitSet*), data );
        data[0]  = new PS_BitSet( bias );
    }

    ~PS_BitMap() {
        for (long i = 0; i < capacity; ++i) {
            delete data[i];
        }
        free( data );
        fprintf( stderr, "~PS_BitMap(%p)   free(%p(%lu))\n", this, data, capacity*sizeof(PS_BitSet*) );
    }
};


// ************************************************************
// * set( x,y,value )
// ************************************************************
void PS_BitMap::set( const long _x, const long _y, const bool _value ) {
    reserve( _x+1 );
    data[_x]->set(_y,_value);
    if (_x > size) size = _x;
}


// ************************************************************
// * get( x,y )
// ************************************************************
bool PS_BitMap::get( const long _x, const long _y ) {
    reserve( _x+1 );
    return data[_x]->get(_y);
}


// ************************************************************
// * triangle_set( x,y,value )
// ************************************************************
void PS_BitMap::triangle_set( const long _x, const long _y, const bool _value ) {
    if (_x > _y) {
        reserve( _x+1 );
        data[_x]->set(_y,_value);
        if (_x > size) size = _x;
    } else {
        reserve( _y+1 );
        data[_y]->set(_x,_value);
        if (_y > size) size = _y;
    }
}


// ************************************************************
// * triangle_get( x,y )
// ************************************************************
bool PS_BitMap::triangle_get( const long _x, const long _y ) {
    if (_x > _y) {
        reserve( _x+1 );
        return data[_x]->get(_y);
    } else {
        reserve( _y+1 );
        return data[_y]->get(_x);
    }
}


// ************************************************************
// * reserve( capacity )
// ************************************************************
bool PS_BitMap::reserve( const long _capacity ) {
    PS_BitSet **new_data;
    long _capacity_bytes = _capacity * sizeof(PS_BitSet*);
    long  capacity_bytes =  capacity * sizeof(PS_BitSet*);
    if (_capacity <= capacity) return true;                      // smaller or same size requested ?
    new_data = (PS_BitSet **)malloc( _capacity_bytes );          // get new memory for pointer array
    if (new_data == 0) return false;
    //fprintf( stderr, "PS_BitMap(%p) malloc(%lu) = %p\n", this, _capacity_bytes, new_data );
    memcpy( new_data,data,capacity_bytes );                      // copy old pointers
    //fprintf( stderr, "PS_BitMap(%p)   free(%p(%lu))\n", this, data, capacity_bytes );
    free( data );                                                // free old memory
    data = new_data;
    for (long i = capacity; i < _capacity; ++i) {                // init new requested bitsets
        data[i] = new PS_BitSet( bias );                         // init field
        if (data[i] == 0) return false;                          // check success
    }
    capacity = _capacity;                                        // store new capacity
    return true;
}


// ************************************************************
// * invert()
// ************************************************************
void PS_BitMap::invert() {
    bias = (bias == false);
    for (long i = 0; i <= size; ++i) {
        data[i]->invert();
    }
}


// ************************************************************
// * print()
// ************************************************************
void PS_BitMap::print() {
    printf( "PS_BitMap : size(%li) capacity(%li)\n",size,capacity );
    for (long i = 0; i <= size; ++i) {
        printf( "[%li] ",i);
        data[i]->print();
    }
}
#else
#error ps_bitmap.hxx included twice
#endif
