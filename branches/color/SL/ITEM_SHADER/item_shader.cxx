// ============================================================ //
//                                                              //
//   File      : item_shader.cxx                                //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "item_shader.h"
#include <arbdb.h>

#define is_assert(cond) arb_assert(cond)

// --------------------------------------------------------------------------------
// test my interface

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

struct DummyPlugin: public ShaderPlugin {
    DummyPlugin() : ShaderPlugin("dummy", "test crash dummy") {}
    ShadedValue shade(GBDATA */*gb_item*/) const OVERRIDE { return NULL; }
    static void reshade() {}
    int get_dimension() const OVERRIDE { return 0; }
    void init_specific_awars(AW_root *) OVERRIDE {}
    bool customizable() const OVERRIDE { return false; }
    void customize(AW_root */*awr*/) OVERRIDE { is_assert(0); }
    void activate(bool /*on*/) OVERRIDE {}
};


void TEST_shader_interface() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("nosuch.arb", "c");

    {
        const char    *SHADY   = "lady";
        AW_root       *NOROOT  = NULL;
        AW_gc_manager *NOGCMAN = NULL;

        BoundItemSel  sel(gb_main, SPECIES_get_selector());
        ItemShader   *shader = registerItemShader(NOROOT, NOGCMAN, sel, SHADY, "undescribed", "", DummyPlugin::reshade, 0);
        TEST_REJECT_NULL(shader);

        ItemShader *unknown = findItemShader("unknown");
        TEST_EXPECT_NULL(unknown);

        ItemShader *found = findItemShader(SHADY);
        TEST_EXPECT_EQUAL(found, shader);

        // check shader plugins
        shader->register_plugin(new DummyPlugin);
        TEST_EXPECT(shader->activate_plugin  ("dummy"));        TEST_EXPECT_EQUAL(shader->active_plugin_name(), "dummy");
        TEST_REJECT(shader->activate_plugin  ("unregistered")); TEST_EXPECT_EQUAL(shader->active_plugin_name(), "dummy");
        TEST_EXPECT(shader->activate_plugin  ("field"));        TEST_EXPECT_EQUAL(shader->active_plugin_name(), "field");
        TEST_EXPECT(shader->deactivate_plugin());               TEST_EXPECT_EQUAL(shader->active_plugin_name(), NO_PLUGIN_SELECTED);
        TEST_REJECT(shader->deactivate_plugin());

        // clean-up
        destroyAllItemShaders();
    }
    GB_close(gb_main);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
// implementation

#include "field_shader.h"

#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awar.hxx>
#include <aw_preset.hxx>
#include <aw_select.hxx>
#include <aw_msg.hxx>

#include <arb_strarray.h>

#include <vector>
#include <string>
#include <algorithm>


using namespace std;

typedef vector<ShaderPluginPtr> Plugins;

#define AWAR_SELECTED_PLUGIN shader_awar("plugin")
#define AWAR_SHOW_DIMENSION  tmp_shader_awar("dimension")

#define AWAR_GUI_RANGE           tmp_shader_awar("range")
#define AWAR_GUI_OVERLAY_GROUPS  tmp_shader_awar("groups")
#define AWAR_GUI_OVERLAY_MARKED  tmp_shader_awar("marked")
#define AWAR_GUI_PHASE_FREQ      tmp_shader_awar("phase")
#define AWAR_GUI_PHASE_ALTER     tmp_shader_awar("alternate")
#define AWAR_GUI_PHASE_PRESHIFT  tmp_shader_awar("preshift")
#define AWAR_GUI_PHASE_POSTSHIFT tmp_shader_awar("postshift")

#define AWAR_PLUGIN_RANGE           plugin_awar("range")
#define AWAR_PLUGIN_OVERLAY_GROUPS  plugin_awar("groups")
#define AWAR_PLUGIN_OVERLAY_MARKED  plugin_awar("marked")
#define AWAR_PLUGIN_PHASE_FREQ      plugin_awar("phase")
#define AWAR_PLUGIN_PHASE_ALTER     plugin_awar("alternate")
#define AWAR_PLUGIN_PHASE_PRESHIFT  plugin_awar("preshift")
#define AWAR_PLUGIN_PHASE_POSTSHIFT plugin_awar("postshift")

