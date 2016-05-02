// ============================================================= //
//                                                               //
//   File      : TreeAdmin.h                                     //
//   Purpose   : Common tree admin functionality                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef TREEADMIN_H
#define TREEADMIN_H

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

class AW_awar;

namespace TreeAdmin {

    class Spec {
        GBDATA     *gb_main;
        const char *awar_selected_tree;
    public:
        Spec(GBDATA *gb_main_, const char *awar_selected_tree_)
            : gb_main(gb_main_),
              awar_selected_tree(awar_selected_tree_)
        {}

        GBDATA *get_gb_main() const { return gb_main; }
        AW_awar *tree_awar(AW_root *awr) const;
    };

    void create_awars(AW_root *root, AW_default aw_def, bool registerTreeAwar);
    AW_awar *dest_tree_awar(AW_root *root);
    AW_awar *source_tree_awar(AW_root *root);

    AW_window *create_rename_window(AW_root *root, const Spec *spec);
    AW_window *create_copy_window(AW_root *root, const Spec *spec);

    void delete_tree_cb(AW_window *aww, const Spec *spec);
};

#else
#error TreeAdmin.h included twice
#endif // TREEADMIN_H
