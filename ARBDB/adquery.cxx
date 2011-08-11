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

#include <cctype>

#define GB_PATH_MAX 1024

static void build_GBDATA_path(GBDATA *gbd, char **buffer) {
    GBCONTAINER *gbc = GB_FATHER(gbd);
    const char  *key;

    if (gbc) {
        build_GBDATA_path((GBDATA*)gbc, buffer);
        key = GB_KEY(gbd);
        {
            char *bp = *buffer;
            *bp++ = '/';
            while (*key) *bp++ = *key++;
            *bp      = 0;

            *buffer = bp;
        }
    }
}

#define BUFFERSIZE 1024

const char *GB_get_GBDATA_path(GBDATA *gbd) {
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
    GB_TYPES realtype = GB_TYPE(gb);
    gb_assert(val);
    if (type == GB_STRING) {
        gb_assert(realtype == GB_STRING || realtype == GB_LINK); // gb_find_internal called with wrong type
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
            double d                      = GB_read_float(gb);
            if (d == *(double*)val) equal = true; // (no aliasing problem here; char* -> double* ok)
            break;
        }
        default: {
            const char *err = GBS_global_string("Value search not supported for data type %i (%i)", GB_TYPE(gb), type);
            GB_internal_error(err);
            break;
        }
    }

    return equal;
}

