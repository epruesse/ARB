// =============================================================== //
//                                                                 //
//   File      : NT_edconf.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"
#include "ad_trees.h"

#include <TreeNode.h>
#include <TreeDisplay.hxx>
#include <RegExpr.hxx>

#include <awt_sel_boxes.hxx>
#include <awt_misc.hxx>
#include <awt_config_manager.hxx>
#include <awt_modules.hxx>

#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <aw_select.hxx>

#include <ad_config.h>
#include <ad_cb_prot.h>

#include <arb_strbuf.h>
#include <arb_global_defs.h>
#include <arb_strarray.h>

#include <map>
#include <set>
#include <string>

using namespace std;

// AWT_canvas-local (%i=canvas-id):
#define AWAR_CL_SELECTED_CONFIGS       "configuration_data/win%i/selected"
#define AWAR_CL_DISPLAY_CONFIG_MARKERS "configuration_data/win%i/display"

#define AWAR_CONFIG_COMMENT "tmp/configuration/comment"

enum extractType {
    CONF_EXTRACT,
    CONF_MARK,
    CONF_UNMARK,
    CONF_INVERT,
    CONF_COMBINE // logical AND
};
static void nt_extract_configuration(UNFIXED, extractType ext_type);

typedef map<string, string> ConfigHits; // key=speciesname; value[markerIdx]==1 -> highlighted

class ConfigMarkerDisplay : public MarkerDisplay, virtual Noncopyable {
    GBDATA                  *gb_main;
    SmartPtr<ConstStrArray>  config; // configuration names
    StrArray                 errors; // config load-errors
    ConfigHits               hits;

    void updateHits() {
        flush_cache();
        hits.clear();
        errors.erase();
        for (int c = 0; c<size(); ++c) {
            GB_ERROR   error;
            GBT_config cfg(gb_main, (*config)[c], error);

            for (int area = 0; area<=1 && !error; ++area) {
                GBT_config_parser cparser(cfg, area);

                while (1) {
                    const GBT_config_item& item = cparser.nextItem(error);
                    if (error || item.type == CI_END_OF_CONFIG) break;
                    if (item.type == CI_SPECIES) {
                        ConfigHits::iterator found = hits.find(item.name);
                        if (found == hits.end()) {
                            string h(size(), '0');
                            h[c]            = '1';
                            hits[item.name] = h;
                        }
                        else {
                            (found->second)[c] = '1';
                        }
                    }
                }
            }

            errors.put(ARB_strdup(error ? error : ""));
        }
    }

public:
    ConfigMarkerDisplay(SmartPtr<ConstStrArray> config_, GBDATA *gb_main_)
        : MarkerDisplay(config_->size()),
          gb_main(gb_main_),
          config(config_)
    {
        updateHits();
    }
    const char *get_marker_name(int markerIdx) const OVERRIDE {
        const char *error = errors[markerIdx];
        const char *name  = (*config)[markerIdx];
        if (error && error[0]) return GBS_global_string("%s (Error: %s)", name, error);
        return name;
    }
    void retrieve_marker_state(const char *speciesName, NodeMarkers& node) OVERRIDE {
        ConfigHits::const_iterator found = hits.find(speciesName);
        if (found != hits.end()) {
            const string& hit = found->second;

            for (int c = 0; c<size(); ++c) {
                if (hit[c] == '1') node.incMarker(c);
            }
        }
        node.incNodeSize();
    }

    void handle_click(int markerIdx, AW_MouseButton button, AWT_graphic_exports& exports) OVERRIDE {
        if (button == AW_BUTTON_LEFT || button == AW_BUTTON_RIGHT) {
            AW_root::SINGLETON->awar(AWAR_CONFIGURATION)->write_string(get_marker_name(markerIdx)); // select config of clicked marker
            if (button == AW_BUTTON_RIGHT) { // extract configuration
                nt_extract_configuration(NULL, CONF_EXTRACT);
                exports.structure_change = 1; // needed to recalculate branch colors
            }
        }
    }
};

inline bool displays_config_markers(MarkerDisplay *md) { return dynamic_cast<ConfigMarkerDisplay*>(md); }

#define CONFIG_SEPARATOR "\1"

inline AW_awar *get_canvas_awar(const char *awar_name_format, int canvas_id) {
    return AW_root::SINGLETON->awar_no_error(GBS_global_string(awar_name_format, canvas_id));
}
inline AW_awar *get_config_awar        (int canvas_id) { return get_canvas_awar(AWAR_CL_SELECTED_CONFIGS,       canvas_id); }
inline AW_awar *get_display_toggle_awar(int canvas_id) { return get_canvas_awar(AWAR_CL_DISPLAY_CONFIG_MARKERS, canvas_id); }

static SmartPtr<ConstStrArray> get_selected_configs_from_awar(int canvas_id) {
    // returns configs stored in awar as array (empty array if awar undefined!)
    SmartPtr<ConstStrArray> config(new ConstStrArray);

    AW_awar *awar = get_config_awar(canvas_id);
    if (awar) {
        char *config_str = awar->read_string();
        GBT_splitNdestroy_string(*config, config_str, CONFIG_SEPARATOR, true);
    }

    return config;
}
static void write_configs_to_awar(int canvas_id, const CharPtrArray& configs) {
    char *config_str = GBT_join_strings(configs, CONFIG_SEPARATOR[0]);
    AW_root::SINGLETON->awar(GBS_global_string(AWAR_CL_SELECTED_CONFIGS, canvas_id))->write_string(config_str);
    free(config_str);
}

// --------------------------------------------------------------------------------

static AW_selection *selected_configs_list[MAX_NT_WINDOWS] = { MAX_NT_WINDOWS_NULLINIT };
static bool allow_selection2awar_update = true;
static bool allow_to_activate_display   = false;

