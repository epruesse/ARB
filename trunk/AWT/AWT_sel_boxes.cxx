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

#include <item_sel_list.h>

#include <aw_awars.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_edit.hxx>

#include <ad_config.h>
#include <ad_cb.h>

#include <arbdbt.h>
#include <arb_strbuf.h>
#include <arb_strarray.h>
#include <arb_file.h>
#include <arb_global_defs.h>

#include <list>
#include "awt_modules.hxx"
#include <FileBuffer.h>

using namespace std;



// --------------------------------------
//      selection boxes on alignments

class AWT_alignment_selection : public AW_DB_selection { // derived from a Noncopyable
    char *ali_type_match;                           // filter for wanted alignments (GBS_string_eval command)
public:
    AWT_alignment_selection(AW_selection_list *sellist_, GBDATA *gb_presets, const char *ali_type_match_)
        : AW_DB_selection(sellist_, gb_presets)
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

            if (!*str) insert(alignment_name, alignment_name);
            free(str);
            free(alignment_type);
            free(alignment_name);
        }
        insert_default(DISPLAY_NONE, NO_ALI_SELECTED);
    }

    void reconfigure(const char *new_ali_type_match) {
        freedup(ali_type_match, new_ali_type_match);
        refresh();
    }
};

AW_DB_selection *awt_create_selection_list_on_alignments(GBDATA *gb_main, AW_window *aws, const char *varname, const char *ali_type_match) {
    // Create selection lists on alignments
    //
    // if 'ali_type_match' is set, then only insert alignments,
    // where 'ali_type_match' GBS_string_eval's the alignment type

    GBDATA *gb_presets;
    {
        GB_transaction ta(gb_main);
        gb_presets = GBT_get_presets(gb_main);
    }
    AW_selection_list       *sellist = aws->create_selection_list(varname, 20, 3);
    AWT_alignment_selection *alisel  = new AWT_alignment_selection(sellist, gb_presets, ali_type_match);
    alisel->refresh(); // belongs to window now
    return alisel;
}

void awt_reconfigure_selection_list_on_alignments(AW_DB_selection *dbsel, const char *ali_type_match) {
    AWT_alignment_selection *alisel = dynamic_cast<AWT_alignment_selection*>(dbsel);
    alisel->reconfigure(ali_type_match);
}

// ---------------------------------
//      selection boxes on trees

struct AWT_tree_selection: public AW_DB_selection {
    AWT_tree_selection(AW_selection_list *sellist_, GBDATA *gb_tree_data)
        : AW_DB_selection(sellist_, gb_tree_data)
    {}

    void fill() {
        GBDATA         *gb_main = get_gb_main();
        GB_transaction  ta(gb_main);

        ConstStrArray tree_names;
        GBT_get_tree_names(tree_names, gb_main, true);

        if (!tree_names.empty()) {
            int maxTreeNameLen = 0;
            for (int i = 0; tree_names[i]; ++i) {
                const char *tree = tree_names[i];
                int         len  = strlen(tree);
                if (len>maxTreeNameLen) maxTreeNameLen = len;
            }
            for (int i = 0; tree_names[i]; ++i) {
                const char *tree = tree_names[i];
                const char *info = GBT_tree_info_string(gb_main, tree, maxTreeNameLen);
                if (info) {
                    insert(info, tree);
                }
                else {
                    aw_message(GB_await_error());
                    insert(tree, tree);
                }
            }
        }
        insert_default(DISPLAY_NONE, NO_TREE_SELECTED); 
    }
};

AW_DB_selection *awt_create_selection_list_on_trees(GBDATA *gb_main, AW_window *aws, const char *varname) {
    GBDATA *gb_tree_data;
    {
        GB_transaction ta(gb_main);
        gb_tree_data = GBT_get_tree_data(gb_main);
    }
    AW_selection_list  *sellist = aws->create_selection_list(varname, 40, 4);
    AWT_tree_selection *treesel = new AWT_tree_selection(sellist, gb_tree_data); // owned by nobody
    treesel->refresh();
    return treesel;
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
    AWT_ptserver_selection(AW_selection_list *sellist_);

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
        insert(choice, (long)i);
        free(choice);
    }

    insert_default("-undefined-", (long)-1);
}

void AWT_ptserver_selection::refresh_all() {
    PTserverSelections::iterator end = ptserver_selections.end();
    for (PTserverSelections::iterator pts_sel = ptserver_selections.begin(); pts_sel != end; ++pts_sel) {
        (*pts_sel)->refresh();
    }
}
static void awt_refresh_all_pt_server_selection_lists() {
    AWT_ptserver_selection::refresh_all();
}
static unsigned track_log_cb(AW_root *) {
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

    return PT_SERVER_TRACKLOG_TIMER;
}

