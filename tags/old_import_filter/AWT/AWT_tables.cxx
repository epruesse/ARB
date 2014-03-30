// ================================================================ //
//                                                                  //
//   File      : AWT_tables.cxx                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "awt.hxx"
#include "awtlocal.hxx"
#include "awt_sel_boxes.hxx"

#include <aw_window.hxx>
#include <aw_awar.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <arbdbt.h>

static void ad_table_field_reorder_cb(AW_window *aws, awt_table *awtt) {
    char *source = aws->get_root()->awar(awtt->awar_field_reorder_source)->read_string();
    char *dest   = aws->get_root()->awar(awtt->awar_field_reorder_dest)  ->read_string();

    GB_ERROR  error    = GB_begin_transaction(awtt->gb_main);
    GBDATA   *gb_table = NULL;
    
    if (!error) {
        gb_table = GBT_open_table(awtt->gb_main, awtt->table_name, true);
        if (!gb_table) error = GBS_global_string("Table '%s' does not exist", awtt->table_name);
    }

    if (!error) {
        GBDATA *gb_source = GBT_find_table_field(gb_table, source);
        GBDATA *gb_dest   = GBT_find_table_field(gb_table, dest);

        if (!gb_source || !gb_dest) error    = "Please select two valid fields";
        else if (gb_dest == gb_source) error = "Please select two different fields";
        else {
            GBDATA *gb_fields = GB_get_father(gb_source);
            int     nitems    = 0;

            for (GBDATA *gb_cnt=GB_child(gb_fields); gb_cnt; gb_cnt = GB_nextChild(gb_cnt)) {
                nitems++;
            }

            GBDATA **new_order = new GBDATA *[nitems];
            nitems             = 0;
            for (GBDATA *gb_cnt=GB_child(gb_fields); gb_cnt; gb_cnt = GB_nextChild(gb_cnt)) {
                if (gb_cnt != gb_source) {
                    new_order[nitems++] = gb_cnt;
                    if (gb_cnt == gb_dest) {
                        new_order[nitems++] = gb_source;
                    }
                }
            }
            GB_ERROR warning = GB_resort_data_base(awtt->gb_main, new_order, nitems);
            if (warning) aw_message(warning);

            delete [] new_order;
        }
    }

    GB_end_transaction_show_error(awtt->gb_main, error, aw_message);

    free(dest);
    free(source);
}

static AW_window *create_ad_table_field_reorder_window(AW_root *root, awt_table *awtt)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "REORDER_FIELDS", "REORDER FIELDS");
    aws->load_xfig("ad_kreo.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"spaf_reorder.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("doit");
    aws->button_length(0);
    aws->callback((AW_CB1)ad_table_field_reorder_cb, (AW_CL)awtt);
    aws->help_text("spaf_reorder.hlp");
    aws->create_button("MOVE_TO_NEW_POSITION", "MOVE  SELECTED LEFT  ITEM\nAFTER SELECTED RIGHT ITEM", "P");

    aws->at("source");
    awt_create_selection_list_on_table_fields(awtt->gb_main, aws, awtt->table_name, awtt->awar_field_reorder_source);

    aws->at("dest");
    awt_create_selection_list_on_table_fields(awtt->gb_main, aws, awtt->table_name, awtt->awar_field_reorder_dest);

    return (AW_window *)aws;
}

static void awt_table_field_hide_cb(AW_window *aws, awt_table *awtt) {
    GB_begin_transaction(awtt->gb_main);
    GB_ERROR  error    = 0;
    GBDATA   *gb_table = GBT_open_table(awtt->gb_main, awtt->table_name, true);

    if (!gb_table) {
        error = GBS_global_string("Table '%s' does not exist", awtt->table_name);
    }
    else {
        char   *source    = aws->get_root()->awar(awtt->awar_selected_field)->read_string();
        GBDATA *gb_source = GBT_find_table_field(gb_table, source);

        if (!gb_source) error = "Please select an item you want to delete";
        else error            = GB_delete(gb_source);

        free(source);
    }
    GB_end_transaction_show_error(awtt->gb_main, error, aw_message);
}

