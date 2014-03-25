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
#include <arb_sort.h>

void CharPtrArray::sort(CharPtrArray_compare_fun compare, void *client_data) {
    GB_sort((void**)str, 0, size(), compare, client_data);
}

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

void GBT_splitNdestroy_string(ConstStrArray& names, char*& namelist, const char *separator, bool dropEmptyTokens) {
    /*! Split 'namelist' into an array of substrings at each member of 'separator'.
     *
     * @param names pointers to splitted parts (into namelist)
     * @param namelist string containing separator delimited parts
     * @param separator contains all characters handled as separators
     * @param dropEmptyTokens if true, empty tokens will be skipped
     *
     * Example:
     * @code
     * ConstStrArray array;
     * char *list = strdup("Peter;Paul;Mary");
     * GBT_splitNdestroy_string(array, list, ";", false);
     * // array[1] contains "Paul"
     * @endcode
     *
     * ownership of namelist is transferred to 'names'
     */

    names.set_memblock(namelist);

    char *sep = namelist;
    while (sep) {
        size_t nonsepcount = strcspn(sep, separator);
        if (nonsepcount || !dropEmptyTokens) {
            // names.put(GB_strpartdup(sep, sep+nonsepcount-1));
            names.put(sep);
            sep += nonsepcount;
        }
        size_t sepcount = strspn(sep, separator);
        sep[0] = 0;
        if (sepcount) {
            if (!dropEmptyTokens) {
                // for (size_t s = 1; s<sepcount; ++s) names.put(strdup(""));
                for (size_t s = 1; s<sepcount; ++s) names.put(sep);
            }
            sep += sepcount;
        }
        else {
            sep = NULL;
        }
    }
    namelist = NULL; // own it
}

void GBT_splitNdestroy_string(ConstStrArray& dest, char*& namelist, char separator) {
    char separator_string[] = "x";
    separator_string[0]   = separator;
    GBT_splitNdestroy_string(dest, namelist, separator_string, false);
}