AWT_ptserver_selection::AWT_ptserver_selection(AW_selection_list *sellist_)
    : AW_selection(sellist_)
{
    if (ptserver_selections.empty()) {
        // first pt server selection list -> install log tracker
        AW_root::SINGLETON->add_timed_callback(PT_SERVER_TRACKLOG_TIMER, makeTimedCallback(track_log_cb));
    }
    ptserver_selections.push_back(this);
}


static void arb_tcp_dat_changed_cb(const char * /* path */, bool fileChanged, bool /* editorTerminated */) {
    if (fileChanged) {
        awt_refresh_all_pt_server_selection_lists();
    }
}

void awt_edit_arbtcpdat_cb(AW_window *aww, GBDATA *gb_main) {
    char *filename = GB_arbtcpdat_path();
    AW_edit(filename, arb_tcp_dat_changed_cb, aww, gb_main);
    free(filename);
}

static char *readable_pt_servername(int index, int maxlength) {
    char *fullname = GBS_ptserver_id_to_choice(index, 0);
    if (!fullname) {
#ifdef DEBUG
        printf("awar given to awt_create_selection_list_on_pt_servers() does not contain a valid index\n");
#endif
        GB_clear_error();
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
    AW_selection_list *sellist = aw_popup->create_selection_list(varname, PT_SERVERNAME_SELLIST_WIDTH, 20);

    aw_popup->at_newline();
    aw_popup->callback((AW_CB0)AW_POPDOWN);
    aw_popup->create_button("CLOSE", "CLOSE", "C");

    aw_popup->window_fit();
    aw_popup->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    (new AWT_ptserver_selection(sellist))->refresh();

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

        awt_assert(!GB_have_error());

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
        (new AWT_ptserver_selection(aws->create_selection_list(varname)))->refresh();
    }
}

// ----------------------------------
//      selection boxes on tables


#if defined(WARN_TODO)
#warning derive awt_sel_list_for_tables from AW_DB_selection
#endif

struct awt_sel_list_for_tables {
    AW_window         *aws;
    GBDATA            *gb_main;
    AW_selection_list *id;
    const char        *table_name;
};

static void awt_create_selection_list_on_tables_cb(GBDATA *, struct awt_sel_list_for_tables *cbs) {
    cbs->id->clear();
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
        cbs->id->insert(info_text, table_name);
        free(table_name);
        free(description);
    }
    cbs->id->insert_default("", "");
    cbs->id->update();
}

void awt_create_selection_list_on_tables(GBDATA *gb_main, AW_window *aws, const char *varname)
{
    AW_selection_list *id;
    GBDATA  *gb_table_data;
    struct awt_sel_list_for_tables *cbs;
    GB_push_transaction(gb_main);

    id = aws->create_selection_list(varname, 40, 8);
    cbs = new awt_sel_list_for_tables;
    cbs->aws = aws;
    cbs->gb_main = gb_main;
    cbs->id = id;

    awt_create_selection_list_on_tables_cb(0, cbs);

    gb_table_data = GB_search(gb_main, "table_data", GB_CREATE_CONTAINER);
    GB_add_callback(gb_table_data, GB_CB_CHANGED, makeDatabaseCallback(awt_create_selection_list_on_tables_cb, cbs));

    GB_pop_transaction(gb_main);
}

static void awt_create_selection_list_on_table_fields_cb(GBDATA *, struct awt_sel_list_for_tables *cbs) {
    cbs->id->clear();
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
        cbs->id->insert(info_text, table_name);
        free(table_name);
        free(description);
    }
    cbs->id->insert_default("", "");
    cbs->id->update();
}

void awt_create_selection_list_on_table_fields(GBDATA *gb_main, AW_window *aws, const char *table_name, const char *varname) {
    // if tablename == 0 -> take fields from species table

    AW_selection_list              *id;
    struct awt_sel_list_for_tables *cbs;
    GB_push_transaction(gb_main);

    id = aws->create_selection_list(varname, 40, 8);
    cbs = new awt_sel_list_for_tables;
    cbs->aws = aws;
    cbs->gb_main = gb_main;
    cbs->id = id;
    cbs->table_name = strdup(table_name);

    awt_create_selection_list_on_table_fields_cb(0, cbs);

    GBDATA  *gb_table = GBT_open_table(gb_main, table_name, true); // read only
    if (gb_table) {
        GB_add_callback(gb_table, GB_CB_CHANGED, makeDatabaseCallback(awt_create_selection_list_on_table_fields_cb, cbs));
    }
    GB_pop_transaction(gb_main);
}

