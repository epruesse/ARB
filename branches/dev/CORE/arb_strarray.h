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

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif


typedef int (*StrArray_compare_fun)(const void *p0, const void *p1, void *client_data); // same as arb_sort.h@gb_compare_function 

class StrArray : virtual Noncopyable {
    char   **str;
    size_t   allocated;
    size_t   elems;

    bool ok() const {
        if (!str) {
            return elems == 0 && allocated == 0;
        }
        return str[elems] == NULL && allocated > elems;
    }

    bool elem_index(int i) const { return i >= 0 && size_t(i)<elems; }
    bool allocated_index(int i) const { return i >= 0 && size_t(i)<allocated; }

public:
    StrArray() : str(NULL), allocated(0), elems(0) {}
    ~StrArray() { erase(); free(str); }

    void erase() {
        arb_assert(ok());
        if (str) {
            for (size_t i = 0; i<allocated; ++i) {
                freenull(str[i]);
            }
        }
        arb_assert(ok());
    }

    void reserve(size_t forElems) {
        arb_assert(ok());
        ++forElems; // always allocate one element more (sentinel element)
        if (allocated<forElems) {
            size_t new_allocated                = forElems*3/2;
            if (new_allocated<10) new_allocated = 10;

            if (str) {
                arb_assert(allocated);
                str = (char**)realloc(str, new_allocated*sizeof(*str));
            }
            else {
                arb_assert(!allocated);
                str = (char**)malloc(new_allocated*sizeof(*str));
            }
            memset(str+allocated, 0, (new_allocated-allocated)*sizeof(*str));
            allocated = new_allocated;
        }
        arb_assert(ok());
    }

    void put(char *elem) { // tranfers ownership!
        arb_assert(ok());
        int i    = elems;
        reserve(i+1);
        arb_assert(allocated_index(i));
        str[i]   = elem;
        str[i+1] = NULL; // sentinel
        elems++;
        arb_assert(ok());
    }

    char *grab(int i) { // transfers ownership!
        arb_assert(ok());
        arb_assert(elem_index(i));
        char *elem = str[i];
        str[i]     = NULL;
        arb_assert(ok());
        return elem;
    }

    void remove(int i) {
        arb_assert(ok());
        arb_assert(elem_index(i));
        free(str[i]);
        while (size_t(i) < elems) { // move elems incl. sentinel
            str[i] = str[i+1];
            ++i;
        }
        elems--;
        arb_assert(ok());
    }

    void swap(int i1, int i2) {
        arb_assert(ok());
        arb_assert(elem_index(i1));
        arb_assert(elem_index(i2));
        std::swap(str[i1], str[i2]);
        arb_assert(ok());
    }

    const char *operator[](int i) const {
        arb_assert(ok());
        arb_assert(i >= 0 && size_t(i) <= elems); // allow sentinel access
        return str[i];
    }

    size_t size() const { return elems; }
    bool empty() const { return elems == 0; }

    void sort(StrArray_compare_fun compare, void *client_data);
};

void  GBT_split_string(StrArray& dest, const char *namelist, const char *separator, bool dropEmptyTokens);
void  GBT_split_string(StrArray& dest, const char *namelist, char separator);
char *GBT_join_names(const StrArray& names, char separator);
int   GBT_names_index_of(const StrArray& names, const char *search_for);
void  GBT_names_erase(StrArray& names, int index);
void  GBT_names_add(StrArray& names, int insert_before, const char *name);
void  GBT_names_move(StrArray& names, int old_index, int new_index);

#else
#error arb_strarray.h included twice
#endif // ARB_STRARRAY_H
