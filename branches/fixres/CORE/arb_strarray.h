// ============================================================ //
//                                                              //
//   File      : arb_strarray.h                                 //
//   Purpose   : handle arrays of strings                       //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2011   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef ARB_STRARRAY_H
#define ARB_STRARRAY_H

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARB_MEM_H
#include "arb_mem.h"
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif
#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif


typedef int (*CharPtrArray_compare_fun)(const void *p0, const void *p1, void *client_data); // same as arb_sort.h@gb_compare_function

class CharPtrArray : virtual Noncopyable {
    size_t allocated;

protected:
    char   **str;
    size_t   elems;

    bool ok() const {
        if (!str) {
            return elems == 0 && allocated == 0;
        }
        return str[elems] == NULL && allocated > elems;
    }

    bool elem_index(int i) const { return i >= 0 && size_t(i)<elems; }
    bool allocated_index(int i) const { return i >= 0 && size_t(i)<allocated; }

    void set_space(size_t new_allocated) {
        if (new_allocated != allocated) {
            arb_assert(ok());

            if (str) ARB_recalloc(str, allocated, new_allocated);
            else str = (char**)ARB_calloc(new_allocated, sizeof(*str));

            allocated = new_allocated;
            arb_assert(ok());
        }
    }

    void reserve_space(size_t forElems, bool alloc_ahead) {
        if (allocated <= forElems) {
            forElems = alloc_ahead ? (forElems>7 ? forElems*3/2 : 10) : forElems;
            set_space(forElems+1); // always allocate one element more (sentinel element)
        }
    }

    CharPtrArray() : allocated(0), str(NULL), elems(0) {}
    virtual ~CharPtrArray() { free(str); }

    virtual void free_elem(int i) = 0;

    void erase_elems() {
        arb_assert(ok());
        if (!empty()) {
            for (size_t i = 0; i<elems; ++i) free_elem(i);
            elems = 0;
            arb_assert(ok());
        }
    }
    
public:
    void reserve(size_t forElems) { reserve_space(forElems, false); }
    void optimize_space() { set_space(elems+1); }

    size_t size() const { return elems; }
    bool empty() const { return elems == 0; }

    const char *operator[](int i) const {
        arb_assert(ok());
        arb_assert(i >= 0 && size_t(i) <= elems); // allow sentinel access
        return elems ? str[i] : NULL;
    }
    
    void swap(int i1, int i2) {
        arb_assert(ok());
        arb_assert(elem_index(i1));
        arb_assert(elem_index(i2));
        std::swap(str[i1], str[i2]);
        arb_assert(ok());
    }
    void move(int from, int to);

    void remove(int i) {
        arb_assert(ok());
        arb_assert(elem_index(i));
        free_elem(i);
        while (size_t(i) < elems) { // move elems incl. sentinel
            str[i] = str[i+1];
            ++i;
        }
        elems--;
        arb_assert(ok());
    }
    void safe_remove(int i) {
        //! like remove, but does NOOP if index out of bounds
        if (elem_index(i)) remove(i);
    }

    void resize(int newsize) {
        // truncates array to 'newsize'
        for (int i = size()-1; i >= newsize; i--) {
            remove(i);
        }
    }
    void clear() { resize(0); }

    void sort(CharPtrArray_compare_fun compare, void *client_data);
    void uniq(CharPtrArray_compare_fun compare, void *client_data);
    void sort_and_uniq(CharPtrArray_compare_fun compare, void *client_data) {
        /*! sort the array and remove all duplicates
         * @param compare function defining order on elements (@see e.g. GB_string_comparator)
         * @param client_data user parameter forwarded to compare
         */
        sort(compare, client_data);
        uniq(compare, client_data);
    }

    int index_of(const char *search_for) const;
};

class StrArray : public CharPtrArray {
    virtual void free_elem(int i) OVERRIDE {
        freenull(str[i]);
    }

public:
    StrArray() {}
    virtual ~StrArray() OVERRIDE { erase(); }

    void erase() { erase_elems(); }

    void put(char *elem) { // tranfers ownership!
        int i    = elems;
        reserve_space(i+1, true);
        arb_assert(allocated_index(i));
        str[i]   = elem;
        str[i+1] = NULL; // sentinel
        elems++;
        arb_assert(ok());
    }
    void put_before(int insert_before, char *elem) { // tranfers ownership of elem!
        // insert a new 'name' before position 'insert_before'
        // if 'insert_before' == -1 (or bigger than array size) -> append at end
        put(elem);
        if (insert_before<int(size())) {
            move(-1, insert_before);
        }
    }

    char *replace(int i, char *elem) { // transfers ownership (in both directions!)
        arb_assert(elem_index(i));
        char *old = str[i];
        str[i]    = elem;
        arb_assert(ok());
        return old;
    }
};

class ConstStrArray : public CharPtrArray { // derived from a Noncopyable
    char *memblock;

    virtual void free_elem(int i) OVERRIDE { str[i] = NULL; }

public:
    ConstStrArray() : memblock(NULL) {}
    virtual ~ConstStrArray() OVERRIDE { free(memblock); }

    void set_memblock(char *block) {
        // hold one memblock until destruction
        arb_assert(!memblock);
        memblock = block;
    }

    void erase() {
        erase_elems();
        freenull(memblock);
    }

    void put(const char *elem) {
        int i    = elems;
        reserve_space(i+1, true);
        arb_assert(allocated_index(i));
        str[i]   = const_cast<char*>(elem);
        str[i+1] = NULL; // sentinel
        elems++;
        arb_assert(ok());
    }
    void put_before(int insert_before, const char *elem) {
        // insert a new 'name' before position 'insert_before'
        // if 'insert_before' == -1 (or bigger than array size) -> append at end
        put(elem);
        if (insert_before<int(size())) {
            move(-1, insert_before);
        }
    }

    const char *replace(int i, const char *elem) {
        arb_assert(elem_index(i));
        const char *old = str[i];
        str[i]    = const_cast<char*>(elem);
        arb_assert(ok());
        return old;
    }

};


void GBT_splitNdestroy_string(ConstStrArray& names, char*& namelist, const char *separator, bool dropEmptyTokens);
void GBT_splitNdestroy_string(ConstStrArray& dest, char*& namelist, char separator);

inline void GBT_split_string(ConstStrArray& dest, const char *namelist, const char *separator, bool dropEmptyTokens) {
    //! same as GBT_splitNdestroy_string, but w/o destroying namelist
    char *dup = strdup(namelist);
    GBT_splitNdestroy_string(dest, dup, separator, dropEmptyTokens);
}
inline void GBT_split_string(ConstStrArray& dest, const char *namelist, char separator) {
    char *dup = strdup(namelist);
    GBT_splitNdestroy_string(dest, dup, separator);
    // cppcheck-suppress memleak (GBT_splitNdestroy_string takes ownership of 'dup')
}

char *GBT_join_strings(const CharPtrArray& strings, char separator);

#else
#error arb_strarray.h included twice
#endif // ARB_STRARRAY_H