// ----------------------
//      ShaderPlugin

void ShaderPlugin::init_awars(AW_root *awr, const char *awar_prefix_) {
    is_assert(awar_prefix.empty()); // called twice?
    awar_prefix = string(awar_prefix_) + '/' + get_id();

    if (awr) {
        // common awars for all shader plugins:
        awr->awar_string(AWAR_PLUGIN_RANGE,           "");  // => will use first available color-range (for dimension reported by actual plugin)
        awr->awar_int   (AWAR_PLUGIN_OVERLAY_GROUPS,  0);
        awr->awar_int   (AWAR_PLUGIN_OVERLAY_MARKED,  0);
        awr->awar_float (AWAR_PLUGIN_PHASE_FREQ,      1.0);
        awr->awar_int   (AWAR_PLUGIN_PHASE_ALTER,     0);
        awr->awar_float (AWAR_PLUGIN_PHASE_PRESHIFT,  0.0);
        awr->awar_float (AWAR_PLUGIN_PHASE_POSTSHIFT, 0.0);

        init_specific_awars(awr);
    }
}

bool ShaderPlugin::overlay_marked() const {
    return AW_root::SINGLETON->awar(AWAR_PLUGIN_OVERLAY_MARKED)->read_int();
}
bool ShaderPlugin::overlay_color_groups() const {
    return AW_root::SINGLETON->awar(AWAR_PLUGIN_OVERLAY_GROUPS)->read_int();
}

// -------------------------
//      ItemShader_impl

#define SKIP_TMP_PREFIX 4

class ItemShader_impl : public ItemShader {
    Plugins plugins;
    string  help_id;
    string  awar_prefix;

    RefPtr<AW_gc_manager>         gcman;
    RefPtr<AW_window>             aw_cfg;            // config window
    RefPtr<AW_option_menu_struct> range_option_menu; // color-range selector in config window

    ShaderPluginPtr find_plugin(const string& plugin_id) const;

    void setup_new_dimension();

public:
    ItemShader_impl(AW_gc_manager *gcman_, const string& id_, const string& description_, const string& help_id_, ReshadeCallback rcb, int undef_gc) :
        ItemShader(id_, description_, rcb, undef_gc),
        help_id(help_id_),
        awar_prefix(GBS_global_string("tmp/shader/%s", get_id().c_str())),
        gcman(gcman_),
        aw_cfg(NULL),
        range_option_menu(NULL)
    {}

    void register_plugin(ShaderPluginPtr plugin) OVERRIDE;
    bool activate_plugin(const string& plugin_id) OVERRIDE;
    bool activate_plugin_impl(const string& plugin_id);
    std::string active_plugin_name() const OVERRIDE {
        if (active_plugin.isSet()) return active_plugin->get_id();
        return NO_PLUGIN_SELECTED;
    }

    const char *shader_awar(const char *name) const {
        return GBS_global_string("%s/%s", awar_prefix.c_str()+SKIP_TMP_PREFIX, name);
    }
    const char *tmp_shader_awar(const char *name) const {
        return GBS_global_string("%s/%s", awar_prefix.c_str(), name);
    }

    void init_awars(AW_root *awr);
    void init() OVERRIDE;

    void check_dimension_change() OVERRIDE;

    void popup_config_window(AW_root *awr) OVERRIDE;

    static void trigger_reshade_cb(AW_root*,ItemShader_impl *shader) { shader->trigger_reshade_callback(SIMPLE_RESHADE); }

    void configure_active_plugin(AW_root *awr) {
        if (active_plugin.isSet()) {
            if (active_plugin->customizable()) active_plugin->customize(awr);
            else aw_message(GBS_global_string("Nothing to configure for '%s'", active_plugin->get_description().c_str()));
        }
        else aw_message("Please select a shader");
    }
    static void configure_active_plugin_cb(AW_window *aww, ItemShader_impl *shader) { shader->configure_active_plugin(aww->get_root()); }

