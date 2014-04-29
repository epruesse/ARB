// =============================================================== //
//                                                                 //
//   File      : AP_Tree.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AP_Tree.hxx"

#include <AP_filter.hxx>
#include <awt_attributes.hxx>
#include <aw_msg.hxx>

#include <math.h>
#include <map>
#include <climits>
#include <arb_progress.h>
#include <ad_cb.h>

using namespace std;

/*!***************************************************************************************
 ************           Rates                       **********
 *****************************************************************************************/
void AP_rates::print()
{
    AP_FLOAT max;
    int i;

    max = 0.0;
    for (i=0; i<rate_len; i++) {
        if (rates[i] > max) max = rates[i];
    }
    printf("rates:");
    for (i=0; i<rate_len; i++) {
        putchar('0' + (int)(rates[i]/max*9.9));
    }
    printf("\n");
}

AP_rates::AP_rates() {
    memset ((char *)this, 0, sizeof(AP_rates));
}

char *AP_rates::init(AP_filter *fil) {
    if (fil->get_timestamp() <= this->update) return 0;

    rate_len = fil->get_filtered_length();
    delete [] rates;
    rates = new AP_FLOAT[rate_len];
    for (int i=0; i<rate_len; i++) {
        rates[i] = 1.0;
    }
    this->update = fil->get_timestamp();
    return 0;
}

char *AP_rates::init(AP_FLOAT * ra, AP_filter *fil)
{
    if (fil->get_timestamp() <= this->update) return 0;

    rate_len = fil->get_filtered_length();
    delete [] rates;
    rates = new AP_FLOAT[rate_len];
    int i, j;
    for (j=i=0; i<rate_len; j++) {
        if (fil->use_position(j)) {
            rates[i++] = ra[j];
        }
    }
    this->update = fil->get_timestamp();
    return 0;
}

AP_rates::~AP_rates() { delete [] rates; }


/*!***************************************************************************************
 ************           AP_tree_root                    **********
 *****************************************************************************************/

AP_tree_root::AP_tree_root(AliView *aliView, RootedTreeNodeFactory *nodeMaker_, AP_sequence *seq_proto, bool add_delete_callbacks)
    : ARB_seqtree_root(aliView, nodeMaker_, seq_proto, add_delete_callbacks),
      root_changed_cb(NULL), root_changed_cd(NULL),
      node_deleted_cb(NULL), node_deleted_cd(NULL),
      gb_tree_gone(NULL),
      gone_tree_name(NULL),
      tree_timer(0),
      species_timer(0),
      table_timer(0),
      rates(NULL)
{
    GBDATA         *gb_main = get_gb_main();
    GB_transaction  ta(gb_main);

    gb_species_data = GBT_get_species_data(gb_main);
    gb_table_data   = GB_search(gb_main, "table_data", GB_CREATE_CONTAINER);
}

AP_tree_root::~AP_tree_root() {
    delete get_root_node();
    free(gone_tree_name);
    ap_assert(!get_root_node());
}

bool AP_tree_root::is_tree_updated() {
    GBDATA *gbtree = get_gb_tree();
    if (gbtree) {
        GB_transaction ta(gbtree);
        return GB_read_clock(gbtree)>tree_timer;
    }
    return true;
}

bool AP_tree_root::is_species_updated() {
    if (gb_species_data) {
        GB_transaction ta(gb_species_data);
        return
            GB_read_clock(gb_species_data)>species_timer ||
            GB_read_clock(gb_table_data)>table_timer;
    }
    return true;
}

void AP_tree_root::update_timers() {
    if (gb_species_data) {
        GB_transaction  ta(GB_get_root(gb_species_data));
        GBDATA         *gbtree = get_gb_tree();
        if (gbtree) tree_timer = GB_read_clock(gbtree);
        species_timer          = GB_read_clock(gb_species_data);
        table_timer            = GB_read_clock(gb_table_data);
    }
}

/*!***************************************************************************************
************           AP_tree                     **********
*****************************************************************************************/

static void ap_tree_node_deleted(GBDATA *, AP_tree *tree) {
    tree->gb_node = 0;
}

AP_tree::~AP_tree() {
    if (gr.callback_exists && gb_node) {
        GB_remove_callback(gb_node, GB_CB_DELETE, makeDatabaseCallback(ap_tree_node_deleted, this));
    }

    AP_tree_root *root = get_tree_root();
    if (root) root->inform_about_delete(this);
}

void AP_tree::clear_branch_flags() {
    br.clear();
    if (!is_leaf) {
        get_leftson()->clear_branch_flags();
        get_rightson()->clear_branch_flags();
    }
}

void AP_tree::initial_insert(AP_tree *new_brother, AP_tree_root *troot) {
    ap_assert(troot);
    ap_assert(is_leaf);
    ap_assert(new_brother->is_leaf);
    ap_assert(!troot->get_root_node());

    ASSERT_VALID_TREE(this);
    ASSERT_VALID_TREE(new_brother);

    AP_tree *new_root = DOWNCAST(AP_tree*, troot->makeNode());

    new_root->leftson  = this;
    new_root->rightson = new_brother;
    new_root->father   = NULL;

    father              = new_root;
    new_brother->father = new_root;

    new_root->leftlen  = 0.5;
    new_root->rightlen = 0.5;

    troot->change_root(NULL, new_root);

    set_tree_root(troot);
    new_brother->set_tree_root(troot);
}

void AP_tree::insert(AP_tree *new_brother) {
    ASSERT_VALID_TREE(this);
    ASSERT_VALID_TREE(new_brother);

    AP_tree  *new_tree = DOWNCAST(AP_tree*, new_brother->get_tree_root()->makeNode());
    AP_FLOAT  laenge;

    new_tree->leftson  = this;
    new_tree->rightson = new_brother;
    new_tree->father   = new_brother->father;
    father             = new_tree;

    if (new_brother->father) {
        if (new_brother->father->leftson == new_brother) {
            laenge = new_brother->father->leftlen = new_brother->father->leftlen*.5;
            new_brother->father->leftson = new_tree;
        }
        else {
            laenge = new_brother->father->rightlen = new_brother->father->rightlen*.5;
            new_brother->father->rightson = new_tree;
        }
    }
    else {
        laenge = 0.5;
    }
    new_tree->leftlen   = laenge;
    new_tree->rightlen  = laenge;
    new_brother->father = new_tree;

    AP_tree_root *troot = new_brother->get_tree_root();
    ap_assert(troot); // Note: initial_insert() has to be used to build initial tree

    if (!new_tree->father) troot->change_root(new_brother, new_tree);
    else new_tree->set_tree_root(troot);
    set_tree_root(troot);

    ASSERT_VALID_TREE(troot->get_root_node());
}

