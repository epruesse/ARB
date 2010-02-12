// ================================================================ //
//                                                                  //
//   File      : AWT_sel_boxes.cxx                                  //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "awt.hxx"
#include "awt_sel_boxes.hxx"
#include "awt_item_sel_list.hxx"

#include <aw_awars.hxx>
#include <aw_file.hxx>

#include <ad_config.h>

#include <arbdbt.h>

#include <list>

using namespace std;


// --------------------------------------
//      selection boxes on alignments

class AWT_alignment_selection : public AW_DB_selection {
    char *ali_type_match;                           // filter for wanted alignments (GBS_string_eval command)
public:
    AWT_alignment_selection(AW_window *win_, AW_selection_list *sellist_, GBDATA *gb_presets, const char *ali_type_match_)
        : AW_DB_selection(win_, sellist_, gb_presets)
        , ali_type_match(nulldup(ali_type_match_))
    {}

    void fill() {
        GBDATA         *gb_presets = get_gbd();
        GB_transaction  ta(gb_presets);

        for (GBDATA *gb_alignment = GB_entry(gb_presets, "alignment");
             gb_alignment;
             gb_alignment = GB_nextEntry(gb_alignment))
        {
            char *alignment_type = GBT_read_string(gb_alignment, "alignment_type");
            char *alignment_name = GBT_read_string(gb_alignment, "alignment_name");
            char *str            = GBS_string_eval(alignment_type, ali_type_match, 0);

            if (!*str) insert_selection(alignment_name, alignment_name);
            free(str);
            free(alignment_type);
            free(alignment_name);
        }
        insert_default_selection("????", "????");
    }
};

void awt_create_selection_list_on_alignments(GBDATA *gb_main, AW_window *aws, const char *varname, const char *comm) {
    // Create selection lists on alignments
    //
    // if comm is set, then only insert alignments,
    // where 'comm' GBS_string_eval's the alignment type

    GBDATA *gb_presets;
    {
        GB_transaction ta(gb_main);
        gb_presets = GB_search(gb_main, "presets", GB_CREATE_CONTAINER);
    }
    AW_selection_list *sellist = aws->create_selection_list(varname, 0, "", 20, 3);
    (new AWT_alignment_selection(aws, sellist, gb_presets, comm))->refresh(); // belongs to window now
}


// ---------------------------------
//      selection boxes on trees

struct AWT_tree_selection: public AW_DB_selection {
    AWT_tree_selection(AW_window *win_, AW_selection_list *sellist_, GBDATA *gb_tree_data)
        : AW_DB_selection(win_, sellist_, gb_tree_data)
    {}

    void fill() {
        GBDATA         *gb_main = get_gb_main();
        GB_transaction  ta(gb_main);

        char **tree_names = GBT_get_tree_names(gb_main);
        if (tree_names) {
            int maxTreeNameLen = 0;
            for (char **tree = tree_names; *tree; tree++) {
                int len = strlen(*tree);
                if (len>maxTreeNameLen) maxTreeNameLen = len;
            }
            for (char **tree = tree_names; *tree; tree++) {
                const char *info = GBT_tree_info_string(gb_main, *tree, maxTreeNameLen);
                if (info) {
                    insert_selection(info, *tree);
                }
                else {
                    insert_selection(*tree, *tree);
                }
            }
            GBT_free_names(tree_names);
        }
        insert_default_selection("????", "????");
    }
};

void awt_create_selection_list_on_trees(GBDATA *gb_main, AW_window *aws, const char *varname) {
    GBDATA *gb_tree_data;
    {
        GB_transaction ta(gb_main);
        gb_tree_data = GB_search(gb_main, "tree_data", GB_CREATE_CONTAINER);
    }
    AW_selection_list *sellist = aws->create_selection_list(varname, 0, "", 40, 4);
    (new AWT_tree_selection(aws, sellist, gb_tree_data))->refresh(); // belongs to window now
}


// --------------------------------------
//      selection boxes on pt-servers