// -------------------------------------------------
//      selection boxes on editor configurations

struct AWT_configuration_selection : public AW_DB_selection {
    AWT_configuration_selection(AW_selection_list *sellist_, GBDATA *gb_configuration_data)
        : AW_DB_selection(sellist_, gb_configuration_data)
    {}

    void fill() {
        ConstStrArray config;
        GBT_get_configuration_names(config, get_gb_main());

        if (!config.empty()) {
            for (int c = 0; config[c]; c++) insert(config[c], config[c]);
        }
        insert_default(DISPLAY_NONE, "????");
    }
};

void awt_create_selection_list_on_configurations(GBDATA *gb_main, AW_window *aws, const char *varname) {
    GBDATA *gb_configuration_data;
    {
        GB_transaction ta(gb_main);
        gb_configuration_data = GB_search(gb_main, CONFIG_DATA_PATH, GB_CREATE_CONTAINER);
    }
    AW_selection_list *sellist = aws->create_selection_list(varname, 40, 15);
    (new AWT_configuration_selection(sellist, gb_configuration_data))->refresh();
}

char *awt_create_string_on_configurations(GBDATA *gb_main) {
    // returns semicolon-separated string containing configuration names
    // (or NULL if no configs exist)

    GB_push_transaction(gb_main);

    ConstStrArray config;
    GBT_get_configuration_names(config, gb_main);

    char *result = 0;

    if (!config.empty()) {
        GBS_strstruct *out = GBS_stropen(1000);
        for (int c = 0; config[c]; c++) {
            if (c>0) GBS_chrcat(out, ';');
            GBS_strcat(out, config[c]);
        }
        result = GBS_strclose(out);
    }

    GB_pop_transaction(gb_main);
    return result;
}

// ----------------------
//      SAI selection


class AWT_sai_selection : public AW_DB_selection { // derived from a Noncopyable
    awt_sai_sellist_filter filter_poc;
    AW_CL                  filter_cd;

public:

    AWT_sai_selection(AW_selection_list *sellist_, GBDATA *gb_sai_data, awt_sai_sellist_filter filter_poc_, AW_CL filter_cd_)
        : AW_DB_selection(sellist_, gb_sai_data),
          filter_poc(filter_poc_),
          filter_cd(filter_cd_)
    {}

    void fill();
};

void AWT_sai_selection::fill() {
    AW_selection_list *sel = get_sellist();
    sel->clear();

    GBDATA         *gb_main = get_gb_main();
    GB_transaction  ta(gb_main);

    for (GBDATA *gb_extended = GBT_first_SAI(gb_main);
         gb_extended;
         gb_extended = GBT_next_SAI(gb_extended))
    {
        if (filter_poc) {
            char *res = filter_poc(gb_extended, filter_cd);
            if (res) {
                sel->insert(res, GBT_read_name(gb_extended));
                free(res);
            }
        }
        else {
            const char *name     = GBT_read_name(gb_extended);
            GBDATA     *gb_group = GB_entry(gb_extended, "sai_group");

            if (gb_group) {
                const char *group          = GB_read_char_pntr(gb_group);
                char       *group_and_name = GBS_global_string_copy("[%s] %s", group, name);

                sel->insert(group_and_name, name);
                free(group_and_name);
            }
            else {
                sel->insert(name, name);
            }
        }
    }
    sel->sort(false, false);

    sel->insert_default(DISPLAY_NONE, "none");
    sel->update();
}

void awt_selection_list_on_sai_update_cb(UNFIXED, AWT_sai_selection *saisel) {
    /* update the selection box defined by awt_create_selection_list_on_sai
     *
     * useful only when filterproc is defined
     * (changes to SAIs will automatically callback this function)
     */

    saisel->refresh();
}

AWT_sai_selection *SAI_selection_list_spec::create_list(AW_window *aws) const {
    GB_transaction ta(gb_main);

    AW_selection_list *sellist     = aws->create_selection_list(awar_name, 40, 4);
    GBDATA            *gb_sai_data = GBT_get_SAI_data(gb_main);
    AWT_sai_selection *saisel      = new AWT_sai_selection(sellist, gb_sai_data, filter_poc, filter_cd);

    awt_selection_list_on_sai_update_cb(0, saisel);
    GB_add_callback(gb_sai_data, GB_CB_CHANGED, makeDatabaseCallback(awt_selection_list_on_sai_update_cb, saisel));

    return saisel;
}

