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

#include <math.h>
#include <map>

using namespace std;

/*****************************************************************************************
 ************           Rates                       **********
 *****************************************************************************************/
void AP_rates::print()
{
    AP_FLOAT max;
    int i;

    max = 0.0;
    for (i=0;i<rate_len; i++) {
        if (rates[i] > max) max = rates[i];
    }
    printf("rates:");
    for (i=0;i<rate_len; i++) {
        putchar('0' + (int)(rates[i]/max*9.9));
    }
    printf("\n");
}

AP_rates::AP_rates() {
    memset ((char *)this,0,sizeof(AP_rates));
}

char *AP_rates::init(AP_filter *fil)
{
    int i;
    if (fil->get_timestamp() <= this->update) return 0;

    rate_len = fil->get_filtered_length();
    delete rates;
    rates = new AP_FLOAT[rate_len];
    for (i=0;i<rate_len;i++) {
        rates[i] = 1.0;
    }
    this->update = fil->get_timestamp();
    return 0;
}

char *AP_rates::init(AP_FLOAT * ra, AP_filter *fil)
{
    int i,j;
    if (fil->get_timestamp() <= this->update) return 0;

    rate_len = fil->get_filtered_length();
    delete rates;
    rates = new AP_FLOAT[rate_len];
    for (j=i=0;i<rate_len;j++) {
        if (fil->use_position(j)) {
            rates[i++] = ra[j];
        }
    }
    this->update = fil->get_timestamp();
    return 0;
}

AP_rates::~AP_rates()   { if (rates) delete(rates);}


/*****************************************************************************************
 ************           AP_tree_root                    **********
 *****************************************************************************************/

AP_tree_root::AP_tree_root(AliView *aliView, const AP_tree& tree_proto, AP_sequence *seq_proto, bool add_delete_callbacks)
    : ARB_tree_root(aliView, tree_proto, seq_proto, add_delete_callbacks)
    , root_changed_cb(NULL), root_changed_cd(NULL)
    , node_deleted_cb(NULL), node_deleted_cd(NULL)
    , gb_tree_gone(NULL)
    , tree_timer(NULL)
    , species_timer(NULL)
    , table_timer(NULL)
    , rates(NULL)
{
    GBDATA         *gb_main = get_gb_main();
    GB_transaction  ta(gb_main);

    gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    gb_table_data   = GB_search(gb_main, "table_data", GB_CREATE_CONTAINER);
}

AP_tree_root::~AP_tree_root() {
    delete get_root_node();
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
        GB_transaction  dummy(GB_get_root(gb_species_data));
        GBDATA         *gbtree = get_gb_tree();
        if (gbtree) tree_timer = GB_read_clock(gbtree);
        species_timer          = GB_read_clock(gb_species_data);
        table_timer            = GB_read_clock(gb_table_data);
    }
}

/*****************************************************************************************
 ************           AP_tree                     **********
 *****************************************************************************************/
static void ap_tree_node_deleted(GBDATA *, int *cl, GB_CB_TYPE){
    AP_tree *THIS = (AP_tree *)cl;
    THIS->gb_node = 0;
}

AP_tree::AP_tree(AP_tree_root *tree_rooti)
    : ARB_tree(tree_rooti)
    , stack_level(0)
{
    gr.clear();
    gr.spread = 1.0;
    br.clear();
}

AP_tree::~AP_tree() {
    if (gr.callback_exists && gb_node){
        GB_remove_callback(gb_node,GB_CB_DELETE,ap_tree_node_deleted,(int *)this);
    }

    AP_tree_root *root = get_tree_root();
    if (root) root->inform_about_delete(this);
}

AP_tree *AP_tree::dup() const {
    return new AP_tree(const_cast<AP_tree_root*>(get_tree_root()));
}

#if defined(DEVEL_RALF)
#warning move to ARB_tree ? 
#endif // DEVEL_RALF
void AP_tree::replace_self(AP_tree *new_son) {
    ap_assert(father);
    if (father) {
        if (is_leftson(father)) father->leftson  = new_son;
        else                    father->rightson = new_son;
    }
}
#if defined(DEVEL_RALF)
#warning move to ARB_tree ? 
#endif // DEVEL_RALF
void AP_tree::set_brother(AP_tree *new_son) {
    ap_assert(father);
    if (father) {
        if (is_leftson(father)) father->rightson = new_son;
        else                    father->leftson  = new_son;
    }
}

void AP_tree::clear_branch_flags() {
    br.clear();
    if (!is_leaf) {
        get_leftson()->clear_branch_flags();
        get_rightson()->clear_branch_flags();
    }
}

