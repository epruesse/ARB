// =============================================================== //
//                                                                 //
//   File      : PARS_main.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "pars_main.hxx"
#include "pars_dtree.hxx"
#include "pars_klprops.hxx"
#include "ap_tree_nlen.hxx"

#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <aw_msg.hxx>

#include <awt.hxx>
#include <ColumnStat.hxx>
#include <nds.h>
#include <awt_sel_boxes.hxx>
#include <awt_filter.hxx>
#include <arb_progress.h>
#include <arb_misc.h>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <gui_aliview.hxx>
#include <ad_cb.h>

#include <TreeCallbacks.hxx>

#include <list>
#include <macros.hxx>

#if defined(DEBUG)
# define TESTMENU
#endif // DEBUG

using namespace std;

AW_HEADER_MAIN

#define AWAR_COLUMNSTAT_BASE "tmp/pars/colstat"
#define AWAR_COLUMNSTAT_NAME AWAR_COLUMNSTAT_BASE "/name"

GBDATA              *GLOBAL_gb_main = NULL;
static ArbParsimony *GLOBAL_PARS    = NULL;

inline AWT_graphic_tree *global_tree() { return GLOBAL_PARS->get_tree(); }

// waaah more globals :(
AP_main *ap_main;

static void pars_saveNrefresh_changed_tree(AWT_canvas *ntw) {
    ap_assert((AWT_TREE(ntw) == global_tree()));

    GB_ERROR error = global_tree()->save(ntw->gb_main, 0, 0, 0);
    if (error) aw_message(error);

    ntw->zoom_reset_and_refresh();
}

__ATTR__NORETURN static void pars_exit(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    shutdown_macro_recording(aw_root);
    aw_root->unlink_awars_from_DB(GLOBAL_gb_main);
#if defined(DEBUG)
    AWT_browser_forget_db(GLOBAL_gb_main);
#endif // DEBUG
    GB_close(GLOBAL_gb_main);
    exit(EXIT_SUCCESS);
}

static void AP_user_push_cb(AW_window *aww, AWT_canvas *) {
    ap_main->user_push();
    aww->get_root()->awar(AWAR_STACKPOINTER)->write_int(ap_main->get_user_push_counter());
}

static void AP_user_pop_cb(AW_window *aww, AWT_canvas *ntw) {
    if (ap_main->get_user_push_counter()<=0) {
        aw_message("No tree on stack.");
        return;
    }
    ap_main->user_pop();
    rootNode()->compute_tree();
    ASSERT_VALID_TREE(rootNode());
    aww->get_root()->awar(AWAR_STACKPOINTER)->write_int(ap_main->get_user_push_counter());

    pars_saveNrefresh_changed_tree(ntw);

    if (ap_main->get_user_push_counter() <= 0) { // last tree was popped => push again
        AP_user_push_cb(aww, ntw);
    }
}

class InsertData {
    int  abort_flag;
    long currentspecies;
    
    arb_progress progress;

public:

    bool quick_add_flag;
    InsertData(bool quick, long spec_count, int add_progress_steps)
        : abort_flag(false),
          currentspecies(0),
          progress(GBS_global_string("Inserting %li species", spec_count), spec_count+add_progress_steps),
          quick_add_flag(quick)
    {
    }

    bool every_sixteenth() { return (currentspecies & 0xf) == 0; }

    bool aborted() const { return abort_flag; }
    void set_aborted(bool aborted_) { abort_flag = aborted_; }

    void inc() {
        ++currentspecies;
        progress.inc();
        abort_flag = progress.aborted();
    }

    arb_progress& get_progress() { return progress; }
};


static int sort_sequences_by_length(const char*, long leaf0_ptr, const char*, long leaf1_ptr) {
    AP_tree *leaf0 = (AP_tree*)leaf0_ptr;
    AP_tree *leaf1 = (AP_tree*)leaf1_ptr;

    AP_FLOAT len0 = leaf0->get_seq()->weighted_base_count();
    AP_FLOAT len1 = leaf1->get_seq()->weighted_base_count();

    return len0<len1 ? 1 : (len0>len1 ? -1 : 0); // longest sequence first
}

static long transform_gbd_to_leaf(const char *key, long val, void *) {
    if (!val) return val;

#if defined(WARN_TODO)
#warning use create_linked_leaf() when impl
#endif

    GBDATA       *gb_node = (GBDATA *)val;
    AP_tree_root *troot   = ap_main->get_tree_root()->tree_static;
    AP_tree_nlen *leaf    = DOWNCAST(AP_tree_nlen*, troot->makeNode());

    leaf->forget_origin(); // new leaf is not part of tree yet

    leaf->gb_node = gb_node;
    leaf->name    = strdup(key);
    leaf->is_leaf = true;

    leaf->set_seq(troot->get_seqTemplate()->dup());
    GB_ERROR error = leaf->get_seq()->bind_to_species(gb_node);
    if (error) {
        aw_message(error);
        delete leaf; leaf = 0;
    }
    return (long)leaf;
}

static AP_tree_nlen *insert_species_in_tree(const char *key, AP_tree_nlen *leaf, InsertData *isits) {
    if (!leaf) return leaf;
    if (isits->aborted()) return leaf;

    AP_tree_nlen *tree = rootNode();

    if (leaf->get_seq()->weighted_base_count() < MIN_SEQUENCE_LENGTH) {
        aw_message(GBS_global_string("Species %s has too short sequence (%f, minimum is %i)",
                                     key,
                                     leaf->get_seq()->weighted_base_count(),
                                     MIN_SEQUENCE_LENGTH));
        delete leaf;
        return 0;
    }

    if (!tree) {                                    // no tree yet
        static AP_tree_nlen *last_inserted = NULL; // @@@ move 'last_inserted' into 'InsertData'

        if (!last_inserted) {                       // store 1st leaf
            last_inserted = leaf;
        }
        else {                                      // 2nd leaf -> create initial tree
            AP_tree_root *troot = ap_main->get_tree_root()->tree_static;

            leaf->initial_insert(last_inserted, troot);
            last_inserted = NULL;

            ap_assert(troot->get_root_node() == leaf->get_father());
            ASSERT_VALID_TREE(troot->get_root_node());
        }
    }
    else {
        ASSERT_VALID_TREE(tree);

        AP_tree_nlen **branchlist;

        {
            AP_tree **blist;
            long      bsum = 0;

            tree->buildBranchList(blist, bsum, true, -1); // get all branches
            branchlist = (AP_tree_nlen**)blist;
        }

        AP_tree_nlen *bestposl = tree->get_leftson();
        AP_tree_nlen *bestposr = tree->get_rightson();

        ap_assert(tree == rootNode());
        leaf->insert(bestposl);
        tree = NULL;                                // tree may have changed

        {
            AP_FLOAT curr_parsimony = rootNode()->costs();
            AP_FLOAT best_parsimony = curr_parsimony;

            ASSERT_VALID_TREE(rootNode());

            for (long counter = 0; !isits->aborted() && branchlist[counter]; counter += 2) {
                AP_tree_nlen *bl  = branchlist[counter];
                AP_tree_nlen *blf = branchlist[counter+1];

                if (blf->father == bl) {
                    bl  = branchlist[counter+1];
                    blf = branchlist[counter];
                }

                if (bl->father) {
                    bl->set_root();
                }

                ASSERT_VALID_TREE(rootNode());

                leaf->moveNextTo(bl, 0.5);
                ASSERT_VALID_TREE(rootNode());

                curr_parsimony = rootNode()->costs();
                ASSERT_VALID_TREE(rootNode());

                if (curr_parsimony < best_parsimony) {
                    best_parsimony = curr_parsimony;
                    bestposl = bl;
                    bestposr = blf;
                }

            }
        }
        delete branchlist; branchlist = 0;
        if (bestposl->father != bestposr) {
            bestposl = bestposr;
        }

        ASSERT_VALID_TREE(rootNode());

        leaf->moveNextTo(bestposl, 0.5);

        ASSERT_VALID_TREE(rootNode());

        if (!isits->quick_add_flag) {
            int deep                           = 5;
            if (isits->every_sixteenth()) deep = -1;
            
            arb_progress progress("optimization");
            bestposl->get_father()->nn_interchange_rek(deep, AP_BL_NNI_ONLY, true);
            ASSERT_VALID_TREE(rootNode());
        }
        AP_tree_nlen *brother = leaf->get_brother();
        if (brother->is_leaf && leaf->father->father) {
            bool brother_is_short = 2 * brother->get_seq()->weighted_base_count() < leaf->get_seq()->weighted_base_count();

            if (brother_is_short) {
                brother->remove();
                leaf->remove();

                for (int firstUse = 1; firstUse >= 0; --firstUse) {
                    AP_tree_nlen *to_insert = firstUse ? leaf : brother;
                    const char   *format    = firstUse ? "2:%s" : "shortseq:%s";

                    char *label = GBS_global_string_copy(format, to_insert->name);
                    insert_species_in_tree(label, to_insert, isits);
                    free(label);
                }
            }
        }

        ASSERT_VALID_TREE(rootNode());
    }

    return leaf;
}

