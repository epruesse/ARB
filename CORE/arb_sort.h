// ============================================================ //
//                                                              //
//   File      : arb_sort.h                                     //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2011   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef ARB_SORT_H
#define ARB_SORT_H

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

typedef int (*gb_compare_function)(const void *p0, const void *p1, void *client_data);

void GB_sort(void **array, size_t first, size_t behind_last, gb_compare_function compare, void *client_data);
int GB_string_comparator(const void *v0, const void *v1, void */*unused*/);

#else
#error arb_sort.h included twice
#endif // ARB_SORT_H