    void selected_plugin_changed_cb(AW_root *awr) {
        AW_awar *awar_plugin = awr->awar(AWAR_SELECTED_PLUGIN);
        if (!activate_plugin_impl(awar_plugin->read_char_pntr())) {
            awar_plugin->write_string(NO_PLUGIN_SELECTED);
        }
    }
    static void selected_plugin_changed_cb(AW_root *awr, ItemShader_impl *shader) { shader->selected_plugin_changed_cb(awr); }

    void selected_range_changed_cb(AW_root *awr) {
        AW_activateColorRange(gcman, awr->awar(AWAR_GUI_RANGE)->read_char_pntr());
        trigger_reshade_callback(SIMPLE_RESHADE);
    }
    static void selected_range_changed_cb(AW_root *awr, ItemShader_impl *shader) { shader->selected_range_changed_cb(awr); }

    void configure_colors_cb(AW_window *aww) { AW_popup_gc_color_range_window(aww, gcman); }
    static void configure_colors_cb(AW_window *aww, ItemShader_impl *shader) { shader->configure_colors_cb(aww); }

    void phase_changed_cb(AW_root *awr) {
        phaser = Phaser(awr->awar(AWAR_GUI_PHASE_FREQ)->read_float(),
                        awr->awar(AWAR_GUI_PHASE_ALTER)->read_int(),
                        awr->awar(AWAR_GUI_PHASE_PRESHIFT)->read_float(),
                        awr->awar(AWAR_GUI_PHASE_POSTSHIFT)->read_float());
        trigger_reshade_callback(SIMPLE_RESHADE);
    }
    static void phase_changed_cb(AW_root *awr, ItemShader_impl *shader) { shader->phase_changed_cb(awr); }

    void reset_phasing_cb(AW_root *awr) const {
        DelayReshade here(this);
        awr->awar(AWAR_GUI_PHASE_FREQ)->reset_to_default();
        awr->awar(AWAR_GUI_PHASE_ALTER)->reset_to_default();
        awr->awar(AWAR_GUI_PHASE_PRESHIFT)->reset_to_default();
        awr->awar(AWAR_GUI_PHASE_POSTSHIFT)->reset_to_default();
    }
    static void reset_phasing_cb(AW_window *aww, const ItemShader_impl *shader) { shader->reset_phasing_cb(aww->get_root()); }
};

struct has_id {
    string id;
    explicit has_id(const char *id_) : id(id_) {}
    explicit has_id(const string& id_) : id(id_) {}

    bool operator()(const ItemShader_impl& shader) { return shader.get_id() == id; }
    bool operator()(const ShaderPluginPtr& plugin) { return plugin->get_id() == id; }
};


// --------------------------------
//      shader plugin registry

ShaderPluginPtr ItemShader_impl::find_plugin(const string& plugin_id) const {
    Plugins::const_iterator found = find_if(plugins.begin(), plugins.end(), has_id(plugin_id));
    return found == plugins.end() ? ShaderPluginPtr() : *found;
}

void ItemShader_impl::register_plugin(ShaderPluginPtr plugin) {
    is_assert(find_plugin(plugin->get_id()).isNull()); // attempt to register two plugins with same name!
    plugins.push_back(plugin);

    plugin->announce_shader(this);
    plugin->init_awars(AW_root::SINGLETON, awar_prefix.c_str()+SKIP_TMP_PREFIX);
}