void awt_popup_filtered_sai_selection_list(AW_root *aw_root, AW_CL cl_sellist_spec) {
    const SAI_selection_list_spec *spec      = (const SAI_selection_list_spec*)cl_sellist_spec;
    const char                    *awar_name = spec->get_awar_name();
    
    static GB_HASH *SAI_window_hash       = 0;
    if (!SAI_window_hash) SAI_window_hash = GBS_create_hash(10, GB_MIND_CASE);

    AW_window_simple *aws = reinterpret_cast<AW_window_simple *>(GBS_read_hash(SAI_window_hash, awar_name));

    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "SELECT_SAI", "SELECT SAI");
        aws->load_xfig("select_simple.fig");

        aws->at("selection");
        aws->callback((AW_CB0)AW_POPDOWN);
        spec->create_list(aws);

        aws->at("button");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->window_fit();

        GBS_write_hash(SAI_window_hash, awar_name, reinterpret_cast<long>(aws));
    }

    aws->activate();
}
void awt_popup_filtered_sai_selection_list(AW_window *aww, AW_CL cl_sellist_spec) {
    awt_popup_filtered_sai_selection_list(aww->get_root(), cl_sellist_spec);
}

void awt_popup_sai_selection_list(AW_root *aw_root, AW_CL cl_awar_name, AW_CL cl_gb_main) {
    const char *awar_name = reinterpret_cast<const char *>(cl_awar_name);
    GBDATA *gb_main = reinterpret_cast<GBDATA *>(cl_gb_main);

    SAI_selection_list_spec spec(awar_name, gb_main);
    awt_popup_filtered_sai_selection_list(aw_root, AW_CL(&spec));
}

void awt_popup_sai_selection_list(AW_window *aww, AW_CL cl_awar_name, AW_CL cl_gb_main) {
    awt_popup_sai_selection_list(aww->get_root(), cl_awar_name, cl_gb_main);
}

AWT_sai_selection *awt_create_selection_list_on_sai(GBDATA *gb_main, AW_window *aws, const char *varname, awt_sai_sellist_filter filter_poc, AW_CL filter_cd) {
    /* Selection list for SAIs
     *
     * if filter_proc is set then show only those items on which
     * filter_proc returns a string (string must be a heap copy)
     */
    SAI_selection_list_spec spec(varname, gb_main);
    spec.define_filter(filter_poc, filter_cd);
    return spec.create_list(aws);
}

void awt_create_SAI_selection_button(GBDATA *gb_main, AW_window *aws, const char *varname, awt_sai_sellist_filter filter_poc, AW_CL filter_cd) {
    SAI_selection_list_spec *spec = new SAI_selection_list_spec(varname, gb_main);
    spec->define_filter(filter_poc, filter_cd);
    aws->callback(awt_popup_filtered_sai_selection_list, AW_CL(spec));
    aws->create_button("SELECT_SAI", varname);
}

// --------------------------------------------------
//      save/load selection content to/from file

static GB_ERROR standard_list2file(const CharPtrArray& display, const CharPtrArray& value, StrArray& line) {
    GB_ERROR error = NULL;
    for (size_t i = 0; i<display.size() && !error; ++i) {
        const char *disp = display[i];

        if (disp[0] == '#') { // would interpret as comment when loaded
            error = "Invalid character '#' at start of displayed text (won't load)";
        }
        else {
            if (strchr(disp, ',')) {
                // would be interpreted as separator between display and value on load
                error = "Invalid character ',' in displayed text (won't load correctly)";
            }
            else {
                awt_assert(strchr(disp, '\n') == 0);

                const char *val = value[i];
                if (strcmp(disp, val) == 0) {
                    line.put(strdup(disp));
                }
                else {
                    char *escaped = GBS_escape_string(val, "\n", '\\');
                    line.put(GBS_global_string_copy("%s,%s", disp, escaped));
                    free(escaped);
                }
            }
        }
    }
    return NULL;
}



static GB_ERROR standard_file2list(const CharPtrArray& line, StrArray& display, StrArray& value) {
    for (size_t i = 0; i<line.size(); ++i) {
        if (line[i][0] == '#') continue; // ignore comments

        const char *comma = strchr(line[i], ',');
        if (comma) {
            display.put(GB_strpartdup(line[i], comma-1));

            comma++;
            const char *rest      = comma+strspn(comma, " \t");
            char       *unescaped = GBS_unescape_string(rest, "\n", '\\');
            value.put(unescaped);
        }
        else {
            display.put(strdup(line[i]));
            value.put(strdup(line[i]));
        }
    }

    return NULL;
}

