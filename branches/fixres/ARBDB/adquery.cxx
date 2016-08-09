// =============================================================== //
//                                                                 //
//   File      : adquery.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_aci.h"
#include "gb_comm.h"
#include "gb_index.h"
#include "gb_key.h"
#include "gb_localdata.h"
#include "gb_ta.h"
#include <algorithm>

#include <arb_strbuf.h>
#include <arb_match.h>

#include <cctype>

#define GB_PATH_MAX 1024

static void build_GBDATA_path(GBDATA *gbd, char **buffer) {
    GBCONTAINER *gbc = GB_FATHER(gbd);
    if (gbc) {
        build_GBDATA_path(gbc, buffer);

        const char *key = GB_KEY(gbd);
        char       *bp  = *buffer;

        *bp++ = '/';
        while (*key) *bp++ = *key++;
        *bp = 0;

        *buffer = bp;
    }
}

#define BUFFERSIZE 1024

static const char *GB_get_GBDATA_path(GBDATA *gbd) {
    static char *orgbuffer = NULL;
    char        *buffer;

    if (!orgbuffer) orgbuffer = (char*)ARB_alloc(BUFFERSIZE);
    buffer                    = orgbuffer;

    build_GBDATA_path(gbd, &buffer);
    assert_or_exit((buffer-orgbuffer) < BUFFERSIZE); // buffer overflow

    return orgbuffer;
}

// ----------------
//      QUERIES

static bool gb_find_value_equal(GBDATA *gb, GB_TYPES type, const char *val, GB_CASE case_sens) {
    bool equal = false;

#if defined(DEBUG)
    GB_TYPES realtype = gb->type();
    gb_assert(val);
    if (type == GB_STRING) {
        gb_assert(gb->is_a_string()); // gb_find_internal called with wrong type
    }
    else {
        gb_assert(realtype == type); // gb_find_internal called with wrong type
    }
#endif // DEBUG

    switch (type) {
        case GB_STRING:
        case GB_LINK:
            equal = GBS_string_matches(GB_read_char_pntr(gb), val, case_sens);
            break;

        case GB_INT: {
            int i                      = GB_read_int(gb);
            if (i == *(int*)val) equal = true;
            break;
        }
        default: {
            const char *err = GBS_global_string("Value search not supported for data type %i (%i)", gb->type(), type);
            GB_internal_error(err);
            break;
        }
    }

    return equal;
}

static GBDATA *find_sub_by_quark(GBCONTAINER *father, GBQUARK key_quark, GB_TYPES type, const char *val, GB_CASE case_sens, GBDATA *after, size_t skip_over) {
    /* search an entry with a key 'key_quark' below a container 'father'
       after position 'after'

       if 'skip_over' > 0 search skips 'skip_over' entries

       if (val != NULL) search for entry with value 'val':

       GB_STRING/GB_LINK: compares string (case_sensitive or not)
       GB_INT: compares values
       GB_FLOAT: ditto (val MUST be a 'double*')
       others: not implemented yet

       Note: to search for non-char*-values use GB_find_int()
             for other types write a new similar function

             if key_quark<0 search everything
    */

    int             end    = father->d.nheader;
    gb_header_list *header = GB_DATA_LIST_HEADER(father->d);

    int index;
    if (after) index = (int)after->index+1; else index = 0;

    if (key_quark<0) { // unspecific key quark (i.e. search all)
        gb_assert(!val);        // search for val not possible if searching all keys!
        if (!val) {
            for (; index < end; index++) {
                if (header[index].flags.key_quark != 0) {
                    if (header[index].flags.changed >= GB_DELETED) continue;
                    GBDATA *gb = GB_HEADER_LIST_GBD(header[index]);
                    if (!gb) {
                        // @@@ DRY here versus below
                        gb_unfold(father, 0, index);
                        header = GB_DATA_LIST_HEADER(father->d);
                        gb     = GB_HEADER_LIST_GBD(header[index]);
                        if (!gb) {
                            const char *err = GBS_global_string("Database entry #%u is missing (in '%s')", index, GB_get_GBDATA_path(father));
                            GB_internal_error(err);
                            continue;
                        }
                    }
                    if (!skip_over--) return gb;
                }
            }
        }
    }
    else { // specific key quark
        for (; index < end; index++) {
            if (key_quark == header[index].flags.key_quark) {
                if (header[index].flags.changed >= GB_DELETED) continue;
                GBDATA *gb = GB_HEADER_LIST_GBD(header[index]);
                if (!gb) {
                    // @@@ DRY here versus section above
                    gb_unfold(father, 0, index);
                    header = GB_DATA_LIST_HEADER(father->d);
                    gb     = GB_HEADER_LIST_GBD(header[index]);
                    if (!gb) {
                        const char *err = GBS_global_string("Database entry #%u is missing (in '%s')", index, GB_get_GBDATA_path(father));
                        GB_internal_error(err);
                        continue;
                    }
                }
                if (val) {
                    if (!gb) {
                        GB_internal_error("Cannot unfold data");
                        continue;
                    }
                    else {
                        if (!gb_find_value_equal(gb, type, val, case_sens)) continue;
                    }
                }
                if (!skip_over--) return gb;
            }
        }
    }
    return NULL;
}

GBDATA *GB_find_sub_by_quark(GBDATA *father, GBQUARK key_quark, GBDATA *after, size_t skip_over) {
    return find_sub_by_quark(father->expect_container(), key_quark, GB_NONE, NULL, GB_MIND_CASE, after, skip_over);
}

static GBDATA *GB_find_subcontent_by_quark(GBDATA *father, GBQUARK key_quark, GB_TYPES type, const char *val, GB_CASE case_sens, GBDATA *after, size_t skip_over) {
    return find_sub_by_quark(father->expect_container(), key_quark, type, val, case_sens, after, skip_over);
}

static GBDATA *find_sub_sub_by_quark(GBCONTAINER *const father, const char *key, GBQUARK sub_key_quark, GB_TYPES type, const char *val, GB_CASE case_sens, GBDATA *after) {
    gb_index_files *ifs    = NULL;
    GB_MAIN_TYPE   *Main   = GBCONTAINER_MAIN(father);
    int             end    = father->d.nheader;
    gb_header_list *header = GB_DATA_LIST_HEADER(father->d);

    int index;
    if (after) index = (int)after->index+1; else index = 0;

    GBDATA *res;
    // ****** look for any hash index tables ********
    // ****** no wildcards allowed       *******
    if (Main->is_client()) {
        if (father->flags2.folded_container) {
            // do the query in the server
            if (GB_ARRAY_FLAGS(father).changed) {
                if (!father->flags2.update_in_server) {
                    GB_ERROR error = Main->send_update_to_server(father);
                    if (error) {
                        GB_export_error(error);
                        return NULL;
                    }
                }
            }
        }
        if (father->d.size > GB_MAX_LOCAL_SEARCH && val) {
            if (after) res = GBCMC_find(after,  key, type, val, case_sens, SEARCH_CHILD_OF_NEXT);
            else       res = GBCMC_find(father, key, type, val, case_sens, SEARCH_GRANDCHILD);
            return res;
        }
    }
    if (val &&
        (ifs=GBCONTAINER_IFS(father))!=NULL &&
        (!strchr(val, '*')) &&
        (!strchr(val, '?')))
    {
        for (; ifs; ifs = GB_INDEX_FILES_NEXT(ifs)) {
            if (ifs->key != sub_key_quark) continue;
            // ***** We found the index table *****
            res = gb_index_find(father, ifs, sub_key_quark, val, case_sens, index);
            return res;
        }
    }

    GBDATA *gb = after ? after : NULL;
    for (; index < end; index++) {
        GBDATA *gbn = GB_HEADER_LIST_GBD(header[index]);

        if (header[index].flags.changed >= GB_DELETED) continue;
        if (!gbn) {
            if (Main->is_client()) {
                if (gb) res = GBCMC_find(gb,     key, type, val, case_sens, SEARCH_CHILD_OF_NEXT);
                else    res = GBCMC_find(father, key, type, val, case_sens, SEARCH_GRANDCHILD);
                return res;
            }
            GB_internal_error("Empty item in server");
            continue;
        }
        gb = gbn;
        if (gb->is_container()) {
            res = GB_find_subcontent_by_quark(gb, sub_key_quark, type, val, case_sens, NULL, 0);
            if (res) return res;
        }
    }
    return NULL;
}


static GBDATA *gb_find_internal(GBDATA *gbd, const char *key, GB_TYPES type, const char *val, GB_CASE case_sens, GB_SEARCH_TYPE gbs) {
    GBDATA *result = NULL;

    if (gbd) {
        GBDATA      *after = NULL;
        GBCONTAINER *gbc   = NULL;

        switch (gbs) {
            case SEARCH_NEXT_BROTHER:
                after = gbd;
            case SEARCH_BROTHER:
                gbs   = SEARCH_CHILD;
                gbc   = GB_FATHER(gbd);
                break;

            case SEARCH_CHILD:
            case SEARCH_GRANDCHILD:
                if (gbd->is_container()) gbc = gbd->as_container();
                break;

            case SEARCH_CHILD_OF_NEXT:
                after = gbd;
                gbs   = SEARCH_GRANDCHILD;
                gbc   = GB_FATHER(gbd);
                break;
        }

        if (gbc) {
            GBQUARK key_quark = GB_find_existing_quark(gbd, key);

            if (key_quark) { // only search if 'key' is known to db
                if (gbs == SEARCH_CHILD) {
                    result = GB_find_subcontent_by_quark(gbc, key_quark, type, val, case_sens, after, 0);
                }
                else {
                    gb_assert(gbs == SEARCH_GRANDCHILD);
                    result = find_sub_sub_by_quark(gbc, key, key_quark, type, val, case_sens, after);
                }
            }
        }
    }
    return result;
}

GBDATA *GB_find(GBDATA *gbd, const char *key, GB_SEARCH_TYPE gbs) {
    // normally you should not need to use GB_find!
    // better use one of the replacement functions
    // (GB_find_string, GB_find_int, GB_child, GB_nextChild, GB_entry, GB_nextEntry, GB_brother)
    return gb_find_internal(gbd, key, GB_NONE, NULL, GB_CASE_UNDEFINED, gbs);
}

