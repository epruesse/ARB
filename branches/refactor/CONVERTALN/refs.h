#ifndef REFS_H
#define REFS_H

template <typename R>
class Refs : Noncopyable {
    R   *ref;
    int  size;
public: 
    Refs() {
        ref  = NULL;
        size = 0;
    }
    ~Refs() {
        delete [] ref;
    }

    void resize(int new_size) {
        ca_assert(new_size >= size);
        if (new_size>size) {
            R *new_ref = new R[new_size];
            for (int i = 0; i<size; ++i) {
                new_ref[i] = ref[i];
            }
            delete [] ref;
            ref  = new_ref;
            size = new_size;
        }
    }

    int get_count() const { return size; }

    const R& get_ref(int num) const {
        ca_assert(num >= 0);
        ca_assert(num < get_count());
        return ref[num];
    }
    R& get_ref(int num) {
        ca_assert(num >= 0);
        ca_assert(num < get_count());
        return ref[num];
    }
};

#else
#error refs.h included twice
#endif // REFS_H