static void init_config_awars(AW_root *root) {
    root->awar_string(AWAR_CONFIGURATION, DEFAULT_CONFIGURATION, GLOBAL.gb_main);
}
static void selected_configs_awar_changed_cb(AW_root *aw_root, AWT_canvas *ntw) {
    AWT_graphic_tree        *agt    = DOWNCAST(AWT_graphic_tree*, ntw->gfx);
    int                      ntw_id = NT_get_canvas_idx(ntw);
    SmartPtr<ConstStrArray>  config = get_selected_configs_from_awar(ntw_id);
    bool                     redraw = false;

    if (config->empty() || get_display_toggle_awar(ntw_id)->read_int() == 0) {
        if (displays_config_markers(agt->get_marker_display())) { // only hide config markers
            agt->hide_marker_display();
            redraw = true;
        }
    }
    else {
        bool activate = allow_to_activate_display || displays_config_markers(agt->get_marker_display());

        if (activate) {
            init_config_awars(aw_root);
            ConfigMarkerDisplay *disp = new ConfigMarkerDisplay(config, ntw->gb_main);
            agt->set_marker_display(disp);
            redraw = true;
        }
    }

    if (selected_configs_list[ntw_id]) { // if configuration_marker_window has been opened
        // update content of subset-selection (needed when reloading a config-set (not implemented yet) or after renaming a config)
        LocallyModify<bool> avoid(allow_selection2awar_update, false);
        awt_set_subset_selection_content(selected_configs_list[ntw_id], *config);
    }

    if (redraw) AW_root::SINGLETON->awar(AWAR_TREE_REFRESH)->touch();
}

static void selected_configs_display_awar_changed_cb(AW_root *root, AWT_canvas *ntw) {
    LocallyModify<bool> allowInteractiveActivation(allow_to_activate_display, true);
    selected_configs_awar_changed_cb(root, ntw);
}

static void configs_selectionlist_changed_cb(AW_selection *selected_configs, bool interactive_change, AW_CL ntw_id) {
    if (allow_selection2awar_update) {
        LocallyModify<bool> allowInteractiveActivation(allow_to_activate_display, interactive_change);

        StrArray config;
        selected_configs->get_values(config);
        write_configs_to_awar(ntw_id, config);
    }
}

static void config_modified_cb(GBDATA *gb_cfg_area) { // called with "top_area" AND "middle_area" entry!
    static GBDATA   *gb_lastname = NULL;
    static GB_ULONG  lastcall     = 0;

    GBDATA   *gb_name  = GB_entry(GB_get_father(gb_cfg_area), "name");
    GB_ULONG  thiscall = GB_time_of_day();

    bool is_same_modification = gb_name == gb_lastname && (thiscall == lastcall || thiscall == (lastcall+1));
    if (!is_same_modification) { // avoid duplicate check if "top_area" and "middle_area" changed (=standard case)
        // touch all canvas-specific awars that contain 'name'
        const char *name = GB_read_char_pntr(gb_name);

        for (int canvas_id = 0; canvas_id<MAX_NT_WINDOWS; ++canvas_id) {
            SmartPtr<ConstStrArray> config = get_selected_configs_from_awar(canvas_id);
            for (size_t c = 0; c<config->size(); ++c) {
                if (strcmp((*config)[c], name) == 0) {
                    get_config_awar(canvas_id)->touch();
                    break;
                }
            }
        }
    }
    gb_lastname = gb_name;
    lastcall    = thiscall;
}

#define CONFIG_BASE_PATH "/configuration_data/configuration"

static void install_config_change_callbacks(GBDATA *gb_main) {
    static bool installed = false;
    if (!installed) {
        DatabaseCallback dbcb = makeDatabaseCallback(config_modified_cb);
        ASSERT_NO_ERROR(GB_add_hierarchy_callback(gb_main, CONFIG_BASE_PATH "/middle_area", GB_CB_CHANGED, dbcb));
        ASSERT_NO_ERROR(GB_add_hierarchy_callback(gb_main, CONFIG_BASE_PATH "/top_area",    GB_CB_CHANGED, dbcb));

        installed = true;
    }
}

void NT_activate_configMarkers_display(AWT_canvas *ntw) {
    GBDATA *gb_main = ntw->gb_main;

    AW_awar *awar_selCfgs = ntw->awr->awar_string(GBS_global_string(AWAR_CL_SELECTED_CONFIGS, NT_get_canvas_idx(ntw)), "", gb_main);
    awar_selCfgs->add_callback(makeRootCallback(selected_configs_awar_changed_cb, ntw));

    AW_awar *awar_dispCfgs = ntw->awr->awar_int(GBS_global_string(AWAR_CL_DISPLAY_CONFIG_MARKERS, NT_get_canvas_idx(ntw)), 1, gb_main);
    awar_dispCfgs->add_callback(makeRootCallback(selected_configs_display_awar_changed_cb, ntw));

    awar_selCfgs->touch(); // force initial refresh
    install_config_change_callbacks(gb_main);
}

// define where to store config-sets (using config-manager):
#define MANAGED_CONFIGSET_SECTION "configmarkers"
#define MANAGED_CONFIGSET_ENTRY   "selected_configs"

static void setup_configmarker_config_cb(AWT_config_definition& config, int ntw_id) {
    AW_awar *selcfg_awar = get_config_awar(ntw_id);
    nt_assert(selcfg_awar);
    if (selcfg_awar) {
        config.add(selcfg_awar->awar_name, MANAGED_CONFIGSET_ENTRY);
    }
}

struct ConfigModifier : virtual Noncopyable {
    virtual ~ConfigModifier() {}
    virtual const char *modify(const char *old) const = 0;

    bool modifyConfig(ConstStrArray& config) const {
        bool changed = false;
        for (size_t i = 0; i<config.size(); ++i) {
            const char *newContent = modify(config[i]);
            if (!newContent) {
                config.remove(i);
                changed = true;
            }
            else if (strcmp(newContent, config[i]) != 0) {
                config.replace(i, newContent);
                changed = true;
            }
        }
        return changed;
    }
};
class ConfigRenamer : public ConfigModifier { // derived from Noncopyable
    const char *oldName;
    const char *newName;
    const char *modify(const char *name) const OVERRIDE {
        return strcmp(name, oldName) == 0 ? newName : name;
    }
public:
    ConfigRenamer(const char *oldName_, const char *newName_)
        : oldName(oldName_),
          newName(newName_)
    {}
};
class ConfigDeleter : public ConfigModifier { // derived from Noncopyable
    const char *toDelete;
    const char *modify(const char *name) const OVERRIDE {
        return strcmp(name, toDelete) == 0 ? NULL : name;
    }
public:
    ConfigDeleter(const char *toDelete_)
        : toDelete(toDelete_)
    {}
};

