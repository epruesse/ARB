// =============================================================== //
//                                                                 //
//   File      : adcolumns.cxx                                     //
//   Purpose   : insert/delete columns                             //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <adGene.h>
#include <arb_progress.h>

#include "gb_local.h"

static char   *insDelBuffer = 0;
static size_t  insDelBuffer_size;

inline void free_insDelBuffer() {
    freenull(insDelBuffer);
}

static const char *gbt_insert_delete(const char *source, long srclen, long destlen, long *newlenPtr, long pos, long nchar, long mod, char insert_what, char insert_tail, int extraByte) {
    /* removes elems from or inserts elems into an array
     *
     * srclen           len of source
     * destlen          if != 0, then cut or append characters to get this len, otherwise keep srclen
     * newlenPtr        the resulting len
     * pos              where to insert/delete
     * nchar            and how many items
     * mod              size of an item
     * insert_what      insert this character (mod times)
     * insert_tail      append this character (if destlen>srclen)
     * extraByte        0 or 1. append extra zero byte at end? use 1 for strings!
     *
     * resulting array has destlen+nchar elements
     *
     * 1. array size is corrected to 'destlen' (by appending/cutting tail)
     * 2. part is deleted inserted
     *
     * return NULL, if nothing modified
     */

    const char *result;

    pos     *= mod;
    nchar   *= mod;
    srclen  *= mod;
    destlen *= mod;

    if (!destlen) destlen                       = srclen; // if no destlen is set then keep srclen
    if ((nchar<0) && (pos-nchar>destlen)) nchar = pos-destlen; // clip maximum characters to delete at end of array

    if (destlen == srclen && (pos>srclen || nchar == 0)) { // length stays same and clip-range is empty or behind end of sequence
        /* before 26.2.09 the complete data was copied in this case - but nevertheless NULL(=failure) was returned.
         * I guess this was some test accessing complete data w/o writing anything back to DB,
         * but AFAIK it was not used anywhere --ralf
         */
        result = NULL;
    }
    else {
        long newlen = destlen+nchar;                // length of result (w/o trailing zero-byte)
        if (newlen == 0) {
            result = "";
        }
        else {
            size_t neededSpace = newlen+extraByte;

            if (insDelBuffer && insDelBuffer_size<neededSpace) freenull(insDelBuffer);
            if (!insDelBuffer) {
                insDelBuffer_size = neededSpace;
                insDelBuffer      = (char*)malloc(neededSpace);
            }

            char *dest = insDelBuffer;
            gb_assert(dest);

            if (pos>srclen) {                       // insert/delete happens inside appended range
                insert_what = insert_tail;
                pos         = srclen;               // insert/delete directly after source, to avoid illegal access below
            }

            gb_assert(pos >= 0);
            if (pos>0) {                            // copy part left of pos
                // @@@ need to check vs smaller newlen
                memcpy(dest, source, (size_t)pos);
                dest   += pos;
                source += pos; srclen -= pos;
            }

            if (nchar>0) {                          // insert
                memset(dest, insert_what, (size_t)nchar);
                dest += nchar;
            }
            else if (nchar<0) {                     // delete
                source += -nchar; srclen -= -nchar;
            }

            if (srclen>0) {                         // copy rest of source
                // @@@ need to check vs smaller newlen
                memcpy(dest, source, (size_t)srclen);
                dest   += srclen;
                source += srclen; srclen = 0;
            }

            long rest = newlen-(dest-insDelBuffer);
            // gb_assert(rest >= 0); // @@@ disabled (fails in test-code below)

            if (rest>0) {                           // append tail
                memset(dest, insert_tail, rest);
                dest += rest;
            }

            if (extraByte) dest[0] = 0;             // append zero byte (used for strings)

            result = insDelBuffer;
        }
        *newlenPtr = newlen/mod;                    // report result length
    }
    return result;
}

enum TargetType {
    IDT_SPECIES = 0,
    IDT_SAI,
    IDT_SECSTRUCT,
};

static GB_CSTR targetTypeName[] = {
    "Species",
    "SAI",
    "SeceditStruct",
};

static bool insdel_shall_be_applied_to(GBDATA *gb_data, TargetType ttype) {
    bool        apply = true;
    const char *key   = GB_read_key_pntr(gb_data);

    if (key[0] == '_') {                            // don't apply to keys starting with '_'
        switch (ttype) {
            case IDT_SECSTRUCT:
            case IDT_SPECIES:
                apply = false;
                break;

            case IDT_SAI:
                if (strcmp(key, "_REF") != 0) {     // despite key is _REF
                    apply = false;
                }
                break;
        }
    }

    return apply;
}

