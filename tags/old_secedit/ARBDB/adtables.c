#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adlocal.h"
/* #include "arbdb.h" */
#include "arbdbt.h"


/* *************** tables, for ARB BIO storage *******************

 *  hierarchical organisation:


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

GBDATA *gbt_table_link_follower(GBDATA *gb_main, GBDATA *gb_link, const char *link) {
    GBDATA *gb_table;
    char save;
    char *sep;
    GBUSE(gb_main);
    GBUSE(gb_link);
    sep = strchr(link,':');
    if (!sep){
        GB_export_error("Link '%s' is missing second ':' tag", link);
        return 0;
    }
    save = *sep;
    *sep = 0;
    gb_table = GBT_open_table(gb_main,link,1);
    *sep = save;

    if (!gb_table){
        GB_export_error("Table '%s' does not exist",link);
        return 0;
    }
    return GBT_find_table_entry(gb_table,sep+1);
}

GB_ERROR GBT_install_table_link_follower(GBDATA *gb_main){
    GB_install_link_follower(gb_main,"T",gbt_table_link_follower);
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

    static void g_bt_table_deleted(GBDATA *gb_table,int *clientdata, GB_CB_TYPE gbtype){
        GB_MAIN_TYPE *Main = gb_get_main_during_cb();
        GBUSE(gb_table);
        GBUSE(gbtype);
        GBUSE(clientdata);
        GBS_free_hash(Main->table_hash);
        Main->table_hash = GBS_create_hash(256,0);
    }

#ifdef __cplusplus
}
#endif

GBDATA *GBT_open_table(GBDATA *gb_table_root,const char *table_name, GB_BOOL read_only){
    /** open a table. This routines is optimized to look for existing tables */
    GBDATA *gb_table;
    GBDATA *gb_table_name;
    GBDATA *gb_table_description;
    GBDATA *gb_table_data;
    GBDATA *gb_table_entries;
    GBDATA *gb_table_fields;
    GBDATA *gb_table_name_field;
    GB_MAIN_TYPE *Main = GB_MAIN(gb_table_root);
    gb_table = (GBDATA *)GBS_read_hash(Main->table_hash,table_name);
    if (gb_table) return gb_table;

    gb_table_data = GB_search(gb_table_root,"table_data",GB_CREATE_CONTAINER);
    GB_create_index(gb_table_data,"name",256);

    gb_table_name = GB_find(gb_table_data,"name",table_name,down_2_level);
    if (gb_table_name) return GB_get_father(gb_table_name);
    if (read_only) return 0;

                /* now lets create the table */
    gb_table = GB_create_container(gb_table_data,"table");      GB_add_callback(gb_table,GB_CB_DELETE,g_bt_table_deleted,0);
    gb_table_name = GB_create(gb_table,"name",GB_STRING);
    gb_table_description = GB_create(gb_table,"description",GB_STRING);
    GB_write_string(gb_table_name,table_name);          GB_write_security_levels(gb_table_name,0,7,7); /* nether delete or change the name */
    GB_write_string(gb_table_description,"No description");
    gb_table_entries = GB_create_container(gb_table,"entries"); GB_write_security_levels(gb_table_entries,0,0,7); /* nether delete this */
    gb_table_fields = GB_create_container(gb_table,"fields");   GB_write_security_levels(gb_table_fields,0,0,7); /* nether intended to be deleted */
    gb_table_name_field = GBT_open_table_field(gb_table,"name",GB_STRING); /* for the id */
    GB_write_security_levels(gb_table_name_field,0,0,7); /* Never delete name field */
    return gb_table;
}

GBDATA *GBT_first_table(GBDATA *gb_main){
    GBDATA *gb_table_data;
    GBDATA *gb_table;
    gb_table_data = GB_search(gb_main,"table_data",GB_CREATE_CONTAINER);
    GB_create_index(gb_table_data,"name",256);
    gb_table = GB_find(gb_table_data,"table",0,down_level);
    return gb_table;
}

GBDATA *GBT_next_table(GBDATA *gb_table){
    return GB_find(gb_table,"table",0,this_level |search_next);
}



GBDATA *GBT_first_table_entry(GBDATA *gb_table){
    GBDATA *gb_entries = GB_find(gb_table,"entries",0,down_level);
    return GB_find(gb_entries,"entry",0,down_level);
}

GBDATA *GBT_first_marked_table_entry(GBDATA *gb_table){
    GBDATA *gb_entries = GB_find(gb_table,"entries",0,down_level);
    return GB_first_marked(gb_entries,"entry");
}