static long hash_insert_species_in_tree(const char *key, long leaf, void *cd_isits) {
    InsertData *isits  = (InsertData*)cd_isits;
    long        result = (long)insert_species_in_tree(key, (AP_tree_nlen*)leaf, isits);
    isits->inc();
    return result;
}

static long count_hash_elements(const char *, long val, void *cd_count) {
    if (val) {
        long *count = (long*)cd_count;
        (*count)++;
    }
    return val;
}

enum AddWhat {
    NT_ADD_MARKED,
    NT_ADD_SELECTED,
};

static void nt_add(AW_window *, AWT_canvas *ntw, AddWhat what, bool quick) {
    GB_ERROR  error = 0;

    AP_tree *oldrootleft  = NULL;
    AP_tree *oldrootright = NULL;
    {
        AP_tree *root = rootNode();
        if (root) {
            root->reset_subtree_layout();
            oldrootleft  = root->get_leftson();
            oldrootright = root->get_rightson();
        }
    }

    GB_HASH *hash = 0;
    {
        GB_transaction ta(GLOBAL_gb_main);
        switch (what) {
            case NT_ADD_SELECTED: {
                char *name = GBT_readOrCreate_string(GLOBAL_gb_main, AWAR_SPECIES_NAME, "");
                if (name && strlen(name)) {
                    GBDATA *gb_species = GBT_find_species(GLOBAL_gb_main, name);
                    if (gb_species) {
                        hash = GBS_create_hash(1, GB_MIND_CASE);
                        GBS_write_hash(hash, name, (long)gb_species);
                    }
                    else error = GBS_global_string("Selected Species (%s) not found", name);
                }
                else error = "Please select a species";
                free(name);
                break;
            }
            case NT_ADD_MARKED: {
                hash = GBT_create_marked_species_hash(GLOBAL_gb_main);
                break;
            }
        }
    }

    if (!error) {
        ap_assert(hash);

        NT_remove_species_in_tree_from_hash(rootNode(), hash);

        long max_species = 0;
        GBS_hash_do_loop(hash, count_hash_elements, &max_species);

        int        implicitSteps = quick ? 1 : 2; // 1 step for calc_branchlengths, 1 step for NNI
        InsertData isits(quick, max_species, implicitSteps);

        GB_begin_transaction(GLOBAL_gb_main);
        GBS_hash_do_loop(hash, transform_gbd_to_leaf, NULL);
        GB_commit_transaction(GLOBAL_gb_main);

        {
            int skipped = max_species - GBS_hash_count_elems(hash);
            if (skipped) {
                aw_message(GBS_global_string("Skipped %i species (no data?)", skipped));
                isits.get_progress().inc_by(skipped);
            }
        }

        GBS_hash_do_sorted_loop(hash, hash_insert_species_in_tree, sort_sequences_by_length, &isits);

        if (!quick) {
            rootEdge()->nni_rek(-1, false, AP_BL_NNI_ONLY, NULL);
            ++isits.get_progress();
        }

        if (rootNode()) {
            rootEdge()->calc_branchlengths();
            ++isits.get_progress();

            ASSERT_VALID_TREE(rootNode());
            rootNode()->compute_tree();

            if (oldrootleft) {
                if (oldrootleft->father == oldrootright) oldrootleft->set_root();
                else                                     oldrootright->set_root();
            }
            else {
                ARB_edge innermost = rootNode()->get_tree_root()->find_innermost_edge();
                innermost.set_root();
            }
        }
        else {
            error = "Tree lost (no leafs left)";
            isits.get_progress().done();
        }
    }

    if (hash) GBS_free_hash(hash);
    if (error) aw_message(error);

    AWT_TREE(ntw)->reorder_tree(BIG_BRANCHES_TO_TOP);
    pars_saveNrefresh_changed_tree(ntw);
}

// ------------------------------------------
//      Adding partial sequences to tree

class PartialSequence { 
    GBDATA               *gb_species;
    mutable AP_tree_nlen *self;                     // self converted to leaf (ready for insertion)
    const AP_tree_nlen   *best_full_match;          // full sequence position which matched best
    long                  overlap;                  // size of overlapping region
    long                  penalty;                  // weighted mismatches
    bool                  released;
    bool                  multi_match;
    string                multi_list;               // list of equal-rated insertion-points (not containing self)

    AP_tree_nlen *get_self() const {
        if (!self) {
            ap_assert(!released); // request not possible, because leaf has already been released!

            self = (AP_tree_nlen*)transform_gbd_to_leaf(GBT_read_name(gb_species), (long)gb_species, NULL);
            ap_assert(self);
        }
        return self;
    }

public:
    PartialSequence(GBDATA *gb_species_)
        : gb_species(gb_species_), self(0), best_full_match(0)
        , overlap(0),  penalty(LONG_MAX), released(false), multi_match(false)
    {
    }
    PartialSequence(const PartialSequence& other)
        : gb_species(other.gb_species),
          self(other.self),
          best_full_match(other.best_full_match),
          overlap(other.overlap),
          penalty(other.penalty),
          released(other.released),
          multi_match(other.multi_match),
          multi_list(other.multi_list)
    {
        ap_assert(self == 0); // copying self not implemented
    }
    DECLARE_ASSIGNMENT_OPERATOR(PartialSequence);
    ~PartialSequence() { ap_assert(self == 0); }