#define PT_SERVERNAME_LENGTH        23              // that's for buttons
#define PT_SERVERNAME_SELLIST_WIDTH 30              // this for lists
#define PT_SERVER_TRACKLOG_TIMER    10000           // every 10 seconds

class AWT_ptserver_selection : public AW_selection {
    typedef list<AWT_ptserver_selection*> PTserverSelections;
    
    static PTserverSelections ptserver_selections;
public:
    AWT_ptserver_selection(AW_window *win_, AW_selection_list *sellist_);

    void fill();

    static void refresh_all();
};

AWT_ptserver_selection::PTserverSelections AWT_ptserver_selection::ptserver_selections;

void AWT_ptserver_selection::fill() {
    const char * const *pt_servers = GBS_get_arb_tcp_entries("ARB_PT_SERVER*");

    int count = 0;
    while (pt_servers[count]) count++;

    for (int i=0; i<count; i++) {
        char *choice = GBS_ptserver_id_to_choice(i, 1);
        if (!choice) {
            aw_message(GB_await_error());
            break;
        }
        insert_selection(choice, (long)i);
        free(choice);
    }

    insert_default_selection("-undefined-", (long)-1);
}

void AWT_ptserver_selection::refresh_all() {
    PTserverSelections::iterator end = ptserver_selections.end();
    for (PTserverSelections::iterator pts_sel = ptserver_selections.begin(); pts_sel != end; ++pts_sel) {
        (*pts_sel)->refresh();
    }
}
void awt_refresh_all_pt_server_selection_lists() {
    AWT_ptserver_selection::refresh_all();
}
static void track_log_cb(AW_root *awr) {
    static long  last_ptserverlog_mod = 0;
    const char  *ptserverlog          = GBS_ptserver_logname();
    long         ptserverlog_mod      = GB_time_of_file(ptserverlog);

    if (ptserverlog_mod != last_ptserverlog_mod) {
#if defined(DEBUG)
        fprintf(stderr, "%s modified!\n", ptserverlog);
#endif // DEBUG
        AWT_ptserver_selection::refresh_all();
        last_ptserverlog_mod = ptserverlog_mod;
    }

    awr->add_timed_callback(PT_SERVER_TRACKLOG_TIMER, track_log_cb);
}

AWT_ptserver_selection::AWT_ptserver_selection(AW_window *win_, AW_selection_list *sellist_)
    : AW_selection(win_, sellist_)
{
    if (ptserver_selections.empty()) {
        // first pt server selection list -> install log tracker
        get_root()->add_timed_callback(PT_SERVER_TRACKLOG_TIMER, track_log_cb);
    }
    ptserver_selections.push_back(this);
}




static char *readable_pt_servername(int index, int maxlength) {
    char *fullname = GBS_ptserver_id_to_choice(index, 0);
    if (!fullname) {
#ifdef DEBUG
        printf("awar given to awt_create_selection_list_on_pt_servers() does not contain a valid index\n");
#endif
        return strdup("-undefined-");
    }

    int len = strlen(fullname);
    if (len <= maxlength) {
        return fullname;
    }

    int remove  = len-maxlength;
    fullname[0] = '.';
    fullname[1] = '.';
    strcpy(fullname+2, fullname+2+remove);

    return fullname;
}

static void update_ptserver_button(AW_root *aw_root, AW_CL cl_varname) {
    const char *varname              = (const char *)cl_varname;
    char       *awar_buttontext_name = GBS_global_string_copy("/tmp/%s_BUTTON", varname);
    char       *readable_name        = readable_pt_servername(aw_root->awar(varname)->read_int(), PT_SERVERNAME_LENGTH);

    aw_root->awar(awar_buttontext_name)->write_string(readable_name);

    free(readable_name);
    free(awar_buttontext_name);
}