GBDATA *GB_find_string(GBDATA *gbd, const char *key, const char *str, GB_CASE case_sens, GB_SEARCH_TYPE gbs) {
    // search for a subentry of 'gbd' that has
    // - fieldname 'key'
    // - type GB_STRING and
    // - content matching 'str'
    // if 'case_sensitive' is true, content is matched case sensitive.
    // GBS_string_matches is used to compare (supports wildcards)
    return gb_find_internal(gbd, key, GB_STRING, str, case_sens, gbs);
}
NOT4PERL GBDATA *GB_find_int(GBDATA *gbd, const char *key, long val, GB_SEARCH_TYPE gbs) {
    // search for a subentry of 'gbd' that has
    // - fieldname 'key'
    // - type GB_INT
    // - and value 'val'
    return gb_find_internal(gbd, key, GB_INT, (const char *)&val, GB_CASE_UNDEFINED, gbs);
}

// ----------------------------------------------------
//      iterate over ALL subentries of a container

GBDATA *GB_child(GBDATA *father) {
    // return first child (or NULL if no children)
    return GB_find(father, NULL, SEARCH_CHILD);
}
GBDATA *GB_nextChild(GBDATA *child) {
    // return next child after 'child' (or NULL if no more children)
    return GB_find(child, NULL, SEARCH_NEXT_BROTHER);
}

// ------------------------------------------------------------------------------
//      iterate over all subentries of a container that have a specified key

GBDATA *GB_entry(GBDATA *father, const char *key) { 
    // return first child of 'father' that has fieldname 'key'
    // (or NULL if none found)
    return GB_find(father, key, SEARCH_CHILD);
}
GBDATA *GB_nextEntry(GBDATA *entry) {
    // return next child after 'entry', that has the same fieldname
    // (or NULL if 'entry' is last one)
    return GB_find_sub_by_quark(GB_FATHER(entry), GB_get_quark(entry), entry, 0);
}
GBDATA *GB_followingEntry(GBDATA *entry, size_t skip_over) {
    // return following child after 'entry', that has the same fieldname
    // (or NULL if no such entry)
    // skips 'skip_over' entries (skip_over == 0 behaves like GB_nextEntry)
    return GB_find_sub_by_quark(GB_FATHER(entry), GB_get_quark(entry), entry, skip_over);
}

GBDATA *GB_brother(GBDATA *entry, const char *key) {
    // searches (first) brother (before or after) of 'entry' which has field 'key'
    // i.e. does same as GB_entry(GB_get_father(entry), key)
    return GB_find(entry, key, SEARCH_BROTHER);
}

GBDATA *gb_find_by_nr(GBCONTAINER *father, int index) {
    /* get a subentry by its internal number:
       Warning: This subentry must exists, otherwise internal error */

    gb_header_list *header = GB_DATA_LIST_HEADER(father->d);
    if (index >= father->d.nheader || index <0) {
        GB_internal_errorf("Index '%i' out of range [%i:%i[", index, 0, father->d.nheader);
        return NULL;
    }
    if (header[index].flags.changed >= GB_DELETED || !header[index].flags.key_quark) {
        GB_internal_error("Entry already deleted");
        return NULL;
    }

    GBDATA *gb = GB_HEADER_LIST_GBD(header[index]);
    if (!gb) {
        gb_unfold(father, 0, index);
        header = GB_DATA_LIST_HEADER(father->d);
        gb = GB_HEADER_LIST_GBD(header[index]);
        if (!gb) {
            GB_internal_error("Could not unfold data");
            return NULL;
        }
    }
    return gb;
}

class keychar_table {
    bool table[256];
public:
    keychar_table() {
        for (int i=0; i<256; i++) {
            table[i] = islower(i) || isupper(i) || isdigit(i) || i=='_' || i=='@';
        }
    }
    const char *first_non_key_character(const char *str) const {
        while (1) {
            int c = *str;
            if (!table[c]) {
                if (c == 0) break;
                return str;
            }
            str++;
        }
        return NULL;
    }
};
static keychar_table keychars;

const char *GB_first_non_key_char(const char *str) {
    return keychars.first_non_key_character(str);
}

inline GBDATA *find_or_create(GBCONTAINER *gb_parent, const char *key, GB_TYPES create, bool internflag) {
    gb_assert(!keychars.first_non_key_character(key));

    GBDATA *gbd = GB_entry(gb_parent, key);
    if (create) {
        if (gbd) {
            GB_TYPES oldType = gbd->type();
            if (create != oldType) { // type mismatch
                GB_export_errorf("Inconsistent type for field '%s' (existing=%i, expected=%i)", key, oldType, create);
                gbd = NULL;
            }
        }
        else {
            if (create == GB_CREATE_CONTAINER) {
                gbd = internflag ? gb_create_container(gb_parent, key) : GB_create_container(gb_parent, key);
            }
            else {
                gbd = gb_create(gb_parent, key, create);
            }
            gb_assert(gbd || GB_have_error());
        }
    }
    return gbd;
}

GBDATA *gb_search(GBCONTAINER *gbc, const char *key, GB_TYPES create, int internflag) {
    /* finds a hierarchical key,
     * if create != GB_FIND(==0), then create the key
     * force types if ! internflag
     */

    gb_assert(!GB_have_error()); // illegal to enter this function when an error is exported!

    GB_test_transaction(gbc);

    if (!key) return NULL; // was allowed in the past (and returned the 1st child). now returns NULL
    
    if (key[0] == '/') {
        gbc = gb_get_root(gbc);
        key++;
    }

    if (!key[0]) {
        return gbc;
    }

    GBDATA     *gb_result     = NULL;
    const char *separator     = keychars.first_non_key_character(key);
    if (!separator) gb_result = find_or_create(gbc, key, create, internflag);
    else {
        int  len = separator-key;
        char firstKey[len+1];
        memcpy(firstKey, key, len);
        firstKey[len] = 0;

        char invalid_char = 0;

        switch (separator[0]) {
            case '/': {
                GBDATA *gb_sub = find_or_create(gbc, firstKey, create ? GB_CREATE_CONTAINER : GB_FIND, internflag);
                if (gb_sub) {
                    if (gb_sub->is_container()) {
                        if (separator[1] == '/') {
                            GB_export_errorf("Invalid '//' in key '%s'", key);
                        }
                        else {
                            gb_result = gb_search(gb_sub->as_container(), separator+1, create, internflag);
                        }
                    }
                    else {
                        GB_export_errorf("terminal entry '%s' cannot be used as container", firstKey);
                    }
                }
                break;
            }
            case '.': {
                if (separator[1] != '.') invalid_char = separator[0];
                else {
                    GBCONTAINER *gb_parent = gbc->get_father();
                    if (gb_parent) {
                        switch (separator[2]) {
                            case 0:   gb_result    = gb_parent; break;
                            case '/': gb_result    = gb_search(gb_parent, separator+3, create, internflag); break;
                            default:
                                GB_export_errorf("Expected '/' after '..' in key '%s'", key);
                                break;
                        }
                    }
                    else { // ".." at root-node
                        if (create) {
                            GB_export_error("cannot use '..' at root node");
                        }
                    }
                }
                break;
            }
            case '-':
                if (separator[1] != '>') invalid_char = separator[0];
                else {
                    if (firstKey[0]) {
                        GBDATA *gb_link = GB_entry(gbc, firstKey);
                        if (!gb_link) {
                            if (create) {
                                GB_export_error("Cannot create links on the fly in gb_search");
                            }
                        }
                        else {
                            if (gb_link->type() == GB_LINK) {
                                GBDATA *gb_target = GB_follow_link(gb_link);
                                if (!gb_target) GB_export_errorf("Link '%s' points nowhere", firstKey);
                                else    gb_result = gb_search(gb_target->as_container(), separator+2, create, internflag);
                            }
                            else GB_export_errorf("'%s' exists, but is not a link", firstKey);
                        }
                    }
                    else GB_export_errorf("Missing linkname before '->' in '%s'", key);
                }
                break;

            default:
                invalid_char = separator[0];
                break;
        }

        if (invalid_char) {
            gb_assert(!gb_result);
            GB_export_errorf("Invalid char '%c' in key '%s'", invalid_char, key);
        }
    }
    gb_assert(!(gb_result && GB_have_error()));
    return gb_result;
}


GBDATA *GB_search(GBDATA *gbd, const char *fieldpath, GB_TYPES create) {
    return gb_search(gbd->expect_container(), fieldpath, create, 0);
}

static GBDATA *gb_expect_type(GBDATA *gbd, GB_TYPES expected_type, const char *fieldname) {
    gb_assert(expected_type != GB_FIND); // impossible

    GB_TYPES type = gbd->type();
    if (type != expected_type) {
        GB_export_errorf("Field '%s' has wrong type (found=%i, expected=%i)", fieldname, type, expected_type);
        gbd = 0;
    }
    return gbd;
}

GBDATA *GB_searchOrCreate_string(GBDATA *gb_container, const char *fieldpath, const char *default_value) {
    gb_assert(!GB_have_error()); // illegal to enter this function when an error is exported!

    GBDATA *gb_str = GB_search(gb_container, fieldpath, GB_FIND);
    if (!gb_str) {
        GB_clear_error();
        gb_str = GB_search(gb_container, fieldpath, GB_STRING);
        GB_ERROR error;

        if (!gb_str) error = GB_await_error();
        else error         = GB_write_string(gb_str, default_value);

        if (error) {
            gb_str = 0;
            GB_export_error(error);
        }
    }
    else {
        gb_str = gb_expect_type(gb_str, GB_STRING, fieldpath);
    }
    return gb_str;
}

GBDATA *GB_searchOrCreate_int(GBDATA *gb_container, const char *fieldpath, long default_value) {
    gb_assert(!GB_have_error()); // illegal to enter this function when an error is exported!

    GBDATA *gb_int = GB_search(gb_container, fieldpath, GB_FIND);
    if (!gb_int) {
        gb_int = GB_search(gb_container, fieldpath, GB_INT);
        GB_ERROR error;

        if (!gb_int) error = GB_await_error();
        else error         = GB_write_int(gb_int, default_value);

        if (error) { // @@@ in case GB_search returned 0, gb_int already is 0 and error is exported. just assert error is exported
            gb_int = 0;
            GB_export_error(error);
        }
    }
    else {
        gb_int = gb_expect_type(gb_int, GB_INT, fieldpath);
    }
    return gb_int;
}

