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

#define nt_assert(cond) arb_assert(cond)

class NT_TreeShader: public AP_TreeShader, virtual Noncopyable {
    ItemShader *shader; // (owned by registry in ITEM_SHADER)

public:
    NT_TreeShader() :
        shader(registerItemShader("tree"))
    {}
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
};

void NT_install_treeShader() {
    AP_tree::set_tree_shader(new NT_TreeShader);
}