static GBDATA *find_sub_by_quark(GBDATA *father, GBQUARK key_quark, GB_TYPES type, const char *val, GB_CASE case_sens, GBDATA *after, size_t skip_over) {
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

    int             end, index;
    GBCONTAINER    *gbf = (GBCONTAINER*)father;
    gb_header_list *header;
    GBDATA         *gb;

    end  = gbf->d.nheader;
    header = GB_DATA_LIST_HEADER(gbf->d);
    if (after) index = (int)after->index+1; else index = 0;

    if (key_quark<0) { // unspecific key quark (i.e. search all)
        gb_assert(!val);        // search for val not possible if searching all keys!
        if (!val) {
            for (; index < end; index++) {
                if (header[index].flags.key_quark != 0) {
                    if (header[index].flags.changed >= GB_DELETED) continue;
                    if (!(gb=GB_HEADER_LIST_GBD(header[index]))) {
                        gb_unfold(gbf, 0, index);
                        header = GB_DATA_LIST_HEADER(gbf->d);
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
            if ((key_quark == header[index].flags.key_quark)) {
                if (header[index].flags.changed >= GB_DELETED) continue;
                if (!(gb=GB_HEADER_LIST_GBD(header[index])))
                {
                    gb_unfold(gbf, 0, index);
                    header = GB_DATA_LIST_HEADER(gbf->d);
                    gb = GB_HEADER_LIST_GBD(header[index]);
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
    return find_sub_by_quark(father, key_quark, GB_NONE, NULL, GB_MIND_CASE, after, skip_over);
}

NOT4PERL GBDATA *GB_find_subcontent_by_quark(GBDATA *father, GBQUARK key_quark, GB_TYPES type, const char *val, GB_CASE case_sens, GBDATA *after, size_t skip_over) {
    return find_sub_by_quark(father, key_quark, type, val, case_sens, after, skip_over);
}

static GBDATA *find_sub_sub_by_quark(GBDATA *father, const char *key, GBQUARK sub_key_quark, GB_TYPES type, const char *val, GB_CASE case_sens, GBDATA *after) {
    int             end, index;
    gb_header_list *header;
    GBCONTAINER    *gbf  = (GBCONTAINER*)father;
    GBDATA         *gb;
    GBDATA         *res;
    gb_index_files *ifs  = NULL;
    GB_MAIN_TYPE   *Main = GBCONTAINER_MAIN(gbf);

    end  = gbf->d.nheader;
    header = GB_DATA_LIST_HEADER(gbf->d);

    if (after) index = (int)after->index+1; else index = 0;

    // ****** look for any hash index tables ********
    // ****** no wildcards allowed       *******
    if (!Main->local_mode) {
        if (gbf->flags2.folded_container) {
            // do the query in the server
            if (GB_ARRAY_FLAGS(gbf).changed) {
                if (!gbf->flags2.update_in_server) {
                    GB_update_server((GBDATA *)gbf);
                }
            }
        }
        if (gbf->d.size > GB_MAX_LOCAL_SEARCH && val) {
            if (after) res = GBCMC_find(after,  key, type, val, case_sens, SEARCH_CHILD_OF_NEXT);
            else       res = GBCMC_find(father, key, type, val, case_sens, SEARCH_GRANDCHILD);
            return res;
        }
    }
    if (val &&
        (ifs=GBCONTAINER_IFS(gbf))!=NULL &&
        (!strchr(val, '*')) &&
        (!strchr(val, '?')))
    {
        for (; ifs; ifs = GB_INDEX_FILES_NEXT(ifs)) {
            if (ifs->key != sub_key_quark) continue;
            // ***** We found the index table *****
            res = gb_index_find(gbf, ifs, sub_key_quark, val, case_sens, index);
            return res;
        }
    }

    if (after)  gb = after;
    else        gb = NULL;

    for (; index < end; index++) {
        GBDATA *gbn = GB_HEADER_LIST_GBD(header[index]);

        if (header[index].flags.changed >= GB_DELETED) continue;
        if (!gbn) {
            if (!Main->local_mode) {
                if (gb) res = GBCMC_find(gb,     key, type, val, case_sens, SEARCH_CHILD_OF_NEXT);
                else    res = GBCMC_find(father, key, type, val, case_sens, SEARCH_GRANDCHILD);
                return res;
            }
            GB_internal_error("Empty item in server");
            continue;
        }
        gb = gbn;
        if (GB_TYPE(gb) != GB_DB) continue;
        res = GB_find_subcontent_by_quark(gb, sub_key_quark, type, val, case_sens, NULL, 0);
        if (res) return res;
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
                if (GB_TYPE(gbd) == GB_DB) gbc = (GBCONTAINER*)gbd;
                break;

            case SEARCH_CHILD_OF_NEXT:
                after = gbd;
                gbs   = SEARCH_GRANDCHILD;
                gbc   = GB_FATHER(gbd);
                break;
        }

        if (gbc) {
            GBQUARK key_quark = key ? GB_key_2_quark(gbd, key) : -1;

            if (gbs == SEARCH_CHILD) {
                result = GB_find_subcontent_by_quark((GBDATA*)gbc, key_quark, type, val, case_sens, after, 0);
            }
            else {
                gb_assert(gbs == SEARCH_GRANDCHILD);
                result = find_sub_sub_by_quark((GBDATA*)gbc, key, key_quark, type, val, case_sens, after);
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
    return GB_find_sub_by_quark((GBDATA*)GB_FATHER(entry), GB_get_quark(entry), entry, 0);
}
GBDATA *GB_followingEntry(GBDATA *entry, size_t skip_over) {
    // return following child after 'entry', that has the same fieldname
    // (or NULL if no such entry)
    // skips 'skip_over' entries (skip_over == 0 behaves like GB_nextEntry)
    return GB_find_sub_by_quark((GBDATA*)GB_FATHER(entry), GB_get_quark(entry), entry, skip_over);
}

GBDATA *GB_brother(GBDATA *entry, const char *key) {
    // searches (first) brother (before or after) of 'entry' which has field 'key'
    // i.e. does same as GB_entry(GB_get_father(entry), key)
    return GB_find(entry, key, SEARCH_BROTHER);
}

GBDATA *gb_find_by_nr(GBDATA *father, int index) {
    /* get a subentry by its internal number:
       Warning: This subentry must exists, otherwise internal error */

    GBCONTAINER    *gbf = (GBCONTAINER*)father;
    gb_header_list *header;
    GBDATA         *gb;

    if (GB_TYPE(father) != GB_DB) {
        GB_internal_error("type is not GB_DB");
        return NULL;
    }
    header = GB_DATA_LIST_HEADER(gbf->d);
    if (index >= gbf->d.nheader || index <0) {
        GB_internal_errorf("Index '%i' out of range [%i:%i[", index, 0, gbf->d.nheader);
        return NULL;
    }
    if (header[index].flags.changed >= GB_DELETED || !header[index].flags.key_quark) {
        GB_internal_error("Entry already deleted");
        return NULL;
    }
    if (!(gb=GB_HEADER_LIST_GBD(header[index])))
    {
        gb_unfold(gbf, 0, index);
        header = GB_DATA_LIST_HEADER(gbf->d);
        gb = GB_HEADER_LIST_GBD(header[index]);
        if (!gb) {
            GB_internal_error("Could not unfold data");
            return NULL;
        }
    }
    return gb;
}

char  gb_ctype_table[256];
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

inline char *gb_first_non_key_character(const char *str) {
    const char *s = str;
    int c;
    while (1) {
        c = *s;
        if (!gb_ctype_table[c]) {
            if (c == 0) break;
            return (char *)(s);
        }
        s++;
    }
    return NULL;
}

char *GB_first_non_key_char(const char *str) {
    return gb_first_non_key_character(str);
}

GBDATA *gb_search(GBDATA * gbd, const char *str, GB_TYPES create, int internflag) {
    /* finds a hierarchical key,
       if create != GB_FIND(==0), then create the key
       force types if ! internflag
    */

    char   *s1, *s2;
    GBDATA *gbp, *gbsp;
    int     len;
    int     separator = 0;
    char    buffer[GB_PATH_MAX];

    GB_test_transaction(gbd);
    if (!str) {
        return GB_child(gbd);
    }
    if (*str == '/') {
        gbd = GB_get_root(gbd);
        str++;
    }

    if (!gb_first_non_key_character(str)) {
        gbsp = GB_entry(gbd, str);
        if (gbsp && create) {
            GB_TYPES oldType = GB_TYPE(gbsp);
            if (create != oldType) { // type mismatch
                GB_export_errorf("Inconsistent type for field '%s' (existing=%i, expected=%i)", str, oldType, create);
                return NULL;
            }
        }
        if (!gbsp && create) {
            if (internflag) {
                if (create == GB_CREATE_CONTAINER) {
                    gbsp = gb_create_container(gbd, str);
                }
                else {
                    gbsp = gb_create(gbd, str, create);
                }
            }
            else {
                if (create == GB_CREATE_CONTAINER) {
                    gbsp = GB_create_container(gbd, str);
                }
                else {
                    gbsp = gb_create(gbd, str, create);
                }
            }
            if (!gbsp) GB_print_error();
        }
        return gbsp;
    }
    {
        len = strlen(str)+1;
        if (len > GB_PATH_MAX) {
            GB_internal_errorf("Path Length '%i' exceeded by '%s'", GB_PATH_MAX, str);
            return NULL;
        }
        memcpy(buffer, str, len);
    }

    gbp = gbd;
    for (s1 = buffer; s1; s1 = s2) {

        s2 = gb_first_non_key_character(s1);
        if (s2) {
            separator = *s2;
            *(s2++) = 0;
            if (separator == '-') {
                if ((*s2)  != '>') {
                    GB_export_errorf("Invalid key for gb_search '%s'", str);
                    GB_print_error();
                    return NULL;
                }
                s2++;
            }
        }

        if (strcmp("..", s1) == 0) {
            gbsp = GB_get_father(gbp);
        }
        else {
            gbsp = GB_entry(gbp, s1);
            if (gbsp && separator == '-') { // follow link !!!
                if (GB_TYPE(gbsp) != GB_LINK) {
                    if (create) {
                        GB_export_error("Cannot create links on the fly in GB_search");
                        GB_print_error();
                    }
                    return NULL;
                }
                gbsp = GB_follow_link(gbsp);
                separator = 0;
                if (!gbsp) return NULL; // cannot resolve link
            }
            while (gbsp && create) {
                if (s2) {                           // non terminal
                    if (GB_DB == GB_TYPE(gbsp)) break;
                }
                else {                              // terminal
                    if (create == GB_TYPE(gbsp)) break;
                }
                GB_internal_errorf("Inconsistent Type %u:%u '%s':'%s', repairing database", create, GB_TYPE(gbsp), str, s1);
                GB_print_error();
                GB_delete(gbsp);
                gbsp = GB_entry(gbd, s1);
            }
        }
        if (!gbsp) {
            if (!create) return NULL; // read only mode
            if (separator == '-') {
                GB_export_error("Cannot create linked objects");
                return NULL; // do not create linked objects
            }

            if (s2 || (create == GB_CREATE_CONTAINER)) {
                gbsp = internflag
                    ? gb_create_container(gbp, s1)
                    : GB_create_container(gbp, s1);
            }
            else {
                gbsp = GB_create(gbp, s1, (GB_TYPES)create);
                if (create == GB_STRING) {
                    GB_ERROR error = GB_write_string(gbsp, "");
                    if (error) GB_internal_error("Couldn't write to just created string entry");
                }
            }

            if (!gbsp) return NULL;
        }
        gbp = gbsp;
    }
    return gbp;
}


GBDATA *GB_search(GBDATA * gbd, const char *fieldpath, GB_TYPES create) {
    return gb_search(gbd, fieldpath, create, 0);
}

static GBDATA *gb_expect_type(GBDATA *gbd, long expected_type, const char *fieldname) {
    gb_assert(expected_type != GB_FIND); // impossible

    long type = GB_TYPE(gbd);
    if (type != expected_type) {
        GB_export_errorf("Field '%s' has wrong type (found=%li, expected=%li)", fieldname, type, expected_type);
        gbd = 0;
    }
    return gbd;
}

GBDATA *GB_searchOrCreate_string(GBDATA *gb_container, const char *fieldpath, const char *default_value) {
    GBDATA *gb_str = GB_search(gb_container, fieldpath, GB_FIND);
    if (!gb_str) {
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

        if (error) {
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

GBDATA *gb_search_marked(GBCONTAINER *gbc, GBQUARK key_quark, int firstindex, size_t skip_over) {
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

GBDATA *GB_search_last_son(GBDATA *gbd) {
    GBCONTAINER    *gbc    = (GBCONTAINER *)gbd;
    int             index;
    int             end    = gbc->d.nheader;
    GBDATA         *gb;
    gb_header_list *header = GB_DATA_LIST_HEADER(gbc->d);

    for (index = end-1; index>=0; index--) {
        if (header[index].flags.changed >= GB_DELETED) continue;
        if ((gb=GB_HEADER_LIST_GBD(header[index]))==NULL)
        {
            gb_unfold(gbc, 0, index);
            header = GB_DATA_LIST_HEADER(gbc->d);
            gb = GB_HEADER_LIST_GBD(header[index]);
        }
        return gb;
    }
    return NULL;
}

long GB_number_of_marked_subentries(GBDATA *gbd) {
    GBCONTAINER    *gbc     = (GBCONTAINER *)gbd;
    int             userbit = GBCONTAINER_MAIN(gbc)->users[0]->userbit;
    int             index;
    int             end     = gbc->d.nheader;
    gb_header_list *header;
    long            count   = 0;

    header = GB_DATA_LIST_HEADER(gbc->d);
    for (index = 0; index<end; index++) {
        if (! (userbit & header[index].flags.flags)) continue;
        if (header[index].flags.changed >= GB_DELETED) continue;
        count++;
    }
    return count;
}



GBDATA *GB_first_marked(GBDATA *gbd, const char *keystring) {
    GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    GBQUARK key_quark;
    if (keystring) {
        key_quark = GB_key_2_quark(gbd, keystring);
    }
    else {
        key_quark = -1;
    }
    GB_test_transaction(gbd);
    return gb_search_marked(gbc, key_quark, 0, 0);
}


GBDATA *GB_following_marked(GBDATA *gbd, const char *keystring, size_t skip_over) {
    GBCONTAINER *gbc = GB_FATHER(gbd);
    GBQUARK key_quark;

    if (keystring) {
        key_quark = GB_key_2_quark(gbd, keystring);
    }
    else {
        key_quark = -1;
    }
    GB_test_transaction(gbd);
    return gb_search_marked(gbc, key_quark, (int)gbd->index+1, skip_over);
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

    gb_assert((GBS_hash_count_elems(Main->command_hash)+1) == table_size);
}

char *gbs_search_second_x(const char *str) {
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


char *gbs_search_next_separator(const char *source, const char *seps) {
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

char *GBS_apply_ACI(GBDATA *gb_main, const char *commands, const char *str, GBDATA *gbd, const char *default_tree_name) {
    int trace = GB_get_ACISRT_trace();
    
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);

    gb_local->gbl.gb_main = gb_main;

    char *buffer = strdup(commands);

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

    orig.insert(strdup(str));

    GB_ERROR error = NULL;
    GBL_streams out;
    {
        char *s1, *s2;
        s1 = buffer;
        if (*s1 == '|') s1++;

        // ** loop over all commands **
        for (s1 = s1; s1;  s1 = s2) {
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
                
                out.insert(strdup(s1+1));
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
                            in.insert(strdup(p1));
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
                            char *dup_error = strdup(error);

#define MAX_PRINT_LEN 200

                            char *paramlist = 0;
                            for (int j = 0; j<args.param.size(); ++j) {
                                const char *param       = args.param.get(j);
                                const char *param_short = shortenLongString(param, MAX_PRINT_LEN);

                                if (!paramlist) paramlist = strdup(param_short);
                                else freeset(paramlist, GBS_global_string_copy("%s,%s", paramlist, param_short));
                            }
                            char *inputstreams = 0;
                            for (int j = 0; j<args.input.size(); ++j) {
                                const char *input       = args.input.get(j);
                                const char *input_short = shortenLongString(input, MAX_PRINT_LEN);

                                if (!inputstreams) inputstreams = strdup(input_short);
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

void GBL_streams::swap(GBL_streams& other) {
    int bigger = std::max(count, other.count);
    for (int i = 0; i<bigger; ++i) {
        std::swap(content[i], other.content[i]);
    }
    std::swap(count, other.count);
}

char *GBL_streams::concatenated() const {
    if (!count) return strdup("");
    if (count == 1) return strdup(get(0));

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
        return strdup(str);
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

    return GBS_apply_ACI(gb_main, commands, str, gbd, default_tree_name);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

#define TEST_CI__INTERNAL(input,cmd,expected,TEST_RESULT) do {          \
        char *result;                                                   \
        TEST_ASSERT_RESULT__NOERROREXPORTED(result = GB_command_interpreter(gb_main, input, cmd, gb_data, NULL)); \
        TEST_RESULT(result,expected);                                   \
        free(result);                                                   \
    } while(0)


#define TEST_CI(input,cmd,expected)         do {                        \
        TEST_CI__INTERNAL(input,cmd,expected,TEST_ASSERT_EQUAL);        \
    } while(0)

#define TEST_CI__BROKEN(input,cmd,expected) do {                        \
        TEST_CI__INTERNAL(input,cmd,expected,TEST_ASSERT_EQUAL__BROKEN); \
    } while(0)

#define TEST_CI_NOOP(inandout,cmd)         TEST_CI__INTERNAL(inandout,cmd,inandout,TEST_ASSERT_EQUAL)
#define TEST_CI_NOOP__BROKEN(inandout,cmd) TEST_CI__INTERNAL(inandout,cmd,inandout,TEST_ASSERT_EQUAL__BROKEN)

#define TEST_CI_INVERSE(in,cmd,inv_cmd,out) do {        \
        TEST_CI(in,  cmd,     out);                     \
        TEST_CI(out, inv_cmd, in);                      \
    } while(0)

#define TEST_CI_ERROR(input,cmd,error_expected)                 TEST_ASSERT_NORESULT__ERROREXPORTED_CHECKERROR(GB_command_interpreter(gb_main, input, cmd, gb_data, NULL), error_expected, NULL)
#define TEST_CI_ERROR_CONTAINS(input,cmd,errorpart_expected)    TEST_ASSERT_NORESULT__ERROREXPORTED_CHECKERROR(GB_command_interpreter(gb_main, input, cmd, gb_data, NULL), NULL, errorpart_expected)
    
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
        TEST_CI__BROKEN("b\\a",   "|count(\"\\a\")",    "1"); // should only count 'a' (which is escaped in param)
        TEST_CI("b\\a",   "|count(\"\a\")",     "0");   // does not contain '\a'
        TEST_CI("b\a",    "|count(\"\a\")",     "1");   // counts '\a'

        // escaping (@@@ wrong behavior?)
        TEST_CI("b\\a",   "|count(\\a)",         "2"); // i would expect '1' as result (the 'a'), but it counts '\\' and 'a'
        TEST_CI("b\\a",   "|contains(\"\\\\\")", "0"); // searches for 2 backslashes, finds none
        TEST_CI__BROKEN("b\\a",   "|contains(\"\")", "0"); // search for nothing, but reports 1 hit
        TEST_CI__BROKEN("b\\a",   "|contains(\\)",       "1"); // searches for 1 backslash (ok), but reports two hits instead of one
        TEST_CI__BROKEN("b\\\\a", "|contains(\"\\\\\")", "1"); // searches for 2 backslashes (ok), but reports two hits instead of one
        TEST_CI_ERROR("b\\a", "|contains(\"\\\")", "ARB ERROR: unbalanced '\"' in '|contains(\"\\\")'"); // raises error (should searches for 1 backslash)

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
            TEST_CI_ERROR_CONTAINS("x",  "dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|dd;dd|streams", "max. parameters exceeded");
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
        TEST_CI("acgt", "format_sequence(firsttab=5,tab=5,width=1,numleft=1)",
                "1    a\n"
                "2    c\n"
                "3    g\n"
                "4    t");
        TEST_CI("acgt", "format_sequence(firsttab=0,tab=0,width=2,gap=1)",
                "a c\n"
                "g t");
        TEST_CI("acgt",     "format_sequence(firsttab=0,tab=0,width=4,gap=1)", "a c g t");
        TEST_CI("acgt",     "format_sequence(firsttab=0,tab=0,width=4,gap=2)", "ac gt");
        TEST_CI("acgtacgt", "format_sequence(firsttab=0,width=10,gap=4)",      "acgt acgt");
        TEST_CI("acgtacgt", "format_sequence(firsttab=1,width=10,gap=4)",      " acgt acgt");

        TEST_CI_ERROR_CONTAINS("acgt", "format_sequence(nl=c)",     "Unknown Parameter 'nl=c' in command 'format_sequence'");
        TEST_CI_ERROR_CONTAINS("acgt", "format_sequence(forcenl=)", "Unknown Parameter 'forcenl=' in command 'format_sequence'");
        // TEST_CI_ERROR("acgt", "format(width=0)", "should_raise_some_error"); // @@@ crashes

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
        TEST_CI("The quick brown fox", "srt(\"quick=lazy:brown fox=dog\")", "The lazy dog");
        TEST_CI__BROKEN("The quick brown fox", "srt(quick=lazy:brown fox=dog)", "The lazy dog"); // @@@ parsing problem ?
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

        TEST_CI_ERROR(NULL, "whatever", "ARB ERROR: Can't read this DB entry as string"); // here gb_data is the species container

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

#endif // UNIT_TESTS
