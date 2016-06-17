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
    bool shades_marked() const OVERRIDE { return false; }
    static void reshade() {}
};


void TEST_shader_interface() {
    const char *SHADY  = "lady";
    AW_root    *NOROOT = NULL;
    ItemShader *shader = registerItemShader(NOROOT, SHADY, "undescribed", "", DummyPlugin::reshade);
    TEST_REJECT_NULL(shader);

    ItemShader *unknown = findItemShader("unknown");
    TEST_EXPECT_NULL(unknown);

    ItemShader *found = findItemShader(SHADY);
    TEST_EXPECT_EQUAL(found, shader);

    // check shader plugins
    shader->register_plugin(new DummyPlugin);
    TEST_EXPECT(shader->activate_plugin  ("dummy"));        TEST_EXPECT_EQUAL(shader->active_plugin_name(), "dummy");
    TEST_REJECT(shader->activate_plugin  ("unregistered")); TEST_EXPECT_EQUAL(shader->active_plugin_name(), "dummy");
    TEST_EXPECT(shader->deactivate_plugin());               TEST_EXPECT_EQUAL(shader->active_plugin_name(), NO_PLUGIN_SELECTED);
    TEST_REJECT(shader->deactivate_plugin());

    // clean-up
    destroyAllItemShaders();
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
// implementation

#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awar.hxx>
#include <aw_select.hxx>

#include <vector>
#include <string>
#include <algorithm>

using namespace std;

typedef vector<ShaderPluginPtr> Plugins;

#define AWAR_SELECTED_PLUGIN shader_awar("plugin")

// -------------------------
//      ItemShader_impl

class ItemShader_impl : public ItemShader {
    Plugins    plugins;
    string     help_id;
    AW_window *aw_cfg; // config window

    ShaderPluginPtr find_plugin(const string& plugin_id) const;

public:
    explicit ItemShader_impl(const string& id_, const string& description_, const string& help_id_, ReshadeCallback rcb) :
        ItemShader(id_, description_, rcb),
        help_id(help_id_),
        aw_cfg(NULL)
    {}

    void register_plugin(ShaderPluginPtr plugin) OVERRIDE;
    bool activate_plugin(const string& plugin_id) OVERRIDE;
    bool activate_plugin_impl(const string& plugin_id);
    std::string active_plugin_name() const OVERRIDE {
        if (active_plugin.isSet()) return active_plugin->get_id();
        return NO_PLUGIN_SELECTED;
    }

    const char *shader_awar(const char *name) const {
        return GBS_global_string("shader/%s/%s", get_id().c_str(), name);
    }

    void init_awars(AW_root *awr);
    void init() OVERRIDE;

    void popup_config_window(AW_root *awr) OVERRIDE;
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
}

bool ItemShader_impl::activate_plugin_impl(const string& plugin_id) {
    bool changed = false;
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
    if (changed) reshade_cb();
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


// ------------------------
//      user-interface

static void selected_plugin_changed_cb(AW_root *awr, ItemShader_impl *shader) {
    AW_awar *awar_plugin = awr->awar(shader->AWAR_SELECTED_PLUGIN);
    if (!shader->activate_plugin_impl(awar_plugin->read_char_pntr())) {
        awar_plugin->write_string(NO_PLUGIN_SELECTED);
    }
}
void ItemShader_impl::init() {
    // initialize ItemShader
    // - activate plugin stored in AWAR

    is_assert(!plugins.empty()); // you have to register all plugins before calling init
    selected_plugin_changed_cb(AW_root::SINGLETON, this);
}

void ItemShader_impl::init_awars(AW_root *awr) {
    awr->awar_string(AWAR_SELECTED_PLUGIN, NO_PLUGIN_SELECTED)->add_callback(makeRootCallback(selected_plugin_changed_cb, this));
}

void ItemShader_impl::popup_config_window(AW_root *awr) {
    if (!aw_cfg) {
        AW_window_simple *aws = new AW_window_simple;
        {
            string wid   = GBS_global_string("shader_cfg_%s", get_id().c_str());
            string title = GBS_global_string("Configure %s", get_description().c_str());
            aws->init(awr, wid.c_str(), title.c_str());
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

        aw_cfg = aws;
    }
    aw_cfg->activate();
}

// -------------------------
//      shader registry

typedef vector<ItemShader_impl> Shaders;

static Shaders registered;

ItemShader *registerItemShader(AW_root *awr, const char *unique_id, const char *description, const char *help_id, ReshadeCallback reshade_cb) {
    if (findItemShader(unique_id)) {
        is_assert(0); // duplicate shader id
        return NULL;
    }

    registered.push_back(ItemShader_impl(unique_id, description, help_id, reshade_cb));
    ItemShader_impl& new_shader = registered.back();
    if (awr) new_shader.init_awars(awr);
    return &new_shader;
}

ItemShader *findItemShader(const char *id) { // @@@ return const shader?
    Shaders::iterator found = find_if(registered.begin(), registered.end(), has_id(id));
    return found == registered.end() ? NULL : &*found;
}

void destroyAllItemShaders() {
    registered.clear();
}