inline int alignment_oversize(GBDATA *gb_data, TargetType ttype, long alilen, long found_size) {
    int oversize = 0;
    const char *key = GB_read_key_pntr(gb_data);
    switch (ttype) {
        case IDT_SPECIES:
            break;
        case IDT_SECSTRUCT:
            if (strcmp(key, "ref") == 0) { // 'ref' may be one byte longer than alignment size
                oversize = found_size-alilen;
            }
            break;
        case IDT_SAI:
            if (strcmp(key, "_REF") == 0) { // _REF may be one byte longer than alignment size
                oversize = found_size-alilen;
            }
            break;
    }
    return oversize;
}

class Alignment {
    SmartCharPtr name; // name of alignment
    long         len;  // length of alignment
public:
    Alignment(const char *name_, long len_) : name(strdup(name_)), len(len_) {}

    const char *get_name() const { return &*name; }
    long get_len() const { return len; }
};

class AlignmentOperator : virtual Noncopyable {
    long pos;        // start position of insert/delete
    long nchar;      // number of elements to insert/delete

    const char *delete_chars; // characters allowed to delete (array with 256 entries, value == 0 means deletion allowed)

    GB_ERROR apply(GBDATA *gb_data, TargetType ttype, const Alignment& ali) const;

    GB_ERROR apply_recursive(GBDATA *gb_data, TargetType ttype, const Alignment& ali) const;
    GB_ERROR apply_to_childs_named(GBDATA *gb_item_data, const char *item_field, TargetType ttype, const Alignment& ali) const;
    GB_ERROR apply_to_secstructs(GBDATA *gb_secstructs, const Alignment& ali) const;

public:


    AlignmentOperator(long pos_, long nchar_, const char *delete_chars_)
        : pos(pos_),
          nchar(nchar_),
          delete_chars(delete_chars_)
    {
        // gb_assert(correlated(nchar<0, delete_chars)); // @@@
    }

    long get_pos() const { return pos; }
    long get_amount() const { return nchar; }
    bool allowed_to_delete(char c) const {
        return delete_chars[safeCharIndex(c)] == 1;
    }

    GB_ERROR apply_to_alignment(GBDATA *gb_main, const Alignment& ali) const;
};

GB_ERROR AlignmentOperator::apply(GBDATA *gb_data, TargetType ttype, const Alignment& ali) const {
    gb_assert(get_pos() >= 0);

    GB_TYPES type  = GB_read_type(gb_data);
    GB_ERROR error = NULL;
    if (type >= GB_BITS && type != GB_LINK) {
        long size = GB_read_count(gb_data);

        if (ali.get_len() != size || get_amount() != 0) { // nothing would change
            if (insdel_shall_be_applied_to(gb_data, ttype)) {
                GB_CSTR source      = 0;
                long    mod         = sizeof(char);
                char    insert_what = 0;
                char    insert_tail = 0;
                char    extraByte   = 0;

                switch (type) {
                    case GB_STRING: {
                        source      = GB_read_char_pntr(gb_data);
                        extraByte   = 1;
                        insert_what = '-';
                        insert_tail = '.';

                        if (source) {
                            if (get_amount() > 0) { // insert
                                if (get_pos()<size) { // otherwise insert pos_obs is behind (old and too short) sequence -> dots are inserted at tail
                                    if ((get_pos()>0 && source[get_pos()-1] == '.') || source[get_pos()] == '.') { // dot at insert position?
                                        insert_what = '.'; // insert dots
                                    }
                                }
                            }
                            else { // delete
                                long after            = get_pos()+(-get_amount()); // position after deleted part
                                if (after>size) after = size;

                                for (long p = get_pos(); p<after; p++) {
                                    if (allowed_to_delete(source[p])) {
                                        error = GBS_global_string("You tried to delete '%c' at position %li  -> Operation aborted", source[p], p);
                                    }
                                }
                            }
                        }

                        break;
                    }
                    case GB_BITS:   source = GB_read_bits_pntr(gb_data, '-', '+');  insert_what = '-'; insert_tail = '-'; break;
                    case GB_BYTES:  source = GB_read_bytes_pntr(gb_data);           break;
                    case GB_INTS:   source = (GB_CSTR)GB_read_ints_pntr(gb_data);   mod = sizeof(GB_UINT4); break;
                    case GB_FLOATS: source = (GB_CSTR)GB_read_floats_pntr(gb_data); mod = sizeof(float); break;

                    default:
                        error = GBS_global_string("Unhandled type '%i'", type);
                        GB_internal_error(error);
                        break;
                }

                if (!error) {
                    if (!source) error = GB_await_error();
                    else {
                        long    modified_len;
                        int     oversize   = alignment_oversize(gb_data, ttype, ali.get_len(), size);
                        long    wanted_len = ali.get_len() + oversize;
                        GB_CSTR modified   = gbt_insert_delete(source, size, wanted_len, &modified_len, get_pos(), get_amount(), mod, insert_what, insert_tail, extraByte);

                        if (modified) {
                            gb_assert(modified_len == (ali.get_len()+get_amount()+oversize));

                            switch (type) {
                                case GB_STRING: error = GB_write_string(gb_data, modified);                          break;
                                case GB_BITS:   error = GB_write_bits  (gb_data, modified, modified_len, "-");       break;
                                case GB_BYTES:  error = GB_write_bytes (gb_data, modified, modified_len);            break;
                                case GB_INTS:   error = GB_write_ints  (gb_data, (GB_UINT4*)modified, modified_len); break;
                                case GB_FLOATS: error = GB_write_floats(gb_data, (float*)modified, modified_len);    break;

                                default: gb_assert(0); break;
                            }
                        }
                    }
                }
            }
        }
    }
    return error;
}

