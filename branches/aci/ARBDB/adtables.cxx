// =============================================================== //
//                                                                 //
//   File      : adtables.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <ad_cb.h>

#include "gb_main.h"
#include "gb_data.h"


/* *************** tables, for ARB BIO storage *******************

 *  hierarchical organization:


 *  main:
 *      table_data:
 *          table:
 *              name    (indexed by default)
 *              entries
 *                  entry
 *                      name (which is an id)
 *                      ....
 *                  entry
 *                  ...
 *              fields
 *                  field
 *                      name
 *                      type
 *                      description
 *                  field
 *                  ....
 *             }
 *      }
 */

static GBDATA *GBT_find_table_entry(GBDATA *gb_table, const char *id) {
    GBDATA *gb_entries = GB_entry(gb_table, "entries");
    GBDATA *gb_entry_name = GB_find_string(gb_entries, "name", id, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
    if (!gb_entry_name) return NULL;
    return GB_get_father(gb_entry_name);
}

static GBDATA *gbt_table_link_follower(GBDATA *gb_main, GBDATA */*gb_link*/, const char *link) {
    GBDATA *gb_table;
    char save;
    char *sep;
    // GBUSE(gb_main);
    // GBUSE(gb_link);
    sep = const_cast<char*>(strchr(link, ':'));
    if (!sep) {
        GB_export_errorf("Link '%s' is missing second ':' tag", link);
        return NULL;
    }
    save = *sep;
    *sep = 0;
    gb_table = GBT_open_table(gb_main, link, 1);
    *sep = save;

    if (!gb_table) {
        GB_export_errorf("Table '%s' does not exist", link);
        return NULL;
    }
    return GBT_find_table_entry(gb_table, sep+1);
}

GB_ERROR GBT_install_table_link_follower(GBDATA *gb_main) {
    GB_install_link_follower(gb_main, "T", gbt_table_link_follower);
    return 0;
}

static void g_bt_table_deleted() {
    GB_MAIN_TYPE *Main = gb_get_main_during_cb();
    GBS_free_hash(Main->table_hash);
    Main->table_hash = GBS_create_hash(256, GB_MIND_CASE);
}

GBDATA *GBT_open_table(GBDATA *gb_table_root, const char *table_name, bool read_only) {
    // open a table. This routines is optimized to look for existing tables
    GBDATA *gb_table;
    GBDATA *gb_table_name;
    GBDATA *gb_table_description;
    GBDATA *gb_table_data;
    GBDATA *gb_table_entries;
    GBDATA *gb_table_fields;
    GBDATA *gb_table_name_field;

    GB_MAIN_TYPE *Main = GB_MAIN(gb_table_root);
    gb_table           = (GBDATA *)GBS_read_hash(Main->table_hash, table_name);
    if (gb_table) return gb_table;

    gb_table_data = GB_search(gb_table_root, "table_data", GB_CREATE_CONTAINER);
    ASSERT_NO_ERROR(GB_create_index(gb_table_data, "name", GB_IGNORE_CASE, 256));

    gb_table_name = GB_find_string(gb_table_data, "name", table_name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
    if (gb_table_name) return GB_get_father(gb_table_name);
    if (read_only) return NULL;

    // now lets create the table
    gb_table = GB_create_container(gb_table_data, "table");
    GB_add_callback(gb_table, GB_CB_DELETE, makeDatabaseCallback(g_bt_table_deleted));

    gb_table_name = GB_create(gb_table, "name", GB_STRING);
    GB_write_string(gb_table_name, table_name);
    GB_write_security_levels(gb_table_name, 0, 7, 7); // neither delete nor change the name

    gb_table_description = GB_create(gb_table, "description", GB_STRING);
    GB_write_string(gb_table_description, "No description");

    gb_table_entries = GB_create_container(gb_table, "entries");
    GB_write_security_levels(gb_table_entries, 0, 0, 7); // never delete this

    gb_table_fields = GB_create_container(gb_table, "fields");
    GB_write_security_levels(gb_table_fields, 0, 0, 7); // not intended to be deleted

    gb_table_name_field = GBT_open_table_field(gb_table, "name", GB_STRING); // for the id
    GB_write_security_levels(gb_table_name_field, 0, 0, 7); // never delete name field

    return gb_table;
}

GBDATA *GBT_first_table(GBDATA *gb_main) {
    GBDATA *gb_table_data;
    GBDATA *gb_table;
    gb_table_data = GB_search(gb_main, "table_data", GB_CREATE_CONTAINER);
    ASSERT_NO_ERROR(GB_create_index(gb_table_data, "name", GB_IGNORE_CASE, 256));
    gb_table = GB_entry(gb_table_data, "table");
    return gb_table;
}

GBDATA *GBT_next_table(GBDATA *gb_table) {
    gb_assert(GB_has_key(gb_table, "table"));
    return GB_nextEntry(gb_table);
}



GBDATA *GBT_first_table_entry(GBDATA *gb_table) {
    GBDATA *gb_entries = GB_entry(gb_table, "entries");
    return GB_entry(gb_entries, "entry");
}

GBDATA *GBT_next_table_entry(GBDATA *gb_table_entry) {
    gb_assert(GB_has_key(gb_table_entry, "entry"));
    return GB_nextEntry(gb_table_entry);
}

GBDATA *GBT_first_table_field(GBDATA *gb_table) {
    GBDATA *gb_fields = GB_entry(gb_table, "fields");
    return GB_entry(gb_fields, "field");
}

GBDATA *GBT_next_table_field(GBDATA *gb_table_field) {
    gb_assert(GB_has_key(gb_table_field, "field"));
    return GB_nextEntry(gb_table_field);
}

GBDATA *GBT_find_table_field(GBDATA *gb_table, const char *id) {
    GBDATA *gb_fields = GB_entry(gb_table, "fields");
    GBDATA *gb_field_name = GB_find_string(gb_fields, "name", id, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
    if (!gb_field_name) return NULL;
    return GB_get_father(gb_field_name);
}

GBDATA *GBT_open_table_field(GBDATA *gb_table, const char *fieldname, GB_TYPES type_of_field) {
    GBDATA *gb_table_field = GBT_find_table_field(gb_table, fieldname);
    GBDATA *gb_table_field_name;
    GBDATA *gb_table_field_type;
    GBDATA *gb_table_field_description;
    GBDATA *gb_fields;

    if (gb_table_field) return gb_table_field;

    gb_fields           = GB_entry(gb_table, "fields");
    gb_table_field      = GB_create_container(gb_fields, "field");
    gb_table_field_name = GB_create(gb_table_field, "name", GB_STRING);

    GB_write_string(gb_table_field_name, fieldname);
    GB_write_security_levels(gb_table_field_name, 0, 7, 7); // never change this

    gb_table_field_type = GB_create(gb_table_field, "type", GB_INT);

    GB_write_int(gb_table_field_type, type_of_field);
    GB_write_security_levels(gb_table_field_type, 0, 7, 7);

    gb_table_field_description = GB_create(gb_table_field, "description", GB_STRING);
    GB_write_string(gb_table_field_description, "No description yet");

    return gb_table_field;
}

