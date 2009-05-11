#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <iostream>
#include <limits.h>
#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <awt_canvas.hxx>
#include <awt.hxx>
#include <awt_nds.hxx>

#include <awt_tree.hxx>
#include <awt_dtree.hxx>
#include <awt_tree_cb.hxx>
#include <awt_sel_boxes.hxx>

#include <awt_csp.hxx>

#include "AP_buffer.hxx"
#include "parsimony.hxx"
#include "ap_tree_nlen.hxx"
#include "pars_main.hxx"
#include "pars_dtree.hxx"

#include <list>

#if defined(DEBUG)
# define TEST_FUNCTIONS
#endif // DEBUG


using namespace std;

AW_HEADER_MAIN

GBDATA    *GLOBAL_gb_main;                          // global gb_main for arb_pars
NT_global *GLOBAL_NT;

// waaah more globals :(
AP_main *ap_main;
AWT_csp *awt_csp = 0;

AW_window *create_kernighan_window(AW_root *aw_root);

static void pars_export_tree(void){
    // GB_ERROR error = GLOBAL_NT->tree->tree_root->AP_tree::saveTree();
    GB_ERROR error = GLOBAL_NT->tree->save(0, 0, 0, 0);
    if (error) aw_message(error);
}

ATTRIBUTED(__ATTR__NORETURN, static void pars_exit(AW_window *aww)) {
    aww->get_root()->unlink_awars_from_DB(GLOBAL_gb_main);
    GB_close(GLOBAL_gb_main);
    exit(0);
}

void PARS_export_cb(AW_window *aww, AWT_canvas *, AW_CL mode){
    if (mode &1){       // export tree
        pars_export_tree();
    }
    if (mode &2) {
        //quit
        pars_exit(aww);
    }
}

void AP_user_push_cb(AW_window *aww, AWT_canvas *)
{
    ap_main->user_push();
    aww->get_root()->awar(AWAR_STACKPOINTER)->write_int(ap_main->user_push_counter);
}

void AP_user_pop_cb(AW_window *aww,AWT_canvas *ntw)
{
    if (ap_main->user_push_counter<=0) {
        aw_message("No tree on stack.");
        return;
    }
    ap_main->user_pop();
    (*ap_main->tree_root)->compute_tree(GLOBAL_gb_main);
    ntw->zoom_reset();
    ntw->refresh();
    (*ap_main->tree_root)->test_tree();
    aww->get_root()->awar(AWAR_STACKPOINTER)->write_int(ap_main->user_push_counter);
    pars_export_tree();

    if (ap_main->user_push_counter <= 0) { // last tree was popped => push again
        AP_user_push_cb(aww, ntw);
    }
}

struct InsertData {
    AP_BOOL quick_add_flag;
    AP_BOOL singlestatus; // update status for single species addition
    int     abort_flag;
    long    maxspecies;
    long    currentspecies;

    InsertData(AP_BOOL quick)
        : quick_add_flag(quick)
        , singlestatus(AP_FALSE)
        , abort_flag(AP_FALSE)
        , maxspecies(0)
        , currentspecies(0)
    {
    }
};

static long insert_species_in_tree_test(const char *key,long val, void *cd_isits) {
    if (!val) return val;

    InsertData *isits = (InsertData*)cd_isits;
    if (isits->abort_flag) return val;

    AP_tree *tree = (*ap_main->tree_root);
    GBDATA *gb_node = (GBDATA *)val;

    GB_begin_transaction(GLOBAL_gb_main);

    GBDATA *gb_data = GBT_read_sequence(gb_node,ap_main->use);
    if (!gb_data)
    {
        sprintf(AW_ERROR_BUFFER,"Warning Species '%s' has no sequence '%s'",
                key,ap_main->use);
        aw_message();
        GB_commit_transaction(GLOBAL_gb_main);
        return val;
    }

    AP_tree *leaf = tree->dup();
    leaf->gb_node = gb_node;
    leaf->name = strdup(key);
    leaf->is_leaf = GB_TRUE;

    leaf->sequence = leaf->tree_root->sequence_template->dup();
    leaf->sequence->set(GB_read_char_pntr(gb_data));
    GB_commit_transaction (GLOBAL_gb_main);

    if (leaf->sequence->real_len() < MIN_SEQUENCE_LENGTH)
    {
        sprintf(AW_ERROR_BUFFER,
                "Species %s has too short sequence (%i, minimum is %i)",
                key,
                (int)leaf->sequence->real_len(),
                MIN_SEQUENCE_LENGTH);
        aw_message();
        delete leaf;
        return val;
    }

    if (isits->singlestatus) {
        sprintf(AW_ERROR_BUFFER,
                "Inserting Species %s (%li:%li)",
                key,
                ++isits->currentspecies,
                isits->maxspecies);
        aw_openstatus(AW_ERROR_BUFFER);
    }
    else {
        isits->abort_flag = aw_status(++isits->currentspecies/(double)isits->maxspecies);
    }

    {
        AP_FLOAT best_val = 9999999999999.0;
        AP_FLOAT this_val;
        int      cnt      = 0;

        while (cnt<2) {
            char buf[200];

            sprintf(buf,"Pre-optimize (pars=%f)",best_val);

            aw_status(buf);
            this_val = rootEdge()->nni_rek(AP_FALSE,isits->abort_flag,-1);
            if (this_val<best_val) {
                best_val = this_val;
                cnt      = 0;
            }
            else if (this_val==best_val) {
                cnt++;
            }
            else {
                cnt = 0;
            }
        }
    }

    rootEdge()->dumpNNI=2;

    aw_status("Searching best position");

    AP_tree      **blist;
    AP_tree_nlen  *bl,*blf;
    long           bsum    = 0;
    long           counter = 0;

    tree->buildBranchList(blist,bsum,AP_TRUE,-1);   // get all branches
    AP_sequence::global_combineCount = 0;

    AP_tree *bestposl = tree->leftson;
    AP_tree *bestposr = tree->rightson;
    leaf->insert(bestposl);
    AP_FLOAT best_parsimony,akt_parsimony;
    best_parsimony = akt_parsimony = (*ap_main->tree_root)->costs();

    for (counter = 0; !isits->abort_flag && blist[counter]; counter += 2)
    {
        if (isits->singlestatus && (counter & 0xf) == 0) {
            isits->abort_flag = (AP_BOOL)aw_status(counter/(bsum*2.0));
        }

        bl = (AP_tree_nlen *)blist[counter];
        blf = (AP_tree_nlen *)blist[counter+1];
        if (blf->father == bl )
        {
            bl = (AP_tree_nlen *)blist[counter+1];
            blf = (AP_tree_nlen *)blist[counter];
        }

        //      if (blf->father) {  //->father
        //          blf->set_root();
        //      }

        if (bl->father)     //->father
        {
            bl->set_root();
        }

        leaf->moveTo(bl,0.5);
        akt_parsimony = (*ap_main->tree_root)->costs();

        if (akt_parsimony<best_parsimony)
        {
            best_parsimony = akt_parsimony;
            bestposl = bl;
            bestposr = blf;
        }
    }
    delete blist;
    blist = 0;
    if (bestposl->father != bestposr) bestposl = bestposr;
    leaf->moveTo(bestposl,0.5);

    // calculate difference in father-sequence

    int baseDiff = 0;

    {
        AP_tree_nlen *fath = ((AP_tree_nlen*)bestposl)->Father()->Father();
        char *seq_with_leaf;
        char *seq_without_leaf;
        long len_with_leaf;
        long len_without_leaf;
        long i;

        seq_with_leaf = fath->getSequence();
        len_with_leaf = fath->sequence->sequence_len;


        ap_main->push();
        leaf->remove();

        seq_without_leaf = fath->getSequence();
        len_without_leaf = fath->sequence->sequence_len;


        ap_main->pop();


        if (len_with_leaf==len_without_leaf) {
            for (i=0; i<len_with_leaf; i++) {
                if (seq_with_leaf[i]!=seq_without_leaf[i]) {
                    baseDiff++;
                }
            }
        }
        else {
            cout << "Laenge der Sequenz hat sich geaendert!!!";
        }

        delete [] seq_with_leaf;
        delete [] seq_without_leaf;
    }

    rootEdge()->countSpecies();

    //    printf("Combines: %i\n",global_combineCount);

    if (!isits->quick_add_flag ) {
        int           deep    = -1;
        AP_tree_nlen *bestpos = (AP_tree_nlen*)bestposl;
        AP_tree_edge *e       = bestpos->edgeTo(bestpos->Father());

        if ((isits->currentspecies & 0xf ) == 0)  deep = -1;

        aw_status("optimization");
        e->dumpNNI = 1;
        e->distInsertBorder = e->distanceToBorder(INT_MAX,(AP_tree_nlen*)leaf);
        e->basesChanged = baseDiff;
        e->nni_rek(AP_FALSE, isits->abort_flag, deep);
        e->dumpNNI = 0;
    }

    return val;
}

static int sort_sequences_by_length(const char*, long leaf0_ptr, const char*, long leaf1_ptr) {
    AP_tree *leaf0 = (AP_tree*)leaf0_ptr;
    AP_tree *leaf1 = (AP_tree*)leaf1_ptr;

    AP_FLOAT len0 = leaf0->sequence->real_len();
    AP_FLOAT len1 = leaf1->sequence->real_len();

    return len0<len1 ? 1 : (len0>len1 ? -1 : 0); // longest sequence first
}