static AW_window *awt_popup_selection_list_on_pt_servers(AW_root *aw_root, const char *varname) {
    AW_window_simple *aw_popup = new AW_window_simple;

    aw_popup->init(aw_root, "SELECT_PT_SERVER", "Select a PT-Server");
    aw_popup->auto_space(10, 10);

    aw_popup->at_newline();
    aw_popup->callback((AW_CB0)AW_POPDOWN);
    AW_selection_list *sellist = aw_popup->create_selection_list(varname, 0, "", PT_SERVERNAME_SELLIST_WIDTH, 20);

    aw_popup->at_newline();
    aw_popup->callback((AW_CB0)AW_POPDOWN);
    aw_popup->create_button("CLOSE", "CLOSE", "C");

    aw_popup->window_fit();
    aw_popup->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    (new AWT_ptserver_selection(aw_popup, sellist))->refresh();

    return aw_popup;
}

void awt_create_selection_list_on_pt_servers(AW_window *aws, const char *varname, bool popup) {
    if (popup) {
        AW_root *aw_root              = aws->get_root();
        char    *awar_buttontext_name = GBS_global_string_copy("/tmp/%s_BUTTON", varname);
        int      ptserver_index       = aw_root->awar(varname)->read_int();

        if (ptserver_index<0) { // fix invalid pt_server indices
            ptserver_index = 0;
            aw_root->awar(varname)->write_int(ptserver_index);
        }

        char  *readable_name = readable_pt_servername(ptserver_index, PT_SERVERNAME_LENGTH);
        AW_CL  cl_varname    = (AW_CL)strdup(varname); // make copy of awar_name for callbacks

        aw_root->awar_string(awar_buttontext_name, readable_name, AW_ROOT_DEFAULT);
        aw_root->awar(varname)->add_callback(update_ptserver_button, cl_varname);

        int old_button_length = aws->get_button_length();

        aws->button_length(PT_SERVERNAME_LENGTH+1);
        aws->callback(AW_POPUP, (AW_CL)awt_popup_selection_list_on_pt_servers, cl_varname);
        aws->create_button("CURR_PT_SERVER", awar_buttontext_name);

        aws->button_length(old_button_length);

        free(readable_name);
        free(awar_buttontext_name);
    }
    else {
        (new AWT_ptserver_selection(aws, aws->create_selection_list(varname)))->refresh();
    }
}

// ----------------------------------
//      selection boxes on tables


#if defined(DEVEL_RALF)
#warning derive awt_sel_list_for_tables from AW_DB_selection
#endif // DEVEL_RALF

struct awt_sel_list_for_tables {
    AW_window         *aws;
    GBDATA            *gb_main;
    AW_selection_list *id;
    const char        *table_name;
};

void awt_create_selection_list_on_tables_cb(GBDATA *dummy, struct awt_sel_list_for_tables *cbs) {
    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);
    GBDATA *gb_table;
    for (gb_table = GBT_first_table(cbs->gb_main);
         gb_table;
         gb_table = GBT_next_table(gb_table)) {

        GBDATA *gb_name = GB_entry(gb_table, "name");
        GBDATA *gb_description = GB_search(gb_table, "description", GB_STRING);
        if (!gb_name) continue;
        char *table_name = GB_read_string(gb_name);
        char *description = GB_read_string(gb_description);
        const char *info_text = GBS_global_string("%s: %s", table_name, description);
        cbs->aws->insert_selection(cbs->id, info_text, table_name);
        free(table_name);
        free(description);
    }
    cbs->aws->insert_default_selection(cbs->id, "", "");
    cbs->aws->update_selection_list(cbs->id);
}

void awt_create_selection_list_on_tables(GBDATA *gb_main, AW_window *aws, const char *varname)
{
    AW_selection_list *id;
    GBDATA  *gb_table_data;
    struct awt_sel_list_for_tables *cbs;
    GB_push_transaction(gb_main);

    id = aws->create_selection_list(varname, 0, "", 40, 8);
    cbs = new awt_sel_list_for_tables;
    cbs->aws = aws;
    cbs->gb_main = gb_main;
    cbs->id = id;

    awt_create_selection_list_on_tables_cb(0, cbs);

    gb_table_data = GB_search(gb_main, "table_data", GB_CREATE_CONTAINER);
    GB_add_callback(gb_table_data, GB_CB_CHANGED, (GB_CB)awt_create_selection_list_on_tables_cb, (int *)cbs);

    GB_pop_transaction(gb_main);
}

