#include <stdio.h>
// #include <malloc.h>
#include <memory.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <awt_tree.hxx>
#include <awt.hxx>

#include <awt_tree_cmp.hxx>

#include <string>

using namespace std;

AWT_species_set_root::AWT_species_set_root(GBDATA *gb_maini,long nspeciesi){
    memset((char *)this,0,sizeof(*this));
    gb_main = gb_maini;
    nspecies = nspeciesi;
    sets = (AWT_species_set **)GB_calloc(sizeof(AWT_species_set *),(size_t)(nspecies*2+1));

    int i;
    for (i=0;i<256;i++){
        int j = i;
        int count = 0;
        while (j) {		// count bits
            if (j&1) count ++;
            j = j>>1;
        }
        diff_bits[i] = count;
    }
    species_hash = GBS_create_hash(nspecies,GB_TRUE);
    species_counter = 1;
}

AWT_species_set_root::~AWT_species_set_root(){
    int i;
    for (i=0;i<nsets;i++){
        delete sets[i];
    }
    delete sets;
}

void AWT_species_set_root::add(const char *species_name){
    if (GBS_read_hash(species_hash,species_name) ){
        aw_message(GBS_global_string("Warning: Species '%s' is found more than once in tree", species_name));
    }else{
        GBS_write_hash(species_hash,species_name,species_counter++);
    }
}

void AWT_species_set_root::add(AWT_species_set *set){
    awt_assert(nsets<nspecies*2);
    sets[nsets++] = set;
}

AWT_species_set *AWT_species_set_root::search(AWT_species_set *set,long *best_co){
    AWT_species_set *result = 0;
    register long i;
    long best_cost = 0x7fffffff;
    unsigned char *sbs = set->bitstring;
    for (i=nsets-1;i>=0;i--){
        register long j;
        register long sum = 0;
        unsigned char *rsb = sets[i]->bitstring;
        for (j=nspecies/8 -1 + 1; j>=0;j--){
            sum += diff_bits[ (sbs[j]) ^ (rsb[j]) ];
        }
        if (sum > nspecies/2) sum = nspecies - sum; // take always the minimum
        if (sum < best_cost){
            best_cost = sum;
            result = sets[i];
        }
    }
    *best_co = best_cost;
    return result;
}

int AWT_species_set_root::search(AWT_species_set *set,FILE *log_file){
    long netto_cost;
    double best_cost;
    AWT_species_set *bs = this->search(set,&netto_cost);
    best_cost = netto_cost + set->unfound_species_count * 0.0001;
    if (best_cost < bs->best_cost){
        bs->best_cost = best_cost;
        bs->best_node = set->node;
    }
    if (log_file){
        if ( netto_cost){
            fprintf(log_file, "Node '%s' placed not optimal, %li errors\n",
                    set->node->name,
                    netto_cost);
        }
    }
    return netto_cost;
}

// --------------------------------------------------------------------------------
//     AWT_species_set::AWT_species_set(AP_tree *nodei,AWT_species_set_root *ssr,char *species_name)
// --------------------------------------------------------------------------------
AWT_species_set::AWT_species_set(AP_tree *nodei,AWT_species_set_root *ssr,char *species_name){
    memset((char *)this,0,sizeof(*this));
    bitstring = (unsigned char *)GB_calloc(sizeof(char),size_t(ssr->nspecies/8)+sizeof(long)+1);
    long species_index = GBS_read_hash(ssr->species_hash,species_name);
    if (species_index){
        bitstring[species_index/8] |= 1 << (species_index %8);
    }else{
        unfound_species_count = 1;
    }
    this->node = nodei;
    best_cost = 0x7fffffff;
}

// --------------------------------------------------------------------------------
//     AWT_species_set::AWT_species_set(AP_tree *nodei,AWT_species_set_root *ssr,AWT_species_set *l,AWT_species_set *r)
// --------------------------------------------------------------------------------
AWT_species_set::AWT_species_set(AP_tree *nodei,AWT_species_set_root *ssr,AWT_species_set *l,AWT_species_set *r){
    memset((char *)this,0,sizeof(*this));
    this->node = node;
    long j = ssr->nspecies/8+1;
    bitstring = (unsigned char *)GB_calloc(sizeof(char),size_t(ssr->nspecies/8)+5);
    register long *lbits = (long *)l->bitstring;
    register long *rbits = (long *)r->bitstring;
    register long *dest = (long *)bitstring;
    for (j = ssr->nspecies/8/sizeof(long) - 1 + 1;j>=0;j--){
        dest[j] = lbits[j] | rbits[j];
    }
    unfound_species_count = l->unfound_species_count + r->unfound_species_count;
    this->node = nodei;
    best_cost = 0x7fffffff;
}