static long transform_gbd_to_leaf(const char *key,long val, void *) {
    if (!val) return val;

    AP_tree *tree    = (*ap_main->tree_root);
    GBDATA  *gb_node = (GBDATA *)val;
    GBDATA  *gb_data = GBT_read_sequence(gb_node,ap_main->use);
    
    if (!gb_data) {
        sprintf(AW_ERROR_BUFFER,"Warning Species '%s' has no sequence '%s'",
                key,ap_main->use);
        aw_message();
        return 0;
    }
    AP_tree *leaf = tree->dup();
    leaf->gb_node = gb_node;
    leaf->name = strdup(key);
    leaf->is_leaf = GB_TRUE;
    leaf->sequence = leaf->tree_root->sequence_template->dup();
    leaf->sequence->set(GB_read_char_pntr(gb_data));
    return (long)leaf;
}

static AP_tree *insert_species_in_tree(const char *key, AP_tree *leaf, void *cd_isits)
{
    if (!leaf) return leaf;

    InsertData *isits = (InsertData*)cd_isits;
    if (isits->abort_flag) return leaf;

    AP_tree *tree = (*ap_main->tree_root);


    if (leaf->sequence->real_len() < MIN_SEQUENCE_LENGTH) {
        sprintf(AW_ERROR_BUFFER,"Species %s has too short sequence (%i, minimum is %i)",
                key,    (int)leaf->sequence->real_len(),    MIN_SEQUENCE_LENGTH);
        aw_message();
        delete leaf;
        return 0;
    }

    ++isits->currentspecies;
    if (isits->singlestatus) {
        sprintf(AW_ERROR_BUFFER, "Inserting Species %s (%li:%li)",
                key, isits->currentspecies, isits->maxspecies);
        aw_openstatus(AW_ERROR_BUFFER);
    }
    aw_status("Searching best position");

    AP_tree      **blist;
    AP_tree_nlen  *bl,*blf;
    long           bsum    = 0;
    long           counter = 0;

    tree->buildBranchList(blist,bsum,AP_TRUE,-1);       // get all branches
    AP_sequence::global_combineCount = 0;

    AP_tree *bestposl = tree->leftson;
    AP_tree *bestposr = tree->rightson;
    leaf->insert(bestposl);
    AP_FLOAT best_parsimony,akt_parsimony;
    best_parsimony = akt_parsimony = (*ap_main->tree_root)->costs();

    for (counter = 0;!isits->abort_flag && blist[counter];counter += 2) {
        if (isits->singlestatus && (counter & 0xf) == 0) {
            isits->abort_flag = (AP_BOOL)aw_status(counter/(bsum*2.0));
        }
        bl = (AP_tree_nlen *)blist[counter];
        blf = (AP_tree_nlen *)blist[counter+1];
        if (blf->father == bl ) {
            bl = (AP_tree_nlen *)blist[counter+1];
            blf = (AP_tree_nlen *)blist[counter];
        }

        if (bl->father) {   //->father
            bl->set_root();
        }

        leaf->moveTo(bl,0.5);
        akt_parsimony = (*ap_main->tree_root)->costs();
        if (akt_parsimony < best_parsimony) {
            best_parsimony = akt_parsimony;
            bestposl = bl;
            bestposr = blf;
        }

    }
    delete blist;blist = 0;
    if (bestposl->father != bestposr){
        bestposl = bestposr;
    }
    leaf->moveTo(bestposl,0.5);

    if (!isits->quick_add_flag) {
        int deep = 5;
        if ( (isits->currentspecies & 0xf ) == 0)  deep = -1;
        aw_status("optimization");
        ((AP_tree_nlen *)bestposl->father)->
            nn_interchange_rek(AP_FALSE, isits->abort_flag, deep, AP_BL_NNI_ONLY, GB_TRUE);
    }
    AP_tree *brother = leaf->brother();
    if (    brother->is_leaf &&     // brother is a short sequence
            brother->sequence->real_len() *2  < leaf->sequence->real_len() &&
            leaf->father->father){      // There are more than two species
        brother->remove();
        leaf->remove();
        isits->currentspecies--;
        char *label = GBS_global_string_copy("2:%s",leaf->name);
        insert_species_in_tree(label, leaf, cd_isits); // reinsert species
        delete label;
        isits->currentspecies--;
        label = GBS_global_string_copy("shortseq:%s",brother->name);
        insert_species_in_tree(label, brother, cd_isits);  // reinsert short sequence
        delete label;
    }

    if (!isits->singlestatus) {
        isits->abort_flag |= aw_status(isits->currentspecies/(double)isits->maxspecies);
    }

    return leaf;
}

static long hash_insert_species_in_tree(const char *key, long leaf, void *cd_isits) {
    return (long)insert_species_in_tree(key, (AP_tree*)leaf, cd_isits);
}

static long count_hash_elements(const char *,long val, void *cd_isits) {
    if (val) {
        InsertData *isits = (InsertData*)cd_isits;
        isits->maxspecies++;
    }
    return val;
}

static void nt_add(AW_window *, AWT_canvas *ntw, int what, AP_BOOL quick, int test)
{
    GB_begin_transaction(GLOBAL_gb_main);
    GB_HASH *hash = 0;
    GB_ERROR error = 0;
    AP_tree *oldrootleft = (*ap_main->tree_root)->leftson;
    AP_tree *oldrootright = (*ap_main->tree_root)->rightson;
    aw_openstatus("Search selected species");

    if (what)
    {
        char *name = GBT_readOrCreate_string(GLOBAL_gb_main, AWAR_SPECIES_NAME, "");
        if (name && strlen(name)) {
            GBDATA *gb_species = GBT_find_species(GLOBAL_gb_main,name);
            if (gb_species) {
                hash = GBS_create_hash(10, GB_MIND_CASE);
                GBS_write_hash(hash,name,(long)gb_species);
            }
            else {
                error = "Error: Selected Species not found";
            }
        }
        else {
            error= "Please select an species"
                "   to select an species:   1. arb_edit: enable global cursor and select sequence"
                "               2. arb_ntree: species/search and select species";
        }
        free(name);
    }
    else {
        hash = GBT_create_marked_species_hash(GLOBAL_gb_main);
    }
    GB_commit_transaction(GLOBAL_gb_main);

    if (!error) {
        NT_remove_species_in_tree_from_hash(*ap_main->tree_root,hash);

        InsertData isits(quick);

        GBS_hash_do_loop(hash, count_hash_elements, &isits);

        aw_openstatus(GBS_global_string("Adding %li species", isits.maxspecies));

        if (test) {
            GBS_hash_do_loop(hash, insert_species_in_tree_test, &isits);
        }
        else {
            aw_status("reading database");
            GB_begin_transaction(GLOBAL_gb_main);
            GBS_hash_do_loop(hash, transform_gbd_to_leaf, NULL);
            GB_commit_transaction(GLOBAL_gb_main);
            GBS_hash_do_sorted_loop(hash, hash_insert_species_in_tree, sort_sequences_by_length, &isits);
        }
        if (!quick ) {
            aw_status("final optimization");
            rootEdge()->nni_rek(AP_FALSE,isits.abort_flag,-1,GB_FALSE, AP_BL_NNI_ONLY);
        }

        aw_status("Calculating Branch lengths");
        rootEdge()->nni_rek(AP_FALSE,isits.abort_flag,-1, GB_FALSE ,AP_BL_BL_ONLY);

        rootNode()->test_tree();
        rootNode()->compute_tree(GLOBAL_gb_main);

        if (oldrootleft->father == oldrootright) oldrootleft->set_root();
        else                     oldrootright->set_root();

        aw_closestatus();
    }

    if (hash) GBS_free_hash(hash);
    if (error) aw_message(error);

    AWT_TREE(ntw)->resort_tree(0);
    ntw->zoom_reset();
    if (!error) pars_export_tree();
}

// -----------------------------------------
//      Adding partial sequences to tree
// -----------------------------------------

class PartialSequence {
    GBDATA          *gb_species;
    mutable AP_tree *self;            // self converted to leaf (ready for insertion)
    const AP_tree   *best_full_match; // full sequence position which matched best
    long             overlap;         // size of overlapping region
    long             penalty;         // weighted mismatches
    bool             released;
    bool             multi_match;
    string           multi_list;      // list of equal-rated insertion-points (not containing self)

    AP_tree *get_self() const {
        if (!self) {
            ap_assert(!released); // request not possible, because leaf has already been released!
            
            self = (AP_tree*)transform_gbd_to_leaf(GBT_read_name(gb_species), (long)gb_species, NULL);
            ap_assert(self);
        }
        return self;
    }

public:
    PartialSequence(GBDATA *gb_species_)
        : gb_species(gb_species_), self(0), best_full_match(0)
        , overlap(0) , penalty(LONG_MAX), released(false), multi_match(false)
    {
    }

    ~PartialSequence() { ap_assert(self == 0); }