static char *correct_managed_configsets_cb(const char *key, const char *value, AW_CL cl_ConfigModifier) {
    char *modified_value = NULL;
    if (strcmp(key, MANAGED_CONFIGSET_ENTRY) == 0) {
        const ConfigModifier *mod = (const ConfigModifier*)cl_ConfigModifier;
        ConstStrArray         config;
        GBT_split_string(config, value, CONFIG_SEPARATOR, true);
        if (mod->modifyConfig(config)) {
            modified_value = GBT_join_strings(config, CONFIG_SEPARATOR[0]);
        }
    }
    return modified_value ? modified_value : ARB_strdup(value);
}
static void modify_configurations(const ConfigModifier& mod) {
    for (int canvas_id = 0; canvas_id<MAX_NT_WINDOWS; ++canvas_id) {
        // modify currently selected configs:
        SmartPtr<ConstStrArray> config = get_selected_configs_from_awar(canvas_id);
        if (mod.modifyConfig(*config)) {
            write_configs_to_awar(canvas_id, *config);
        }
    }
    // change all configuration-sets stored in config-manager (shared by all windows)
    AWT_modify_managed_configs(GLOBAL.gb_main, MANAGED_CONFIGSET_SECTION, correct_managed_configsets_cb, AW_CL(&mod));
}

static void configuration_renamed_cb(const char *old_name, const char *new_name) { modify_configurations(ConfigRenamer(old_name, new_name)); }
static void configuration_deleted_cb(const char *name)                           { modify_configurations(ConfigDeleter(name)); }

static AW_window *create_configuration_marker_window(AW_root *root, AWT_canvas *ntw) {
    AW_window_simple *aws = new AW_window_simple;

    int ntw_id = NT_get_canvas_idx(ntw);
    aws->init(root, GBS_global_string("MARK_CONFIGS_%i", ntw_id), "Highlight configurations in tree");
    aws->load_xfig("mark_configs.fig");

    aws->auto_space(10, 10);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("species_configs_highlight.hlp"));
    aws->create_button("HELP", "HELP", "H");


    aws->at("list");
    AW_DB_selection *all_configs = awt_create_CONFIG_selection_list(GLOBAL.gb_main, aws, AWAR_CONFIGURATION, true);
    AW_selection *sub_sel;
    {
        LocallyModify<bool> avoid(allow_selection2awar_update, false); // avoid awar gets updated from empty sub-selectionlist
        sub_sel = awt_create_subset_selection_list(aws, all_configs->get_sellist(), "selected", "add", "sort", false, configs_selectionlist_changed_cb, NT_get_canvas_idx(ntw));
    }

    awt_set_subset_selection_content(sub_sel, *get_selected_configs_from_awar(ntw_id));
    selected_configs_list[ntw_id] = sub_sel;

    // @@@ would like to use ntw-specific awar for this selection list (opening two lists links them)

    aws->at("show");
    aws->label("Display?");
    aws->create_toggle(get_display_toggle_awar(ntw_id)->awar_name);

    aws->at("settings");
    aws->callback(TREE_create_marker_settings_window);
    aws->create_autosize_button("SETTINGS", "Settings", "S");

    AWT_insert_config_manager(aws, GLOBAL.gb_main, MANAGED_CONFIGSET_SECTION, makeConfigSetupCallback(setup_configmarker_config_cb, ntw_id));

    return aws;
}

// -----------------------------
//      class Store_species

class Store_species : virtual Noncopyable {
    // stores an amount of species:
    TreeNode *node;
    Store_species *next;
public:
    Store_species(TreeNode *aNode) {
        node = aNode;
        next = 0;
    }
    ~Store_species();

    Store_species* add(Store_species *list) {
        nt_assert(next==0);
        next = list;
        return this;
    }

    Store_species* remove() {
        Store_species *follower = next;
        next = 0;
        return follower;
    }

    TreeNode *getNode() const { return node; }

    void call(void (*aPizza)(TreeNode*)) const;
};

Store_species::~Store_species() {
    delete next;
}

void Store_species::call(void (*aPizza)(TreeNode*)) const {
    aPizza(node);
    if (next) next->call(aPizza);
}

static void unmark_species(TreeNode *node) {
    nt_assert(node);
    nt_assert(node->gb_node);
    nt_assert(GB_read_flag(node->gb_node)!=0);
    GB_write_flag(node->gb_node, 0);
}

static void mark_species(TreeNode *node, Store_species **extra_marked_species) {
    nt_assert(node);
    nt_assert(node->gb_node);
    nt_assert(GB_read_flag(node->gb_node)==0);
    GB_write_flag(node->gb_node, 1);

    *extra_marked_species = (new Store_species(node))->add(*extra_marked_species);
}



static TreeNode *rightmost_leaf(TreeNode *node) {
    nt_assert(node);
    while (!node->is_leaf) {
        node = node->get_rightson();
        nt_assert(node);
    }
    return node;
}

static TreeNode *left_neighbour_leaf(TreeNode *node) {
    if (node) {
        TreeNode *father = node->get_father();
        while (father) {
            if (father->rightson==node) {
                node = rightmost_leaf(father->get_leftson());
                nt_assert(node->is_leaf);
                if (!node->gb_node) { // Zombie
                    node = left_neighbour_leaf(node);
                }
                return node;
            }
            node = father;
            father = node->get_father();
        }
    }
    return 0;
}