GBDATA *GBT_next_table_entry(GBDATA *gb_table_entry){
    return GB_find(gb_table_entry,"entry",0,this_level |search_next);
}

GBDATA *GBT_next_marked_table_entry(GBDATA *gb_table_entry){
    return GB_next_marked(gb_table_entry,"entry");
}

GBDATA *GBT_find_table_entry(GBDATA *gb_table,const char *id){
    GBDATA *gb_entries = GB_find(gb_table,"entries",0,down_level);
    GBDATA *gb_entry_name = GB_find(gb_entries,"name",id,down_2_level);
    if (!gb_entry_name) return 0;
    return GB_get_father(gb_entry_name);
}

GBDATA *GBT_open_table_entry(GBDATA *gb_table, const char *id){
    GBDATA *gb_entries = GB_find(gb_table,"entries",0,down_level);
    GBDATA *gb_entry_name = GB_find(gb_entries,"name",id,down_2_level);
    GBDATA *gb_entry;
    if (gb_entry_name) GB_get_father(gb_entry_name);
    gb_entry = GB_create_container(gb_entries,"entry");
    gb_entry_name = GB_create(gb_entry,"name",GB_STRING);
    GB_write_string(gb_entry_name,id);
    return gb_entry;
}

GBDATA *GBT_first_table_field(GBDATA *gb_table){
    GBDATA *gb_fields = GB_find(gb_table,"fields",0,down_level);
    return GB_find(gb_fields,"field",0,down_level);
}

GBDATA *GBT_first_marked_table_field(GBDATA *gb_table){
    GBDATA *gb_fields = GB_find(gb_table,"fields",0,down_level);
    return GB_first_marked(gb_fields,"field");
}
GBDATA *GBT_next_table_field(GBDATA *gb_table_field){
    return GB_find(gb_table_field,"field",0,this_level |search_next);
}

GBDATA *GBT_next_marked_table_field(GBDATA *gb_table_field){
    return GB_next_marked(gb_table_field,"field");
}

GBDATA *GBT_find_table_field(GBDATA *gb_table,const char *id){
    GBDATA *gb_fields = GB_find(gb_table,"fields",0,down_level);
    GBDATA *gb_field_name = GB_find(gb_fields,"name",id,down_2_level);
    if (!gb_field_name) return 0;
    return GB_get_father(gb_field_name);
}

GB_TYPES GBT_get_type_of_table_entry_field(GBDATA *gb_table,const char *fieldname){
    GBDATA *gb_fields = GB_find(gb_table,"fields",0,down_level);
    GBDATA *gb_field_name = GB_find(gb_fields,"name",fieldname,down_2_level);
    GBDATA *gb_field_type;
    if (!gb_field_name) return GB_NONE;
    gb_field_type = GB_find(gb_field_name,"type",0,down_level);
    return (GB_TYPES) GB_read_int(gb_field_type);
}

GB_ERROR GBT_savely_write_table_entry_field(GBDATA *gb_table,GBDATA *gb_entry, const char *fieldname,const char *value_in_ascii_format){
    GBDATA *gb_entry_field;
    GB_TYPES type = GBT_get_type_of_table_entry_field(gb_table,fieldname);
    if (type == GB_NONE){
        return GB_export_error("There is no field description '%s' for your table", fieldname);
    }
    gb_entry_field = GB_search(gb_entry,"fieldname",type);
    return GB_write_as_string(gb_entry_field,value_in_ascii_format);
}

GBDATA *GBT_open_table_field(GBDATA *gb_table, const char *fieldname, GB_TYPES type_of_field){
    GBDATA *gb_table_field = GBT_find_table_field(gb_table, fieldname);
    GBDATA *gb_table_field_name;
    GBDATA *gb_table_field_type;
    GBDATA *gb_table_field_description;
    GBDATA *gb_fields;
    if (gb_table_field) return gb_table_field;

    gb_fields = GB_find(gb_table,"fields",0,down_level);
    gb_table_field = GB_create_container(gb_fields,"field");
    gb_table_field_name = GB_create(gb_table_field,"name",GB_STRING);
    GB_write_string(gb_table_field_name,fieldname); GB_write_security_levels(gb_table_field_name,0,7,7); /* never change this */
    gb_table_field_type = GB_create(gb_table_field,"type",GB_INT);
    GB_write_int(gb_table_field_type,type_of_field);    GB_write_security_levels(gb_table_field_type,0,7,7);
    gb_table_field_description = GB_create(gb_table_field,"description",GB_STRING);
    GB_write_string(gb_table_field_description,"No description yet");
    return gb_table_field;
}