    GBDATA *get_species() const { return gb_species; }
    const AP_tree *get_best_match() const { return best_full_match; }
    AP_FLOAT get_branchlength() const { return AP_FLOAT(penalty)/overlap; }
    void test_match(const AP_tree *leaf_full);
    bool is_multi_match() const { return multi_match; }

    const char *get_name() const {
        const char *name = get_self()->name;
        ap_assert(name);
        return name;
    }

    string get_multilist() const {
        ap_assert(is_multi_match());
        return string(best_full_match->name)+multi_list;
    }

    AP_tree *release() {
        AP_tree *s = self;
        self       = 0;
        released   = true;
        return s;
    }

    void dump() const {
        printf("best match for '%s' is '%s' (overlap=%li penalty=%li)\n",
               get_name(), best_full_match->name,
               overlap, penalty);
    }

};

void PartialSequence::test_match(const AP_tree *leaf_full) {
    long curr_overlap;
    long curr_penalty;

    leaf_full->sequence->partial_match(get_self()->sequence, &curr_overlap, &curr_penalty);

    bool better = false;

    if (curr_overlap > overlap) {
        better = true;
    }
    else if (curr_overlap == overlap) {
        if (curr_penalty<penalty) {
            better = true;
        }
        else if (curr_penalty == penalty) {
            // found two equal-rated insertion points -> store data for warning
#if defined(DEBUG)
            if (!multi_match) dump();
            printf("Another equal match is against '%s' (overlap=%li penalty=%li)\n", leaf_full->name, curr_overlap, curr_penalty);
#endif // DEBUG
            //             aw_message("Several full sequences match equal against one partial sequence");
            //             aw_message("Please save a copy of the current database and inform Ralf!!!!");

            multi_match  = true;
            multi_list.append(1, '/');
            multi_list.append(leaf_full->name);
        }
    }

    if (better) {
        overlap         = curr_overlap;
        penalty         = curr_penalty;
        best_full_match = leaf_full;
        multi_match     = false;
        multi_list      = "";
    }
}

static GB_ERROR nt_best_partial_match_rek(list<PartialSequence>& partial, AP_tree *tree) {
    GB_ERROR error = 0;

    if (tree) {
        if (tree->is_leaf && tree->name) {
            if (tree->gb_node) {
                int is_partial = GBT_is_partial(tree->gb_node, 0, true); // marks undef as 'full sequence'
                if (is_partial == 0) { // do not consider other partial sequences
                    list<PartialSequence>::iterator i = partial.begin();
                    list<PartialSequence>::iterator e = partial.end();
                    for (;  i != e; ++i) {
                        i->test_match(tree);
                    }
                }
                else if (is_partial == -1) {
                    error = GB_await_error();
                }
            }
        }
        else {
            error             = nt_best_partial_match_rek(partial, tree->leftson);
            if (!error) error = nt_best_partial_match_rek(partial, tree->rightson);
        }
    }
    return error;
}

static void count_partial_and_full(AP_tree *at, int *partial, int *full, int *zombies, int default_value, int define_if_undef) {
    if (at->is_leaf) {
        if (at->gb_node) {
            int is_partial = GBT_is_partial(at->gb_node, default_value, define_if_undef);
            if (is_partial) ++(*partial);
            else ++(*full);
        }
        else {
            ++(*zombies);
        }
    }
    else {
        count_partial_and_full(at->leftson,  partial, full, zombies, default_value, define_if_undef);
        count_partial_and_full(at->rightson, partial, full, zombies, default_value, define_if_undef);
    }
}

static AP_tree *find_least_deep_leaf(AP_tree *at, int depth, int *min_depth) {
    if (depth >= *min_depth) {
        return 0; // already found better or equal
    }

    if (at->is_leaf) {
        if (at->gb_node) {
            *min_depth = depth;
            return at;
        }
        return 0;
    }

    AP_tree *left  = find_least_deep_leaf(at->leftson, depth+1, min_depth);
    AP_tree *right = find_least_deep_leaf(at->rightson, depth+1, min_depth);

    return right ? right : left;
}

static void nt_add_partial(AW_window */*aww*/, AWT_canvas *ntw) {
    GB_begin_transaction(GLOBAL_gb_main);
    GB_ERROR error                    = 0;
    int      full_marked_sequences    = 0;
    int      partial_marked_sequences = 0;
    int      no_data                  = 0; // no data in alignment

    aw_openstatus("Adding partial sequences");

    {
        list<PartialSequence> partial;
        {
            GB_HASH *partial_hash = GBS_create_hash(GBT_get_species_hash_size(GLOBAL_gb_main), GB_MIND_CASE);
            int      marked_found = 0;

            for (GBDATA *gb_marked = GBT_first_marked_species(GLOBAL_gb_main);
                 !error && gb_marked;
                 gb_marked = GBT_next_marked_species(gb_marked))
            {
                ++marked_found;

                if (GBT_read_sequence(gb_marked,ap_main->use)) { // species has sequence in alignment
                    const char *name = GBT_read_name(gb_marked);

                    switch (GBT_is_partial(gb_marked, 1, true)) { // marks undef as 'partial sequence'
                        case 0: { // full sequences
                            aw_message(GBS_global_string("'%s' is a full sequence (cannot add partial)", name));
                            ++full_marked_sequences;
                            break;
                        }
                        case 1:     // partial sequences
                            ++partial_marked_sequences;
                            GBS_write_hash(partial_hash, name, (long)gb_marked);
                            break;
                        case -1:    // error
                            error = GB_await_error();
                            break;
                        default :
                            ap_assert(0);
                            break;
                    }
                }
                else {
                    no_data++;
                }
            }

            if (!error && !marked_found) error = "There are no marked species";
            
            if (!error) {
                // skip all species which are in tree
                NT_remove_species_in_tree_from_hash(*ap_main->tree_root,partial_hash);

                // build partial list from hash
                long val;
                for (GBS_hash_first_element(partial_hash, 0, &val);
                     val;
                     GBS_hash_next_element(partial_hash, 0, &val))
                {
                    partial.push_back(PartialSequence((GBDATA*)val));
                }

                int partials_already_in_tree = partial_marked_sequences - partial.size();
                
                if (no_data>0) aw_message(GBS_global_string("%i marked species have no data in '%s'", no_data, ap_main->use));
                if (full_marked_sequences>0) aw_message(GBS_global_string("%i marked species are declared full sequences", full_marked_sequences));
                if (partials_already_in_tree>0) aw_message(GBS_global_string("%i marked species are already in tree", partials_already_in_tree));

                if (partial.empty()) error = "No species left to add";
            }
        }

        if (!error) {
            // find best matching full sequence for each partial sequence
            error = nt_best_partial_match_rek(partial, *ap_main->tree_root);

            list<PartialSequence>::iterator i = partial.begin();
            list<PartialSequence>::iterator e = partial.end();
            
#if defined(DEBUG)
            // show results :
            for (; i != e; ++i) i->dump();
            i = partial.begin();
#endif // DEBUG

            for (; i != e && !error; ++i) {
                const char *name = i->get_name();
                
                if (i->is_multi_match()) {
                    aw_message(GBS_global_string("Insertion of '%s' is ambiguous.\n"
                                                 "(took first of equal scored insertion points: %s)",
                                                 name, i->get_multilist().c_str()));
                }

                AP_tree *part_leaf  = i->release();
                AP_tree *full_seq   = const_cast<AP_tree*>(i->get_best_match());
                AP_tree *brother    = full_seq->brother();
                int      is_partial = 0;
                AP_tree *target     = 0;

                if (brother->is_leaf) {
                    if (brother->gb_node) {
                        is_partial = GBT_is_partial(brother->gb_node, 0, 1);

                        if (is_partial) { // brother is partial sequence
                            target = brother; // insert as brother of brother
                        }
                        else {
                            target = full_seq; // insert as brother of full_seq
                        }
                    }
                    else {
                        error = "There are zombies in your tree - please remove them";
                    }
                }
                else {
                    int partial_count = 0;
                    int full_count    = 0;
                    int zombie_count  = 0;

                    count_partial_and_full(brother, &partial_count, &full_count, &zombie_count, 0, 1);

                    if (zombie_count) {
                        error = "There are zombies in your tree - please remove them";
                    }
                    else if (full_count) {
                        // brother is a subtree containing full sequences
                        // -> add new brother to full_seq found above
                        target = full_seq;
                    }
                    else {      // brother subtree only contains partial sequences
                        // find one of the least-deep leafs
                        int depth  = INT_MAX;
                        target     = find_least_deep_leaf(brother, 0, &depth);
                        is_partial = 1;
                    }
                }


                if (!error) {
#if defined(DEBUG)
                    printf("inserting '%s'\n", name);
#endif // DEBUG
                    part_leaf->insert(target);
                    
                    // we need to create the sequence of the father node!
                    AP_tree *father = part_leaf->father;
                    father->costs();

                    // ensure full-sequence is always on top
                    if (father->rightson == target) {
                        father->swap_sons();
                    }

                    if (!error) {
                        // now correct the branch lengths.
                        // First calc the original branchlen
                        GBT_LEN orglen = father->get_branchlength()+target->get_branchlength();

                        if (is_partial) { // we now have a subtree of partial sequences
                            brother->set_branchlength(orglen); // each partial sequence has it's branchlength.
                            father->set_branchlength(0); // all father branches are zero length
                        }
                        else { // we have a subtree of one full+one partial sequence
                            father->set_branchlength(orglen); // father branch represents original length (w/o partial seq)
                            full_seq->set_branchlength(0); // full seq has no sub-branch length
                        }
                        part_leaf->set_branchlength(i->get_branchlength());
                        printf("Adding with branchlength=%f\n", i->get_branchlength());
                    }
                }
                else {
                    delete part_leaf;
                }
            }
        }
    }

    aw_closestatus();

    if (full_marked_sequences) {
        aw_message(GBS_global_string("%i marked full sequences were not added", full_marked_sequences));
    }

    if (error) {
        aw_message(error);
        GB_abort_transaction(GLOBAL_gb_main);
    }
    else {
        GB_commit_transaction(GLOBAL_gb_main);
    }

//     AWT_TREE(ntw)->resort_tree(0);
    ntw->zoom_reset();
    if (!error) pars_export_tree();
}

