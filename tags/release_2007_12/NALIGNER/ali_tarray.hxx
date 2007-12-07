

#ifndef _ALI_TARRAY_INC_
#define _ALI_TARRAY_INC_

// #include <malloc.h>

#include "ali_misc.hxx"
#include "ali_tlist.hxx"

template<class T>
class ALI_TARRAY {
    T **array;
    unsigned long size_of_array;

public:
    ALI_TARRAY(unsigned long size) {
        size_of_array = size;
        array = (T **) calloc((unsigned int) size, sizeof(T));
        //array = (T (*) [1]) calloc((unsigned int) size, sizeof(T));
        if (array == 0)
            ali_fatal_error("Out of memory");
    }
    ALI_TARRAY(ALI_TLIST<T>* list) {
        unsigned long l = 0;
        size_of_array = list->cardinality();
        array = (T (*) []) calloc((unsigned int) size_of_array,sizeof(T));
        if (array == 0)
            ali_fatal_error("Out of memory");
        if (!list->is_empty()) {
            (*array)[l++] = list->first();
            while (list->is_next() && l < size_of_array)
                (*array)[l++] = list->next();
            if (list->is_next())
                ali_fatal_error("List inconsitent","ALI_TARRAY::ALI_TARRAY()");
        }
    }
    /*
      ALI_TARRAY(ALI_TARRAY<T> *ar) {
      unsigned long l = 0;
      size_of_array = ar->size_of_array;
      array = (T (*) []) calloc((unsigned int) size_of_array,sizeof(T));
      if (array == 0)
      ali_fatal_error("Out of memory");
      for (l = 0; l < size_of_array; l++)
      (*array)[l] = (*ar->array)[l];
      }
    */
    ~ALI_TARRAY(void) {
        if (array)
            free((char *) array);
    }
    unsigned long size(void) {
        return size_of_array;
    }
    void set(unsigned long position, T value) {
        if (position >= size_of_array)
            ali_fatal_error("Access out of array","ALI_TARRAY::set()");
        (*array)[position] = value;
    }
    T get(unsigned long position) {
        if (position >= size_of_array)
            ali_fatal_error("Access out of array","ALI_TARRAY::get()");
        return (*array)[position];
    }
};

#endif