static int nt_build_conf_string_rek(GB_HASH *used, TreeNode *tree, GBS_strstruct *memfile,
                             Store_species **extra_marked_species, int use_species_aside,
                             int *auto_mark, int marked_at_left, int *marked_at_right)
{
    /*! Builds a configuration string from a tree.
     *
     * @param used                      all species inserted by this function are stored here
     * @param tree                      used for group information
     * @param memfile                   generated configuration string is stored here
     * @param extra_marked_species      all extra marked species are inserted here
     * @param use_species_aside         number of species to mark left and right of marked species
     * @param auto_mark                 number species to extra-mark (if not already marked)
     * @param marked_at_left            number of species which were marked (looking to left)
     * @param marked_at_right           number of species which are marked (when returning from recursion)
     *
     * @return the number of marked species
     *
     * --------------------------------------------------
     * Format of configuration string : [Part]+ \0
     *
     * Part : '\A' ( Group | Species | Sai )
     *
     * Group : ( OpenedGroup | ClosedGroup )
     * OpenedGroup : 'G' GroupDef
     * ClosedGroup : 'F' GroupDef
     * GroupDef : 'groupname' [PART]* EndGroup
     * EndGroup : '\AE'
     *
     * SPECIES : 'L' 'speciesname'
     * SAI : 'S' 'sainame'
     *
     * \0 : ASCII 0 (eos)
     * \A : ASCII 1
     */

    if (!tree) return 0;
    if (tree->is_leaf) {
        if (!tree->gb_node) {
            *marked_at_right = marked_at_left;
            return 0;   // Zombie
        }

        if (!GB_read_flag(tree->gb_node)) { // unmarked species
            if (*auto_mark) {
                (*auto_mark)--;
                mark_species(tree, extra_marked_species);
            }
            else {
                *marked_at_right = 0;
                return 0;
            }
        }
        else { // marked species
            if (marked_at_left<use_species_aside) {
                // on the left side there are not as many marked species as needed!

                nt_assert(marked_at_left>=0);

                TreeNode *leaf_at_left = tree;
                int       step_over    = marked_at_left+1; // step over myself
                int       then_mark    = use_species_aside-marked_at_left;

                while (step_over--) { // step over self and over any adjacent, marked species
                    leaf_at_left = left_neighbour_leaf(leaf_at_left);
                }

                Store_species *marked_back = 0;
                while (leaf_at_left && then_mark--) { // then additionally mark some species
                    if (GB_read_flag(leaf_at_left->gb_node) == 0) { // if they are not marked yet
                        mark_species(leaf_at_left, extra_marked_species);
                        marked_back = (new Store_species(leaf_at_left))->add(marked_back);
                    }
                    leaf_at_left = left_neighbour_leaf(leaf_at_left);
                }

                while (marked_back) {
                    GBS_chrcat(memfile, 1);             // Separated by 1
                    GBS_strcat(memfile, "L");
                    GBS_strcat(memfile, marked_back->getNode()->name);
                    GBS_write_hash(used, marked_back->getNode()->name, 1);      // Mark species

                    Store_species *rest = marked_back->remove();
                    delete marked_back;
                    marked_back = rest;
                }

                marked_at_left = use_species_aside;
            }
            // now use_species_aside species to left are marked!
            *auto_mark = use_species_aside;
        }

        GBS_chrcat(memfile, 1);             // Separated by 1
        GBS_strcat(memfile, "L");
        GBS_strcat(memfile, tree->name);
        GBS_write_hash(used, tree->name, 1);    // Mark species

        *marked_at_right = marked_at_left+1;
        return 1;
    }

    long oldpos = GBS_memoffset(memfile);
    if (tree->gb_node && tree->name) {      // but we are a group
        GBDATA *gb_grouped = GB_entry(tree->gb_node, "grouped");
        GBS_chrcat(memfile, 1);             // Separated by 1
        if (gb_grouped && GB_read_byte(gb_grouped)) {
            GBS_strcat(memfile, "F");
        }
        else {
            GBS_strcat(memfile, "G");
        }

        GBS_strcat(memfile, tree->name);
    }

    int  right_of_leftson;
    long nspecies=   nt_build_conf_string_rek(used, tree->get_leftson(),  memfile, extra_marked_species, use_species_aside, auto_mark, marked_at_left,   &right_of_leftson);
    nspecies      += nt_build_conf_string_rek(used, tree->get_rightson(), memfile, extra_marked_species, use_species_aside, auto_mark, right_of_leftson, marked_at_right);

    if (tree->gb_node && tree->name) {      // but we are a group
        GBS_chrcat(memfile, 1);         // Separated by 1
        GBS_chrcat(memfile, 'E');        // Group end indicated by 'E'
    }

    if (!nspecies) {
        long newpos = GBS_memoffset(memfile);
        GBS_str_cut_tail(memfile, newpos-oldpos);   // delete group info
    }
    return nspecies;
}

struct SAI_string_builder {
    GBS_strstruct *sai_middle;
    const char    *last_group_name;
};

static long nt_build_sai_string_by_hash(const char *key, long val, void *cd_sai_builder) {
    SAI_string_builder *sai_builder = (SAI_string_builder*)cd_sai_builder;

    const char *sep = strchr(key, 1);
    if (!sep) return val;                           // what's wrong

    GBS_strstruct *sai_middle      = sai_builder->sai_middle;
    const char    *last_group_name = sai_builder->last_group_name;

    if (!last_group_name || strncmp(key, last_group_name, sep-key)) { // new group
        if (last_group_name) {
            GBS_chrcat(sai_middle, 1);              // Separated by 1
            GBS_chrcat(sai_middle, 'E');             // End of old group
        }
        GBS_chrcat(sai_middle, 1);                  // Separated by 1
        GBS_strcat(sai_middle, "FSAI:");
        GBS_strncat(sai_middle, key, sep-key);
        sai_builder->last_group_name = key;
    }
    GBS_chrcat(sai_middle, 1);                      // Separated by 1
    GBS_strcat(sai_middle, "S");
    GBS_strcat(sai_middle, sep+1);
    return val;
}