    GBDATA *get_species() const { return gb_species; }
    const AP_tree_nlen *get_best_match() const { return best_full_match; }
    AP_FLOAT get_branchlength() const { return AP_FLOAT(penalty)/overlap; }
    void test_match(const AP_tree_nlen *leaf_full);
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

    AP_tree_nlen *release() {
        AP_tree_nlen *s = self;
        self            = 0;
        released        = true;
        return s;
    }

    void dump() const {
        printf("best match for '%s' is '%s' (overlap=%li penalty=%li)\n",
               get_name(), best_full_match->name,
               overlap, penalty);
    }

};

void PartialSequence::test_match(const AP_tree_nlen *leaf_full) {
    long curr_overlap;
    long curr_penalty;

    leaf_full->get_seq()->partial_match(get_self()->get_seq(), &curr_overlap, &curr_penalty);

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

static GB_ERROR nt_best_partial_match_rek(list<PartialSequence>& partial, const AP_tree_nlen *tree) {
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
            error             = nt_best_partial_match_rek(partial, tree->get_leftson());
            if (!error) error = nt_best_partial_match_rek(partial, tree->get_rightson());
        }
    }
    return error;
}

static void count_partial_and_full(const AP_tree_nlen *at, int *partial, int *full, int *zombies, int default_value, bool define_if_undef) {
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
        count_partial_and_full(at->get_leftson(),  partial, full, zombies, default_value, define_if_undef);
        count_partial_and_full(at->get_rightson(), partial, full, zombies, default_value, define_if_undef);
    }
}

static const AP_tree_nlen *find_least_deep_leaf(const AP_tree_nlen *at, int depth, int *min_depth) {
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

    const AP_tree_nlen *left  = find_least_deep_leaf(at->get_leftson(), depth+1, min_depth);
    const AP_tree_nlen *right = find_least_deep_leaf(at->get_rightson(), depth+1, min_depth);

    return right ? right : left;
}
inline AP_tree_nlen *find_least_deep_leaf(AP_tree_nlen *at, int depth, int *min_depth) {
    return const_cast<AP_tree_nlen*>(find_least_deep_leaf(const_cast<const AP_tree_nlen*>(at), depth, min_depth));
}

static long push_partial(const char *, long val, void *cd_partial) {
    list<PartialSequence> *partial = reinterpret_cast<list<PartialSequence> *>(cd_partial);
    partial->push_back(PartialSequence((GBDATA*)val));
    return val;
}

static void nt_add_partial(AW_window * /* aww */, AWT_canvas *ntw) {
    GB_begin_transaction(GLOBAL_gb_main);
    GB_ERROR error = 0;

    int full_marked_sequences = 0;

    arb_progress part_add_progress("Adding partial sequences");

    {
        list<PartialSequence> partial;
        {
            GB_HASH *partial_hash = GBS_create_hash(GBT_get_species_count(GLOBAL_gb_main), GB_MIND_CASE);

            int marked_found             = 0;
            int partial_marked_sequences = 0;
            int no_data                  = 0;      // no data in alignment

            for (GBDATA *gb_marked = GBT_first_marked_species(GLOBAL_gb_main);
                 !error && gb_marked;
                 gb_marked = GBT_next_marked_species(gb_marked))
            {
                ++marked_found;

                if (GBT_read_sequence(gb_marked, ap_main->get_aliname())) { // species has sequence in alignment
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
                        default:
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
                NT_remove_species_in_tree_from_hash(rootNode(), partial_hash); // skip all species which are in tree
                GBS_hash_do_loop(partial_hash, push_partial, &partial); // build partial list from hash

                int partials_already_in_tree = partial_marked_sequences - partial.size();

                if (no_data>0) aw_message(GBS_global_string("%i marked species have no data in '%s'", no_data, ap_main->get_aliname()));
                if (full_marked_sequences>0) aw_message(GBS_global_string("%i marked species are declared full sequences", full_marked_sequences));
                if (partials_already_in_tree>0) aw_message(GBS_global_string("%i marked species are already in tree", partials_already_in_tree));

                if (partial.empty()) error = "No species left to add";
            }
        }

        if (!error) {
            rootNode()->reset_subtree_layout();

            // find best matching full sequence for each partial sequence
            error = nt_best_partial_match_rek(partial, rootNode());

            list<PartialSequence>::iterator i = partial.begin();
            list<PartialSequence>::iterator e = partial.end();

            arb_progress part_insert_progress(partial.size());

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

                AP_tree_nlen *part_leaf  = i->release();
                AP_tree_nlen *full_seq   = const_cast<AP_tree_nlen*>(i->get_best_match());
                AP_tree_nlen *brother    = full_seq->get_brother();
                int           is_partial = 0;
                AP_tree_nlen *target     = 0;

                if (brother->is_leaf) {
                    if (brother->gb_node) {
                        is_partial = GBT_is_partial(brother->gb_node, 0, true);

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

                    count_partial_and_full(brother, &partial_count, &full_count, &zombie_count, 0, true);

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
                    AP_tree_nlen *father = part_leaf->get_father();
                    father->costs();

                    // ensure full-sequence is always on top
                    if (father->rightson == target) {
                        father->swap_sons();
                    }

                    if (!error) { // now correct the branch lengths modified by insert()
                        // calc the original branchlen (of target leaf branch)
                        GBT_LEN orglen = father->get_branchlength()+target->get_branchlength();

                        if (is_partial) { // we have a subtree of partial sequences
                            target->set_branchlength(orglen); // restore original branchlength
                            father->set_branchlength(0); // all father branches are zero length
                        }
                        else { // we have a subtree of one full+one partial sequence
                            ap_assert(full_seq->get_father() == father);

                            father->set_branchlength(orglen); // father branch represents original length (w/o partial seq)
                            full_seq->set_branchlength(0);    // full seq has no sub-branch length
                        }
                        part_leaf->set_branchlength(i->get_branchlength());
                        printf("Adding with branchlength=%f\n", i->get_branchlength());
                    }
                }
                else {
                    delete part_leaf;
                }

                part_insert_progress.inc_and_check_user_abort(error);
            }
        }
    }

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

    pars_saveNrefresh_changed_tree(ntw);
}

// -------------------------------
//      add marked / selected

static void NT_add      (AW_window * aww, AWT_canvas *ntw, AddWhat what) { nt_add(aww, ntw, what, false); }
static void NT_quick_add(AW_window * aww, AWT_canvas *ntw, AddWhat what) { nt_add(aww, ntw, what, true); }

// ------------------------------------------
//      remove and add marked / selected

static void NT_radd_internal(AW_window * aww, AWT_canvas *ntw, AddWhat what, bool quick) {
    AW_awar *awar_best_pars = aww->get_root()->awar(AWAR_BEST_PARSIMONY);
    int      oldparsval     = awar_best_pars->read_int();

    AWT_graphic_tree *agt = AWT_TREE(ntw);
    if (agt->get_root_node()) {
        agt->tree_static->remove_leafs(AWT_RemoveType(AWT_REMOVE_BUT_DONT_FREE|AWT_REMOVE_MARKED));
    }

    // restore old parsimony value (otherwise the state where species were removed would count) :
    awar_best_pars->write_int(oldparsval);

    nt_add(aww, ntw, what, quick);
}

static void NT_radd      (AW_window * aww, AWT_canvas *ntw, AddWhat what) { NT_radd_internal(aww, ntw, what, false); }
static void NT_rquick_add(AW_window * aww, AWT_canvas *ntw, AddWhat what) { NT_radd_internal(aww, ntw, what, true); }

// -------------------------------
//      Add Partial sequences


static void NT_partial_add(AW_window *aww, AW_CL cl_ntw, AW_CL) {
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    nt_add_partial(aww, ntw);
}

// --------------------------------------------------------------------------------

static void NT_branch_lengths(AW_window *, AWT_canvas *ntw) {
    arb_progress progress("Calculating Branch Lengths");
    rootEdge()->calc_branchlengths();
    AWT_TREE(ntw)->reorder_tree(BIG_BRANCHES_TO_TOP);
    pars_saveNrefresh_changed_tree(ntw);
}

static void NT_bootstrap(AW_window *, AWT_canvas *ntw, AW_CL limit_only) {
    arb_progress progress("Calculating Bootstrap Limit");
    AP_BL_MODE mode       = AP_BL_MODE((limit_only ? AP_BL_BOOTSTRAP_LIMIT : AP_BL_BOOTSTRAP_ESTIMATE)|AP_BL_BL_ONLY);
    rootEdge()->nni_rek(-1, false, mode, NULL);
    AWT_TREE(ntw)->reorder_tree(BIG_BRANCHES_TO_TOP);
    AWT_TREE(ntw)->displayed_root = AWT_TREE(ntw)->get_root_node();
    pars_saveNrefresh_changed_tree(ntw);
}

static void NT_optimize(AW_window *, AWT_canvas *ntw) {
    arb_progress progress("Optimizing Tree");
    GLOBAL_PARS->optimize_tree(rootNode(), progress);
    ASSERT_VALID_TREE(rootNode());
    rootEdge()->calc_branchlengths();
    AWT_TREE(ntw)->reorder_tree(BIG_BRANCHES_TO_TOP);
    rootNode()->compute_tree();
    pars_saveNrefresh_changed_tree(ntw);
}

static void NT_recursiveNNI(AW_window *, AWT_canvas *ntw) {
    arb_progress progress("Recursive NNI");
    AP_FLOAT orgPars = rootNode()->costs();
    AP_FLOAT prevPars = orgPars;
    progress.subtitle(GBS_global_string("Old parsimony: %.1f", orgPars));
    while (!progress.aborted()) {
        AP_FLOAT currPars = rootEdge()->nni_rek(-1, true, AP_BL_NNI_ONLY, NULL);
        if (currPars == prevPars) break; // no improvement -> abort
        progress.subtitle(GBS_global_string("New parsimony: %.1f (gain: %.1f)", currPars, orgPars-currPars));
        prevPars = currPars;
    }
    rootEdge()->calc_branchlengths();
    AWT_TREE(ntw)->reorder_tree(BIG_BRANCHES_TO_TOP);
    rootNode()->compute_tree();
    pars_saveNrefresh_changed_tree(ntw);
}

static AW_window *PARS_create_tree_settings_window(AW_root *aw_root)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init(aw_root, "SAVE_DB", "SAVE ARB DB");
    aws->load_xfig("awt/tree_settings.fig");

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help"); aws->callback(makeHelpCallback("nt_tree_settings.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("button");
    aws->auto_space(10, 10);
    aws->label_length(30);

    aws->label("Base Line Width");
    aws->create_input_field(AWAR_DTREE_BASELINEWIDTH, 4);
    aws->at_newline();

    aws->label("Relative vert. Dist");
    aws->create_input_field(AWAR_DTREE_VERICAL_DIST, 4);
    aws->at_newline();

    aws->label("Auto Jump (list tree)");
    aws->create_toggle(AWAR_DTREE_AUTO_JUMP);
    aws->at_newline();


    return (AW_window *)aws;

}