// -----------------------------
//      add marked / seleted
// -----------------------------

// normal versions :

static void NT_add(AW_window * aww, AWT_canvas *ntw, int what) {
    // what == 0 marked  ==1 selected
    nt_add(aww,ntw,what,AP_FALSE,0);
}

static void NT_quick_add(AW_window * aww, AWT_canvas *ntw, int what) {
    // what == 0 marked  ==1 selected
    nt_add(aww,ntw,what,AP_TRUE,0);
}

// test versions :

#if defined(TEST_FUNCTIONS)
static void NT_add_test(AW_window * aww, AWT_canvas *ntw, int what) {
    // what == 0 marked  ==1 selected
    nt_add(aww,ntw,what,AP_FALSE,1);
}

static void NT_quick_add_test(AW_window * aww, AWT_canvas *ntw, int what) {
    // what == 0 marked  ==1 selected
    nt_add(aww,ntw,what,AP_TRUE,1);
}
#endif // TEST_FUNCTIONS

// -----------------------------------------
//      remove and add marked / selected
// -----------------------------------------

static void NT_radd_internal(AW_window * aww, AWT_canvas *ntw, int what, AP_BOOL quick, int test) {
    // what == 0 marked  ==1 selected

    AW_awar *awar_best_pars = aww->get_root()->awar(AWAR_BEST_PARSIMONY);
    int      oldparsval     = awar_best_pars->read_int();

    NT_remove_leafs(0,ntw,AWT_REMOVE_BUT_DONT_FREE | AWT_REMOVE_MARKED);

    // restore old parsimony value (otherwise the state where species were removed would count) :
    awar_best_pars->write_int(oldparsval);

    nt_add(aww,ntw,what,quick,test);
}

// normal versions :

static void NT_radd(AW_window * aww, AWT_canvas *ntw, int what) {
    // what == 0 marked  ==1 selected
    NT_radd_internal(aww, ntw, what, AP_FALSE, 0);
}

static void NT_rquick_add(AW_window * aww, AWT_canvas *ntw, int what) {
    // what == 0 marked  ==1 selected
    NT_radd_internal(aww, ntw, what, AP_TRUE, 0);
}

// static void NT_radd_test_internal(AW_window * aww, AWT_canvas *ntw, int what, AP_BOOL quick) {
//     // what == 0 marked  ==1 selected
//     NT_remove_leafs(0,ntw,AWT_REMOVE_BUT_DONT_FREE | AWT_REMOVE_MARKED);
//     nt_add(aww,ntw,what,quick,1);
// }

// test versions :

#if defined(TEST_FUNCTIONS)
static void NT_radd_test(AW_window * aww, AWT_canvas *ntw, int what) {
    // what == 0 marked  ==1 selected
    NT_radd_internal(aww, ntw, what, AP_FALSE, 1);
}

static void NT_rquick_add_test(AW_window * aww, AWT_canvas *ntw, int what) {
    // what == 0 marked  ==1 selected
    NT_radd_internal(aww, ntw, what, AP_TRUE, 1);
}
#endif // TEST_FUNCTIONS

// --------------------------------------------------------------------------------
// Add Partial sequences
// --------------------------------------------------------------------------------


static void NT_partial_add(AW_window *aww, AW_CL cl_ntw, AW_CL) {
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    nt_add_partial(aww, ntw);
}

// --------------------------------------------------------------------------------

static void NT_branch_lengths(AW_window *, AWT_canvas *ntw) {
    aw_openstatus("Calculating Branch Lengths");
    int abort_flag = AP_FALSE;
    rootEdge()->nni_rek(AP_FALSE, abort_flag, -1, GB_FALSE, AP_BL_BL_ONLY);

    aw_closestatus();
    AWT_TREE(ntw)->resort_tree(0);

    ntw->zoom_reset();
    ntw->refresh();
    pars_export_tree();
}

static void NT_bootstrap(AW_window *aw,AWT_canvas *ntw,AW_CL limit_only)
{
    AW_POPUP_HELP(aw, (AW_CL)"pa_bootstrap.hlp");
    aw_openstatus("Calculating Bootstrap Limit");
    int abort_flag = AP_FALSE;
    rootEdge()->nni_rek(AP_FALSE, abort_flag, -1, GB_FALSE,
                        limit_only ? AP_BL_BOOTSTRAP_LIMIT : AP_BL_BOOTSTRAP_ESTIMATE);
    aw_closestatus();

    AWT_TREE(ntw)->resort_tree(0);

    AWT_TREE(ntw)->tree_root_display = AWT_TREE(ntw)->tree_root;
    ntw->zoom_reset();
    ntw->refresh();
    pars_export_tree();
}

static void NT_optimize(AW_window *, AWT_canvas *ntw)
{
    aw_openstatus("Optimize Tree");

    PARS_optimizer_cb((*ap_main->tree_root));
    rootNode()->test_tree();

    aw_openstatus("Calculating Branch Lengths");
    int abort_flag = AP_FALSE;
    rootEdge()->nni_rek(AP_FALSE, abort_flag, -1, GB_FALSE, AP_BL_BL_ONLY);
    AWT_TREE(ntw)->resort_tree(0);
    rootNode()->compute_tree(GLOBAL_gb_main);
    aw_closestatus();
    ntw->zoom_reset();
    ntw->refresh();
    pars_export_tree();
}

static void NT_recursiveNNI(AW_window *,AWT_canvas *ntw)
{
    aw_openstatus("Recursive NNI");
    int abort_flag = AP_FALSE;

    AP_FLOAT thisPars = rootNode()->costs();
    AP_FLOAT lastPars = -1.0;

    aw_status(GBS_global_string("Old parsimony: %f", thisPars));
    while (thisPars!=lastPars && abort_flag == AP_FALSE) {
        lastPars    = thisPars;
        thisPars    = rootEdge()->nni_rek(AP_FALSE, abort_flag, -1, GB_TRUE);
        abort_flag |= aw_status(GBS_global_string("New Parsimony: %f",thisPars));
    }

    aw_status("Calculating Branch Lengths");
    abort_flag = AP_FALSE;
    rootEdge()->nni_rek(AP_FALSE, abort_flag, -1, GB_FALSE, AP_BL_BL_ONLY);

    AWT_TREE(ntw)->resort_tree(0);
    rootNode()->compute_tree(GLOBAL_gb_main);
    aw_closestatus();
    ntw->zoom_reset();
    ntw->refresh();
    pars_export_tree();
}

static AW_window *NT_create_tree_setting(AW_root *aw_root)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "SAVE_DB","SAVE ARB DB");
    aws->load_xfig("awt/tree_settings.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"nt_tree_settings.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("button");
    aws->auto_space(10,10);
    aws->label_length(30);

    aws->label("Base Line Width");
    aws->create_input_field(AWAR_DTREE_BASELINEWIDTH,4);
    aws->at_newline();

    aws->label("Relativ vert. Dist");
    aws->create_input_field(AWAR_DTREE_VERICAL_DIST,4);
    aws->at_newline();

    aws->label("Auto Jump (list tree)");
    aws->create_toggle(AWAR_DTREE_AUTO_JUMP);
    aws->at_newline();


    return (AW_window *)aws;

}

static void PA_focus_cb(AW_window *,GBDATA *gb_main_par)
{
    GB_transaction dummy(gb_main_par);
}

/******************************************************
    Test-Funktionen
******************************************************/

#if defined(TEST_FUNCTIONS)
static void refreshTree(AWT_canvas *ntw) {
    GB_transaction gb_dummy(ntw->gb_main);
    
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    GB_ERROR error = AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    if (error) aw_message(error);
    ntw->zoom_reset();
    ntw->refresh();
}

static void testTree(AP_tree_nlen *tree,int *nodeCount, int *edgeCount)
{
    tree->test();
    (*nodeCount)++;

    if (tree->father)   // we do only test edges if were not at root
    {
        AP_tree_edge *e =0;
        int skipTest=0;

        if (tree->father->father)
        {
            e = tree->edgeTo(tree->Father());
        }
        else        // son of root!
        {
            if (tree->father->leftson==(AP_tree*)tree)  // to not test twice
            {
                e = tree->edgeTo(tree->Brother());
            }
            else
            {
                skipTest=1;
            }
        }

        if (!skipTest)
        {
            if (e)
            {
                int dist2bord;

                e->test();
                (*edgeCount)++;

                dist2bord = e->distanceToBorder();
                if (dist2bord != e->Distance())
                {
                    cout << *e << "distance error: should=" << dist2bord << " is=" << e->Distance() << endl;
                }
            }
            else
            {
                cout << *tree << "has no edge to father\n";
            }
        }
    }

    if (!tree->is_leaf)
    {
        testTree((AP_tree_nlen*)tree->leftson,nodeCount,edgeCount);
        testTree((AP_tree_nlen*)tree->rightson,nodeCount,edgeCount);
    }
}

