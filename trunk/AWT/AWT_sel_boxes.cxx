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

#include "awt_modules.hxx"
#include <BufferedFileReader.h>

#include <list>
#include <map>
#include <set>

using namespace std;

typedef map<string, AW_window*> WinMap;

class SelectionListSpec {
    string        awar_name;
    static WinMap window_map; // contains popup windows of all selection list popups

    virtual AW_DB_selection *create(AW_selection_list *sellist) const = 0;
    AW_DB_selection *init(AW_selection_list *sellist) const {
        AW_DB_selection *sel = create(sellist);
        sel->refresh();
        return sel;
    }

public:
    SelectionListSpec(const char *awar_name_)
        : awar_name(awar_name_)
    {}
    virtual ~SelectionListSpec() {}

    virtual const char *get_macro_id() const = 0;
    virtual const char *get_title() const    = 0;

    const char *get_awar_name() const { return awar_name.c_str(); }

    AW_DB_selection *create_list(AW_window *aws, bool fallback2default) const {
        return init(aws->create_selection_list(get_awar_name(), 40, 4, fallback2default));
    }

#if defined(ARB_GTK)
    AW_DB_selection *create_optionMenu(AW_window *aws, bool fallback2default) const {
        return init(aws->create_option_menu(get_awar_name(), fallback2default));
    }
#endif

    void popup() const {
        WinMap::iterator  found  = window_map.find(awar_name);
        if (found == window_map.end()) {
            AW_window_simple *aws = new AW_window_simple;
            aws->init(AW_root::SINGLETON, get_macro_id(), get_title());
            aws->load_xfig("select_simple.fig");

            aws->at("selection");
            aws->callback((AW_CB0)AW_POPDOWN);
            create_list(aws, true);

            aws->at("button");
            aws->callback(AW_POPDOWN);
            aws->create_button("CLOSE", "CLOSE", "C");

            aws->window_fit();

            window_map[awar_name] = aws;
            aws->activate();
        }
        else {
            found->second->activate();
        }
    }

    void createButton(AW_window *aws) const;
};
WinMap SelectionListSpec::window_map; 

#if defined(ARB_MOTIF)
static void popup_SelectionListSpec_cb(UNFIXED, const SelectionListSpec *spec) {
    spec->popup();
}
#endif
void SelectionListSpec::createButton(AW_window *aws) const {
    // WARNING: this is bound to callback (do not free)
#if defined(ARB_GTK) // use option menu in gtk
    create_optionMenu(aws, true);
    aws->update_option_menu();
#else // !defined(ARB_GTK)
    aws->callback(makeWindowCallback(popup_SelectionListSpec_cb, this));
    aws->create_button(get_macro_id(), get_awar_name());
#endif
}

// --------------------------------------
//      selection boxes on alignments

class ALI_selection : public AW_DB_selection { // derived from a Noncopyable
    char *ali_type_match;                           // filter for wanted alignments (GBS_string_eval command)
public:
    ALI_selection(AW_selection_list *sellist_, GBDATA *gb_presets, const char *ali_type_match_)
        : AW_DB_selection(sellist_, gb_presets),
          ali_type_match(nulldup(ali_type_match_))
    {}
    