void AP_tree_root::change_root(RootedTree *oldroot, RootedTree *newroot) {
    if (root_changed_cb) {
        root_changed_cb(root_changed_cd, DOWNCAST(AP_tree*, oldroot), DOWNCAST(AP_tree*, newroot));
    }
    if (!oldroot) {
        ap_assert(newroot);
        if (gb_tree_gone) {                         // when tree was temporarily deleted (e.g. by 'Remove & add all')
            set_gb_tree(gb_tree_gone);              // re-use previous DB entry
            gb_tree_gone = NULL;
        }
    }
    if (!newroot) {                                 // tree empty
        GBDATA *gbtree = get_gb_tree();
        if (gbtree) {
            ap_assert(gb_tree_gone == NULL);        // no tree should be remembered yet
            gb_tree_gone = gbtree;                  // remember for deletion (done in AP_tree::save)
        }
    }
    ARB_seqtree_root::change_root(oldroot, newroot);
}

void AP_tree_root::inform_about_delete(AP_tree *del) {
    if (node_deleted_cb) node_deleted_cb(node_deleted_cd, del);
}

void AP_tree_root::set_root_changed_callback(AP_rootChangedCb cb, void *cd) {
    root_changed_cb = cb;
    root_changed_cd = cd;
}

void AP_tree_root::set_node_deleted_callback(AP_nodeDelCb cb, void *cd) {
    node_deleted_cb = cb;
    node_deleted_cd = cd;
}


void AP_tree::remove() {
    // remove this + father from tree
    // Note: does not delete this or father!

    ASSERT_VALID_TREE(this);
    if (father == 0) {
        get_tree_root()->change_root(this, NULL); // tell AP_tree_root that the root node has been removed
        forget_origin(); // removed nodes are rootless
    }
    else {
        AP_tree      *brother     = get_brother();  // brother remains in tree
        GBT_LEN       brothersLen = brother->get_branchlength();
        AP_tree      *fath        = get_father();   // fath of this is removed as well
        ARB_seqtree  *grandfather = fath->get_father();
        AP_tree_root *troot       = get_tree_root();

        if (fath->gb_node) {                      // move inner information to remaining subtree
            if (!brother->gb_node && !brother->is_leaf) {
                brother->gb_node = fath->gb_node;
                fath->gb_node    = 0;
            }
        }

        if (grandfather) {
            brother->unlink_from_father();

            bool wasLeftSon = fath->is_leftson();
            fath->unlink_from_father();

            if (wasLeftSon) {
                ap_assert(!grandfather->leftson);
                grandfather->leftlen += brothersLen;
                grandfather->leftson  = brother;
            }
            else {
                ap_assert(!grandfather->rightson);
                grandfather->rightlen += brothersLen;
                grandfather->rightson  = brother;
            }
            brother->father = grandfather;
        }
        else {                                      // father is root, make brother the new root
            if (brother->is_leaf) {
                troot->change_root(fath, NULL);     // erase tree from root
            }
            else {
                brother->unlink_from_father();
                troot->change_root(fath, brother);
            }
        }

        ap_assert(fath == father);

        ASSERT_VALID_TREE_OR_NULL(troot->get_root_node());

        troot->inform_about_delete(fath);
        troot->inform_about_delete(this);

        fath->forget_origin();
        ASSERT_VALID_TREE(fath);
    }
}

GB_ERROR AP_tree::cantMoveNextTo(AP_tree *new_brother) {
    GB_ERROR error = 0;

    if (!father)                                error = "Can't move the root of the tree";
    else if (!new_brother->father)              error = "Can't move to the root of the tree";
    else if (new_brother->father == father)     error = "Already there";
    else if (!father->father)                   error = "Can't move son of root";
    else if (new_brother->is_inside(this))      error = "Can't move a subtree into itself";

    return error;
}

void AP_tree::moveNextTo(AP_tree *new_brother, AP_FLOAT rel_pos) {
    // rel_pos == 0.0 -> at father
    //         == 1.0 -> at brother

    ap_assert(father);
    ap_assert(new_brother->father);
    ap_assert(new_brother->father != father);       // already there
    ap_assert(!new_brother->is_inside(this));       // can't move tree into itself

    if (father->leftson != this) get_father()->swap_sons();

    if (father->father == 0) {
        get_brother()->father = 0;
        get_tree_root()->change_root(get_father(), get_brother());
    }
    else {
        ARB_seqtree *grandfather = get_father()->get_father();
        if (father == new_brother) {    // just pull branches !!
            new_brother  = get_brother();
            if (grandfather->leftson == father) {
                rel_pos *= grandfather->leftlen / (father->rightlen+grandfather->leftlen);
            }
            else {
                rel_pos *= grandfather->rightlen / (father->rightlen+grandfather->rightlen);
            }
        }
        else if (new_brother->father == father) { // just pull branches !!
            rel_pos =
                1.0 + (rel_pos-1.0) * father->rightlen
                /
                (father->rightlen + (grandfather->leftson == father ? grandfather->leftlen : grandfather->rightlen));
        }

        if (grandfather->leftson == father) {
            grandfather->leftlen     += father->rightlen;
            grandfather->leftson      = father->rightson;
            father->rightson->father  = grandfather;
        }
        else {
            grandfather->rightlen    += father->rightlen;
            grandfather->rightson     = father->rightson;
            father->rightson->father  = grandfather;
        }
    }

    ARB_seqtree *new_tree          = get_father();
    ARB_seqtree *brother_father    = new_brother->get_father();
    AP_FLOAT  laenge;

    if (brother_father->leftson == new_brother) {
        laenge  = brother_father->leftlen;
        laenge -= brother_father->leftlen = laenge * rel_pos;
        new_brother->father->leftson = new_tree;
    }
    else {
        laenge  = brother_father->rightlen;
        laenge -= brother_father->rightlen = laenge * rel_pos;
        brother_father->rightson = new_tree;
    }

    new_tree->rightlen  = laenge;
    new_brother->father = new_tree;
    new_tree->rightson  = new_brother;
    new_tree->father    = brother_father;
}

