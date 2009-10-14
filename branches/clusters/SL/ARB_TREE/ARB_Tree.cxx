// =============================================================== //
//                                                                 //
//   File      : ARB_Tree.cxx                                      //
//   Purpose   : Minimal interface between GBT_TREE and C++        //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ARB_Tree.hxx"

using namespace std;

ARB_Tree::ARB_Tree() {
    CLEAR_GBT_TREE_ELEMENTS(this);
}

ARB_Tree::~ARB_Tree() {
    at_assert(tree_is_one_piece_of_memory == GB_FALSE);
    unlink_from_father();
    delete leftson;                                 // implicitely sets leftson = NULL
    at_assert(!leftson);
    delete rightson;                                // implicitely sets rightson = NULL
    at_assert(!rightson);
}

void ARB_Tree::move_gbt_info(GBT_TREE *tree) {
    is_leaf  = tree->is_leaf;
    leftlen  = tree->leftlen;
    rightlen = tree->rightlen;
    gb_node  = tree->gb_node;

    reassign(name, tree->name);
    reassign(remark_branch, tree->remark_branch);

    if (!is_leaf) {                                 // inner node
        leftson         = dup();                    // creates two clones of 'this'
        leftson->father = this;
        
        rightson         = dup();
        rightson->father = this;

        leftson->move_gbt_info(tree->leftson);
        rightson->move_gbt_info(tree->rightson);
    }
}

GB_ERROR ARB_Tree::load(GBDATA *gb_main, const char *tree_name, bool link_to_database, bool show_status) {
    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBT_TREE *gbt_tree   = GBT_read_tree(gb_main, tree_name, -sizeof(GBT_TREE));
        if (!gbt_tree) error = GB_await_error();
        else {
            if (link_to_database) {
                int zombies    = 0;
                int duplicates = 0;
                
                error = GBT_link_tree(gbt_tree, gb_main,
                                      show_status ? GB_TRUE : GB_FALSE,
                                      &zombies, &duplicates);

                if (!error) {
                    if (zombies || duplicates) {
                        error = GBS_global_string("Tree contains zombies (%i) or duplicates (%i)",
                                                  zombies, duplicates);
                    }
                }
            }
            if (!error) move_gbt_info(gbt_tree);
            GBT_delete_tree(gbt_tree);
        }
    }
    error = GB_end_transaction(gb_main, error);
    return error;
}

GB_ERROR ARB_Tree::save(GBDATA *gb_main, const char *tree_name) {

}


