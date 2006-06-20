
#include <stdio.h>
#include <stdlib.h>

// #include <malloc.h>
#include <math.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <memory.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt_canvas.hxx>
#include "awt.hxx"
#include "awt_tree.hxx"
#include "awt_attributes.hxx"

/*****************************************************************************************
 ************           Filter                      **********
 *****************************************************************************************/

AP_filter::AP_filter(void){
    memset ((char *)this,0,sizeof(*this));
    int i;
    for (i=0;i<256;i++){
        simplify[i] = i;
    }
}

GB_ERROR AP_filter::init(const char *ifilter, const char *zerobases, long size)
{
    int             i;
    if (!ifilter || !*ifilter) {        // select all
        return this->init(size);
    }

    delete  filter_mask;
    filter_mask = new char[size];
    filter_len = size;
    real_len = 0;
    int slen = strlen(ifilter);
    if (slen>size) slen = size;
    for (i = 0; i < slen; i++) {
        if (zerobases) {
            if (strchr(zerobases, ifilter[i])) {
                filter_mask[i] = 0;
            } else {
                filter_mask[i] = 1;
                real_len++;
            }
        } else {
            if (ifilter[i]) {
                filter_mask[i] = 1;
                real_len++;
            } else {
                filter_mask[i] = 0;
            }
        }
    }
    for (; i < size; i++) {
        filter_mask[i] = 1;
        real_len++;
    }
    update = AP_timer();
    return 0;
}



GB_ERROR AP_filter::init(long size)
{
    int             i;
    delete  filter_mask;
    filter_mask = new char[size];
    real_len = filter_len = size;
    for (i = 0; i < size; i++) {
        filter_mask[i] = 1;
    }
    update = AP_timer();
    return 0;
}

AP_filter::~AP_filter(void){
    delete [] bootstrap;
    delete [] filter_mask;
    delete filterpos_2_seqpos;
}


char *AP_filter::to_string(){
    char *data = new char[filter_len+1];
    data[filter_len] = 0;
    int i;
    for (i=0;i<filter_len;i++){
        if (filter_mask[i]){
            data[i] = '1';
        }else{
            data[i] = '0';
        }
    }
    return data;
}


void AP_filter::enable_simplify(AWT_FILTER_SIMPLIFY type){
    int i;
    for (i=0;i<32;i++){
        simplify[i] = '.';
    }
    for (;i<256;i++){
        simplify[i] = i;
    }
    switch (type){
        case AWT_FILTER_SIMPLIFY_DNA:
            simplify[(unsigned char)'g'] = 'a';
            simplify[(unsigned char)'G'] = 'A';
            simplify[(unsigned char)'u'] = 'c';
            simplify[(unsigned char)'t'] = 'c';
            simplify[(unsigned char)'U'] = 'C';
            simplify[(unsigned char)'T'] = 'C';
            break;
        case AWT_FILTER_SIMPLIFY_PROTEIN:
            GB_CORE;
        case AWT_FILTER_SIMPLIFY_NONE:
            break;
    }
}

void AP_filter::calc_filter_2_seq(){
    delete filterpos_2_seqpos;
    filterpos_2_seqpos = new int[real_len];
    int i;
    int j = 0;
    for (i=0;i<filter_len;i++){
        if (filter_mask[i]){
            filterpos_2_seqpos[j++] = i;
        }
    }
}

void AP_filter::enable_bootstrap(int random_seed){
    delete [] bootstrap;
    bootstrap = new int[real_len];
    int i;
    if (random_seed) srand(random_seed);

    awt_assert(filter_len < RAND_MAX);

    for (i=0;i<this->real_len;i++){
        int r = rand();
        awt_assert(r >= 0);     // otherwise overflow in random number generator
        bootstrap[i] = r % filter_len;
    }
}

/*****************************************************************************************
 ************           Rates                       **********
 *****************************************************************************************/