// -----------------------
//      test functions

#if defined(TESTMENU)
static void refreshTree(AWT_canvas *ntw) {
    GB_transaction ta(ntw->gb_main);

    AWT_TREE(ntw)->check_update(ntw->gb_main);
    GB_ERROR error = AWT_TREE(ntw)->save(ntw->gb_main, 0, 0, 0);
    if (error) aw_message(error);
    ntw->zoom_reset_and_refresh();
}
#endif // TESTMENU

static void setBranchlens(AP_tree_nlen *node, double newLen)
{
    node->setBranchlen(newLen, newLen);

    if (!node->is_leaf)
    {
        setBranchlens(node->get_leftson(), newLen);
        setBranchlens(node->get_rightson(), newLen);
    }
}
#if defined(TESTMENU)
static void TESTMENU_setBranchlen(AW_window *, AWT_canvas *ntw)
{
    AP_tree_nlen *root = rootNode();

    setBranchlens(root, 1.0);
    refreshTree(ntw);
}

static void TESTMENU_treeStats(AW_window *, AWT_canvas *) {
    ARB_tree_info tinfo;
    AP_tree_nlen *root = rootNode();

    if (root) {
        {
            GB_transaction ta(root->get_tree_root()->get_gb_main());
            root->calcTreeInfo(tinfo);
        }

        puts("Tree stats:");

        printf("nodes      =%6zu\n", tinfo.nodes());
        printf(" inner     =%6zu\n", tinfo.innerNodes);
        printf("  groups   =%6zu\n", tinfo.groups);
        printf(" leafs     =%6zu\n", tinfo.leafs);
        printf("  unlinked =%6zu (zombies?)\n", tinfo.unlinked);
        printf("  linked   =%6zu\n", tinfo.linked());
        printf("   marked  =%6zu\n", tinfo.marked);
    }
    else {
        puts("No tree");
    }
}

static void TESTMENU_mixTree(AW_window *, AWT_canvas *ntw)
{
    rootEdge()->mixTree(100);
    refreshTree(ntw);
}

static void TESTMENU_sortTreeByName(AW_window *, AWT_canvas *ntw)
{
    AP_tree_nlen *root = rootNode();

    root->sortByName();
    refreshTree(ntw);
}

static void TESTMENU_buildAndDumpChain(AW_window *, AWT_canvas *)
{
    AP_tree_nlen *root = rootNode();

    root->get_leftson()->edgeTo(root->get_rightson())->testChain(2);
    root->get_leftson()->edgeTo(root->get_rightson())->testChain(3);
    root->get_leftson()->edgeTo(root->get_rightson())->testChain(4);
    root->get_leftson()->edgeTo(root->get_rightson())->testChain(5);
    root->get_leftson()->edgeTo(root->get_rightson())->testChain(6);
    root->get_leftson()->edgeTo(root->get_rightson())->testChain(7);
    root->get_leftson()->edgeTo(root->get_rightson())->testChain(8);
    root->get_leftson()->edgeTo(root->get_rightson())->testChain(9);
    root->get_leftson()->edgeTo(root->get_rightson())->testChain(10);
    root->get_leftson()->edgeTo(root->get_rightson())->testChain(11);
    root->get_leftson()->edgeTo(root->get_rightson())->testChain(-1);
}