GB_ERROR AlignmentOperator::apply_recursive(GBDATA *gb_data, TargetType ttype, const Alignment& ali) const {
    GB_ERROR error = 0;
    GB_TYPES type  = GB_read_type(gb_data);

    if (type == GB_DB) {
        GBDATA *gb_child;
        for (gb_child = GB_child(gb_data); gb_child && !error; gb_child = GB_nextChild(gb_child)) {
            error = apply_recursive(gb_child, ttype, ali);
        }
    }
    else {
        error = apply(gb_data, ttype, ali);
    }

    return error;
}

GB_ERROR AlignmentOperator::apply_to_childs_named(GBDATA *gb_item_data, const char *item_field, TargetType ttype, const Alignment& ali) const {
    GBDATA   *gb_item;
    GB_ERROR  error      = 0;
    long      item_count = GB_number_of_subentries(gb_item_data);

    if (item_count) {
        arb_progress progress(item_field, item_count);

        for (gb_item = GB_entry(gb_item_data, item_field);
             gb_item && !error;
             gb_item = GB_nextEntry(gb_item))
        {
            GBDATA *gb_ali = GB_entry(gb_item, ali.get_name());
            if (gb_ali) {
                error = apply_recursive(gb_ali, ttype, ali);
                if (error) {
                    const char *item_name = GBT_read_name(gb_item);
                    error = GBS_global_string("%s '%s': %s", targetTypeName[ttype], item_name, error);
                }
            }
            progress.inc_and_check_user_abort(error);
        }
    }
    else {
        arb_progress dummy; // inc parent
    }
    return error;
}

GB_ERROR AlignmentOperator::apply_to_secstructs(GBDATA *gb_secstructs, const Alignment& ali) const {
    GB_ERROR  error  = 0;
    GBDATA   *gb_ali = GB_entry(gb_secstructs, ali.get_name());
    
    if (gb_ali) {
        long item_count = GB_number_of_subentries(gb_ali)-1;

        if (item_count<1) item_count = 1;
        arb_progress progress("secstructs", item_count);

        GBDATA *gb_item;
        for (gb_item = GB_entry(gb_ali, "struct");
             gb_item && !error;
             gb_item = GB_nextEntry(gb_item))
        {
            GBDATA *gb_ref = GB_entry(gb_item, "ref");
            if (gb_ref) {
                error = apply_recursive(gb_ref, IDT_SECSTRUCT, ali);
                if (error) {
                    const char *item_name = GBT_read_name(gb_item);
                    error = GBS_global_string("%s '%s': %s", targetTypeName[IDT_SECSTRUCT], item_name, error);
                }
            }
            progress.inc_and_check_user_abort(error);
        }
    }
    else {
        arb_progress dummy; // inc parent
    }
    return error;
}

GB_ERROR AlignmentOperator::apply_to_alignment(GBDATA *gb_main, const Alignment& ali) const {
    GB_ERROR error    = apply_to_childs_named(GBT_find_or_create(gb_main, "species_data",  7), "species",  IDT_SPECIES, ali);
    if (!error) error = apply_to_childs_named(GBT_find_or_create(gb_main, "extended_data", 7), "extended", IDT_SAI,     ali);
    if (!error) error = apply_to_secstructs(GB_search(gb_main, "secedit/structs", GB_CREATE_CONTAINER), ali);
    return error;
}

// --------------------------------------------------------------------------------

static GB_ERROR GBT_check_lengths(GBDATA *Main, const char *alignment_name) {
    GB_ERROR  error      = 0;
    GBDATA   *gb_presets = GBT_find_or_create(Main, "presets", 7);
    GBDATA   *gb_ali;

    AlignmentOperator op(0, 0, 0);

    for (gb_ali = GB_entry(gb_presets, "alignment");
         gb_ali && !error;
         gb_ali = GB_nextEntry(gb_ali))
    {
        GBDATA *gb_name = GB_find_string(gb_ali, "alignment_name", alignment_name, GB_IGNORE_CASE, SEARCH_CHILD);

        if (gb_name) {
            arb_progress progress("Formatting alignment", 3); // SAI, species and secstructs
            GBDATA *gb_len = GB_entry(gb_ali, "alignment_len");

            Alignment ali(GB_read_char_pntr(gb_name), GB_read_int(gb_len));
            error = op.apply_to_alignment(Main, ali);
            if (error) progress.done();
        }
    }
    free_insDelBuffer();
    return error;
}