void AP_tree::insert(AP_tree *new_brother) {
    ASSERT_VALID_TREE(this);
    ASSERT_VALID_TREE(new_brother);

    AP_tree  *new_tree = dup();
    AP_FLOAT  laenge;

    // ap_assert(new_brother->get_tree_root()); // brother has to be inside a tree!

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
    if (troot) { 
        if (!new_tree->father) troot->change_root(new_brother, new_tree);
        else new_tree->set_tree_root(troot);

        ASSERT_VALID_TREE(troot->get_root_node());
    }
    else { // loose nodes (not in a tree)
        ASSERT_VALID_TREE(new_tree);
    }
}

#if defined(DEVEL_RALF)
#warning move to ARB_tree ? 
#endif // DEVEL_RALF
void AP_tree_root::change_root(AP_tree *oldroot, AP_tree *newroot) {
    if (root_changed_cb) root_changed_cb(root_changed_cd, oldroot, newroot);
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
    ARB_tree_root::change_root(oldroot, newroot);
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
        get_tree_root()->change_root(this, NULL); // wot ?
    }
    else {
        AP_tree      *brother     = get_brother();  // brother remains in tree
        GBT_LEN       brothersLen = brother->get_branchlength();
        AP_tree      *fath        = get_father();   // fath of this is removed as well
        ARB_tree     *grandfather = fath->father;
        AP_tree_root *troot       = get_tree_root();

        if (fath->gb_node) {                      // move inner information to remaining subtree
            if (!brother->gb_node && !brother->is_leaf) {
                brother->gb_node = fath->gb_node;
                fath->gb_node    = 0;
            }
        }

        if (grandfather) {
            brother->unlink_from_father();

            bool isLeftSon = fath->is_leftson(grandfather);
            fath->unlink_from_father();

            if (isLeftSon) {
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

#if defined(CHECK_TREE_STRUCTURE)
        if (troot->get_root_node()) ASSERT_VALID_TREE(troot->get_root_node());
#endif // CHECK_TREE_STRUCTURE
        
        troot->inform_about_delete(fath);
        troot->inform_about_delete(this);
    }
}

GB_ERROR AP_tree::cantMoveTo(AP_tree *new_brother) {
    GB_ERROR error = 0;

    if (!father)                                error = "Can't move the root of the tree";
    else if (!new_brother->father)              error = "Can't move to the root of the tree";
    else if (new_brother->father == father)     error = "Already there";
    else if (!father->father)                   error = "Can't move son of root";
    else if (new_brother->is_inside(this))      error = "Can't move a subtree into itself";

    return error;
}

void AP_tree::moveTo(AP_tree *new_brother, AP_FLOAT rel_pos) {
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
        ARB_tree *grandfather = father->father;
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

    ARB_tree *new_tree          = father;
    ARB_tree *brother_father    = new_brother->father;
    AP_FLOAT  laenge;

    if (brother_father->leftson == new_brother) {
        laenge  = brother_father->leftlen;
        laenge -= brother_father->leftlen = laenge * rel_pos ;
        new_brother->father->leftson = new_tree;
    }
    else {
        laenge  = brother_father->rightlen;
        laenge -= brother_father->rightlen = laenge * rel_pos ;
        brother_father->rightson = new_tree;
    }
    
    new_tree->rightlen  = laenge;
    new_brother->father = new_tree;
    new_tree->rightson  = new_brother;
    new_tree->father    = brother_father;
}

void AP_tree::swap_sons() {
    ARB_tree *h_at = this->leftson;
    this->leftson = this->rightson;
    this->rightson = h_at;

    double h = this->leftlen;
    this->leftlen = this->rightlen;
    this->rightlen = h;
}

void AP_tree::swap_assymetric(AP_TREE_SIDE mode){
    // mode AP_LEFT exchanges lefts with brother
    // mode AP_RIGHT exchanges rights with brother

    ap_assert(!is_leaf);                           // cannot swap leafs
    ap_assert(father);                             // cannot swap root (has no brother)
    ap_assert(mode == AP_LEFT || mode == AP_RIGHT); // illegal mode

    ARB_tree *pntr;

    if (father->father == 0) { // father is root
        AP_tree *pntr_brother = get_brother();
        if (!pntr_brother->is_leaf) {
            if (mode == AP_LEFT) {
                pntr_brother->leftson->father = this;
                pntr                          = pntr_brother->leftson;
                pntr_brother->leftson         = leftson;
                leftson->father               = pntr_brother;
                leftson                       = pntr;
            }
            else {
                pntr_brother->leftson->father = this;
                rightson->father              = pntr_brother;
                pntr                          = pntr_brother->leftson;
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
                pntr                     = father->rightson;
                AP_FLOAT help_len        = father->rightlen;
                father->rightlen         = leftlen;
                leftlen                  = help_len;
                father->rightson         = leftson;
                leftson                  = pntr;
            }
            else {
                father->leftson->father = this;
                leftson->father         = father;
                pntr                    = father->leftson;
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
                pntr                     = father->rightson;
                AP_FLOAT help_len        = father->rightlen;
                father->rightlen         = rightlen;
                rightlen                 = help_len;
                father->rightson         = rightson;
                rightson                 = pntr;
            }
            else {
                father->leftson->father = this;
                rightson->father        = father;
                pntr                    = father->leftson;
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
    if (!father) return;                            // already root
    if (!father->father) return;                    // already root?

    AP_tree *old_root    = 0;
    AP_tree *old_brother = 0;
    {
        AP_branch_members  br1 = br;
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
        old_root = pntr;
    }
    if (old_brother) old_brother = old_brother->get_brother();

    {
        // move remark branches to top
        ARB_tree *node;
        char     *remark = nulldup(remark_branch);

        for (node = this; node->father; node = node->father) {
            char *sh            = node->remark_branch;
            node->remark_branch = remark;
            remark              = sh;
        }
        delete remark;
    }
    AP_FLOAT old_root_len = old_root->leftlen + old_root->rightlen;

    //new node & this init

    old_root->leftson  = this;
    old_root->rightson = father;                    // will be set later

    if (father->leftson == this) {
        old_root->leftlen = old_root->rightlen = father->leftlen*.5;
    }
    else {
        old_root->leftlen = old_root->rightlen = father->rightlen*.5;
    }

    ARB_tree *next = father->father;
    ARB_tree *prev = old_root;
    ARB_tree *pntr = father;

    if (father->leftson == this)    father->leftson = old_root; // to set the flag correctly

    // loop from father to son of root, rotate tree
    while  (next->father) {
        double len = (next->leftson == pntr) ? next->leftlen : next->rightlen;

        if (pntr->leftson == prev) {
            pntr->leftson = next;
            pntr->leftlen = len;
        }
        else {
            pntr->rightson = next;
            pntr->rightlen = len;
        }
        
        pntr->father = prev;
        prev         = pntr;
        pntr         = next;
        next         = next->father;
    }
    // now next points to the old root, which has been destroyed above
    //
    // pointer at oldroot
    // pntr == brother before old root == next

    if (pntr->leftson == prev) {
        pntr->leftlen = old_root_len;
        pntr->leftson = old_brother;
    }
    else {
        pntr->rightlen = old_root_len;
        pntr->rightson = old_brother;
    }

    old_brother->father = pntr;
    pntr->father        = prev;
    father              = old_root;
}


inline short tree_read_byte(GBDATA *tree,const char *key, int init) {
    GBDATA *gbd;
    if (!tree) return init;
    gbd = GB_entry(tree,key);
    if (!gbd) return init;
    return (short)GB_read_byte(gbd);
}

inline float tree_read_float(GBDATA *tree,const char *key, double init) {
    GBDATA *gbd;
    if (!tree) return (float)init;
    gbd = GB_entry(tree,key);
    if (!gbd) return (float)init;
    return (float)GB_read_float(gbd);
}



/** moves all node/leaf information from struct GBT_TREE to AP_tree */
void AP_tree::load_node_info() {
    gr.spread          = tree_read_float(gb_node, "spread",          1.0);
    gr.left_angle      = tree_read_float(gb_node, "left_angle",      0.0);
    gr.right_angle     = tree_read_float(gb_node, "right_angle",     0.0);
    gr.left_linewidth  = tree_read_byte (gb_node, "left_linewidth",  0);
    gr.right_linewidth = tree_read_byte (gb_node, "right_linewidth", 0);
    gr.grouped         = tree_read_byte (gb_node, "grouped",         0);
}

void AP_tree::move_gbt_info(GBT_TREE *tree) {
    ARB_tree::move_gbt_info(tree);
    load_node_info();
}



#if defined(DEBUG)
#if defined(DEVEL_RALF)
#define DEBUG_tree_write_byte
#endif // DEVEL_RALF
#endif // DEBUG


static GB_ERROR tree_write_byte(GBDATA *gb_tree, AP_tree *node,short i,const char *key, int init) {
    GBDATA *gbd;
    GB_ERROR error= 0;
    if (i==init) {
        if (node->gb_node){
            gbd = GB_entry(node->gb_node,key);
            if (gbd) {
#if defined(DEBUG_tree_write_byte)
                printf("[tree_write_byte] deleting db entry %p\n", gbd);
#endif // DEBUG_tree_write_byte
                GB_delete(gbd);
            }
        }
    }else{
        if (!node->gb_node){
            node->gb_node = GB_create_container(gb_tree,"node");
#if defined(DEBUG_tree_write_byte)
            printf("[tree_write_byte] created node-container %p\n", node->gb_node);
#endif // DEBUG_tree_write_byte
        }
        gbd = GB_entry(node->gb_node,key);
        if (!gbd) {
            gbd = GB_create(node->gb_node,key,GB_BYTE);
#if defined(DEBUG_tree_write_byte)
            printf("[tree_write_byte] created db entry %p\n", gbd);
#endif // DEBUG_tree_write_byte
        }
        error = GB_write_byte(gbd,i);
    }
    return error;
}

static GB_ERROR tree_write_float(GBDATA *gb_tree, AP_tree *node,float f,const char *key, float init) {
    GB_ERROR error = NULL;
    if (f==init) {
        if (node->gb_node){
            GBDATA *gbd    = GB_entry(node->gb_node,key);
            if (gbd) error = GB_delete(gbd);
        }
    }
    else {
        if (!node->gb_node){
            node->gb_node             = GB_create_container(gb_tree,"node");
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
    if (!error) error = ARB_tree_root::saveToDB();
    if (!error) update_timers();
    
    return GB_end_transaction(get_gb_main(), error);
}


GB_ERROR AP_tree::move_group_info(AP_tree *new_group) {
    GB_ERROR error = 0;
    if (is_leaf || !name) {
        error = GB_export_error("Please select a valid group");
    }
    else if (!gb_node){
        error = GB_export_error("Internal Error: group with name is missing DB-entry");
    }
    else if (new_group->is_leaf) {
        if (new_group->name) {
            error = GB_export_errorf("'%s' is not a valid target for group information of '%s'.", new_group->name, name);
        }
        else if (new_group->gb_node) {
            error = GB_export_error("Internal Error: Target node already has a database entry (but no name)");
        }
    }

    if (!error) {
        if (new_group->name) {
            if (!new_group->gb_node) {
                error = GB_export_error("Internal Error: Target node has a database entry (but no name)");
            }
            else { /* exchange information of two groups */
                GBDATA *tmp_node   = new_group->gb_node;
                char   *tmp_name   = new_group->name;
                new_group->gb_node = gb_node;
                new_group->name    = name;
                name               = tmp_name;
                gb_node            = tmp_node;
            }
        }
        else { /* move group info */
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
    GB_transaction dummy(get_tree_root()->get_gb_main());
    get_tree_root()->update_timers();
}

GBT_LEN AP_tree::arb_tree_deep()
{
    GBT_LEN l,r;
    if (is_leaf) return 0.0;
    l = leftlen + get_leftson()->arb_tree_deep();
    r = rightlen + get_rightson()->arb_tree_deep();
    if (l<r) l=r;
    gr.tree_depth = l;
    return l;
}

GBT_LEN AP_tree::arb_tree_min_deep()
{
    GBT_LEN l,r;
    if (is_leaf) return 0.0;
    l = leftlen + get_leftson()->arb_tree_min_deep();
    r = rightlen + get_rightson()->arb_tree_min_deep();
    if (l>r) l=r;
    gr.min_tree_depth = l;
    return l;
}

int AP_tree::arb_tree_set_leafsum_viewsum() // count all visible leafs
{
    int l,r;
    if (is_leaf) {
        gr.view_sum = 1;
        gr.leave_sum = 1;
        return 1;
    }
    l =  get_leftson()->arb_tree_set_leafsum_viewsum();
    r =  get_rightson()->arb_tree_set_leafsum_viewsum();
    gr.leave_sum = r+l;
    gr.view_sum = get_leftson()->gr.view_sum + get_rightson()->gr.view_sum;
    if (gr.grouped) {
        gr.view_sum = (int)pow((double)(gr.leave_sum - GROUPED_SUM + 9),.33);
    }
    return gr.leave_sum;
}

int AP_tree::arb_tree_leafsum2()    // count all leafs
{
    if (is_leaf) return 1;
    return get_leftson()->arb_tree_leafsum2() + get_rightson()->arb_tree_leafsum2();
}

void AP_tree::calc_hidden_flag(int father_is_hidden){
    gr.hidden = father_is_hidden;
    if (!is_leaf) {
        if (gr.grouped) father_is_hidden = 1;
        get_leftson()->calc_hidden_flag(father_is_hidden);
        get_rightson()->calc_hidden_flag(father_is_hidden);
    }
}

int AP_tree::calc_color() {
    int l,r;
    int res;
    if (is_leaf) {
        if (gb_node) {
            if (GB_read_flag(gb_node)) {
                res = AWT_GC_SELECTED;
            }
            else {
                // check for user color
                long color_group = AWT_species_get_dominant_color(gb_node);
                // long color_group = AW_find_color_group(gb_node);
                if (color_group == 0) {
                    res = AWT_GC_NSELECTED;
                }
                else {
                    res = AWT_GC_FIRST_COLOR_GROUP+color_group-1;
                }
            }
        }
        else {
            res = AWT_GC_SOME_MISMATCHES;
        }
    }
    else {
        l = get_leftson()->calc_color();
        r = get_rightson()->calc_color();

        if ( l == r) res = l;

        else if ( l == AWT_GC_SELECTED && r != AWT_GC_SELECTED) res = AWT_GC_UNDIFF;
        else if ( l != AWT_GC_SELECTED && r == AWT_GC_SELECTED) res = AWT_GC_UNDIFF;

        else if ( l == AWT_GC_SOME_MISMATCHES) res = r;
        else if ( r == AWT_GC_SOME_MISMATCHES) res = l;

        else if ( l == AWT_GC_UNDIFF || r == AWT_GC_UNDIFF) res = AWT_GC_UNDIFF;

        else {
            ap_assert(l != AWT_GC_SELECTED && r != AWT_GC_SELECTED);
            ap_assert(l != AWT_GC_UNDIFF && r != AWT_GC_UNDIFF);
            res = AWT_GC_NSELECTED; // was : res = AWT_GC_UNDIFF;
        }
    }

    gr.gc = res;
    if (res == AWT_GC_NSELECTED){
        gr.has_marked_children = 0;
    }else{
        gr.has_marked_children = 1;
    }
    return res;
}

//Diese Funktion nimmt eine Hashtabelle mit Bakteriennamen und
//faerbt Bakterien die darin vorkommen mit den entsprechenden Farben
//in der Hashtabelle ist eine Struktur aus Bak.namen und Farben(GC's)
int AP_tree::calc_color_probes(GB_HASH *hashptr) {
    int l,r;
    int res;

    if (is_leaf) {
        if (gb_node) {
            res = GBS_read_hash(hashptr, name);
            if (!res && GB_read_flag(gb_node)) { // marked but not in hash -> black
                res = AWT_GC_BLACK;
            }
        }
        else {
            res = AWT_GC_SOME_MISMATCHES;
        }
    }else{
        l = get_leftson()->calc_color_probes(hashptr);
        r = get_rightson()->calc_color_probes(hashptr);
        
        if      (l == r)                      res = l;
        else if (l == AWT_GC_SOME_MISMATCHES) res = r;
        else if (r == AWT_GC_SOME_MISMATCHES) res = l;
        else                                  res = AWT_GC_UNDIFF;
    }
    gr.gc = res;
    return res;
}

int AP_tree::compute_tree(GBDATA *gb_main)
{
    GB_transaction dummy(gb_main);
    arb_tree_deep();
    arb_tree_min_deep();
    arb_tree_set_leafsum_viewsum();
    calc_color();
    calc_hidden_flag(0);
    return 0;
}

GB_ERROR AP_tree_root::loadFromDB(const char *name) {
    GB_ERROR error = ARB_tree_root::loadFromDB(name);
    update_timers();                                // maybe after link() ?
    return error;
}

GB_ERROR AP_tree::relink() {
    GB_transaction dummy(get_tree_root()->get_gb_main()); // open close a transaction
    GB_ERROR error = GBT_link_tree(get_gbt_tree(), get_tree_root()->get_gb_main(), GB_FALSE, 0, 0); // no status
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
        GB_transaction dummy(gb_main);

        if (troot->is_tree_updated()) return AP_UPDATE_RELOADED;
        if (troot->is_species_updated()) return AP_UPDATE_RELINKED;
        return AP_UPDATE_OK;
    }
}

#if defined(DEVEL_RALF)
#warning buildLeafList, buildNodeList and buildBranchList should return a AP_tree_list (new class!)
#endif // DEVEL_RALF

void AP_tree::buildLeafList_rek(AP_tree **list,long& num) {
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
    num        = arb_tree_leafsum2();
    list       = new AP_tree *[num+1];
    list[num]  = 0;
    long count = 0;

    buildLeafList_rek(list,count);

    ap_assert(count == num);
}

void AP_tree::buildNodeList_rek(AP_tree **list, long& num) {
    // builds a list of all inner nodes (w/o root node)
    if (!is_leaf) {
        if (father) list[num++] = this;
        get_leftson()->buildNodeList_rek(list,num);
        get_rightson()->buildNodeList_rek(list,num);
    }
}

void AP_tree::buildNodeList(AP_tree **&list, long &num) {
    num = this->arb_tree_leafsum2()-1;
    list = new AP_tree *[num+1];
    list[num] = 0;
    num  = 0;
    buildNodeList_rek(list,num);
}

void AP_tree::buildBranchList_rek(AP_tree **list,long& num, bool create_terminal_branches, int deep) {
    // builds a list of all species
    // (returns pairs of leafs/father and nodes/father)
    
    if (deep) {
        if (father && (create_terminal_branches || !is_leaf) ) {
            if (father->father){
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
        for (int i=0; i<deep; i++) num *=2;
    }
    else {
        num = arb_tree_leafsum2() * (create_terminal_branches ? 2 : 1);
    }

    ap_assert(num >= 0);
    // if (num<0) num = 0;

    list = new AP_tree *[num*2+4];

    if (num) {
        long count = 0;
        
        buildBranchList_rek(list,count,create_terminal_branches,deep);
        list[count] = 0;
        num         = count/2;
    }
}


void AP_tree_root::remove_leafs(int awt_remove_type) {
    // may remove the complete tree (if awt_remove_type does not contain AWT_REMOVE_BUT_DONT_FREE)
    
    ASSERT_VALID_TREE(get_root_node());

    AP_tree **list;
    long      count;
    get_root_node()->buildLeafList(list,count);

    long           i;
    GB_transaction ta(get_gb_main());

    for (i=0; i<count; i++) {
        bool     removeNode = false;
        AP_tree *leaf       = list[i];

        if (leaf->gb_node) {
            if ((awt_remove_type & AWT_REMOVE_NO_SEQUENCE) && !leaf->get_seq()) {
                removeNode = true;
            }
            else if (awt_remove_type & (AWT_REMOVE_MARKED|AWT_REMOVE_NOT_MARKED)) {
                long flag  = GB_read_flag(list[i]->gb_node);
                removeNode = (flag && (awt_remove_type&AWT_REMOVE_MARKED)) || (!flag && (awt_remove_type&AWT_REMOVE_NOT_MARKED));
            }
        }
        else {
            if (awt_remove_type & AWT_REMOVE_DELETED) {
                removeNode = true;
            }
        }

        if (removeNode) {
            list[i]->remove();
            if (!(awt_remove_type & AWT_REMOVE_BUT_DONT_FREE)){
                delete list[i]->father;
            }
            if (!get_root_node()) {
                break; // tree has been deleted
            }
        }
        ASSERT_VALID_TREE(get_root_node());
    }
    delete [] list;

#if defined(CHECK_TREE_STRUCTURE)
    if (get_root_node()) ASSERT_VALID_TREE(get_root_node());
#endif // CHECK_TREE_STRUCTURE
}

// ----------------------------
//      find_innermost_edge

class NodeLeafDistance {
    AP_FLOAT  downdist, updist;
    enum { NLD_NODIST = 0, NLD_DOWNDIST, NLD_BOTHDIST } state;

public:

    NodeLeafDistance()
        : downdist(-1.0)
        , updist(-1.0)
        , state(NLD_NODIST)
    {}

    AP_FLOAT get_downdist() const { ap_assert(state >= NLD_DOWNDIST); return downdist; }
    void set_downdist(AP_FLOAT DownDist) {
        if (state < NLD_DOWNDIST) state = NLD_DOWNDIST;
        downdist = DownDist;
    }
    
    AP_FLOAT get_updist() const { ap_assert(state >= NLD_BOTHDIST); return updist; }
    void set_updist(AP_FLOAT UpDist) {
        if (state < NLD_BOTHDIST) state = NLD_BOTHDIST;
        updist = UpDist;
    }

};

class EdgeFinder {
    map<AP_tree*, NodeLeafDistance> data;
    
    ARB_edge innermost;
    AP_FLOAT min_maxDist;

    void insert_tree(AP_tree *node) {
        if (node->is_leaf) {
            data[node].set_downdist(0.0);
        }
        else {
            insert_tree(node->get_leftson());
            insert_tree(node->get_rightson());

            data[node].set_downdist(max(data[node->get_leftson()].get_downdist()+node->leftlen,
                                        data[node->get_rightson()].get_downdist()+node->rightlen));
        }
    }

    void findBetterEdge_sub(AP_tree *node) {
        AP_tree *father  = node->get_father();
        AP_tree *brother = node->get_brother();

        AP_FLOAT len      = node->get_branchlength();
        AP_FLOAT brothLen = brother->get_branchlength();

        AP_FLOAT upDist   = max(data[father].get_updist(), data[brother].get_downdist()+brothLen);
        AP_FLOAT downDist = data[node].get_downdist();

        AP_FLOAT edgedist = max(upDist, downDist)+len/2;

        if (edgedist<min_maxDist) { // found better edge
            innermost   = ARB_edge(node, father);
            min_maxDist = edgedist;
        }

        data[node].set_updist(upDist+len);

        if (!node->is_leaf) {
            findBetterEdge_sub(node->get_leftson());
            findBetterEdge_sub(node->get_rightson());
        }
    }

    void findBetterEdge(AP_tree *node) {
        if (!node->is_leaf) {
            findBetterEdge_sub(node->get_leftson());
            findBetterEdge_sub(node->get_rightson());
        }
    }

public:
    EdgeFinder(AP_tree *rootNode)
        : innermost(rootNode->get_leftson(), rootNode->get_rightson()) // root-edge
    {
        insert_tree(rootNode);

        AP_tree *lson = rootNode->get_leftson();
        AP_tree *rson = rootNode->get_rightson();

        AP_FLOAT rootEdgeLen = rootNode->leftlen + rootNode->rightlen;

        AP_FLOAT lddist = data[lson].get_downdist();
        AP_FLOAT rddist = data[rson].get_downdist();

        data[lson].set_updist(rddist+rootEdgeLen);
        data[rson].set_updist(lddist+rootEdgeLen);

        min_maxDist = max(lddist, rddist)+rootEdgeLen/2;

        findBetterEdge(lson);
        findBetterEdge(rson);
    }

    const ARB_edge& innermost_edge() const { return innermost; }
};

ARB_edge AP_tree_root::find_innermost_edge() {
    EdgeFinder edgeFinder(get_root_node());
    return edgeFinder.innermost_edge();
}

// ----------------------------------------

void AP_tree::remove_bootstrap() {
    freenull(remark_branch);
    if (!is_leaf) {
        get_leftson()->remove_bootstrap();
        get_rightson()->remove_bootstrap();
    }
}
void AP_tree::reset_branchlengths() {
    if (!is_leaf) {
        leftlen = rightlen = 0.1;

        get_leftson()->reset_branchlengths();
        get_rightson()->reset_branchlengths();
    }
}

void AP_tree::scale_branchlengths(double factor) {
    if (!is_leaf) {
        leftlen  *= factor;
        rightlen *= factor;

        get_leftson()->scale_branchlengths(factor);
        get_rightson()->scale_branchlengths(factor);
    }
}

void AP_tree::bootstrap2branchlen() { // copy bootstraps to branchlengths
    if (is_leaf) {
        set_branchlength(0.1);
    }
    else {
        if (remark_branch && father) {
            int    bootstrap = atoi(remark_branch);
            double len       = bootstrap/100.0;
            set_branchlength(len);
        }
        get_leftson()->bootstrap2branchlen();
        get_rightson()->bootstrap2branchlen();
    }
}

void AP_tree::branchlen2bootstrap() {               // copy branchlengths to bootstraps
    freenull(remark_branch);
    if (!is_leaf) {
        if (!is_root_node()) {
            remark_branch = GBS_global_string_copy("%i%%", int(get_branchlength()*100.0 + .5));
        }

        get_leftson()->branchlen2bootstrap();
        get_rightson()->branchlen2bootstrap();
    }
}


AP_tree ** AP_tree::getRandomNodes(int anzahl) {
    // function returns a random constructed tree
    // root is tree with species (needed to build a list of species)

    AP_tree **list;
    AP_tree **retlist;
    long num;
    long count = 0,i =0;

    long sumnodes;
    if (!anzahl) return 0;
    buildNodeList(list,sumnodes);
    if (!sumnodes) {
        delete list;
        return 0;
    }

    retlist = (AP_tree **)calloc(anzahl,sizeof(AP_tree *));

    i = 0;
    count = sumnodes;
    for  (i=0; i< anzahl; i++) {
        num = GB_random(count);
        
        retlist[i] = list[num]; // export node
        count--;                // exclude node

        list[num]   = list[count];
        list[count] = retlist[i];

        if (count == 0) count = sumnodes; // restart it
    }
    delete list;
    return retlist;
}

static double ap_search_strange_species_rek(AP_tree *at, double min_rel_diff, double min_abs_diff, bool& marked) {
    marked = false;
    if (at->is_leaf) return 0.0;

    bool   max_is_left = true;
    bool   marked_left;
    bool   marked_right;
    double max         = ap_search_strange_species_rek(at->get_leftson(),  min_rel_diff, min_abs_diff, marked_left) + at->leftlen;
    double min         = ap_search_strange_species_rek(at->get_rightson(), min_rel_diff, min_abs_diff, marked_right) + at->rightlen;

    if (max<min) {
        double h = max; max = min; min = h;
        max_is_left = false;
    }

    if ((max-min)>min_abs_diff && max > (min * (1.0 + min_rel_diff))) {
        if (max_is_left) {
            if (!marked_left) {
                at->leftson->mark_subtree();
                marked = true;
            }
        }
        else {
            if (!marked_right) {
                at->rightson->mark_subtree();
                marked = true;
            }
        }
    }
    if (!marked && (marked_left||marked_right)) marked = true;
    return (max + min) *.5;
}

void AP_tree::mark_long_branches(GBDATA *gb_main, double min_rel_diff, double min_abs_diff) {
    // look for asymmetric parts of the tree and mark all species with long branches
    GB_transaction dummy(gb_main);
    bool           marked;
    ap_search_strange_species_rek(this, min_rel_diff, min_abs_diff, marked);
}

static int ap_mark_degenerated(AP_tree *at, double degeneration_factor, double& max_degeneration)  {
    // returns number of species in subtree

    if (at->is_leaf) return 1;

    int lSons = ap_mark_degenerated(at->get_leftson(), degeneration_factor, max_degeneration);
    int rSons = ap_mark_degenerated(at->get_rightson(), degeneration_factor, max_degeneration);

    double this_degeneration = 0;

    if (lSons<rSons) {
        this_degeneration = rSons/double(lSons);
        if (this_degeneration >= degeneration_factor) {
            at->leftson->mark_subtree();
        }

    }
    else if (rSons<lSons) {
        this_degeneration = lSons/double(rSons);
        if (this_degeneration >= degeneration_factor) {
            at->rightson->mark_subtree();
        }
    }

    if (this_degeneration >= max_degeneration) {
        max_degeneration = this_degeneration;
    }

    return lSons+rSons;
}

void AP_tree::mark_degenerated_branches(GBDATA *,double degeneration_factor){
    // marks all species in degenerated branches.
    // For all nodes, where one branch contains 'degeneration_factor' more species than the
    // other branch, the smaller branch is considered degenerated.

    double max_degeneration = 0;
    ap_mark_degenerated(this, degeneration_factor, max_degeneration);
    aw_message(GBS_global_string("Maximum degeneration = %.2f", max_degeneration));
}

static void ap_mark_below_depth(AP_tree *at, int mark_depth, long *marked, long *marked_depthsum) {
    if (at->is_leaf) {
        if (mark_depth <= 0) {
            if (at->gb_node) {
                GB_write_flag(at->gb_node, 1);
                (*marked)++;
                (*marked_depthsum) += mark_depth;
            }
        }
    }
    else {
        ap_mark_below_depth(at->get_leftson(),  mark_depth-1, marked, marked_depthsum);
        ap_mark_below_depth(at->get_rightson(), mark_depth-1, marked, marked_depthsum);
    }
}

static void ap_check_depth(AP_tree *at, int depth, long *depthsum, long *leafs, long *maxDepth) {
    if (at->is_leaf) {
        (*leafs)++;
        (*depthsum) += depth;
        if (depth>*maxDepth) *maxDepth = depth;
    }
    else {
        ap_check_depth(at->get_leftson(),  depth+1, depthsum, leafs, maxDepth);
        ap_check_depth(at->get_rightson(), depth+1, depthsum, leafs, maxDepth);
    }
}

void AP_tree::mark_deep_branches(GBDATA *,int mark_depth){
    // mark all species below mark_depth

    long depthsum  = 0;
    long leafs     = 0;
    long max_depth = 0;

    ap_check_depth(this, 0, &depthsum, &leafs, &max_depth);

    double balanced_depth = log10(leafs) / log10(2);

    long marked_depthsum = 0;
    long marked          = 0;
    ap_mark_below_depth(this, mark_depth, &marked, &marked_depthsum);
    
    marked_depthsum = -marked_depthsum + marked*mark_depth;

    aw_message(GBS_global_string(
                                 "optimal depth would be %.2f\n"
                                 "mean depth = %.2f\n"
                                 "max depth  = %li\n"
                                 "marked species  = %li\n"
                                 "mean depth of marked  = %.2f\n"
                                 ,
                                 balanced_depth,
                                 depthsum/(double)leafs,
                                 max_depth,
                                 marked,
                                 marked_depthsum/(double)marked
                                 ));
}

static int ap_mark_duplicates_rek(AP_tree *at, GB_HASH *seen_species) {
    if (at->is_leaf) {
        if (at->name) {
            if (GBS_read_hash(seen_species, at->name)) { // already seen -> mark species
                if (at->gb_node) {
                    GB_write_flag(at->gb_node,1);
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

void AP_tree::mark_duplicates(GBDATA *gb_main) {
    GB_transaction  ta(gb_main);
    GB_HASH        *seen_species = GBS_create_hash(GBT_get_species_hash_size(gb_main), GB_IGNORE_CASE);

    int dup_zombies = ap_mark_duplicates_rek(this, seen_species);
    if (dup_zombies) {
        aw_message(GBS_global_string("Warning: Detected %i duplicated zombies", dup_zombies));
    }
    GBS_free_hash(seen_species);
}

static double ap_just_tree_rek(AP_tree *at){
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


void AP_tree::justify_branch_lenghs(GBDATA *gb_main){
    // shift branches to create a symmetric looking tree
    //    double max_deep = gr.tree_depth;
    GB_transaction dummy(gb_main);
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

    GB_transaction  dummy(gb_main);
    relink_tree_rek(this, relinker, organism_hash);
}

void AP_tree::reset_spread() {
    gr.spread = 1.0;
    if (!is_leaf) {
        get_leftson()->reset_spread();
        get_rightson()->reset_spread();
    }
}

void AP_tree::reset_rotation() {
    if (!is_leaf) {
        gr.left_angle  = 0.0;
        gr.right_angle = 0.0;
        get_leftson()->reset_rotation();
        get_rightson()->reset_rotation();
    }
}

void AP_tree::reset_line_width() {
    if (!is_leaf) {
        gr.left_linewidth  = 0;
        gr.right_linewidth = 0;

        get_leftson()->reset_line_width();
        get_rightson()->reset_line_width();
    }
}