void AP_rates::print(void)
{
    AP_FLOAT max;
    register int i;

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

AP_rates::AP_rates(void) {
    memset ((char *)this,0,sizeof(AP_rates));
}

char *AP_rates::init(AP_filter *fil)
{
    register int i;
    if (fil->update<= this->update) return 0;

    rate_len = fil->real_len;
    delete rates;
    rates = new AP_FLOAT[rate_len];
    for (i=0;i<rate_len;i++) {
        rates[i] = 1.0;
    }
    this->update = fil->update;
    return 0;
}

char *AP_rates::init(AP_FLOAT * ra, AP_filter *fil)
{
    register int i,j;
    if (fil->update<= this->update) return 0;

    rate_len = fil->real_len;
    delete rates;
    rates = new AP_FLOAT[rate_len];
    for (j=i=0;i<rate_len;j++) {
        if (fil->filter_mask[j]){
            rates[i++] = ra[j];
        }
    }
    this->update = fil->update;
    return 0;
}

AP_rates::~AP_rates(void)   { if (rates) delete(rates);}


/*****************************************************************************************
 ************           Weights                         **********
 *****************************************************************************************/

AP_weights::AP_weights(void) {
    memset ((char *)this,0,sizeof(AP_weights));
}

char *AP_weights::init(AP_filter *fil)
{
    register int i;
    if (fil->update<= this->update) return 0;

    weight_len = fil->real_len;
    delete weights;
    weights = new GB_UINT4[weight_len];
    for (i=0;i<weight_len;i++) {
        weights[i] = 1;
    }
    this->dummy_weights = 1;
    this->update = fil->update;
    return 0;
}

char *AP_weights::init(GB_UINT4 *w, AP_filter *fil)
{
    register int i,j;
    if (fil->update<= this->update) return 0;

    weight_len = fil->real_len;
    delete weights;
    weights = new GB_UINT4[weight_len];
    for (j=i=0;i<weight_len;j++) {
        if (fil->filter_mask[j]){
            weights[i++] = w[j];
        }
    }
    this->update = fil->update;
    return 0;}

AP_weights::~AP_weights(void)
{
    delete [] weights;
}

/*****************************************************************************************
 ************           Matrizes                            **********
 *****************************************************************************************/

void AP_matrix::set_description(const char *xstring,const char *ystring){
    char *x = strdup(xstring);
    char *y = strdup(ystring);
    char *t;
    int xpos = 0;
    x_description = (char **)GB_calloc(sizeof(char *),size);
    y_description = (char **)GB_calloc(sizeof(char *),size);
    for (t=strtok(x," ,;\n");t;t = strtok(0," ,;\n")){
        if(xpos>=size) GB_CORE;
        x_description[xpos++] = strdup(t);
    }
    int ypos = 0;
    for (t=strtok(y," ,;\n");t;t = strtok(0," ,;\n")){
        if(ypos>=size) GB_CORE;
        x_description[ypos++] = strdup(t);
    }
    free(x);
    free(y);
}

void AP_matrix::create_awars(AW_root *awr,const char *awar_prefix){
    char buffer[1024];
    int x,y;
    for (x = 0;x<size;x++){
        if (x_description[x]){
            for (y = 0;y<size;y++){
                if (y_description[y]){
                    sprintf(buffer,"%s/B%s/B%s",awar_prefix,x_description[x],y_description[y]);
                    if (x==y){
                        awr->awar_float(buffer,0)->set_minmax(0.0,2.0);
                    }else{
                        awr->awar_float(buffer,1.0)->set_minmax(0.0,2.0);
                    }
                }

            }
        }
    }
}
void AP_matrix::read_awars(AW_root *awr,const char *awar_prefix){
    char buffer[1024];
    int x,y;
    for (x = 0;x<size;x++){
        if (x_description[x]){
            for (y = 0;y<size;y++){
                if (y_description[y]){
                    sprintf(buffer,"%s/B%s/B%s",awar_prefix,x_description[x],y_description[y]);
                    this->set(x,y,awr->awar(buffer)->read_float());
                }
            }
        }
    }
}

void AP_matrix::create_input_fields(AW_window *aww,const char *awar_prefix){
    char buffer[1024];
    int x,y;
    aww->create_button(0,"    ");
    for (x = 0;x<size ;x++){
        if (x_description[x]){
            aww->create_button(0,x_description[x]);
        }
    }
    aww->at_newline();
    for (x = 0;x<size ;x++){
        if (x_description[x]){
            aww->create_button(0,x_description[x]);
            for (y = 0;y<size ;y++){
                if (y_description[y]){
                    sprintf(buffer,"%s/B%s/B%s",awar_prefix,x_description[x],y_description[y]);
                    aww->create_input_field(buffer,4);
                }
            }
            aww->at_newline();
        }
    }
}

void AP_matrix::normize(){  // set values so that average of non diag elems == 1.0
    int x,y;
    double sum = 0.0;
    double elems = 0.0;
    for (x = 0;x<size ;x++){
        if (x_description[x]){
            for (y = 0;y<size ;y++){
                if (y!=x && y_description[y]){
                    sum += this->get(x,y);
                    elems += 1.0;
                }
            }
        }
    }
    if (sum == 0.0) return;
    sum /= elems;
    for (x = 0;x<size ;x++){
        for (y = 0;y<size ;y++){
            this->set(x,y,get(x,y)/sum);
        }
    }
}

AP_smatrix::AP_smatrix(long si)
{
    m = (AP_FLOAT **)calloc(sizeof(AP_FLOAT *),(size_t)si);
    long i;
    for (i=0;i<si;i++){
        m[i] = (AP_FLOAT *)calloc(sizeof(AP_FLOAT),(size_t)(i+1));
    }

    size = si;
}

AP_smatrix::~AP_smatrix(void)
{
    long i;
    for (i=0;i<size;i++) free((char *)m[i]);
    free((char *)m);
}

AP_matrix::AP_matrix(long si)
{
    m = (AP_FLOAT **)calloc(sizeof(AP_FLOAT *),(size_t)si);
    long i;
    for (i=0;i<si;i++){
        m[i] = (AP_FLOAT *)calloc(sizeof(AP_FLOAT),(size_t)(si));
    }
    size = si;
}

AP_matrix::~AP_matrix(void)
{
    long i;
    for (i=0;i<size;i++){
        free((char *)(m[i]));
        if (x_description) free(x_description[i]);
        if (y_description) free(y_description[i]);
    }
    free(x_description);
    free(y_description);
    free((char *)m);
}



/*****************************************************************************************
 ************           AP_Sequence                 **********
 *****************************************************************************************/


char *AP_sequence::mutation_per_site = 0;
char *AP_sequence::static_mutation_per_site[3] = { 0,0,0 };

AP_sequence::~AP_sequence(void) { ; }
AP_FLOAT AP_sequence::real_len(void) { return 0.0; }

AP_sequence::AP_sequence(AP_tree_root *rooti){
    cashed_real_len = -1.0;
    is_set_flag = AP_FALSE;
    sequence_len = 0;
    update = 0;
    costs = 0.0;
    root = rooti;
}

void AP_sequence::set_gb(GBDATA *gb_data){
    this->set(GB_read_char_pntr(gb_data));
}
/*****************************************************************************************
 ************           AP_tree_root                    **********
 *****************************************************************************************/

void
AP_tree_tree_deleted(GBDATA * gbd, class AP_tree_root * tr
                     /* , GB_CB_TYPE gbtype */ )
{
    if (gbd == tr->gb_tree) {
        tr->gb_tree = 0;
    } else {
        printf("internal warning:: AP_tree_tree_deleted :: a callback to unknown tree occur \n");
    }
}

AP_tree_root::AP_tree_root(GBDATA * gb_maini, class AP_tree * tree_protoi,const char *name)
{
    memset((char *) this, 0, sizeof(AP_tree_root));
    if (tree_protoi) {
        this->tree_template = tree_protoi->dup();
    }
    this->gb_main = gb_maini;
    if(name){
        this->tree_name = strdup(name);
        GB_push_transaction(gb_main);
        this->gb_tree = GBT_get_tree(gb_main,name);
        if (gb_tree){
            GB_add_callback(this->gb_tree, GB_CB_DELETE, (GB_CB) AP_tree_tree_deleted, (int *) this);
        }
        this->gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
        this->gb_table_data = GB_search(gb_main, "table_data", GB_CREATE_CONTAINER);

        GB_pop_transaction(gb_main);
    }

}

GB_BOOL AP_tree_root::is_tree_updated(void)
{
    if (!this->gb_tree) return GB_TRUE;
    GB_transaction dummy(gb_tree);
    if (GB_read_clock(this->gb_tree) > tree_timer) return GB_TRUE;
    return GB_FALSE;
}

GB_BOOL AP_tree_root::is_species_updated(void)
{
    if (!this->gb_species_data) return GB_TRUE;
    GB_transaction dummy(gb_species_data);
    if (GB_read_clock(this->gb_species_data) > species_timer) return GB_TRUE;
    if (GB_read_clock(this->gb_table_data) > table_timer) return GB_TRUE;
    return GB_FALSE;
}

AP_tree_root::~AP_tree_root()
{
    free(tree_name);
    if (this->gb_tree) {
        GB_transaction dummy(this->gb_tree);
        GB_remove_callback(this->gb_tree,GB_CB_DELETE, (GB_CB)AP_tree_tree_deleted,(int *)this);
    }
    delete tree_template;
    delete sequence_template;
}

void AP_tree_root::update_timers(void)
{
    if (!this->gb_species_data) return;
    GB_transaction dummy(GB_get_root(this->gb_species_data));
    if (this->gb_tree) tree_timer = GB_read_clock(this->gb_tree);
    species_timer = GB_read_clock(this->gb_species_data);
    table_timer = GB_read_clock(this->gb_table_data);
}

/*****************************************************************************************
 ************           AP_tree                     **********
 *****************************************************************************************/
void ap_tree_node_deleted(GBDATA *, int *cl, GB_CB_TYPE){
    AP_tree *THIS = (AP_tree *)cl;
    THIS->gb_node = 0;
}

static bool vtable_ptr_check_done = false;

AP_tree::AP_tree(AP_tree_root   *tree_rooti)
{
    //memset(((char*)this)+sizeof(char*), 0, sizeof(*this)-sizeof(char*));

    CLEAR_GBT_TREE_ELEMENTS(this);
    gr.clear();
    br.clear();

    mutation_rate = 0;
    stack_level = 0;
    tree_root = tree_rooti;
    sequence = 0;

    gr.spread = 1.0;

    if (!vtable_ptr_check_done) {
        vtable_ptr_check_done = true;
        GBT_TREE *tree        = get_gbt_tree(); // hack-warning: points to part of this!
        bool      was_leaf    = tree->is_leaf;

        // if one of the assertions below fails, then there is a problem with the
        // vtable-pointer position (grep for FAKE_VTAB_PTR for more info)
        tree->is_leaf = GB_FALSE; awt_assert(is_leaf == GB_FALSE);
        tree->is_leaf = GB_TRUE; awt_assert(is_leaf == GB_TRUE);
        tree->is_leaf = was_leaf;
    }
}

AP_tree::~AP_tree(void)
{
    free(name);
    free(remark_branch);
    delete leftson;
    delete rightson;
    delete sequence;
    if (gr.callback_exists && gb_node){
        GB_remove_callback(gb_node,GB_CB_DELETE,ap_tree_node_deleted,(int *)this);
    }
    if (this->tree_root) this->tree_root->inform_about_delete(this);
}

AP_tree *AP_tree::dup(void) {
    return new AP_tree(this->tree_root);
}

AP_tree * AP_tree::brother() const {
    if (father == NULL) {
        AW_ERROR("AP_tree::brother called at root");
        return 0;
    }else {
        if (father->leftson == this) return father->rightson;
        if (father->rightson == this) return father->leftson;
        AW_ERROR("AP_tree::brother: no brother: tree damaged !!!");
        return 0;
    }
}

void AP_tree::set_fatherson(AP_tree *new_son) {
    if (father == NULL) {
        AW_ERROR("set_fatherson called at root");
        return ;
    }
    else {
        if (father->leftson == this) { father->leftson = new_son; return; }
        if (father->rightson == this) { father->rightson = new_son; return; }
        AW_ERROR("AP_tree::set_fatherson(AP_tree *new_son): tree damaged!");
        return ;
    }
}

void AP_tree::set_fathernotson(AP_tree *new_son) {
    if (father == NULL) {
        //new AP_ERR("fathernotson called at root\n");
        return ;
    }
    else {
        if (father->leftson == this)  { father->rightson = new_son; return;}
        if (father->rightson == this)  { father->leftson = new_son; return; }
        AW_ERROR("AP_tree::set_fathernotson: tree damaged!");
        return ;
    }
}

void AP_tree::clear_branch_flags(void)
{
    this->br.touched = 0;
    this->br.kl_marked = 0;
    if (is_leaf) return;
    leftson->clear_branch_flags();
    rightson->clear_branch_flags();
}


AP_BOOL AP_tree::is_son(AP_tree *mfather) {
    if (this->father == mfather) return AP_TRUE;
    if (this->father) return this->father->is_son(mfather);
    return AP_FALSE;
}

GB_ERROR AP_tree::insert(AP_tree *new_brother){
    AP_tree        *new_tree = dup();
    AP_FLOAT    laenge;

    new_tree->leftson = this;
    new_tree->rightson = new_brother;
    new_tree->father = new_brother->father;
    this->father = new_tree;

    if (new_brother->father) {
        if (new_brother->father->leftson == new_brother) {
            laenge = new_brother->father->leftlen = new_brother->father->leftlen*.5;
            new_brother->father->leftson = new_tree;
        } else {
            laenge = new_brother->father->rightlen = new_brother->father->rightlen*.5;
            new_brother->father->rightson = new_tree;
        }
    }else{
        laenge = 0.5;
    }
    new_tree->leftlen = laenge;
    new_tree->rightlen = laenge;
    new_brother->father = new_tree;

    if (!new_tree->father) {
        this->tree_root->inform_about_changed_root(new_brother,new_tree);
    }
    return 0;
}

char *AP_tree_root::inform_about_changed_root(class AP_tree *old, class AP_tree *newroot) {
    if (this->root_changed) root_changed(root_changed_cd,old,newroot);
    this -> tree = newroot;
    if (!newroot) {             // tree empty
        if (gb_tree) {
            GB_delete(gb_tree);
            gb_tree = 0;
            aw_message(GBS_global_string("Tree '%s' lost all leafs and has been deleted!", tree_name));
        }
    }
    return 0;
}

char *AP_tree_root::inform_about_delete(class AP_tree *old) {
    if (this->node_deleted) node_deleted(node_deleted_cd,old);
    return 0;
}


GB_ERROR AP_tree::remove(void){
    if (father == 0) {
        this->tree_root->inform_about_changed_root(father,0);
        return 0;
    }

    if (father->leftson != this) {
        father->swap_sons();
    }
    if (father->gb_node) {      // move inner information to subtree
        if (!father->rightson->gb_node && !father->rightson->is_leaf){
            father->rightson->gb_node = father->gb_node;
            father->gb_node = 0;
        }
    }

    if (father->father != 0) {
        AP_tree        *ff_pntr = father->father;

        if (ff_pntr->leftson == father) {
            ff_pntr->leftlen += father->rightlen;
            ff_pntr->leftson = father->rightson;
            father->rightson->father = ff_pntr;
        } else {
            ff_pntr->rightlen += father->rightlen;
            ff_pntr->rightson = father->rightson;
            father->rightson->father = ff_pntr;
        }
    } else {
        AP_tree        *newbroth = brother();
        newbroth->father = 0;
        this->tree_root->inform_about_changed_root(father,newbroth);
    }
    this->tree_root->inform_about_delete(father);
    this->tree_root->inform_about_delete(this);
    this->set_fathernotson(0);
    return 0;
}

const char *AP_tree::move(AP_tree *new_brother, AP_FLOAT rel_pos)
    // rel_pos == 0.0 -> at father
    //  == 1.0 -> at brother
{
    if (new_brother->is_son(this)) {
        return GB_export_error("You cannot move a tree to itself");
    }
    if (father == 0) return 0;
    if (father->leftson != this) {
        father->swap_sons();
    }

    if (father->father == 0) {
        if (new_brother->father == this->father) return 0;  // move root to root
        brother()->father = 0;
        this->tree_root->inform_about_changed_root(father,brother());
    }else{
        AP_tree        *ff_pntr = father->father;
        if (father == new_brother) {    // just pull branches !!
            new_brother = brother();
            if (ff_pntr->leftson == father) {
                rel_pos *= ff_pntr->leftlen / (father->rightlen+ff_pntr->leftlen);
            }else{
                rel_pos *= ff_pntr->rightlen/(father->rightlen+ff_pntr->rightlen);
            }
        }else if (new_brother->father == this->father) {    // just pull branches !!
            if (ff_pntr->leftson == father) {
                rel_pos = 1.0 + (rel_pos -1.0) *
                    father->rightlen / (father->rightlen+ff_pntr->leftlen);
            }else{
                rel_pos = 1.0 + (rel_pos -1.0) *
                    father->rightlen/(father->rightlen+ff_pntr->rightlen);
            }
        }

        if (ff_pntr->leftson == father) {
            ff_pntr->leftlen += father->rightlen;
            ff_pntr->leftson = father->rightson;
            father->rightson->father = ff_pntr;
        } else {
            ff_pntr->rightlen += father->rightlen;
            ff_pntr->rightson = father->rightson;
            father->rightson->father = ff_pntr;
        }
    }

    AP_tree        *new_tree = father;
    AP_tree     *brother_father = new_brother->father;
    AP_FLOAT    laenge;

    if (brother_father->leftson == new_brother) {
        laenge = brother_father->leftlen;
        laenge -= brother_father->leftlen = laenge * rel_pos ;
        new_brother->father->leftson = new_tree;
    } else {
        laenge = brother_father->rightlen;
        laenge -= brother_father->rightlen = laenge * rel_pos ;
        brother_father->rightson = new_tree;
    }
    new_tree->rightlen = laenge;

    new_brother->father = new_tree;

    new_tree->rightson = new_brother;
    new_tree->father = brother_father;

    return 0;
}

void AP_tree::swap_sons(void)
{
    AP_tree *h_at = this->leftson;
    this->leftson = this->rightson;
    this->rightson = h_at;

    double h = this->leftlen;
    this->leftlen = this->rightlen;
    this->rightlen = h;
}

GB_ERROR AP_tree::swap_assymetric(AP_TREE_SIDE mode){
    //mode 1 exchanges lefts with brother
    // mode 2 exchanges rights with brother
    AP_tree * pntr;
    AP_tree        *pntr_father;
    AP_tree        *pntr_brother;

    if (this->is_leaf == AP_TRUE) {
        return (char *)GB_export_error("swap not allowed at leaf  !!");
    }
    if (this->father == 0) {
        return (char *)GB_export_error("swap not allowed at root  !!");
    }
    if (this->father->father == 0) {
        pntr_brother = this->brother();
        if (pntr_brother->is_leaf == AP_TRUE) {
            // no swap needed !
            return 0;
        }
        switch (mode) {
            case AP_LEFT:
                pntr_brother->leftson->father = this;
                pntr = pntr_brother->leftson;
                pntr_brother->leftson = leftson;
                this->leftson->father = pntr_brother;
                this->leftson = pntr;
                break;

            case AP_RIGHT:
                //rightson with brother's leftson
                pntr_brother->leftson->father = this;
                this->rightson->father = pntr_brother;
                pntr = pntr_brother->leftson;
                pntr_brother->leftson = this->rightson;
                this->rightson = pntr;
                break;
            default:
                GB_warning("Cannot handle switch in swap()");
                break;
        }

        return 0;
    }
    pntr_father = father->father;
    AP_FLOAT help_len;
    switch (mode) {
        case AP_LEFT:
            //swap leftson with brother

            if (father->leftson == this) {
                father->rightson->father = this;
                this->leftson->father = this->father;
                pntr = father->rightson;
                help_len = father->rightlen;
                father->rightlen = this->leftlen;
                this->leftlen = help_len;
                father->rightson = leftson;
                this->leftson = pntr;
            } else {
                father->leftson->father = this;
                this->leftson->father = this->father;
                pntr = father->leftson;
                help_len = father->leftlen;
                father->leftlen = this->leftlen;
                this->leftlen = help_len;
                father->leftson = leftson;
                this->leftson = pntr;
            }
            break;
        case AP_RIGHT:
            //rightson with brother
            if (father->leftson == this) {
                father->rightson->father = this;
                this->rightson->father = this->father;
                pntr = father->rightson;
                help_len = father->rightlen;
                father->rightlen = this->rightlen;
                this->rightlen = help_len;
                father->rightson = rightson;
                this->rightson = pntr;
            } else {
                father->leftson->father = this;
                this->rightson->father = this->father;
                pntr = father->leftson;
                help_len = father->leftlen;
                father->leftson = rightson;
                father->leftlen = this->rightlen;
                this->rightlen = help_len;
                this->rightson = pntr;
            }
            break;
        default:
            GB_warning("Cannot handle switch in swap() 2");
    }
    return 0;
}

GB_ERROR AP_tree::set_root()
{
    AP_tree        *pntr;
    AP_tree        *prev;
    AP_tree        *next;
    AP_branch_members br1,br2;
    if (father == 0) {      // already root
        return 0;
    }

    if (father->father == 0) {  // already root  ???
        return 0;
    }

    AP_tree *old_brother = 0;
    br1 = this->br;
    for  (pntr = this->father; pntr->father; pntr = pntr->father) {
        br2 = pntr->br;
        pntr->br = br1;
        br1 = br2;
        old_brother= pntr;
    }
    if ( pntr->leftson == old_brother) {
        pntr->rightson->br = br1;
    }

    if (old_brother)    old_brother = old_brother->brother();
    AP_tree        *    old_root = pntr;


    {           // move remark branches to top
        AP_tree *node;
        char *remark = GBS_strdup(this->remark_branch);
        for (node = this;node->father;node = node->father){
            char *sh = node->remark_branch;
            node->remark_branch = remark;
            remark = sh;
        }
        delete remark;
    }
    AP_FLOAT old_root_len = old_root->leftlen + old_root->rightlen;

    //
    //new node & this init
    //

    old_root->leftson = this;
    old_root->rightson = this->father;  // will be set later

    if (this->father->leftson == this) {
        old_root->leftlen = old_root->rightlen = this->father->leftlen*.5;
    } else {
        old_root->leftlen = old_root->rightlen = this->father->rightlen*.5;
    }

    next = father->father;
    prev = old_root;
    pntr = father;

    if (father->leftson == this)    father->leftson = old_root; // to set the flag correctly

    //
    //loop from father to son of root   rotate tree
    //
    while  (next->father) {
        double len = 0;
        if (next->leftson == pntr){
            len = next->leftlen;
        }else{
            len = next->rightlen;
        }

        if (pntr->leftson == prev) {
            pntr->leftson = next;
            pntr->leftlen = len;
        }else{
            pntr->rightson = next;
            pntr->rightlen = len;
        }
        pntr->father = prev;
        prev = pntr;
        pntr = next;
        next = next->father;
    }
    // now next points to the old root, which has been destroyed 20 lines above
    //
    //pointer at oldroot
    // pntr == brother before old root == next
    //
    if (pntr->leftson == prev) {
        pntr->leftlen = old_root_len;
        pntr->leftson = old_brother;
    }else{
        pntr->rightlen = old_root_len;
        pntr->rightson = old_brother;
    }
    old_brother->father = pntr;
    pntr->father = prev;
    this->father = old_root;
    return 0;
}

void AP_tree::test_tree(void) const
{
    int d = 0;
    if (this->is_leaf) return;
    if (this != this->rightson->father) d = 1;
    if (this != this->leftson->father) d = 1;
    if (d ) {
        AW_ERROR("AP_tree::test_tree: Tree damaged");
    }else{
        this->rightson->test_tree();
        this->leftson->test_tree();
    }
}


GB_INLINE short PH_tree_read_byte(GBDATA *tree,const char *key, int init)
{
    GBDATA *gbd;
    if (!tree) return init;
    gbd = GB_find(tree,key,0,down_level);
    if (!gbd) return init;
    return (short)GB_read_byte(gbd);
}

GB_INLINE float PH_tree_read_float(GBDATA *tree,const char *key, double init)
{
    GBDATA *gbd;
    if (!tree) return (float)init;
    gbd = GB_find(tree,key,0,down_level);
    if (!gbd) return (float)init;
    return (float)GB_read_float(gbd);
}



/** moves all node/leaf information from struct GBT_TREE to AP_tree */
void AP_tree::load_node_info(){
    this->gr.spread = PH_tree_read_float(this->gb_node, "spread", 1.0);
    this->gr.left_angle = PH_tree_read_float(this->gb_node, "left_angle", 0.0);
    this->gr.right_angle = PH_tree_read_float(this->gb_node, "right_angle", 0.0);
    this->gr.left_linewidth = PH_tree_read_byte(this->gb_node, "left_linewidth", 0);
    this->gr.right_linewidth = PH_tree_read_byte(this->gb_node, "right_linewidth", 0);
    this->gr.grouped = PH_tree_read_byte(this->gb_node, "grouped", 0);
}

void AP_tree::move_gbt_2_ap(GBT_TREE* tree, GB_BOOL insert_remove_cb)
{
    this->is_leaf = tree->is_leaf;
    this->leftlen = tree->leftlen;
    this->rightlen = tree->rightlen;
    this->gb_node = tree->gb_node;

    this->name = tree->name;
    tree->name = NULL;

    this->remark_branch = tree->remark_branch;
    tree->remark_branch = NULL;

    if(this->is_leaf) return;

    this->leftson = dup();
    this->rightson = dup();
    this->leftson->father = this;
    this->rightson->father = this;

    leftson->move_gbt_2_ap(tree->leftson,insert_remove_cb);
    rightson->move_gbt_2_ap(tree->rightson,insert_remove_cb);

    this->load_node_info();
    if (insert_remove_cb && gb_node){
        gr.callback_exists = 1;
        GB_add_callback(gb_node,GB_CB_DELETE,ap_tree_node_deleted,(int *)this);
    }

    return;
}

#if defined(DEBUG)
#if defined(DEVEL_RALF)
#define DEBUG_PH_tree_write_byte
#endif // DEVEL_RALF
#endif // DEBUG


GB_ERROR PH_tree_write_byte(GBDATA *gb_tree, AP_tree *node,short i,const char *key, int init)
{
    GBDATA *gbd;
    GB_ERROR error= 0;
    if (i==init) {
        if (node->gb_node){
            gbd = GB_find(node->gb_node,key,0,down_level);
            if (gbd) {
#if defined(DEBUG_PH_tree_write_byte)
                printf("[PH_tree_write_byte] deleting db entry %p\n", gbd);
#endif // DEBUG_PH_tree_write_byte
                GB_delete(gbd);
            }
        }
    }else{
        if (!node->gb_node){
            node->gb_node = GB_create_container(gb_tree,"node");
#if defined(DEBUG_PH_tree_write_byte)
            printf("[PH_tree_write_byte] created node-container %p\n", node->gb_node);
#endif // DEBUG_PH_tree_write_byte
        }
        gbd = GB_find(node->gb_node,key,0,down_level);
        if (!gbd) {
            gbd = GB_create(node->gb_node,key,GB_BYTE);
#if defined(DEBUG_PH_tree_write_byte)
            printf("[PH_tree_write_byte] created db entry %p\n", gbd);
#endif // DEBUG_PH_tree_write_byte
        }
        error = GB_write_byte(gbd,i);
    }
    return error;
}

GB_ERROR PH_tree_write_float(GBDATA *gb_tree, AP_tree *node,float f,const char *key, float init)
{
    GBDATA *gbd;
    GB_ERROR error= 0;
    if (f==init) {
        if (node->gb_node){
            gbd = GB_find(node->gb_node,key,0,down_level);
            if (gbd) GB_delete(gbd);
        }
    }else{
        if (!node->gb_node){
            node->gb_node = GB_create_container(gb_tree,"node");
        }
        gbd = GB_find(node->gb_node,key,0,down_level);
        if (!gbd) gbd = GB_create(node->gb_node,key,GB_FLOAT);
        error = GB_write_float(gbd,f);
    }
    return error;
}

GB_ERROR PH_tree_write_tree_rek(GBDATA *gb_tree, AP_tree * THIS)
{
    GB_ERROR error;
    if (THIS->is_leaf){
        return 0;
    }
    error             = PH_tree_write_tree_rek(gb_tree,THIS->leftson);
    if (!error) error = PH_tree_write_tree_rek(gb_tree,THIS->rightson);

    if (!error) error = PH_tree_write_float(gb_tree,THIS, THIS->gr.spread,"spread", 1.0);
    if (!error) error = PH_tree_write_float(gb_tree,THIS, THIS->gr.left_angle,"left_angle", 0.0);
    if (!error) error = PH_tree_write_float(gb_tree,THIS, THIS->gr.right_angle,"right_angle", 0.0);
    if (!error) error = PH_tree_write_byte(gb_tree,THIS, THIS->gr.left_linewidth,"left_linewidth", 0);
    if (!error) error = PH_tree_write_byte(gb_tree,THIS, THIS->gr.right_linewidth,"right_linewidth", 0);
    if (!error) error = PH_tree_write_byte(gb_tree,THIS, THIS->gr.grouped,"grouped", 0);
    return error;
}

const char *AP_tree::save(char  *tree_name)
{
    GB_ERROR error = 0;
    tree_name = tree_name;
    GBDATA *gb_tree = this->tree_root->gb_tree;
    GBDATA *gb_main = this->tree_root->gb_main;

    if (!gb_tree) return "I cannot save your tree, it has been deleted";

    GB_push_transaction(gb_main);
    if (!error) error = PH_tree_write_tree_rek(gb_tree,this);
    if (!error) error = GBT_write_tree(gb_main,gb_tree,0,this->get_gbt_tree());
    this->tree_root->update_timers();
    GB_pop_transaction(gb_main);
    return error;
}

GB_ERROR AP_tree::move_group_info(AP_tree *new_group){
    GB_ERROR error = 0;
    if (is_leaf || !name) {
        error = GB_export_error("Please select a valid group");
    }
    else if (!gb_node){
        error = GB_export_error("Internal Error: group with name is missing DB-entry");
    }
    else if (new_group->is_leaf) {
        if (new_group->name) {
            error = GB_export_error("'%s' is not a valid target for group information of '%s'.", new_group->name, name);
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
            else { /* exchange two group infos */
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
            gb_group_name = GB_find(new_group->gb_node, "group_name", 0, down_level);
            if (gb_group_name) GB_touch(gb_group_name); // force taxonomy reload
        }
    }
    return error;
}

void AP_tree::update(  )
{
    GB_transaction dummy(tree_root->gb_main);
    this->tree_root->update_timers();
}

GBT_LEN AP_tree::arb_tree_deep()
{
    GBT_LEN l,r;
    if (is_leaf) return 0.0;
    l = leftlen + leftson->arb_tree_deep();
    r = rightlen + rightson->arb_tree_deep();
    if (l<r) l=r;
    gr.tree_depth = l;
    return l;
}

GBT_LEN AP_tree::arb_tree_min_deep()
{
    GBT_LEN l,r;
    if (is_leaf) return 0.0;
    l = leftlen + leftson->arb_tree_min_deep();
    r = rightlen + rightson->arb_tree_min_deep();
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
    l =  leftson->arb_tree_set_leafsum_viewsum();
    r =  rightson->arb_tree_set_leafsum_viewsum();
    gr.leave_sum = r+l;
    gr.view_sum = leftson->gr.view_sum + rightson->gr.view_sum;
    if (gr.grouped) {
        gr.view_sum = (int)pow((double)(gr.leave_sum - GROUPED_SUM + 9),.33);
    }
    return gr.leave_sum;
}

int AP_tree::arb_tree_leafsum2()    // count all leafs
{
    if (is_leaf) return 1;
    return leftson->arb_tree_leafsum2() + rightson->arb_tree_leafsum2();
}

void AP_tree::calc_hidden_flag(int father_is_hidden){
    gr.hidden = father_is_hidden;
    if (is_leaf) {
        return;
    }
    if (gr.grouped){
        father_is_hidden = 1;
    }
    this->leftson->calc_hidden_flag(father_is_hidden);
    this->rightson->calc_hidden_flag(father_is_hidden);
}

int AP_tree::calc_color(void) {
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
        l = leftson->calc_color();
        r = rightson->calc_color();

        if ( l == r) res = l;

        else if ( l == AWT_GC_SELECTED && r != AWT_GC_SELECTED) res = AWT_GC_UNDIFF;
        else if ( l != AWT_GC_SELECTED && r == AWT_GC_SELECTED) res = AWT_GC_UNDIFF;

        else if ( l == AWT_GC_SOME_MISMATCHES) res = r;
        else if ( r == AWT_GC_SOME_MISMATCHES) res = l;

        else if ( l == AWT_GC_UNDIFF || r == AWT_GC_UNDIFF) res = AWT_GC_UNDIFF;

        else {
            awt_assert(l != AWT_GC_SELECTED && r != AWT_GC_SELECTED);
            awt_assert(l != AWT_GC_UNDIFF && r != AWT_GC_UNDIFF);
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
        if (gb_node){
            res = GBS_read_hash( hashptr, name );
            //          printf("Ausgabe aus Alex' Hashtabelle : %d\n",res);
            if (GB_read_flag(gb_node))          //Bakt. ist markiert
                if (!res)
                    res = AWT_GC_BLACK;
                else
                    if (!res)
                        res =  AWT_GC_BLACK;
        }else{
            res =   AWT_GC_SOME_MISMATCHES;
        }
    }else{
        l = leftson->calc_color_probes(hashptr);
        r = rightson->calc_color_probes(hashptr);
        if ( l == r) {
            res = l;
        }else if ( l == AWT_GC_SOME_MISMATCHES) {
            res = r;
        }else if ( r == AWT_GC_SOME_MISMATCHES) {
            res = l;
        }else{
            res = AWT_GC_UNDIFF;
        }
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

void AP_tree::parsimony_rek(void)
{
    AW_ERROR("Abstract class has no default AP_tree::parsimony_rek");
}

AP_BOOL AP_tree::push(AP_STACK_MODE smode, unsigned long slevel) {
    AW_ERROR("AP_tree::push");
    smode=smode;slevel=slevel;return AP_FALSE;
}
void AP_tree::pop(unsigned long slevel){
    AW_ERROR("AP_tree::pop");
    slevel=slevel;
}
AP_BOOL AP_tree::clear( unsigned long stack_update, unsigned long user_push_counter){
    AW_ERROR("AP_tree::clear");
    stack_update=stack_update;user_push_counter=user_push_counter;
    return AP_FALSE;
}
void AP_tree::unhash_sequence(void){
    AW_ERROR("AP_tree::unhash_sequence");
}
AP_FLOAT AP_tree::costs(void){
    AW_ERROR("AP_tree::costs");
    return 0.0;
}

GB_ERROR AP_tree::load( AP_tree_root *tr, int link_to_database, GB_BOOL insert_delete_cbs, GB_BOOL show_status, int *zombies, int *duplicates)    {
    GBDATA *gb_main = tr->gb_main;
    char *tree_name = tr->tree_name;
    GB_transaction dummy(gb_main);  // open close a transaction

    GBT_TREE *gbt_tree;
    GBDATA *gb_tree_data;
    GBDATA *gb_tree;

    gbt_tree = GBT_read_tree(gb_main,tree_name,-sizeof(GBT_TREE));
    if (!gbt_tree) {
        return GB_get_error();
    }

    gb_tree_data = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    gb_tree = GB_search(gb_tree_data,tree_name,GB_FIND);

    if (link_to_database) {
        GB_ERROR error = GBT_link_tree(gbt_tree,gb_main,show_status, zombies, duplicates);
        if (error) {
            GBT_delete_tree(gbt_tree);
            return error;
        }
    }
    this->tree_root = tr;
    move_gbt_2_ap(gbt_tree,insert_delete_cbs);
    GBT_delete_tree(gbt_tree);
    this->tree_root->update_timers();
    return 0;
}

GB_ERROR AP_tree::relink(   )
{
    GB_transaction dummy(this->tree_root->gb_main); // open close a transaction
    GB_ERROR error = GBT_link_tree(this->get_gbt_tree(),this->tree_root->gb_main,GB_FALSE, 0, 0); // no status
    this->tree_root->update_timers();
    return error;
}

AP_UPDATE_FLAGS AP_tree::check_update( )
{
    GBDATA *gb_main = this->tree_root->gb_main;
    if (!gb_main) return AP_UPDATE_RELOADED;
    GB_transaction dummy(gb_main);

    if (this->tree_root->is_tree_updated()) {
        return AP_UPDATE_RELOADED;
    }
    if (this->tree_root->is_species_updated()){
        return AP_UPDATE_RELINKED;
    }
    return AP_UPDATE_OK;
}

void AP_tree::delete_tree() {
    if (is_leaf) {
        delete this;
        return;
    } else {
        leftson->delete_tree();
        rightson->delete_tree();
    }
}

void AP_tree::_load_sequences_rek(char *use,GB_BOOL set_by_gbdata,long nnodes, long *counter) {
    /* uses sequence -> filter !!!
     * loads all sequences rekursivly
     * clears sequence->is_set_flag
     * flag = 0 with loading - 1 = without
     *  use = alignment
     */

    GBDATA *gb_data;
    if (is_leaf) {
        if (gb_node  ) {
            if ( sequence == 0) {
                if (nnodes){
                    aw_status((*counter)++/(double)nnodes);
                }
                gb_data = GBT_read_sequence(gb_node,use);
                if (!gb_data) return;
                sequence = this->tree_root->sequence_template->dup();
                if (set_by_gbdata){
                    sequence->set_gb(gb_data);
                }else{
                    sequence->set(GB_read_char_pntr(gb_data));
                }
            }
        }
        return;
    } else {
        if (sequence) sequence->is_set_flag = AP_FALSE;
        leftson->_load_sequences_rek(use,set_by_gbdata,nnodes,counter);
        rightson->_load_sequences_rek(use,set_by_gbdata,nnodes,counter);
    }
    return;
}

void AP_tree::load_sequences_rek(char *use,GB_BOOL set_by_gbdata,GB_BOOL show_status){
    long counter = 0;
    long nnodes = 0;
    if (show_status){
        nnodes = arb_tree_leafsum2();
        aw_status("Loading sequences");
    }
    _load_sequences_rek(use,set_by_gbdata,nnodes, &counter);
}

char *buildLeafList_rek(AP_tree *THIS, AP_tree **list,long& num) {
    // builds a list of all species
    if (THIS->is_leaf == AP_FALSE) {
        char *error = 0;
        if (!error) error = buildLeafList_rek(THIS->leftson,list,num);
        if (!error) error = buildLeafList_rek(THIS->rightson,list,num);
        return error;
    }
    list[num] = THIS;
    num++;
    return 0;
}

GB_ERROR AP_tree::buildLeafList(AP_tree **&list, long &num)
{
    num = this->arb_tree_leafsum2();
    list = new AP_tree *[num+1];
    list[num] = 0;
    long count = 0;
    return buildLeafList_rek(this,list,count);
}

char *buildNodeList_rek(AP_tree *THIS, AP_tree **list,long& num) {
    // builds a list of all species
    if (!THIS->is_leaf) {
        if (THIS->father) list[num++] = THIS;
        char *error = 0;
        if (!error) error = buildNodeList_rek(THIS->leftson,list,num);
        if (!error) error = buildNodeList_rek(THIS->rightson,list,num);
        return error;
    }
    return 0;
}

GB_ERROR AP_tree::buildNodeList(AP_tree **&list, long &num)
{
    num = this->arb_tree_leafsum2()-1;
    list = new AP_tree *[num+1];
    list[num] = 0;
    num  = 0;
    return buildNodeList_rek(this,list,num);
}

char *buildBranchList_rek(AP_tree *THIS, AP_tree **list,long& num,
                          AP_BOOL create_terminal_branches, int deep) {
    // builds a list of all species
    if (!deep) return 0;

    if (THIS->father && (create_terminal_branches || !THIS->is_leaf) ) {
        if (THIS->father->father){
            list[num++] = THIS;
            list[num++] = THIS->father;
        }else{                  // root
            if (THIS->father->leftson == THIS) {
                list[num++] = THIS;
                list[num++] = THIS->father->rightson;
            }
        }
    }
    if (!THIS->is_leaf) {
        char *error = 0;
        if (!error) error = buildBranchList_rek(THIS->leftson,list,num,
                                                create_terminal_branches,deep-1);
        if (!error) error = buildBranchList_rek(THIS->rightson,list,num,
                                                create_terminal_branches,deep-1);
        return error;
    }
    return 0;
}

GB_ERROR AP_tree::buildBranchList(AP_tree **&list, long &num,
                                  AP_BOOL create_terminal_branches, int deep)
{
    if (deep>=0) {
        int i;
        num = 2;
        for (i=0;i<deep;i++) num *=2;
    }else{
        if (create_terminal_branches) {
            num = this->arb_tree_leafsum2()*2;
        }else{
            num = this->arb_tree_leafsum2();
        }
    }
    if (num<0) num = 0;
    list = new AP_tree *[num*2+4];
    long count = 0;
    if (!num) return 0;
    char *error = buildBranchList_rek(this,list,count,create_terminal_branches,deep);
    list[count] = 0;
    num = count/2;
    return error;

}


GB_ERROR AP_tree::remove_leafs(GBDATA *gb_main,int awt_remove_type)
{
    AP_tree        **list;
    long             count;
    GB_ERROR         error   = 0;
    if (!error) error        = buildLeafList(list,count);
    long             i;
    GB_transaction   ta(gb_main);

    for (i=0;i<count; i++) {
        long flag;
        if (list[i]->gb_node) {
            flag = GB_read_flag(list[i]->gb_node);
            if (    ( flag && (awt_remove_type & AWT_REMOVE_MARKED) ) ||
                    ( !flag && (awt_remove_type & AWT_REMOVE_NOT_MARKED) ) ||
                    ( !list[i]->sequence && (awt_remove_type & AWT_REMOVE_NO_SEQUENCE) )
                    ){
                error = list[i]->remove();
                if (!(awt_remove_type & AWT_REMOVE_BUT_DONT_FREE)){
                    delete list[i]->father;
                }
            }
        }else{
            if (awt_remove_type & AWT_REMOVE_DELETED) {
                error = list[i]->remove();
                if (!(awt_remove_type & AWT_REMOVE_BUT_DONT_FREE)){
                    delete list[i]->father;
                }
            }
        }
        if (error) break;
    }
    delete [] list;

    return error;
}

void AP_tree::remove_bootstrap(GBDATA *gb_main){
    delete this->remark_branch;
    this->remark_branch = 0;
    if (this->is_leaf) return;
    this->leftson->remove_bootstrap(gb_main);
    this->rightson->remove_bootstrap(gb_main);
}
void AP_tree::reset_branchlengths(GBDATA *gb_main){
    if (is_leaf) return;

    leftlen = rightlen = 0.1;

    leftson->reset_branchlengths(gb_main);
    rightson->reset_branchlengths(gb_main);
}

void AP_tree::scale_branchlengths(GBDATA *gb_main, double factor) {
    if (is_leaf) return;

    leftlen  *= factor;
    rightlen *= factor;

    leftson->scale_branchlengths(gb_main, factor);
    rightson->scale_branchlengths(gb_main, factor);
}

void AP_tree::bootstrap2branchlen(GBDATA *gb_main) { // copy bootstraps to branchlengths
    if (is_leaf) {
        set_branchlength(0.1);
    }
    else {
        if (remark_branch && father) {
            int    bootstrap = atoi(remark_branch);
            double len       = bootstrap/100.0;
            set_branchlength(len);
        }
        leftson->bootstrap2branchlen(gb_main);
        rightson->bootstrap2branchlen(gb_main);
    }
}

void AP_tree::branchlen2bootstrap(GBDATA *gb_main) { // copy branchlengths to bootstraps
    if (remark_branch) {
        delete remark_branch;
        remark_branch = 0;
    }
    if (!is_leaf) {
        remark_branch = GBS_global_string_copy("%i%%", int(get_branchlength()*100.0 + .5));

        leftson->branchlen2bootstrap(gb_main);
        rightson->branchlen2bootstrap(gb_main);
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
    GB_ERROR error  = this->buildNodeList(list,sumnodes);
    if (error) return 0;
    if (!sumnodes) {
        delete list;
        return 0;
    }

    retlist = (AP_tree **)calloc(anzahl,sizeof(AP_tree *));

    i = 0;
    count = sumnodes;
    for  (i=0; i< anzahl; i++) {
        // choose two random numbers
#if defined(SUN4)
        num = (int)random();        // random node
#else
        num = (int)rand();      // random node
#endif
        if (num<0) num *=-1;
        num = num%count;        // exclude already selected nodes
        retlist[i] = list[num];     // export node
        count--;            // exclude node
        list[num] = list[count];
        list[count] = retlist[i];
        if (count == 0) count = sumnodes;   //restart it
    }
    delete list;
    return retlist;
}

long AP_timer(void)
{
    static long time = 0;
    return ++time;
}

void ap_mark_species_rek(AP_tree *at){
    if (at->is_leaf){
        if (at->gb_node) GB_write_flag(at->gb_node,1);
        return;
    }
    ap_mark_species_rek(at->leftson);
    ap_mark_species_rek(at->rightson);
}

double ap_search_strange_species_rek(AP_tree *at,double diff_eps,double max_diff){
    if (at->is_leaf) return 0.0;
    int max_is_left = 1;
    double max = ap_search_strange_species_rek(at->leftson,diff_eps,max_diff) + at->leftlen;
    double min = ap_search_strange_species_rek(at->rightson,diff_eps,max_diff) + at->rightlen;
    if (max<min) {
        double h = max; max = min; min = h;
        max_is_left = 0;
    }
    if (min < 0) return - 2.0 * max_diff;

    if (max > min * (1.0 + max_diff) + diff_eps){
        if (max_is_left) ap_mark_species_rek(at->leftson);
        else        ap_mark_species_rek(at->rightson);
        return  - 2.0 * max_diff;
    }
    return (max + min) *.5;
}

void AP_tree::mark_long_branches(GBDATA *gb_main,double diff){
    // look for asymmetric parts of the tree and mark all species with long branches
    double max_deep =   gr.tree_depth;
    double diff_eps = max_deep * 0.05;
    GB_transaction dummy(gb_main);
    ap_search_strange_species_rek(this,diff_eps,diff);
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
            ap_mark_duplicates_rek(at->leftson, seen_species) +
            ap_mark_duplicates_rek(at->rightson, seen_species);
    }
    return 0;
}

void AP_tree::mark_duplicates(GBDATA *gb_main) {
    GB_transaction  ta(gb_main);
    GB_HASH        *seen_species = GBS_create_hash(GBT_get_species_hash_size(gb_main), 1);

    int dup_zombies = ap_mark_duplicates_rek(this, seen_species);
    if (dup_zombies) {
        aw_message(GBS_global_string("Warning: Detected %i duplicated zombies", dup_zombies));
    }
    GBS_free_hash(seen_species);
}

double ap_just_tree_rek(AP_tree *at){
    if (at->is_leaf) return 0.0;
    double bl = ap_just_tree_rek(at->leftson);
    double br = ap_just_tree_rek(at->rightson);

    double l = at->leftlen + at->rightlen;
    double diff = fabs(bl - br);
    if (l < diff * 1.1) l = diff * 1.1;
    double go = (bl + br + l) * .5;
    at->leftlen = go - bl;
    at->rightlen = go - br;
    return go;
}


void AP_tree::justify_branch_lenghs(GBDATA *gb_main){
    // shift branches to create a symmetric looking tree
    //    double max_deep = gr.tree_depth;
    GB_transaction dummy(gb_main);
    ap_just_tree_rek(this);
}

static void relink_tree_rek(AP_tree *node, GBDATA *gb_main, void (*relinker)(GBDATA *&ref_gb_node, char *&ref_name)) {
    if (node->is_leaf) {
        relinker(node->gb_node, node->name);
    }
    else {
        relink_tree_rek(node->leftson, gb_main, relinker);
        relink_tree_rek(node->rightson, gb_main, relinker);
    }
}

void AP_tree::relink_tree(GBDATA *gb_main, void (*relinker)(GBDATA *&ref_gb_node, char *&ref_name)) {
    // relinks the tree using a relinker-function
    // every node in tree is passed to relinker, relinker might modify
    // these values (ref_gb_node and ref_name) and the modified values are written back into tree

    GB_transaction dummy(gb_main);
    relink_tree_rek(this, gb_main, relinker);
}
