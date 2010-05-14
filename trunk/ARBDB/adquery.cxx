// =============================================================== //
//                                                                 //
//   File      : adquery.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <cctype>

#include "gb_aci.h"
#include "gb_comm.h"
#include "gb_index.h"
#include "gb_key.h"
#include "gb_localdata.h"
#include "gb_ta.h"


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
            if (d == *(double*)val) equal = true;
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

GBDATA *gb_search(GBDATA * gbd, const char *str, GB_TYPES create, int internflag)
{
    /* finds a hierarchical key,
       if create != GB_FIND(==0), then create the key
       force types if ! internflag
    */

    char   *s1, *s2;
    GBDATA *gbp, *gbsp;
    int     len;
    int     separator = 0;
    char    buffer[GB_PATH_MAX];

    GB_TEST_TRANSACTION(gbd);
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
    GB_TEST_TRANSACTION(gbd);
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
    GB_TEST_TRANSACTION(gbd);
    return gb_search_marked(gbc, key_quark, (int)gbd->index+1, skip_over);
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

char *gbs_search_second_x(const char *str)
{
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

#if defined(DEVEL_RALF)
#warning rewrite GB_command_interpreter (error+ressource handling)
#endif // DEVEL_RALF

char *GB_command_interpreter(GBDATA *gb_main, const char *str, const char *commands, GBDATA *gbd, const char *default_tree_name) {
    /* simple command interpreter returns NULL on error (which should be exported in that case)
     * if first character is == ':' run string parser
     * if first character is == '/' run regexpr
     * else command interpreter
     */
    int           strmalloc = 0;
    int           len;
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
    len = strlen(commands)+1;
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
                GBL *h;
                for (i=0; i<argcinput; i++) {
                    if (orig[i].str)    free(orig[i].str);
                }
                memset((char*)orig, 0, sizeof(GBL)*GBL_MAX_ARGUMENTS);
                argcinput = 0;

                h = out;            // swap orig and out
                out = orig;
                orig = h;

                argcinput = argcout;
                argcout = 0;
            }

        }
    }
    for (i=0; i<argcinput; i++) {
        if (orig[i].str) free((char *)(orig[i].str));
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