static void awt_table_field_delete_cb(AW_window *aws, awt_table *awtt) {
    GB_begin_transaction(awtt->gb_main);

    GB_ERROR  error    = 0;
    GBDATA   *gb_table = GBT_open_table(awtt->gb_main, awtt->table_name, true);
    if (!gb_table) {
        error = GBS_global_string("Table '%s' does not exist", awtt->table_name);
    }
    else {
        char   *source    = aws->get_root()->awar(awtt->awar_selected_field)->read_string();
        GBDATA *gb_source = GBT_find_table_field(gb_table, source);

        if (!gb_source) error = "Please select an item you want to delete";
        else error            = GB_delete(gb_source);

        for (GBDATA *gb_table_entry = GBT_first_table_entry(gb_table);
             gb_table_entry && !error;
             gb_table_entry = GBT_next_table_entry(gb_table_entry))
        {
            GBDATA *gb_table_entry_field;
            while (!error && (gb_table_entry_field = GB_search(gb_table_entry, source, GB_FIND))) {
                error = GB_delete(gb_table_entry_field);
            }
        }
        free(source);
    }

    GB_end_transaction_show_error(awtt->gb_main, error, aw_message);
}


static void ad_table_field_create_cb(AW_window *aws, awt_table *awtt) {
    GB_push_transaction(awtt->gb_main);
    char *name = aws->get_root()->awar(awtt->awar_field_new_name)->read_string();
    GB_ERROR error = GB_check_key(name);
    GB_ERROR error2 = GB_check_hkey(name);
    if (error && !error2) {
        aw_message("Warning: Your key contain a '/' character,\n"
                   "    that means it is a hierarchical key");
        error = 0;
    }
    GBDATA *gb_table = GBT_open_table(awtt->gb_main, awtt->table_name, true);
    if (gb_table) {
        GB_TYPES type = (GB_TYPES)aws->get_root()->awar(awtt->awar_field_new_type)->read_int();
        if (!error) {
            GBDATA *gb_table_field = GBT_open_table_field(gb_table, name, type);
            if (!gb_table_field) error = GB_await_error();
        }
    }
    else {
        error = GBS_global_string("Table '%s' does not exist", awtt->table_name);
    }
    aws->hide_or_notify(error);

    free(name);
    GB_pop_transaction(awtt->gb_main);
}

static AW_window *create_ad_table_field_create_window(AW_root *root, awt_table *awtt) {
    static AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "CREATE_FIELD", "CREATE A NEW FIELD");
    aws->load_xfig("ad_fcrea.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");


    aws->at("input");
    aws->label("FIELD NAME");
    aws->create_input_field(awtt->awar_field_new_name, 15);

    aws->at("type");
    aws->create_toggle_field(awtt->awar_field_new_type, "FIELD TYPE", "F");
    aws->insert_toggle("Ascii Text", "S",       (int)GB_STRING);
    aws->insert_toggle("Link", "L",         (int)GB_LINK);
    aws->insert_toggle("Rounded Numerical", "N", (int)GB_INT);
    aws->insert_toggle("Numerical", "R",    (int)GB_FLOAT);
    aws->insert_toggle("MASK = 01 Text", "0",   (int)GB_BITS);
    aws->update_toggle_field();


    aws->at("ok");
    aws->callback((AW_CB1)ad_table_field_create_cb, (AW_CL)awtt);
    aws->create_button("CREATE", "CREATE", "C");

    return (AW_window *)aws;
}

