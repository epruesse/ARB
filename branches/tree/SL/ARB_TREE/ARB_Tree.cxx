// =============================================================== //
//                                                                 //
//   File      : ARB_Tree.cxx                                      //
//   Purpose   : C++ wrapper for (linked) GBT_TREEs                //
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

// ----------------------
//      ARB_tree_root

static void tree_deleted_cbwrapper(GBDATA *gb_tree, int *cl_arb_tree_root, GB_CB_TYPE) {
    ARB_tree_root *troot = (ARB_tree_root*)cl_arb_tree_root;
    troot->tree_deleted_cb(gb_tree);
}


ARB_tree_root::ARB_tree_root(AliView *aliView, const ARB_tree& nodeTempl, AP_sequence *seqTempl, bool add_delete_callbacks)
    : ali(aliView)
    , rootNode(NULL)
    , nodeTemplate(nodeTempl.dup())
    , seqTemplate(seqTempl ? seqTempl->dup() : NULL)
    , tree_name(NULL)
    , gb_tree(NULL)
    , isLinkedToDB(false)
    , addDeleteCallbacks(add_delete_callbacks)
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

ARB_tree_root::~ARB_tree_root() {
    delete rootNode;
    at_assert(!rootNode);
    delete ali;
    delete nodeTemplate;
    delete seqTemplate;

    if (gb_tree) GB_remove_callback(gb_tree, GB_CB_DELETE, tree_deleted_cbwrapper, (int*)this);
    free(tree_name);
}

void ARB_tree_root::tree_deleted_cb(GBDATA *gb_tree_del) {
    if (gb_tree == gb_tree_del) {                   // ok - it's my tree
        gb_tree = NULL;
        freenull(tree_name);
    }
    else {
        at_assert(0); // callback for wrong tree received
    }
}
void ARB_tree_root::change_root(ARB_tree *oldroot, ARB_tree *newroot) {
    at_assert(rootNode == oldroot);
    rootNode = newroot;

    if (oldroot && oldroot->get_tree_root() && !oldroot->is_inside(newroot)) oldroot->set_tree_root(0); // unlink from this
    if (newroot && newroot->get_tree_root() != this) newroot->set_tree_root(this); // link to this
}

GB_ERROR ARB_tree_root::loadFromDB(const char *name) {
    GBDATA   *gb_main = get_gb_main();
    GB_ERROR  error   = GB_push_transaction(gb_main);

    if (!error) {
        ARB_tree *old_root = get_root_node();
        if (old_root) {
            change_root(old_root, NULL);
            delete old_root;
        }

        if (gb_tree) {
            GB_remove_callback(gb_tree, GB_CB_DELETE, tree_deleted_cbwrapper, (int*)this);
            gb_tree = NULL;
            freenull(tree_name);
        }

        GBT_TREE *gbt_tree   = GBT_read_tree(gb_main, name, sizeof(GBT_TREE));
        if (!gbt_tree) error = GB_await_error();
        else {
            gb_tree             = GBT_find_tree(gb_main, name);
            if (!gb_tree) error = GB_await_error();
            else {
                error = GB_add_callback(gb_tree, GB_CB_DELETE, tree_deleted_cbwrapper, (int*)this);
                if (!error) {
                    ARB_tree *arb_tree = nodeTemplate->dup();
                    arb_tree->move_gbt_info(gbt_tree);

                    change_root(NULL, arb_tree);
                    tree_name    = strdup(name);
                    isLinkedToDB = false;
                }
                else {
                    gb_tree = NULL;
                }
            }
            GBT_delete_tree(gbt_tree);
        }
    }

    return GB_end_transaction(gb_main, error);
}

GB_ERROR ARB_tree_root::saveToDB() {
    GB_ERROR error;
    if (!gb_tree) {
        error = "Can't save your tree (no tree loaded or tree has been deleted)";
    }
    else {
        GBDATA *gb_main   = get_gb_main();
        error             = GB_push_transaction(gb_main);
        at_assert(rootNode);
        if (!error) error = GBT_overwrite_tree(gb_main, gb_tree, rootNode->get_gbt_tree());
        error             = GB_end_transaction(gb_main, error);
    }
    return error;
}

static void arb_tree_species_deleted_cb(GBDATA *gb_species, int *cl_ARB_tree, GB_CB_TYPE) {
    // called whenever a species (which is linked to tree) gets deleted
    ARB_tree *arb_tree = (ARB_tree*)cl_ARB_tree;
    at_assert(arb_tree->gb_node == gb_species);
    if (arb_tree->gb_node == gb_species) {
        arb_tree->gb_node = NULL; // unlink from tree
    }
}

GB_ERROR ARB_tree_root::linkToDB(int *zombies, int *duplicates) {
    at_assert(!ali->has_data() || get_seqTemplate()); // if ali has data, you have to set_seqTemplate() before linking

    GB_ERROR error = 0;
    if (!isLinkedToDB) {
        error = GBT_link_tree(rootNode->get_gbt_tree(), get_gb_main(), false, zombies, duplicates);
        if (!error && addDeleteCallbacks) error = rootNode->add_delete_cb_rec(arb_tree_species_deleted_cb);
        if (!error) {
            if (ali->has_data() && seqTemplate) rootNode->preloadLeafSequences();
            isLinkedToDB = true;
        }
    }
    return error;
}