void awt_create_selection_list_on_table_fields_cb(GBDATA *dummy, struct awt_sel_list_for_tables *cbs) {
    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);
    GBDATA  *gb_table = GBT_open_table(cbs->gb_main, cbs->table_name, true); // read only
    GBDATA *gb_table_field;
    for (gb_table_field = GBT_first_table_field(gb_table);
         gb_table_field;
         gb_table_field = GBT_next_table_field(gb_table_field))
    {
        GBDATA *gb_name = GB_entry(gb_table_field, "name");
        GBDATA *gb_description = GB_search(gb_table_field, "description", GB_STRING);
        if (!gb_name) continue;
        char *table_name = GB_read_string(gb_name);
        char *description = GB_read_string(gb_description);
        const char *info_text = GBS_global_string("%s: %s", table_name, description);
        cbs->aws->insert_selection(cbs->id, info_text, table_name);
        free(table_name);
        free(description);
    }
    cbs->aws->insert_default_selection(cbs->id, "", "");
    cbs->aws->update_selection_list(cbs->id);
}

void awt_create_selection_list_on_table_fields(GBDATA *gb_main, AW_window *aws, const char *table_name, const char *varname) {
    // if tablename == 0 -> take fields from species table

    AW_selection_list              *id;
    struct awt_sel_list_for_tables *cbs;
    GB_push_transaction(gb_main);

    id = aws->create_selection_list(varname, 0, "", 40, 8);
    cbs = new awt_sel_list_for_tables;
    cbs->aws = aws;
    cbs->gb_main = gb_main;
    cbs->id = id;
    cbs->table_name = strdup(table_name);

    awt_create_selection_list_on_table_fields_cb(0, cbs);

    GBDATA  *gb_table = GBT_open_table(gb_main, table_name, true); // read only
    if (gb_table) {
        GB_add_callback(gb_table, GB_CB_CHANGED, (GB_CB)awt_create_selection_list_on_table_fields_cb, (int *)cbs);
    }
    GB_pop_transaction(gb_main);
}

// -------------------------------------------------
//      selection boxes on editor configurations

class AWT_configuration_selection : public AW_DB_selection {
public:
    AWT_configuration_selection(AW_window *win_, AW_selection_list *sellist_, GBDATA *gb_configuration_data)
        : AW_DB_selection(win_, sellist_, gb_configuration_data)
    {}

    void fill() {
        int    config_count;
        char **config = GBT_get_configuration_names_and_count(get_gb_main(), &config_count);

        if (config) {
            for (int c = 0; c<config_count; c++) insert_selection(config[c], config[c]);
            GBT_free_names(config);
        }
        insert_default_selection("????", "????");
    }
};

void awt_create_selection_list_on_configurations(GBDATA *gb_main, AW_window *aws, const char *varname) {
    GBDATA *gb_configuration_data;
    {
        GB_transaction ta(gb_main);
        gb_configuration_data = GB_search(gb_main, CONFIG_DATA_PATH, GB_CREATE_CONTAINER);
    }
    AW_selection_list *sellist = aws->create_selection_list(varname, 0, "", 40, 15);
    (new AWT_configuration_selection(aws, sellist, gb_configuration_data))->refresh();
}

char *awt_create_string_on_configurations(GBDATA *gb_main) {
    // returns semicolon-separated string containing configuration names
    // (or NULL if no configs exist)

    GB_push_transaction(gb_main);

    int    config_count;
    char **config = GBT_get_configuration_names_and_count(gb_main, &config_count);
    char  *result = 0;

    if (config) {
        GBS_strstruct *out = GBS_stropen(1000);
        for (int c = 0; c<config_count; c++) {
            if (c>0) GBS_chrcat(out, ';');
            GBS_strcat(out, config[c]);
        }
        result = GBS_strclose(out);
    }

    GBT_free_names(config);
    GB_pop_transaction(gb_main);
    return result;
}

