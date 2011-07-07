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

typedef int (*StrArray_compare_fun)(const void *p0, const void *p1, void *client_data); // same as arb_sort.h@gb_compare_function 

class StrArray : virtual Noncopyable {
    char   **str;
    size_t   allocated;

public:
    StrArray() : str(NULL), allocated(0) {}
    ~StrArray() { erase(); free(str); }

    void erase() {
        if (str) {
            for (size_t i = 0; i<allocated; ++i) {
                freenull(str[i]);
            }
        }
    }

    void reserve(size_t forElems) {
        ++forElems; // always allocate one element more (sentinel element)
        if (allocated<forElems) {
            size_t new_allocated = forElems*3/2;
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
    }


    char*& operator[](int i) {
        reserve(i+1);
        arb_assert(i >= 0 && size_t(i)<allocated);
        return str[i];
    }
    const char *operator[](int i) const {
        arb_assert(i >= 0 && size_t(i)<allocated);
        return str[i];
    }

    size_t elems() const {
        size_t count = 0;
        if (allocated) { while (str[count]) ++count; }
        return count;
    }
    bool empty() const { return elems() == 0; }

    void sort(StrArray_compare_fun compare, void *client_data);
};

void    GBT_split_string(StrArray& dest, const char *namelist, const char *separator, bool dropEmptyTokens, size_t *countPtr);
void    GBT_split_string(StrArray& dest, const char *namelist, char separator, size_t *countPtr);
char   *GBT_join_names(const StrArray& names, char separator);
size_t  GBT_count_names(const StrArray& names);
int     GBT_names_index_of(const StrArray& names, const char *search_for);
void    GBT_names_erase(StrArray& names, int index);
void    GBT_names_add(StrArray& names, int insert_before, const char *name);
void    GBT_names_move(StrArray& names, int old_index, int new_index);

#else
#error arb_strarray.h included twice
#endif // ARB_STRARRAY_H
