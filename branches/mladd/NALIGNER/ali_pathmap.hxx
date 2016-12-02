// =============================================================== //
//                                                                 //
//   File      : ali_pathmap.hxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ALI_PATHMAP_HXX
#define ALI_PATHMAP_HXX

#ifndef ALI_TARRAY_HXX
#include "ali_tarray.hxx"
#endif

#define ALI_UNDEF 0x00
#define ALI_LEFT        0x01
#define ALI_DIAG  0x02
#define ALI_UP    0x04
#define ALI_LUP   0x08

// Structure for a long up pointer (a multi gap)
struct ali_pathmap_up_pointer {
    unsigned long start;
    unsigned char operation;
};

class ALI_PATHMAP : virtual Noncopyable {
    unsigned long width, height;
    unsigned long height_real;
    unsigned char **pathmap;
    ALI_TARRAY<ali_pathmap_up_pointer> ****up_pointers;
    unsigned char **optimized;

public:

    ALI_PATHMAP(unsigned long width, unsigned long height);
    ~ALI_PATHMAP();

    void set(unsigned long x, unsigned long y, unsigned char val,
             ALI_TARRAY<ali_pathmap_up_pointer> *up_pointer = 0);
    void get(unsigned long x, unsigned long y, unsigned char *val,
             ALI_TARRAY<ali_pathmap_up_pointer> **up_pointer);
    unsigned char get_value(unsigned long x, unsigned long y) {
        if (x >= width || y >= height)
            ali_fatal_error("Out of range", "ALI_PATHMAP::get_value()");
        if (y & 0x01)
            return (*pathmap)[x*height_real + y/2] & 0x0f;
        else
            return (*pathmap)[x*height_real + y/2] >> 4;
    }
    void optimize(unsigned long x);
    void print();
};

#else
#error ali_pathmap.hxx included twice
#endif // ALI_PATHMAP_HXX