GBDATA *GB_searchOrCreate_float(GBDATA *gb_container, const char *fieldpath, float default_value) {
    gb_assert(!GB_have_error()); // illegal to enter this function when an error is exported!

    GBDATA *gb_float = GB_search(gb_container, fieldpath, GB_FIND);
    if (!gb_float) {
        gb_float = GB_search(gb_container, fieldpath, GB_FLOAT);
        GB_ERROR error;

        if (!gb_float) error = GB_await_error();
        else error           = GB_write_float(gb_float, default_value);

        if (error) {
            gb_float = 0;
            GB_export_error(error);
        }
    }
    else {
        gb_float = gb_expect_type(gb_float, GB_FLOAT, fieldpath);
    }
    return gb_float;
}

static GBDATA *gb_search_marked(GBCONTAINER *gbc, GBQUARK key_quark, int firstindex, size_t skip_over) {
    int             userbit = GBCONTAINER_MAIN(gbc)->users[0]->userbit;
    int             index;
    int             end     = gbc->d.nheader;
    gb_header_list *header  = GB_DATA_LIST_HEADER(gbc->d);

    for (index = firstindex; index<end; index++) {
        GBDATA *gb;

        if (! (userbit & header[index].flags.flags)) continue;
        if ((key_quark>=0) && (header[index].flags.key_quark  != key_quark)) continue;
        if (header[index].flags.changed >= GB_DELETED) continue;
        if ((gb=GB_HEADER_LIST_GBD(header[index]))==NULL) {
            gb_unfold(gbc, 0, index);
            header = GB_DATA_LIST_HEADER(gbc->d);
            gb = GB_HEADER_LIST_GBD(header[index]);
        }
        if (!skip_over--) return gb;
    }
    return NULL;
}

long GB_number_of_marked_subentries(GBDATA *gbd) {
    long count = 0;
    if (gbd->is_container()) {
        GBCONTAINER    *gbc    = gbd->as_container();
        gb_header_list *header = GB_DATA_LIST_HEADER(gbc->d);

        int userbit = GBCONTAINER_MAIN(gbc)->users[0]->userbit;
        int end     = gbc->d.nheader;

        for (int index = 0; index<end; index++) {
            if (!(userbit & header[index].flags.flags)) continue;
            if (header[index].flags.changed >= GB_DELETED) continue;
            count++;
        }
    }
    return count;
}



GBDATA *GB_first_marked(GBDATA *gbd, const char *keystring) {
    GBCONTAINER *gbc       = gbd->expect_container();
    GBQUARK      key_quark = GB_find_existing_quark(gbd, keystring);
    GB_test_transaction(gbc);
    return key_quark ? gb_search_marked(gbc, key_quark, 0, 0) : NULL;
}


GBDATA *GB_following_marked(GBDATA *gbd, const char *keystring, size_t skip_over) {
    GBCONTAINER *gbc       = GB_FATHER(gbd);
    GBQUARK      key_quark = GB_find_existing_quark(gbd, keystring);
    GB_test_transaction(gbc);
    return key_quark ? gb_search_marked(gbc, key_quark, (int)gbd->index+1, skip_over) : NULL;
}

GBDATA *GB_next_marked(GBDATA *gbd, const char *keystring) {
    return GB_following_marked(gbd, keystring, 0);
}

// ----------------------------
//      Command interpreter

void gb_install_command_table(GBDATA *gb_main, struct GBL_command_table *table, size_t table_size) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    if (!Main->command_hash) Main->command_hash = GBS_create_hash(table_size, GB_IGNORE_CASE);

    for (; table->command_identifier; table++) {
        GBS_write_hash(Main->command_hash, table->command_identifier, (long)table->function);
    }

    gb_assert((GBS_hash_elements(Main->command_hash)+1) == table_size);
}

static char *gbs_search_second_x(const char *str) {
    int c;
    for (; (c=*str); str++) {
        if (c=='\\') {      // escaped characters
            str++;
            if (!(c=*str)) return NULL;
            continue;
        }
        if (c=='"') return (char *)str;
    }
    return NULL;
}

char *gbs_search_second_bracket(const char *source) {
    int c;
    int deep = 0;
    if (*source != '(') deep --;    // first bracket
    for (; (c=*source); source++) {
        if (c=='\\') {      // escaped characters
            source++;
            if (!*source) break;
            continue;
        }
        if (c=='(') deep--;
        else if (c==')') deep++;
        if (!deep) return (char *)source;
        if (c=='"') {       // search the second "
            source = gbs_search_second_x(source);
            if (!source) return NULL;
        }
    }
    if (!c) return NULL;
    return (char *)source;
}


static char *gbs_search_next_separator(const char *source, const char *seps) {
    // search the next separator
    static char tab[256];
    static int flag = 0;
    int c;
    const char *p;
    if (!flag) {
        flag = 1;
        memset(tab, 0, 256);
    }
    for (p = seps; (c=*p); p++) tab[c] = 1; // tab[seps[x]] = 1
    tab['('] = 1;               // exclude () pairs
    tab['"'] = 1;               // exclude " pairs
    tab['\\'] = 1;              // exclude \-escaped chars

    for (; (c=*source); source++) {
        if (tab[c]) {
            if (c=='\\') {
                source++;
                continue;
            }
            if (c=='(') {
                source = gbs_search_second_bracket(source);
                if (!source) break;
                continue;
            }
            if (c=='"') {
                source = gbs_search_second_x(source+1);
                if (!source) break;
                continue;
            }
            for (p = seps; (c=*p); p++) tab[c] = 0;
            return (char *)source;
        }
    }
    for (p = seps; (c=*p); p++) tab[c] = 0; // clear tab
    return NULL;
}

static void dumpStreams(const char *name, const GBL_streams& streams) {
    int count = streams.size();
    printf("%s=%i\n", name, count);
    if (count > 0) {
        for (int c = 0; c<count; c++) {
            printf("  %02i='%s'\n", c, streams.get(c));
        }
    }
}

static const char *shortenLongString(const char *str, size_t wanted_len) {
    // shortens the string 'str' to 'wanted_len' (appends '[..]' if string was shortened)

    const char *result;
    size_t      len = strlen(str);

    gb_assert(wanted_len>4);

    if (len>wanted_len) {
        static char   *shortened_str;
        static size_t  short_len = 0;

        if (short_len >= wanted_len) {
            memcpy(shortened_str, str, wanted_len-4);
        }
        else {
            freeset(shortened_str, ARB_strpartdup(str, str+wanted_len));
            short_len = wanted_len;
        }
        strcpy(shortened_str+wanted_len-4, "[..]");
        result = shortened_str;
    }
    else {
        result = str;
    }
    return result;
}

static char *apply_ACI(GBDATA *gb_main, const char *commands, const char *str, GBDATA *gbd, const char *default_tree_name) {
    int trace = GB_get_ACISRT_trace();

    GB_MAIN_TYPE *Main    = GB_MAIN(gb_main);
    gb_local->gbl.gb_main = gb_main;

    char *buffer = ARB_strdup(commands);

    // ********************** remove all spaces and tabs *******************
    {
        const char *s1;
        char *s2;
        s1 = commands;
        s2 = buffer;
        {
            int c;
            for (; (c = *s1); s1++) {
                if (c=='\\') {
                    *(s2++) = c;
                    if (!(c=*++s1)) { break; }
                    *(s2++) = c;
                    continue;
                }

                if (c=='"') {       // search the second "
                    const char *hp = gbs_search_second_x(s1+1);
                    if (!hp) {
                        GB_export_errorf("unbalanced '\"' in '%s'", commands);
                        free(buffer);
                        return NULL;
                    }
                    while (s1 <= hp) *(s2++) = *(s1++);
                    s1--;
                    continue;
                }
                if (c!=' ' && c!='\t') *(s2++) = c;
            }
        }
        *s2 = 0;
    }

    GBL_streams orig;

    orig.insert(ARB_strdup(str));

    GB_ERROR error = NULL;
    GBL_streams out;
    {
        char *s1, *s2;
        s1 = buffer;
        if (*s1 == '|') s1++;

        // ** loop over all commands **
        for (; s1;  s1 = s2) {
            int separator;
            GBL_COMMAND command;
            s2 = gbs_search_next_separator(s1, "|;,");
            if (s2) {
                separator = *(s2);
                *(s2++) = 0;
            }
            else {
                separator = 0;
            }
            // collect the parameters
            GBL_streams in;
            if (*s1 == '"') {           // copy "text" to out
                char *end = gbs_search_second_x(s1+1);
                if (!end) {
                    UNCOVERED(); // seems unreachable (balancing is already ensured by gbs_search_next_separator)
                    error = "Missing second '\"'";
                    break;
                }
                *end = 0;

                out.insert(ARB_strdup(s1+1));
            }
            else {
                char *bracket   = strchr(s1, '(');

                if (bracket) {      // I got the parameter list
                    int slen;
                    *(bracket++) = 0;
                    slen  = strlen(bracket);
                    if (bracket[slen-1] != ')') {
                        error = "Missing ')'";
                    }
                    else {
                        // go through the parameters
                        char *p1, *p2;
                        bracket[slen-1] = 0;
                        for (p1 = bracket; p1;  p1 = p2) {
                            p2 = gbs_search_next_separator(p1, ";,");
                            if (p2) {
                                *(p2++) = 0;
                            }
                            if (p1[0] == '"') { // remove "" pairs
                                int len2;
                                p1++;
                                len2 = strlen(p1)-1;

                                if (p1[len2] != '\"') {
                                    error = "Missing '\"'";
                                }
                                else {
                                    p1[len2] = 0;
                                }
                            }
                            in.insert(ARB_strdup(p1));
                        }
                    }
                }
                if (!error && (bracket || *s1)) {
                    char *p = s1;
                    int c;
                    while ((c = *p)) {          // command to lower case
                        if (c>='A' && c<='Z') {
                            c += 'a'-'A';
                            *p = c;
                        }
                        p++;
                    }

                    command = (GBL_COMMAND)GBS_read_hash(Main->command_hash, s1);
                    if (!command) {
                        error = GBS_global_string("Unknown command '%s'", s1);
                    }
                    else {
                        GBL_command_arguments args(gbd, default_tree_name, orig, in, out);

                        args.command = s1;

                        if (trace) {
                            printf("-----------------------\nExecution of command '%s':\n", args.command);
                            dumpStreams("Arguments", args.param);
                            dumpStreams("InputStreams", args.input);
                        }

                        error = command(&args); // execute the command

                        if (!error && trace) dumpStreams("OutputStreams", args.output);

                        if (error) {
                            char *dup_error = ARB_strdup(error);

#define MAX_PRINT_LEN 200

                            char *paramlist = 0;
                            for (int j = 0; j<args.param.size(); ++j) {
                                const char *param       = args.param.get(j);
                                const char *param_short = shortenLongString(param, MAX_PRINT_LEN);

                                if (!paramlist) paramlist = ARB_strdup(param_short);
                                else freeset(paramlist, GBS_global_string_copy("%s,%s", paramlist, param_short));
                            }
                            char *inputstreams = 0;
                            for (int j = 0; j<args.input.size(); ++j) {
                                const char *input       = args.input.get(j);
                                const char *input_short = shortenLongString(input, MAX_PRINT_LEN);

                                if (!inputstreams) inputstreams = ARB_strdup(input_short);
                                else freeset(inputstreams, GBS_global_string_copy("%s;%s", inputstreams, input_short));
                            }
#undef MAX_PRINT_LEN
                            if (paramlist) {
                                error = GBS_global_string("while applying '%s(%s)'\nto '%s':\n%s", s1, paramlist, inputstreams, dup_error);
                            }
                            else {
                                error = GBS_global_string("while applying '%s'\nto '%s':\n%s", s1, inputstreams, dup_error);
                            }

                            free(inputstreams);
                            free(paramlist);
                            free(dup_error);
                        }
                    }
                }
            }

            if (error) break;

            if (separator == '|') { // out -> in pipe; clear in
                out.swap(orig);
                out.erase();
            }
        }
    }

    {
        char *s1 = out.concatenated();
        free(buffer);

        if (!error) {
            if (trace) printf("GB_command_interpreter: result='%s'\n", s1);
            return s1;
        }
        free(s1);
    }

    GB_export_errorf("Command '%s' failed:\nReason: %s", commands, error);
    return NULL;
}
// --------------------------------------------------------------------------------

