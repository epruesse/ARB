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

#include <awt_canvas.hxx>

#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_awar_defs.hxx>

#define nt_assert(cond) arb_assert(cond)

#if defined(DEVEL_RALF)
#define IMPLEMENT_TEST_SHADER // @@@ remove the test shader later
#endif

#if defined(IMPLEMENT_TEST_SHADER)
struct RelposShader: public ShaderPlugin {
    RelposShader() : ShaderPlugin("relpos", "Demo-'relpos'-Shader") {}

    ShadedValue shade(GBDATA *gb_item) const OVERRIDE {
        GB_transaction  ta(gb_item);
        GBDATA         *gb_relpos = GB_entry(gb_item, "relpos");
        return gb_relpos ? ValueTuple::make(GB_read_float(gb_relpos)) : ValueTuple::undefined();
    }
    int get_dimension() const OVERRIDE { return 1; }
    void init_specific_awars(AW_root *) OVERRIDE {}
    bool customizable() const OVERRIDE { return false; }
    void customize(AW_root */*awr*/) OVERRIDE { nt_assert(0); }
    void activate(bool /*on*/) OVERRIDE {}
};

#endif

class NT_TreeShader: public AP_TreeShader, virtual Noncopyable {
    ItemShader *shader; // (owned by registry in ITEM_SHADER)

    static void reshade() {
#if defined(DEBUG)
        fprintf(stderr, "[NT_TreeShader::reshade] @ %zu\n", clock());
#endif
        AW_root::SINGLETON->awar(AWAR_TREE_RECOMPUTE)->touch();
    }

public:
    NT_TreeShader(AWT_canvas *ntw, GBDATA *gb_main) :
        shader(registerItemShader(ntw->awr,
                                  ntw->gc_manager,
                                  BoundItemSel(gb_main, SPECIES_get_selector()),
                                  "tree",
                                  "tree shading",
                                  "tree_shading.hlp",
                                  NT_TreeShader::reshade,
                                  AWT_GC_NONE_MARKED))
    {
#if defined(IMPLEMENT_TEST_SHADER)
        ShaderPluginPtr relpos_shader = new RelposShader;
        shader->register_plugin(relpos_shader);
#endif
    }
    ~NT_TreeShader() OVERRIDE {}
    void init() OVERRIDE { shader->init(); } // called by AP_tree::set_tree_shader when installed

    void update_settings() OVERRIDE {
        colorize_marked = shader->overlay_marked();
        colorize_groups = shader->overlay_color_groups();
        shade_species   = shader->active();
    }

    ShadedValue calc_shaded_leaf_GC(GBDATA *gb_node) const OVERRIDE { return shader->shade(gb_node); }
    ShadedValue calc_shaded_inner_GC(const ShadedValue& left, float left_ratio, const ShadedValue& right) const OVERRIDE {
        return mix(left, left_ratio, right);
    }
    int to_GC(const ShadedValue& val) const OVERRIDE { return shader->to_GC(val); }

    void popup_config() const {
        shader->popup_config_window(AW_root::SINGLETON);
    }
};

void NT_install_treeShader(AWT_canvas *ntw, GBDATA *gb_main) {
    AP_tree::set_tree_shader(new NT_TreeShader(ntw, gb_main));
}

void NT_configure_treeShader() {
    const AP_TreeShader *tshader = AP_tree::get_tree_shader();
    nt_assert(tshader);
    if (tshader) static_cast<const NT_TreeShader*>(tshader)->popup_config();
}