static void init_TEST_menu(AW_window_menu_modes *awm, AWT_canvas *ntw)
{
    awm->create_menu("Test[debug]", "g", AWM_ALL);

    awm->insert_menu_topic("mixtree",         "Mix tree",           "M", "", AWM_ALL, (AW_CB)TESTMENU_mixTree,           (AW_CL)ntw, 0);
    awm->insert_menu_topic("treestat",        "Tree statistics",    "s", "", AWM_ALL, (AW_CB)TESTMENU_treeStats,         (AW_CL)ntw, 0);
    awm->insert_menu_topic("setlens",         "Set branchlens",     "b", "", AWM_ALL, (AW_CB)TESTMENU_setBranchlen,      (AW_CL)ntw, 0);
    awm->insert_menu_topic("sorttreebyname",  "Sort tree by name",  "o", "", AWM_ALL, (AW_CB)TESTMENU_sortTreeByName,    (AW_CL)ntw, 0);
    awm->insert_menu_topic("buildndumpchain", "Build & dump chain", "c", "", AWM_ALL, (AW_CB)TESTMENU_buildAndDumpChain, (AW_CL)ntw, 0);
}
#endif // TESTMENU

static GB_ERROR pars_check_size(AW_root *awr, GB_ERROR& warning) {
    GB_ERROR error = NULL;
    warning        = NULL;

    char *tree_name = awr->awar(AWAR_TREE)->read_string();
    char *filter    = awr->awar(AWAR_FILTER_FILTER)->read_string();
    long  ali_len   = 0;

    if (strlen(filter)) {
        int i;
        for (i=0; filter[i]; i++) {
            if (filter[i] != '0') ali_len++;
        }
    }
    else {
        char *ali_name = awr->awar(AWAR_ALIGNMENT)->read_string();
        ali_len        = GBT_get_alignment_len(GLOBAL_gb_main, ali_name);
        if (ali_len<=0) {
            error = "Please select a valid alignment";
            GB_clear_error();
        }
        free(ali_name);
    }

    if (!error) {
        long tree_size = GBT_size_of_tree(GLOBAL_gb_main, tree_name);
        if (tree_size == -1) {
            error = "Please select an existing tree";
        }
        else {
            unsigned long expected_memuse = (ali_len * tree_size * 4 / 1024);
            if (expected_memuse > GB_get_usable_memory()) {
                warning = GBS_global_string("Estimated memory usage (%s) exceeds physical memory (will swap)\n"
                                            "(did you specify a filter?)",
                                            GBS_readable_size(expected_memuse, "b"));
            }
        }
    }

    free(filter);
    free(tree_name);

    ap_assert(!GB_have_error());
    return error;
}

static void pars_reset_optimal_parsimony(AW_window *aww, AW_CL *cl_ntw) {
    AW_root *awr = aww->get_root();
    awr->awar(AWAR_BEST_PARSIMONY)->write_int(awr->awar(AWAR_PARSIMONY)->read_int());

    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    ntw->refresh();
}