AWT_species_set::~AWT_species_set(){
    free(bitstring);
}

AWT_species_set *AWT_species_set_root::move_tree_2_ssr(AP_tree *node){
    AWT_species_set *ss;
    if (node->is_leaf){
        this->add(node->name);
        ss = new AWT_species_set(node,this,node->name);
        // 	ssr->add(ss);
    }else{
        AWT_species_set *ls = this->move_tree_2_ssr(node->leftson);
        AWT_species_set *rs = this->move_tree_2_ssr(node->rightson);
        ss = new AWT_species_set(node,this,ls,rs);
        this->add(ss);
    }
    return ss;
}

// --------------------------------------------------------------------------------
//     AWT_species_set *AWT_species_set_root::find_best_matches_info(AP_tree *tree_source,FILE *log,AW_BOOL compare_node_info)
// --------------------------------------------------------------------------------
/*	Go through all node of the source tree and search for the best
 *	matching node in dest_tree (meaning searching ssr->sets)
 *	If a match is found, set ssr->sets to this match)
 */

AWT_species_set *AWT_species_set_root::find_best_matches_info(AP_tree *tree_source,FILE *log,AW_BOOL compare_node_info){
    AWT_species_set *ss;
    if (tree_source->is_leaf){
        aw_status(this->status++/(double)this->mstatus);
        ss = new AWT_species_set(tree_source,this,tree_source->name);
        return ss;
    }
    aw_status(this->status++/(double)this->mstatus);
    AWT_species_set *ls = this->find_best_matches_info(tree_source->leftson,log,compare_node_info);
    AWT_species_set *rs = this->find_best_matches_info(tree_source->rightson,log,compare_node_info);

    ss = new AWT_species_set(tree_source,this,ls,rs); // Generate new bitstring
    if (compare_node_info){
        int mismatches = this->search(ss,log);	// Search optimal position
        delete ss->node->remark_branch;
        ss->node->remark_branch = 0;
        if (mismatches){
            char remark[20];
            sprintf(remark,"# %i",mismatches); // the #-sign is important (otherwise AWT_export_tree will not work correctly)
            ss->node->remark_branch = strdup(remark);
        }
    }else{
        if(tree_source->name){
            this->search(ss,log);	// Search optimal position
        }
    }
    delete rs;
    delete ls;
    return ss;			// return bitstring for this node
}

// --------------------------------------------------------------------------------
//     static AW_BOOL containsMarkedSpecies(const AP_tree *tree)
// --------------------------------------------------------------------------------
static AW_BOOL containsMarkedSpecies(const AP_tree *tree) {
    if (tree->is_leaf) {
        GBDATA *gb_species = tree->gb_node;
        int flag = GB_read_flag(gb_species);
        return flag!=0;
    }
    return containsMarkedSpecies(tree->leftson) || containsMarkedSpecies(tree->rightson);
}

// --------------------------------------------------------------------------------
//     GB_ERROR AWT_species_set_root::copy_node_infos(FILE *log, AW_BOOL delete_old_nodes, AW_BOOL nodes_with_marked_only)
// --------------------------------------------------------------------------------
GB_ERROR AWT_species_set_root::copy_node_infos(FILE *log, AW_BOOL delete_old_nodes, AW_BOOL nodes_with_marked_only) {
    GB_ERROR error = 0;
    long j;

    for (j=this->nsets-1;j>=0;j--){
        AWT_species_set *set = this->sets[j];
        char *old_group_name = 0;
        AW_BOOL insert_new_node = set->best_node && set->best_node->name;

        if (nodes_with_marked_only && insert_new_node) {
            int hasMarked = containsMarkedSpecies(set->node);
            if (!hasMarked) insert_new_node = AW_FALSE;
        }

        if (set->node->gb_node && (delete_old_nodes || insert_new_node)) { // There is already a node, delete old
	    if (set->node->name == 0) {
		GBDATA *gb_name = GB_find(set->node->gb_node, "group_name", 0, down_level);
		if (gb_name) {
		    set->node->name = GB_read_string(gb_name);
		}
		else {
		    set->node->name = strdup("<gb_node w/o name>");
		}
	    }	

            old_group_name = strdup(set->node->name); // store old_group_name to rename new inserted node
#if defined(DEBUG)
            printf("delete node '%s'\n", set->node->name);
#endif // DEBUG
            error = GB_delete(set->node->gb_node);
            if (error) break;
            if (log) fprintf(log,"Destination Tree not empty, destination node '%s' deleted\n",  old_group_name);
            set->node->gb_node = 0;
            set->node->name = 0;
        }
        if (insert_new_node) {
            set->node->gb_node = GB_create_container(set->node->tree_root->gb_tree,"node");
            error = GB_copy(set->node->gb_node,set->best_node->gb_node);
            if (error) break;

            GB_dump(set->node->gb_node);

            GBDATA *gb_name = GB_find(set->node->gb_node, "group_name", 0, down_level);
            gb_assert(gb_name);
            if (gb_name) {
                char *name = GB_read_string(gb_name);

                if (old_group_name && strcmp(old_group_name, name)!=0 && !delete_old_nodes) {
                    // there was a group with a different name and we are adding nodes
                    string new_group_name = (string)name+" [was: "+old_group_name+"]";
                    GB_write_string(gb_name, new_group_name.c_str());
                    delete name; name = GB_read_string(gb_name);
                }
#if defined(DEBUG)
                printf("insert node '%s'\n", name);
#endif // DEBUG
                delete name;
            }
        }
        delete old_group_name;
    }

    return error;
}

