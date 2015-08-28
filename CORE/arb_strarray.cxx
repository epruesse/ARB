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
    /*! sort the array
     * @see sort_and_uniq for parameters
     */
    GB_sort((void**)str, 0, size(), compare, client_data);
}

void CharPtrArray::uniq(CharPtrArray_compare_fun compare, void *client_data) {
    /*! remove consecutive equal elements
     * @see sort_and_uniq for parameters
     */
    for (int i = size()-2; i >= 0; --i) {
        if (compare(str[i], str[i+1], client_data) == 0) {
            remove(i+1);
        }
    }
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
     * @param names pointers to split parts (into namelist)
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

char *GBT_join_strings(const CharPtrArray& strings, char separator) {
    /*! Joins a NULL-terminated array of 'char*' into one string
     *
     * @param strings   array of strings to join (maybe generated using GBT_split_string() or GBT_splitNdestroy_string)
     * @param separator is put between the concatenated strings
     *
     * @return heap-copy of joined strings
     */

    if (!strings[0]) return strdup("");

    GBS_strstruct *out = GBS_stropen(1000);
    GBS_strcat(out, strings[0]);
    arb_assert(strchr(strings[0], separator) == 0); // otherwise you'll never be able to GBT_split_string
    for (int n = 1; strings[n]; ++n) {
        GBS_chrcat(out, separator);
        GBS_strcat(out, strings[n]);
        arb_assert(strchr(strings[n], separator) == 0); // otherwise you'll never be able to GBT_split_string
    }
    return GBS_strclose(out);
}

int CharPtrArray::index_of(const char *search_for) const {
    // return index of 'search_for' or -1 if not found or given
    int index = -1;
    if (search_for) {
        for (int i = 0; str[i]; ++i) {
            if (strcmp(str[i], search_for) == 0) {
                index = i;
                break;
            }
        }
    }
    return index;
}

void CharPtrArray::move(int oidx, int nidx) {
    /*! moves an array-entry from 'oidx' to 'nidx'
     *  (entries between get shifted by one)
     * -1 means "last entry"
     * if 'nidx' is out of bounds, it'll be moved to start of array
     */
    int siz = size();

    if (oidx == -1)       oidx = siz-1;
    if (nidx == -1)       nidx = siz-1;
    else if (nidx >= siz) nidx = 0;

    arb_assert(nidx<siz);

    if (oidx != nidx && oidx<siz) {
        if (oidx>nidx) for (int i = oidx-1; i>= nidx; --i) swap(i, i+1);
        else           for (int i = oidx;   i<  nidx; ++i) swap(i, i+1);
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
        char *joined = GBT_join_strings(cnames, sep);   \
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

void TEST_StrArray_index_of() {
    ConstStrArray names;
    GBT_split_string(names, "**a**b*c*", '*');

    TEST_EXPECT_EQUAL(names.index_of("a"), 2);
    TEST_EXPECT_EQUAL(names.index_of("b"), 4);
    TEST_EXPECT_EQUAL(names.index_of("c"), 5);
    TEST_EXPECT_EQUAL(names.index_of(""), 0);
    TEST_EXPECT_EQUAL(names.index_of("no"), -1);
}

#define TEST_EXPECT_NAMES_JOIN_TO(names, sep, expected) \
    do {                                                \
        char *joined = GBT_join_strings(names, sep);    \
        TEST_EXPECT_EQUAL(joined, expected);            \
        free(joined);                                   \
    } while(0)                                          \
    
void TEST_StrArray_safe_remove() {
    ConstStrArray names;
    GBT_split_string(names, "a*b*c*d*e", '*');

    TEST_EXPECT_EQUAL(names.size(), 5U);

    names.safe_remove(0);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*c*d*e");

    names.safe_remove(3);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*c*d");

    names.safe_remove(3);                      // index out of range
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*c*d");

    names.safe_remove(-1);                     // illegal index
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*c*d");

    names.safe_remove(1);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*d");
}

void TEST_StrArray_move() {
    ConstStrArray names;
    GBT_split_string(names, "a*b*c*dee", '*');

    names.move(0, -1); // -1 means last
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "b*c*dee*a");
    names.move(-1, 0);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b*c*dee");
    names.move(2, 3);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b*dee*c");
    names.move(2, 1);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*dee*b*c");

    // test wrap arounds
    names.move(0, -1);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "dee*b*c*a");
    names.move(-1, 99999);
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*dee*b*c");
}

void TEST_StrArray_put_before() { // test after TEST_StrArray_move (cause put_before() depends on move())
    ConstStrArray names;
    GBT_split_string(names, "a", '*');

    names.put_before(-1, "b");          // append at end
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b");

    names.put_before(2, "c");           // append at end (using non-existing index)
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b*c");

    names.put_before(99, "d");          // append at end (using even bigger non-existing index)
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b*c*d");

    names.put_before(2, "b2");          // insert inside
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a*b*b2*c*d");

    names.put_before(0, "a0");          // insert at beginning
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a0*a*b*b2*c*d");

    names.put_before(5, "d0");          // insert before last
    TEST_EXPECT_NAMES_JOIN_TO(names, '*', "a0*a*b*b2*c*d0*d");
}
TEST_PUBLISH(TEST_StrArray_put_before);

#endif // UNIT_TESTS

