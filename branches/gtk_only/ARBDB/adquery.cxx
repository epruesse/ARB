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

    if (!orgbuffer) orgbuffer = (char*)malloc(BUFFERSIZE);
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
        case GB_FLOAT: { 
            GBK_terminate("cant search float by value"); // @@@ search by comparing floats is nonsense - should be removed/replaced/rewritten 
            double d = GB_read_float(gb);
            if (d == *(double*)(void*)val) equal = true; // (no aliasing problem here; char* -> double* ok)
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

static char  gb_ctype_table[256];
void gb_init_ctype_table() {
    int i;
    for (i=0; i<256; i++) {
        if (islower(i) || isupper(i) || isdigit(i) || i=='_' || i=='@') {
            gb_ctype_table[i] = 1;
        }
        else {
            gb_ctype_table[i] = 0;
        }
    }
}

inline const char *first_non_key_character(const char *str) {
    while (1) {
        int c = *str;
        if (!gb_ctype_table[c]) {
            if (c == 0) break;
            return str;
        }
        str++;
    }
    return NULL;
}

const char *GB_first_non_key_char(const char *str) {
    return first_non_key_character(str);
}

inline GBDATA *find_or_create(GBCONTAINER *gb_parent, const char *key, GB_TYPES create, bool internflag) {
    gb_assert(!first_non_key_character(key));

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
    const char *separator     = first_non_key_character(key);
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

GBDATA *GB_searchOrCreate_float(GBDATA *gb_container, const char *fieldpath, double default_value) {
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

void gb_install_command_table(GBDATA *gb_main, struct GBL_command_table *table, size_t table_size)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    if (!Main->command_hash) Main->command_hash = GBS_create_hash(table_size, GB_IGNORE_CASE);

    for (; table->command_identifier; table++) {
        GBS_write_hash(Main->command_hash, table->command_identifier, (long)table->function);
    }

    gb_assert((GBS_hash_count_elems(Main->command_hash)+1) == table_size);
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

char *gbs_search_second_bracket(const char *source)
{
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

static void dumpStreams(const char *name, int count, const GBL *args) {
    printf("%s=%i\n", name, count);
    if (count > 0) {
        int c;
        for (c = 0; c<count; c++) {
            printf("  %02i='%s'\n", c, args[c].str);
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
            freeset(shortened_str, GB_strpartdup(str, str+wanted_len));
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

#if defined(WARN_TODO)
#warning rewrite GB_command_interpreter (error+ressource handling)
#endif

char *GB_command_interpreter(GBDATA *gb_main, const char *str, const char *commands, GBDATA *gbd, const char *default_tree_name) {
    /* simple command interpreter returns NULL on error (which should be exported in that case)
     * if first character is == ':' run string parser
     * if first character is == '/' run regexpr
     * else command interpreter
     */
    int           strmalloc = 0;
    char         *buffer;
    GB_ERROR      error;
    int           i;
    int           argcinput;
    int           argcparam;
    int           argcout;
    char         *bracket;
    GB_MAIN_TYPE *Main      = GB_MAIN(gb_main);

    GBL morig[GBL_MAX_ARGUMENTS];
    GBL min[GBL_MAX_ARGUMENTS];
    GBL mout[GBL_MAX_ARGUMENTS];

    GBL *orig  = & morig[0];
    GBL *in    = & min[0];
    GBL *out   = & mout[0];
    int  trace = GB_get_ACISRT_trace();

    if (!str) {
        if (!gbd) {
            GB_export_error("ACI: no input streams found");
            return NULL;
        }
        str = GB_read_as_string(gbd);
        strmalloc = 1;
    }

    if (trace) {
        printf("GB_command_interpreter: str='%s'\n"
               "                        command='%s'\n", str, commands);
    }

    if (!commands || !commands[0]) { // empty command -> do not modify string
        if (!strmalloc) return strdup(str);
        return (char *)str;
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

                if (matched) result   = GB_strndup(matched, matchlen);
                else if (!err) result = strdup("");
            }

            if (!result && err) result = GBS_global_string_copy("<Error: %s>", err);
        }
        return result;
    }

    // ********************** init *******************

    gb_local->gbl.gb_main = gb_main;
    buffer = strdup(commands);

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
                    if (!(c=*s1)) { break; }
                    *(s2++) = c;
                    continue;
                }

                if (c=='"') {       // search the second "
                    const char *hp = gbs_search_second_x(s1+1);
                    if (!hp) {
                        GB_export_errorf("unbalanced '\"' in '%s'", commands);
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



    memset((char *)orig, 0, sizeof(GBL)*GBL_MAX_ARGUMENTS);
    memset((char *)in, 0, sizeof(GBL)*GBL_MAX_ARGUMENTS);
    memset((char *)out, 0, sizeof(GBL)*GBL_MAX_ARGUMENTS);

    if (strmalloc) {
        orig[0].str = (char *)str;
    }
    else {
        orig[0].str = strdup(str);
    }

    argcinput = 1;
    argcout   = 0;
    error     = 0;
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
            memset((char*)in, 0, sizeof(GBL)*GBL_MAX_ARGUMENTS);
            if (*s1 == '"') {           // copy "text" to out
                char *end = gbs_search_second_x(s1+1);
                if (!end) {
                    error = "Missing second '\"'";
                    break;
                }
                *end = 0;
                out[argcout++].str = strdup(s1+1);
            }
            else {
                argcparam = 0;
                bracket = strchr(s1, '(');
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
                            in[argcparam++].str = strdup(p1);
                        }
                    }
                    if (error) break;
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
                        GBL_command_arguments args;
                        args.gb_ref            = gbd;
                        args.default_tree_name = default_tree_name;
                        args.command           = s1;
                        args.cinput            = argcinput;
                        args.vinput            = orig;
                        args.cparam            = argcparam;
                        args.vparam            = in;
                        args.coutput           = &argcout;
                        args.voutput           = &out;

                        if (trace) {
                            printf("-----------------------\nExecution of command '%s':\n", args.command);
                            dumpStreams("Arguments", args.cparam, args.vparam);
                            dumpStreams("InputStreams", args.cinput, args.vinput);
                        }

                        error = command(&args); // execute the command

                        if (!error && trace) dumpStreams("OutputStreams", *args.coutput, *args.voutput);

                        if (error) {
                            char *inputstreams = 0;
                            char *paramlist    = 0;
                            int   j;

#define MAX_PRINT_LEN 200

                            for (j = 0; j<args.cparam; ++j) {
                                const char *param       = args.vparam[j].str;
                                const char *param_short = shortenLongString(param, MAX_PRINT_LEN);

                                if (!paramlist) paramlist = strdup(param_short);
                                else freeset(paramlist, GBS_global_string_copy("%s,%s", paramlist, param_short));
                            }
                            for (j = 0; j<args.cinput; ++j) {
                                const char *param       = args.vinput[j].str;
                                const char *param_short = shortenLongString(param, MAX_PRINT_LEN);

                                if (!inputstreams) inputstreams = strdup(param_short);
                                else freeset(inputstreams, GBS_global_string_copy("%s;%s", inputstreams, param_short));
                            }
#undef MAX_PRINT_LEN
                            if (paramlist) {
                                error = GBS_global_string("while applying '%s(%s)'\nto '%s':\n%s", s1, paramlist, inputstreams, error);
                            }
                            else {
                                error = GBS_global_string("while applying '%s'\nto '%s':\n%s", s1, inputstreams, error);
                            }

                            free(inputstreams);
                            free(paramlist);
                        }
                    }
                }

                for (i=0; i<argcparam; i++) {   // free intermediate arguments
                    if (in[i].str) free(in[i].str);
                }
            }

            if (error) break;

            if (separator == '|') {         // swap in and out in pipes
                for (i=0; i<argcinput; i++) {
                    if (orig[i].str)    free(orig[i].str);
                }
                memset((char*)orig, 0, sizeof(GBL)*GBL_MAX_ARGUMENTS);

                {
                    GBL *h;
                    h = out;            // swap orig and out
                    out = orig;
                    orig = h;
                }

                argcinput = argcout;
                argcout = 0;
            }

        }
    }
    for (i=0; i<argcinput; i++) {
        if (orig[i].str) free(orig[i].str);
    }

    {
        char *s1;
        if (!argcout) {
            s1 = strdup(""); // returned '<NULL>' in the past
        }
        else if (argcout == 1) {
            s1 = out[0].str;
        }
        else {             // concatenate output strings
            GBS_strstruct *strstruct = GBS_stropen(1000);
            for (i=0; i<argcout; i++) {
                if (out[i].str) {
                    GBS_strcat(strstruct, out[i].str);
                    free(out[i].str);
                }
            }
            s1 = GBS_strclose(strstruct);
        }
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

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

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
            TEST_EXPECT_SIMILAR(GB_read_float    (GB_searchOrCreate_float (db.gb_cont_misc, "sub3/float", 3.1415)), 3.1415, 0.0001);

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

// --------------------------------------------------------------------------------