void AP_tree::swap_assymetric(AP_TREE_SIDE mode) {
    // mode AP_LEFT exchanges lefts with brother
    // mode AP_RIGHT exchanges rights with brother

    ap_assert(!is_leaf);                           // cannot swap leafs
    ap_assert(father);                             // cannot swap root (has no brother)
    ap_assert(mode == AP_LEFT || mode == AP_RIGHT); // illegal mode

    ARB_seqtree *pntr;

    if (father->father == 0) { // father is root
        AP_tree *pntr_brother = get_brother();
        if (!pntr_brother->is_leaf) {
            if (mode == AP_LEFT) {
                pntr_brother->leftson->father = this;
                pntr                          = pntr_brother->get_leftson();
                pntr_brother->leftson         = leftson;
                leftson->father               = pntr_brother;
                leftson                       = pntr;
            }
            else {
                pntr_brother->leftson->father = this;
                rightson->father              = pntr_brother;
                pntr                          = pntr_brother->get_leftson();
                pntr_brother->leftson         = rightson;
                rightson                      = pntr;
            }
        }
    }
    else {
        if (mode == AP_LEFT) { // swap leftson with brother
            if (father->leftson == this) {
                father->rightson->father = this;
                leftson->father          = father;
                pntr                     = get_father()->get_rightson();
                AP_FLOAT help_len        = father->rightlen;
                father->rightlen         = leftlen;
                leftlen                  = help_len;
                father->rightson         = leftson;
                leftson                  = pntr;
            }
            else {
                father->leftson->father = this;
                leftson->father         = father;
                pntr                    = get_father()->get_leftson();
                AP_FLOAT help_len       = father->leftlen;
                father->leftlen         = leftlen;
                leftlen                 = help_len;
                father->leftson         = leftson;
                leftson                 = pntr;
            }
        }
        else { // swap rightson with brother
            if (father->leftson == this) {
                father->rightson->father = this;
                rightson->father         = father;
                pntr                     = get_father()->get_rightson();
                AP_FLOAT help_len        = father->rightlen;
                father->rightlen         = rightlen;
                rightlen                 = help_len;
                father->rightson         = rightson;
                rightson                 = pntr;
            }
            else {
                father->leftson->father = this;
                rightson->father        = father;
                pntr                    = get_father()->get_leftson();
                AP_FLOAT help_len       = father->leftlen;
                father->leftson         = rightson;
                father->leftlen         = rightlen;
                rightlen                = help_len;
                rightson                = pntr;
            }
        }
    }
}

void AP_tree::set_root() {
    if (at_root()) return; // already root

    {
        AP_tree           *old_brother = 0;
        AP_branch_members  br1         = br;
        AP_tree           *pntr;

        for  (pntr = get_father(); pntr->father; pntr = pntr->get_father()) {
            AP_branch_members br2 = pntr->br;
            pntr->br              = br1;
            br1                   = br2;
            old_brother           = pntr;
        }
        if (pntr->leftson == old_brother) {
            pntr->get_rightson()->br = br1;
        }
    }

    ARB_seqtree::set_root();
}


inline int tree_read_byte(GBDATA *tree, const char *key, int init) {
    if (tree) {
        GBDATA *gbd = GB_entry(tree, key);
        if (gbd) return GB_read_byte(gbd);
    }
    return init;
}

inline float tree_read_float(GBDATA *tree, const char *key, float init) {
    if (tree) {
        GBDATA *gbd = GB_entry(tree, key);
        if (gbd) return GB_read_float(gbd);
    }
    return init;
}



//! moves all node/leaf information from struct GBT_TREE to AP_tree
void AP_tree::load_node_info() {
    gr.spread          = tree_read_float(gb_node, "spread",          1.0);
    gr.left_angle      = tree_read_float(gb_node, "left_angle",      0.0);
    gr.right_angle     = tree_read_float(gb_node, "right_angle",     0.0);
    gr.left_linewidth  = tree_read_byte (gb_node, "left_linewidth",  0);
    gr.right_linewidth = tree_read_byte (gb_node, "right_linewidth", 0);
    gr.grouped         = tree_read_byte (gb_node, "grouped",         0);
}

void AP_tree::load_subtree_info() {
    load_node_info();
    if (!is_leaf) {
        get_leftson()->load_subtree_info();
        get_rightson()->load_subtree_info();
    }
}

#if defined(DEBUG)
#if defined(DEVEL_RALF)
#define DEBUG_tree_write_byte
#endif // DEVEL_RALF
#endif // DEBUG


static GB_ERROR tree_write_byte(GBDATA *gb_tree, AP_tree *node, short i, const char *key, int init) {
    GBDATA *gbd;
    GB_ERROR error = 0;
    if (i==init) {
        if (node->gb_node) {
            gbd = GB_entry(node->gb_node, key);
            if (gbd) {
#if defined(DEBUG_tree_write_byte)
                printf("[tree_write_byte] deleting db entry %p\n", gbd);
#endif // DEBUG_tree_write_byte
                GB_delete(gbd);
            }
        }
    }
    else {
        if (!node->gb_node) {
            node->gb_node = GB_create_container(gb_tree, "node");
#if defined(DEBUG_tree_write_byte)
            printf("[tree_write_byte] created node-container %p\n", node->gb_node);
#endif // DEBUG_tree_write_byte
        }
        gbd = GB_entry(node->gb_node, key);
        if (!gbd) {
            gbd = GB_create(node->gb_node, key, GB_BYTE);
#if defined(DEBUG_tree_write_byte)
            printf("[tree_write_byte] created db entry %p\n", gbd);
#endif // DEBUG_tree_write_byte
        }
        error = GB_write_byte(gbd, i);
    }
    return error;
}

static GB_ERROR tree_write_float(GBDATA *gb_tree, AP_tree *node, float f, const char *key, float init) {
    GB_ERROR error = NULL;
    if (f==init) {
        if (node->gb_node) {
            GBDATA *gbd    = GB_entry(node->gb_node, key);
            if (gbd) error = GB_delete(gbd);
        }
    }
    else {
        if (!node->gb_node) {
            node->gb_node             = GB_create_container(gb_tree, "node");
            if (!node->gb_node) error = GB_await_error();
        }
        if (!error) error = GBT_write_float(node->gb_node, key, f);
    }
    return error;
}

GB_ERROR AP_tree::tree_write_tree_rek(GBDATA *gb_tree) {
    GB_ERROR error = NULL;
    if (!is_leaf) {
        error             = get_leftson()->tree_write_tree_rek(gb_tree);
        if (!error) error = get_rightson()->tree_write_tree_rek(gb_tree);

        if (!error) error = tree_write_float(gb_tree, this, gr.spread,          "spread",          1.0);
        if (!error) error = tree_write_float(gb_tree, this, gr.left_angle,      "left_angle",      0.0);
        if (!error) error = tree_write_float(gb_tree, this, gr.right_angle,     "right_angle",     0.0);
        if (!error) error = tree_write_byte (gb_tree, this, gr.left_linewidth,  "left_linewidth",  0);
        if (!error) error = tree_write_byte (gb_tree, this, gr.right_linewidth, "right_linewidth", 0);
        if (!error) error = tree_write_byte (gb_tree, this, gr.grouped,         "grouped",         0);
    }
    return error;
}