static void nt_build_sai_string(GBS_strstruct *topfile, GBS_strstruct *middlefile) {
    //! collect all Sais, place some SAI in top area, rest in middle

    GBDATA *gb_sai_data = GBT_get_SAI_data(GLOBAL.gb_main);
    if (gb_sai_data) {
        GB_HASH *hash = GBS_create_hash(GB_number_of_subentries(gb_sai_data), GB_IGNORE_CASE);

        for (GBDATA *gb_sai = GBT_first_SAI_rel_SAI_data(gb_sai_data); gb_sai; gb_sai = GBT_next_SAI(gb_sai)) {
            GBDATA *gb_name = GB_search(gb_sai, "name", GB_FIND);
            if (gb_name) {
                char *name = GB_read_string(gb_name);

                if (strcmp(name,  "HELIX") == 0  || strcmp(name,  "HELIX_NR") == 0 || strcmp(name,  "ECOLI") == 0) {
                    GBS_chrcat(topfile, 1);             // Separated by 1
                    GBS_strcat(topfile, "S");
                    GBS_strcat(topfile, name);
                }
                else {
                    GBDATA *gb_gn = GB_search(gb_sai, "sai_group", GB_FIND);
                    char   *gn;

                    if (gb_gn)  gn = GB_read_string(gb_gn);
                    else        gn = ARB_strdup("SAI's");

                    char *cn = new char[strlen(gn) + strlen(name) + 2];
                    sprintf(cn, "%s%c%s", gn, 1, name);
                    GBS_write_hash(hash, cn, 1);
                    delete [] cn;
                    free(gn);
                }
                free(name);
            }
        }

        // open surrounding SAI-group:
        GBS_chrcat(middlefile, 1);
        GBS_strcat(middlefile, "GSAI-Maingroup");

        SAI_string_builder sai_builder = { middlefile, 0 };
        GBS_hash_do_sorted_loop(hash, nt_build_sai_string_by_hash, GBS_HCF_sortedByKey, &sai_builder);
        if (sai_builder.last_group_name) {
            GBS_chrcat(middlefile, 1);              // Separated by 1
            GBS_chrcat(middlefile, 'E');             // End of old group
        }

        // close surrounding SAI-group:
        GBS_chrcat(middlefile, 1);
        GBS_chrcat(middlefile, 'E');

        GBS_free_hash(hash);
    }
}

static void nt_build_conf_marked(GB_HASH *used, GBS_strstruct *file) {
    GBS_chrcat(file, 1);            // Separated by 1
    GBS_strcat(file, "FMore Sequences");
    GBDATA *gb_species;
    for (gb_species = GBT_first_marked_species(GLOBAL.gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species)) {
        char *name = GBT_read_string(gb_species, "name");
        if (GBS_read_hash(used, name)) {
            free(name);
            continue;
        }
        GBS_chrcat(file, 1);
        GBS_strcat(file, "L");
        GBS_strcat(file, name);
        free(name);
    }

    GBS_chrcat(file, 1); // Separated by 1
    GBS_chrcat(file, 'E');   // Group end indicated by 'E'
}

static void nt_extract_configuration(UNFIXED, extractType ext_type) {
    GB_transaction  ta(GLOBAL.gb_main);
    AW_root        *aw_root = AW_root::SINGLETON;
    char           *cn      = aw_root->awar(AWAR_CONFIGURATION)->read_string();

    if (strcmp(cn, NO_CONFIG_SELECTED) == 0) {
        aw_message("Please select a configuration");
    }
    else {
        GB_ERROR   error = NULL;
        GBT_config cfg(GLOBAL.gb_main, cn, error);

        if (!error) {
            size_t  unknown_species = 0;
            bool    refresh         = false;

            GB_HASH *was_marked = NULL; // only used for CONF_COMBINE

            switch (ext_type) {
                case CONF_EXTRACT: // unmark all
                    GBT_mark_all(GLOBAL.gb_main, 0);
                    refresh = true;
                    break;

                case CONF_COMBINE: // store all marked species in hash and unmark them
                    was_marked = GBT_create_marked_species_hash(GLOBAL.gb_main);
                    GBT_mark_all(GLOBAL.gb_main, 0);
                    refresh    = GBS_hash_elements(was_marked);
                    break;

                default:
                    break;
            }

            for (int area = 0; area<=1 && !error; ++area) {
                GBT_config_parser cparser(cfg, area);

                while (1) {
                    const GBT_config_item& citem = cparser.nextItem(error);
                    if (error || citem.type == CI_END_OF_CONFIG) break;

                    if (citem.type == CI_SPECIES) {
                        GBDATA *gb_species = GBT_find_species(GLOBAL.gb_main, citem.name);

                        if (gb_species) {
                            int oldmark = GB_read_flag(gb_species);
                            int newmark = oldmark;
                            switch (ext_type) {
                                case CONF_EXTRACT:
                                case CONF_MARK:     newmark = 1; break;
                                case CONF_UNMARK:   newmark = 0; break;
                                case CONF_INVERT:   newmark = !oldmark; break;
                                case CONF_COMBINE: {
                                    nt_assert(!oldmark); // should have been unmarked above
                                    newmark = GBS_read_hash(was_marked, citem.name); // mark if was_marked
                                    break;
                                }
                                default: nt_assert(0); break;
                            }
                            if (newmark != oldmark) {
                                GB_write_flag(gb_species, newmark);
                                refresh = true;
                            }
                        }
                        else {
                            unknown_species++;
                        }
                    }
                }
            }

            if (was_marked) GBS_free_hash(was_marked);
            if (unknown_species>0 && !error) error = GBS_global_string("configuration '%s' contains %zu unknown species", cn, unknown_species);
            if (refresh) aw_root->awar(AWAR_TREE_REFRESH)->touch();
        }
        aw_message_if(error);
    }
    free(cn);
}

static void nt_delete_configuration(AW_window *aww, AW_DB_selection *dbsel) {
    GB_transaction ta(GLOBAL.gb_main);

    AW_awar *awar_selected    = aww->get_root()->awar(AWAR_CONFIGURATION);
    char    *name             = awar_selected->read_string();
    GBDATA  *gb_configuration = GBT_find_configuration(GLOBAL.gb_main, name);

    if (gb_configuration) {
        dbsel->get_sellist()->move_selection(1);

        GB_ERROR error = GB_delete(gb_configuration);
        error          = ta.close(error);
        if (error) {
            aw_message(error);
        }
        else {
            configuration_deleted_cb(name);
        }
    }
    free(name);
}

enum ConfigCreation {
    BY_CALLING_THE_EDITOR,
    FROM_IMPORTER,
    FROM_MANAGER,
};