static void pars_start_cb(AW_window *aw_parent, WeightedFilter *wfilt, const PARS_commands *cmds) {
    AW_root *awr = aw_parent->get_root();
    GB_begin_transaction(GLOBAL_gb_main);
    {
        GB_ERROR warning;
        GB_ERROR error = pars_check_size(awr, warning);

        if (warning && !error) {
            char *question = GBS_global_string_copy("%s\nDo you want to continue?", warning);
            bool  cont     = aw_ask_sure("swap_warning", question);
            free(question);

            if (!cont) error = "User abort";

        }

        if (error) {
            aw_message(error);
            GB_commit_transaction(GLOBAL_gb_main);
            return;
        }
    }


    AW_window_menu_modes *awm = new AW_window_menu_modes;
    awm->init(awr, "ARB_PARSIMONY", "ARB_PARSIMONY", 400, 200);

    GLOBAL_PARS->generate_tree(wfilt);

    AWT_canvas *ntw;
    {
        AP_tree_display_type  old_sort_type = global_tree()->tree_sort;
        global_tree()->set_tree_type(AP_LIST_SIMPLE, NULL); // avoid NDS warnings during startup
        ntw = new AWT_canvas(GLOBAL_gb_main, awm, awm->get_window_id(), global_tree(), AWAR_TREE);
        global_tree()->set_tree_type(old_sort_type, ntw);
    }

    {
        GB_ERROR error = 0;
        arb_progress progress("loading tree");
        NT_reload_tree_event(awr, ntw, 0);             // load tree (but do not expose - first zombies need to be removed)
        if (!global_tree()->get_root_node()) {
            error = "Failed to load the selected tree";
        }
        else {
            AP_tree_edge::initialize(rootNode());   // builds edges
            long removed = global_tree()->tree_static->remove_leafs(AWT_REMOVE_ZOMBIES);

            PARS_tree_init(global_tree());
            removed += global_tree()->tree_static->remove_leafs(AWT_RemoveType(AWT_REMOVE_ZOMBIES | AWT_REMOVE_NO_SEQUENCE));

            if (!global_tree()->get_root_node()) {
                const char *aliname = global_tree()->tree_static->get_aliview()->get_aliname();
                error               = GBS_global_string("Less than 2 species contain data in '%s'\n"
                                                        "Tree vanished", aliname);
            }
            else if (removed) {
                aw_message(GBS_global_string("Removed %li leafs (zombies or species w/o data in alignment)", removed));
            }

            error = GB_end_transaction(ntw->gb_main, error);
            if (!error) {
                progress.subtitle("Calculating inner nodes");
                GLOBAL_PARS->get_root_node()->costs();
            }
        }
        if (error) aw_popup_exit(error);
    }

    awr->awar(AWAR_COLOR_GROUPS_USE)->add_callback(makeRootCallback(TREE_recompute_cb, ntw));

    if (cmds->add_marked)           NT_quick_add(awm, ntw, NT_ADD_MARKED);
    if (cmds->add_selected)         NT_quick_add(awm, ntw, NT_ADD_SELECTED);
    if (cmds->calc_branch_lengths)  NT_branch_lengths(awm, ntw);
    if (cmds->calc_bootstrap)       NT_bootstrap(awm, ntw, 0);
    if (cmds->quit)                 pars_exit(awm);

    GB_transaction ta(ntw->gb_main);

    GBDATA *gb_arb_presets = GB_search(ntw->gb_main, "arb_presets", GB_CREATE_CONTAINER);
    GB_add_callback(gb_arb_presets, GB_CB_CHANGED, makeDatabaseCallback(AWT_expose_cb, ntw));

#if defined(DEBUG)
    AWT_create_debug_menu(awm);
#endif // DEBUG

    awm->create_menu("File", "F", AWM_ALL);
    {
        insert_macro_menu_entry(awm, false);
        awm->insert_menu_topic("print_tree", "Print Tree ...", "P", "tree2prt.hlp", AWM_ALL, makeWindowCallback(AWT_popup_print_window, ntw));
        awm->insert_menu_topic("quit",       "Quit",           "Q", "quit.hlp",     AWM_ALL, pars_exit);
    }

    awm->create_menu("Species", "S", AWM_ALL);
    {
        NT_insert_mark_submenus(awm, ntw, 0);

    }
    awm->create_menu("Tree", "T", AWM_ALL);
    {

        awm->insert_menu_topic("nds",       "NDS (Node Display Setup) ...",      "N", "props_nds.hlp",   AWM_ALL, makeCreateWindowCallback(AWT_create_nds_window, ntw->gb_main));

        awm->sep______________();
        awm->insert_menu_topic("tree_2_xfig", "Export tree to XFIG ...", "E", "tree2file.hlp", AWM_ALL, makeWindowCallback(AWT_popup_tree_export_window, ntw));
        awm->insert_menu_topic("tree_print",  "Print tree ...",          "P", "tree2prt.hlp",  AWM_ALL, makeWindowCallback(AWT_popup_print_window,       ntw));
        awm->sep______________();
        awm->insert_sub_menu("Collapse/Expand Tree",         "C");
        {
            awm->insert_menu_topic("tree_group_all",        "Group all",      "a", "tgroupall.hlp",   AWM_ALL, makeWindowCallback(NT_group_tree_cb,       ntw));
            awm->insert_menu_topic("tree_group_not_marked", "Group unmarked", "m", "tgroupnmrkd.hlp", AWM_ALL, makeWindowCallback(NT_group_not_marked_cb, ntw));
            awm->insert_menu_topic("tree_ungroup_all",      "Ungroup all",    "U", "tungroupall.hlp", AWM_ALL, makeWindowCallback(NT_ungroup_all_cb,      ntw));
            awm->sep______________();
            NT_insert_color_collapse_submenu(awm, ntw);
        }
        awm->close_sub_menu();
        awm->sep______________();
        awm->insert_sub_menu("Remove Species from Tree",     "R");
        {
            awm->insert_menu_topic("tree_remove_deleted", "Remove Zombies", "Z", "trm_del.hlp",    AWM_ALL, makeWindowCallback(NT_remove_leafs, ntw, AWT_RemoveType(AWT_REMOVE_BUT_DONT_FREE|AWT_REMOVE_ZOMBIES)));
            awm->insert_menu_topic("tree_remove_marked",  "Remove Marked",  "M", "trm_mrkd.hlp",   AWM_ALL, makeWindowCallback(NT_remove_leafs, ntw, AWT_RemoveType(AWT_REMOVE_BUT_DONT_FREE|AWT_REMOVE_MARKED)));
            awm->insert_menu_topic("tree_keep_marked",    "Keep Marked",    "K", "tkeep_mrkd.hlp", AWM_ALL, makeWindowCallback(NT_remove_leafs, ntw, AWT_RemoveType(AWT_REMOVE_BUT_DONT_FREE|AWT_KEEP_MARKED)));
        }
        awm->close_sub_menu();
        awm->insert_sub_menu("Add Species to Tree",      "A");
        {
            awm->insert_menu_topic("add_marked",         "Add Marked Species",                              "M", "pa_quick.hlp",     AWM_ALL, (AW_CB)NT_quick_add,  (AW_CL)ntw, NT_ADD_MARKED);
            awm->insert_menu_topic("add_marked_nni",     "Add Marked Species + Local Optimization (NNI)",   "N", "pa_add.hlp",       AWM_ALL, (AW_CB)NT_add,        (AW_CL)ntw, NT_ADD_MARKED);
            awm->insert_menu_topic("rm_add_marked",      "Remove & Add Marked Species",                     "R", "pa_add.hlp",       AWM_ALL, (AW_CB)NT_rquick_add, (AW_CL)ntw, NT_ADD_MARKED);
            awm->insert_menu_topic("rm_add_marked_nni|", "Remove & Add Marked + Local Optimization (NNI)",  "L", "pa_add.hlp",       AWM_ALL, (AW_CB)NT_radd,       (AW_CL)ntw, NT_ADD_MARKED);
            awm->sep______________();
            awm->insert_menu_topic("add_marked_partial", "Add Marked Partial Species",                      "P", "pa_partial.hlp",   AWM_ALL, NT_partial_add,       (AW_CL)ntw, (AW_CL)0);
            awm->sep______________();
            awm->insert_menu_topic("add_selected",       "Add Selected Species",                            "S", "pa_quick_sel.hlp", AWM_ALL, (AW_CB)NT_quick_add,  (AW_CL)ntw, NT_ADD_SELECTED);
            awm->insert_menu_topic("add_selected_nni",   "Add Selected Species + Local Optimization (NNI)", "O", "pa_add_sel.hlp",   AWM_ALL, (AW_CB)NT_add,        (AW_CL)ntw, NT_ADD_SELECTED);
        }
        awm->close_sub_menu();
        awm->sep______________();
        awm->insert_sub_menu("Tree Optimization",        "O");
        {
            awm->insert_menu_topic("nni",           "Local Optimization (NNI) of Marked Visible Nodes", "L", "",        AWM_ALL,    (AW_CB)NT_recursiveNNI, (AW_CL)ntw, 0);
            awm->insert_menu_topic("kl_optimization",   "Global Optimization of Marked Visible Nodes",      "G", "pa_optimizer.hlp", AWM_ALL,   (AW_CB)NT_optimize, (AW_CL)ntw, 0);
        }
        awm->close_sub_menu();
        awm->insert_menu_topic("reset", "Reset optimal parsimony", "s", "", AWM_ALL, (AW_CB)pars_reset_optimal_parsimony, (AW_CL)ntw, 0);
        awm->sep______________();
        awm->insert_menu_topic("beautify_tree",       "Beautify Tree",            "B", "resorttree.hlp",       AWM_ALL, makeWindowCallback(NT_resort_tree_cb, ntw, BIG_BRANCHES_TO_TOP));
        awm->insert_menu_topic("calc_branch_lengths", "Calculate Branch Lengths", "L", "pa_branchlengths.hlp", AWM_ALL, (AW_CB)NT_branch_lengths, (AW_CL)ntw, 0);
        awm->sep______________();
        awm->insert_menu_topic("calc_upper_bootstrap_indep", "Calculate Upper Bootstrap Limit (dependent NNI)",   "d", "pa_bootstrap.hlp", AWM_ALL, (AW_CB)NT_bootstrap,        (AW_CL)ntw, 0);
        awm->insert_menu_topic("calc_upper_bootstrap_dep",   "Calculate Upper Bootstrap Limit (independent NNI)", "i", "pa_bootstrap.hlp", AWM_ALL, (AW_CB)NT_bootstrap,        (AW_CL)ntw, 1);
        awm->insert_menu_topic("tree_remove_remark",         "Remove Bootstrap Values",                           "V", "trm_boot.hlp",     AWM_ALL, makeWindowCallback(NT_remove_bootstrap, ntw));
    }

#if defined(TESTMENU)
    init_TEST_menu(awm, ntw);
#endif // TESTMENU

    awm->create_menu("Reset", "R", AWM_ALL);
    {
        awm->insert_menu_topic("reset_logical_zoom",  "Logical Zoom",  "L", "rst_log_zoom.hlp",  AWM_ALL, makeWindowCallback(NT_reset_lzoom_cb, ntw));
        awm->insert_menu_topic("reset_physical_zoom", "Physical Zoom", "P", "rst_phys_zoom.hlp", AWM_ALL, makeWindowCallback(NT_reset_pzoom_cb, ntw));
    }

    awm->create_menu("Properties", "P", AWM_ALL);
    {
        awm->insert_menu_topic("props_menu",  "Menu: Colors and Fonts ...", "M", "props_frame.hlp",      AWM_ALL, AW_preset_window);
        awm->insert_menu_topic("props_tree",  "Tree: Colors and Fonts ...", "C", "pars_props_data.hlp",  AWM_ALL, makeCreateWindowCallback(AW_create_gc_window, ntw->gc_manager));
        awm->insert_menu_topic("props_tree2", "Tree: Settings ...",         "T", "nt_tree_settings.hlp", AWM_ALL, PARS_create_tree_settings_window);
        awm->insert_menu_topic("props_kl",    "KERN. LIN ...",              "K", "kernlin.hlp",          AWM_ALL, AW_POPUP,(AW_CL)create_kernighan_window, 0);
        awm->sep______________();
        AW_insert_common_property_menu_entries(awm);
        awm->sep______________();
        awm->insert_menu_topic("save_props", "Save Defaults (pars.arb)", "D", "savedef.hlp", AWM_ALL, AW_save_properties);
    }
    awm->button_length(5);

    awm->insert_help_topic("ARB_PARSIMONY help", "P", "arb_pars.hlp", AWM_ALL, makeHelpCallback("arb_pars.hlp"));

    // ----------------------
    //      mode buttons
    //
    // keep them synchronized as far as possible with those in ARB_PARSIMONY
    // see ../NTREE/NT_extern.cxx@keepModesSynchronized

    awm->create_mode("mode_select.xpm", "mode_select.hlp", AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_SELECT));
    awm->create_mode("mode_mark.xpm",   "mode_mark.hlp",   AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_MARK));
    awm->create_mode("mode_group.xpm",  "mode_group.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_GROUP));
    awm->create_mode("mode_zoom.xpm",   "mode_pzoom.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_ZOOM));
    awm->create_mode("mode_lzoom.xpm",  "mode_lzoom.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_LZOOM));

    // reserve mode-locations (to put the modes below at the same position as in ARB_NT)
    awm->create_mode("mode_empty.xpm", "mode.hlp", AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_EMPTY));
    awm->create_mode("mode_empty.xpm", "mode.hlp", AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_EMPTY));

    // topology-modification-modes
    awm->create_mode("mode_setroot.xpm", "mode_setroot.hlp", AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_SETROOT));
    awm->create_mode("mode_swap.xpm",    "mode_swap.hlp",    AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_SWAP));
    awm->create_mode("mode_move.xpm",    "mode_move.hlp",    AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_MOVE));

    awm->create_mode("mode_nni.xpm",      "mode_nni.hlp",      AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_NNI));
    awm->create_mode("mode_kernlin.xpm",  "mode_kernlin.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_KERNINGHAN));
    awm->create_mode("mode_optimize.xpm", "mode_optimize.hlp", AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_OPTIMIZE));

    awm->at(5, 2);
    awm->auto_space(0, -2);
    awm->shadow_width(1);


    int db_treex, db_treey;
    awm->get_at_position(&db_treex, &db_treey);
    awm->callback(makeHelpCallback("nt_tree_select.hlp"));
    awm->button_length(16);
    awm->help_text("nt_tree_select.hlp");
    awm->create_button(0, AWAR_TREE);


    int db_stackx, db_stacky;
    awm->label_length(8);
    awm->label("Stored");
    awm->get_at_position(&db_stackx, &db_stacky);
    awm->button_length(6);
    awm->callback(makeHelpCallback("ap_stack.hlp"));
    awm->help_text("ap_stack.hlp");
    awm->create_button(0, AWAR_STACKPOINTER);

    int db_parsx, db_parsy;
    awm->label_length(14);
    awm->label("Current Pars:");
    awm->get_at_position(&db_parsx, &db_parsy);

    awm->button_length(10);
    awm->create_button(0, AWAR_PARSIMONY, 0, "+");

    awm->button_length(0);

    awm->callback(makeWindowCallback(NT_jump_cb, ntw, AP_JUMP_BY_BUTTON));
    awm->help_text("tr_jump.hlp");
    awm->create_button("JUMP", "Jump", 0);

    awm->callback(makeHelpCallback("arb_pars.hlp"));
    awm->help_text("help.hlp");
    awm->create_button("HELP", "HELP", "H");

    awm->at_newline();

    awm->button_length(8);
    awm->at_x(db_stackx);
    awm->callback((AW_CB1)AP_user_pop_cb, (AW_CL)ntw);
    awm->help_text("ap_stack.hlp");
    awm->create_button("POP", "RESTORE", 0);

    awm->button_length(6);
    awm->callback((AW_CB1)AP_user_push_cb, (AW_CL)ntw);
    awm->help_text("ap_stack.hlp");
    awm->create_button("PUSH", "STORE", 0);

    awm->at_x(db_parsx);
    awm->label_length(14);
    awm->label("Optimal Pars:");

    awm->button_length(10);
    awm->create_button(0, AWAR_BEST_PARSIMONY, 0, "+");

    awm->button_length(0);
    awm->auto_space(0, -2);

    awm->at_x(db_treex);
    awm->callback(makeWindowCallback(NT_set_tree_style, ntw, AP_TREE_RADIAL));
    awm->help_text("tr_type_radial.hlp");
    awm->create_button("RADIAL_TREE", "#radial.xpm", 0);

    awm->callback(makeWindowCallback(NT_set_tree_style, ntw, AP_TREE_NORMAL));
    awm->help_text("tr_type_list.hlp");
    awm->create_button("LIST_TREE", "#dendro.xpm", 0);

    awm->at_newline();
    awm->at(db_treex, awm->get_at_yposition());

    {
        AW_at_maxsize maxSize; // store size (so AWAR_FOOTER does not affect min. window size)
        maxSize.store(awm->_at);
        awm->button_length(AWAR_FOOTER_MAX_LEN);
        awm->create_button(0, AWAR_FOOTER);
        awm->at_newline();
        maxSize.restore(awm->_at);
    }

    awm->get_at_position(&db_treex, &db_treey);
    awm->set_info_area_height(db_treey);

    awm->set_bottom_area_height(0);

    aw_parent->hide(); // hide parent
    awm->show();

    AP_user_push_cb(aw_parent, ntw); // push initial tree
}