GB_ERROR AP_tree_root::saveToDB() {
    GB_ERROR  error  = GB_push_transaction(get_gb_main());
    if (get_gb_tree()) {
        error = get_root_node()->tree_write_tree_rek(get_gb_tree());
    }
    else {
        ap_assert(!gb_tree_gone);      // should have been handled by caller (e.g. in AWT_graphic_tree::save)
    }
    if (!error) {
        if (!get_gb_tree() && gone_tree_name) { // tree was deleted before
            GBDATA *gb_tree_exists = GBT_find_tree(get_gb_main(), gone_tree_name);
            if (gb_tree_exists) {
                error = "tree already exists";
            }
            else {
                error = GBT_write_tree(get_gb_main(), gone_tree_name, get_root_node());
                if (!error) {
                    gb_tree_exists = GBT_find_tree(get_gb_main(), gone_tree_name);
                    ap_assert(gb_tree_exists);
                    if (gb_tree_exists) {
                        set_gb_tree_and_name(GBT_find_tree(get_gb_main(), gone_tree_name), gone_tree_name);
                        aw_message(GBS_global_string("Recreated previously deleted '%s'", gone_tree_name));
                        freenull(gone_tree_name);
                    }
                }
            }

            if (error) aw_message(GBS_global_string("Failed to recreate '%s' (Reason: %s)", gone_tree_name, error));
        }

        if (!error) error = ARB_seqtree_root::saveToDB();
    }
    if (!error) update_timers();

    return GB_end_transaction(get_gb_main(), error);
}


GB_ERROR AP_tree::move_group_info(AP_tree *new_group) {
    GB_ERROR error = 0;
    if (is_leaf || !name) {
        error = "Please select a valid group";
    }
    else if (!gb_node) {
        error = "Internal Error: group with name is missing DB-entry";
    }
    else if (new_group->is_leaf) {
        if (new_group->name) {
            error = GBS_global_string("'%s' is not a valid target for group information of '%s'.", new_group->name, name);
        }
        else if (new_group->gb_node) {
            error = "Internal Error: Target node already has a database entry (but no name)";
        }
    }

    if (!error) {
        if (new_group->name) {
            if (!new_group->gb_node) {
                error = "Internal Error: Target node has a database entry (but no name)";
            }
            else { // exchange information of two groups
                GBDATA *tmp_node   = new_group->gb_node;
                char   *tmp_name   = new_group->name;
                new_group->gb_node = gb_node;
                new_group->name    = name;
                name               = tmp_name;
                gb_node            = tmp_node;
            }
        }
        else { // move group info
            new_group->gb_node = this->gb_node;
            new_group->name    = this->name;
            this->name         = 0;
            this->gb_node      = 0;
        }

        this->load_node_info();
        new_group->load_node_info();

        {
            GBDATA *gb_group_name;
            gb_group_name = GB_entry(new_group->gb_node, "group_name");
            if (gb_group_name) GB_touch(gb_group_name); // force taxonomy reload
        }
    }
    return error;
}

void AP_tree::update() {
    GB_transaction ta(get_tree_root()->get_gb_main());
    get_tree_root()->update_timers();
}

void AP_tree::update_subtree_information() {
    gr.hidden = get_father() ? (get_father()->gr.hidden || get_father()->gr.grouped) : 0;

    if (is_leaf) {
        gr.view_sum = 1;
        gr.leaf_sum = 1;

        gr.max_tree_depth = 0.0;
        gr.min_tree_depth = 0.0;

        bool is_marked = gb_node && GB_read_flag(gb_node);

        gr.has_marked_children = is_marked;

        // colors:
        if (gb_node) {
            if (is_marked) {
                gr.gc = AWT_GC_SELECTED;
            }
            else {
                // check for user color
                long color_group = AWT_species_get_dominant_color(gb_node);
                if (color_group == 0) {
                    gr.gc = AWT_GC_NSELECTED;
                }
                else {
                    gr.gc = AWT_GC_FIRST_COLOR_GROUP+color_group-1;
                }
            }
        }
        else {
            gr.gc = AWT_GC_ZOMBIES;
        }
    }
    else {
        get_leftson()->update_subtree_information();
        get_rightson()->update_subtree_information();

        {
            AP_tree_members& left  = get_leftson()->gr;
            AP_tree_members& right = get_rightson()->gr;

            gr.leaf_sum = left.leaf_sum + right.leaf_sum;
            gr.view_sum = left.view_sum + right.view_sum;
            if (gr.grouped) {
                gr.view_sum = (int)pow((double)(gr.leaf_sum - GROUPED_SUM + 9), .33);
            }

            gr.min_tree_depth = std::min(leftlen+left.min_tree_depth, rightlen+right.min_tree_depth);
            gr.max_tree_depth = std::max(leftlen+left.max_tree_depth, rightlen+right.max_tree_depth);

            gr.has_marked_children = left.has_marked_children || right.has_marked_children;

            // colors:
            if (left.gc == right.gc) gr.gc = left.gc;

            else if (left.gc == AWT_GC_SELECTED || right.gc == AWT_GC_SELECTED) gr.gc = AWT_GC_UNDIFF;

            else if (left.gc  == AWT_GC_ZOMBIES) gr.gc = right.gc;
            else if (right.gc == AWT_GC_ZOMBIES) gr.gc = left.gc;

            else if (left.gc == AWT_GC_UNDIFF || right.gc == AWT_GC_UNDIFF) gr.gc = AWT_GC_UNDIFF;

            else {
                ap_assert(left.gc != AWT_GC_SELECTED && right.gc != AWT_GC_SELECTED);
                ap_assert(left.gc != AWT_GC_UNDIFF   && right.gc != AWT_GC_UNDIFF);
                gr.gc = AWT_GC_NSELECTED;
            }
        }
    }
}

unsigned AP_tree::count_leafs() const {
    return is_leaf
        ? 1
        : get_leftson()->count_leafs() + get_rightson()->count_leafs();
}