char *GBL_streams::concatenated() const {
    int count = size();
    if (!count) return ARB_strdup("");
    if (count == 1) return ARB_strdup(get(0));

    GBS_strstruct *strstruct = GBS_stropen(1000);
    for (int i=0; i<count; i++) {
        const char *s = get(i);
        if (s) GBS_strcat(strstruct, s);
    }
    return GBS_strclose(strstruct);
}

char *GB_command_interpreter(GBDATA *gb_main, const char *str, const char *commands, GBDATA *gbd, const char *default_tree_name) {
    /* simple command interpreter returns NULL on error (which should be exported in that case)
     * if first character is == ':' run string parser
     * if first character is == '/' run regexpr
     * else run ACI
     */

    int trace = GB_get_ACISRT_trace();
    SmartMallocPtr(char) heapstr;

    if (!str) {
        if (!gbd) {
            GB_export_error("ACI: no input streams found");
            return NULL;
        }

        if (GB_read_type(gbd) == GB_STRING) {
            str = GB_read_char_pntr(gbd);
        }
        else {
            char *asstr = GB_read_as_string(gbd);
            if (!asstr) {
                GB_export_error("Can't read this DB entry as string");
                return NULL;
            }

            heapstr = asstr;
            str     = &*heapstr;
        }
    }

    if (trace) {
        printf("GB_command_interpreter: str='%s'\n"
               "                        command='%s'\n", str, commands);
    }

    if (!commands || !commands[0]) { // empty command -> do not modify string
        return ARB_strdup(str);
    }

    if (commands[0] == ':') { // ':' -> string parser
        return GBS_string_eval(str, commands+1, gbd);
    }

    if (commands[0] == '/') { // regular expression
        GB_ERROR  err    = 0;
        char     *result = GBS_regreplace(str, commands, &err);

        if (!result) {
            if (strcmp(err, "Missing '/' between search and replace string") == 0) {
                // if GBS_regreplace didn't find a third '/' -> silently use GBS_regmatch:
                size_t matchlen;
                err    = 0;
                const char *matched = GBS_regmatch(str, commands, &matchlen, &err);

                if (matched) result   = ARB_strndup(matched, matchlen);
                else if (!err) result = ARB_strdup("");
            }

            if (!result && err) result = GBS_global_string_copy("<Error: %s>", err);
        }
        return result;
    }

    return apply_ACI(gb_main, commands, str, gbd, default_tree_name);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

#define TEST_CI__INTERNAL(input,cmd,expected,got,TEST_RESULT) do {                                                      \
        char *result;                                                                                                   \
        TEST_EXPECT_RESULT__NOERROREXPORTED(result = GB_command_interpreter(gb_main, input, cmd, gb_data, NULL));       \
        TEST_RESULT(result,expected,got);                                                                               \
        free(result);                                                                                                   \
    } while(0)

#define TEST_CI(input,cmd,expected)              TEST_CI__INTERNAL(input,cmd,expected,narg, TEST_EXPECT_EQUAL__IGNARG)
#define TEST_CI__BROKEN(input,cmd,expected,regr) TEST_CI__INTERNAL(input,cmd,expected,regr, TEST_EXPECT_EQUAL__BROKEN)
#define TEST_CI_NOOP(inandout,cmd)               TEST_CI__INTERNAL(inandout,cmd,inandout,narg,TEST_EXPECT_EQUAL__IGNARG)
#define TEST_CI_NOOP__BROKEN(inandout,regr,cmd)  TEST_CI__INTERNAL(inandout,cmd,inandout,regr,TEST_EXPECT_EQUAL__BROKEN)

#define TEST_CI_INVERSE(in,cmd,inv_cmd,out) do {        \
        TEST_CI(in,  cmd,     out);                     \
        TEST_CI(out, inv_cmd, in);                      \
    } while(0)

#define TEST_CI_ERROR_CONTAINS(input,cmd,errorpart_expected) \
    TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_command_interpreter(gb_main, input, cmd, gb_data, NULL), errorpart_expected)

#define ACI_SPLIT          "|split(\",\",0)"
#define ACI_MERGE          "|merge(\",\")"
#define WITH_SPLITTED(aci) ACI_SPLIT aci ACI_MERGE

void TEST_GB_command_interpreter() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_nuc.arb", "rw"); // ../UNIT_TESTER/run/TEST_nuc.arb

    GBT_set_default_alignment(gb_main, "ali_16s"); // for sequence ACI command (@@@ really needed ? )

    int old_trace = GB_get_ACISRT_trace();
#if 0
    GB_set_ACISRT_trace(1); // used to detect coverage and for debugging purposes
