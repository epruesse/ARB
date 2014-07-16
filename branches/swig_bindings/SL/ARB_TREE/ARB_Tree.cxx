// =============================================================== //
//                                                                 //
//   File      : ARB_Tree.cxx                                      //
//   Purpose   : Tree types with sequence knowledge                //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ARB_Tree.hxx"

#include <AP_filter.hxx>
#include <AP_sequence.hxx>

#include <ad_cb.h>

using namespace std;

// --------------------------
//      ARB_seqtree_root

static void tree_deleted_cbwrapper(GBDATA *gb_tree, ARB_seqtree_root *troot) {
    troot->tree_deleted_cb(gb_tree);
}


ARB_seqtree_root::ARB_seqtree_root(AliView *aliView, RootedTreeNodeFactory *nodeMaker_, AP_sequence *seqTempl, bool add_delete_callbacks)
    : TreeRoot(nodeMaker_, false),
      ali(aliView),
      seqTemplate(seqTempl ? seqTempl->dup() : NULL),
      tree_name(NULL),
      gb_tree(NULL),
      isLinkedToDB(false),
      addDeleteCallbacks(add_delete_callbacks)
{
#if defined(DEBUG)
    at_assert(ali);
    if (seqTemplate) {
        at_assert(ali->has_data());
        at_assert(seqTemplate->get_aliview() == ali);
    }
    else {
        at_assert(!ali->has_data());
    }
#endif // DEBUG
}

ARB_seqtree_root::~ARB_seqtree_root() {
    delete ali;
    delete seqTemplate;

    if (gb_tree) GB_remove_callback(gb_tree, GB_CB_DELETE, makeDatabaseCallback(tree_deleted_cbwrapper, this));
    free(tree_name);
}

void ARB_seqtree_root::tree_deleted_cb(GBDATA *gb_tree_del) {
    if (gb_tree == gb_tree_del) {                   // ok - it's my tree
        gb_tree = NULL;
        freenull(tree_name);
    }
    else {
        at_assert(0); // callback for wrong tree received
    }
}
GB_ERROR ARB_seqtree_root::loadFromDB(const char *name) {
    GBDATA   *gb_main = get_gb_main();
    GB_ERROR  error   = GB_push_transaction(gb_main);

    if (!error) {
        ARB_seqtree *old_root = get_root_node();
        if (old_root) {
            change_root(old_root, NULL);
            delete old_root;
        }

        if (gb_tree) {
            GB_remove_callback(gb_tree, GB_CB_DELETE, makeDatabaseCallback(tree_deleted_cbwrapper, this));
            gb_tree = NULL;
            freenull(tree_name);
        }

        ARB_seqtree *arb_tree   = DOWNCAST(ARB_seqtree*, GBT_read_tree(gb_main, name, *this));
        if (!arb_tree) error = GB_await_error();
        else {
            gb_tree             = GBT_find_tree(gb_main, name);
            if (!gb_tree) error = GB_await_error();
            else {
                error = GB_add_callback(gb_tree, GB_CB_DELETE, makeDatabaseCallback(tree_deleted_cbwrapper, this));
                if (!error) {
                    at_assert(arb_tree == arb_tree->get_root_node());
                    at_assert(arb_tree == get_root_node());
                    tree_name    = strdup(name);
                    isLinkedToDB = false;
                }
                else {
                    gb_tree = NULL;
                }
            }
            if (error) delete arb_tree;
        }
    }

    return GB_end_transaction(gb_main, error);
}

GB_ERROR ARB_seqtree_root::saveToDB() {
    GB_ERROR error;
    if (!gb_tree) {
        error = "Can't save your tree (no tree loaded or tree has been deleted)";
    }
    else {
        GBDATA *gb_main   = get_gb_main();
        error             = GB_push_transaction(gb_main);
        at_assert(get_root_node());
        if (!error) error = GBT_overwrite_tree(gb_tree, get_root_node());
        error             = GB_end_transaction(gb_main, error);
    }
    return error;
}

static void arb_tree_species_deleted_cb(GBDATA *gb_species, ARB_seqtree *arb_tree) {
    // called whenever a species (which is linked to tree) gets deleted
    at_assert(arb_tree->gb_node == gb_species);
    if (arb_tree->gb_node == gb_species) {
        arb_tree->gb_node = NULL; // unlink from tree
    }
}