int AP_tree::colorize(GB_HASH *hashptr) {
    // colorizes the tree according to 'hashptr'
    // hashkey   = species id
    // hashvalue = gc

    int res;
    if (is_leaf) {
        if (gb_node) {
            res = GBS_read_hash(hashptr, name);
            if (!res && GB_read_flag(gb_node)) { // marked but not in hash -> black
                res = AWT_GC_BLACK;
            }
        }
        else {
            res = AWT_GC_ZOMBIES;
        }
    }
    else {
        int l = get_leftson()->colorize(hashptr);
        int r = get_rightson()->colorize(hashptr);

        if      (l == r)              res = l;
        else if (l == AWT_GC_ZOMBIES) res = r;
        else if (r == AWT_GC_ZOMBIES) res = l;
        else                          res = AWT_GC_UNDIFF;
    }
    gr.gc = res;
    return res;
}

void AP_tree::compute_tree() {
    GB_transaction ta(get_tree_root()->get_gb_main());
    update_subtree_information();
}

GB_ERROR AP_tree_root::loadFromDB(const char *name) {
    GB_ERROR error = ARB_seqtree_root::loadFromDB(name);
    if (!error) {
        AP_tree *rNode = DOWNCAST(AP_tree*, get_root_node());
        rNode->load_subtree_info();
    }
    update_timers(); // maybe after link() ? // @@@ really do if error?
    return error;
}

GB_ERROR AP_tree::relink() {
    GB_transaction ta(get_tree_root()->get_gb_main()); // open close a transaction
    GB_ERROR error = GBT_link_tree(this, get_tree_root()->get_gb_main(), false, 0, 0); // no status
    get_tree_root()->update_timers();
    return error;
}

AP_UPDATE_FLAGS AP_tree::check_update() {
    AP_tree_root *troot   = get_tree_root();
    GBDATA       *gb_main = troot->get_gb_main();

    if (!gb_main) {
        return AP_UPDATE_RELOADED;
    }
    else {
        GB_transaction ta(gb_main);

        if (troot->is_tree_updated()) return AP_UPDATE_RELOADED;
        if (troot->is_species_updated()) return AP_UPDATE_RELINKED;
        return AP_UPDATE_OK;
    }
}

#if defined(WARN_TODO)
#warning buildLeafList, buildNodeList and buildBranchList should return a AP_tree_list (new class!)
#endif

void AP_tree::buildLeafList_rek(AP_tree **list, long& num) {
    // builds a list of all species
    if (!is_leaf) {
        get_leftson()->buildLeafList_rek(list, num);
        get_rightson()->buildLeafList_rek(list, num);
    }
    else {
        list[num] = this;
        num++;
    }
}

void AP_tree::buildLeafList(AP_tree **&list, long &num) {
    num        = count_leafs();
    list       = new AP_tree *[num+1];
    list[num]  = 0;
    long count = 0;

    buildLeafList_rek(list, count);

    ap_assert(count == num);
}

void AP_tree::buildNodeList_rek(AP_tree **list, long& num) {
    // builds a list of all inner nodes (w/o root node)
    if (!is_leaf) {
        if (father) list[num++] = this;
        get_leftson()->buildNodeList_rek(list, num);
        get_rightson()->buildNodeList_rek(list, num);
    }
}

void AP_tree::buildNodeList(AP_tree **&list, long &num) {
    num = this->count_leafs()-1;
    list = new AP_tree *[num+1];
    list[num] = 0;
    num  = 0;
    buildNodeList_rek(list, num);
}

void AP_tree::buildBranchList_rek(AP_tree **list, long& num, bool create_terminal_branches, int deep) {
    // builds a list of all species
    // (returns pairs of leafs/father and nodes/father)

    if (deep) {
        if (father && (create_terminal_branches || !is_leaf)) {
            if (father->father) {
                list[num++] = this;
                list[num++] = get_father();
            }
            else {                  // root
                if (father->leftson == this) {
                    list[num++] = this;
                    list[num++] = get_brother();
                }
            }
        }
        if (!is_leaf) {
            get_leftson() ->buildBranchList_rek(list, num, create_terminal_branches, deep-1);
            get_rightson()->buildBranchList_rek(list, num, create_terminal_branches, deep-1);
        }
    }
}

void AP_tree::buildBranchList(AP_tree **&list, long &num, bool create_terminal_branches, int deep) {
    if (deep>=0) {
        num = 2;
        for (int i=0; i<deep; i++) num *= 2;
    }
    else {
        num = count_leafs() * (create_terminal_branches ? 2 : 1);
    }

    ap_assert(num >= 0);

    list = new AP_tree *[num*2+4];

    if (num) {
        long count = 0;

        buildBranchList_rek(list, count, create_terminal_branches, deep);
        list[count] = 0;
        num         = count/2;
    }
}


long AP_tree_root::remove_leafs(AWT_RemoveType awt_remove_type) {
    // may remove the complete tree (if awt_remove_type does not contain AWT_REMOVE_BUT_DONT_FREE)

    ASSERT_VALID_TREE(get_root_node());

    AP_tree **list;
    long      count;
    get_root_node()->buildLeafList(list, count);

    GB_transaction ta(get_gb_main());
    long removed = 0;

    for (long i=0; i<count; i++) {
        bool     removeNode = false;
        AP_tree *leaf       = list[i];

        if (leaf->gb_node) {
            if ((awt_remove_type & AWT_REMOVE_NO_SEQUENCE) && !leaf->get_seq()) {
                removeNode = true;
            }
            else if (awt_remove_type & (AWT_REMOVE_MARKED|AWT_REMOVE_UNMARKED)) {
                long flag  = GB_read_flag(list[i]->gb_node);
                removeNode = (flag && (awt_remove_type&AWT_REMOVE_MARKED)) || (!flag && (awt_remove_type&AWT_REMOVE_UNMARKED));
            }
        }
        else {
            if (awt_remove_type & AWT_REMOVE_ZOMBIES) {
                removeNode = true;
            }
        }

        if (removeNode) {
            list[i]->remove();
            removed++;
            if (!(awt_remove_type & AWT_REMOVE_BUT_DONT_FREE)) {
                delete list[i]->father;
            }
            if (!get_root_node()) {
                break; // tree has been deleted
            }
        }
        ASSERT_VALID_TREE(get_root_node());
    }
    delete [] list;

    ASSERT_VALID_TREE_OR_NULL(get_root_node());
    return removed;
}

// ----------------------------------------

AP_tree ** AP_tree::getRandomNodes(int anzahl) {
    // function returns a random constructed tree
    // root is tree with species (needed to build a list of species)

    AP_tree **retlist = NULL;
    if (anzahl) {
        AP_tree **list; 
        long      sumnodes;
        buildNodeList(list, sumnodes);

        if (sumnodes) {
            retlist = (AP_tree **)calloc(anzahl, sizeof(AP_tree *));

            long count = sumnodes;
            for (int i=0; i< anzahl; i++) {
                long num = GB_random(count);

                retlist[i] = list[num]; // export node
                count--;                // exclude node

                list[num]   = list[count];
                list[count] = retlist[i];

                if (count == 0) count = sumnodes; // restart it
            }
        }
        delete [] list;
    }
    return retlist;
}

