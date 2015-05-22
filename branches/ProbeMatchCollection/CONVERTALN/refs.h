#ifndef REFS_H
#define REFS_H

#ifndef INPUT_FORMAT_H
#include "input_format.h"
#endif
#ifndef RDP_INFO_H
#include "rdp_info.h"
#endif

template <typename REF>
class Refs : virtual Noncopyable {
    REF   *ref;
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
            REF *new_ref = new REF[new_size];
            for (int i = 0; i<size; ++i) {
                new_ref[i] = ref[i];
            }
            delete [] ref;
            ref  = new_ref;
            size = new_size;
        }
    }

    int get_count() const { return size; }

    const REF& get_ref(int num) const {
        ca_assert(num >= 0);
        ca_assert(num < get_count());
        return ref[num];
    }
    REF& get_ref(int num) {
        ca_assert(num >= 0);
        ca_assert(num < get_count());
        return ref[num];
    }
};

template<typename REF>
class RefContainer {
    Refs<REF> refs;

public:
    void reinit_refs() { INPLACE_RECONSTRUCT(Refs<REF>, &refs); }
    void resize_refs(int new_size) { refs.resize(new_size); }
    int get_refcount() const  { return refs.get_count(); }
    bool has_refs() const  { return get_refcount() > 0; }
    const REF& get_ref(int num) const { return refs.get_ref(num); }
    const REF& get_latest_ref() const { return get_ref(get_refcount()-1); }
    REF& get_ref(int num) { return refs.get_ref(num); }
    REF& get_latest_ref() { return get_ref(get_refcount()-1); }
    REF& get_new_ref() { resize_refs(get_refcount()+1); return get_latest_ref(); }
};

#else
#error refs.h included twice
#endif // REFS_H