GB_ERROR GBT_format_alignment(GBDATA *Main, const char *alignment_name) {
    GB_ERROR err = 0;

    if (strcmp(alignment_name, GENOM_ALIGNMENT) != 0) { // NEVER EVER format 'ali_genom'
        err           = GBT_check_data(Main, alignment_name); // detect max. length
        if (!err) err = GBT_check_lengths(Main, alignment_name); // format sequences in alignment
        if (!err) err = GBT_check_data(Main, alignment_name); // sets state to "formatted"
    }
    else {
        err = "It's forbidden to format '" GENOM_ALIGNMENT "'!";
    }
    return err;
}


GB_ERROR GBT_insert_character(GBDATA *Main, const char *alignment_name, long pos, long count, const char *char_delete)
{
    /* if count > 0     insert 'count' characters at pos
     * if count < 0     delete pos to pos+|count|
     *
     * Note: deleting is only performed, if found characters in deleted range are listed in 'char_delete'
     *       otherwise function returns with error
     *
     * This affects all species' and SAIs having data in given 'alignment_name' and
     * modifies several data entries found there
     * (see insdel_shall_be_applied_to for details which fields are affected).
     */

    GB_ERROR error = 0;

    if (pos<0) {
        error = GBS_global_string("Illegal sequence position %li", pos);
    }
    else {
        GBDATA *gb_ali;
        GBDATA *gb_presets = GBT_find_or_create(Main, "presets", 7);
        char    char_delete_list[256];

        if (strchr(char_delete, '%')) {
            memset(char_delete_list, 0, 256);
        }
        else {
            int ch;
            for (ch = 0; ch<256; ch++) {
                if (strchr(char_delete, ch)) char_delete_list[ch] = 0;
                else                         char_delete_list[ch] = 1;
            }
        }

        for (gb_ali = GB_entry(gb_presets, "alignment");
             gb_ali && !error;
             gb_ali = GB_nextEntry(gb_ali))
        {
            GBDATA *gb_name = GB_find_string(gb_ali, "alignment_name", alignment_name, GB_IGNORE_CASE, SEARCH_CHILD);

            if (gb_name) {
                GBDATA *gb_len = GB_entry(gb_ali, "alignment_len");
                long    len    = GB_read_int(gb_len);
                char   *use    = GB_read_string(gb_name);

                if (pos > len) {
                    error = GBS_global_string("Can't insert at position %li (exceeds length %li of alignment '%s')", pos, len, use);
                }
                else {
                    if (count < 0 && pos-count > len) count = pos - len;
                    error = GB_write_int(gb_len, len + count);
                }

                if (!error) {
                    arb_progress      insert_progress("Insert/delete characters", 3);
                    Alignment         ali(use, len);
                    AlignmentOperator op(pos, count, char_delete_list);

                    error = op.apply_to_alignment(Main, ali);
                    if (error) insert_progress.done();
                }
                free(use);
            }
        }

        free_insDelBuffer();

        if (!error) GB_disable_quicksave(Main, "a lot of sequences changed");
    }
    return error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>
#include <arb_unit_test.h>
#include <arb_defs.h>

#define DO_INSERT(str,alilen,pos,amount)                                                                \
    long        newlen;                                                                                 \
    const char *res = gbt_insert_delete(str, strlen(str), alilen, &newlen, pos, amount, 1, '-', '.', 1)
    
#define TEST_INSERT(str,alilen,pos,amount,expected)         do { DO_INSERT(str,alilen,pos,amount); TEST_ASSERT_EQUAL(res, expected);         } while(0)
#define TEST_INSERT__BROKEN(str,alilen,pos,amount,expected) do { DO_INSERT(str,alilen,pos,amount); TEST_ASSERT_EQUAL__BROKEN(res, expected); } while(0)

#define TEST_FORMAT(str,alilen,expected)         TEST_INSERT(str,alilen,0,0,expected)
#define TEST_FORMAT__BROKEN(str,alilen,expected) TEST_INSERT__BROKEN(str,alilen,0,0,expected)

#define TEST_DELETE(str,alilen,pos,amount,expected)         TEST_INSERT(str,alilen,pos,-(amount),expected)
#define TEST_DELETE__BROKEN(str,alilen,pos,amount,expected) TEST_INSERT__BROKEN(str,alilen,pos,-(amount),expected)

void TEST_insert_delete() {
    TEST_FORMAT("xxx",   5, "xxx..");
    TEST_FORMAT(".x.",   5, ".x...");
    TEST_FORMAT(".x..",  5, ".x...");
    TEST_FORMAT(".x...", 5, NULL); // NULL means "result == source"

    TEST_FORMAT__BROKEN("xxx--", 3, "xxx");
    TEST_FORMAT__BROKEN("xxx..", 3, "xxx");
    TEST_FORMAT__BROKEN("xxxxx", 3, "xxx");
    TEST_FORMAT__BROKEN("xxx",   0, ""); 

    // insert/delete in the middle
    TEST_INSERT("abcde", 5, 3, 0, NULL);
    TEST_INSERT("abcde", 5, 3, 1, "abc-de");
    TEST_INSERT("abcde", 5, 3, 2, "abc--de");

    TEST_DELETE("abcde",   5, 3, 0, NULL);
    TEST_DELETE("abc-de",  5, 3, 1, "abcde");
    TEST_DELETE("abc--de", 5, 3, 2, "abcde");

    // insert/delete at end
    TEST_INSERT("abcde", 5, 5, 1, "abcde-");
    TEST_INSERT("abcde", 5, 5, 4, "abcde----");

    TEST_DELETE__BROKEN("abcde-",    5, 5, 1, "abcde");
    TEST_DELETE__BROKEN("abcde----", 5, 5, 4, "abcde");
    
    // insert/delete at start
    TEST_INSERT("abcde", 5, 0, 1, "-abcde");
    TEST_INSERT("abcde", 5, 0, 4, "----abcde");

    TEST_DELETE("-abcde",    5, 0, 1, "abcde");
    TEST_DELETE("----abcde", 5, 0, 4, "abcde");


    // insert behind end
    TEST_INSERT__BROKEN("abcde", 10, 8, 1, "abcde...-.."); // expected_behavior ? 
    TEST_INSERT        ("abcde", 10, 8, 1, "abcde......"); // current_behavior

    TEST_INSERT__BROKEN("abcde", 10, 8, 4, "abcde...----.."); // expected_behavior ?
    TEST_INSERT        ("abcde", 10, 8, 4, "abcde........."); // current_behavior

    // insert/delete all
    TEST_INSERT("",    0, 0, 3, "---");
    TEST_DELETE("---", 3, 0, 3, "");

    free_insDelBuffer();
}

// ------------------------------

struct arb_unit_test::test_alignment_data TADinsdel[] = {
    { 1, "MtnK1722", "...G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUCACCUCC....." }, 
    { 1, "MhnFormi", "---A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUCACCUCCU....." }, 
    { 1, "MhnT1916", "...A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUCACCUCCU----" }, 
};

struct arb_unit_test::test_alignment_data EXTinsdel[] = {
    { 0, "ECOLI",    "---U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCCCACCUGA...." },
    { 0, "HELIX",    ".....[<[.........[[..[<<.[..].>>]....]]....].>......]" },
    { 0, "HELIX_NR", ".....1.1.........22..34..34.34..34...22....1........1" },
};

#define HELIX_REF    ".....x..x........x.x.x....x.x....x...x.x...x.........x"
#define HELIX_STRUCT "VERSION=3\nLOOP={etc.pp\n}\n"

const char *read_item_entry(GBDATA *gb_item, const char *ali_name, const char *entry_name) {
    const char *result = NULL;
    if (gb_item) {
        GBDATA *gb_ali = GB_find(gb_item, ali_name, SEARCH_CHILD);
        if (gb_ali) {
            GBDATA *gb_entry = GB_entry(gb_ali, entry_name);
            if (gb_entry) {
                result = GB_read_char_pntr(gb_entry);
            }
        }
    }
    if (!result) TEST_ASSERT_NO_ERROR(GB_await_error());
    return result;
}

#define TEST_ITEM_HAS_ENTRY(find,name,ename,expected)                                          \
    TEST_ASSERT_EQUAL(read_item_entry(find(gb_main, name), ali_name, ename), expected)

#define TEST_ITEM_HAS_DATA(find,name,expected) TEST_ITEM_HAS_ENTRY(find,name,"data",expected)

#define TEST_SPECIES_HAS_DATA(ad,sd)    TEST_ITEM_HAS_DATA(GBT_find_species,ad.name,sd)
#define TEST_SAI_HAS_DATA(ad,sd)        TEST_ITEM_HAS_DATA(GBT_find_SAI,ad.name,sd)
#define TEST_SAI_HAS_ENTRY(ad,ename,sd) TEST_ITEM_HAS_ENTRY(GBT_find_SAI,ad.name,ename,sd)

#define TEST_DATA(sd0,sd1,sd2,ed0,ed1,ed2,ref,struct) do {      \
        TEST_SPECIES_HAS_DATA(TADinsdel[0], sd0);               \
        TEST_SPECIES_HAS_DATA(TADinsdel[1], sd1);               \
        TEST_SPECIES_HAS_DATA(TADinsdel[2], sd2);               \
        TEST_SAI_HAS_DATA(EXTinsdel[0], ed0);                   \
        TEST_SAI_HAS_DATA(EXTinsdel[1], ed1);                   \
        TEST_SAI_HAS_DATA(EXTinsdel[2], ed2);                   \
        TEST_SAI_HAS_ENTRY(EXTinsdel[1], "_REF", ref);          \
        TEST_ASSERT_EQUAL(GB_read_char_pntr(GB_search(gb_main, "secedit/structs/ali_mini/struct/ref", GB_FIND)), ref); \
        TEST_SAI_HAS_ENTRY(EXTinsdel[1], "_STRUCT", struct);    \
    } while(0)