bool ItemShader_impl::activate_plugin_impl(const string& plugin_id) {
    bool            changed    = false;
    ShaderPluginPtr prevActive = active_plugin;
    if (plugin_id == NO_PLUGIN_SELECTED) {
        if (active_plugin.isSet()) {
            active_plugin.SetNull();
            changed = true;
        }
    }
    else {
        ShaderPluginPtr found = find_plugin(plugin_id);
        if (found.isSet()) {
            active_plugin = found;
            changed       = true;
        }
    }
    if (changed) {
        if (prevActive.isSet()) prevActive->activate(false);
        if (active_plugin.isSet()) active_plugin->activate(true);

        DelayReshade here(this);

        AW_root *awr = AW_root::SINGLETON;
        if (awr) {
            if (active_plugin.isSet()) {
                // map common GUI awars to awars of active plugin:
                awr->awar(AWAR_GUI_RANGE)          ->map(active_plugin->AWAR_PLUGIN_RANGE);
                awr->awar(AWAR_GUI_OVERLAY_MARKED) ->map(active_plugin->AWAR_PLUGIN_OVERLAY_MARKED);
                awr->awar(AWAR_GUI_OVERLAY_GROUPS) ->map(active_plugin->AWAR_PLUGIN_OVERLAY_GROUPS);
                awr->awar(AWAR_GUI_PHASE_FREQ)     ->map(active_plugin->AWAR_PLUGIN_PHASE_FREQ);
                awr->awar(AWAR_GUI_PHASE_ALTER)    ->map(active_plugin->AWAR_PLUGIN_PHASE_ALTER);
                awr->awar(AWAR_GUI_PHASE_PRESHIFT) ->map(active_plugin->AWAR_PLUGIN_PHASE_PRESHIFT);
                awr->awar(AWAR_GUI_PHASE_POSTSHIFT)->map(active_plugin->AWAR_PLUGIN_PHASE_POSTSHIFT);
                check_dimension_change();
            }
            else {
                // unmap GUI awars:
                awr->awar(AWAR_GUI_RANGE)          ->unmap();
                awr->awar(AWAR_GUI_OVERLAY_MARKED) ->unmap();
                awr->awar(AWAR_GUI_OVERLAY_GROUPS) ->unmap();
                awr->awar(AWAR_GUI_PHASE_FREQ)     ->unmap();
                awr->awar(AWAR_GUI_PHASE_ALTER)    ->unmap();
                awr->awar(AWAR_GUI_PHASE_PRESHIFT) ->unmap();
                awr->awar(AWAR_GUI_PHASE_POSTSHIFT)->unmap();
                setup_new_dimension();
            }
        }
        trigger_reshade_callback(SIMPLE_RESHADE);
    }
    return changed;
}
bool ItemShader_impl::activate_plugin(const string& plugin_id) {
    AW_root *awr = AW_root::SINGLETON;
    if (!awr) {
        return activate_plugin_impl(plugin_id);
    }

    AW_awar *awar_plugin = awr->awar(AWAR_SELECTED_PLUGIN);
    awar_plugin->write_string(plugin_id.c_str());

    return (strcmp(awar_plugin->read_char_pntr(), plugin_id.c_str()) == 0);
}
void ItemShader_impl::setup_new_dimension() {
    AW_root *awr = AW_root::SINGLETON;
    int      dim = active_plugin.isSet() ? active_plugin->get_dimension() : 0;

    AW_awar *awar_range = awr->awar(AWAR_GUI_RANGE);

    if (dim>0) {
        // auto-select another color-range (if selected does not match dimension)

        ConstStrArray ids, names;
        AW_getColorRangeNames(gcman, dim, ids, names);
        aw_assert(!ids.empty()); // no range defined with dimension 'dim' // @@@ check during ItemShader-setup!

        const char *selected_range_id  = awar_range->read_char_pntr();
        bool        seen_selected      = false;

        if (aw_cfg) aw_cfg->clear_option_menu(range_option_menu);
        for (unsigned i = 0; i<ids.size(); ++i) {
            if (aw_cfg) aw_cfg->insert_option(names[i], "", ids[i]);
            if (!seen_selected && strcmp(selected_range_id, ids[i]) == 0) { // check if awar-value exists
                seen_selected = true;
            }
        }
        if (aw_cfg) aw_cfg->update_option_menu();

        if (!seen_selected) selected_range_id = ids[0]; // autoselect first color-range

        awar_range->write_string(selected_range_id);
        AW_activateColorRange(gcman, selected_range_id);
    }
    else {
        if (aw_cfg) {
            aw_cfg->clear_option_menu(range_option_menu);
            aw_cfg->insert_option("<none>", "", "");
            aw_cfg->update_option_menu();
        }
        awar_range->write_string("");
    }

    awr->awar(AWAR_SHOW_DIMENSION)->write_int(dim);
}
void ItemShader_impl::check_dimension_change() {
    aw_assert(active_plugin.isSet());

    int wanted_dim = active_plugin->get_dimension();
    if (wanted_dim>0) {
        int         range_dim;
        /*const char *range_id = */ AW_getActiveColorRangeID(gcman, &range_dim);

        int awar_dim = AW_root::SINGLETON->awar(AWAR_SHOW_DIMENSION)->read_int();

        if (wanted_dim != range_dim || wanted_dim != awar_dim) { // active color-range has wrong dimension (or config window needs update)
            setup_new_dimension();
        }
    }
}