static void TEST_testWholeTree(AW_window *, AWT_canvas *)
// Tests the whole tree structure (edges and nodes) for consistency
{
    AP_tree_nlen *root  = (AP_tree_nlen*)*ap_main->tree_root;
    int           nodes = 0;
    int           edges = 0;

    if (root->father)
    {
        int m = INT_MAX;

        cout << "OOPS! Root has father:\n" << *root << '\n';
        while (m-- && root->father) root = root->Father();

        if (root->father)
        {
            cout << "Root node lost between:\n"
                 << *root << '\n'
                 << *(root->Father()) << '\n';
        }
        else
        {
            cout << "Found a root:\n" << *root << "\n Let's test from here:\n";
        }
    }

    root->test_tree();
    testTree(root, &nodes, &edges);

    cout << "Nodes tested: " << nodes << '\n'
         << "Edges tested: " << edges << '\n';
}
#endif // TEST_FUNCTIONS

static int dumpNodes(AP_tree_nlen *node)
{
    int cnt = 1;

    cout << *node
         << "(llen=" << node->leftlen
         << ",rlen=" << node->rightlen << ")\n";

    if (!node->is_leaf)
    {
        cnt += dumpNodes(node->Leftson());
        cnt += dumpNodes(node->Rightson());
    }

    return cnt;
}

#if defined(TEST_FUNCTIONS)
static void TEST_dumpNodes(AW_window *, AWT_canvas *)
{
    AP_tree_nlen *root = (AP_tree_nlen*)*ap_main->tree_root;
    int cnt;

    cout << "----- Dumping all nodes in tree\n";
    cnt = dumpNodes(root);
    cout << "----- " << cnt << "nodes dumped!\n";
}
#endif // TEST_FUNCTIONS

static void setBranchlens(AP_tree_nlen *node,double newLen)
{
    node->setBranchlen(newLen,newLen);

    if (!node->is_leaf)
    {
        setBranchlens(node->Leftson(),newLen);
        setBranchlens(node->Rightson(),newLen);
    }
}
#if defined(TEST_FUNCTIONS)
static void TEST_setBranchlen(AW_window *, AWT_canvas *ntw)
{
    AP_tree_nlen *root = (AP_tree_nlen*)*ap_main->tree_root;

    setBranchlens(root,1.0);
    refreshTree(ntw);
}
#endif // TEST_FUNCTIONS

/*
static AP_tree_nlen *getRandomSonOf(AP_tree_nlen *daddy, int innerNodeAllowed,int rootAllowed)
{
    if (daddy->is_leaf) return daddy;
    int r = rand() % (innerNodeAllowed && rootAllowed ? 3 : 2);
    if (r==2) return daddy;
    return getRandomSonOf(r ? daddy->Leftson() : daddy->Rightson(),
              innerNodeAllowed, 1);
}
static void TEST_performRandomMoves(AW_window *aww,AWT_canvas *ntw)
{
    AWUSE(aww);

    int cnt=10000;

    while(cnt)
    {
    AP_tree_nlen *root = (AP_tree_nlen*)*ap_main->tree_root;
    AP_tree_nlen *source = getRandomSonOf(root,1,0);
    AP_tree_nlen *dest = getRandomSonOf(root,1,0);

    if (source!=dest && !dest->is_son(source))
    {
        root->setBranchlen(1.0,1.0);
        source->setBranchlen(1.0,1.0);
        dest->setBranchlen(1.0,1.0);

        char *error = source->move(dest,0.5);

        if (error)
        {
        cout << error << '\n';
        break;
        }

        cnt--;
//      cout << "Moves left: " << cnt << '\n';
    }
    }

    cout << "Testing tree\n";
    TEST_testWholeTree(aww,ntw);

//    cout << "setting branchlens\n";
//    TEST_setBranchlen(aww,ntw);

//    cout << "dumping nodes\n";
//    TEST_dumpNodes(aww,ntw);

    cout << "done\n";
//    refreshTree(ntw);
}
*/

#if defined(TEST_FUNCTIONS)
static void TEST_mixTree(AW_window *, AWT_canvas *ntw)
{
    rootEdge()->mixTree(100);
    refreshTree(ntw);
}

static void TEST_sortTreeByName(AW_window *, AWT_canvas *ntw)
{
    AP_tree_nlen *root = (AP_tree_nlen*)*ap_main->tree_root;

    root->sortByName();
    refreshTree(ntw);
}

static void TEST_buildAndDumpChain(AW_window *, AWT_canvas *)
{
    AP_tree_nlen *root = (AP_tree_nlen*)*ap_main->tree_root;

    root->Leftson()->edgeTo(root->Rightson())->testChain(2);
    root->Leftson()->edgeTo(root->Rightson())->testChain(3);
    root->Leftson()->edgeTo(root->Rightson())->testChain(4);
    root->Leftson()->edgeTo(root->Rightson())->testChain(5);
    root->Leftson()->edgeTo(root->Rightson())->testChain(6);
    root->Leftson()->edgeTo(root->Rightson())->testChain(7);
    root->Leftson()->edgeTo(root->Rightson())->testChain(8);
    root->Leftson()->edgeTo(root->Rightson())->testChain(9);
    root->Leftson()->edgeTo(root->Rightson())->testChain(10);
    root->Leftson()->edgeTo(root->Rightson())->testChain(11);
    root->Leftson()->edgeTo(root->Rightson())->testChain(-1);
}

static void init_TEST_menu(AW_window_menu_modes *awm,AWT_canvas *ntw)
{
    awm->create_menu( 0,   "Test", "T", "",  AWM_ALL );

    awm->insert_menu_topic(0, "Test edges",         "T", "", AWM_ALL, (AW_CB)TEST_testWholeTree,     (AW_CL)ntw, 0);
    awm->insert_menu_topic(0, "Mix tree",           "M", "", AWM_ALL, (AW_CB)TEST_mixTree,           (AW_CL)ntw, 0);
    awm->insert_menu_topic(0, "Dump nodes",         "D", "", AWM_ALL, (AW_CB)TEST_dumpNodes,         (AW_CL)ntw, 0);
    awm->insert_menu_topic(0, "Set branchlens",     "b", "", AWM_ALL, (AW_CB)TEST_setBranchlen,      (AW_CL)ntw, 0);
    awm->insert_menu_topic(0, "Sort tree by name",  "S", "", AWM_ALL, (AW_CB)TEST_sortTreeByName,    (AW_CL)ntw, 0);
    awm->insert_menu_topic(0, "Build & dump chain", "c", "", AWM_ALL, (AW_CB)TEST_buildAndDumpChain, (AW_CL)ntw, 0);
    awm->insert_separator();
    awm->insert_menu_topic(0, "Add marked species",          "A", "pa_quick.hlp", AWM_ALL, (AW_CB)NT_quick_add_test,  (AW_CL)ntw, 0);
    awm->insert_menu_topic(0, "Add marked species + NNI",    "i", "pa_add.hlp",   AWM_ALL, (AW_CB)NT_add_test,        (AW_CL)ntw, 0);
    awm->insert_menu_topic(0, "Remove & add marked species", "o", "pa_add.hlp",   AWM_ALL, (AW_CB)NT_rquick_add_test, (AW_CL)ntw, 0);
    awm->insert_menu_topic(0, "Remove & add marked + NNI",   "v", "pa_add.hlp",   AWM_ALL, (AW_CB)NT_radd_test,       (AW_CL)ntw, 0);
    awm->insert_separator();
    awm->insert_menu_topic(0, "Add selected species",       "", "pa_quick_sel.hlp", AWM_ALL, (AW_CB)NT_quick_add_test, (AW_CL)ntw, 1);
    awm->insert_menu_topic(0, "Add selected species + NNI", "", "pa_add_sel.hlp",   AWM_ALL, (AW_CB)NT_add_test,       (AW_CL)ntw, 1);
}
#endif // TEST_FUNCTIONS

static GB_ERROR pars_check_size(AW_root *awr){
    char *tree_name = awr->awar(AWAR_TREE)->read_string();
    char *filter = awr->awar(AWAR_FILTER_FILTER)->read_string();
    GB_ERROR error = 0;
    long ali_len = 0;
    if (strlen(filter)){
        int i;
        for (i=0;filter[i];i++){
            if (filter[i] != '0') ali_len++;
        }
    }else{
        char *ali_name = awr->awar(AWAR_ALIGNMENT)->read_string();
        ali_len = GBT_get_alignment_len(GLOBAL_gb_main,ali_name);
        delete ali_name;
    }

    long tree_size = GBT_size_of_tree(GLOBAL_gb_main,tree_name);
    if (tree_size == -1) {
        error = GB_export_error("Please select an existing tree");
    }
    else {
        if ((unsigned long)(ali_len * tree_size * 4/ 1000) > GB_get_physical_memory()){
            error  = GB_export_error("Very big tree");
        }
    }
    free(filter);
    free(tree_name);
    return error;
}

