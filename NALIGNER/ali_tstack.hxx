

#ifndef _ALI_TSTACK_INC_
#define _ALI_TSTACK_INC_

// #include <malloc.h>
#include "ali_misc.hxx"

template<class T>
class ALI_TSTACK {
    T **array;
    unsigned long size_of_array;
    unsigned long next_elem;

public:
    ALI_TSTACK(unsigned long size) {
        size_of_array = size;
        next_elem = 0;
        array = (T **) calloc((unsigned int) size, sizeof(T));
        //array = (T (*) [1]) calloc((unsigned int) size, sizeof(T));
    }
    ~ALI_TSTACK(void) {
        if (array)
            free((char *) array);
    }
    unsigned long max_size(void) {
        return size_of_array;
    }
    unsigned long akt_size(void) {
        return next_elem;
    }
    void push(T value, unsigned long count = 1) {
        if (next_elem + count - 1 >= size_of_array)
            ali_fatal_error("Access out of array","ALI_TSTACK::push()");
        for (; count > 0; count--)
            (*array)[next_elem++] = value;
    }
    T pop(unsigned long count = 1) {
        if (count == 0)
            ali_fatal_error("Nothing poped","ALI_TSTACK::pop()");
        if (next_elem - count + 1 <= 0)
            ali_fatal_error("Access out of array","ALI_TSTACK::pop()");
        next_elem -= count;
        return (*array)[next_elem];
    }
    T top(void) {
        if (next_elem <= 0)
            ali_fatal_error("Access out of array","ALI_TSTACK::top()");
        return (*array)[next_elem - 1];
    }
    T get(unsigned long position) {
        if (position >= next_elem) {
            ali_fatal_error("Access out of array","ALI_TSTACK::get()");
        }
        return (*array)[position];
    }
    void clear(void) {
        next_elem = 0;
    }
};

#endif