// ------------------------
//      user-interface

static void global_colorgroup_use_changed_cb(AW_root *awr, ItemShader_impl *shader) {
    awr->awar(shader->AWAR_GUI_OVERLAY_GROUPS)->write_int(awr->awar(AW_get_color_groups_active_awarname())->read_int());
}

void ItemShader_impl::init() {
    // initialize ItemShader
    // - activate plugin stored in AWAR

    is_assert(!plugins.empty()); // you have to register all plugins before calling init

    first_range_gc = AW_getFirstRangeGC(gcman);
    selected_plugin_changed_cb(AW_root::SINGLETON, this);
}

void ItemShader_impl::init_awars(AW_root *awr) {
    RootCallback Reshade_cb       = makeRootCallback(ItemShader_impl::trigger_reshade_cb, this);
    RootCallback PhaseChanged_cb  = makeRootCallback(ItemShader_impl::phase_changed_cb, this);
    RootCallback PluginChanged_cb = makeRootCallback(ItemShader_impl::selected_plugin_changed_cb, this);
    RootCallback RangeChanged_cb  = makeRootCallback(ItemShader_impl::selected_range_changed_cb, this);

    awr->awar_string(AWAR_SELECTED_PLUGIN,     NO_PLUGIN_SELECTED)->add_callback(PluginChanged_cb);
    awr->awar_int   (AWAR_SHOW_DIMENSION,      -1);
    awr->awar_string(AWAR_GUI_RANGE,           "") ->add_callback(RangeChanged_cb);
    awr->awar_int   (AWAR_GUI_OVERLAY_GROUPS,  0)  ->add_callback(Reshade_cb);
    awr->awar_int   (AWAR_GUI_OVERLAY_MARKED,  0)  ->add_callback(Reshade_cb);
    awr->awar_float (AWAR_GUI_PHASE_FREQ,      1.0)->set_minmax(0.01, 100.0)->add_callback(PhaseChanged_cb);
    awr->awar_float (AWAR_GUI_PHASE_PRESHIFT,  0.0)->set_minmax(0.0,    1.0)->add_callback(PhaseChanged_cb);
    awr->awar_float (AWAR_GUI_PHASE_POSTSHIFT, 0.0)->set_minmax(0.0,    1.0)->add_callback(PhaseChanged_cb);
    awr->awar_int   (AWAR_GUI_PHASE_ALTER,     0)  ->add_callback(PhaseChanged_cb);

    const char *awarname_global_colorgroups = AW_get_color_groups_active_awarname();
    awr->awar(awarname_global_colorgroups)->add_callback(makeRootCallback(global_colorgroup_use_changed_cb, this));
}

