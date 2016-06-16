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
};


void TEST_shader_interface() {
    const char *SHADY  = "lady";
    ItemShader *shader = registerItemShader(SHADY);
    TEST_REJECT_NULL(shader);

    ItemShader *unknown = findItemShader("unknown");
    TEST_EXPECT_NULL(unknown);

    ItemShader *found = findItemShader(SHADY);
    TEST_EXPECT_EQUAL(found, shader);

    // check shader plugins
    shader->register_plugin(new DummyPlugin);
    TEST_EXPECT(shader->activate_plugin("dummy"));
    TEST_REJECT(shader->activate_plugin("unregistered"));

    // clean-up
    destroyAllItemShaders();
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
// implementation

#include <vector>
#include <string>
#include <algorithm>

using namespace std;

typedef vector<ShaderPluginPtr> Plugins;

// -------------------------
//      ItemShader_impl

class ItemShader_impl : public ItemShader {
    string id;

    Plugins         plugins;
    ShaderPluginPtr active_plugin; // @@@ move into abstract class ItemShader

    ShaderPluginPtr find_plugin(const std::string& plugin_id) const;

public:
    ItemShader_impl(const string& id_) : id(id_) {}

    const string& get_id() const { return id; }

    void register_plugin(ShaderPluginPtr plugin) OVERRIDE;
    bool activate_plugin(const std::string& plugin_id) OVERRIDE;

    bool shades_marked() const OVERRIDE { return active() && active_plugin->shades_marked(); }
    bool active() const OVERRIDE { return active_plugin.isSet(); }

    ShadedValue shade(GBDATA *gb_item) const OVERRIDE {
        is_assert(active()); // don't call if no shader active
        return active() ? active_plugin->shade(gb_item) : ValueTuple::undefined();
    }
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

ShaderPluginPtr ItemShader_impl::find_plugin(const std::string& plugin_id) const {
    Plugins::const_iterator found = find_if(plugins.begin(), plugins.end(), has_id(plugin_id));
    return found == plugins.end() ? ShaderPluginPtr() : *found;
}

void ItemShader_impl::register_plugin(ShaderPluginPtr plugin) {
    is_assert(find_plugin(plugin->get_id()).isNull()); // attempt to register two plugins with same name!
    plugins.push_back(plugin);
}

bool ItemShader_impl::activate_plugin(const std::string& plugin_id) {
    ShaderPluginPtr found = find_plugin(plugin_id);
    if (found.isSet()) {
        active_plugin = found;
        return true;
    }
    return false;
}

// -------------------------
//      shader registry

typedef vector<ItemShader_impl> Shaders;

static Shaders registered;

ItemShader *registerItemShader(const char *unique_id) { // @@@ return const shader?
    if (findItemShader(unique_id)) {
        is_assert(0); // duplicate shader id
        return NULL;
    }

    registered.push_back(ItemShader_impl(unique_id));
    return &registered.back();
}

ItemShader *findItemShader(const char *id) { // @@@ return const shader?
    Shaders::iterator found = find_if(registered.begin(), registered.end(), has_id(id));
    return found == registered.end() ? NULL : &*found;
}

void destroyAllItemShaders() {
    registered.clear();
}