StorableSelectionList::StorableSelectionList(const TypedSelectionList& tsl_)
    : tsl(tsl_),
      list2file(standard_list2file), 
      file2list(standard_file2list)
{}

inline char *get_shared_sellist_awar_base(const TypedSelectionList& typedsellst) {
    return GBS_global_string_copy("tmp/sellist/%s", typedsellst.get_shared_id());
}
inline char *get_shared_sellist_awar_name(const TypedSelectionList& typedsellst, const char *name) {
    char *base      = get_shared_sellist_awar_base(typedsellst);
    char *awar_name = GBS_global_string_copy("%s/%s", base, name);
    free(base);
    return awar_name;
}

GB_ERROR StorableSelectionList::save(const char *filename, long number_of_lines) const {
    // number_of_lines == 0 -> save all lines (otherwise truncate after 'number_of_lines')

    StrArray           display, values;
    AW_selection_list *sellist = tsl.get_sellist();

    sellist->to_array(display, false);
    sellist->to_array(values, true);

    awt_assert(display.size() == values.size());

    if (number_of_lines>0) { // limit number of lines?
        display.resize(number_of_lines);
        values.resize(number_of_lines);
    }

    GB_ERROR error = NULL;
    if (display.size()<1) {
        error = "List is empty (did not save)";
    }
    else {
        StrArray line;
        error = list2file(display, values, line);
        if (!error) {
            if (line.size()<1) {
                error = "list>file conversion produced nothing (internal error)";
            }
            else {
                FILE *out = fopen(filename, "wt");
                if (!out) {
                    error = GB_IO_error("writing", filename);
                }
                else {
                    const char *warning = NULL;
                    for (size_t i = 0; i<line.size(); ++i) {
                        if (!warning && strchr(line[i], '\n')) {
                            warning = "Warning: Saved content contains LFs (loading will be impossible)";
                        }
                        fputs(line[i], out);
                        fputc('\n', out);
                    }
                    fclose(out);
                    error = warning;
                }
            }
        }
    }

    return error;
}



inline char *string2heapcopy(const string& s) {
    char *copy = (char*)malloc(s.length()+1);
    memcpy(copy, s.c_str(), s.length()+1);
    return copy;
}

GB_ERROR StorableSelectionList::load(const char *filemask, bool append) const {
    GB_ERROR error = NULL;
    StrArray fnames;

    if (GB_is_directory(filemask)) {
        error = GBS_global_string("refusing to load all files from directory\n"
                                  "'%s'\n"
                                  "(one possibility is to enter '*.%s')'",
                                  filemask, get_filter());
    }
    else {
        GBS_read_dir(fnames, filemask, NULL);
    }

    StrArray lines;
    for (int f = 0; fnames[f] && !error; ++f) {
        FILE *in = fopen(fnames[f], "rb");
        if (!in) {
            error = GB_IO_error("reading", fnames[f]);
        }
        else {
            FileBuffer file(fnames[f], in);
            string line;
            while (file.getLine(line)) {
                if (!line.empty()) lines.put(string2heapcopy(line));
            }
        }
    }

    AW_selection_list *sellist = tsl.get_sellist();
    if (!append) sellist->clear();

    if (!error) {
        StrArray displayed, values;
        error = file2list(lines, displayed, values);
        if (!error) {
            int dsize = displayed.size();
            int vsize = values.size();

            if (dsize != vsize) {
                error = GBS_global_string("Error in translation (value/display mismatch: %i!=%i)", vsize, dsize);
            }
            else {
                for (int i = 0; i<dsize; ++i) {
                    sellist->insert(displayed[i], values[i]);
                }
                sellist->insert_default("", "");
            }
        }
    }

    if (error) {
        sellist->insert_default(GBS_global_string("Error: %s", error), "");
    }
    sellist->update();

    return error;
}

static void save_list_cb(AW_window *aww, AW_CL cl_storabsellist) {
    const StorableSelectionList *storabsellist = (const StorableSelectionList*)cl_storabsellist;
    const TypedSelectionList&    typedsellist  = storabsellist->get_typedsellist();

    char    *awar_prefix = get_shared_sellist_awar_base(typedsellist);
    char    *bline_anz   = get_shared_sellist_awar_name(typedsellist, "line_anz");
    AW_root *aw_root     = aww->get_root();
    char    *filename    = AW_get_selected_fullname(aw_root, awar_prefix);

    long     lineLimit = aw_root->awar(bline_anz)->read_int();
    GB_ERROR error     = storabsellist->save(filename, lineLimit);

    if (!error) AW_refresh_fileselection(aw_root, awar_prefix);
    aww->hide_or_notify(error);

    free(filename);
    free(bline_anz);
    free(awar_prefix);
}

