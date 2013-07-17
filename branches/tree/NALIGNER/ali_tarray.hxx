// =============================================================== //
//                                                                 //
//   File      : ali_tarray.hxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ALI_TARRAY_HXX
#define ALI_TARRAY_HXX

#ifndef ALI_TLIST_HXX
#include "ali_tlist.hxx"
#endif

template<class T>
class ALI_TARRAY : virtual Noncopyable {
    T **array;
    unsigned long size_of_array;

public:
    ALI_TARRAY(unsigned long Size) {
        size_of_array = Size;
        array = (T **) calloc((unsigned int) Size, sizeof(T));
        if (array == 0)
            ali_fatal_error("Out of memory");
    }
    ALI_TARRAY(ALI_TLIST<T>* list) {
        size_of_array = list->cardinality();
        array = (T (*) []) calloc((unsigned int) size_of_array, sizeof(T));
        if (array == 0)
            ali_fatal_error("Out of memory");
        if (!list->is_empty()) {
            unsigned long l = 0;
            (*array)[l++] = list->first();
            while (list->is_next() && l < size_of_array)
                (*array)[l++] = list->next();
            if (list->is_next())
                ali_fatal_error("List inconsitent", "ALI_TARRAY::ALI_TARRAY()");
        }
    }
    ~ALI_TARRAY() {
        if (array)
            free((char *) array);
    }
    unsigned long size() {
        return size_of_array;
    }
    void set(unsigned long position, T value) {
        if (position >= size_of_array)
            ali_fatal_error("Access out of array", "ALI_TARRAY::set()");
        (*array)[position] = value;
    }
    T get(unsigned long position) {
        if (position >= size_of_array)
            ali_fatal_error("Access out of array", "ALI_TARRAY::get()");
        return (*array)[position];
    }
};

#else
#error ali_tarray.hxx included twice
#endif // ALI_TARRAY_HXX