static GB_ERROR nt_create_configuration(TreeNode *tree, const char *conf_name, int use_species_aside, ConfigCreation creation) {
    GB_ERROR error = NULL;

    if (!conf_name || !conf_name[0]) error = "no config name given";
    else {
        if (use_species_aside==-1) {
            static int last_used_species_aside = 3;
            {
                const char *val                    = GBS_global_string("%i", last_used_species_aside);
                char       *use_species            = aw_input("How many extra species to view aside marked:", val);
                if (use_species) use_species_aside = atoi(use_species);
                free(use_species);
            }

            if (use_species_aside<1) error = "illegal number of 'species aside'";
            else last_used_species_aside = use_species_aside; // remember for next time
        }

        if (!error) {
            GB_transaction ta(GLOBAL.gb_main);  // open close transaction

            GBT_config newcfg;
            {
                GB_HASH       *used    = GBS_create_hash(GBT_get_species_count(GLOBAL.gb_main), GB_MIND_CASE);
                GBS_strstruct topfile(1000);
                GBS_strstruct topmid(10000);
                {
                    GBS_strstruct middlefile(10000);
                    nt_build_sai_string(&topfile, &topmid);

                    if (use_species_aside) {
                        Store_species *extra_marked_species = 0;
                        int            auto_mark            = 0;
                        int            marked_at_right;
                    
                        nt_build_conf_string_rek(used, tree, &middlefile, &extra_marked_species, use_species_aside, &auto_mark, use_species_aside, &marked_at_right);
                        if (extra_marked_species) {
                            extra_marked_species->call(unmark_species);
                            delete extra_marked_species;
                        }
                    }
                    else {
                        int dummy_1=0, dummy_2;
                        nt_build_conf_string_rek(used, tree, &middlefile, 0, 0, &dummy_1, 0, &dummy_2);
                    }
                    nt_build_conf_marked(used, &topmid);
                    topmid.ncat(middlefile.get_data(), middlefile.get_position());
                }

                newcfg.set_definition(GBT_config::TOP_AREA,    topfile.release());
                newcfg.set_definition(GBT_config::MIDDLE_AREA, topmid.release());

                GBS_free_hash(used);
            }

            AW_root *awr = AW_root::SINGLETON;

            GBT_config previous(GLOBAL.gb_main, conf_name, error);
            error = NULL; // ignore

            const char *prevComment         = NULL; // old or fixed comment
            const char *comment             = NULL;
            bool        warnIfSavingDefault = true;
            switch (creation) {
                case BY_CALLING_THE_EDITOR: { // always saves DEFAULT_CONFIGURATION!
                    prevComment         = "This configuration will be OVERWRITTEN each time\nARB_EDIT4 is started w/o specifying a config!\n---";
                    comment             = "created for ARB_EDIT4";
                    warnIfSavingDefault = false;
                    break;
                }
                case FROM_MANAGER: {
                    if (previous.exists()) {
                        prevComment = previous.get_comment();
                        comment     = "updated manually";
                    }
                    else {
                        prevComment                      = awr->awar(AWAR_CONFIG_COMMENT)->read_char_pntr();
                        if (!prevComment[0]) prevComment = NULL;
                        comment                          = "created manually";
                    }
                    break;
                }
                case FROM_IMPORTER:
                    nt_assert(!previous.exists());
                    comment = "created by importer";
                    break;
            }

            nt_assert(implicated(prevComment, comment));
            if (comment) {
                // annotate with treename
                const char *treename = awr->awar(AWAR_TREE_NAME)->read_char_pntr();
                if (treename[0]) {
                    comment = GBS_global_string("%s (tree=%s)", comment, treename);
                }
                else {
                    comment = GBS_global_string("%s (no tree)", comment);
                }
                char *dated = GBS_log_dated_action_to(prevComment, comment);
                newcfg.set_comment(dated);
                free(dated);
            }

            error = newcfg.save(GLOBAL.gb_main, conf_name, warnIfSavingDefault);
            awr->awar(AWAR_CONFIGURATION)->touch(); // refreshes comment field
        }
    }

    return error;
}

static void nt_store_configuration(AW_window*, AWT_canvas *ntw) {
    const char *cfgName = AW_root::SINGLETON->awar(AWAR_CONFIGURATION)->read_char_pntr();
    GB_ERROR    err     = nt_create_configuration(NT_get_tree_root_of_canvas(ntw), cfgName, 0, FROM_MANAGER);
    aw_message_if(err);
}

static void nt_rename_configuration(AW_window *aww) {
    AW_awar  *awar_curr_cfg = aww->get_root()->awar(AWAR_CONFIGURATION);
    char     *old_name      = awar_curr_cfg->read_string();

    {
        char *new_name = aw_input("Rename selection", "Enter the new name of the selection", old_name);
        if (new_name) {
            GB_ERROR err = NULL;

            {
                GB_transaction ta(GLOBAL.gb_main);

                GBDATA *gb_existing_cfg  = GBT_find_configuration(GLOBAL.gb_main, new_name);
                if (gb_existing_cfg) err = GBS_global_string("There is already a selection named '%s'", new_name);
                else {
                    GBDATA *gb_old_cfg = GBT_find_configuration(GLOBAL.gb_main, old_name);
                    if (gb_old_cfg) {
                        GBDATA *gb_name = GB_entry(gb_old_cfg, "name");
                        if (gb_name) {
                            err = GB_write_string(gb_name, new_name);
                            if (!err) awar_curr_cfg->write_string(new_name);
                        }
                        else err = "Selection has no name";
                    }
                    else err = "Can't find that selection";
                }
                err = ta.close(err);
            }

            if (err) {
                aw_message(err);
            }
            else {
                nt_assert(GB_get_transaction_level(GLOBAL.gb_main) == 0); // otherwise callback below behaves wrong
                configuration_renamed_cb(old_name, new_name);
            }
            free(new_name);
        }
    }
    free(old_name);
}