static void load_list_cb(AW_window *aww, AW_CL cl_storabsellist) {
    const StorableSelectionList *storabsellist = (const StorableSelectionList*)cl_storabsellist;
    const TypedSelectionList&    typedsellist  = storabsellist->get_typedsellist();

    AW_root  *aw_root     = aww->get_root();
    char     *awar_prefix = get_shared_sellist_awar_base(typedsellist);
    char     *awar_append = get_shared_sellist_awar_name(typedsellist, "append");
    bool      append      = aw_root->awar(awar_append)->read_int();
    char     *filename    = AW_get_selected_fullname(aw_root, awar_prefix);
    GB_ERROR  error       = storabsellist->load(filename, append);

    aww->hide_or_notify(error);

    free(filename);
    free(awar_append);
    free(awar_prefix);
}

AW_window *create_save_box_for_selection_lists(AW_root *aw_root, AW_CL cl_storabsellist) {
    const StorableSelectionList *storabsellist = (const StorableSelectionList*)cl_storabsellist;
    const TypedSelectionList&    typedsellist  = storabsellist->get_typedsellist();

    char *awar_base     = get_shared_sellist_awar_base(typedsellist);
    char *awar_line_anz = get_shared_sellist_awar_name(typedsellist, "line_anz");
    {
        char *def_name = GBS_string_2_key(typedsellist.whats_contained());
        AW_create_fileselection_awars(aw_root, awar_base, ".", storabsellist->get_filter(), def_name);
        free(def_name);
        aw_root->awar_int(awar_line_anz, 0, AW_ROOT_DEFAULT);
    }

    AW_window_simple *aws = new AW_window_simple;

    char *window_id    = GBS_global_string_copy("SAVE_SELECTION_BOX_%s", typedsellist.get_unique_id());
    char *window_title = GBS_global_string_copy("Save %s", typedsellist.whats_contained());

    aws->init(aw_root, window_id, window_title);
    aws->load_xfig("sl_s_box.fig");

    aws->button_length(10);

    aws->at("cancel");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CANCEL", "CANCEL", "C");

    aws->at("save");
    aws->highlight();
    aws->callback(save_list_cb, cl_storabsellist); 
    aws->create_button("SAVE", "SAVE", "S");

    aws->at("nlines");
    aws->create_option_menu(awar_line_anz);
    aws->insert_default_option("all",   "a", 0);
    aws->insert_option        ("10",    "",  10);
    aws->insert_option        ("50",    "",  50);
    aws->insert_option        ("100",   "",  100);
    aws->insert_option        ("500",   "",  500);
    aws->insert_option        ("1000",  "",  1000);
    aws->insert_option        ("5000",  "",  5000);
    aws->insert_option        ("10000", "",  10000);
    aws->update_option_menu();

    AW_create_fileselection(aws, awar_base);

    free(window_title);
    free(window_id);
    free(awar_line_anz);
    free(awar_base);

    aws->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    return aws;
}

AW_window *create_load_box_for_selection_lists(AW_root *aw_root, AW_CL cl_storabsellist) {
    const StorableSelectionList *storabsellist = (const StorableSelectionList*)cl_storabsellist;
    const TypedSelectionList&    typedsellist  = storabsellist->get_typedsellist();

    char *awar_base_name = get_shared_sellist_awar_base(typedsellist);
    char *awar_append    = get_shared_sellist_awar_name(typedsellist, "append");

    AW_create_fileselection_awars(aw_root, awar_base_name, ".", storabsellist->get_filter(), "");
    aw_root->awar_int(awar_append, 1); // append is default ( = old behavior)

    AW_window_simple *aws = new AW_window_simple;

    char *window_id    = GBS_global_string_copy("LOAD_SELECTION_BOX_%s", typedsellist.get_unique_id());
    char *window_title = GBS_global_string_copy("Load %s", typedsellist.whats_contained());

    aws->init(aw_root, window_id, window_title);
    aws->load_xfig("sl_l_box.fig");

    aws->at("cancel");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CANCEL", "CANCEL", "C");

    aws->at("load");
    aws->highlight();
    aws->callback(load_list_cb, cl_storabsellist);
    aws->create_button("LOAD", "LOAD", "L");

    aws->at("append");
    aws->label("Append?");
    aws->create_toggle(awar_append);

    AW_create_fileselection(aws, awar_base_name, "", "PWD", true, true);

    aws->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    free(window_title);
    free(window_id);
    free(awar_append);
    free(awar_base_name);

    return aws;
}