GB_ERROR ARB_seqtree_root::linkToDB(int *zombies, int *duplicates) {
    at_assert(!ali->has_data() || get_seqTemplate()); // if ali has data, you have to set_seqTemplate() before linking

    GB_ERROR error = 0;
    if (!isLinkedToDB) {
        error = GBT_link_tree(get_root_node(), get_gb_main(), false, zombies, duplicates);
        if (!error && addDeleteCallbacks) {
            error = get_root_node()->add_delete_cb_rec(arb_tree_species_deleted_cb);
        }
        if (!error && ali->has_data() && seqTemplate) {
            error = get_root_node()->preloadLeafSequences();
        }
        if (!error) isLinkedToDB = true;
    }
    return error;
}

void ARB_seqtree_root::unlinkFromDB() {
    if (isLinkedToDB) {
        if (addDeleteCallbacks) get_root_node()->remove_delete_cb_rec(arb_tree_species_deleted_cb);
        GBT_unlink_tree(get_root_node());
        if (ali->has_data() && seqTemplate) get_root_node()->unloadSequences();
        isLinkedToDB = false;
    }
}

// ----------------------
//      ARB_tree_info

ARB_tree_info::ARB_tree_info() {
    memset(this, 0, sizeof(*this));
}

void ARB_seqtree::calcTreeInfo(ARB_tree_info& info) {
    if (is_leaf) {
        info.leafs++;
        if (gb_node) {
            if (GB_read_flag(gb_node)) info.marked++;
        }
        else {
            info.unlinked++;
        }
    }
    else {
        info.innerNodes++;
        if (gb_node) info.groups++;
        get_leftson()->calcTreeInfo(info);
        get_rightson()->calcTreeInfo(info);
    }
}

// ---------------------
//      ARB_seqtree

ARB_seqtree::~ARB_seqtree() {
    delete seq;
}

void ARB_seqtree::mark_subtree() {
    if (is_leaf) {
        if (gb_node) GB_write_flag(gb_node, 1);
    }
    else {
        get_leftson()->mark_subtree();
        get_rightson()->mark_subtree();
    }
}

bool ARB_seqtree::contains_marked_species() {
    if (is_leaf) {
        return gb_node && GB_read_flag(gb_node) != 0;
    }
    return
        get_leftson()->contains_marked_species() ||
        get_rightson()->contains_marked_species();
}

GB_ERROR ARB_seqtree::add_delete_cb_rec(ARB_tree_node_del_cb cb) {
    GB_ERROR error = NULL;
    if (is_leaf) {
        if (gb_node) {
            error = GB_add_callback(gb_node, GB_CB_DELETE, makeDatabaseCallback(cb, this));
        }
    }
    else {
        error            = get_leftson() ->add_delete_cb_rec(cb);
        if (error) error = get_rightson()->add_delete_cb_rec(cb);
    }
    return error;
}

void ARB_seqtree::remove_delete_cb_rec(ARB_tree_node_del_cb cb) {
    if (is_leaf) {
        if (gb_node) GB_remove_callback(gb_node, GB_CB_DELETE, makeDatabaseCallback(cb, this));
    }
    else {
        get_leftson() ->remove_delete_cb_rec(cb);
        get_rightson()->remove_delete_cb_rec(cb);
    }

}

GB_ERROR ARB_seqtree::preloadLeafSequences() {
    GB_ERROR error;
    if (is_leaf) {
        if (gb_node) {
            seq   = get_tree_root()->get_seqTemplate()->dup();
            error = seq->bind_to_species(gb_node); // does not load sequences yet
        }
    }
    else {
        error             = get_leftson()->preloadLeafSequences();
        if (!error) error = get_rightson()->preloadLeafSequences();
    }
    return error;
}

void ARB_seqtree::unloadSequences() {
    delete seq;
    seq = NULL;
    if (!is_leaf) {
        get_leftson()->unloadSequences();
        get_rightson()->unloadSequences();
    }
}

void ARB_seqtree::replace_seq(AP_sequence *sequence) {
    if (seq) {
        delete seq;
        seq = 0;
    }
    set_seq(sequence);
}

// ------------------------
//      ARB_countedTree

size_t ARB_countedTree::relative_position_in(const ARB_countedTree *upgroup) const {
    at_assert(is_inside(upgroup));

    size_t pos = 0;
    if (this != upgroup) {
        pos = is_upper_son() ? 0 : get_brother()->get_leaf_count();
        pos += get_father()->relative_position_in(upgroup);
    }
    return pos;
}