static AW_window *create_pars_init_window(AW_root *awr, const PARS_commands *cmds) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(awr, "PARS_PROPS", "SET PARSIMONY OPTIONS");
    aws->load_xfig("pars/init.fig");

    aws->button_length(10);
    aws->label_length(10);

    aws->callback(pars_exit);
    aws->at("close");
    aws->create_button("ABORT", "ABORT", "A");

    aws->callback(makeHelpCallback("arb_pars_init.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    WeightedFilter *weighted_filter = // do NOT free (bound to callbacks)
        new WeightedFilter(GLOBAL_gb_main, aws->get_root(), AWAR_FILTER_NAME, AWAR_COLUMNSTAT_NAME, aws->get_root()->awar_string(AWAR_ALIGNMENT));

    aws->at("filter");
    aws->callback(makeCreateWindowCallback(awt_create_select_filter_win, weighted_filter->get_adfiltercbstruct()));
    aws->create_button("SELECT_FILTER", AWAR_FILTER_NAME);

    aws->at("weights");
    aws->callback(makeCreateWindowCallback(COLSTAT_create_selection_window, weighted_filter->get_column_stat()));
    aws->sens_mask(AWM_EXP);
    aws->create_button("SELECT_CSP", AWAR_COLUMNSTAT_NAME);
    aws->sens_mask(AWM_ALL);

    aws->at("alignment");
    awt_create_selection_list_on_alignments(GLOBAL_gb_main, (AW_window *)aws, AWAR_ALIGNMENT, "*=");

    aws->at("tree");
    awt_create_selection_list_on_trees(GLOBAL_gb_main, aws, AWAR_TREE, true);

    aws->callback(makeWindowCallback(pars_start_cb, weighted_filter, cmds));
    aws->at("go");
    aws->create_button("GO", "GO", "G");

    return aws;
}

static void create_parsimony_variables(AW_root *aw_root, AW_default db) {
    // kernighan

    aw_root->awar_float("genetic/kh/nodes", 1.7, db);
    aw_root->awar_int("genetic/kh/maxdepth", 15, db);
    aw_root->awar_int("genetic/kh/incdepth", 5, db);

    aw_root->awar_int("genetic/kh/static/enable", 1, db);
    aw_root->awar_int("genetic/kh/static/depth0", 2, db);
    aw_root->awar_int("genetic/kh/static/depth1", 2, db);
    aw_root->awar_int("genetic/kh/static/depth2", 2, db);
    aw_root->awar_int("genetic/kh/static/depth3", 2, db);
    aw_root->awar_int("genetic/kh/static/depth4", 1, db);

    aw_root->awar_int("genetic/kh/dynamic/enable", 1,   db);
    aw_root->awar_int("genetic/kh/dynamic/start",  100, db);
    aw_root->awar_int("genetic/kh/dynamic/maxx",   6,   db);
    aw_root->awar_int("genetic/kh/dynamic/maxy",   150, db);

    aw_root->awar_int("genetic/kh/function_type", AP_QUADRAT_START, db);

    awt_create_dtree_awars(aw_root, db);
}

static void pars_create_all_awars(AW_root *awr, AW_default aw_def)
{
    awr->awar_string(AWAR_SPECIES_NAME, "",     GLOBAL_gb_main);
    awr->awar_string(AWAR_FOOTER,       "",     aw_def);
    awr->awar_string(AWAR_FILTER_NAME,  "none", GLOBAL_gb_main);

    {
        GB_transaction  ta(GLOBAL_gb_main);
        char           *dali = GBT_get_default_alignment(GLOBAL_gb_main);

        awr->awar_string(AWAR_ALIGNMENT, dali, GLOBAL_gb_main)->write_string(dali);
        free(dali);
    }

    awt_set_awar_to_valid_filter_good_for_tree_methods(GLOBAL_gb_main, awr, AWAR_FILTER_NAME);

    awr->awar_string(AWAR_FILTER_FILTER,    "", GLOBAL_gb_main);
    awr->awar_string(AWAR_FILTER_ALIGNMENT, "", aw_def);

    awr->awar(AWAR_FILTER_ALIGNMENT)->map(AWAR_ALIGNMENT);

    awr->awar_int(AWAR_PARS_TYPE, PARS_WAGNER, GLOBAL_gb_main);

    {
        GB_transaction  ta(GLOBAL_gb_main);
        GBDATA *gb_tree_name = GB_search(GLOBAL_gb_main, AWAR_TREE, GB_STRING);
        char   *tree_name    = GB_read_string(gb_tree_name);

        awr->awar_string(AWAR_TREE, "", aw_def)->write_string(tree_name);
        free(tree_name);
    }

    awr->awar_int(AWAR_PARSIMONY,      0, aw_def);
    awr->awar_int(AWAR_BEST_PARSIMONY, 0, aw_def);
    awr->awar_int(AWAR_STACKPOINTER,   0, aw_def);

    create_parsimony_variables(awr, GLOBAL_gb_main);
    create_nds_vars(awr, aw_def, GLOBAL_gb_main);

#if defined(DEBUG)
    AWT_create_db_browser_awars(awr, aw_def);
#endif // DEBUG

    GB_ERROR error = ARB_init_global_awars(awr, aw_def, GLOBAL_gb_main);
    if (error) aw_message(error);
}

static AW_root *AD_map_viewer_aw_root = 0;

void PARS_map_viewer(GBDATA *gb_species, AD_MAP_VIEWER_TYPE vtype) {
    if (vtype == ADMVT_SELECT && AD_map_viewer_aw_root && gb_species) {
        AD_map_viewer_aw_root->awar(AWAR_SPECIES_NAME)->write_string(GBT_read_name(gb_species));
    }
}

int ARB_main(int argc, char *argv[]) {
    aw_initstatus();

    GB_shell shell;
    AW_root *aw_root      = AWT_create_root("pars.arb", "ARB_PARS", need_macro_ability(), &argc, &argv);
    AD_map_viewer_aw_root = aw_root;

    ap_main     = new AP_main;
    GLOBAL_PARS = new ArbParsimony(aw_root);

    const char *db_server = ":";

    PARS_commands cmds;

    while (argc>=2 && argv[1][0] == '-') {
        argc--;
        argv++;
        if (!strcmp(argv[0], "-quit"))                   cmds.quit = 1;
        else if (!strcmp(argv[0], "-add_marked"))        cmds.add_marked = 1;
        else if (!strcmp(argv[0], "-add_selected"))      cmds.add_selected = 1;
        else if (!strcmp(argv[0], "-calc_branchlengths")) cmds.calc_branch_lengths = 1;
        else if (!strcmp(argv[0], "-calc_bootstrap"))    cmds.calc_bootstrap = 1;
        else {
            fprintf(stderr, "Unknown option '%s'\n", argv[0]);

            printf("    Options:                Meaning:\n"
                   "\n"
                   "    -add_marked             add marked species   (without changing topology)\n"
                   "    -add_selected           add selected species (without changing topology)\n"
                   "    -calc_branchlengths     calculate branch lengths only\n"
                   "    -calc_bootstrap         estimate bootstrap values\n"
                   "    -quit                   quit after performing operations\n"
                   );

            exit(EXIT_FAILURE);
        }
    }


    if (argc==2) db_server = argv[1];

    GB_ERROR error = NULL;
    GLOBAL_gb_main = GBT_open(db_server, "rw");

    if (!GLOBAL_gb_main) error = GB_await_error();
    else {
        error = configure_macro_recording(aw_root, "ARB_PARS", GLOBAL_gb_main);

        if (!error) {
#if defined(DEBUG)
            AWT_announce_db_to_browser(GLOBAL_gb_main, GBS_global_string("ARB-database (%s)", db_server));
#endif // DEBUG

            pars_create_all_awars(aw_root, AW_ROOT_DEFAULT);

            AW_window *aww = create_pars_init_window(aw_root, &cmds);
            aww->show();

            AWT_install_cb_guards();
            aw_root->main_loop();
        }
    }

    if (error) aw_popup_exit(error);
    return EXIT_SUCCESS;
}