void awt_clear_selection_list_cb(AW_window *, AW_CL cl_sellist) {
    AW_selection_list *sellist = (AW_selection_list*)cl_sellist;
    sellist->clear();
    sellist->insert_default("", "");
    sellist->update();
}

void create_print_box_for_selection_lists(AW_window *aw_window, AW_CL cl_typedsellst) {
    const TypedSelectionList *typedsellist = (const TypedSelectionList*)cl_typedsellst;
    
    char *data = typedsellist->get_sellist()->get_content_as_string(0);
    AWT_create_ascii_print_window(aw_window->get_root(), data, typedsellist->whats_contained());
    free(data);
}

AW_window *awt_create_load_box(AW_root     *aw_root,
                               const char  *action,
                               const char  *what,
                               const char  *default_directory,
                               const char  *file_extension,
                               char       **set_file_name_awar,
                               const WindowCallback& ok_cb,
                               const WindowCallback& close_cb,
                               const char *close_button_text)
{
    /* general purpose file selection box
     *
     * 'action' describes what is intended to be done (e.g. "Load").
     * used for window title and button.
     *
     * 'what' describes what is going to be loaded (e.g. "destination database")
     * It is also used to create the awars for the filebox, i.e. same description for multiple
     * fileboxes makes them share the awars.
     *
     * if 'set_file_name_awar' is non-NULL, it'll be set to a heap-copy of the awar-name
     * containing the full selected filename.
     *
     * 'default_directory' specifies the directory opened in the filebox
     *
     * 'file_extension' specifies the filter to be used (which files are shown)
     *
     * You have to provide an 'ok_cb', which will be called when 'OK' is pressed.
     * Optionally you may pass a 'close_cb' which will be called when 'CLOSE' is pressed.
     * If not given, AW_POPDOWN will be called.
     *
     * Both callbacks will be called as callbacks of the load-box-window.
     * The load-box does not popdown, the callback has to do that.
     *
     * Optionally you may also pass the button text for the 'CLOSE'-button (e.g. 'EXIT' or 'Abort')
     */


    char *what_key  = GBS_string_2_key(what);
    char *base_name = GBS_global_string_copy("tmp/load_box_%s", what_key);

    AW_create_fileselection_awars(aw_root, base_name, default_directory, file_extension, "");

    if (set_file_name_awar) {
        *set_file_name_awar = GBS_global_string_copy("%s/file_name", base_name);
    }

    AW_window_simple *aws = new AW_window_simple;
    {
        char title[100];
        sprintf(title, "%s %s", action, what);
        aws->init(aw_root, title, title);
        aws->load_xfig("load_box.fig");
    }

    aws->at("close");
    aws->callback(close_cb);
    if (close_button_text) {
        aws->create_button("CLOSE", close_button_text, "");
    }
    else {
        aws->create_button("CLOSE", "CLOSE", "C");
    }
    // @@@ gtk: set_close_action

#if 0
    // @@@ allow to pass helpfile
    aws->at("help");
    aws->callback(makeHelpCallback(""));
    aws->create_button("HELP", "HELP");
#endif

    aws->at("go");
    aws->callback(ok_cb);
    aws->create_button("GO", action);

    AW_create_fileselection(aws, base_name);
    free(base_name);
    free(what_key);
    aws->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    return aws;
}

// --------------------------------------------------------------------------------

#define SUBSET_NOELEM_DISPLAY "<none>"

class AW_subset_selection : public AW_selection {
    AW_selection_list& parent_sellist;

    static void finish_fill_box(AW_selection_list *parent_sellist, AW_selection_list *sub_sellist) {
        sub_sellist->insert_default(parent_sellist->get_default_display(), parent_sellist->get_default_value());
        sub_sellist->update();
    }

    static AW_selection_list *create_box(AW_window *aww, AW_selection_list& parent_sellist) {
        const char *parent_awar_name = parent_sellist.variable_name;
        awt_assert(parent_awar_name[0] != '/');
        awt_assert(parent_sellist.variable_type == AW_STRING); // only impl for strings

        AW_root *aw_root   = aww->get_root();
        char    *awar_name = GBS_global_string_copy("tmp/subsel/%s", parent_awar_name);

        aw_root->awar_string(awar_name);

        AW_selection_list *sub_sellist = aww->create_selection_list(awar_name);
        finish_fill_box(&parent_sellist, sub_sellist);

        free(awar_name);

        return sub_sellist;
    }

public:
    AW_subset_selection(AW_window *aww, AW_selection_list& parent_sellist_)
        : AW_selection(create_box(aww, parent_sellist_)),
          parent_sellist(parent_sellist_)
    {}