// ----------------------
//      SAI selection


#if defined(DEVEL_RALF)
#warning derive awt_sel_list_for_sai from AW_DB_selection
#endif // DEVEL_RALF


struct awt_sel_list_for_sai {
    AW_window         *aws;
    GBDATA            *gb_main;
    AW_selection_list *id;
    char *(*filter_poc)(GBDATA *gb_ext, AW_CL);
    AW_CL              filter_cd;
    bool               add_selected_species;
};

void awt_create_selection_list_on_extendeds_update(GBDATA *dummy, void *cbsid) {
    /* update the selection box defined by awt_create_selection_list_on_extendeds
     *
     * useful only when filterproc is defined
     * (changes to SAIs will automatically callback this function)
     */

#if defined(DEVEL_RALF)
    printf("start awt_create_selection_list_on_extendeds_update\n"); // @@@
#endif // DEVEL_RALF

    struct awt_sel_list_for_sai *cbs = (struct awt_sel_list_for_sai *)cbsid;

    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);
    GB_transaction ta(cbs->gb_main);

    for (GBDATA *gb_extended = GBT_first_SAI(cbs->gb_main);
         gb_extended;
         gb_extended = GBT_next_SAI(gb_extended))
    {
        if (cbs->filter_poc) {
            char *res = cbs->filter_poc(gb_extended, cbs->filter_cd);
            if (res) {
                cbs->aws->insert_selection(cbs->id, res, GBT_read_name(gb_extended));
                free(res);
            }
        }
        else {
            const char *name     = GBT_read_name(gb_extended);
            GBDATA     *gb_group = GB_entry(gb_extended, "sai_group");

            if (gb_group) {
                const char *group          = GB_read_char_pntr(gb_group);
                char       *group_and_name = GBS_global_string_copy("[%s] %s", group, name);

                cbs->aws->insert_selection(cbs->id, group_and_name, name);
                free(group_and_name);
            }
            else {
                cbs->aws->insert_selection(cbs->id, name, name);
            }
        }
    }
    cbs->aws->sort_selection_list(cbs->id, 0, 0);

    if (cbs->add_selected_species) {
        GBDATA *gb_sel = GB_search(cbs->gb_main, AWAR_SPECIES_NAME, GB_STRING);
        char   *name   = GB_read_string(gb_sel);
        if (strlen(name)) {
            char *sname = (char *)calloc(1, strlen(name)+2);
            sprintf(sname+1, "%s", name);
            sname[0] = 1;
            char *text = (char *)GBS_global_string("Selected Species: '%s'", name);
            cbs->aws->insert_selection(cbs->id, text, sname);
            delete name;
        }
        delete name;
    }
    cbs->aws->insert_default_selection(cbs->id, "- none -", "none");
    cbs->aws->update_selection_list(cbs->id);

#if defined(DEVEL_RALF)
    printf("done  awt_create_selection_list_on_extendeds_update\n"); // @@@
#endif // DEVEL_RALF
}