#endif
    {
        GB_transaction  ta(gb_main);
        GBDATA         *gb_data = GBT_find_species(gb_main, "LcbReu40");

        TEST_CI_NOOP("bla", "");

        TEST_CI("bla", ":a=u", "blu"); // simple SRT

        TEST_CI("bla",    "/a/u/",   "blu"); // simple regExp replace
        TEST_CI("blabla", "/l.*b/",  "lab"); // simple regExp match
        TEST_CI("blabla", "/b.b/",   "");    // simple regExp match (failing)

        // escape / quote
        TEST_CI_INVERSE("ac", "|quote",        "|unquote",          "\"ac\"");
        TEST_CI_INVERSE("ac", "|escape",       "|unescape",         "ac");
        TEST_CI_INVERSE("ac", "|escape|quote", "|unquote|unescape", "\"ac\"");
        TEST_CI_INVERSE("ac", "|quote|escape", "|unescape|unquote", "\\\"ac\\\"");

        TEST_CI_INVERSE("a\"b\\c", "|quote",        "|unquote",          "\"a\"b\\c\"");
        TEST_CI_INVERSE("a\"b\\c", "|escape",       "|unescape",         "a\\\"b\\\\c");
        TEST_CI_INVERSE("a\"b\\c", "|escape|quote", "|unquote|unescape", "\"a\\\"b\\\\c\"");
        TEST_CI_INVERSE("a\"b\\c", "|quote|escape", "|unescape|unquote", "\\\"a\\\"b\\\\c\\\"");

        TEST_CI_NOOP("ac", "|unquote");
        TEST_CI_NOOP("\"ac", "|unquote");
        TEST_CI_NOOP("ac\"", "|unquote");

        TEST_CI("blabla", "|coUNT(ab)",         "4");   // simple ACI
        TEST_CI("l",      "|\"b\";dd;\"a\"|dd", "bla"); // ACI with muliple streams
        TEST_CI("bla",    "|count()",           "0");   // one empty parameter
        TEST_CI("bla",    "|count(\"\")",       "0");   // empty parameter
        TEST_CI("b a",    "|count(\" \")",      "1");   // space in quotes
        TEST_CI("b\\a",   "|count(\\a)",        "2");   // count '\\' and 'a' (ok)
        TEST_CI__BROKEN("b\\a",   "|count(\"\\a\")",    "1", "2"); // should only count 'a' (which is escaped in param)
        TEST_CI("b\\a",   "|count(\"\a\")",     "0");   // does not contain '\a'
        TEST_CI("b\a",    "|count(\"\a\")",     "1");   // counts '\a'

        // escaping (@@@ wrong behavior?)
        TEST_CI("b\\a",   "|count(\\a)",         "2"); // i would expect '1' as result (the 'a'), but it counts '\\' and 'a'
        TEST_CI("b\\a",   "|contains(\"\\\\\")", "0"); // searches for 2 backslashes, finds none
        TEST_CI__BROKEN("b\\a",   "|contains(\"\")",     "0", "1"); // search for nothing,              but reports 1 hit
        TEST_CI__BROKEN("b\\a",   "|contains(\\)",       "1", "2"); // searches for 1 backslash (ok),   but reports two hits instead of one
        TEST_CI__BROKEN("b\\\\a", "|contains(\"\\\\\")", "1", "2"); // searches for 2 backslashes (ok), but reports two hits instead of one
        TEST_CI_ERROR_CONTAINS("b\\a", "|contains(\"\\\")", "ARB ERROR: unbalanced '\"' in '|contains(\"\\\")'"); // raises error (should search for 1 backslash)

        // test binary ops
        TEST_CI("", "\"5\";\"7\"|minus",            "-2");
        TEST_CI("", "\"5\"|minus(\"7\")",           "-2");
        TEST_CI("", "|minus(\"\"5\"\", \"\"7\"\")", "-2"); // @@@ syntax needed here 'minus(""5"", ""7"")' is broken - need to unescape correctly after parsing parameters!


        TEST_CI_NOOP("ab,bcb,abac", WITH_SPLITTED(""));
        TEST_CI     ("ab,bcb,abac", WITH_SPLITTED("|len"),                       "2,3,4");
        TEST_CI     ("ab,bcb,abac", WITH_SPLITTED("|count(a)"),                  "1,0,2");
        TEST_CI     ("ab,bcb,abac", WITH_SPLITTED("|minus(len,count(a))"),       "1,3,2");
        TEST_CI     ("ab,bcb,abac", WITH_SPLITTED("|minus(\"\"5\"\",count(a))"), "4,5,3"); // @@@ escaping broken here as well

        // test other recursive uses of GB_command_interpreter
        TEST_CI("one",   "|dd;\"two\";dd|command(\"dd;\"_\";dd;\"-\"\")",                          "one_one-two_two-one_one-");
        TEST_CI("",      "|sequence|command(\"/^([\\\\.-]*)[A-Z].*/\\\\1/\")|len",                 "9"); // count gaps at start of sequence
        TEST_CI("one",   "|dd;dd|eval(\"\"up\";\"per\"|merge\")",                                  "ONEONE");
        TEST_CI("1,2,3", WITH_SPLITTED("|select(\"\",  \"\"one\"\", \"\"two\"\", \"\"three\"\")"), "one,two,three");
        TEST_CI_ERROR_CONTAINS("1,4", WITH_SPLITTED("|select(\"\",  \"\"one\"\", \"\"two\"\", \"\"three\"\")"), "Illegal param number '4' (allowed [0..3])");

        // test define and do (@@@ SLOW)
        TEST_CI("ignored", "define(d4, \"dd;dd;dd;dd\")",        "");
        TEST_CI("ignored", "define(d16, \"do(d4)|do(d4)\")",     "");
        TEST_CI("ignored", "define(d64, \"do(d4)|do(d16)\")",    "");
        TEST_CI("ignored", "define(d4096, \"do(d64)|do(d64)\")", "");

        TEST_CI("x",  "do(d4)",        "xxxx");
        TEST_CI("xy", "do(d4)",        "xyxyxyxy");
        TEST_CI("x",  "do(d16)",       "xxxxxxxxxxxxxxxx");
        TEST_CI("x",  "do(d64)|len",   "64");
        TEST_CI("xy", "do(d4)|len",    "8");
        TEST_CI("xy", "do(d4)|len()",  "8");
        TEST_CI("xy", "do(d4)|len(x)", "4");
        TEST_CI("x",  "do(d4096)|len", "4096");

        {
            int prev_trace = GB_get_ACISRT_trace();
            GB_set_ACISRT_trace(0); // do not trace here
            // create 4096 streams:
            TEST_CI("x",  "dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|streams", "4096");
            GB_set_ACISRT_trace(prev_trace);
        }

        // other commands

        // streams
        TEST_CI("x", "dd;dd|streams",             "2");
        TEST_CI("x", "dd;dd|dd;dd|streams",       "4");
        TEST_CI("x", "dd;dd|dd;dd|dd;dd|streams", "8");
        TEST_CI("x", "do(d4)|streams",            "1"); // stream is merged when do() returns

        TEST_CI("", "ali_name", "ali_16s");  // ask for default-alignment name
        TEST_CI("", "sequence_type", "rna"); // ask for sequence_type of default-alignment

        // format
        TEST_CI("acgt", "format", "          acgt");
        TEST_CI("acgt", "format(firsttab=1)", " acgt");
        TEST_CI("acgt", "format(firsttab=1, width=2)",
                " ac\n"
                "          gt");
        TEST_CI("acgt", "format(firsttab=1,tab=1,width=2)",
                " ac\n"
                " gt");
        TEST_CI("acgt", "format(firsttab=0,tab=0,width=2)",
                "ac\n"
                "gt");
        TEST_CI("acgt", "format(firsttab=0,tab=0,width=1)",
                "a\n"
                "c\n"
                "g\n"
                "t");

        TEST_CI_ERROR_CONTAINS("acgt", "format(gap=0)",   "Unknown Parameter 'gap=0' in command 'format'");
        TEST_CI_ERROR_CONTAINS("acgt", "format(numleft)", "Unknown Parameter 'numleft' in command 'format'");

        // format_sequence
        TEST_CI_ERROR_CONTAINS("acgt", "format_sequence(numright=5, numleft)", "You may only specify 'numleft' OR 'numright',  not both");

        TEST_CI("acgtacgtacgtacg", "format_sequence(firsttab=5,tab=5,width=4,numleft=1)",
                "1    acgt\n"
                "5    acgt\n"
                "9    acgt\n"
                "13   acg");

        TEST_CI("acgtacgtacgtacg", "format_sequence(firsttab=5,tab=5,width=4,numright=9)", // test EMBL sequence formatting
                "     acgt         4\n"
                "     acgt         8\n"
                "     acgt        12\n"
                "     acg         15");

        TEST_CI("acgtacgtacgtac", "format_sequence(firsttab=5,tab=5,width=4,gap=2,numright=-1)", // autodetect width for 'numright'
                "     ac gt  4\n"
                "     ac gt  8\n"
                "     ac gt 12\n"
                "     ac    14");

        TEST_CI("acgt", "format_sequence(firsttab=0,tab=0,width=2,gap=1)",
                "a c\n"
                "g t");
        TEST_CI("acgt",     "format_sequence(firsttab=0,tab=0,width=4,gap=1)", "a c g t");
        TEST_CI("acgt",     "format_sequence(firsttab=0,tab=0,width=4,gap=2)", "ac gt");
        TEST_CI("acgtacgt", "format_sequence(firsttab=0,width=10,gap=4)",      "acgt acgt");
        TEST_CI("acgtacgt", "format_sequence(firsttab=1,width=10,gap=4)",      " acgt acgt");

        TEST_CI_ERROR_CONTAINS("acgt", "format_sequence(nl=c)",     "Unknown Parameter 'nl=c' in command 'format_sequence'");
        TEST_CI_ERROR_CONTAINS("acgt", "format_sequence(forcenl=)", "Unknown Parameter 'forcenl=' in command 'format_sequence'");

        TEST_CI_ERROR_CONTAINS("acgt", "format(width=0)",          "Illegal zero width");
        TEST_CI_ERROR_CONTAINS("acgt", "format_sequence(width=0)", "Illegal zero width");

        // remove + keep
        TEST_CI_NOOP("acgtacgt",         "remove(-.)");
        TEST_CI     ("..acg--ta-cgt...", "remove(-.)", "acgtacgt");
        TEST_CI     ("..acg--ta-cgt...", "remove(acgt)", "..---...");

        TEST_CI_NOOP("acgtacgt",         "keep(acgt)");
        TEST_CI     ("..acg--ta-cgt...", "keep(-.)", "..---...");
        TEST_CI     ("..acg--ta-cgt...", "keep(acgt)", "acgtacgt");

        // compare + icompare
        TEST_CI("x,z,y,y,z,x,x,Z,y,Y,Z,x", WITH_SPLITTED("|compare"),  "-1,0,1,1,1,-1");
        TEST_CI("x,z,y,y,z,x,x,Z,y,Y,Z,x", WITH_SPLITTED("|icompare"), "-1,0,1,-1,0,1");

        TEST_CI("x,y,z", WITH_SPLITTED("|compare(\"y\")"), "-1,0,1");

        // equals + iequals
        TEST_CI("a,b,a,a,a,A", WITH_SPLITTED("|equals"),  "0,1,0");
        TEST_CI("a,b,a,a,a,A", WITH_SPLITTED("|iequals"), "0,1,1");

        // contains + icontains
        TEST_CI("abc,bcd,BCD", WITH_SPLITTED("|contains(\"bc\")"),   "2,1,0");
        TEST_CI("abc,bcd,BCD", WITH_SPLITTED("|icontains(\"bc\")"),  "2,1,1");
        TEST_CI("abc,bcd,BCD", WITH_SPLITTED("|icontains(\"d\")"),   "0,3,3");

        // partof + ipartof
        TEST_CI("abc,BCD,def,deg", WITH_SPLITTED("|partof(\"abcdefg\")"),   "1,0,4,0");
        TEST_CI("abc,BCD,def,deg", WITH_SPLITTED("|ipartof(\"abcdefg\")"),  "1,2,4,0");

        // translate
        TEST_CI("abcdefgh", "translate(abc,cba)",     "cbadefgh");
        TEST_CI("abcdefgh", "translate(cba,abc)",     "cbadefgh");
        TEST_CI("abcdefgh", "translate(hcba,abch,-)", "hcb----a");
        TEST_CI("abcdefgh", "translate(aceg,aceg,-)", "a-c-e-g-");
        TEST_CI("abbaabba", "translate(ab,ba,-)",     "baabbaab");
        TEST_CI("abbaabba", "translate(a,x,-)",       "x--xx--x");
        TEST_CI("abbaabba", "translate(,,-)",         "--------");

        // echo
        TEST_CI("", "echo", "");
        TEST_CI("", "echo(x,y,z)", "xyz");
        TEST_CI("", "echo(x;y,z)", "xyz"); // check ';' as param-separator
        TEST_CI("", "echo(x;y;z)", "xyz");
        TEST_CI("", "echo(x,y,z)|streams", "3");

        // upper, lower + caps
        TEST_CI("the QUICK brOwn Fox", "lower", "the quick brown fox");
        TEST_CI("the QUICK brOwn Fox", "upper", "THE QUICK BROWN FOX");
        TEST_CI("the QUICK brOwn FoX", "caps",  "The Quick Brown Fox");

        // head, tail + mid/mid0
        TEST_CI     ("1234567890", "head(3)", "123");
        TEST_CI     ("1234567890", "head(9)", "123456789");
        TEST_CI_NOOP("1234567890", "head(10)");
        TEST_CI_NOOP("1234567890", "head(20)");

        TEST_CI     ("1234567890", "tail(4)", "7890");
        TEST_CI     ("1234567890", "tail(9)", "234567890");
        TEST_CI_NOOP("1234567890", "tail(10)");
        TEST_CI_NOOP("1234567890", "tail(20)");

        TEST_CI("1234567890", "tail(0)", "");
        TEST_CI("1234567890", "head(0)", "");
        TEST_CI("1234567890", "tail(-2)", "");
        TEST_CI("1234567890", "head(-2)", "");

        TEST_CI("1234567890", "mid(3,5)", "345");
        TEST_CI("1234567890", "mid(2,2)", "2");

        TEST_CI("1234567890", "mid0(3,5)", "456");

        TEST_CI("1234567890", "mid(9,20)", "90");
        TEST_CI("1234567890", "mid(20,20)", "");

        TEST_CI("1234567890", "tail(3)",     "890"); // example from ../HELP_SOURCE/oldhelp/commands.hlp@mid0
        TEST_CI("1234567890", "mid(-2,0)",   "890");
        TEST_CI("1234567890", "mid0(-3,-1)", "890");

        // tab + pretab
        TEST_CI("x,xx,xxx", WITH_SPLITTED("|tab(2)"),    "x ,xx,xxx");
        TEST_CI("x,xx,xxx", WITH_SPLITTED("|tab(3)"),    "x  ,xx ,xxx");
        TEST_CI("x,xx,xxx", WITH_SPLITTED("|tab(4)"),    "x   ,xx  ,xxx ");
        TEST_CI("x,xx,xxx", WITH_SPLITTED("|pretab(2)"), " x,xx,xxx");
        TEST_CI("x,xx,xxx", WITH_SPLITTED("|pretab(3)"), "  x, xx,xxx");
        TEST_CI("x,xx,xxx", WITH_SPLITTED("|pretab(4)"), "   x,  xx, xxx");

        // crop
        TEST_CI("   x  x  ",         "crop(\" \")",     "x  x");
        TEST_CI("\n \t  x  x \n \t", "crop(\"\t\n \")", "x  x");

        // cut, drop, dropempty and dropzero
        TEST_CI("one,two,three,four,five,six", WITH_SPLITTED("|cut(2,3,5)"),        "two,three,five");
        TEST_CI("one,two,three,four,five,six", WITH_SPLITTED("|drop(2,3,5)"),       "one,four,six");

        TEST_CI_ERROR_CONTAINS("a", "drop(2)", "Illegal stream number '2' (allowed [1..1])");
        TEST_CI_ERROR_CONTAINS("a", "drop(0)", "Illegal stream number '0' (allowed [1..1])");
        TEST_CI_ERROR_CONTAINS("a", "cut(2)",  "Illegal stream number '2' (allowed [1..1])");
        TEST_CI_ERROR_CONTAINS("a", "cut(0)",  "Illegal stream number '0' (allowed [1..1])");

        TEST_CI("one,two,three,four,five,six", WITH_SPLITTED("|dropempty|streams"), "6");
        TEST_CI("one,two,,,five,six",          WITH_SPLITTED("|dropempty|streams"), "4");
        TEST_CI(",,,,,",                       WITH_SPLITTED("|dropempty"),         "");
        TEST_CI(",,,,,",                       WITH_SPLITTED("|dropempty|streams"), "0");

        TEST_CI("1,0,0,2,3,0",                 WITH_SPLITTED("|dropzero"),          "1,2,3");
        TEST_CI("0,0,0,0,0,0",                 WITH_SPLITTED("|dropzero"),          "");
        TEST_CI("0,0,0,0,0,0",                 WITH_SPLITTED("|dropzero|streams"),  "0");

        // swap
        TEST_CI("1,2,3,four,five,six", WITH_SPLITTED("|swap"),                "1,2,3,four,six,five");
        TEST_CI("1,2,3,four,five,six", WITH_SPLITTED("|swap(2,3)"),           "1,3,2,four,five,six");
        TEST_CI("1,2,3,four,five,six", WITH_SPLITTED("|swap(2,3)|swap(4,3)"), "1,3,four,2,five,six");
        TEST_CI_NOOP("1,2,3,four,five,six", WITH_SPLITTED("|swap(3,3)"));
        TEST_CI_NOOP("1,2,3,four,five,six", WITH_SPLITTED("|swap(3,2)|swap(2,3)"));
        TEST_CI_NOOP("1,2,3,four,five,six", WITH_SPLITTED("|swap(3,2)|swap(3,1)|swap(2,1)|swap(1,3)"));

        TEST_CI_ERROR_CONTAINS("a",   "swap",                      "need at least two input streams");
        TEST_CI_ERROR_CONTAINS("a,b", WITH_SPLITTED("|swap(2,3)"), "Illegal stream number '3' (allowed [1..2])");
        TEST_CI_ERROR_CONTAINS("a,b", WITH_SPLITTED("|swap(3,2)"), "Illegal stream number '3' (allowed [1..2])");

        // toback + tofront
        TEST_CI     ("front,mid,back", WITH_SPLITTED("|toback(2)"),  "front,back,mid");
        TEST_CI     ("front,mid,back", WITH_SPLITTED("|tofront(2)"), "mid,front,back");
        TEST_CI_NOOP("front,mid,back", WITH_SPLITTED("|toback(3)"));
        TEST_CI_NOOP("front,mid,back", WITH_SPLITTED("|tofront(1)"));
        TEST_CI_NOOP("a",              WITH_SPLITTED("|tofront(1)"));
        TEST_CI_NOOP("a",              WITH_SPLITTED("|toback(1)"));

        TEST_CI_ERROR_CONTAINS("a,b", WITH_SPLITTED("|tofront(3)"), "Illegal stream number '3' (allowed [1..2])");
        TEST_CI_ERROR_CONTAINS("a,b", WITH_SPLITTED("|toback(3)"),  "Illegal stream number '3' (allowed [1..2])");

        // split (default)
        TEST_CI("a\nb", "|split" ACI_MERGE, "a,b");

        // extract_words + extract_sequence
        TEST_CI("1,2,3,four,five,six", "extract_words(\"0123456789\",1)",                  "1 2 3");
        TEST_CI("1,2,3,four,five,six", "extract_words(\"abcdefghijklmnopqrstuvwxyz\", 3)", "five four six");
        TEST_CI("1,2,3,four,five,six", "extract_words(\"abcdefghijklmnopqrstuvwxyz\", 4)", "five four");
        TEST_CI("1,2,3,four,five,six", "extract_words(\"abcdefghijklmnopqrstuvwxyz\", 5)", "");

        TEST_CI     ("1,2,3,four,five,six",    "extract_sequence(\"acgtu\", 1.0)",   "");
        TEST_CI     ("1,2,3,four,five,six",    "extract_sequence(\"acgtu\", 0.5)",   "");
        TEST_CI     ("1,2,3,four,five,six",    "extract_sequence(\"acgtu\", 0.0)",   "four five six");
        TEST_CI     ("..acg--ta-cgt...",       "extract_sequence(\"acgtu\", 1.0)",   "");
        TEST_CI_NOOP("..acg--ta-cgt...",       "extract_sequence(\"acgtu-.\", 1.0)");
        TEST_CI_NOOP("..acg--ta-ygt...",       "extract_sequence(\"acgtu-.\", 0.7)");
        TEST_CI     ("70 ..acg--ta-cgt... 70", "extract_sequence(\"acgtu-.\", 1.0)", "..acg--ta-cgt...");

        // checksum + gcgchecksum
        TEST_CI("", "sequence|checksum",      "4C549A5F");
        TEST_CI("", "sequence | gcgchecksum", "4308");

        // SRT
        TEST_CI(        "The quick brown fox", "srt(\"quick=lazy:brown fox=dog\")", "The lazy dog");
        TEST_CI__BROKEN("The quick brown fox", "srt(quick=lazy:brown fox=dog)",
                        "The lazy dog",        // @@@ parsing problem ?
                        "The lazy brown fox"); // document current (unwanted behavior)
        TEST_CI_ERROR_CONTAINS("x", "srt(x=y,z)", "SRT ERROR: no '=' found in command");

        // REG
        TEST_CI("stars*to*stripes", "/\\*/--/", "stars--to--stripes");

        TEST_CI("sImILaRWIllBE,GonEEASIly", WITH_SPLITTED("|command(/[A-Z]//)"),   "small,only");
        TEST_CI("sthBIGinside,FATnotCAP",   WITH_SPLITTED("|command(/([A-Z])+/)"), "BIG,FAT");

        // calculator
        TEST_CI("", "echo(9,3)|plus",     "12");
        TEST_CI("", "echo(9,3)|minus",    "6");
        TEST_CI("", "echo(9,3)|mult",     "27");
        TEST_CI("", "echo(9,3)|div",      "3");
        TEST_CI("", "echo(9,3)|rest",     "0");
        TEST_CI("", "echo(9,3)|per_cent", "300");

        TEST_CI("", "echo(1,2,3)|plus(1)" ACI_MERGE,     "2,3,4");
        TEST_CI("", "echo(1,2,3)|minus(2)" ACI_MERGE,    "-1,0,1");
        TEST_CI("", "echo(1,2,3)|mult(42)" ACI_MERGE,    "42,84,126");
        TEST_CI("", "echo(1,2,3)|div(2)" ACI_MERGE,      "0,1,1");
        TEST_CI("", "echo(1,2,3)|rest(2)" ACI_MERGE,     "1,0,1");
        TEST_CI("", "echo(1,2,3)|per_cent(3)" ACI_MERGE, "33,66,100");

        // readdb
        TEST_CI("", "readdb(name)", "LcbReu40");
        TEST_CI("", "readdb(acc)",  "X76328");

        // taxonomy
        TEST_CI("", "taxonomy(1)",           "No default tree");
        TEST_CI("", "taxonomy(tree_nuc, 1)", "group1");
        TEST_CI("", "taxonomy(tree_nuc, 5)", "top/lower-red/group1");
        TEST_CI_ERROR_CONTAINS("", "taxonomy", "syntax: taxonomy([tree_name,]count)");

        // diff, filter + change
        TEST_CI("..acg--ta-cgt..." ","
                "..acg--ta-cgt...", WITH_SPLITTED("|diff(pairwise=1)"),
                "................");
        TEST_CI("..acg--ta-cgt..." ","
                "..cgt--ta-acg...", WITH_SPLITTED("|diff(pairwise=1,equal==)"),
                "==cgt=====acg===");
        TEST_CI("..acg--ta-cgt..." ","
                "..cgt--ta-acg...", WITH_SPLITTED("|diff(pairwise=1,differ=X)"),
                "..XXX.....XXX...");
        TEST_CI("", "sequence|diff(species=LcbFruct)|checksum", "645E3107");

        TEST_CI("..XXX.....XXX..." ","
                "..acg--ta-cgt...", WITH_SPLITTED("|filter(pairwise=1,exclude=X)"),
                "..--ta-...");
        TEST_CI("..XXX.....XXX..." ","
                "..acg--ta-cgt...", WITH_SPLITTED("|filter(pairwise=1,include=X)"),
                "acgcgt");
        TEST_CI("", "sequence|filter(species=LcbFruct,include=.-)", "-----------T----T-------G----------C-----T----T...");

        TEST_CI("...XXX....XXX..." ","
                "..acg--ta-cgt...", WITH_SPLITTED("|change(pairwise=1,include=X,to=C,change=100)"),
                "..aCC--ta-CCC...");
        TEST_CI("...XXXXXXXXX...." ","
                "..acg--ta-cgt...", WITH_SPLITTED("|change(pairwise=1,include=X,to=-,change=100)"),
                "..a---------t...");


        // exec
        TEST_CI("c,b,c,b,a,a", WITH_SPLITTED("|exec(\"(sort|uniq)\")|split|dropempty"),              "a,b,c");
        TEST_CI("a,aba,cac",   WITH_SPLITTED("|exec(\"perl\",-pe,s/([bc])/$1$1/g)|split|dropempty"), "a,abba,ccacc");

         // error cases
        TEST_CI_ERROR_CONTAINS("", "nocmd",          "Unknown command 'nocmd'");
        TEST_CI_ERROR_CONTAINS("", "|nocmd",         "Unknown command 'nocmd'");
        TEST_CI_ERROR_CONTAINS("", "caps(x)",        "syntax: caps (no parameters)");
        TEST_CI_ERROR_CONTAINS("", "trace",          "syntax: trace(0|1)");
        TEST_CI_ERROR_CONTAINS("", "count",          "syntax: count(\"characters to count\")");
        TEST_CI_ERROR_CONTAINS("", "count(a,b)",     "syntax: count(\"characters to count\")");
        TEST_CI_ERROR_CONTAINS("", "len(a,b)",       "syntax: len[(\"characters not to count\")]");
        TEST_CI_ERROR_CONTAINS("", "plus(a,b,c)",    "syntax: plus[(Expr1[,Expr2])]");
        TEST_CI_ERROR_CONTAINS("", "count(a,b",      "Reason: Missing ')'");
        TEST_CI_ERROR_CONTAINS("", "count(a,\"b)",   "unbalanced '\"' in 'count(a,\"b)'");
        TEST_CI_ERROR_CONTAINS("", "count(a,\"b)\"", "Reason: Missing ')'");
        TEST_CI_ERROR_CONTAINS("", "dd;dd|count",    "syntax: count(\"characters to count\")");
        TEST_CI_ERROR_CONTAINS("", "|count(\"a\"x)", "Reason: Missing '\"'");
        TEST_CI_ERROR_CONTAINS("", "|count(\"a)",    "unbalanced '\"' in '|count(\"a)'");

        TEST_CI_ERROR_CONTAINS("", "translate(a)",       "syntax: translate(old,new[,other])");
        TEST_CI_ERROR_CONTAINS("", "translate(a,b,c,d)", "syntax: translate(old,new[,other])");

        TEST_CI_ERROR_CONTAINS(NULL, "whatever", "ARB ERROR: Can't read this DB entry as string"); // here gb_data is the species container

        gb_data = GB_entry(gb_data, "full_name"); // execute ACI on 'full_name' from here on ------------------------------

        TEST_CI(NULL, "", "Lactobacillus reuteri"); // noop
        TEST_CI(NULL, "|len", "21");
        TEST_CI(NULL, ":tobac=", "Lacillus reuteri");
        TEST_CI(NULL, "/ba.*us/B/", "LactoB reuteri");

        TEST_CI(NULL, "|taxonomy(1)", "No default tree");
        TEST_CI_ERROR_CONTAINS(NULL, "|taxonomy(tree_nuc,2)", "Container has neither 'name' nor 'group_name' entry - can't detect container type");

        gb_data = NULL;
        TEST_CI_ERROR_CONTAINS(NULL, "", "no input streams found");
    }

    GB_set_ACISRT_trace(old_trace); // avoid side effect of TEST_GB_command_interpreter
    GB_close(gb_main);
}