    AW_selection_list *get_parent_sellist() const { return &parent_sellist; }

    const char *default_select_value() const { return parent_sellist.get_default_value(); }
    const char *default_select_display() const { return parent_sellist.get_default_display(); }

    void get_subset(StrArray& subset) {
        get_sellist()->to_array(subset, true);
    }

    void fill() { awt_assert(0); } // unused

    void collect_subset_cb(awt_collect_mode what) {
        AW_selection_list *subset_list = get_sellist();
        AW_selection_list *whole_list  = get_parent_sellist();

        switch(what) {
            case ACM_FILL:
                for (AW_selection_list_iterator listEntry(whole_list); listEntry; ++listEntry) {
                    if (subset_list->get_index_of(listEntry.get_value()) == -1) { // only add not already existing elements
                        subset_list->insert(listEntry.get_displayed(), listEntry.get_value());
                    }
                }
                finish_fill_box(whole_list, subset_list);
                break;

            case ACM_ADD: {
                if (!whole_list->default_is_selected()) {
                    const char *selected   = whole_list->get_awar_value();
                    int         src_index = whole_list->get_index_of(selected);
                    
                    if (subset_list->get_index_of(selected) == -1) { // not yet in subset_list
                        AW_selection_list_iterator entry(whole_list, src_index);
                        subset_list->insert(entry.get_displayed(), entry.get_value());
                        subset_list->update();
                    }

                    subset_list->set_awar_value(selected); // position right side to newly added or already existing alignment
                    whole_list->select_element_at(src_index+1);    // go down 1 position on left side
                }

                break;
            }
            case ACM_REMOVE: {
                if (!subset_list->default_is_selected()) {
                    char *selected     = strdup(subset_list->get_awar_value());
                    int   old_position = subset_list->get_index_of(selected);

                    subset_list->delete_element_at(old_position);
                    finish_fill_box(whole_list, subset_list);

                    subset_list->select_element_at(old_position);
                    whole_list->set_awar_value(selected); // set left selection to deleted alignment
                    free(selected);
                }
                break;
            }
            case ACM_EMPTY:
                subset_list->clear();
                finish_fill_box(whole_list, subset_list);
                break;
        }
    }
    void reorder_subset_cb(awt_reorder_mode dest) {
        AW_selection_list *subset_list = get_sellist();

        if (!subset_list->default_is_selected()) {
            const char *selected = subset_list->get_awar_value();

            StrArray listContent;
            subset_list->to_array(listContent, true);

            int old_pos = GBT_names_index_of(listContent, selected);
            if (old_pos >= 0) {
                int new_pos = 0;
                switch (dest) {
                    case ARM_TOP:    new_pos= 0;         break;
                    case ARM_UP:     new_pos= old_pos-1; break;
                    case ARM_DOWN:   new_pos= old_pos+1; break;
                    case ARM_BOTTOM: new_pos= -1;        break;
                }
                if (old_pos != new_pos) {
                    GBT_names_move(listContent, old_pos, new_pos);
                    subset_list->init_from_array(listContent, subset_list->get_default_value());
                }
            }
        }
    }
};

static void collect_subset_cb(AW_window *, awt_collect_mode what, AW_CL cl_subsel) { ((AW_subset_selection*)cl_subsel)->collect_subset_cb(what); }
static void reorder_subset_cb(AW_window *, awt_reorder_mode dest, AW_CL cl_subsel) { ((AW_subset_selection*)cl_subsel)->reorder_subset_cb(dest); }

AW_selection *awt_create_subset_selection_list(AW_window *aww, AW_selection_list *parent_selection, const char *at_box, const char *at_add, const char *at_sort) {
    awt_assert(parent_selection);

    aww->at(at_box);
    int x_list = aww->get_at_xposition();

    AW_subset_selection *subsel = new AW_subset_selection(aww, *parent_selection);

    aww->button_length(0);

    aww->at(at_add);
    int x_buttons = aww->get_at_xposition();

    bool move_rightwards = x_list>x_buttons;
    awt_create_collect_buttons(aww, move_rightwards, collect_subset_cb, (AW_CL)subsel);

    aww->at(at_sort);
    awt_create_order_buttons(aww, reorder_subset_cb, (AW_CL)subsel);

    return subsel;
}


