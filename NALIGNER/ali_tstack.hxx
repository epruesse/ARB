// =============================================================== //
//                                                                 //
//   File      : ali_tstack.hxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ALI_TSTACK_HXX
#define ALI_TSTACK_HXX

template<class T>
class ALI_TSTACK : virtual Noncopyable {
    T **array;
    unsigned long size_of_array;
    unsigned long next_elem;

public:
    ALI_TSTACK(unsigned long size) {
        size_of_array = size;
        next_elem = 0;
        array = (T **) calloc((unsigned int) size, sizeof(T));
    }
    ~ALI_TSTACK() {
        if (array)
            free((char *) array);
    }
    unsigned long max_size() {
        return size_of_array;
    }
    unsigned long akt_size() {
        return next_elem;
    }
    void push(T value, unsigned long count = 1) {
        if (next_elem + count - 1 >= size_of_array)
            ali_fatal_error("Access out of array", "ALI_TSTACK::push()");
        for (; count > 0; count--)
            (*array)[next_elem++] = value;
    }
    T pop(unsigned long count = 1) {
        if (count == 0)
            ali_fatal_error("Nothing poped", "ALI_TSTACK::pop()");
        if (next_elem - count + 1 <= 0)
            ali_fatal_error("Access out of array", "ALI_TSTACK::pop()");
        next_elem -= count;
        return (*array)[next_elem];
    }
    T top() {
        if (next_elem <= 0)
            ali_fatal_error("Access out of array", "ALI_TSTACK::top()");
        return (*array)[next_elem - 1];
    }
    T get(unsigned long position) {
        if (position >= next_elem) {
            ali_fatal_error("Access out of array", "ALI_TSTACK::get()");
        }
        return (*array)[position];
    }
    void clear() {
        next_elem = 0;
    }
};

#else
#error ali_tstack.hxx included twice
#endif // ALI_TSTACK_HXX