GB_ERROR AWT_move_info(GBDATA *gb_main, const char *tree_source,const char *tree_dest,const char *log_file, AW_BOOL compare_node_info, AW_BOOL delete_old_nodes, AW_BOOL nodes_with_marked_only) {
    GB_ERROR error = 0;
    GB_begin_transaction(gb_main);
    FILE *log = 0;
    if (log_file){
        log = fopen(log_file,"w");
        fprintf(log,
                "**********************************************************************\n"
                "       LOGFILE: %s Node Info From Tree '%s' to Tree '%s'\n"
                "**********************************************************************\n\n",
                delete_old_nodes ? "Moving" : "Adding",
                tree_source,tree_dest);
    }

    AP_tree *source= 0;
    AP_tree *dest= 0;

    source = new AP_tree(0);
    dest = new AP_tree(0);

    AP_tree_root *rsource= new AP_tree_root(gb_main,source,tree_source);
    AP_tree_root *rdest= new AP_tree_root(gb_main,dest,tree_dest);

    aw_openstatus("Comparing Topologies");
    do {
        aw_status("Load Tree 1");
        error = source->load(rsource, 1, GB_FALSE, GB_FALSE, 0, 0); // link to database
        if (error) break;
        aw_status("Load Tree 2");
        error = dest->load(rdest, 1, GB_FALSE, GB_FALSE, 0, 0);	// link also
        if (error) break;

        long nspecies = dest->arb_tree_leafsum2();
        AWT_species_set_root *ssr = new AWT_species_set_root(gb_main,nspecies);

        aw_status("Building Search Table for Tree 2");
        ssr->move_tree_2_ssr(dest);

        aw_status("Compare Both Trees");
        ssr->mstatus = source->arb_tree_leafsum2()*2; ssr->status = 0;
        if(ssr->mstatus<2) {
            error = GB_export_error("Destination tree has less than 3 species");
            break;
        }

        AWT_species_set *root_setl = ssr->find_best_matches_info(source->leftson,log,compare_node_info);
        AWT_species_set *root_setr = ssr->find_best_matches_info(source->rightson,log,compare_node_info);

        if (!compare_node_info){
            aw_status("Copy Node Informations");
            ssr->copy_node_infos(log, delete_old_nodes, nodes_with_marked_only);
        }
        long dummy = 0;
        AWT_species_set *new_root_setl = ssr->search(root_setl,&dummy);
        AWT_species_set *new_root_setr = ssr->search(root_setr,&dummy);
        AP_tree *new_rootl = (AP_tree *)new_root_setl->node;
        AP_tree *new_rootr = (AP_tree *)new_root_setr->node;
        new_rootl->set_root();	// set root correctly
        new_rootr->set_root();	// set root correctly
        aw_status("Save Tree");
        AP_tree *root = new_rootr;
        while (root->father) root = root->father;
        error = GBT_write_tree(gb_main,rdest->gb_tree,0,root->get_gbt_tree());
        if (!error) error = GBT_write_tree(gb_main,rsource->gb_tree,0,source->get_gbt_tree());
    }while(0);

    if (log){
#if !defined(DEBUG)
        fclose(log);
#endif // DEBUG
    }
    aw_closestatus();
    delete source;
    delete dest;
    delete rsource;
    delete rdest;

    if(error){
        aw_message(error);
        GB_abort_transaction(gb_main);
    }else{
        GB_commit_transaction(gb_main);
    }
    return error;
}