static void selected_config_changed_cb(AW_root *root) {
    const char *config = root->awar(AWAR_CONFIGURATION)->read_char_pntr();

    bool    nonexisting_config = false;
    GBDATA *gb_target_commment = NULL;
    if (config[0]) {
        GBDATA *gb_configuration = GBT_find_configuration(GLOBAL.gb_main, config);
        if (gb_configuration) {
            gb_target_commment = GB_entry(gb_configuration, "comment");
        }
        else {
            nonexisting_config = true;
        }
    }

    AW_awar *awar_comment = root->awar(AWAR_CONFIG_COMMENT);
    if (gb_target_commment) {
        if (!awar_comment->is_mapped()) awar_comment->write_string("");
        awar_comment->map(gb_target_commment);
    }
    else {
        char *reuse_comment = nonexisting_config ? awar_comment->read_string() : ARB_strdup("");
        if (awar_comment->is_mapped()) {
            awar_comment->unmap();
        }
        awar_comment->write_string(reuse_comment);
        free(reuse_comment);
    }
}
static void config_comment_changed_cb(AW_root *root) {
    // called when comment-awar changes or gets re-map-ped

    AW_awar    *awar_comment = root->awar(AWAR_CONFIG_COMMENT);
    const char *comment      = awar_comment->read_char_pntr();

    const char *config           = root->awar(AWAR_CONFIGURATION)->read_char_pntr();
    GBDATA     *gb_configuration = config[0] ? GBT_find_configuration(GLOBAL.gb_main, config) : NULL;

    GB_ERROR error = NULL;
    if (awar_comment->is_mapped()) {
        if (!comment[0]) { // empty existing comment
            nt_assert(gb_configuration);
            GBDATA *gb_commment = GB_entry(gb_configuration, "comment");
            nt_assert(gb_commment);
            if (gb_commment) {
                awar_comment->unmap();
                error = GB_delete(gb_commment);
            }
        }
    }
    else {
        if (comment[0]) { // ignore empty comment for unmapped awar
            if (gb_configuration) {
                nt_assert(!GB_entry(gb_configuration, "comment"));
                error = GBT_write_string(gb_configuration, "comment", comment);
                if (!error) {
                    awar_comment->write_string("");
                    selected_config_changed_cb(root);
                }
            }
            else if (!config[0]) {
                // do NOT warn if name field contains (not yet) existing name
                // (allows to edit comment while creating new config)
                error = "Please select an existing species selection to edit its comment";
            }
        }
    }

    aw_message_if(error);
}

static void init_config_admin_awars(AW_root *root) {
    init_config_awars(root);
    root->awar_string(AWAR_CONFIG_COMMENT, "", GLOBAL.gb_main)->add_callback(config_comment_changed_cb);
    root->awar(AWAR_CONFIGURATION)->add_callback(selected_config_changed_cb)->touch();
}

static GB_ERROR swap_configs(GBDATA *gb_main, StrArray& config, int i1, int i2) {
    GB_ERROR error = NULL;
    if (i1>i2) swap(i1, i2); // otherwise overwrite below does not work
    nt_assert(i1<i2 && i1>=0 && i2<int(config.size()));

    GBT_config c1(gb_main, config[i1], error);
    if (!error) {
        GBT_config c2(gb_main, config[i2], error);
        if (!error) error = c1.saveAsOver(gb_main, config[i1], config[i2], false);
        if (!error) error = c2.saveAsOver(gb_main, config[i2], config[i1], false);
        if (!error) config.swap(i1, i2);
    }
    return error;
}

static void reorder_configs_cb(AW_window *aww, awt_reorder_mode mode, AW_DB_selection *sel) {
    AW_root    *awr         = aww->get_root();
    AW_awar    *awar_config = awr->awar(AWAR_CONFIGURATION);
    const char *selected    = awar_config->read_char_pntr();

    if (selected && selected[0]) {
        AW_selection_list *sellist = sel->get_sellist();

        int source_idx = sellist->get_index_of(selected);
        int target_idx = -1;
        switch (mode) {
            case ARM_TOP:    target_idx = 0;            break;
            case ARM_UP:     target_idx = source_idx-1; break;
            case ARM_DOWN:   target_idx = source_idx+1; break;
            case ARM_BOTTOM: target_idx = -1;           break;
        }

        int entries = sellist->size();
        target_idx  = (target_idx+entries)%entries;

        {
            GBDATA         *gb_main = sel->get_gb_main();
            GB_transaction  ta(gb_main);

            StrArray config;
            sellist->to_array(config, true);

            GB_ERROR error = NULL;
            if (source_idx<target_idx) {
                for (int i = source_idx+1; i<=target_idx; ++i) {
                    swap_configs(gb_main, config, i-1, i);
                }
            }
            else if (source_idx>target_idx) {
                for (int i = source_idx-1; i>=target_idx; --i) {
                    swap_configs(gb_main, config, i+1, i);
                }
            }

            error = ta.close(error);
            aw_message_if(error);
        }
        awar_config->touch();
    }
}

static void clear_comment_cb(AW_window *aww) {
    AW_awar *awar_comment = aww->get_root()->awar(AWAR_CONFIG_COMMENT);
    char    *comment      = awar_comment->read_string();

    ConstStrArray line;
    GBT_splitNdestroy_string(line, comment, '\n');

    bool    removedDatedLines = false;
    RegExpr datedLine("^([A-Z][a-z]{2}\\s){2}[0-9]+\\s([0-9]{2}:){2}[0-9]{2}\\s[0-9]{4}:\\s", false); // matches lines created with GBS_log_dated_action_to()
    for (int i = line.size()-1; i >= 0; --i) {
        const RegMatch *match = datedLine.match(line[i]);
        if (match && match->didMatch()) {
            line.safe_remove(i);
            removedDatedLines = true;
        }
    }

    if (!removedDatedLines) line.clear(); // erase all

    comment = GBT_join_strings(line, '\n');
    awar_comment->write_string(comment);
}