void *awt_create_selection_list_on_extendeds(GBDATA *gb_main, AW_window *aws, const char *varname,
                                             char *(*filter_poc)(GBDATA *gb_ext, AW_CL), AW_CL filter_cd,
                                             bool add_sel_species)
{
    /* Selection list for all extendeds (SAIs)
     *
     * if filter_proc is set then show only those items on which
     * filter_proc returns a string (string must be a heap copy)
     */

    AW_selection_list           *id;
    struct awt_sel_list_for_sai *cbs;

    GB_push_transaction(gb_main);

    id  = aws->create_selection_list(varname, 0, "", 40, 4);
    cbs = new awt_sel_list_for_sai;

    cbs->aws                  = aws;
    cbs->gb_main              = gb_main;
    cbs->id                   = id;
    cbs->filter_poc           = filter_poc;
    cbs->filter_cd            = filter_cd;
    cbs->add_selected_species = add_sel_species;

    awt_create_selection_list_on_extendeds_update(0, (void *)cbs);

    GBDATA *gb_sai_data = GBT_get_SAI_data(gb_main);
    GB_add_callback(gb_sai_data, GB_CB_CHANGED, (GB_CB)awt_create_selection_list_on_extendeds_update, (int *)cbs);

    if (add_sel_species) {      // update box if another species is selected
        GBDATA *gb_sel = GB_search(gb_main, AWAR_SPECIES_NAME, GB_STRING);
        GB_add_callback(gb_sel, GB_CB_CHANGED, (GB_CB)awt_create_selection_list_on_extendeds_update, (int *)cbs);
    }
    GB_pop_transaction(gb_main);

    return (void *)cbs;
}




// ******************** selection boxes on saving selection lists ********************

void create_save_box_for_selection_lists_save(AW_window *aws, AW_CL selidcd, AW_CL basenamecd)
{
    AW_selection_list *selid       = (AW_selection_list *)selidcd;
    char              *awar_prefix = (char *)basenamecd;

    char    *bline_anz = GBS_global_string_copy("%s/line_anz", awar_prefix);
    AW_root *aw_root   = aws->get_root();
    long     lineanz   = aw_root->awar(bline_anz)->read_int();
    char    *filename  = AW_get_selected_fullname(aw_root, awar_prefix);

    GB_ERROR error = aws->save_selection_list(selid, filename, lineanz);

    if (!error) AW_refresh_fileselection(aw_root, awar_prefix);
    aws->hide_or_notify(error);
    free(filename);
    free(bline_anz);
}

AW_window *create_save_box_for_selection_lists(AW_root *aw_root, AW_CL selid)
{
    AW_selection_list *selection_list = (AW_selection_list*)selid;

    char *var_id         = GBS_string_2_key(selection_list->variable_name);
    char *awar_base_name = GBS_global_string_copy("tmp/save_box_sel_%s", var_id); // don't free (passed to callback)
    char *awar_line_anz  = GBS_global_string_copy("%s/line_anz", awar_base_name);
    {
        AW_create_fileselection_awars(aw_root, awar_base_name, ".", GBS_global_string("noname.list"), "list");
        aw_root->awar_int(awar_line_anz, 0, AW_ROOT_DEFAULT);
    }

    AW_window_simple *aws       = new AW_window_simple;
    char             *window_id = GBS_global_string_copy("SAVE_SELECTION_BOX_%s", var_id);

    aws->init(aw_root, window_id, "SAVE BOX");
    aws->load_xfig("sl_s_box.fig");

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("save");
    aws->highlight();
    aws->callback(create_save_box_for_selection_lists_save, selid, (AW_CL)awar_base_name); // loose ownership of awar_base_name!
    aws->create_button("SAVE", "SAVE", "S");

    aws->at("nlines");
    aws->create_option_menu(awar_line_anz, 0, "");
    aws->insert_default_option("all", "a", 0);
    aws->insert_option("50",    "a", 50);
    aws->insert_option("100",   "a", 100);
    aws->insert_option("500",   "a", 500);
    aws->insert_option("1000",  "a", 1000);
    aws->insert_option("5000",  "a", 5000);
    aws->insert_option("10000", "a", 10000);
    aws->update_option_menu();

    AW_create_fileselection(aws, awar_base_name);

    free(window_id);
    free(awar_line_anz);
    free(var_id);

    aws->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    return aws;
}