void ARB_tree_root::unlinkFromDB() {
    if (isLinkedToDB) {
        if (addDeleteCallbacks) rootNode->remove_delete_cb_rec(arb_tree_species_deleted_cb);
        GBT_unlink_tree(rootNode->get_gbt_tree());
        if (ali->has_data() && seqTemplate) rootNode->unloadSequences();
        isLinkedToDB = false;
    }
}

// ----------------------
//      ARB_tree_info

ARB_tree_info::ARB_tree_info() {
    memset(this, 0, sizeof(*this));
}

void ARB_tree::calcTreeInfo(ARB_tree_info& info) {
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
        leftson->calcTreeInfo(info);
        rightson->calcTreeInfo(info);
    }
}

// -----------------
//      ARB_tree


#if defined(DEBUG)
static bool vtable_ptr_check_done = false;
#endif // DEBUG

ARB_tree::ARB_tree(ARB_tree_root *troot)
    : seq(NULL)
{
    CLEAR_GBT_TREE_ELEMENTS(this);
    tree_root = troot;

#if defined(DEBUG)
    if (!vtable_ptr_check_done) {
        GBT_TREE *tree     = get_gbt_tree();        // hack-warning: points to part of this!
        bool      was_leaf = tree->is_leaf;

        // if one of the assertions below fails, then there is a problem with the
        // vtable-pointer position (grep for FAKE_VTAB_PTR for more info)
        tree->is_leaf = false; at_assert(!is_leaf);
        tree->is_leaf = true;  at_assert(is_leaf);
        tree->is_leaf = was_leaf;

        vtable_ptr_check_done = true;
    }
#endif // DEBUG
}

ARB_tree::~ARB_tree() {
    free(name);
    free(remark_branch);

    unlink_from_father();

    if (tree_root && tree_root->get_root_node() == this) {
        tree_root->ARB_tree_root::change_root(this, NULL);
    }

    delete leftson;                                 // implicitely sets leftson  = NULL
    at_assert(!leftson);
    delete rightson;                                // implicitely sets rightson = NULL
    at_assert(!rightson);

    delete seq;
}

void ARB_tree::move_gbt_info(GBT_TREE *tree) {
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

bool ARB_tree::is_inside(const ARB_tree *subtree) const {
    return this == subtree || (father && father->is_inside(subtree));
}

#if defined(CHECK_TREE_STRUCTURE)

void ARB_tree::assert_valid() const {
    at_assert(this);
    if (!is_leaf) {
        at_assert(rightson);
        at_assert(leftson);
        rightson->assert_valid();
        leftson->assert_valid();
    }

    ARB_tree_root *troot = get_tree_root();
    if (father) {
        at_assert(is_son(father));
        if (troot) {
            at_assert(troot->get_root_node()->is_anchestor_of(this));
        }
        else {
            at_assert(father->get_tree_root() == NULL); // if this has no root, father as well shouldn't have root
        }
    }
    else {                                          // this is root
        if (troot) {
            at_assert(troot->get_root_node()  == this);
            at_assert(!is_leaf);                    // leaf@root (tree has to have at least 2 leafs)
        }
    }
}
#endif // CHECK_TREE_STRUCTURE

void ARB_tree::mark_subtree() {
    if (is_leaf) {
        if (gb_node) GB_write_flag(gb_node, 1);
    }
    else {
        leftson->mark_subtree();
        rightson->mark_subtree();
    }
}

bool ARB_tree::contains_marked_species() {
    if (is_leaf) {
        return gb_node && GB_read_flag(gb_node) != 0;
    }
    return leftson->contains_marked_species() || rightson->contains_marked_species();
}

void ARB_tree::set_tree_root(ARB_tree_root *new_root) {
    if (tree_root != new_root) {
        tree_root = new_root;
        if (leftson) leftson->set_tree_root(new_root);
        if (rightson) rightson->set_tree_root(new_root);
    }
}

GB_ERROR ARB_tree::add_delete_cb_rec(ARB_tree_node_del_cb cb) {
    GB_ERROR error = NULL;
    if (is_leaf) {
        if (gb_node) {
            error = GB_add_callback(gb_node, GB_CB_DELETE, (GB_CB)cb, (int*)this);
        }
    }
    else {
        error            = leftson ->add_delete_cb_rec(cb);
        if (error) error = rightson->add_delete_cb_rec(cb);
    }
    return error;
}

void ARB_tree::remove_delete_cb_rec(ARB_tree_node_del_cb cb) {
    if (is_leaf) {
        if (gb_node) GB_remove_callback(gb_node, GB_CB_DELETE, (GB_CB)cb, (int*)this);
    }
    else {
        leftson ->remove_delete_cb_rec(cb);
        rightson->remove_delete_cb_rec(cb);
    }

}

void ARB_tree::preloadLeafSequences() {
    if (is_leaf) {
        if (gb_node) {
            seq = tree_root->get_seqTemplate()->dup();
            seq->bind_to_species(gb_node); // does not load sequences yet
        }
    }
    else {
        leftson->preloadLeafSequences();
        rightson->preloadLeafSequences();
    }
}

void ARB_tree::unloadSequences() {
    delete seq;
    seq = NULL;
    if (!is_leaf) {
        leftson->unloadSequences();
        rightson->unloadSequences();
    }
}

void ARB_tree::replace_seq(AP_sequence *sequence) {
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