// --------------------------------------------------------------------------------

template <typename T>
class ValueCounter {
    T   min, max, sum;
    int count;

    char *mean_min_max_impl() const;
    char *mean_min_max_percent_impl() const;

    mutable char *buf;
    const char *set_buf(char *content) const { freeset(buf, content); return buf; }

public:
    ValueCounter()
        : min(INT_MAX),
          max(INT_MIN),
          sum(0),
          count(0),
          buf(NULL)
    {}
    ValueCounter(const ValueCounter<T>& other)
        : min(other.min),
          max(other.max),
          sum(other.sum),
          count(other.count),
          buf(NULL)
    {}
    ~ValueCounter() { free(buf); }

    DECLARE_ASSIGNMENT_OPERATOR(ValueCounter<T>);

    void count_value(T val) {
        count++;
        min  = std::min(min, val);
        max  = std::max(max, val);
        sum += val;
    }

    int get_count() const { return count; }
    T get_min() const { return min; }
    T get_max() const { return max; }
    double get_mean() const { return sum/double(count); }

    const char *mean_min_max()         const { return count ? set_buf(mean_min_max_impl())         : "<not available>"; }
    const char *mean_min_max_percent() const { return count ? set_buf(mean_min_max_percent_impl()) : "<not available>"; }

    ValueCounter<T>& operator += (const T& inc) {
        min += inc;
        max += inc;
        sum += inc*count;
        return *this;
    }
    ValueCounter<T>& operator += (const ValueCounter<T>& other) {
        min    = std::min(min, other.min);
        max    = std::max(max, other.max);
        sum   += other.sum;
        count += other.count;
        return *this;
    }
};

template<typename T>
inline ValueCounter<T> operator+(const ValueCounter<T>& c1, const ValueCounter<T>& c2) {
    return ValueCounter<T>(c1) += c2;
}
template<typename T>
inline ValueCounter<T> operator+(const ValueCounter<T>& c, const T& inc) {
    return ValueCounter<T>(c) += inc;
}

template<> char *ValueCounter<int>::mean_min_max_impl() const {
    return GBS_global_string_copy("%.2f (range: %i .. %i)", get_mean(), get_min(), get_max());
}
template<> char *ValueCounter<double>::mean_min_max_impl() const {
    return GBS_global_string_copy("%.2f (range: %.2f .. %.2f)", get_mean(), get_min(), get_max());
}
template<> char *ValueCounter<double>::mean_min_max_percent_impl() const {
    return GBS_global_string_copy("%.2f%% (range: %.2f%% .. %.2f%%)", get_mean()*100.0, get_min()*100.0, get_max()*100.0);
}

class LongBranchMarker {
    double min_rel_diff;
    double min_abs_diff;

    int leafs;
    int nonzeroleafs;
    int multifurcs;

    ValueCounter<double> absdiff;
    ValueCounter<double> reldiff;
    ValueCounter<double> absdiff_marked;
    ValueCounter<double> reldiff_marked;

    double perform_marking(AP_tree *at, bool& marked) {
        marked = false;
        if (at->is_leaf) {
            if (at->get_branchlength() != 0.0) {
                nonzeroleafs++;
            }
            leafs++;
            return 0.0;
        }

        if (!at->is_root_node()) {
            if (at->get_branchlength() == 0.0) { // is multifurcation
                if (!at->get_father()->is_root_node() || at->is_leftson()) { // do not count two multifurcs @ sons of root
                    multifurcs++;
                }
            }
        }

        bool   marked_left;
        bool   marked_right;
        double max = perform_marking(at->get_leftson(),  marked_left)  + at->leftlen;
        double min = perform_marking(at->get_rightson(), marked_right) + at->rightlen;

        bool max_is_left = true;
        if (max<min) {
            double h = max; max = min; min = h;
            max_is_left = false;
        }

        double abs_diff = max-min;
        absdiff.count_value(abs_diff);

        double rel_diff = (max == 0.0) ? 0.0 : abs_diff/max;
        reldiff.count_value(rel_diff);

        if (abs_diff>min_abs_diff && rel_diff>min_rel_diff) {
            if (max_is_left) {
                if (!marked_left) {
                    at->get_leftson()->mark_subtree();
                    marked = true;
                }
            }
            else {
                if (!marked_right) {
                    at->get_rightson()->mark_subtree();
                    marked = true;
                }
            }
        }

        if (marked) { // just marked one of my subtrees
            absdiff_marked.count_value(abs_diff);
            reldiff_marked.count_value(rel_diff);
        }
        else {
            marked = marked_left||marked_right;
        }

        return min; // use minimal distance for whole subtree
    }

    static char *meanDiffs(const ValueCounter<double>& abs, const ValueCounter<double>& rel) {
        return GBS_global_string_copy(
            "Mean absolute diff: %s\n"
            "Mean relative diff: %s",
            abs.mean_min_max(), 
            rel.mean_min_max_percent());
    }

public:
    LongBranchMarker(AP_tree *root, double min_rel_diff_, double min_abs_diff_)
        : min_rel_diff(min_rel_diff_),
          min_abs_diff(min_abs_diff_),
          leafs(0),
          nonzeroleafs(0),
          multifurcs(0)
    {
        bool UNUSED;
        perform_marking(root, UNUSED);
    }

    const char *get_report() const {
        char *diffs_all    = meanDiffs(absdiff, reldiff);
        char *diffs_marked = meanDiffs(absdiff_marked, reldiff_marked);

        int nodes     = leafs_2_nodes(leafs, UNROOTED);
        int edges     = nodes_2_edges(nodes);
        int zeroleafs = leafs-nonzeroleafs;
        int zeroedges = multifurcs+zeroleafs;
        int realedges = edges-zeroedges;
        int furcs     = nodes-leafs;    // = inner nodes
        int realfurcs = furcs-multifurcs;

        int node_digits = log10(nodes)+1;

        ap_assert(zeroleafs<=leafs);
        ap_assert(zeroedges<=edges);
        ap_assert(realedges<=edges);
        ap_assert(multifurcs<=furcs);
        ap_assert(realfurcs<=furcs);

        const char *msg = GBS_global_string(
            "Unrooted tree contains %*i furcations,\n"
            "              of which %*i are multifurcations,\n"
            "                  i.e. %*i are \"real\" furcations.\n"
            "\n"
            "Unrooted tree contains %*i edges,\n"
            "              of which %*i have a length > zero.\n"
            "\n"
            "%s\n"
            "\n"
            "%i subtrees have been marked:\n"
            "%s\n"
            "\n",
            node_digits, furcs,
            node_digits, multifurcs,
            node_digits, realfurcs,
            node_digits, edges,
            node_digits, realedges,
            diffs_all,
            absdiff_marked.get_count(),
            diffs_marked);

        free(diffs_all);
        free(diffs_marked);

        return msg;
    }
};