void AWT_load_list(AW_window *aww, AW_CL sel_id, AW_CL ibase_name)
{
    AW_selection_list * selid       = (AW_selection_list *)sel_id;
    char *basename = (char *)ibase_name;

    AW_root     *aw_root    = aww->get_root();
    GB_ERROR    error;

    char *filename = AW_get_selected_fullname(aw_root, basename);
    error          = aww->load_selection_list(selid, filename);

    if (error) aw_message(error);

    AW_POPDOWN(aww);

    delete filename;
}

AW_window *create_load_box_for_selection_lists(AW_root *aw_root, AW_CL selid)
{
    char base_name[100];
    sprintf(base_name, "tmp/load_box_sel_%li", (long)selid);

    AW_create_fileselection_awars(aw_root, base_name, ".", "list", "");

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "LOAD_SELECTION_BOX", "Load box");
    aws->load_xfig("sl_l_box.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("load");
    aws->highlight();
    aws->callback(AWT_load_list, selid, (AW_CL)strdup(base_name));
    aws->create_button("LOAD", "LOAD", "L");

    AW_create_fileselection(aws, base_name);

    aws->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    return aws;
}


void create_print_box_for_selection_lists(AW_window *aw_window, AW_CL selid) {
    AW_root *aw_root = aw_window->get_root();
    char *data = aw_window->get_selection_list_contents((AW_selection_list *)selid);
    AWT_create_ascii_print_window(aw_root, data, "no title");
    delete data;
}



AW_window *awt_create_load_box(AW_root *aw_root, const char *load_what, const char *file_extension, char **set_file_name_awar,
                               void (*callback)(AW_window*),
                               AW_window* (*create_popup)(AW_root *, AW_default))
{
    /* general purpose file selection box
     *
     * You can either provide a normal 'callback' or a 'create_popup'-callback
     * (the not-used callback has to be NULL)
     */


    char *base_name = GBS_global_string_copy("tmp/load_box_%s", load_what);

    AW_create_fileselection_awars(aw_root, base_name, ".", file_extension, "");

    if (set_file_name_awar) {
        *set_file_name_awar = GBS_global_string_copy("%s/file_name", base_name);
    }

    AW_window_simple *aws = new AW_window_simple;
    {
        char title[100];
        sprintf(title, "Load %s", load_what);
        aws->init(aw_root, title, title);
        aws->load_xfig("load_box.fig");
    }

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"");
    aws->create_button("HELP", "HELP");

    aws->at("go");
    aws->highlight();

    if (callback) {
        awt_assert(!create_popup);
        aws->callback((AW_CB0)callback);
    }
    else {
        awt_assert(create_popup);
        aws->callback((AW_CB1)AW_POPUP, (AW_CL)create_popup);
    }

    aws->create_button("LOAD", "LOAD", "L");

    AW_create_fileselection(aws, base_name);
    free(base_name);
    aws->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    return aws;
}

void awt_set_long(AW_window *aws, AW_CL varname, AW_CL value)   // set an awar
{
    aws->get_root()->awar((char *)varname)->write_int((long) value);
}

void awt_write_string(AW_window *aws, AW_CL varname, AW_CL value)   // set an awar
{
    aws->get_root()->awar((char *)varname)->write_string((char *)value);
}

void AWT_popup_select_species_field_window(AW_window *aww, AW_CL cl_awar_name, AW_CL cl_gb_main)
{
    static AW_window_simple *aws = 0;

    // everytime map selection awar to latest user awar:
    AW_root    *aw_root   = aww->get_root();
    const char *awar_name = (const char *)cl_awar_name;
    aw_root->awar("tmp/viewkeys/key_text_select")->map(awar_name);

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aw_root, "SELECT_SPECIES_FIELD", "Select species field");
        aws->load_xfig("awt/nds_sel.fig");
        aws->button_length(13);

        aws->callback(AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        awt_create_selection_list_on_itemfields((GBDATA *)cl_gb_main,
                                                aws,
                                                "tmp/viewkeys/key_text_select",
                                                AWT_NDS_FILTER,
                                                "scandb", "rescandb", &AWT_species_selector, 20, 10);
        aws->recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    }
    aws->activate();
}