awt_table::awt_table(GBDATA *igb_main, AW_root *awr, const char *itable_name) {
    gb_main    = igb_main;
    GB_transaction tscope(gb_main);
    table_name = strdup(itable_name);

    char *tname = GBS_string_2_key(table_name);
    
    awar_field_reorder_source = GBS_global_string_copy(AWAR_TABLE_FIELD_REORDER_SOURCE_TEMPLATE, tname);
    awar_field_reorder_dest   = GBS_global_string_copy(AWAR_TABLE_FIELD_REORDER_DEST_TEMPLATE, tname);
    awar_field_rem            = GBS_global_string_copy(AWAR_TABLE_FIELD_REM_TEMPLATE, tname);
    awar_field_new_name       = GBS_global_string_copy(AWAR_TABLE_FIELD_NEW_NAME_TEMPLATE, tname);
    awar_field_new_type       = GBS_global_string_copy(AWAR_TABLE_FIELD_NEW_TYPE_TEMPLATE, tname);
    awar_selected_field       = GBS_global_string_copy(AWAR_TABLE_SELECTED_FIELD_TEMPLATE, tname);

    awr->awar_string(awar_field_reorder_source, "");
    awr->awar_string(awar_field_reorder_dest, "");
    awr->awar_string(awar_field_new_name, "");
    awr->awar_int(awar_field_new_type, GB_STRING);
    awr->awar_string(awar_field_rem, "No comment");
    awr->awar_string(awar_selected_field, "");

    free(tname);
}

awt_table::~awt_table() {
    free(table_name);
    free(awar_field_reorder_source);
    free(awar_field_reorder_dest);
    free(awar_field_new_name);
    free(awar_field_rem);
    free(awar_selected_field);
}

static void   awt_map_table_field_rem(AW_root *aw_root, awt_table *awtt) {
    GB_transaction tscope(awtt->gb_main);
    GBDATA *gb_table = GBT_open_table(awtt->gb_main, awtt->table_name, true);
    if (!gb_table) {
        aw_root->awar(awtt->awar_field_rem)->unmap();
        return;
    }
    char *field_name  = aw_root->awar(awtt->awar_selected_field)->read_string();
    GBDATA *gb_table_field = GBT_find_table_field(gb_table, field_name);
    if (!gb_table_field) {
        delete field_name;
        aw_root->awar(awtt->awar_field_rem)->unmap();
        return;
    }
    GBDATA *gb_desc = GB_search(gb_table_field, "description", GB_STRING);
    aw_root->awar(awtt->awar_field_rem)->map(gb_desc);
}