const char *GBT_get_name(GBDATA *gb_item); 

struct TestDB : virtual Noncopyable {
    GB_shell  shell;
    GBDATA   *gb_main;
    GB_ERROR  error;

    GBDATA *gb_cont1;
    GBDATA *gb_cont2;
    GBDATA *gb_cont_empty;
    GBDATA *gb_cont_misc;

    GB_ERROR create_many_items(GBDATA *gb_parent, const char **item_key_list, int item_count) {
        int k = 0;
        for (int i = 0; i<item_count && !error; i++) {
            const char *item_key    = item_key_list[k++];
            if (!item_key) { item_key = item_key_list[0]; k = 1; }

            GBDATA *gb_child = GB_create_container(gb_parent, item_key);
            if (!gb_child) {
                error = GB_await_error();
            }
            else {
                if ((i%7) == 0) GB_write_flag(gb_child, 1); // mark some
                
                GBDATA *gb_name = GB_create(gb_child, "name", GB_STRING);
                error           = GB_write_string(gb_name, GBS_global_string("%s %i", item_key, i));
            }
        }
        return error;
    }

    TestDB() {
        gb_main = GB_open("nosuch.arb", "c");
        error   = gb_main ? NULL : GB_await_error();

        if (!error) {
            GB_transaction ta(gb_main);
            gb_cont1      = GB_create_container(gb_main, "container1");
            gb_cont2      = GB_create_container(gb_main, "container2");
            gb_cont_empty = GB_create_container(gb_main, "empty");
            gb_cont_misc  = GB_create_container(gb_main, "misc");

            if (!gb_cont1 || !gb_cont2) error = GB_await_error();

            const char *single_key[] = { "entry", NULL };
            const char *mixed_keys[] = { "item", "other", NULL };

            if (!error) error = create_many_items(gb_cont1, single_key, 100);
            if (!error) error = create_many_items(gb_cont2, mixed_keys, 20);
        }
        TEST_EXPECT_NO_ERROR(error);
    }
    ~TestDB() {
        GB_close(gb_main);
    }
};