char *GBT_join_names(const CharPtrArray& names, char separator) {
    /*! Joins a NULL-terminated array of 'char*' into one string
     *
     * @param names array of strings to join (maybe generated using GBT_split_string() or GBT_splitNdestroy_string)
     * @param separator is put between the concatenated strings
     *
     * @return heap-copy of joined strings
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

int GBT_names_index_of(const CharPtrArray& names, const char *search_for) {
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

void GBT_names_erase(CharPtrArray& names, int index) {
    if (index >= 0 && size_t(index)<names.size()) {
        names.remove(index);
    }
}

inline void move_last_elem(CharPtrArray& names, int before_pos) {
    int last_idx = int(names.size())-1;
    if (before_pos != -1 && before_pos < last_idx) {
        GBT_names_move(names, last_idx, before_pos);
    }
}
void GBT_names_add(ConstStrArray& names, int insert_before, const char *name) {
    // insert a new 'name' before position 'insert_before'
    // if 'insert_before' == -1 (or bigger than array size) -> append at end
    names.put(name);
    move_last_elem(names, insert_before);
}

void GBT_names_move(CharPtrArray& names, int old_index, int new_index) {
    /*! moves array-entry from 'old_index' to 'new_index' 
     * -1 means "last entry"
     * if new_index is out of bounds, it'll be moved to start of array
     */
    int size = (int)names.size();

    if (old_index == -1) old_index        = size-1;
    if (new_index == -1) new_index        = size-1;
    else if (new_index >= size) new_index = 0;

    if (old_index != new_index && new_index<size && old_index<size) {
        if (old_index>new_index) {
            for (int i = old_index-1; i >= new_index; --i) names.swap(i, i+1);
        }
        else {
            for (int i = old_index; i < new_index; ++i) names.swap(i, i+1);
        }
    }
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

void TEST_StrArray() {
    StrArray array;

    TEST_EXPECT(array.empty());
    TEST_EXPECT_EQUAL(array.size(), 0);
    TEST_EXPECT_NULL(array[0]);

    array.put(strdup("first"));

    TEST_REJECT(array.empty());
    TEST_EXPECT_EQUAL(array.size(), 1);
    TEST_EXPECT_EQUAL(array[0], "first");
    TEST_EXPECT_NULL(array[1]);

    array.put(strdup("second"));
    
    TEST_EXPECT_EQUAL(array.size(), 2);
    TEST_EXPECT_EQUAL(array[0], "first");
    TEST_EXPECT_EQUAL(array[1], "second");
    TEST_EXPECT_NULL(array[2]);

    array.remove(0);

    TEST_EXPECT_EQUAL(array.size(), 1);
    TEST_EXPECT_EQUAL(array[0], "second");
    TEST_EXPECT_NULL(array[1]);
    
    array.remove(0);

    TEST_EXPECT(array.empty());
    TEST_EXPECT_EQUAL(array.size(), 0);
    TEST_EXPECT_NULL(array[0]);
}

void TEST_StrArray_truncate() {
    ConstStrArray parts;
    GBT_split_string(parts, "test;word;bla", ';');

    TEST_EXPECT_EQUAL(parts.size(), 3);
    parts.resize(1000); TEST_EXPECT_EQUAL(parts.size(), 3);
    parts.resize(2); TEST_EXPECT_EQUAL(parts.size(), 2);
    parts.resize(1); TEST_EXPECT_EQUAL(parts.size(), 1);
    parts.resize(0); TEST_EXPECT(parts.empty());
}

#define TEST_SPLIT_JOIN(str,sep)                        \
    do {                                                \
        ConstStrArray cnames;                           \
        GBT_split_string(cnames, str, sep);             \
        char *joined = GBT_join_names(cnames, sep);     \
        TEST_EXPECT_EQUAL(str, joined);                 \
        free(joined);                                   \
    } while(0)

void TEST_GBT_split_join_names() {
    {                                               // simple split
        ConstStrArray names;
        GBT_split_string(names, "a*b*c", '*');
        size_t   count = names.size();

        TEST_EXPECT_EQUAL(count, 3U);
        TEST_EXPECT_EQUAL(names[0], "a");
        TEST_EXPECT_EQUAL(names[1], "b");
        TEST_EXPECT_EQUAL(names[2], "c");
    }
    {                                               // split string containing empty tokens
        ConstStrArray names;
        GBT_split_string(names, "**a**b*c*", '*');
        size_t   count = names.size();

        TEST_EXPECT_EQUAL(count, 7U);
        TEST_EXPECT_EQUAL(names[0], "");
        TEST_EXPECT_EQUAL(names[1], "");
        TEST_EXPECT_EQUAL(names[2], "a");
        TEST_EXPECT_EQUAL(names[3], "");
        TEST_EXPECT_EQUAL(names[4], "b");
        TEST_EXPECT_EQUAL(names[5], "c");
        TEST_EXPECT_EQUAL(names[6], "");
        TEST_EXPECT_NULL(names[7]);
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
    ConstStrArray names;
    GBT_split_string(names, "**a**b*c*", '*');

    TEST_EXPECT_EQUAL(GBT_names_index_of(names, "a"), 2);
    TEST_EXPECT_EQUAL(GBT_names_index_of(names, "b"), 4);
    TEST_EXPECT_EQUAL(GBT_names_index_of(names, "c"), 5);
    TEST_EXPECT_EQUAL(GBT_names_index_of(names, ""), 0);
    TEST_EXPECT_EQUAL(GBT_names_index_of(names, "no"), -1);
}

#define TEST_EXPECT_NAMES_JOIN_TO(names, sep, expected) \
    do {                                                \
        char *joined = GBT_join_names(names, sep);      \
        TEST_EXPECT_EQUAL(joined, expected);            \
        free(joined);                                   \
    } while(0)                                          \
    
void TEST_GBT_names_erase() {
    ConstStrArray names;
    GBT_split_string(names, "a*b*c*d*e", '*');

    TEST_EXPECT_EQUAL(names.size(), 5U);

    GBT_names_erase(names, 0); 
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*c*d*e");

    GBT_names_erase(names, 3);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*c*d");

    GBT_names_erase(names, 3);                      // index out of range
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*c*d");

    GBT_names_erase(names, -1);                     // illegal index
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*c*d");

    GBT_names_erase(names, 1);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*d");
}

void TEST_GBT_names_move() {
    ConstStrArray names;
    GBT_split_string(names, "a*b*c*dee", '*');

    GBT_names_move(names, 0, -1); // -1 means last
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*c*dee*a");
    GBT_names_move(names, -1, 0); 
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b*c*dee");
    GBT_names_move(names, 2, 3); 
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b*dee*c");
    GBT_names_move(names, 2, 1); 
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*dee*b*c");

    // test wrap arounds
    GBT_names_move(names, 0, -1);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "dee*b*c*a");
    GBT_names_move(names, -1, 99999);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*dee*b*c");
}

void TEST_GBT_names_add() { // test after GBT_names_move (cause add depends on move)
    ConstStrArray names;
    GBT_split_string(names, "a", '*');

    GBT_names_add(names, -1, "b");                  // append at end
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b");

    GBT_names_add(names, 2, "c");                   // append at end (using non-existing index)
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b*c");

    GBT_names_add(names, 99, "d");                  // append at end (using even bigger non-existing index)
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b*c*d");

    GBT_names_add(names, 2, "b2");                  // insert inside
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b*b2*c*d");

    GBT_names_add(names, 0, "a0");                  // insert at beginning
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a0*a*b*b2*c*d");

    GBT_names_add(names, 5, "d0");                  // insert before last
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a0*a*b*b2*c*d0*d");
}

#endif // UNIT_TESTS

