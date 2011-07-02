// ============================================================ //
//                                                              //
//   File      : arb_strarray.cxx                               //
//   Purpose   : handle arrays of strings                       //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2011   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "arb_strarray.h"

#include <arb_str.h>
#include <arb_string.h>
#include <arb_strbuf.h>

/* ----------------------------------------
 * conversion between
 *
 * - char ** (heap-allocated array of heap-allocated char*'s)
 * - one string containing several substrings separated by a separator
 *   (e.g. "name1,name2,name3")
 */

#if defined(WARN_TODO)
#warning search for code which is splitting strings and use GBT_split_string there
#warning rename to GBS_split_string and move to string functions
#endif

char **GBT_split_string(const char *namelist, const char *separator, bool dropEmptyTokens, size_t *countPtr) {
    /*! Split 'namelist' into an array of substrings at each member of 'separator'.
     *
     * @result NULL-terminated array of heap-copies
     * Use GBT_free_names() to free it.
     *
     * @param dropEmptyTokens if true, empty tokens will be skipped
     * @param countPtr if != NULL it is set to the number of substrings found
     */

    char **names = NULL;

    for (int pass = 1; pass <= 2; ++pass) {
        const char *sep        = namelist;
        int         tokenCount = 0;

        while (sep) {
            size_t nonsepcount = strcspn(sep, separator);
            if (nonsepcount || !dropEmptyTokens) {
                if (pass == 2) {
                    names[tokenCount] = GB_strpartdup(sep, sep+nonsepcount-1);
                }
                tokenCount++;
                sep += nonsepcount;
            }
            size_t sepcount = strspn(sep, separator);
            if (sepcount) {
                if (!dropEmptyTokens) {
                    if (pass == 1) tokenCount += sepcount-1;
                    else {
                        for (size_t s = 1; s<sepcount; ++s) names[tokenCount++] = strdup("");
                    }
                }
                sep += sepcount;
            }
            else {
                sep = NULL;
            }
        }

        if (pass == 1) {
            names = (char**)malloc((tokenCount+1)*sizeof(*names));
        }
        else {
            names[tokenCount]       = NULL;
            if (countPtr) *countPtr = tokenCount;
        }
    }

    return names;
}

char **GBT_split_string(const char *namelist, char separator, size_t *countPtr) {
    char separator_string[] = "x";
    separator_string[0]   = separator;
    return GBT_split_string(namelist, separator_string, false, countPtr);
}

char *GBT_join_names(const char *const *names, char separator) {
    /*! Joins a NULL-terminated array of 'char*' into one string
     *
     * @param separator is put between the concatenated strings
     *
     * @return heap-copy of joined strings
     *
     * Note: inverse of GBT_split_string()
     */
    GBS_strstruct *out = GBS_stropen(1000);

    if (names[0]) {
        GBS_strcat(out, names[0]);
        arb_assert(strchr(names[0], separator) == 0); // otherwise you'll never be able to GBT_split_string
        int n;
        for (n = 1; names[n]; ++n) {
            GBS_chrcat(out, separator);
            GBS_strcat(out, names[n]);
            arb_assert(strchr(names[n], separator) == 0); // otherwise you'll never be able to GBT_split_string
        }
    }

    return GBS_strclose(out);
}

size_t GBT_count_names(const char *const *names) {
    size_t count = 0;
    if (names) while (names[count]) ++count;
    return count;
}

int GBT_names_index_of(const char *const *names, const char *search_for) {
    // return index of 'search_for' or -1 if not found or given
    int index = -1;
    if (search_for) {
        for (int i = 0; names[i]; ++i) {
            if (strcmp(names[i], search_for) == 0) {
                index = i;
                break;
            }
        }
    }
    return index;
}

void GBT_names_erase(char **names, int index) {
    if (index >= 0) {
        int i;
        for (i = 0; names[i] && i<index; ++i) {}
        free(names[i]);
        for (; names[i]; ++i) names[i] = names[i+1];
    }
}

void GBT_names_add(char**& names, int insert_before, const char *name) {
    // insert a new 'name' before position 'insert_before'
    // if 'insert_before' == -1 (or bigger than array size) -> append at end

    size_t old_count = GBT_count_names(names);
    size_t new_count = old_count+1;

    names              = (char**)realloc(names, (new_count+2)*sizeof(*names));
    names[new_count-1] = strdup(name);
    names[new_count]   = NULL;

    if (insert_before != -1 && insert_before < (int)old_count) {
        GBT_names_move(names, new_count-1, insert_before);
    }
}

void GBT_names_move(char **names, int old_index, int new_index) {
    /*! moves array-entry from 'old_index' to 'new_index' 
     * -1 means "last entry"
     * if new_index is out of bounds, it'll be moved to start of array
     */
    int size = (int)GBT_count_names(names);

    if (old_index == -1) old_index        = size-1;
    if (new_index == -1) new_index        = size-1;
    else if (new_index >= size) new_index = 0;

    if (old_index != new_index && new_index<size && old_index<size) {
        char *moved = names[old_index];
        if (old_index>new_index) {
            for (int i = old_index; i>new_index; --i) names[i] = names[i-1];
        }
        else {
            for (int i = old_index; i<new_index; ++i) names[i] = names[i+1];
        }
        names[new_index] = moved;
    }
}

