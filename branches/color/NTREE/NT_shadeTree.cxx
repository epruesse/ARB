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
#include <item_shader.h>
#include <AP_Tree.hxx>

using namespace std;

class NT_TreeShader: public AP_TreeShader, virtual Noncopyable {
    ItemShader *shader; // (owned by registry in ITEM_SHADER)

public:
    NT_TreeShader() :
        shader(registerItemShader("tree"))
    {}
    ~NT_TreeShader() OVERRIDE {
    }

    void update_settings() OVERRIDE {
#if defined(DEVEL_RALF)
        colorize_marked = false; // @@@ test-code: disables coloring marked species in tree
#else
        colorize_marked = true; // @@@ should ask ItemShader
#endif
        colorize_groups = AW_color_groups_active();
        shade_species   = false; // @@@ should ask ItemShader
    }
};

void NT_install_treeShader() {
    AP_tree::set_tree_shader(new NT_TreeShader);
}