struct DepthMarker {
    // limits (marked if depth and dist are above)
    int    min_depth;
    double min_rootdist;

    // current values (for recursion)
    int    depth;
    double dist;

    // results
    ValueCounter<int>    depths,    depths_marked;
    ValueCounter<double> distances, distances_marked;

    void perform_marking(AP_tree *at, AP_FLOAT atLen) {
        int depthInc = atLen == 0.0 ? 0 : 1;  // do NOT increase depth at multifurcations

        depth += depthInc;
        dist  += atLen;

        if (at->is_leaf) {
            depths.count_value(depth);
            distances.count_value(dist);

            int mark = depth >= min_depth && dist >= min_rootdist;
            if (at->gb_node) {
                GB_write_flag(at->gb_node, mark);
                if (mark) {
                    depths_marked.count_value(depth);
                    distances_marked.count_value(dist);
                }
            }
        }
        else {
            perform_marking(at->get_leftson(), at->leftlen);
            perform_marking(at->get_rightson(), at->rightlen);
        }

        depth -= depthInc;
        dist  -= atLen;
    }

public:
    DepthMarker(AP_tree *root, int min_depth_, double min_rootdist_)
        : min_depth(min_depth_),
          min_rootdist(min_rootdist_),
          depth(0),
          dist(0.0)
    {
        perform_marking(root, 0.0);
    }

    const char *get_report() const {
        int    leafs          = depths.get_count();
        int    marked         = depths_marked.get_count();
        double balanced_depth = log10(leafs) / log10(2);

        const char *msg = GBS_global_string(
            "The optimal mean depth of a tree with %i leafs\n"
            "      would be %.2f\n"
            "\n"
            "Your tree:\n"
            "mean depth:    %s\n"
            "mean distance: %s\n"
            "\n"
            "%i species (%.2f%%) have been marked:\n"
            "mean depth:    %s\n"
            "mean distance: %s\n"
            ,
            leafs,
            balanced_depth,
            depths.mean_min_max(),
            distances.mean_min_max(), 
            marked, marked/double(leafs)*100.0,
            depths_marked.mean_min_max(), 
            distances_marked.mean_min_max()
            );
        return msg;
    }
};

const char *AP_tree::mark_long_branches(double min_rel_diff, double min_abs_diff) {
    // look for asymmetric parts of the tree and mark all species with long branches
    return LongBranchMarker(this, min_rel_diff, min_abs_diff).get_report();
}
const char *AP_tree::mark_deep_leafs(int min_depth, double min_rootdist) {
    // mark all leafs with min_depth and min_rootdist
    return DepthMarker(this, min_depth, min_rootdist).get_report();
}

// --------------------------------------------------------------------------------

typedef ValueCounter<double> Distance;

class DistanceCounter {
    Distance min, max, mean;
public:

    void count_distance(const Distance& d) {
        mean.count_value(d.get_mean());
        min.count_value(d.get_min());
        max.count_value(d.get_max());
    }

    int get_count() const { return mean.get_count(); }

    char *get_report() const {
        return GBS_global_string_copy(
            "Mean mean distance: %s\n"
            "Mean min. distance: %s\n"
            "Mean max. distance: %s",
            mean.mean_min_max(), 
            min.mean_min_max(), 
            max.mean_min_max()
            );
    }
};

class EdgeDistances {
    typedef map<AP_tree*, Distance> DistanceMap;

    DistanceMap downdist; // inclusive length of branch itself
    DistanceMap updist;   // inclusive length of branch itself

    GBT_LEN distSum; // of all distances in tree

    arb_progress progress;

    const Distance& calc_downdist(AP_tree *at, AP_FLOAT len) {
        if (at->is_leaf) {
            Distance d;
            d.count_value(len);
            downdist[at] = d;

            progress.inc();
        }
        else {
            downdist[at] =
                calc_downdist(at->get_leftson(), at->leftlen) +
                calc_downdist(at->get_rightson(), at->rightlen) +
                len;
        }
        return downdist[at];
    }

    const Distance& calc_updist(AP_tree *at, AP_FLOAT len) {
        ap_assert(at->father); // impossible - root has no updist!

        AP_tree *father  = at->get_father();
        AP_tree *brother = at->get_brother();

        if (father->father) {
            ap_assert(updist.find(father) != updist.end());
            ap_assert(downdist.find(brother) != downdist.end());

            updist[at] = updist[father] + downdist[brother] + len;
        }
        else {
            ap_assert(downdist.find(brother) != downdist.end());

            updist[at] = downdist[brother]+len;
        }

        if (!at->is_leaf) {
            calc_updist(at->get_leftson(), at->leftlen);
            calc_updist(at->get_rightson(), at->rightlen);
        }
        else {
            progress.inc();
        }
        
        return updist[at];
    }

    DistanceCounter alldists, markeddists;

    void calc_distance_stats(AP_tree *at) {
        if (at->is_leaf) {
            ap_assert(updist.find(at) != updist.end());

            const Distance& upwards = updist[at];

            alldists.count_distance(upwards);
            if (at->gb_node && GB_read_flag(at->gb_node)) {
                markeddists.count_distance(upwards);
            }

            progress.inc();
        }
        else {
            calc_distance_stats(at->get_leftson());
            calc_distance_stats(at->get_rightson());
        }
    }

public:

    EdgeDistances(AP_tree *root)
        : distSum(root->sum_child_lengths()),
          progress("Analysing distances", root->count_leafs()*3)
    {
        calc_downdist(root->get_leftson(),  root->leftlen);
        calc_downdist(root->get_rightson(), root->rightlen);

        calc_updist(root->get_leftson(),  root->leftlen);
        calc_updist(root->get_rightson(), root->rightlen);

        calc_distance_stats(root);
    }