static void pars_reset_optimal_parsimony(AW_window *aww, AW_CL *cl_ntw) {
    AW_root *awr = aww->get_root();
    awr->awar(AWAR_BEST_PARSIMONY)->write_int(awr->awar(AWAR_PARSIMONY)->read_int());

    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    ntw->refresh();
}

/******************************************************

******************************************************/

static void pars_start_cb(AW_window *aww, AW_CL cd_adfiltercbstruct) {
    AW_root *awr = aww->get_root();
    GB_begin_transaction(GLOBAL_gb_main);
    {
        GB_ERROR error = pars_check_size(awr);
        if (error){
            if (!aw_ask_sure("This program will need a lot of computer memory\n"
                             "    (Hint: the use of a filter often helps)\n"
                             "Do you want to continue?")) {
                GB_commit_transaction(GLOBAL_gb_main);
                return;
            }
        }
    }


    AW_window_menu_modes *awm = new AW_window_menu_modes();
    awm->init(awr,"ARB_PARSIMONY", "ARB_PARSIMONY", 400,200);

    AW_gc_manager aw_gc_manager = 0;
    GLOBAL_NT->tree             = PARS_generate_tree(awr);

    AWT_canvas *ntw;
    {
        AP_tree_sort  old_sort_type = GLOBAL_NT->tree->tree_sort;
        GLOBAL_NT->tree->set_tree_type(AP_LIST_SIMPLE); // avoid NDS warnings during startup
        ntw = new AWT_canvas(GLOBAL_gb_main, (AW_window *)awm, GLOBAL_NT->tree, aw_gc_manager, AWAR_TREE);
        GLOBAL_NT->tree->set_tree_type(old_sort_type);
    }

    {
        aw_openstatus("load tree");
        NT_reload_tree_event(awr,ntw,GB_TRUE);          // load first tree and set delete callbacks
        if (!GLOBAL_NT->tree->tree_root) {
            aw_closestatus();
            aw_popup_exit("I cannot load the selected tree");
        }

        AP_tree_edge::initialize((AP_tree_nlen*)*ap_main->tree_root);   // builds edges
        GLOBAL_NT->tree->tree_root->remove_leafs(ntw->gb_main, AWT_REMOVE_DELETED);

        adfiltercbstruct *acbs = (adfiltercbstruct*)cd_adfiltercbstruct;
        NT_tree_init(GLOBAL_NT->tree, acbs);
            
        GLOBAL_NT->tree->tree_root->remove_leafs(ntw->gb_main, AWT_REMOVE_DELETED | AWT_REMOVE_NO_SEQUENCE);

        GB_commit_transaction(ntw->gb_main);
        aw_status("Calculating inner nodes");
        GLOBAL_NT->tree->tree_root->costs();

        aw_closestatus();
    }

    awr->awar( AWAR_COLOR_GROUPS_USE)->add_callback( (AW_RCB)NT_recompute_cb, (AW_CL)ntw,0);

    if (ap_main->commands.add_marked)           NT_quick_add(awm,ntw,0);
    if (ap_main->commands.add_selected)         NT_quick_add(awm,ntw,1);
    if (ap_main->commands.calc_branch_lenths)   NT_branch_lengths(awm,ntw);
    if (ap_main->commands.calc_bootstrap)       NT_bootstrap(awm,ntw,0);
    if (ap_main->commands.quit)                 pars_exit(awm);

    GB_transaction dummy(ntw->gb_main);

    GBDATA *gb_arb_presets = GB_search(ntw->gb_main,"arb_presets", GB_CREATE_CONTAINER);
    GB_add_callback(gb_arb_presets,GB_CB_CHANGED, (GB_CB)AWT_expose_cb,(int *)ntw);

    awm->create_menu(       0,   "File",     "F", "pars_file.hlp",  AWM_ALL );
    {
#if defined(DEBUG)
        awm->insert_menu_topic("db_browser", "Browse loaded database(s)", "", "db_browser.hlp", AWM_ALL, AW_POPUP, (AW_CL)AWT_create_db_browser, 0);
        awm->insert_separator();
#endif // DEBUG
        awm->insert_menu_topic("print_tree", "Print Tree ...",          "P","tree2prt.hlp", AWM_ALL, AWT_popup_print_window, (AW_CL)ntw, 0 );
        awm->insert_menu_topic( "quit",     "Quit",             "Q","quit.hlp",     AWM_ALL, (AW_CB)PARS_export_cb, (AW_CL)ntw,2);
    }

    awm->create_menu( 0,   "Species", "S", "nt_tree.hlp",  AWM_ALL );
    {
        NT_insert_mark_submenus(awm, ntw, 0);

    }
    awm->create_menu( 0,   "Tree", "T", "nt_tree.hlp",  AWM_ALL );
    {

        awm->insert_menu_topic( "nds",      "NDS (Node Display Setup) ...",      "N","props_nds.hlp",    AWM_ALL, AW_POPUP, (AW_CL)AWT_create_nds_window, (AW_CL)ntw->gb_main );

        awm->insert_separator();
        awm->insert_menu_topic("tree_2_xfig", "Edit Tree View using XFIG ...",  "E", "tree2file.hlp", AWM_ALL, AWT_popup_tree_export_window, (AW_CL)ntw, 0);
        awm->insert_menu_topic("tree_print",  "Print Tree View to Printer ...", "P", "tree2prt.hlp",  AWM_ALL, AWT_popup_print_window,       (AW_CL)ntw, 0);
        awm->insert_separator();
        awm->insert_sub_menu(0, "Collapse/Expand Tree",         "C");
        {
            awm->insert_menu_topic("tree_group_all",        "Group All",            "A","tgroupall.hlp",    AWM_ALL,    (AW_CB)NT_group_tree_cb,    (AW_CL)ntw, 0 );
            awm->insert_menu_topic("tree_group_not_marked", "Group All Except Marked",  "M","tgroupnmrkd.hlp",  AWM_ALL,    (AW_CB)NT_group_not_marked_cb,  (AW_CL)ntw, 0 );
            awm->insert_menu_topic("tree_ungroup_all",      "Ungroup All",          "U","tungroupall.hlp",  AWM_ALL,    (AW_CB)NT_ungroup_all_cb,   (AW_CL)ntw, 0 );
            awm->insert_separator();
            NT_insert_color_collapse_submenu(awm, ntw);
        }
        awm->close_sub_menu();
        awm->insert_separator();
        awm->insert_sub_menu(0, "Remove Species from Tree",     "R");
        {
            awm->insert_menu_topic("tree_remove_deleted", "Remove Zombies", "Z", "trm_del.hlp",    AWM_ALL, (AW_CB)NT_remove_leafs, (AW_CL)ntw, AWT_REMOVE_DELETED|AWT_REMOVE_BUT_DONT_FREE);
            awm->insert_menu_topic("tree_remove_marked",  "Remove Marked",  "M", "trm_mrkd.hlp",   AWM_ALL, (AW_CB)NT_remove_leafs, (AW_CL)ntw, AWT_REMOVE_MARKED|AWT_REMOVE_BUT_DONT_FREE);
            awm->insert_menu_topic("tree_keep_marked",    "Keep Marked",    "K", "tkeep_mrkd.hlp", AWM_ALL, (AW_CB)NT_remove_leafs, (AW_CL)ntw, AWT_REMOVE_NOT_MARKED|AWT_REMOVE_DELETED|AWT_REMOVE_BUT_DONT_FREE);
        }
        awm->close_sub_menu();
        awm->insert_sub_menu(0, "Add Species to Tree",      "A");
        {
            awm->insert_menu_topic("add_marked",         "Add Marked Species",                              "M", "pa_quick.hlp",     AWM_ALL, (AW_CB)NT_quick_add,  (AW_CL)ntw, 0);
            awm->insert_menu_topic("add_marked_nni",     "Add Marked Species + Local Optimization (NNI)",   "N", "pa_add.hlp",       AWM_ALL, (AW_CB)NT_add,        (AW_CL)ntw, 0);
            awm->insert_menu_topic("rm_add_marked",      "Remove & Add Marked Species",                     "R", "pa_add.hlp",       AWM_ALL, (AW_CB)NT_rquick_add, (AW_CL)ntw, 0);
            awm->insert_menu_topic("rm_add_marked_nni|", "Remove & Add Marked + Local Optimization (NNI)",  "L", "pa_add.hlp",       AWM_ALL, (AW_CB)NT_radd,       (AW_CL)ntw, 0);
            awm->insert_separator();
            awm->insert_menu_topic("add_marked_partial", "Add Marked Partial Species",                      "P", "pa_partial.hlp",   AWM_ALL, NT_partial_add,       (AW_CL)ntw, (AW_CL)0);
            awm->insert_separator();
            awm->insert_menu_topic("add_selected",       "Add Selected Species",                            "S", "pa_quick_sel.hlp", AWM_ALL, (AW_CB)NT_quick_add,  (AW_CL)ntw, 1);
            awm->insert_menu_topic("add_selected_nni",   "Add Selected Species + Local Optimization (NNI)", "O", "pa_add_sel.hlp",   AWM_ALL, (AW_CB)NT_add,        (AW_CL)ntw, 1);
        }
        awm->close_sub_menu();
        awm->insert_separator();
        awm->insert_sub_menu(0, "Tree Optimization",        "O");
        {
            awm->insert_menu_topic("nni",           "Local Optimization (NNI) of Marked Visible Nodes", "L","",         AWM_ALL,    (AW_CB)NT_recursiveNNI, (AW_CL)ntw, 0 );
            awm->insert_menu_topic("kl_optimization",   "Global Optimization of Marked Visible Nodes",      "G","pa_optimizer.hlp", AWM_ALL,    (AW_CB)NT_optimize, (AW_CL)ntw, 0 );
        }
        awm->close_sub_menu();
        awm->insert_menu_topic("reset", "Reset optimal parsimony", "s", "", AWM_ALL, (AW_CB)pars_reset_optimal_parsimony, (AW_CL)ntw, 0);
        awm->insert_separator();
        awm->insert_menu_topic("beautify_tree",      "Beautify Tree",            "B", "resorttree.hlp",       AWM_ALL, (AW_CB)NT_resort_tree_cb, (AW_CL)ntw, 0);
        awm->insert_menu_topic("calc_branch_lenths", "Calculate Branch Lengths", "L", "pa_branchlengths.hlp", AWM_ALL, (AW_CB)NT_branch_lengths, (AW_CL)ntw, 0);
        awm->insert_separator();
        awm->insert_menu_topic("calc_upper_bootstrap_indep", "Calculate Upper Bootstrap Limit (dependend NNI)",   "d", "pa_bootstrap.hlp", AWM_ALL, (AW_CB)NT_bootstrap,        (AW_CL)ntw, 0);
        awm->insert_menu_topic("calc_upper_bootstrap_dep",   "Calculate Upper Bootstrap Limit (independend NNI)", "i", "pa_bootstrap.hlp", AWM_ALL, (AW_CB)NT_bootstrap,        (AW_CL)ntw, 1);
        awm->insert_menu_topic("tree_remove_remark",         "Remove Bootstrap Values",                           "V", "trm_boot.hlp",     AWM_ALL, (AW_CB)NT_remove_bootstrap, (AW_CL)ntw, 0);
    }

#if defined(TEST_FUNCTIONS)
    init_TEST_menu(awm,ntw);
#endif // TEST_FUNCTIONS

    awm->create_menu("props","Properties","r","properties.hlp", AWM_ALL);
    {
        awm->insert_menu_topic("props_menu",  "Menu: Colors and Fonts ...",              "M", "props_frame.hlp",      AWM_ALL, AW_POPUP,(AW_CL)AW_preset_window,        0);
        awm->insert_menu_topic("props_tree",  "Tree: Colors and Fonts ...",              "C", "pars_props_data.hlp",  AWM_ALL, AW_POPUP,(AW_CL)AW_create_gc_window,     (AW_CL)aw_gc_manager );
        awm->insert_menu_topic("props_tree2", "Tree: Settings ...",                      "T", "nt_tree_settings.hlp", AWM_ALL, AW_POPUP,(AW_CL)NT_create_tree_setting,  (AW_CL)ntw );
        awm->insert_menu_topic("props_kl",    "KERN. LIN ...",                           "K", "kernlin.hlp",          AWM_ALL, AW_POPUP,(AW_CL)create_kernighan_window, 0);
        awm->insert_menu_topic("save_props",  "Save Defaults (in ~/.arb_prop/pars.arb)", "D", "savedef.hlp",          AWM_ALL, (AW_CB)AW_save_defaults, 0, 0);
    }
    awm->button_length(5);

    awm->create_menu(0,"ETC","E","nt_etc.hlp", AWM_ALL);
    {
        awm->insert_menu_topic("reset_logical_zoom",    "Reset Logical Zoom",   "L","rst_log_zoom.hlp", AWM_ALL, (AW_CB)NT_reset_lzoom_cb, (AW_CL)ntw, 0 );
        awm->insert_menu_topic("reset_physical_zoom",   "Reset Physical Zoom",  "P","rst_phys_zoom.hlp",AWM_ALL, (AW_CB)NT_reset_pzoom_cb, (AW_CL)ntw, 0 );
    }

    awm->insert_help_topic("help_how",      "How to use Help",      "H","help.hlp",     AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"help.hlp", 0);
    awm->insert_help_topic("help_arb",      "ARB Help",         "A","arb.hlp",      AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"arb.hlp", 0);
    awm->insert_help_topic("help_arb_pars",     "ARB PARSIMONY Help",       "N","arb_pars.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"arb_pars.hlp", 0);

    awm->create_mode( 0, "select.bitmap", "mode_select.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SELECT);
    awm->create_mode( 0, "mark.bitmap", "mode_mark.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_MARK);
    awm->create_mode( 0, "group.bitmap", "mode_group.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_GROUP);
    awm->create_mode( 0, "pzoom.bitmap", "mode_pzoom.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_ZOOM);
    awm->create_mode( 0, "lzoom.bitmap", "mode_lzoom.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_LZOOM);
    awm->create_mode( 0, "swap.bitmap", "mode_swap.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SWAP);
    awm->create_mode( 0, "move.bitmap", "mode_move.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_MOVE);
#ifdef NNI_MODES
    awm->create_mode( 0, "nearestn.bitmap", "mode_nni.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_NNI);
    awm->create_mode( 0, "kernlin.bitmap", "mode_kernlin.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_KERNINGHAN);
    awm->create_mode( 0, "optimize.bitmap", "mode_optimize.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_OPTIMIZE);
#endif // NNI_MODES
    awm->create_mode( 0, "setroot.bitmap", "mode_set_root.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SETROOT);
    awm->create_mode( 0, "reset.bitmap", "mode_reset.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_RESET);

    awm->at(5,2);
    awm->auto_space(0,-2);
    awm->shadow_width(1);


    int db_treex,db_treey;
    awm->get_at_position( &db_treex,&db_treey );
    awm->callback(AW_POPUP_HELP,(AW_CL)"nt_tree_select.hlp");
    awm->button_length(16);
    awm->help_text("nt_tree_select.hlp");
    awm->create_button(0,AWAR_TREE);


    int db_stackx,db_stacky;
    awm->label_length(8);
    awm->label("Stored");
    awm->get_at_position( &db_stackx,&db_stacky );
    awm->button_length(6);
    awm->callback( AW_POPUP_HELP, (AW_CL)"ap_stack.hlp");
    awm->help_text("ap_stack.hlp");
    awm->create_button(0,AWAR_STACKPOINTER);

    int db_parsx,db_parsy;
    awm->label_length(14);
    awm->label("Current Pars:");
    awm->get_at_position( &db_parsx,&db_parsy );

    awm->button_length(10);
    awm->create_button(0,AWAR_PARSIMONY);

    awm->button_length(0);

    awm->callback((AW_CB)NT_jump_cb,(AW_CL)ntw,1);
    awm->help_text("tr_jump.hlp");
    awm->create_button("JUMP","#pjump.bitmap",0);

    awm->callback(AW_POPUP_HELP,(AW_CL)"arb_pars.hlp");
    awm->help_text("help.hlp");
    awm->create_button("HELP","HELP","H");

    awm->at_newline();

    awm->button_length(8);
    awm->at_x(db_stackx);
    awm->callback((AW_CB1)AP_user_pop_cb,(AW_CL)ntw);
    awm->help_text("ap_stack.hlp");
    awm->create_button("POP","RESTORE",0);

    awm->button_length(6);
    awm->callback((AW_CB1)AP_user_push_cb,(AW_CL)ntw);
    awm->help_text("ap_stack.hlp");
    awm->create_button("PUSH", "STORE",0);

    awm->button_length(7);

    awm->at_x(db_treex);
    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_TREE_RADIAL);
    awm->help_text("tr_type_radial.hlp");
    awm->create_button("RADIAL_TREE", "#radial.bitmap",0);

    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_TREE_NORMAL);
    awm->help_text("tr_type_list.hlp");
    awm->create_button("LIST_TREE", "#list.bitmap",0);

    awm->at_x(db_parsx);
    awm->label_length(14);
    awm->label("Optimal Pars:");

    awm->button_length(10);
    awm->create_button(0,AWAR_BEST_PARSIMONY);

    awm->at_newline();

    {
        AW_at_maxsize maxSize; // store size (so AWAR_FOOTER does not affect min. window size)
        maxSize.store(awm->_at);
        awm->button_length(AWAR_FOOTER_MAX_LEN);
        awm->create_button(0,AWAR_FOOTER);
        awm->at_newline();
        maxSize.restore(awm->_at);
    }

//     awm->button_length(AWAR_FOOTER_MAX_LEN);
//     awm->create_button(0,AWAR_FOOTER);

//     awm->at_newline();
    awm->get_at_position( &db_treex,&db_treey );
    awm->set_info_area_height( db_treey+6 );

    awm->set_bottom_area_height( 0 );
    awm->set_focus_callback((AW_CB)PA_focus_cb,(AW_CL)ntw->gb_main,0);

    aww->hide();
    awm->show();

    AP_user_push_cb(aww, ntw); // push initial tree
}

static AW_window *create_pars_init_window(AW_root *awr)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( awr, "PARS_PROPS", "SET PARSIMONY OPTIONS");
    aws->load_xfig("pars/init.fig");

    aws->button_length( 10 );
    aws->label_length( 10 );

    aws->callback(pars_exit);
    aws->at("close");
    aws->create_button("ABORT","ABORT","A");

    aws->callback(AW_POPUP_HELP,(AW_CL)"arb_pars_init.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("filter");
    adfiltercbstruct *filtercd = awt_create_select_filter(aws->get_root(),GLOBAL_gb_main,AWAR_FILTER_NAME);
    aws->callback(AW_POPUP, (AW_CL)awt_create_select_filter_win, (AW_CL)filtercd);
    aws->create_button("SELECT_FILTER",AWAR_FILTER_NAME);

    aws->callback(pars_start_cb, (AW_CL)filtercd);
    aws->at("go");
    aws->create_button("GO","GO","G");

    aws->at("alignment");
    awt_create_selection_list_on_ad(GLOBAL_gb_main,(AW_window *)aws,AWAR_ALIGNMENT,"*=");

    aws->at("tree");
    awt_create_selection_list_on_trees(GLOBAL_gb_main,(AW_window *)aws,AWAR_TREE);

    awt_csp = new AWT_csp(GLOBAL_gb_main,awr,"tmp/pars/csp/name");

    aws->at("weights");
    aws->callback(AW_POPUP,(AW_CL)create_csp_window,(AW_CL)awt_csp);
    aws->create_button("SELECT_CSP","tmp/pars/csp/name");

    return (AW_window *)aws;
}

static void create_parsimony_variables(AW_root *aw_root, AW_default def)
{
    // parsimony
#if 0
    aw_root->awar_float("genetic/m_rate",0,def);
    aw_root->awar_int("genetic/input_1",0,def);
    aw_root->awar_int("genetic/input_2",0,def);
    aw_root->awar_float("genetic/input_3",0,def);
    aw_root->awar_float("genetic/input_4",0,def);
    aw_root->awar_float("genetic/input_5",0,def);
    aw_root->awar_float("genetic/input_6",0,def);
    aw_root->awar_float("genetic/input_7",0,def);
    aw_root->awar_float("genetic/input_8",0,def);
    aw_root->awar_float("genetic/input_9",0,def);
    aw_root->awar_float("genetic/input_10",0,def);
    aw_root->awar_float("genetic/input_11",0,def);
    aw_root->awar_int("genetic/out_1",0,def);
    aw_root->awar_float("genetic/out_2",0,def);
    aw_root->awar_float("genetic/out_3",0,def);
    aw_root->awar_float("genetic/out_4",0,def);
    aw_root->awar_float("genetic/out_5",0,def);
    aw_root->awar_float("genetic/out_6",0,def);
    aw_root->awar_float("genetic/out_7",0,def);
    aw_root->awar_float("genetic/out_8",0,def);
#endif
    // kernighan

    aw_root->awar_float("genetic/kh/nodes",1.7,def);
    aw_root->awar_int("genetic/kh/maxdepth",15,def);
    aw_root->awar_int("genetic/kh/incdepth",5,def);

    aw_root->awar_int("genetic/kh/static/enable",1,def);
    aw_root->awar_int("genetic/kh/static/depth0",2,def);
    aw_root->awar_int("genetic/kh/static/depth1",2,def);
    aw_root->awar_int("genetic/kh/static/depth2",2,def);
    aw_root->awar_int("genetic/kh/static/depth3",2,def);
    aw_root->awar_int("genetic/kh/static/depth4",1,def);

    aw_root->awar_int("genetic/kh/dynamic/enable",1,def);
    aw_root->awar_int("genetic/kh/dynamic/start",100,def);
    aw_root->awar_int("genetic/kh/dynamic/maxx",6,def);
    aw_root->awar_int("genetic/kh/dynamic/maxy",150,def);

    aw_root->awar_int("genetic/kh/function_type",AP_QUADRAT_START,def);

    awt_create_dtree_awars(aw_root,def);
}

static void pars_create_all_awars(AW_root *awr, AW_default aw_def)
{
    awr->awar_string(AWAR_SPECIES_NAME, "",     GLOBAL_gb_main);
    awr->awar_string(AWAR_FOOTER,       "",     aw_def);
    awr->awar_string(AWAR_FILTER_NAME,  "none", GLOBAL_gb_main);

    {
        GB_transaction  ta(GLOBAL_gb_main);
        char           *dali = GBT_get_default_alignment(GLOBAL_gb_main);
        
        awr->awar_string( AWAR_ALIGNMENT,dali ,GLOBAL_gb_main )->write_string(dali);
        free(dali);
    }

    awt_set_awar_to_valid_filter_good_for_tree_methods(GLOBAL_gb_main,awr,AWAR_FILTER_NAME);

    awr->awar_string(AWAR_FILTER_FILTER,"" ,GLOBAL_gb_main);
    awr->awar_string(AWAR_FILTER_ALIGNMENT,"" ,aw_def)     ;
    awr->awar       (AWAR_FILTER_ALIGNMENT)                ->map(AWAR_ALIGNMENT);

    awr->awar_string( "tmp/pars/csp/alignment",0 ,aw_def );
    awr->awar("tmp/pars/csp/alignment")->map(AWAR_ALIGNMENT);

    awr->awar_int( AWAR_PARS_TYPE,PARS_WAGNER ,GLOBAL_gb_main );

    {
        GB_transaction  ta(GLOBAL_gb_main);
        GBDATA *gb_tree_name = GB_search(GLOBAL_gb_main,AWAR_TREE,GB_STRING);
        char   *tree_name    = GB_read_string(gb_tree_name);
        
        awr->awar_string( AWAR_TREE,0 ,aw_def )->write_string(tree_name);
        free(tree_name);
    }

    awr->awar_int(AWAR_PARSIMONY,      0, aw_def);
    awr->awar_int(AWAR_BEST_PARSIMONY, 0, aw_def);
    awr->awar_int(AWAR_STACKPOINTER,   0, aw_def);

    create_parsimony_variables(awr, GLOBAL_gb_main);
    create_nds_vars(awr,aw_def,GLOBAL_gb_main);

    AWT_create_db_browser_awars(awr, aw_def);

    GB_ERROR error = ARB_init_global_awars(awr, aw_def, GLOBAL_gb_main);
    if (error) aw_message(error);
}

static AW_root *AD_map_viewer_aw_root = 0;

void AD_map_viewer(GBDATA *gb_species, AD_MAP_VIEWER_TYPE vtype) {
    if (vtype == ADMVT_SELECT && AD_map_viewer_aw_root) {
        AD_map_viewer_aw_root->awar(AWAR_SPECIES_NAME)->write_string(GBT_read_name(gb_species));
    }
}

int main(int argc, char **argv)
{
    AW_root *aw_root;
    AW_default aw_default;
    AW_window *aww;

    aw_initstatus();

    aw_root = new AW_root;
    aw_default = aw_root->open_default(".arb_prop/pars.arb");
    aw_root->init_variables(aw_default);
    aw_root->init_root("ARB_PARS", false);

    AD_map_viewer_aw_root = aw_root;

    ap_main = new AP_main;

    GLOBAL_NT      = (NT_global *)calloc(sizeof(NT_global),1);
    GLOBAL_NT->awr = aw_root;

    const char *db_server = ":";

    while (argc>=2 && argv[1][0] == '-'){
        argc--;
        argv++;
        if (!strcmp(argv[0],"-quit")){
            ap_main->commands.quit = 1;
            continue;
        }
        if (!strcmp(argv[0],"-add_marked")){
            ap_main->commands.add_marked = 1;
            continue;
        }
        if (!strcmp(argv[0],"-add_selected")){
            ap_main->commands.add_selected = 1;
            continue;
        }
        if (!strcmp(argv[0],"-calc_branchlengths")){
            ap_main->commands.calc_branch_lenths = 1;
            continue;
        }
        if (!strcmp(argv[0],"-calc_bootstrap")){
            ap_main->commands.calc_bootstrap = 1;
            continue;
        }
        GB_export_error("Unknown option '%s'",argv[0]);
        GB_print_error();
        printf("    Options:                Meaning:\n"
               "\n"
               "    -add_marked             add marked species   (without changing topology)\n"
               "    -add_selected           add selected species (without changing topology)\n"
               "    -calc_branchlengths     calculate branch lengths only\n"
               "    -calc_bootstrap         estimate bootstrap values\n"
               "    -quit                   quit after performing operations\n"
               );

        exit(-1);
    }


    if (argc==2) {
        db_server = argv[1];
    }

    GLOBAL_gb_main = GBT_open(db_server,"rw",0);
    if (!GLOBAL_gb_main) aw_popup_exit(GB_await_error()); // exits
    
    AWT_announce_db_to_browser(GLOBAL_gb_main, GBS_global_string("ARB-database (%s)", db_server));

    pars_create_all_awars(aw_root,aw_default);

    aww = create_pars_init_window(aw_root);
    aww->show();

    aw_root->main_loop();
    return EXIT_SUCCESS;
}