static void create_ad_table_field_admin(AW_window *aww, GBDATA *gb_main, const char *tname) {
    static GB_HASH *table_to_win_hash = GBS_create_hash(256, GB_MIND_CASE);
    AW_root        *aw_root           = aww->get_root();
    char           *table_name;
    if (tname) {
        table_name = strdup(tname);
    }
    else {
        table_name = aw_root->awar(AWAR_TABLE_NAME)->read_string();
    }

    AW_window_simple *aws = (AW_window_simple *)GBS_read_hash(table_to_win_hash, table_name);
    if (!aws) {
        awt_table *awtt = new awt_table(gb_main, aw_root, table_name);
        aws = new AW_window_simple;
        const char *table_header = GBS_global_string("TABLE_ADMIN_%s", table_name);
        aws->init(aw_root, table_header, table_header);

        aws->load_xfig("ad_table_fields.fig");

        aws->callback(AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(AW_POPUP_HELP, (AW_CL)"tableadm.hlp");
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        aws->at("table_name");
        aws->create_button(NULL, table_name, "A");

        aws->button_length(13);

        aws->at("delete");
        aws->callback((AW_CB1)awt_table_field_delete_cb, (AW_CL)awtt);
        aws->create_button("DELETE", "DELETE", "D");

        aws->at("hide");
        aws->callback((AW_CB1)awt_table_field_hide_cb, (AW_CL)awtt);
        aws->create_button("HIDE", "HIDE", "D");

        aws->at("create");
        aws->callback(AW_POPUP, (AW_CL)create_ad_table_field_create_window, (AW_CL)awtt);
        aws->create_button("CREATE", "CREATE", "C");

        aws->at("reorder");
        aws->callback(AW_POPUP, (AW_CL)create_ad_table_field_reorder_window, (AW_CL)awtt);
        aws->create_button("REORDER", "REORDER", "R");

        aws->at("list");
        awt_create_selection_list_on_table_fields(gb_main, (AW_window *)aws, table_name, awtt->awar_selected_field);

        aws->at("rem");
        aws->create_text_field(awtt->awar_field_rem);

        awt_map_table_field_rem(aw_root, awtt);
        aw_root->awar(awtt->awar_selected_field)->add_callback((AW_RCB1)awt_map_table_field_rem, (AW_CL)awtt);
    }

    aws->activate();
    free(table_name);
}

// --------------------
//      table admin


// #define AWAR_TABLE_IMPORT "tmp/ad_table/import_table"

static void table_vars_callback(AW_root *aw_root, GBDATA *gb_main)     // Map table vars to display objects
{
    GB_push_transaction(gb_main);
    char *tablename = aw_root->awar(AWAR_TABLE_NAME)->read_string();
    GBDATA *gb_table = GBT_open_table(gb_main, tablename, true);
    if (!gb_table) {
        aw_root->awar(AWAR_TABLE_REM)->unmap();
    }
    else {
        GBDATA *table_rem = GB_search(gb_table, "description",  GB_STRING);
        aw_root->awar(AWAR_TABLE_REM)->map(table_rem);
    }
    char *fname = GBS_string_eval(tablename, "*=*1.table:table_*=*1", 0);
    aw_root->awar(AWAR_TABLE_EXPORT "/file_name")->write_string(fname); // create default file name
    delete fname;
    GB_pop_transaction(gb_main);
    free(tablename);
}



static void table_rename_cb(AW_window *aww, GBDATA *gb_main) {
    GB_ERROR  error  = 0;
    char     *source = aww->get_root()->awar(AWAR_TABLE_NAME)->read_string();
    char     *dest   = aww->get_root()->awar(AWAR_TABLE_DEST)->read_string();

    GB_begin_transaction(gb_main);
    GBDATA *gb_table_dest = GBT_open_table(gb_main, dest, true);
    if (gb_table_dest) {
        error = "Table already exists";
    }
    else {
        GBDATA *gb_table = GBT_open_table(gb_main, source, true);
        if (gb_table) {
            GBDATA *gb_name = GB_search(gb_table, "name", GB_STRING);

            if (!gb_name) error = GB_await_error();
            else error          = GB_write_string(gb_name, dest);
        }
    }
    GB_end_transaction_show_error(gb_main, error, aw_message);

    free(source);
    free(dest);
}

static void table_copy_cb(AW_window *aww, GBDATA *gb_main) {
    GB_ERROR  error  = 0;
    char     *source = aww->get_root()->awar(AWAR_TABLE_NAME)->read_string();
    char     *dest   = aww->get_root()->awar(AWAR_TABLE_DEST)->read_string();

    GB_begin_transaction(gb_main);
    GBDATA *gb_table_dest = GBT_open_table(gb_main, dest, true);
    if (gb_table_dest) {
        error = "Table already exists";
    }
    else {
        GBDATA *gb_table = GBT_open_table(gb_main, source, true);
        if (gb_table) {
            GBDATA *gb_table_data = GB_entry(gb_main, "table_data");
            gb_table_dest = GB_create_container(gb_table_data, "table");
            error = GB_copy(gb_table_dest, gb_table);
            if (!error) {
                GBDATA *gb_name = GB_search(gb_table_dest, "name", GB_STRING);
                error = GB_write_string(gb_name, dest);
            }
        }
    }
    GB_end_transaction_show_error(gb_main, error, aw_message);

    free(source);
    free(dest);
}
static void table_create_cb(AW_window *aww, GBDATA *gb_main) {
    char     *dest  = aww->get_root()->awar(AWAR_TABLE_DEST)->read_string();
    GB_ERROR  error = GB_begin_transaction(gb_main);
    if (!error) {
        error = GB_check_key(dest);
        if (!error) {
            GBDATA *gb_table     = GBT_open_table(gb_main, dest, false);
            if (!gb_table) error = GB_await_error();
        }
    }
    error = GB_end_transaction(gb_main, error);
    aww->hide_or_notify(error);
    free(dest);
}

static AW_window *create_table_rename_window(AW_root *root, GBDATA *gb_main)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "RENAME_TABLE", "TABLE RENAME");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the table");

    aws->at("input");
    aws->create_input_field(AWAR_TABLE_DEST, 15);

    aws->at("ok");
    aws->callback((AW_CB1)table_rename_cb, (AW_CL)gb_main);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static AW_window *create_table_copy_window(AW_root *root, GBDATA *gb_main)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "COPY_TABLE", "TABLE COPY");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the new table");

    aws->at("input");
    aws->create_input_field(AWAR_TABLE_DEST, 15);

    aws->at("ok");
    aws->callback((AW_CB1)table_copy_cb, (AW_CL)gb_main);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}
