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


char **GBT_split_string(const char *namelist, const char *separator, bool dropEmptyTokens, size_t *countPtr);
char **GBT_split_string(const char *namelist, char separator, size_t *countPtr);
char *GBT_join_names(const char *const *names, char separator);
size_t GBT_count_names(const char *const *names);
int GBT_names_index_of(const char *const *names, const char *search_for);
void GBT_names_erase(char **names, int index);
void GBT_names_add(char **&names, int insert_before, const char *name);
void GBT_names_move(char **names, int old_index, int new_index);
void GBT_free_names(char **names);

#else
#error arb_strarray.h included twice
#endif // ARB_STRARRAY_H