    const char *get_report() const {
        char *alldists_report    = alldists.get_report();
        char *markeddists_report = markeddists.get_report();

        const char *msg = GBS_global_string(
            "Overall in-tree-distance (ITD): %.3f\n"
            "    per-species-distance (PSD): %.6f\n"
            "\n"
            "Distance statistic for %i leafs:\n"
            "(each leaf to all other leafs)\n"
            "\n"
            "%s\n"
            "\n"
            "Distance statistic for %i marked leafs:\n"
            "\n"
            "%s\n",
            distSum,
            distSum / alldists.get_count(),
            alldists.get_count(), alldists_report,
            markeddists.get_count(), markeddists_report);

        free(markeddists_report);
        free(alldists_report);

        return msg;
    }
};

const char *AP_tree::analyse_distances() { return EdgeDistances(this).get_report(); }

// --------------------------------------------------------------------------------

static int ap_mark_degenerated(AP_tree *at, double degeneration_factor, double& max_degeneration) {
    // returns number of species in subtree

    if (at->is_leaf) return 1;

    int lSons = ap_mark_degenerated(at->get_leftson(), degeneration_factor, max_degeneration);
    int rSons = ap_mark_degenerated(at->get_rightson(), degeneration_factor, max_degeneration);

    double this_degeneration = 0;

    if (lSons<rSons) {
        this_degeneration = rSons/double(lSons);
        if (this_degeneration >= degeneration_factor) {
            at->get_leftson()->mark_subtree();
        }

    }
    else if (rSons<lSons) {
        this_degeneration = lSons/double(rSons);
        if (this_degeneration >= degeneration_factor) {
            at->get_rightson()->mark_subtree();
        }
    }

    if (this_degeneration >= max_degeneration) {
        max_degeneration = this_degeneration;
    }

    return lSons+rSons;
}

const char *AP_tree::mark_degenerated_branches(double degeneration_factor) {
    // marks all species in degenerated branches.
    // For all nodes, where one branch contains 'degeneration_factor' more species than the
    // other branch, the smaller branch is considered degenerated.

    double max_degeneration = 0;
    ap_mark_degenerated(this, degeneration_factor, max_degeneration);
    return GBS_global_string("Maximum degeneration = %.2f", max_degeneration);
}

static int ap_mark_duplicates_rek(AP_tree *at, GB_HASH *seen_species) {
    if (at->is_leaf) {
        if (at->name) {
            if (GBS_read_hash(seen_species, at->name)) { // already seen -> mark species
                if (at->gb_node) {
                    GB_write_flag(at->gb_node, 1);
                }
                else { // duplicated zombie
                    return 1;
                }
            }
            else { // first occurrence
                GBS_write_hash(seen_species, at->name, 1);
            }
        }
    }
    else {
        return
            ap_mark_duplicates_rek(at->get_leftson(), seen_species) +
            ap_mark_duplicates_rek(at->get_rightson(), seen_species);
    }
    return 0;
}

void AP_tree::mark_duplicates() {
    GB_HASH *seen_species = GBS_create_hash(gr.leaf_sum, GB_IGNORE_CASE);

    int dup_zombies = ap_mark_duplicates_rek(this, seen_species);
    if (dup_zombies) {
        aw_message(GBS_global_string("Warning: Detected %i duplicated zombies (can't mark them)", dup_zombies));
    }
    GBS_free_hash(seen_species);
}

static double ap_just_tree_rek(AP_tree *at) {
    if (at->is_leaf) {
        return 0.0;
    }
    else {
        double bl = ap_just_tree_rek(at->get_leftson());
        double br = ap_just_tree_rek(at->get_rightson());

        double l = at->leftlen + at->rightlen;
        double diff = fabs(bl - br);
        if (l < diff * 1.1) l = diff * 1.1;
        double go = (bl + br + l) * .5;
        at->leftlen = go - bl;
        at->rightlen = go - br;
        return go;
    }
}


void AP_tree::justify_branch_lenghs(GBDATA *gb_main) {
    // shift branches to create a symmetric looking tree
    GB_transaction ta(gb_main);
    ap_just_tree_rek(this);
}

static void relink_tree_rek(AP_tree *node, void (*relinker)(GBDATA *&ref_gb_node, char *&ref_name, GB_HASH *organism_hash), GB_HASH *organism_hash) {
    if (node->is_leaf) {
        relinker(node->gb_node, node->name, organism_hash);
    }
    else {
        relink_tree_rek(node->get_leftson(), relinker, organism_hash);
        relink_tree_rek(node->get_rightson(), relinker, organism_hash);
    }
}

void AP_tree::relink_tree(GBDATA *gb_main, void (*relinker)(GBDATA *&ref_gb_node, char *&ref_name, GB_HASH *organism_hash), GB_HASH *organism_hash) {
    // relinks the tree using a relinker-function
    // every node in tree is passed to relinker, relinker might modify
    // these values (ref_gb_node and ref_name) and the modified values are written back into tree

    GB_transaction  ta(gb_main);
    relink_tree_rek(this, relinker, organism_hash);
}


void AP_tree::reset_child_angles() {
    if (!is_leaf) {
        gr.reset_both_child_angles();
        get_leftson()->reset_child_angles();
        get_rightson()->reset_child_angles();
    }
}

void AP_tree::reset_child_linewidths() {
    if (!is_leaf) {
        gr.reset_both_child_linewidths();
        get_leftson()->reset_child_linewidths();
        get_rightson()->reset_child_linewidths();
    }
}

void AP_tree::set_linewidth_recursive(int width) {
    set_linewidth(width);
    if (!is_leaf) {
        get_leftson()->set_linewidth_recursive(width);
        get_rightson()->set_linewidth_recursive(width);
    }
}

void AP_tree::reset_child_layout() {
    if (!is_leaf) {
        gr.reset_child_spread();
        gr.reset_both_child_angles();
        gr.reset_both_child_linewidths();
        get_leftson()->reset_child_layout();
        get_rightson()->reset_child_layout();
    }
}

void AP_tree::reset_subtree_spreads() {
    gr.reset_child_spread();
    if (!is_leaf) {
        get_leftson()->reset_subtree_spreads();
        get_rightson()->reset_subtree_spreads();
    }
}
void AP_tree::reset_subtree_angles() {
    reset_angle();
    if (!is_leaf) reset_child_angles();
}
void AP_tree::reset_subtree_linewidths() {
    reset_linewidth();
    if (!is_leaf) reset_child_linewidths();
}
void AP_tree::reset_subtree_layout() {
    reset_linewidth();
    reset_angle();
    if (!is_leaf) reset_child_layout();
}

bool AP_tree::is_inside_folded_group() const {
    if (gr.grouped) return true;
    if (!father) return false;
    return get_father()->is_inside_folded_group();
}