#define TEST_ALI_LEN_ALIGNED(len,aligned) do {                                          \
        TEST_ASSERT_EQUAL(GBT_get_alignment_len(gb_main, ali_name), len);               \
        TEST_ASSERT_EQUAL(GBT_get_alignment_aligned(gb_main, ali_name), aligned);       \
    }while(0)

void TEST_insert_delete_DB() {
    GB_shell    shell;
    ARB_ERROR   error;
    const char *ali_name = "ali_mini";
    GBDATA     *gb_main  = TEST_CREATE_DB(error, ali_name, TADinsdel, false);

    if (!error) {
        GB_transaction ta(gb_main);
        TEST_DB_INSERT_SAI(gb_main, error, ali_name, EXTinsdel);

        // add secondary structure to "HELIX"
        GBDATA *gb_helix     = GBT_find_SAI(gb_main, "HELIX");
        if (!gb_helix) error = GB_await_error();
        else {
            GBDATA *gb_struct     = GBT_add_data(gb_helix, ali_name, "_STRUCT", GB_STRING);
            if (!gb_struct) error = GB_await_error();
            else    error         = GB_write_string(gb_struct, HELIX_STRUCT);

            GBDATA *gb_struct_ref     = GBT_add_data(gb_helix, ali_name, "_REF", GB_STRING);
            if (!gb_struct_ref) error = GB_await_error();
            else    error             = GB_write_string(gb_struct_ref, HELIX_REF);
        }

        // add stored secondary structure
        GBDATA *gb_ref     = GB_search(gb_main, "secedit/structs/ali_mini/struct/ref", GB_STRING);
        if (!gb_ref) error = GB_await_error();
        else    error      = GB_write_string(gb_ref, HELIX_REF);
    }

    if (!error) {
        GB_transaction ta(gb_main);

        for (int pass = 1; pass <= 2; ++pass) {
            if (pass == 1) TEST_ALI_LEN_ALIGNED(52, 1);
            if (pass == 2) TEST_ALI_LEN_ALIGNED(53, 0); // was marked as "not aligned"
            TEST_DATA("...G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUCACCUCC.....",
                      "---A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUCACCUCCU.....",
                      "...A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUCACCUCCU----",
                      "---U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCCCACCUGA....",
                      ".....[<[.........[[..[<<.[..].>>]....]]....].>......]",
                      ".....1.1.........22..34..34.34..34...22....1........1",
                      ".....x..x........x.x.x....x.x....x...x.x...x.........x",
                      HELIX_STRUCT);

            if (pass == 1) TEST_ASSERT_NO_ERROR(GBT_check_data(gb_main, ali_name));
        }

        TEST_ASSERT_NO_ERROR(GBT_format_alignment(gb_main, ali_name));
        TEST_ALI_LEN_ALIGNED(53, 1);
        TEST_DATA("...G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUCACCUCC......",
                  "---A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUCACCUCCU.....",
                  "...A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUCACCUCCU----.", // @@@ <- should convert '-' to '.' or append '-' 
                  "---U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCCCACCUGA.....",
                  ".....[<[.........[[..[<<.[..].>>]....]]....].>......]",
                  ".....1.1.........22..34..34.34..34...22....1........1",
                  ".....x..x........x.x.x....x.x....x...x.x...x.........x",
                  HELIX_STRUCT);

// editor column -> alignment column
#define COL(col) ((col)-19)

        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(60), 2, "")); // insert in middle
        TEST_ALI_LEN_ALIGNED(55, 1);
        TEST_DATA("...G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC......",
                  "---A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.....",
                  "...A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU----.", 
                  "---U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.....",
                  ".....[<[.........[[..[<<.[..].>>]....]]......].>......]",
                  ".....1.1.........22..34..34.34..34...22......1........1",
                  ".....x..x........x.x.x....x.x....x...x.x.....x.........x",
                  HELIX_STRUCT);
        
        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(71), 2, "")); // insert near end
        TEST_ALI_LEN_ALIGNED(57, 1);
        TEST_DATA("...G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "---A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  "...A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "---U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".....[<[.........[[..[<<.[..].>>]....]]......].>........]",
                  ".....1.1.........22..34..34.34..34...22......1..........1",
                  ".....x..x........x.x.x....x.x....x...x.x.....x...........x",
                  HELIX_STRUCT);

        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(20), 2, "")); // insert near start
        TEST_ALI_LEN_ALIGNED(59, 1);
        TEST_DATA(".....G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "-----A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".......[<[.........[[..[<<.[..].>>]....]]......].>........]",
                  ".......1.1.........22..34..34.34..34...22......1..........1",
                  ".......x..x........x.x.x....x.x....x...x.x.....x...........x",
                  HELIX_STRUCT);

        
        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(26), 2, "")); // insert at left helix start
        TEST_ALI_LEN_ALIGNED(61, 1);
        TEST_DATA(".....G---GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "-----A---CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".........[<[.........[[..[<<.[..].>>]....]]......].>........]",
                  ".........1.1.........22..34..34.34..34...22......1..........1",
                  ".........x..x........x.x.x....x.x....x...x.x.....x...........x",
                  HELIX_STRUCT);

        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(29), 2, "")); // insert behind left helix start
        TEST_ALI_LEN_ALIGNED(63, 1);
        TEST_DATA(".....G---G--GC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "-----A---C--GA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---C--GA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---G--CC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".........[--<[.........[[..[<<.[..].>>]....]]......].>........]", // @@@ <- should insert dots
                  ".........1...1.........22..34..34.34..34...22......1..........1",
                  ".........x....x........x.x.x....x.x....x...x.x.....x...........x",
                  HELIX_STRUCT);

        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(32), 2, "")); // insert at left helix end
        TEST_ALI_LEN_ALIGNED(65, 1);
        TEST_DATA(".....G---G--G--C-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "-----A---C--G--A-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".........[--<--[.........[[..[<<.[..].>>]....]]......].>........]", // @@@ <- should insert dots
                  ".........1.....1.........22..34..34.34..34...22......1..........1",
                  ".........x......x........x.x.x....x.x....x...x.x.....x...........x",
                  HELIX_STRUCT);

        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(35), 2, "")); // insert behind left helix end
        TEST_ALI_LEN_ALIGNED(67, 1);
        TEST_DATA(".....G---G--G--C---C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".........[--<--[...........[[..[<<.[..].>>]....]]......].>........]", 
                  ".........1.....1...........22..34..34.34..34...22......1..........1",
                  ".........x........x........x.x.x....x.x....x...x.x.....x...........x", // @@@ _REF gets destroyed here! (see #159)
                  HELIX_STRUCT);

        
        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(57), 2, "")); // insert at right helix start
        TEST_ALI_LEN_ALIGNED(69, 1);
        TEST_DATA(".....G---G--G--C---C-G...--AGGGGAA-CCU--G-CGGC-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----CGGGGAA-CCU--G-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....GGGGGAA-CCU--G-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-----GCGGCCU-UAG--C-GCGG-UGGUCC--CACCUGA.......",
                  ".........[--<--[...........[[..[<<.[....].>>]....]]......].>........]",
                  ".........1.....1...........22..34..34...34..34...22......1..........1",
                  ".........x........x........x.x.x....x...x....x...x.x.....x...........x",
                  HELIX_STRUCT);

        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(60), 2, "")); // insert behind right helix start
        TEST_ALI_LEN_ALIGNED(71, 1);
        TEST_DATA(".....G---G--G--C---C-G...--AGGGGAA-CCU--G---CGGC-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----CGGGGAA-CCU--G---CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....GGGGGAA-CCU--G---CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-----GCGGCCU-UAG--C---GCGG-UGGUCC--CACCUGA.......",
                  ".........[--<--[...........[[..[<<.[....]...>>]....]]......].>........]", 
                  ".........1.....1...........22..34..34...3--4..34...22......1..........1", // @@@ <- helix nr destroyed + shall use dots
                  ".........x........x........x.x.x....x...x......x...x.x.....x...........x", 
                  HELIX_STRUCT);

        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(65), 2, "")); // insert at right helix end
        TEST_ALI_LEN_ALIGNED(73, 1);
        TEST_DATA(".....G---G--G--C---C-G...--AGGGGAA-CCU--G---CG--GC-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----CGGGGAA-CCU--G---CG--GC-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....GGGGGAA-CCU--G---CG--GC-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-----GCGGCCU-UAG--C---GC--GG-UGGUCC--CACCUGA.......",
                  ".........[--<--[...........[[..[<<.[....]...>>--]....]]......].>........]", // @@@ <- shall insert dots
                  ".........1.....1...........22..34..34...3--4....34...22......1..........1", 
                  ".........x........x........x.x.x....x...x........x...x.x.....x...........x",
                  HELIX_STRUCT);

        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(68), 2, ""));           // insert behind right helix end
        TEST_ALI_LEN_ALIGNED(75, 1);
        TEST_DATA(".....G---G--G--C---C-G...--AGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----CGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....GGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-----GCGGCCU-UAG--C---GC--G--G-UGGUCC--CACCUGA.......",
                  ".........[--<--[...........[[..[<<.[....]...>>--]......]]......].>........]",
                  ".........1.....1...........22..34..34...3--4....3--4...22......1..........1", // @@@ <- helix nr destroyed + shall use dots
                  ".........x........x........x.x.x....x...x..........x...x.x.....x...........x", // @@@ _REF gets destroyed here! (see #159)
                  HELIX_STRUCT);

        

        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(44), 2, ""));           // insert at gap border
        TEST_ALI_LEN_ALIGNED(77, 1);
        TEST_DATA(".....G---G--G--C---C-G.....--AGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-------CGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.......GGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-------GCGGCCU-UAG--C---GC--G--G-UGGUCC--CACCUGA.......",
                  ".........[--<--[.............[[..[<<.[....]...>>--]......]]......].>........]",
                  ".........1.....1.............22..34..34...3--4....3--4...22......1..........1", 
                  ".........x........x..........x.x.x....x...x..........x...x.x.....x...........x", 
                  HELIX_STRUCT);

        
        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(42), -6, "-.")); // delete gaps
        TEST_ALI_LEN_ALIGNED(71, 1);
        TEST_DATA(".....G---G--G--C---C-G.AGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-CGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.GGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-GCGGCCU-UAG--C---GC--G--G-UGGUCC--CACCUGA.......",
                  ".........[--<--[.......[[..[<<.[....]...>>--]......]]......].>........]",
                  ".........1.....1.......22..34..34...3--4....3--4...22......1..........1", 
                  ".........x........x....x.x.x....x...x..........x...x.x.....x...........x", 
                  HELIX_STRUCT);
        
        
        TEST_ASSERT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(71), -4, "%")); // delete anything
        TEST_ALI_LEN_ALIGNED(67, 1);
        TEST_DATA(".....G---G--G--C---C-G.AGGGGAA-CCU--G---CG--G--C-UGG-ACCUCC........",
                  "-----A---C--G--A---U-C-CGGGGAA-CCU--G---CG--G--C-UGG-ACCUCCU.......",
                  ".....A---C--G--A---A-C.GGGGGAA-CCU--G---CG--G--C-UGG-ACCUCCU------.",
                  "-----U---G--C--C---U-G-GCGGCCU-UAG--C---GC--G--G-UGG-CACCUGA.......",
                  ".........[--<--[.......[[..[<<.[....]...>>--]......]...].>........]",
                  ".........1.....1.......22..34..34...3--4....3--4...2...1..........1", 
                  ".........x........x....x.x.x....x...x..........x...x...x...........x", 
                  HELIX_STRUCT);

    }

    if (!error) {
        {
            GB_transaction ta(gb_main);
            TEST_ASSERT_EQUAL(GBT_insert_character(gb_main, ali_name, COL(35), -3, "-."), // illegal delete
                              "SAI 'HELIX': You tried to delete 'x' at position 18  -> Operation aborted");
            ta.close("xxx");
        }
        {
            GB_transaction ta(gb_main);
            TEST_ASSERT_EQUAL(GBT_insert_character(gb_main, ali_name, COL(56), -3, "-."), // illegal delete
                              "SAI 'HELIX_NR': You tried to delete '4' at position 39  -> Operation aborted");
            ta.close("xxx");
        }
        {
            GB_transaction ta(gb_main);
            TEST_ASSERT_EQUAL(GBT_insert_character(gb_main, ali_name, 4711, 3, "-."), // illegal insert
                              "Can't insert at position 4711 (exceeds length 67 of alignment 'ali_mini')");
            ta.close("xxx");
        }
        {
            GB_transaction ta(gb_main);
            TEST_ASSERT_EQUAL(GBT_insert_character(gb_main, ali_name, -1, 3, "-."), // illegal insert
                              "Illegal sequence position -1");
            ta.close("xxx");
        }
    }
    if (!error) {
        GB_transaction ta(gb_main);
        TEST_DATA(".....G---G--G--C---C-G.AGGGGAA-CCU--G---CG--G--C-UGG-ACCUCC........",
                  "-----A---C--G--A---U-C-CGGGGAA-CCU--G---CG--G--C-UGG-ACCUCCU.......",
                  ".....A---C--G--A---A-C.GGGGGAA-CCU--G---CG--G--C-UGG-ACCUCCU------.",
                  "-----U---G--C--C---U-G-GCGGCCU-UAG--C---GC--G--G-UGG-CACCUGA.......",
                  ".........[--<--[.......[[..[<<.[....]...>>--]......]...].>........]",
                  ".........1.....1.......22..34..34...3--4....3--4...2...1..........1", 
                  ".........x........x....x.x.x....x...x..........x...x...x...........x", 
                  HELIX_STRUCT);
    }

    GB_close(gb_main);
    TEST_ASSERT_NO_ERROR(error.deliver());
}

#endif // UNIT_TESTS