static AW_window *create_table_create_window(AW_root *root, GBDATA *gb_main)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "CREATE_TABLE", "TABLE CREATE");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the new table");

    aws->at("input");
    aws->create_input_field(AWAR_TABLE_DEST, 15);

    aws->at("ok");
    aws->callback((AW_CB1)table_create_cb, (AW_CL)gb_main);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}
static void awt_table_delete_cb(AW_window *aww, GBDATA *gb_main) {
    GB_ERROR  error  = 0;
    char     *source = aww->get_root()->awar(AWAR_TABLE_NAME)->read_string();

    GB_begin_transaction(gb_main);
    GBDATA *gb_table = GBT_open_table(gb_main, source, true);
    if (gb_table) {
        error = GB_delete(gb_table);
    }
    else {
        error = "Please select a table first";
    }
    GB_end_transaction_show_error(gb_main, error, aw_message);

    free(source);
}

static void create_tables_var(GBDATA *gb_main, AW_root *aw_root) {
    aw_root->awar_string(AWAR_TABLE_NAME);
    aw_root->awar_string(AWAR_TABLE_DEST);
    aw_root->awar_string(AWAR_TABLE_REM, "no rem");

    AW_create_fileselection_awars(aw_root, AWAR_TABLE_EXPORT, "", "table", "tablefile");

    AW_create_fileselection_awars(aw_root, AWAR_TABLE_IMPORT, "", "table", "tablefile");
    aw_root->awar_string(AWAR_TABLE_IMPORT "/table_name", "table_");

    aw_root->awar(AWAR_TABLE_NAME)->add_callback((AW_RCB1)table_vars_callback, (AW_CL)gb_main);
    table_vars_callback(aw_root, gb_main);
}

AW_window *AWT_create_tables_admin_window(AW_root *aw_root, GBDATA *gb_main)
{
    static AW_window_simple *aws = 0;

    if (aws) return aws;
    GB_transaction tscope(gb_main);
    create_tables_var(gb_main, aw_root);

    aws = new AW_window_simple;
    aws->init(aw_root, "TABLE_ADMIN", "TABLE ADMIN");
    aws->load_xfig("ad_table_admin.fig");

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"tableadm.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->button_length(13);

    aws->at("delete");
    aws->callback((AW_CB1)awt_table_delete_cb, (AW_CL)gb_main);
    aws->create_button("DELETE", "DELETE", "D");

    aws->at("rename");
    aws->callback(AW_POPUP, (AW_CL)create_table_rename_window, (AW_CL)gb_main);
    aws->create_button("RENAME", "RENAME", "R");

    aws->at("copy");
    aws->callback(AW_POPUP, (AW_CL)create_table_copy_window, (AW_CL)gb_main);
    aws->create_button("COPY", "COPY", "C");

    aws->at("cmp");
    aws->callback(AW_POPUP, (AW_CL)create_table_create_window, (AW_CL)gb_main);
    aws->create_button("CREATE", "CREATE", "C");

    aws->at("export");
    aws->callback((AW_CB)create_ad_table_field_admin, (AW_CL)gb_main, 0);
    aws->create_button("ADMIN", "ADMIN", "C");

    aws->at("list");
    awt_create_selection_list_on_tables(gb_main, (AW_window *)aws, AWAR_TABLE_NAME);

    aws->at("rem");
    aws->create_text_field(AWAR_TABLE_REM);

    return (AW_window *)aws;
}