void GBT_free_names(char **names) {
    if (names) {
        for (char **pn = names; *pn; pn++) free(*pn);
        free(names);
    }
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

#define TEST_SPLIT_JOIN(str, sep)                               \
    do {                                                        \
        size_t count;                                           \
        char **names  = GBT_split_string(str, sep, &count);     \
        char  *joined = GBT_join_names(names, sep);             \
        TEST_ASSERT_EQUAL(count, GBT_count_names(names));       \
        TEST_ASSERT_EQUAL(str, joined);                         \
        free(joined);                                           \
        GBT_free_names(names);                                  \
    } while(0)

void TEST_GBT_split_join_names() {
    {                                               // simple split
        size_t   count;
        char   **names = GBT_split_string("a*b*c", '*', &count);

        TEST_ASSERT_EQUAL(count, 3U);
        TEST_ASSERT_EQUAL(count, GBT_count_names(names));
        TEST_ASSERT_EQUAL(names[0], "a");
        TEST_ASSERT_EQUAL(names[1], "b");
        TEST_ASSERT_EQUAL(names[2], "c");

        GBT_free_names(names);
    }
    {                                               // split string containing empty tokens
        size_t   count;
        char   **names = GBT_split_string("**a**b*c*", '*', &count);

        TEST_ASSERT_EQUAL(count, 7U);
        TEST_ASSERT_EQUAL(count, GBT_count_names(names));
        TEST_ASSERT_EQUAL(names[0], "");
        TEST_ASSERT_EQUAL(names[1], "");
        TEST_ASSERT_EQUAL(names[2], "a");
        TEST_ASSERT_EQUAL(names[3], "");
        TEST_ASSERT_EQUAL(names[4], "b");
        TEST_ASSERT_EQUAL(names[5], "c");
        TEST_ASSERT_EQUAL(names[6], "");
        TEST_ASSERT_NULL(names[7]);

        GBT_free_names(names);
    }

    TEST_SPLIT_JOIN("a.b.c", '.');
    TEST_SPLIT_JOIN("a.b.c", '*');
    
    TEST_SPLIT_JOIN("..a.b.c", '.');
    TEST_SPLIT_JOIN("a.b.c..", '.');
    TEST_SPLIT_JOIN("a..b..c", '.');
    TEST_SPLIT_JOIN(".", '.');
    TEST_SPLIT_JOIN("....", '.');
}

void TEST_GBT_names_index_of() {
    char **names = GBT_split_string("**a**b*c*", '*', NULL);

    TEST_ASSERT_EQUAL(GBT_names_index_of(names, "a"), 2);
    TEST_ASSERT_EQUAL(GBT_names_index_of(names, "b"), 4);
    TEST_ASSERT_EQUAL(GBT_names_index_of(names, "c"), 5);
    TEST_ASSERT_EQUAL(GBT_names_index_of(names, ""), 0);
    TEST_ASSERT_EQUAL(GBT_names_index_of(names, "no"), -1);

    GBT_free_names(names);
}

#define TEST_ASSERT_NAMES_JOIN_TO(names, sep, expected) \
    do {                                                \
        char *joined = GBT_join_names(names, sep);      \
        TEST_ASSERT_EQUAL(joined, expected);            \
        free(joined);                                   \
    } while(0)                                          \
    
void TEST_GBT_names_erase() {
    char **names = GBT_split_string("a*b*c*d*e", '*', NULL);

    TEST_ASSERT_EQUAL(GBT_count_names(names), 5U);

    GBT_names_erase(names, 0); 
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "b*c*d*e");

    GBT_names_erase(names, 3);
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "b*c*d");

    GBT_names_erase(names, 3);                      // index out of range
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "b*c*d");

    GBT_names_erase(names, -1);                     // illegal index
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "b*c*d");

    GBT_names_erase(names, 1);
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "b*d");
    
    GBT_free_names(names);
}

void TEST_GBT_names_move() {
    char **names = GBT_split_string("a*b*c*dee", '*', NULL);

    GBT_names_move(names, 0, -1); // -1 means last
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "b*c*dee*a");
    GBT_names_move(names, -1, 0); 
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "a*b*c*dee");
    GBT_names_move(names, 2, 3); 
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "a*b*dee*c");
    GBT_names_move(names, 2, 1); 
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "a*dee*b*c");

    // test wrap arounds
    GBT_names_move(names, 0, -1);
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "dee*b*c*a");
    GBT_names_move(names, -1, 99999);
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "a*dee*b*c");
    
    GBT_free_names(names);
}

void TEST_GBT_names_add() { // test after GBT_names_move (cause add depends on move)
    char **names = GBT_split_string("a", '*', NULL);

    GBT_names_add(names, -1, "b");                  // append at end
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "a*b");

    GBT_names_add(names, 2, "c");                   // append at end (using non-existing index)
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "a*b*c");

    GBT_names_add(names, 99, "d");                  // append at end (using even bigger non-existing index)
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "a*b*c*d");

    GBT_names_add(names, 2, "b2");                  // insert inside
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "a*b*b2*c*d");

    GBT_names_add(names, 0, "a0");                  // insert at beginning
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "a0*a*b*b2*c*d");

    GBT_names_add(names, 5, "d0");                  // insert before last
    TEST_ASSERT_NAMES_JOIN_TO(names, '*', "a0*a*b*b2*c*d0*d");
    
    GBT_free_names(names);
}

#endif