void TEST_DB_search() {
    TestDB db;
    TEST_EXPECT_NO_ERROR(db.error);

    {
        GB_transaction ta(db.gb_main);

        TEST_EXPECT_EQUAL(GB_number_of_subentries(db.gb_cont1), 100);
        TEST_EXPECT_EQUAL(GB_number_of_subentries(db.gb_cont2), 20);

        {
            GBDATA *gb_any_child = GB_child(db.gb_cont1);
            TEST_REJECT_NULL(gb_any_child);
            TEST_EXPECT_EQUAL(gb_any_child, GB_entry(db.gb_cont1, "entry"));
            TEST_EXPECT_EQUAL(gb_any_child, GB_search(db.gb_main, "container1/entry", GB_FIND));

            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_main, "zic-zac", GB_FIND), "Invalid char '-' in key 'zic-zac'");

            // check link-syntax
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_main, "->entry", GB_FIND), "Missing linkname before '->' in '->entry'");
            TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_search(db.gb_main, "entry->bla", GB_FIND));
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_main, "container1/entry->nowhere", GB_FIND), "'entry' exists, but is not a link");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_main, "container1/nosuchentry->nowhere", GB_STRING), "Cannot create links on the fly in gb_search");
            TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_search(db.gb_main, "entry->", GB_FIND)); // valid ? just deref link

            // check ..
            TEST_EXPECT_EQUAL(GB_search(gb_any_child, "..", GB_FIND), db.gb_cont1);
            TEST_EXPECT_EQUAL(GB_search(gb_any_child, "../..", GB_FIND), db.gb_main);
            // above main entry
            TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_search(gb_any_child, "../../..", GB_FIND));
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(gb_any_child, "../../../impossible", GB_STRING), "cannot use '..' at root node");

            TEST_EXPECT_EQUAL(GB_search(gb_any_child, "", GB_FIND), gb_any_child); // return self
            TEST_EXPECT_EQUAL(GB_search(gb_any_child, "/container1/", GB_FIND), db.gb_cont1); // accept trailing slash for container ..
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(gb_any_child, "/container1/entry/name/", GB_FIND), "terminal entry 'name' cannot be used as container"); // .. but not for normal entries

            TEST_EXPECT_EQUAL(GB_search(gb_any_child, "/", GB_FIND), db.gb_main);
            TEST_EXPECT_EQUAL(GB_search(gb_any_child, "/container1/..", GB_FIND), db.gb_main);
            TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_search(gb_any_child, "/..", GB_FIND)); // main has no parent
        }

        {
            GBDATA *gb_child1 = GB_child(db.gb_cont2);   TEST_REJECT_NULL(gb_child1);
            GBDATA *gb_child2 = GB_nextChild(gb_child1); TEST_REJECT_NULL(gb_child2);
            GBDATA *gb_child3 = GB_nextChild(gb_child2); TEST_REJECT_NULL(gb_child3);
            GBDATA *gb_child4 = GB_nextChild(gb_child3); TEST_REJECT_NULL(gb_child4);

            TEST_EXPECT_EQUAL(GB_read_key_pntr(gb_child1), "item");
            TEST_EXPECT_EQUAL(GB_read_key_pntr(gb_child2), "other");
            TEST_EXPECT_EQUAL(GB_read_key_pntr(gb_child3), "item");
            TEST_EXPECT_EQUAL(GB_read_key_pntr(gb_child4), "other");

            TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_entry(db.gb_cont2, "entry"));
            TEST_EXPECT_EQUAL(GB_entry(db.gb_cont2, "item"),  gb_child1);
            TEST_EXPECT_EQUAL(GB_entry(db.gb_cont2, "other"), gb_child2);

            TEST_EXPECT_EQUAL(GB_nextEntry(gb_child1), gb_child3);
            TEST_EXPECT_EQUAL(GB_nextEntry(gb_child2), gb_child4);

            // check ..
            TEST_EXPECT_EQUAL(GB_search(gb_child3, "../item", GB_FIND), gb_child1);
            TEST_EXPECT_EQUAL(GB_search(gb_child3, "../other", GB_FIND), gb_child2);
            TEST_EXPECT_EQUAL(GB_search(gb_child3, "../other/../item", GB_FIND), gb_child1);
        }

        // ------------------------
        //      single entries

        {
            GBDATA *gb_str      = GB_searchOrCreate_string(db.gb_cont_misc, "str", "bla");   TEST_REJECT_NULL(gb_str);
            GBDATA *gb_str_same = GB_searchOrCreate_string(db.gb_cont_misc, "str", "blub");

            TEST_EXPECT_EQUAL(gb_str, gb_str_same);
            TEST_EXPECT_EQUAL(GB_read_char_pntr(gb_str), "bla");

            GBDATA *gb_int      = GB_searchOrCreate_int(db.gb_cont_misc, "int", 4711);   TEST_REJECT_NULL(gb_int);
            GBDATA *gb_int_same = GB_searchOrCreate_int(db.gb_cont_misc, "int", 2012);

            TEST_EXPECT_EQUAL(gb_int, gb_int_same);
            TEST_EXPECT_EQUAL(GB_read_int(gb_int), 4711);

            GBDATA *gb_float      = GB_searchOrCreate_float(db.gb_cont_misc, "float", 0.815);   TEST_REJECT_NULL(gb_float);
            GBDATA *gb_float_same = GB_searchOrCreate_float(db.gb_cont_misc, "float", 3.1415);

            TEST_EXPECT_EQUAL(gb_float, gb_float_same);
            TEST_EXPECT_SIMILAR(GB_read_float(gb_float), 0.815, 0.0001);

            TEST_EXPECT_EQUAL  (GB_read_char_pntr(GB_searchOrCreate_string(db.gb_cont_misc, "sub1/str",    "blub")), "blub");
            TEST_EXPECT_EQUAL  (GB_read_int      (GB_searchOrCreate_int   (db.gb_cont_misc, "sub2/int",    2012)),   2012);
            TEST_EXPECT_SIMILAR(GB_read_float    (GB_searchOrCreate_float (db.gb_cont_misc, "sub3/float", 3.1415)), 3.1415, 0.00001);

            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_searchOrCreate_float (db.gb_cont_misc, "int",   0.815), "has wrong type");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_searchOrCreate_float (db.gb_cont_misc, "str",   0.815), "has wrong type");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_searchOrCreate_int   (db.gb_cont_misc, "float", 4711),  "has wrong type");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_searchOrCreate_int   (db.gb_cont_misc, "str",   4711),  "has wrong type");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_searchOrCreate_string(db.gb_cont_misc, "float", "bla"), "has wrong type");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_searchOrCreate_string(db.gb_cont_misc, "int",   "bla"), "has wrong type");

            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_searchOrCreate_string(db.gb_cont_misc, "*", "bla"), "Invalid char '*' in key '*'");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_searchOrCreate_string(db.gb_cont_misc, "int*", "bla"), "Invalid char '*' in key 'int*'");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_searchOrCreate_string(db.gb_cont_misc, "sth_else*", "bla"), "Invalid char '*' in key 'sth_else*'");

            GBDATA *gb_entry;
            TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_search(db.gb_cont_misc, "subcont/entry", GB_FIND));
            TEST_EXPECT_RESULT__NOERROREXPORTED(gb_entry = GB_search(db.gb_cont_misc, "subcont/entry", GB_INT));
            TEST_EXPECT_EQUAL(GB_read_int(gb_entry), 0);

            GBDATA *gb_cont1;
            TEST_EXPECT_RESULT__NOERROREXPORTED(gb_cont1 = GB_search(db.gb_cont_misc, "subcont", GB_CREATE_CONTAINER));
            TEST_EXPECT_EQUAL(GB_child(gb_cont1), gb_entry); // test GB_search found the container created implicitely above

            GBDATA *gb_cont2;
            TEST_EXPECT_RESULT__NOERROREXPORTED(gb_cont2 = GB_search(db.gb_cont_misc, "subcont2", GB_CREATE_CONTAINER)); // create new container

            // -----------------------
            //      search values

            GBDATA *gb_4711;
            TEST_EXPECT_RESULT__NOERROREXPORTED(gb_4711 = GB_find_int(db.gb_cont_misc, "int", 4711, SEARCH_CHILD));
            TEST_EXPECT_EQUAL(gb_4711, gb_int);

            TEST_EXPECT_NULL(GB_find_int(db.gb_cont_misc, "int", 2012, SEARCH_CHILD));

            GBDATA *gb_bla;
            TEST_EXPECT_RESULT__NOERROREXPORTED(gb_bla = GB_find_string(db.gb_cont_misc, "str", "bla", GB_MIND_CASE, SEARCH_CHILD));
            TEST_EXPECT_EQUAL(gb_bla, gb_str);
            TEST_EXPECT_RESULT__NOERROREXPORTED(gb_bla = GB_find_string(gb_4711, "str", "bla", GB_MIND_CASE, SEARCH_BROTHER));
            TEST_EXPECT_EQUAL(gb_bla, gb_str);

            TEST_EXPECT_NULL(GB_find_string(db.gb_cont_misc, "str", "blub", GB_MIND_CASE, SEARCH_CHILD));

            GBDATA *gb_name;
            TEST_REJECT_NULL                  (GB_find_string          (db.gb_cont1, "name", "entry 77",  GB_MIND_CASE,   SEARCH_GRANDCHILD));
            TEST_REJECT_NULL                  (GB_find_string          (db.gb_cont1, "name", "entry 99",  GB_MIND_CASE,   SEARCH_GRANDCHILD));
            TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_find_string          (db.gb_cont1, "name", "entry 100", GB_MIND_CASE,   SEARCH_GRANDCHILD));
            TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_find_string          (db.gb_cont1, "name", "ENTRY 13",  GB_MIND_CASE,   SEARCH_GRANDCHILD));
            TEST_REJECT_NULL                  (gb_name = GB_find_string(db.gb_cont1, "name", "ENTRY 13",  GB_IGNORE_CASE, SEARCH_GRANDCHILD));

            GBDATA *gb_sub;
            TEST_REJECT_NULL(gb_sub = GB_get_father(gb_name));        TEST_EXPECT_EQUAL(GBT_get_name(gb_sub), "entry 13");
            TEST_REJECT_NULL(gb_sub = GB_followingEntry(gb_sub, 0));  TEST_EXPECT_EQUAL(GBT_get_name(gb_sub), "entry 14");
            TEST_REJECT_NULL(gb_sub = GB_followingEntry(gb_sub, 1));  TEST_EXPECT_EQUAL(GBT_get_name(gb_sub), "entry 16");
            TEST_REJECT_NULL(gb_sub = GB_followingEntry(gb_sub, 10)); TEST_EXPECT_EQUAL(GBT_get_name(gb_sub), "entry 27");
            TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_followingEntry(gb_sub, -1U));
            TEST_REJECT_NULL(gb_sub = GB_brother(gb_sub, "entry"));   TEST_EXPECT_EQUAL(GBT_get_name(gb_sub), "entry 0");

            TEST_EXPECT_EQUAL(gb_bla = GB_search(gb_cont1, "/misc/str", GB_FIND), gb_str); // fullpath (ignores passed GBDATA)

            // keyless search
            TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_search(db.gb_cont_misc, NULL, GB_FIND));

            // ----------------------------
            //      GB_get_GBDATA_path

            TEST_EXPECT_EQUAL(GB_get_GBDATA_path(gb_int),   "/main/misc/int");
            TEST_EXPECT_EQUAL(GB_get_GBDATA_path(gb_str),   "/main/misc/str");
            TEST_EXPECT_EQUAL(GB_get_GBDATA_path(gb_entry), "/main/misc/subcont/entry");
            TEST_EXPECT_EQUAL(GB_get_GBDATA_path(gb_cont2),  "/main/misc/subcont2");

            // -----------------------------------------
            //      search/create with changed type

            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_cont_misc, "str", GB_INT), "Inconsistent type for field");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_cont_misc, "subcont", GB_STRING), "Inconsistent type for field");

            // ---------------------------------------------
            //      search containers with trailing '/'

            GBDATA *gb_cont2_slash;
            TEST_EXPECT_RESULT__NOERROREXPORTED(gb_cont2_slash = GB_search(db.gb_cont_misc, "subcont2/", GB_FIND));
            TEST_EXPECT_EQUAL(gb_cont2_slash, gb_cont2);

            GBDATA *gb_rootcont;
            GBDATA *gb_rootcont_slash;
            TEST_EXPECT_RESULT__NOERROREXPORTED(gb_rootcont = GB_search(db.gb_main, "/container1", GB_FIND));
            TEST_EXPECT_RESULT__NOERROREXPORTED(gb_rootcont_slash = GB_search(db.gb_main, "/container1/", GB_FIND));
            TEST_EXPECT_EQUAL(gb_rootcont_slash, gb_rootcont);
        }

        {
            // check invalid searches
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_cont_misc, "sub/inva*lid",   GB_INT),  "Invalid char '*' in key 'inva*lid'");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_cont_misc, "sub/1 3",        GB_INT),  "Invalid char ' ' in key '1 3'");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_cont_misc, "sub//sub",       GB_INT),  "Invalid '//' in key 'sub//sub'");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_cont_misc, "subcont2//",     GB_FIND), "Invalid '//' in key 'subcont2//'");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_cont_misc, "sub/..sub",      GB_INT),  "Expected '/' after '..' in key '..sub'");
            TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_search(db.gb_cont_misc, "subcont/entry/", GB_FIND), "terminal entry 'entry' cannot be used as container");
        }

        // ---------------
        //      marks

        TEST_EXPECT_EQUAL(GB_number_of_marked_subentries(db.gb_cont1), 15);
        TEST_EXPECT_EQUAL(GB_number_of_marked_subentries(db.gb_cont2), 3);

        TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_first_marked(db.gb_cont2, "entry"));

        GBDATA *gb_marked;
        TEST_REJECT_NULL(gb_marked = GB_first_marked(db.gb_cont2, "item"));
        TEST_EXPECT_EQUAL(GBT_get_name(gb_marked), "item 0");

        TEST_EXPECT_NORESULT__NOERROREXPORTED(GB_following_marked(gb_marked, "item", 1)); // skip over last
        
        TEST_REJECT_NULL(gb_marked = GB_next_marked(gb_marked, "item")); // find last
        TEST_EXPECT_EQUAL(GBT_get_name(gb_marked), "item 14");
    }

    // @@@ delete some species, then search again

    TEST_EXPECT_NO_ERROR(db.error);
}


#endif // UNIT_TESTS