    void fill() OVERRIDE {
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

class ALI_sellst_spec : public SelectionListSpec, virtual Noncopyable {
    GBDATA *gb_main;
    string  ali_type_match;

    AW_DB_selection *create(AW_selection_list *sellist) const {
        GB_transaction ta(gb_main);
        return new ALI_selection(sellist, GBT_get_presets(gb_main), ali_type_match.c_str());
    }

public:
    ALI_sellst_spec(const char *awar_name_, GBDATA *gb_main_, const char *ali_type_match_)
        : SelectionListSpec(awar_name_),
          gb_main(gb_main_),
          ali_type_match(ali_type_match_)
    {}

    const char *get_macro_id() const { return "SELECT_ALI"; }
    const char *get_title() const { return "Select alignment"; }
};

AW_DB_selection *awt_create_ALI_selection_list(GBDATA *gb_main, AW_window *aws, const char *varname, const char *ali_type_match) {
    // Create selection lists on alignments
    //
    // if 'ali_type_match' is set, then only insert alignments,
    // where 'ali_type_match' GBS_string_eval's the alignment type

    ALI_sellst_spec spec(varname, gb_main, ali_type_match);
    return spec.create_list(aws, true);
}

void awt_create_ALI_selection_button(GBDATA *gb_main, AW_window *aws, const char *varname, const char *ali_type_match) {
    (new ALI_sellst_spec(varname, gb_main, ali_type_match))->createButton(aws); // do not free yet (bound to callback in ARB_MOTIF)
}

void awt_reconfigure_ALI_selection_list(AW_DB_selection *dbsel, const char *ali_type_match) {
    ALI_selection *alisel = dynamic_cast<ALI_selection*>(dbsel);
    alisel->reconfigure(ali_type_match);
}

// ---------------------------------
//      selection boxes on trees

struct AWT_tree_selection: public AW_DB_selection {
    AWT_tree_selection(AW_selection_list *sellist_, GBDATA *gb_tree_data)
        : AW_DB_selection(sellist_, gb_tree_data)
    {}

    void fill() OVERRIDE {
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

AW_DB_selection *awt_create_TREE_selection_list(GBDATA *gb_main, AW_window *aws, const char *varname, bool fallback2default) {
    GBDATA *gb_tree_data;
    {
        GB_transaction ta(gb_main);
        gb_tree_data = GBT_get_tree_data(gb_main);
    }
    AW_selection_list  *sellist = aws->create_selection_list(varname, 40, 4, fallback2default);
    AWT_tree_selection *treesel = new AWT_tree_selection(sellist, gb_tree_data); // owned by nobody
    treesel->refresh();
    return treesel;
}


// --------------------------------------
//      selection boxes on pt-servers

#define PT_SERVERNAME_LENGTH        23              // that's for buttons
#define PT_SERVERNAME_SELLIST_WIDTH 30              // this for lists
#define PT_SERVER_TRACKLOG_TIMER    10000           // every 10 seconds

class PT_selection : public AW_selection {
    typedef list<PT_selection*> PT_selections;
    
    static PT_selections ptserver_selections;
public:
    PT_selection(AW_selection_list *sellist_);

    void fill() OVERRIDE;

    static void refresh_all();
};

PT_selection::PT_selections PT_selection::ptserver_selections;

void PT_selection::fill() {
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

void PT_selection::refresh_all() {
    PT_selections::iterator end = ptserver_selections.end();
    for (PT_selections::iterator pts_sel = ptserver_selections.begin(); pts_sel != end; ++pts_sel) {
        (*pts_sel)->refresh();
    }
}
static void refresh_all_PTSERVER_selection_lists() {
    PT_selection::refresh_all();
}
static unsigned track_log_cb(AW_root *) {
    static long  last_ptserverlog_mod = 0;
    const char  *ptserverlog          = GBS_ptserver_logname();
    long         ptserverlog_mod      = GB_time_of_file(ptserverlog);

    if (ptserverlog_mod != last_ptserverlog_mod) {
#if defined(DEBUG)
        fprintf(stderr, "%s modified!\n", ptserverlog);
#endif // DEBUG
        PT_selection::refresh_all();
        last_ptserverlog_mod = ptserverlog_mod;
    }

    return PT_SERVER_TRACKLOG_TIMER;
}

PT_selection::PT_selection(AW_selection_list *sellist_)
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
        refresh_all_PTSERVER_selection_lists();
    }
}

void awt_edit_arbtcpdat_cb(AW_window *aww, GBDATA *gb_main) {
    char *filename = GB_arbtcpdat_path();
    AW_edit(filename, arb_tcp_dat_changed_cb, aww, gb_main);
    free(filename);
}

#if defined(ARB_MOTIF)
static char *readable_pt_servername(int index, int maxlength) {
    char *fullname = GBS_ptserver_id_to_choice(index, 0);
    if (!fullname) {
#ifdef DEBUG
        printf("awar given to ptserver-selection does not contain a valid index\n");
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

static void update_ptserver_button(AW_root *, AW_awar *awar_ptserver, AW_awar *awar_buttontext_name) {
    char *readable_name = readable_pt_servername(awar_ptserver->read_int(), PT_SERVERNAME_LENGTH);
    awar_buttontext_name->write_string(readable_name);
    free(readable_name);
}

static AW_window *create_PTSERVER_selection_window(AW_root *aw_root, const char *varname) {
    AW_window_simple *aw_popup = new AW_window_simple;

    aw_popup->init(aw_root, "SELECT_PT_SERVER", "Select a PT-Server");
    aw_popup->auto_space(10, 10);

    aw_popup->at_newline();
    aw_popup->callback((AW_CB0)AW_POPDOWN); // @@@ used as SELLIST_CLICK_CB (see #559)
    AW_selection_list *sellist = aw_popup->create_selection_list(varname, PT_SERVERNAME_SELLIST_WIDTH, 20, true);

    aw_popup->at_newline();
    aw_popup->callback((AW_CB0)AW_POPDOWN);
    aw_popup->create_button("CLOSE", "CLOSE", "C");

    aw_popup->window_fit();
    aw_popup->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    (new PT_selection(sellist))->refresh();

    return aw_popup;
}
#endif // ARB_MOTIF

void awt_create_PTSERVER_selection_button(AW_window *aws, const char *varname) {
#ifdef ARB_GTK
    (new PT_selection(aws->create_option_menu(varname, true)))->refresh();

    int old_button_length = aws->get_button_length();
    aws->button_length(PT_SERVERNAME_LENGTH+1);
    aws->update_option_menu();
    aws->button_length(old_button_length);
#else // ARB_MOTIF

    AW_root *aw_root              = aws->get_root();
    char    *awar_buttontext_name = GBS_global_string_copy("/tmp/%s_BUTTON", varname);
    AW_awar *awar_ptserver        = aw_root->awar(varname);
    int      ptserver_index       = awar_ptserver->read_int();

    if (ptserver_index<0) { // fix invalid pt_server indices
        ptserver_index = 0;
        awar_ptserver->write_int(ptserver_index);
    }

    char *readable_name = readable_pt_servername(ptserver_index, PT_SERVERNAME_LENGTH);

    awt_assert(!GB_have_error());

    AW_awar *awar_buttontext = aw_root->awar_string(awar_buttontext_name, readable_name, AW_ROOT_DEFAULT);
    awar_ptserver->add_callback(makeRootCallback(update_ptserver_button, awar_ptserver, awar_buttontext));

    int old_button_length = aws->get_button_length();

    aws->button_length(PT_SERVERNAME_LENGTH+1);
    aws->callback(makeCreateWindowCallback(create_PTSERVER_selection_window, awar_ptserver->awar_name));
    aws->create_button("CURR_PT_SERVER", awar_buttontext_name);

    aws->button_length(old_button_length);

    free(readable_name);
    free(awar_buttontext_name);
#endif
}
void awt_create_PTSERVER_selection_list(AW_window *aws, const char *varname) {
    (new PT_selection(aws->create_selection_list(varname, true)))->refresh();
}

// -------------------------------------------------
//      selection boxes on editor configurations

struct AWT_configuration_selection : public AW_DB_selection {
    AWT_configuration_selection(AW_selection_list *sellist_, GBDATA *gb_configuration_data)
        : AW_DB_selection(sellist_, gb_configuration_data)
    {}

    int getConfigInfo(const char *name, string& comment) {
        // returns number of species in config + sets comment
        GB_ERROR   error;
        GBT_config cfg(get_gb_main(), name, error);

        int count;
        if (!error) {
            const char *cmt = cfg.get_comment();
            comment         = cmt ? cmt : "";
            for (int area = 0; area<2; ++area) {
                GBT_config_parser parser(cfg, area);
                while (1) {
                    const GBT_config_item& item = parser.nextItem(error);
                    if (error || item.type == CI_END_OF_CONFIG) break;
                    if (item.type == CI_SPECIES) ++count;
                }
            }
        }
        else {
            comment = "";
        }
        return count;
    }

    void fill() OVERRIDE {
        ConstStrArray config;
        GBT_get_configuration_names(config, get_gb_main());

        if (!config.empty()) {
            int     maxlen   = 0;
            int     maxcount = 0;
            int    *count    = new int[config.size()];
            string *comment  = new string[config.size()];

            for (int c = 0; config[c]; ++c) {
                maxlen   = max(maxlen, int(strlen(config[c])));
                count[c] = getConfigInfo(config[c], comment[c]);
                maxcount = max(maxcount, count[c]);
            }
            int maxdigits = calc_digits(maxcount);
            for (int c = 0; config[c]; ++c) {
                int digits = calc_digits(count[c]);
                insert(GBS_global_string("%-*s %*s(%i) %s", maxlen, config[c], (maxdigits-digits), "", count[c], comment[c].c_str()), config[c]);
            }
            delete [] comment;
            delete [] count;
        }
        insert_default(DISPLAY_NONE, NO_CONFIG_SELECTED);
    }
};

AW_DB_selection *awt_create_CONFIG_selection_list(GBDATA *gb_main, AW_window *aws, const char *varname, bool fallback2default) {
    GBDATA *gb_configuration_data;
    {
        GB_transaction ta(gb_main);
        gb_configuration_data = GB_search(gb_main, CONFIG_DATA_PATH, GB_CREATE_CONTAINER);
    }
    AW_selection_list           *sellist = aws->create_selection_list(varname, 40, 15, fallback2default);
    AWT_configuration_selection *confSel = new AWT_configuration_selection(sellist, gb_configuration_data);
    confSel->refresh();
    return confSel;
}

char *awt_create_CONFIG_string(GBDATA *gb_main) {
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


class SAI_selection : public AW_DB_selection { // derived from a Noncopyable
    awt_sai_sellist_filter filter_poc;
    AW_CL                  filter_cd;

public:

    SAI_selection(AW_selection_list *sellist_, GBDATA *gb_sai_data, awt_sai_sellist_filter filter_poc_, AW_CL filter_cd_)
        : AW_DB_selection(sellist_, gb_sai_data),
          filter_poc(filter_poc_),
          filter_cd(filter_cd_)
    {}

    void fill() OVERRIDE;
};

void SAI_selection::fill() {
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

    sel->insert_default(DISPLAY_NONE, "");
    sel->update();
}

class SAI_sellst_spec : public SelectionListSpec, virtual Noncopyable {
    GBDATA                 *gb_main;
    awt_sai_sellist_filter  filter_poc;
    AW_CL                   filter_cd;

    AW_DB_selection *create(AW_selection_list *sellist) const {
        GB_transaction ta(gb_main);
        return new SAI_selection(sellist, GBT_get_SAI_data(gb_main), filter_poc, filter_cd);
    }

public:
    SAI_sellst_spec(const char *awar_name_, GBDATA *gb_main_, awt_sai_sellist_filter filter_poc_, AW_CL filter_cd_)
        : SelectionListSpec(awar_name_),
          gb_main(gb_main_),
          filter_poc(filter_poc_),
          filter_cd(filter_cd_)
    {
        // Warning: do not use different filters for same awar! (wont work as expected) // @@@ add assertion against
    }

    const char *get_macro_id() const { return "SELECT_SAI"; }
    const char *get_title() const { return "Select SAI"; }
};

void awt_popup_SAI_selection_list(AW_window *, const char *awar_name, GBDATA *gb_main) {
    SAI_sellst_spec spec(awar_name, gb_main, NULL, 0);
    spec.popup();
}

AW_DB_selection *awt_create_SAI_selection_list(GBDATA *gb_main, AW_window *aws, const char *varname, bool fallback2default, awt_sai_sellist_filter filter_poc, AW_CL filter_cd) {
    /* Selection list for SAIs
     *
     * if filter_proc is set then show only those items on which
     * filter_proc returns a string (string must be a heap copy)
     */
    SAI_sellst_spec spec(varname, gb_main, filter_poc, filter_cd);
    return spec.create_list(aws, fallback2default);
}

void awt_create_SAI_selection_button(GBDATA *gb_main, AW_window *aws, const char *varname, awt_sai_sellist_filter filter_poc, AW_CL filter_cd) {
    (new SAI_sellst_spec(varname, gb_main, filter_poc, filter_cd))->createButton(aws); // do not free yet (bound to callback in ARB_MOTIF)
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
        if (fnames.empty() && GB_have_error()) {
            error = GB_await_error();
        }
    }

    StrArray lines;
    for (int f = 0; fnames[f] && !error; ++f) {
        FILE *in = fopen(fnames[f], "rb");
        if (!in) {
            error = GB_IO_error("reading", fnames[f]);
        }
        else {
            BufferedFileReader file(fnames[f], in);
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

static void save_list_cb(AW_window *aww, const StorableSelectionList *storabsellist) {
    const TypedSelectionList& typedsellist = storabsellist->get_typedsellist();

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

static void load_list_cb(AW_window *aww, const StorableSelectionList *storabsellist) {
    const TypedSelectionList& typedsellist = storabsellist->get_typedsellist();

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

AW_window *create_save_box_for_selection_lists(AW_root *aw_root, const StorableSelectionList *storabsellist) {
    const TypedSelectionList& typedsellist = storabsellist->get_typedsellist();

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
    aws->callback(makeWindowCallback(save_list_cb, storabsellist));
    aws->create_button("SAVE", "SAVE", "S");

    aws->at("nlines");
    aws->create_option_menu(awar_line_anz, true);
    aws->insert_default_option("all",   "a", 0);
    aws->insert_option        ("10",    "",  10);
    aws->insert_option        ("50",    "",  50);
    aws->insert_option        ("100",   "",  100);
    aws->insert_option        ("500",   "",  500);
    aws->insert_option        ("1000",  "",  1000);
    aws->insert_option        ("5000",  "",  5000);
    aws->insert_option        ("10000", "",  10000);
    aws->update_option_menu();

    AW_create_standard_fileselection(aws, awar_base);

    free(window_title);
    free(window_id);
    free(awar_line_anz);
    free(awar_base);

    aws->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    return aws;
}

AW_window *create_load_box_for_selection_lists(AW_root *aw_root, const StorableSelectionList *storabsellist) {
    const TypedSelectionList& typedsellist = storabsellist->get_typedsellist();

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
    aws->callback(makeWindowCallback(load_list_cb, storabsellist));
    aws->create_button("LOAD", "LOAD", "L");

    aws->at("append");
    aws->label("Append?");
    aws->create_toggle(awar_append);

    AW_create_fileselection(aws, awar_base_name, "", "PWD", ANY_DIR, true);

    aws->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    free(window_title);
    free(window_id);
    free(awar_append);
    free(awar_base_name);

    return aws;
}

void awt_clear_selection_list_cb(AW_window *, AW_selection_list *sellist) {
    sellist->clear();
    sellist->insert_default("", "");
    sellist->update();
}

void create_print_box_for_selection_lists(AW_window *aw_window, const TypedSelectionList *typedsellist) {
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
#if defined(ARB_GTK)
    aws->set_close_action("CLOSE");
#endif

#if 0
    // @@@ allow to pass helpfile
    aws->at("help");
    aws->callback(makeHelpCallback(""));
    aws->create_button("HELP", "HELP");
#endif

    aws->at("go");
    aws->callback(ok_cb);
    aws->create_autosize_button("GO", action);

    AW_create_standard_fileselection(aws, base_name);
    free(base_name);
    free(what_key);
    aws->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

    return aws;
}

// --------------------------------------------------------------------------------

#define SUBSET_NOELEM_DISPLAY "<none>"

class AW_subset_selection : public AW_selection {
    AW_selection_list& parent_sellist;

    SubsetChangedCb subChanged_cb;
    AW_CL           cl_user;

    static void finish_fill_box(AW_selection_list *parent_sellist, AW_selection_list *sub_sellist) {
        sub_sellist->insert_default(parent_sellist->get_default_display(), parent_sellist->get_default_value());
        sub_sellist->update();
    }

    static AW_selection_list *create_box(AW_window *aww, AW_selection_list& parent_sellist) {
        const char *parent_awar_name = parent_sellist.get_awar_name();
        awt_assert(parent_awar_name[0] != '/');
        awt_assert(parent_sellist.get_awar_type() == GB_STRING); // only impl for strings

        AW_root *aw_root   = aww->get_root();
        char    *awar_name = GBS_global_string_copy("tmp/subsel/%s", parent_awar_name);

        aw_root->awar_string(awar_name);

        AW_selection_list *sub_sellist = aww->create_selection_list(awar_name, true);
        finish_fill_box(&parent_sellist, sub_sellist);

        free(awar_name);

        return sub_sellist;
    }

    void callChangedCallback(bool interactive_change) { if (subChanged_cb) subChanged_cb(this, interactive_change, cl_user); }

public:
    AW_subset_selection(AW_window *aww, AW_selection_list& parent_sellist_, SubsetChangedCb subChanged_cb_, AW_CL cl_user_)
        : AW_selection(create_box(aww, parent_sellist_)),
          parent_sellist(parent_sellist_),
          subChanged_cb(subChanged_cb_),
          cl_user(cl_user_)
    {
        callChangedCallback(false);
    }

    AW_selection_list *get_parent_sellist() const { return &parent_sellist; }

    const char *default_select_value() const { return parent_sellist.get_default_value(); }
    const char *default_select_display() const { return parent_sellist.get_default_display(); }

    void fill() OVERRIDE { awt_assert(0); } // unused

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
        callChangedCallback(true);
    }
    void reorder_subset_cb(awt_reorder_mode dest) {
        AW_selection_list *subset_list = get_sellist();

        if (!subset_list->default_is_selected()) {
            const char *selected = subset_list->get_awar_value();

            StrArray listContent;
            subset_list->to_array(listContent, true);

            int old_pos = listContent.index_of(selected);
            if (old_pos >= 0) {
                int new_pos = 0;
                switch (dest) {
                    case ARM_TOP:    new_pos= 0;         break;
                    case ARM_UP:     new_pos= old_pos-1; break;
                    case ARM_DOWN:   new_pos= old_pos+1; break;
                    case ARM_BOTTOM: new_pos= -1;        break;
                }
                if (old_pos != new_pos) {
                    listContent.move(old_pos, new_pos);
                    subset_list->init_from_array(listContent, subset_list->get_default_display(), subset_list->get_default_value());
                }
            }
        }
        callChangedCallback(true);
    }

    void delete_entries_missing_in_parent() {
        // check subset for entries missing in parent,
        // delete these and update
        typedef std::set<const char*, charpLess> Entries;

        bool    deleted = false;
        Entries pEntry;
        {
            AW_selection_list_iterator pIter(&parent_sellist);
            while (pIter) {
                pEntry.insert(pIter.get_value());
                ++pIter;
            }
        }

        AW_selection_list *subsel = get_sellist();
        int                size   = subsel->size();

        for (int i = 0; i<size; ++i) {
            if (pEntry.find(subsel->get_value_at(i)) == pEntry.end()) { // entry missing in parent list
                subsel->delete_element_at(i);
                deleted = true;
                --i; --size;
            }
        }

        if (deleted) {
            subsel->update();
            callChangedCallback(false);
        }
    }

    void fill_entries_matching_values(const CharPtrArray& values) {
        AW_selection_list *subset_list = get_sellist();
        subset_list->clear();

        for (size_t e = 0; e<values.size(); ++e) {
            const char *value = values[e];

            AW_selection_list_iterator pIter(&parent_sellist);
            while (pIter) {
                if (strcmp(pIter.get_value(), value) == 0) {
                    subset_list->insert(pIter.get_displayed(), pIter.get_value());
                    break;
                }
                ++pIter;
            }
        }

        finish_fill_box(&parent_sellist, subset_list);
        callChangedCallback(false);
    }
};

static void collect_subset_cb(AW_window *, awt_collect_mode what, AW_subset_selection *subsel) { subsel->collect_subset_cb(what); }
static void reorder_subset_cb(AW_window *, awt_reorder_mode dest, AW_subset_selection *subsel) { subsel->reorder_subset_cb(dest); }

static void correct_subselection_cb(AW_selection_list *IF_ASSERTION_USED(parent_sel), AW_CL cl_subsel) {
    AW_subset_selection *subsel = (AW_subset_selection*)cl_subsel;
    aw_assert(subsel->get_parent_sellist() == parent_sel);
    subsel->delete_entries_missing_in_parent();
}

AW_selection *awt_create_subset_selection_list(AW_window *aww, AW_selection_list *parent_selection, const char *at_box, const char *at_add, const char *at_sort, bool autocorrect_subselection, SubsetChangedCb subChanged_cb, AW_CL cl_user) {
    awt_assert(parent_selection);

    aww->at(at_box);
    int x_list = aww->get_at_xposition();

    AW_subset_selection *subsel = new AW_subset_selection(aww, *parent_selection, subChanged_cb, cl_user);

    aww->button_length(0);

    aww->at(at_add);
    int x_buttons = aww->get_at_xposition();

    bool move_rightwards = x_list>x_buttons;
    awt_create_collect_buttons(aww, move_rightwards, collect_subset_cb, subsel);

    aww->at(at_sort);
    awt_create_order_buttons(aww, reorder_subset_cb, subsel);

    if (autocorrect_subselection) parent_selection->set_update_callback(correct_subselection_cb, AW_CL(subsel));

    return subsel;
}

void awt_set_subset_selection_content(AW_selection *subset_sel_, const CharPtrArray& values) {
    /*! sets content of a subset-selection-list
     * @param subset_sel_ selection list created by awt_create_subset_selection_list()
     * @param values      e.g. retrieved using subset_sel_->get_values()
     */
    AW_subset_selection *subset_sel = dynamic_cast<AW_subset_selection*>(subset_sel_);
    if (subset_sel) subset_sel->fill_entries_matching_values(values);
}

AW_selection_list *awt_create_selection_list_with_input_field(AW_window *aww, const char *awar_name, const char *at_box, const char *at_field) {
    /*! create selection_list and input_field on awar 'awar_name'
     * @param aww window where to create gui elements
     * @param at_box position of selection_list
     * @param at_field position of input_field
     */

    aww->at(at_field);
    aww->create_input_field(awar_name);
    aww->at(at_box);
    return aww->create_selection_list(awar_name, false);
}