static AW_window *create_configuration_admin_window(AW_root *root, AWT_canvas *ntw) {
    static AW_window_simple *existing_aws[MAX_NT_WINDOWS] = { MAX_NT_WINDOWS_NULLINIT };

    int ntw_id = NT_get_canvas_idx(ntw);
    if (!existing_aws[ntw_id]) {
        init_config_admin_awars(root);

        AW_window_simple *aws = new AW_window_simple;
        aws->init(root, GBS_global_string("SPECIES_SELECTIONS_%i", ntw_id), "Species Selections");
        aws->load_xfig("nt_selection.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("species_configs.hlp"));
        aws->create_button("HELP", "HELP", "H");

        aws->at("name");
        aws->create_input_field(AWAR_CONFIGURATION);

        aws->at("comment");
        aws->create_text_field(AWAR_CONFIG_COMMENT);

        aws->at("clr");
        aws->callback(clear_comment_cb);
        aws->create_autosize_button("CLEAR", "Clear", "l");

        aws->at("list");
        AW_DB_selection *dbsel = awt_create_CONFIG_selection_list(GLOBAL.gb_main, aws, AWAR_CONFIGURATION, false);

        aws->button_length(8);

        aws->at("store");
        aws->callback(makeWindowCallback(nt_store_configuration, ntw));
        aws->create_button(GBS_global_string("STORE_%i", ntw_id), "STORE", "S");

        aws->at("extract");
        aws->callback(makeWindowCallback(nt_extract_configuration, CONF_EXTRACT));
        aws->create_button("EXTRACT", "EXTRACT", "E");

        aws->at("mark");
        aws->callback(makeWindowCallback(nt_extract_configuration, CONF_MARK));
        aws->create_button("MARK", "MARK", "M");

        aws->at("unmark");
        aws->callback(makeWindowCallback(nt_extract_configuration, CONF_UNMARK));
        aws->create_button("UNMARK", "UNMARK", "U");

        aws->at("invert");
        aws->callback(makeWindowCallback(nt_extract_configuration, CONF_INVERT));
        aws->create_button("INVERT", "INVERT", "I");

        aws->at("combine");
        aws->callback(makeWindowCallback(nt_extract_configuration, CONF_COMBINE));
        aws->create_button("COMBINE", "COMBINE", "C");

        aws->at("delete");
        aws->callback(makeWindowCallback(nt_delete_configuration, dbsel));
        aws->create_button("DELETE", "DELETE", "D");

        aws->at("rename");
        aws->callback(nt_rename_configuration);
        aws->create_button("RENAME", "RENAME", "R");

        aws->at("highlight");
        aws->callback(makeCreateWindowCallback(create_configuration_marker_window, ntw));
        aws->create_autosize_button(GBS_global_string("HIGHLIGHT_%i", ntw_id), "Highlight in tree", "t");

        aws->button_length(0);
        aws->at("sort");
        awt_create_order_buttons(aws, reorder_configs_cb, dbsel);

        existing_aws[ntw_id] = aws;
    }
    return existing_aws[ntw_id];
}

void NT_popup_configuration_admin(AW_window *aw_main, AWT_canvas *ntw) {
    create_configuration_admin_window(aw_main->get_root(), ntw)->activate();
}

// -----------------------------------------
//      various ways to start the editor

static void nt_start_editor_on_configuration(AW_window *aww) {
    aww->hide();

    const char *cfgName   = aww->get_root()->awar(AWAR_CONFIGURATION)->read_char_pntr();
    char       *quotedCfg = GBK_singlequote(cfgName);

    AWT_system_cb(GBS_global_string("arb_edit4 -c %s &", quotedCfg));

    free(quotedCfg);
}

AW_window *NT_create_startEditorOnOldConfiguration_window(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        init_config_awars(awr);

        aws = new AW_window_simple;
        aws->init(awr, "SELECT_CONFIGURATION", "SELECT A CONFIGURATION");
        aws->at(10, 10);
        aws->auto_space(0, 0);
        awt_create_CONFIG_selection_list(GLOBAL.gb_main, aws, AWAR_CONFIGURATION, false);
        aws->at_newline();

        aws->callback(nt_start_editor_on_configuration);
        aws->create_button("START", "START");

        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->window_fit();
    }
    return aws;
}

void NT_start_editor_on_tree(AW_window *aww, int use_species_aside, AWT_canvas *ntw) {
    init_config_awars(aww->get_root());
    GB_ERROR error    = nt_create_configuration(NT_get_tree_root_of_canvas(ntw), DEFAULT_CONFIGURATION, use_species_aside, BY_CALLING_THE_EDITOR);
    if (!error) error = GBK_system("arb_edit4 -c " DEFAULT_CONFIGURATION " &");
    aw_message_if(error);
}

inline void nt_create_config_after_import(AWT_canvas *ntw) {
    init_config_awars(ntw->awr);

    const char *dated_suffix = ARB_dateTime_suffix();
    char       *configName   = GBS_global_string_copy("imported_%s", dated_suffix);

    // ensure unique config-name
    {
        int unique = 1;
        GB_transaction ta(ntw->gb_main);
        while (GBT_find_configuration(ntw->gb_main, configName)) {
            freeset(configName, GBS_global_string_copy("imported_%s_%i", dated_suffix, ++unique));
        }
    }

    GB_ERROR error = nt_create_configuration(NT_get_tree_root_of_canvas(ntw), configName, 0, FROM_IMPORTER);
    aw_message_if(error);

    free(configName);
}

void NT_create_config_after_import(AWT_canvas *ntw, bool imported_from_scratch) {
    /*! create a new config after import
     * @param imported_from_scratch if true -> DB was created from scratch, all species in DB are marked.
     *                              if false -> data was imported into existing DB. Other species may be marked as well, imported species are "queried".
     */

    if (imported_from_scratch) {
        nt_create_config_after_import(ntw);
    }
    else {
        GB_transaction ta(ntw->gb_main);

        // remember marks + mark queried species:
        for (GBDATA *gb_species = GBT_first_species(ntw->gb_main); gb_species; gb_species = GBT_next_species(gb_species)) {
            GB_write_user_flag(gb_species, GB_USERFLAG_WASMARKED, GB_read_flag(gb_species));
            GB_write_flag(gb_species, GB_user_flag(gb_species, GB_USERFLAG_QUERY));
        }

        nt_create_config_after_import(ntw);

        // restore old marks:
        for (GBDATA *gb_species = GBT_first_species(ntw->gb_main); gb_species; gb_species = GBT_next_species(gb_species)) {
            GB_write_flag(gb_species, GB_user_flag(gb_species, GB_USERFLAG_WASMARKED));
            GB_clear_user_flag(gb_species, GB_USERFLAG_WASMARKED);
        }
    }
}



