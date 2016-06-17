// ============================================================ //
//                                                              //
//   File      : NT_shadeTree.cxx                               //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include <AP_TreeShader.hxx>
#include <AP_Tree.hxx>

#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_awar_defs.hxx>

#define nt_assert(cond) arb_assert(cond)

#if defined(DEVEL_RALF)
#define IMPLEMENT_TEST_SHADER
#endif

#if defined(IMPLEMENT_TEST_SHADER)
struct RelposShader: public ShaderPlugin {
    RelposShader() : ShaderPlugin("relpos", "Demo-'relpos'-Shader") {}

    ShadedValue shade(GBDATA *gb_item) const OVERRIDE {
        GB_transaction  ta(gb_item);
        GBDATA         *gb_relpos = GB_entry(gb_item, "relpos");
        return gb_relpos ? ValueTuple::make(GB_read_float(gb_relpos)) : ValueTuple::undefined();
    }
    bool shades_marked() const OVERRIDE {
        // true if shader-plugin likes to shade marked species
        return true; // @@@ fake
    }

};

#endif

class NT_TreeShader: public AP_TreeShader, virtual Noncopyable {
    ItemShader *shader; // (owned by registry in ITEM_SHADER)

    static void reshade() {
        AW_root::SINGLETON->awar(AWAR_TREE_RECOMPUTE)->touch();
    }

public:
    NT_TreeShader(AW_root *awr) :
        shader(registerItemShader(awr, "tree", "tree shading", "tree_shading.hlp", NT_TreeShader::reshade))
    {
#if defined(IMPLEMENT_TEST_SHADER)
        ShaderPluginPtr relpos_shader = new RelposShader;
        shader->register_plugin(relpos_shader);
        shader->init();
#endif
    }
    ~NT_TreeShader() OVERRIDE {}

    void update_settings() OVERRIDE {
        colorize_marked = !shader->shades_marked();
        colorize_groups = AW_color_groups_active();
        shade_species   = shader->active();
    }

    ShadedValue calc_shaded_leaf_GC(GBDATA *gb_node) const OVERRIDE { return shader->shade(gb_node); }
    ShadedValue calc_shaded_inner_GC(const ShadedValue& left, float left_ratio, const ShadedValue& right) const OVERRIDE {
        return mix(left, left_ratio, right);
    }
    int to_GC(const ShadedValue& val) const OVERRIDE {
        nt_assert(val->is_defined()); // @@@ who should handle undefined values? (test tree with two zombie-brothers)
        return AWT_GC_FIRST_RANGE_COLOR + val->range_offset(); // @@@ delegate to ItemShader (shader needs to know first range-gc and an "undefined"-gc)
    }

    void popup_config() const {
        shader->popup_config_window(AW_root::SINGLETON);
    }
};

void NT_install_treeShader(AW_root *awr) {
    AP_tree::set_tree_shader(new NT_TreeShader(awr));
}

void NT_configure_treeShader() {
    const AP_TreeShader *tshader = AP_tree::get_tree_shader();
    nt_assert(tshader);
    if (tshader) static_cast<const NT_TreeShader*>(tshader)->popup_config();
}