void ItemShader_impl::popup_config_window(AW_root *awr) {
    if (!aw_cfg) {
        AW_window_simple *aws = new AW_window_simple;
        {
            string wid = GBS_global_string("shader_cfg_%s", get_id().c_str());
            aws->init(awr, wid.c_str(), get_description().c_str());
        }
        aws->load_xfig("item_shader_cfg.fig");

        aws->button_length(8);

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "O");

        aws->at("help");
        aws->callback(makeHelpCallback(help_id.c_str()));
        aws->create_button("HELP", "HELP");

        aws->at("plugin");
        AW_selection_list *sel = aws->create_selection_list(AWAR_SELECTED_PLUGIN, true);
        sel->insert_default("<No shading>", NO_PLUGIN_SELECTED);
        for (Plugins::iterator p = plugins.begin(); p != plugins.end(); ++p) {
            const ShaderPlugin& plugged = **p;
            sel->insert(plugged.get_description().c_str(), plugged.get_id().c_str());
        }
        sel->update();

        aws->at("plugin_cfg");
        aws->callback(makeWindowCallback(ItemShader_impl::configure_active_plugin_cb, this));
        aws->create_autosize_button("configure", "Configure");

        aws->at("range");
        range_option_menu = aws->create_option_menu(AWAR_GUI_RANGE, false);
        aws->insert_option("<none>", "", "");
        aws->update_option_menu();

        aws->at("color_cfg");
        aws->callback(makeWindowCallback(ItemShader_impl::configure_colors_cb, this));
        aws->create_autosize_button("color_cfg", "Color setup");

        aws->at("groups");
        aws->create_toggle(AWAR_GUI_OVERLAY_GROUPS);

        aws->at("marked");
        aws->create_toggle(AWAR_GUI_OVERLAY_MARKED);

        aws->auto_space(5,5);
        const int FIELDSIZE    = 12;
        const int SCALERLENGTH = 400;

        aws->at("alt");
        aws->create_toggle(AWAR_GUI_PHASE_ALTER);

        aws->at("dim");
        aws->create_button(0, AWAR_SHOW_DIMENSION, 0, "+");

        aws->at("reset");
        aws->callback(makeWindowCallback(ItemShader_impl::reset_phasing_cb, this));
        aws->create_button("reset", "RESET", "R");

        aws->at("freq");      aws->create_input_field_with_scaler(AWAR_GUI_PHASE_FREQ,      FIELDSIZE, SCALERLENGTH, AW_SCALER_EXP_LOWER);
        aws->at("preshift");  aws->create_input_field_with_scaler(AWAR_GUI_PHASE_PRESHIFT,  FIELDSIZE, SCALERLENGTH, AW_SCALER_LINEAR);
        aws->at("postshift"); aws->create_input_field_with_scaler(AWAR_GUI_PHASE_POSTSHIFT, FIELDSIZE, SCALERLENGTH, AW_SCALER_LINEAR);

        aws->window_fit();

        aw_cfg = aws;

        setup_new_dimension();
    }
    aw_cfg->activate();
}

// -------------------------
//      shader registry

typedef vector<ItemShader_impl> Shaders;

static Shaders registered;

ItemShader *registerItemShader(AW_root *awr, AW_gc_manager *gcman, BoundItemSel& itemtype, const char *unique_id, const char *description, const char *help_id, ReshadeCallback reshade_cb, int undef_gc) {
    /*! create a new ItemShader
     *
     * @param awr             the application root
     * @param gcman           gc-manager used for shading
     * @param itemtype        type of item
     * @param unique_id       unique ID
     * @param description     short description used ia. as title of config window (eg. "Tree shading")
     * @param help_id         helpfile
     * @param reshade_cb      callback which updates the shading + refreshes the display
     * @param undef_gc        GC reported for undefined ShadedValue
     *
     * (Note: the reshade_cb may be called very often! best is to trigger a temp. DB-awar like AWAR_TREE_RECOMPUTE)
     */
    if (findItemShader(unique_id)) {
        is_assert(0); // duplicate shader id
        return NULL;
    }

    registered.push_back(ItemShader_impl(gcman, unique_id, description, help_id, reshade_cb, undef_gc));
    ItemShader_impl& new_shader = registered.back();
    if (awr) new_shader.init_awars(awr);
    new_shader.register_plugin(makeItemFieldShader(itemtype));
    return &new_shader;
}

ItemShader *findItemShader(const char *id) { // @@@ return const shader?
    Shaders::iterator found = find_if(registered.begin(), registered.end(), has_id(id));
    return found == registered.end() ? NULL : &*found;
}

void destroyAllItemShaders() {
    registered.clear();
}

