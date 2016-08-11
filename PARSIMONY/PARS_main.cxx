// =============================================================== //
//                                                                 //
//   File      : PARS_main.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "PerfMeter.h"
#include "pars_main.hxx"
#include "pars_klprops.hxx"
#include "pars_awars.h"
#include "ap_tree_nlen.hxx"
#include "ap_main.hxx"

#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>

#include <awt.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_filter.hxx>

#include <ColumnStat.hxx>
#include <nds.h>
#include <arb_progress.h>
#include <arb_misc.h>
#include <arb_defs.h>

#include <gui_aliview.hxx>
#include <ad_cb.h>

#include <TreeCallbacks.hxx>

#include <list>
#include <macros.hxx>
#include <awt_config_manager.hxx>

#if defined(DEBUG)
# define TESTMENU
#endif // DEBUG

using namespace std;

AW_HEADER_MAIN

#define AWAR_COLUMNSTAT_BASE "tmp/pars/colstat"
#define AWAR_COLUMNSTAT_NAME AWAR_COLUMNSTAT_BASE "/name"

#define AWT_TREE_PARS(ntw) DOWNCAST(AWT_graphic_parsimony*, (ntw)->gfx)

static ArbParsimony *GLOBAL_PARS = NULL;

inline AWT_graphic_parsimony *global_tree() { return GLOBAL_PARS->get_tree(); }
inline AP_pars_root *global_tree_root() { return global_tree()->get_tree_root(); }

// waaah more globals :(
AP_main *ap_main; // @@@ move into ArbParsimony? or eliminate ArbParsimony

void ArbParsimony::set_tree(AWT_graphic_parsimony *tree_) {
    ap_assert(!tree); // only call once
    tree = tree_;
    ap_main->set_tree_root(tree);
}

static void pars_saveNrefresh_changed_tree(AWT_canvas *ntw) {
    ap_assert((AWT_TREE(ntw) == global_tree()));

    GB_ERROR error = global_tree()->save(ntw->gb_main, NULL);
    if (error) aw_message(error);

    ntw->zoom_reset_and_refresh();
}

static void set_keep_ghostnodes() {
    // avoid that saving tree to DB does delete removed nodes
    // (hack to fix #528)
    // see ../ARBDB/adtree.cxx@keep_ghostnodes
    GBDATA         *gb_tree = ap_main->get_tree_root()->get_gb_tree();
    GB_transaction  ta(gb_tree);
    GBDATA         *gb_keep = GB_searchOrCreate_int(gb_tree, "keep_ghostnodes", 1);
    ASSERT_NO_ERROR(GB_set_temporary(gb_keep));
}
static void delete_kept_ghostnodes() {
    GBDATA         *gb_tree = ap_main->get_tree_root()->get_gb_tree();
    GB_transaction  ta(gb_tree);

    GBDATA *gb_keep = GB_entry(gb_tree, "keep_ghostnodes");
    if (gb_keep) { // e.g. wrong for quick-add species
        GB_ERROR error = GB_delete(gb_keep);
        if (!error) {
            if (ap_main->get_tree_root()->was_saved()) {
                // if tree was saved, DB may contain ghostnodes
                // -> save again to delete them
                error = global_tree()->save(GB_get_root(gb_tree), NULL);
            }
        }
        if (error) aw_message(error);
    }
}

__ATTR__NORETURN static void pars_exit(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    shutdown_macro_recording(aw_root);

    ap_main->accept_all();
    delete_kept_ghostnodes();

    aw_root->unlink_awars_from_DB(ap_main->get_gb_main());
#if defined(DEBUG)
    AWT_browser_forget_db(ap_main->get_gb_main());
#endif // DEBUG
    delete ap_main; // closes DB
    ap_main = NULL;

    exit(EXIT_SUCCESS);
}

static void AP_user_push_cb(AW_window *aww, AWT_canvas *) {
    ap_main->remember_user_state();
    aww->get_root()->awar(AWAR_STACKPOINTER)->write_int(ap_main->get_user_push_counter());
}

static void AP_user_pop_cb(AW_window *aww, AWT_canvas *ntw) {
    if (ap_main->get_user_push_counter()<=0) {
        aw_message("No tree on stack.");
        return;
    }
    ap_main->revert_user_state();
    rootNode()->compute_tree();
    ASSERT_VALID_TREE(rootNode());
    aww->get_root()->awar(AWAR_STACKPOINTER)->write_int(ap_main->get_user_push_counter());

    pars_saveNrefresh_changed_tree(ntw);

    if (ap_main->get_user_push_counter() <= 0) { // last tree was popped => push again
        AP_user_push_cb(aww, ntw);
    }
}

class InsertData {
    bool         abort_flag;
    arb_progress progress;

public:

    bool quick_add_flag;
    InsertData(bool quick, long spec_count)
        : abort_flag(false),
          progress(GBS_global_string("Inserting %li species", spec_count), spec_count),
          quick_add_flag(quick)
    {
    }

    bool aborted() const { return abort_flag; }
    void set_aborted(bool aborted_) { abort_flag = aborted_; }

    void inc() {
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
    AP_pars_root *troot   = ap_main->get_tree_root();
    AP_tree_nlen *leaf    = DOWNCAST(AP_tree_nlen*, troot->makeNode());

    leaf->forget_origin(); // new leaf is not part of tree yet

    leaf->gb_node = gb_node;
    leaf->name    = ARB_strdup(key);
    leaf->is_leaf = true;

    leaf->set_seq(troot->get_seqTemplate()->dup());
    GB_ERROR error = leaf->get_seq()->bind_to_species(gb_node);
    if (!error) {
        if (leaf->get_seq()->weighted_base_count() < MIN_SEQUENCE_LENGTH) {
            error = GBS_global_string("Species %s has too short sequence (%f, minimum is %i)",
                                      key,
                                      leaf->get_seq()->weighted_base_count(),
                                      MIN_SEQUENCE_LENGTH);
        }
    }
    if (error) {
        GBT_message(gb_node, error);
        destroy(leaf, troot); leaf = 0;
    }
    return (long)leaf;
}

static AP_tree_nlen *insert_species_in_tree(AP_tree_nlen *leaf, InsertData *isits) {
    if (!leaf) return leaf;
    if (isits->aborted()) return leaf;

    ap_assert(leaf->get_seq()->weighted_base_count() >= MIN_SEQUENCE_LENGTH);

    AP_tree_nlen *tree = rootNode();
    if (!tree) {                                   // no tree yet
        static AP_tree_nlen *last_inserted = NULL; // @@@ move 'last_inserted' into 'InsertData'

        if (!last_inserted) {                       // store 1st leaf
            last_inserted = leaf;
        }
        else {                                      // 2nd leaf -> create initial tree
            AP_pars_root *troot = ap_main->get_tree_root();

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
            long bsum = 0;
            tree->buildBranchList(branchlist, bsum, true, -1); // get all branches
        }

        AP_tree_nlen *bestposl = tree->get_leftson();
        AP_tree_nlen *bestposr = tree->get_rightson();

        ap_assert(tree == rootNode());
        leaf->insert(bestposl);
        tree = NULL;                                // tree may have changed

        bool final_move = false;
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

                ASSERT_VALID_TREE(rootNode());
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
                    final_move = false;
                }
                else {
                    final_move = true;
                }
            }
        }
        delete [] branchlist; branchlist = 0;

        ASSERT_VALID_TREE(rootNode());

        if (final_move) {
            if (bestposr->father == bestposl) std::swap(bestposr, bestposl);
            if (leaf->father != bestposl->father) {
                leaf->moveNextTo(bestposl, 0.5);
            }
        }

        ASSERT_VALID_TREE(rootNode());

        // @@@ section below should better be handled using "partial species"
        AP_tree_nlen *brother = leaf->get_brother();
        if (brother->is_leaf && leaf->father->father) {
            bool brother_is_short = 2 * brother->get_seq()->weighted_base_count() < leaf->get_seq()->weighted_base_count();

            if (brother_is_short) {
                brother->REMOVE();
                leaf->REMOVE();

                for (int firstUse = 1; firstUse >= 0; --firstUse) {
                    AP_tree_nlen *to_insert = firstUse ? leaf : brother;
                    insert_species_in_tree(to_insert, isits);
                }
            }
        }

        ASSERT_VALID_TREE(rootNode());
    }

    return leaf;
}

static long hash_insert_species_in_tree(const char *, long leaf, void *cd_isits) {
    InsertData *isits  = (InsertData*)cd_isits;
    long        result = (long)insert_species_in_tree((AP_tree_nlen*)leaf, isits);
    isits->inc();
    return result;
}

enum AddWhat {
    NT_ADD_MARKED,
    NT_ADD_SELECTED,
};

static void nt_add(AWT_graphic_parsimony *agt, AddWhat what, bool quick) {
    GB_ERROR  error = 0;

    AP_tree *oldrootleft  = NULL;
    AP_tree *oldrootright = NULL;
    {
        AP_tree_nlen *root = rootNode();
        if (root) {
            root->reset_subtree_layout();
            oldrootleft  = root->get_leftson();
            oldrootright = root->get_rightson();
        }
    }

    GB_HASH *hash    = 0;
    GBDATA  *gb_main = agt->gb_main;
    {
        GB_transaction ta(gb_main);
        switch (what) {
            case NT_ADD_SELECTED: {
                char *name = GBT_readOrCreate_string(gb_main, AWAR_SPECIES_NAME, "");
                if (name && strlen(name)) {
                    GBDATA *gb_species = GBT_find_species(gb_main, name);
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
                hash = GBT_create_marked_species_hash(gb_main);
                break;
            }
        }
    }

    if (!error) {
        ap_assert(hash);

        arb_progress progress(quick ? "Quick add" : "Add + NNI");

        NT_remove_species_in_tree_from_hash(rootNode(), hash);

        size_t          species_count = GBS_hash_elements(hash);
        InsertPerfMeter insertPerf("(quick-)add", species_count);

        {
            InsertData isits(quick, species_count);
            {
                GB_transaction ta(gb_main);
                GBS_hash_do_loop(hash, transform_gbd_to_leaf, NULL);
            }
            {
                size_t skipped = species_count - GBS_hash_elements(hash);
                if (skipped) {
                    GBT_message(gb_main, GBS_global_string("Skipped %zu species (no data?)", skipped));
                    isits.get_progress().inc_by(skipped);
                }
            }
            GBS_hash_do_sorted_loop(hash, hash_insert_species_in_tree, sort_sequences_by_length, &isits);
        }

        if (rootNode()) {
            if (oldrootleft) {
                if (oldrootleft->father == oldrootright) oldrootleft->set_root();
                else                                     oldrootright->set_root();
            }
            else {
                ARB_edge innermost = rootNode()->get_tree_root()->find_innermost_edge();
                innermost.set_root();
            }

            if (!quick) {
                AP_FLOAT pars_prev = rootNode()->costs();
                rootNode()->compute_tree(); // see AP_tree_edge.cxx@flags_broken_by_moveNextTo
                progress.subtitle("local optimize (repeated NNI)");
                while (1) {
                    rootEdge()->nni_rec(EdgeSpec(SKIP_UNMARKED_EDGES|SKIP_LEAF_EDGES), AP_BL_NNI_ONLY, NULL, true);
                    AP_FLOAT pars_curr = rootNode()->costs();
                    if (pars_curr == pars_prev) break;
                    ap_assert(pars_curr<pars_prev);
                    pars_prev          = pars_curr;
                }
            }

            rootEdge()->calc_branchlengths();

            ASSERT_VALID_TREE(rootNode());
            rootNode()->compute_tree();
        }
        else {
            error = "Tree lost (no leafs left)";
        }

        insertPerf.dump(stdout);
    }

    if (hash) GBS_free_hash(hash);
    if (error) aw_message(error);

    // @@@ quick-add w/o NNI should sort according to original tree
    agt->reorder_tree(BIG_BRANCHES_TO_TOP);
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

    void dump(const char *whichMatch) const {
        ap_assert(best_full_match);
        printf("%s match for '%s' is '%s' (overlap=%li penalty=%li)\n",
               whichMatch, get_name(), best_full_match->name,
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
            if (!multi_match) dump("better");
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

#if defined(DEBUG)
        dump("better");
#endif
    }
#if defined(DEBUG)
    else if (!multi_match) {
        printf("Worse match against '%s' (overlap=%li penalty=%li)\n", leaf_full->name, curr_overlap, curr_penalty);
    }
#endif
}

static GB_ERROR nt_best_partial_match_rec(list<PartialSequence>& partial, const AP_tree_nlen *tree) {
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
            error             = nt_best_partial_match_rec(partial, tree->get_leftson());
            if (!error) error = nt_best_partial_match_rec(partial, tree->get_rightson());
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

// -------------------------------
//      Add Partial sequences

static void nt_add_partial(AWT_graphic_parsimony *agt) {
    GB_ERROR  error   = NULL;
    GBDATA   *gb_main = agt->gb_main;

    GB_begin_transaction(gb_main);

    int full_marked_sequences = 0;

    arb_progress part_add_progress("Adding partial sequences");

    {
        list<PartialSequence> partial;
        {
            GB_HASH *partial_hash = GBS_create_hash(GBT_get_species_count(gb_main), GB_MIND_CASE);

            int marked_found             = 0;
            int partial_marked_sequences = 0;
            int no_data                  = 0;      // no data in alignment

            for (GBDATA *gb_marked = GBT_first_marked_species(gb_main);
                 !error && gb_marked;
                 gb_marked = GBT_next_marked_species(gb_marked))
            {
                ++marked_found;

                if (GBT_find_sequence(gb_marked, ap_main->get_aliname())) { // species has sequence in alignment
                    const char *name = GBT_read_name(gb_marked);

                    switch (GBT_is_partial(gb_marked, 1, true)) { // marks undef as 'partial sequence'
                        case 0: { // full sequences
                            GBT_message(gb_main, GBS_global_string("'%s' is a full sequence (cannot add partial)", name));
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

                if (no_data>0)                  GBT_message(gb_main, GBS_global_string("%i marked species have no data in '%s'",        no_data, ap_main->get_aliname()));
                if (full_marked_sequences>0)    GBT_message(gb_main, GBS_global_string("%i marked species are declared full sequences", full_marked_sequences));
                if (partials_already_in_tree>0) GBT_message(gb_main, GBS_global_string("%i marked species are already in tree",         partials_already_in_tree));

                if (partial.empty()) error = "No species left to add";
            }

            GBS_free_hash(partial_hash);
        }

        if (!error) error = GBT_add_new_changekey(gb_main, "ARB_partial", GB_INT);

        if (!error) {
            rootNode()->reset_subtree_layout();

            // find best matching full sequence for each partial sequence
            error = nt_best_partial_match_rec(partial, rootNode());

            list<PartialSequence>::iterator i = partial.begin();
            list<PartialSequence>::iterator e = partial.end();

            arb_progress part_insert_progress(partial.size());

#if defined(DEBUG)
            // show results :
            for (; i != e; ++i) i->dump("best");
            i = partial.begin();
#endif // DEBUG

            for (; i != e && !error; ++i) {
                const char *name = i->get_name();

                if (i->is_multi_match()) {
                    GBT_message(gb_main, GBS_global_string("Insertion of '%s' is ambiguous.\n"
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
                    destroy(part_leaf);
                }

                part_insert_progress.inc_and_check_user_abort(error);
            }
        }
    }

    if (full_marked_sequences) {
        GBT_message(gb_main, GBS_global_string("%i marked full sequences were not added", full_marked_sequences));
    }

    if (error) {
        GBT_message(gb_main, error);
        GB_abort_transaction(gb_main);
    }
    else {
        GB_commit_transaction(gb_main);
    }
}

static void NT_add_partial_and_update(UNFIXED, AWT_canvas *ntw) {
    nt_add_partial(AWT_TREE_PARS(ntw));
    pars_saveNrefresh_changed_tree(ntw);
}

// -------------------------------
//      add marked / selected

static void nt_add_and_update(AWT_canvas *ntw, AddWhat what, bool quick) {
    nt_add(AWT_TREE_PARS(ntw), what, quick);
    pars_saveNrefresh_changed_tree(ntw);
}

static void NT_add_and_NNI(UNFIXED, AWT_canvas *ntw, AddWhat what) { nt_add_and_update(ntw, what, false); }
static void NT_add_quick  (UNFIXED, AWT_canvas *ntw, AddWhat what) { nt_add_and_update(ntw, what, true);  }

// ------------------------------------------
//      remove and add marked / selected

static void nt_reAdd(AWT_graphic_parsimony *agt, AddWhat what, bool quick) {
    if (agt->get_root_node()) {
        ap_assert(what == NT_ADD_MARKED); // code below will misbehave for NT_ADD_SELECTED
        agt->get_tree_root()->remove_leafs(AWT_REMOVE_MARKED);
        nt_add(agt, what, quick);
    }
}

static void nt_reAdd_and_update(AWT_canvas *ntw, AddWhat what, bool quick) {
    nt_reAdd(AWT_TREE_PARS(ntw), what, quick);
    pars_saveNrefresh_changed_tree(ntw);
}

static void NT_reAdd_and_NNI(UNFIXED, AWT_canvas *ntw, AddWhat what) { nt_reAdd_and_update(ntw, what, false); }
static void NT_reAdd_quick  (UNFIXED, AWT_canvas *ntw, AddWhat what) { nt_reAdd_and_update(ntw, what, true);  }

// --------------------------------------------------------------------------------

static void calc_branchlengths(AWT_graphic_parsimony *agt) {
    arb_progress progress("Calculating branchlengths");
    rootEdge()->calc_branchlengths();
    agt->reorder_tree(BIG_BRANCHES_TO_TOP);
}

static void NT_calc_branch_lengths(AW_window *, AWT_canvas *ntw) {
    calc_branchlengths(AWT_TREE_PARS(ntw));
    pars_saveNrefresh_changed_tree(ntw);
}

static void NT_bootstrap(AW_window *, AWT_canvas *ntw, bool limit_only) {
    arb_progress progress("Calculating bootstrap limit");
    AP_BL_MODE   mode = AP_BL_MODE((limit_only ? AP_BL_BOOTSTRAP_LIMIT : AP_BL_BOOTSTRAP_ESTIMATE)|AP_BL_BL_ONLY);

    rootEdge()->nni_rec(ANY_EDGE, mode, NULL, true);
    AWT_graphic_tree *agt = AWT_TREE(ntw);
    agt->reorder_tree(BIG_BRANCHES_TO_TOP);
    agt->set_logical_root_to(agt->get_root_node());
    pars_saveNrefresh_changed_tree(ntw);
}

static void optimizeTree(AWT_graphic_parsimony *agt, const KL_Settings& settings) {
    arb_progress progress("Optimizing tree");
    agt->get_parsimony().optimize_tree(rootNode(), settings, progress);
    ASSERT_VALID_TREE(rootNode());
    rootEdge()->calc_branchlengths();
    agt->reorder_tree(BIG_BRANCHES_TO_TOP);
    rootNode()->compute_tree();
}
static void NT_optimize(AW_window *, AWT_canvas *ntw) {
    optimizeTree(AWT_TREE_PARS(ntw), KL_Settings(ntw->awr));
    pars_saveNrefresh_changed_tree(ntw);
}

static void recursiveNNI(AWT_graphic_parsimony *agt, EdgeSpec whichEdges) {
    arb_progress progress("Recursive NNI");
    AP_FLOAT     orgPars  = rootNode()->costs();
    AP_FLOAT     prevPars = orgPars;
    progress.subtitle(GBS_global_string("best=%.1f", orgPars));

    while (!progress.aborted()) {
        AP_FLOAT currPars = rootEdge()->nni_rec(whichEdges, AP_BL_NNI_ONLY, NULL, true);
        if (currPars == prevPars) break; // no improvement -> abort
        progress.subtitle(GBS_global_string("best=%.1f (gain=%.1f)", currPars, orgPars-currPars));
        prevPars          = currPars;
    }
    rootEdge()->calc_branchlengths();
    agt->reorder_tree(BIG_BRANCHES_TO_TOP);
    rootNode()->compute_tree();
}

static void NT_recursiveNNI(AW_window *, AWT_canvas *ntw) {
    EdgeSpec whichEdges = KL_Settings(ntw->awr).whichEdges;
    recursiveNNI(AWT_TREE_PARS(ntw), whichEdges);
    pars_saveNrefresh_changed_tree(ntw);
}

static int calculate_default_random_repeat(long leafs) {
    double balanced_depth = log10(leafs) / log10(2);
    int    repeat         = int(balanced_depth*2.0 + .5);
    if (repeat<1) repeat  = 1;
    return repeat;
}

static void update_random_repeat(AW_root *awr, AWT_graphic_parsimony *agt) {
    long leafs  = agt->get_root_node()->count_leafs();
    int  repeat = calculate_default_random_repeat(leafs);
    awr->awar(AWAR_RAND_REPEAT)->write_int(repeat);
}

static void randomMixTree(AW_window *aww, AWT_canvas *ntw) {
    arb_progress progress("Randomizing tree");

    progress.subtitle("mixing");

    AW_root *awr = aww->get_root();
    rootEdge()->mixTree(awr->awar(AWAR_RAND_REPEAT)->read_int(), awr->awar(AWAR_RAND_PERCENT)->read_int(), KL_Settings(awr).whichEdges);

    progress.subtitle("calculating branchlengths");
    rootEdge()->calc_branchlengths();
    {
        ARB_edge newRootEdge = rootNode()->get_tree_root()->find_innermost_edge();
        newRootEdge.son()->set_root();
    }
    AWT_TREE_PARS(ntw)->reorder_tree(BIG_BRANCHES_TO_TOP);
    rootNode()->compute_tree();
    pars_saveNrefresh_changed_tree(ntw);
}


static AWT_config_mapping_def optimizer_config_mapping[] = {
    { AWAR_OPTI_MARKED_ONLY, "marked_only" },
    { AWAR_OPTI_SKIP_FOLDED, "skip_folded" },

    // { AWAR_RAND_REPEAT,     "rand_repeat" }, // do not store (use treesize-dependent default)
    { AWAR_RAND_PERCENT, "rand_percent" },

    { AWAR_KL_MAXDEPTH, "maxdepth" },
    { AWAR_KL_INCDEPTH, "incdepth" },

    { AWAR_KL_STATIC_ENABLED, "static" },
    { AWAR_KL_STATIC_DEPTH1,  "s_depth1" },
    { AWAR_KL_STATIC_DEPTH2,  "s_depth2" },
    { AWAR_KL_STATIC_DEPTH3,  "s_depth3" },
    { AWAR_KL_STATIC_DEPTH4,  "s_depth4" },
    { AWAR_KL_STATIC_DEPTH5,  "s_depth5" },

    { AWAR_KL_DYNAMIC_ENABLED, "dynamic" },
    { AWAR_KL_DYNAMIC_START,   "start" },
    { AWAR_KL_DYNAMIC_MAXX,    "maxx" },
    { AWAR_KL_DYNAMIC_MAXY,    "maxy" },

    { 0, 0 }
};

static AWT_predefined_config optimizer_predefined_configs[] = {
    {
        "*minimum_static_reduction",
        "Sets paths allowed by static reduction to maximum\n(causing the minimal reduction)",
        "s_depth1='8';s_depth2='6';s_depth3='6';s_depth4='6';s_depth5='6';static='1'" // only defines/affects settings related to static path reduction
    },
    {
        "*whole_tree_level8",
        "Level-8-optimization of whole tree\n(no path reduction)",
        "dynamic='0';incdepth='0';marked_only='0';maxdepth='8';skip_folded='0';static='0'"
    },
    { 0, 0, 0 }
};

static AW_window *createOptimizeWindow(AW_root *aw_root, AWT_canvas *ntw) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "TREE_OPTIMIZE", "Tree optimization");
    aws->load_xfig("pars/tree_opti.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("pa_optimizer.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("marked");
    aws->label("Only subtrees containing marked species");
    aws->create_toggle(AWAR_OPTI_MARKED_ONLY);

    aws->at("folded");
    aws->label("Do not modify folded subtrees");
    aws->create_toggle(AWAR_OPTI_SKIP_FOLDED);

    aws->button_length(18);

    aws->at("rec_nni");
    aws->callback(makeWindowCallback(NT_recursiveNNI, ntw));
    aws->create_button("REC_NNI", "Recursive NNI", "N");

    aws->at("heuristic");
    aws->callback(makeWindowCallback(NT_optimize, ntw));
    aws->create_button("HEURISTIC", "Heuristic\noptimizer", "H");

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "treeopti", optimizer_config_mapping, NULL, optimizer_predefined_configs);

    aws->at("settings");
    aws->callback(makeCreateWindowCallback(create_kernighan_properties_window));
    aws->create_button("SETTINGS", "Settings", "S");

    aws->at("randomize");
    aws->callback(makeWindowCallback(randomMixTree, ntw));
    aws->create_button("RANDOMIZE", "Randomize tree", "R");

    aws->button_length(5);

    aws->at("repeat");   aws->create_input_field(AWAR_RAND_REPEAT);
    aws->at("percent");  aws->create_input_field(AWAR_RAND_PERCENT);

    return aws;
}

// -----------------------
//      test functions

#if defined(TESTMENU)
static void refreshTree(AWT_canvas *ntw) {
    GB_transaction ta(ntw->gb_main);

    AWT_TREE(ntw)->check_update(ntw->gb_main);
    GB_ERROR error = AWT_TREE(ntw)->save(ntw->gb_main, NULL);
    if (error) aw_message(error);
    ntw->zoom_reset_and_refresh();
}

static void setBranchlens(AP_tree_nlen *node, double newLen)
{
    node->setBranchlen(newLen, newLen);

    if (!node->is_leaf)
    {
        setBranchlens(node->get_leftson(), newLen);
        setBranchlens(node->get_rightson(), newLen);
    }
}

static void TESTMENU_setBranchlen(AW_window *, AWT_canvas *ntw)
{
    AP_tree_nlen *root = rootNode();

    setBranchlens(root, 1.0);
    refreshTree(ntw);
}

static void TESTMENU_treeStats(AW_window *) {
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

static void TESTMENU_sortTreeByName(AW_window *, AWT_canvas *ntw)
{
    AP_tree_nlen *root = rootNode();

    root->sortByName();
    refreshTree(ntw);
}

static void init_TEST_menu(AW_window_menu_modes *awm, AWT_canvas *ntw)
{
    awm->create_menu("Test[debug]", "g", AWM_ALL);

    awm->insert_menu_topic("treestat",        "Tree statistics",    "s", "", AWM_ALL, TESTMENU_treeStats);
    awm->insert_menu_topic("setlens",         "Set branchlens",     "b", "", AWM_ALL, makeWindowCallback(TESTMENU_setBranchlen, ntw));
    awm->insert_menu_topic("sorttreebyname",  "Sort tree by name",  "o", "", AWM_ALL, makeWindowCallback(TESTMENU_sortTreeByName, ntw));
}
#endif // TESTMENU

static GB_ERROR pars_check_size(AW_root *awr, GB_ERROR& warning, const adfiltercbstruct *filterDef) {
    GB_ERROR error = NULL;
    warning        = NULL;

    char *tree_name = awr->awar(AWAR_TREE)->read_string();
    char *filter    = awr->awar(filterDef->def_filter)->read_string();
    long  ali_len   = 0;

    if (strlen(filter)) {
        int i;
        for (i=0; filter[i]; i++) {
            if (filter[i] != '0') ali_len++;
        }
    }
    else {
        char *ali_name = awr->awar(AWAR_ALIGNMENT)->read_string();
        ali_len        = GBT_get_alignment_len(ap_main->get_gb_main(), ali_name);
        if (ali_len<=0) {
            error = "Please select a valid alignment";
            GB_clear_error();
        }
        free(ali_name);
    }

    if (!error) {
        long tree_size = GBT_size_of_tree(ap_main->get_gb_main(), tree_name);
        if (tree_size == -1) {
            error = "Please select an existing tree";
        }
        else {
            size_t expected_memuse = (ali_len * tree_size * 4 / 1024);
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

static void pars_reset_optimal_parsimony(AW_window *aww, AWT_canvas *ntw) {
    AW_root *awr = aww->get_root();
    awr->awar(AWAR_BEST_PARSIMONY)->write_int(awr->awar(AWAR_PARSIMONY)->read_int());
    ntw->refresh();
}

class LowDataCheck {
    int leafs; // counts leafs with insufficiant data
    int inner; // same for inner nodes

public:
    LowDataCheck() : leafs(0), inner(0) {}

    void count(AP_tree_nlen *node);

    int get_leafs() const { return leafs; }
    int get_inner() const { return inner; }
};

void LowDataCheck::count(AP_tree_nlen *node) {
    const AP_sequence *seq   = node->get_seq();
    AP_FLOAT           bases = seq->weighted_base_count();

    if (node->is_leaf) {
        if (bases<MIN_SEQUENCE_LENGTH) ++leafs;
    }
    else {
        if (bases<MIN_SEQUENCE_LENGTH) ++inner;

        count(node->get_leftson());
        count(node->get_rightson());
    }
}

static void PARS_infomode_cb(UNFIXED, AWT_canvas *canvas, AWT_COMMAND_MODE mode) {
    AWT_trigger_remote_action(NULL, canvas->gb_main, "ARB_NT:species_info");
    nt_mode_event(NULL, canvas, mode);
}

static void pars_start_cb(AW_window *aw_parent, WeightedFilter *wfilt, const PARS_commands *cmds) {
    AW_root *awr     = aw_parent->get_root();
    GBDATA  *gb_main = ap_main->get_gb_main();
    GB_begin_transaction(gb_main);
    {
        GB_ERROR warning;
        GB_ERROR error = pars_check_size(awr, warning, wfilt->get_adfiltercbstruct());

        if (warning && !error) {
            char *question = GBS_global_string_copy("%s\nDo you want to continue?", warning);
            bool  cont     = aw_ask_sure("swap_warning", question);
            free(question);

            if (!cont) error = "User abort";

        }

        if (error) {
            aw_message(error);
            GB_commit_transaction(gb_main);
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
        ntw = new AWT_canvas(gb_main, awm, awm->get_window_id(), global_tree(), AWAR_TREE);
        global_tree()->set_tree_type(old_sort_type, ntw);
    }

    {
        GB_ERROR error = 0;
        arb_progress progress("loading tree");
        NT_reload_tree_event(awr, ntw, false);             // load tree (but do not expose - first zombies need to be removed)
        if (!global_tree()->get_root_node()) {
            error = "Failed to load the selected tree";
        }
        else {
            AP_tree_edge::initialize(rootNode());   // builds edges
            long removed = global_tree_root()->remove_leafs(AWT_REMOVE_ZOMBIES);

            PARS_tree_init(global_tree());
            removed += global_tree_root()->remove_leafs(AWT_RemoveType(AWT_REMOVE_ZOMBIES | AWT_REMOVE_NO_SEQUENCE));

            if (!global_tree()->get_root_node()) {
                const char *aliname = global_tree_root()->get_aliview()->get_aliname();
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

                progress.subtitle("Checking amount of data");
                LowDataCheck lowData;
                lowData.count(GLOBAL_PARS->get_root_node());

                bool warned = false;
                if (lowData.get_inner()>0) {
                    aw_message(GBS_global_string("Inner nodes with insufficient data: %i", lowData.get_leafs()));
                    warned = true;
                }
                if (lowData.get_leafs()>0) {
                    aw_message(GBS_global_string("Species with insufficient data: %i", lowData.get_leafs()));
                    warned = true;
                }
                if (warned) {
                    aw_message("Warning: low sequence data detected! (or filter too restrictive)");
                }
            }
        }
        if (error) aw_popup_exit(error);
    }

    if (cmds->add_marked)           NT_add_quick(NULL, ntw, NT_ADD_MARKED);
    if (cmds->add_selected)         NT_add_quick(NULL, ntw, NT_ADD_SELECTED);
    if (cmds->calc_branch_lengths)  NT_calc_branch_lengths(awm, ntw);
    if (cmds->calc_bootstrap)       NT_bootstrap(awm, ntw, 0);
    if (cmds->quit)                 pars_exit(awm);

    GB_transaction ta(ntw->gb_main);

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
        awm->insert_menu_topic("tree_print",  "Print tree ...",          "P", "tree2prt.hlp",  AWM_ALL, makeWindowCallback(AWT_popup_print_window,       ntw));
        awm->insert_menu_topic("tree_2_xfig", "Export tree to XFIG ...", "F", "tree2file.hlp", AWM_ALL, makeWindowCallback(AWT_popup_tree_export_window, ntw));
        awm->sep______________();
        NT_insert_collapse_submenu(awm, ntw);
        awm->sep______________();
        awm->insert_sub_menu("Remove Species from Tree",     "R");
        {
            awm->insert_menu_topic("tree_remove_deleted", "Remove Zombies", "Z", "trm_del.hlp",    AWM_ALL, makeWindowCallback(NT_remove_leafs, ntw, AWT_REMOVE_ZOMBIES));
            awm->insert_menu_topic("tree_remove_marked",  "Remove Marked",  "M", "trm_mrkd.hlp",   AWM_ALL, makeWindowCallback(NT_remove_leafs, ntw, AWT_REMOVE_MARKED));
            awm->insert_menu_topic("tree_keep_marked",    "Keep Marked",    "K", "tkeep_mrkd.hlp", AWM_ALL, makeWindowCallback(NT_remove_leafs, ntw, AWT_KEEP_MARKED));
        }
        awm->close_sub_menu();
        awm->insert_sub_menu("Add Species to Tree",      "A");
        {
            awm->insert_menu_topic("add_marked",         "Add Marked Species",                              "M", "pa_quick.hlp",   AWM_ALL, makeWindowCallback(NT_add_quick,     ntw, NT_ADD_MARKED));
            awm->insert_menu_topic("add_marked_nni",     "Add Marked Species + Local Optimization (NNI)",   "N", "pa_add.hlp",     AWM_ALL, makeWindowCallback(NT_add_and_NNI,   ntw, NT_ADD_MARKED));
            awm->insert_menu_topic("rm_add_marked",      "Remove & Add Marked Species",                     "R", "pa_quick.hlp",   AWM_ALL, makeWindowCallback(NT_reAdd_quick,   ntw, NT_ADD_MARKED));
            awm->insert_menu_topic("rm_add_marked_nni|", "Remove & Add Marked + Local Optimization (NNI)",  "L", "pa_add.hlp",     AWM_ALL, makeWindowCallback(NT_reAdd_and_NNI, ntw, NT_ADD_MARKED));
            awm->sep______________();
            awm->insert_menu_topic("add_marked_partial", "Add Marked Partial Species",                      "P", "pa_partial.hlp", AWM_ALL, makeWindowCallback(NT_add_partial_and_update, ntw));
            awm->sep______________();
            awm->insert_menu_topic("add_selected",       "Add Selected Species",                            "S", "pa_quick.hlp",   AWM_ALL, makeWindowCallback(NT_add_quick,     ntw, NT_ADD_SELECTED));
            awm->insert_menu_topic("add_selected_nni",   "Add Selected Species + Local Optimization (NNI)", "O", "pa_add.hlp",     AWM_ALL, makeWindowCallback(NT_add_and_NNI,   ntw, NT_ADD_SELECTED));
        }
        awm->close_sub_menu();
        awm->sep______________();
        awm->insert_menu_topic("optimize", "Tree Optimization ...",   "O", "pa_optimizer.hlp", AWM_ALL, makeCreateWindowCallback(createOptimizeWindow,         ntw));
        awm->insert_menu_topic("reset",    "Reset optimal parsimony", "s", "pa_reset.hlp",     AWM_ALL, makeWindowCallback      (pars_reset_optimal_parsimony, ntw));
        awm->sep______________();
        awm->insert_menu_topic("beautify_tree",       "Beautify Tree",            "B", "resorttree.hlp",       AWM_ALL, makeWindowCallback(NT_resort_tree_cb, ntw, BIG_BRANCHES_TO_TOP));
        awm->insert_menu_topic("calc_branch_lengths", "Calculate Branch Lengths", "L", "pa_branchlengths.hlp", AWM_ALL, makeWindowCallback(NT_calc_branch_lengths, ntw));
        awm->sep______________();
        awm->insert_menu_topic("calc_upper_bootstrap_indep", "Calculate Upper Bootstrap Limit (dependent NNI)",   "U", "pa_bootstrap.hlp", AWM_ALL, makeWindowCallback(NT_bootstrap,        ntw, false));
        awm->insert_menu_topic("calc_upper_bootstrap_dep",   "Calculate Upper Bootstrap Limit (independent NNI)", "i", "pa_bootstrap.hlp", AWM_ALL, makeWindowCallback(NT_bootstrap,        ntw, true));
        awm->insert_menu_topic("tree_remove_remark",         "Remove bootstrap values",                           "v", "trm_boot.hlp",     AWM_ALL, makeWindowCallback(NT_remove_bootstrap, ntw));
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
#if defined(ARB_MOTIF)
        awm->insert_menu_topic("props_menu",  "Frame settings ...",      "F", "props_frame.hlp",      AWM_ALL, AW_preset_window);
#endif
        awm->insert_menu_topic("props_tree2", "Tree options",            "o", "nt_tree_settings.hlp", AWM_ALL, TREE_create_settings_window);
        awm->insert_menu_topic("props_tree",  "Tree colors & fonts",     "c", "color_props.hlp",      AWM_ALL, makeCreateWindowCallback(AW_create_gc_window, ntw->gc_manager));
        awm->insert_menu_topic("props_kl",    "Optimizer settings (KL)", "K", "kernlin.hlp",          AWM_ALL, makeCreateWindowCallback(create_kernighan_properties_window));
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

    awm->create_mode("mode_info.xpm",   "mode_info.hlp",   AWM_ALL, makeWindowCallback(PARS_infomode_cb, ntw, AWT_MODE_INFO));
    // reserve mode-locations (to put the modes below at the same position as in ARB_NT)
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
    awm->callback(makeWindowCallback(AP_user_pop_cb, ntw));
    awm->help_text("ap_stack.hlp");
    awm->create_button("POP", "RESTORE", 0);

    awm->button_length(6);
    awm->callback(makeWindowCallback(AP_user_push_cb, ntw));
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
        SmartPtr<AW_at_storage> maxSize(AW_at_storage::make(awm, AW_AT_MAXSIZE));

        awm->button_length(AWAR_FOOTER_MAX_LEN);
        awm->create_button(0, AWAR_FOOTER);
        awm->at_newline();
        awm->restore_at_from(*maxSize);
    }

    awm->get_at_position(&db_treex, &db_treey);
    awm->set_info_area_height(db_treey);

    awm->set_bottom_area_height(0);

    aw_parent->hide(); // hide parent
    awm->show();

    TREE_install_update_callbacks(ntw);

    update_random_repeat(awr, AWT_TREE_PARS(ntw));
    AP_user_push_cb(aw_parent, ntw); // push initial tree
    set_keep_ghostnodes(); // make sure no stacked nodes get deleted
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
#if defined(ARB_GTK)
    aws->set_close_action("ABORT");
#endif

    aws->callback(makeHelpCallback("arb_pars_init.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    GBDATA *gb_main                 = ap_main->get_gb_main();
    WeightedFilter *weighted_filter = // do NOT free (bound to callbacks)
        new WeightedFilter(gb_main, aws->get_root(), AWAR_FILTER_NAME, AWAR_COLUMNSTAT_NAME, aws->get_root()->awar_string(AWAR_ALIGNMENT));

    aws->at("filter");
    aws->callback(makeCreateWindowCallback(awt_create_select_filter_win, weighted_filter->get_adfiltercbstruct()));
    aws->create_button("SELECT_FILTER", AWAR_FILTER_NAME);

    aws->at("weights");
    aws->callback(makeCreateWindowCallback(COLSTAT_create_selection_window, weighted_filter->get_column_stat()));
    aws->sens_mask(AWM_EXP);
    aws->create_button("SELECT_CSP", AWAR_COLUMNSTAT_NAME);
    aws->sens_mask(AWM_ALL);

    aws->at("alignment");
    awt_create_ALI_selection_list(gb_main, aws, AWAR_ALIGNMENT, "*=");

    aws->at("tree");
    awt_create_TREE_selection_list(gb_main, aws, AWAR_TREE, true);

    aws->callback(makeWindowCallback(pars_start_cb, weighted_filter, cmds));
    aws->at("go");
    aws->create_button("GO", "GO", "G");

    return aws;
}

KL_Settings::KL_Settings(AW_root *aw_root) {
    maxdepth = aw_root->awar(AWAR_KL_MAXDEPTH)->read_int();
    incdepth = aw_root->awar(AWAR_KL_INCDEPTH)->read_int();

    Static.enabled  = aw_root->awar(AWAR_KL_STATIC_ENABLED)->read_int();
    Static.depth[0] = 2; // always test both possibilities at starting edge
    Static.depth[1] = aw_root->awar(AWAR_KL_STATIC_DEPTH1)->read_int();
    Static.depth[2] = aw_root->awar(AWAR_KL_STATIC_DEPTH2)->read_int();
    Static.depth[3] = aw_root->awar(AWAR_KL_STATIC_DEPTH3)->read_int();
    Static.depth[4] = aw_root->awar(AWAR_KL_STATIC_DEPTH4)->read_int();
    Static.depth[5] = aw_root->awar(AWAR_KL_STATIC_DEPTH5)->read_int();

    Dynamic.enabled = aw_root->awar(AWAR_KL_DYNAMIC_ENABLED)->read_int();
    Dynamic.start   = aw_root->awar(AWAR_KL_DYNAMIC_START)->read_int();
    Dynamic.maxx    = aw_root->awar(AWAR_KL_DYNAMIC_MAXX)->read_int();
    Dynamic.maxy    = aw_root->awar(AWAR_KL_DYNAMIC_MAXY)->read_int();
    Dynamic.type    = (KL_DYNAMIC_THRESHOLD_TYPE)aw_root->awar(AWAR_KL_FUNCTION_TYPE)->read_int();

    whichEdges = ANY_EDGE;
    if (aw_root->awar(AWAR_OPTI_MARKED_ONLY)->read_int()) whichEdges = EdgeSpec(whichEdges|SKIP_UNMARKED_EDGES);
    if (aw_root->awar(AWAR_OPTI_SKIP_FOLDED)->read_int()) whichEdges = EdgeSpec(whichEdges|SKIP_FOLDED_EDGES);
}
#if defined(UNIT_TESTS)
KL_Settings::KL_Settings() {
    // set default values
    maxdepth = 15;

    Static.enabled  = true;
    Static.depth[0] = 2; // always test both possibilities at starting edge
    Static.depth[1] = 8;
    Static.depth[2] = 6;
    Static.depth[3] = 6;
    Static.depth[4] = 6;
    Static.depth[5] = 6;

    Dynamic.enabled = true;
    Dynamic.start   = 100;
    Dynamic.maxy    = 150;
    Dynamic.maxx    = 6;

    // these values do not seem to have any effect (i.e. are not covered by unit-tests):
    incdepth = 4;

    // const setting (not configurable)
    Dynamic.type = AP_QUADRAT_START;
    whichEdges   = EdgeSpec(SKIP_UNMARKED_EDGES|SKIP_FOLDED_EDGES);
}
#endif

static void create_optimize_vars(AW_root *aw_root, AW_default props) {
    // kernighan

    aw_root->awar_int(AWAR_OPTI_MARKED_ONLY, 1, props);
    aw_root->awar_int(AWAR_OPTI_SKIP_FOLDED, 1, props);

    aw_root->awar_int(AWAR_KL_MAXDEPTH, 15, props);
    aw_root->awar_int(AWAR_KL_INCDEPTH, 4,  props);

    aw_root->awar_int(AWAR_KL_STATIC_ENABLED, 1, props);
    aw_root->awar_int(AWAR_KL_STATIC_DEPTH1,  5, props)->set_minmax(1, 8);
    aw_root->awar_int(AWAR_KL_STATIC_DEPTH2,  3, props)->set_minmax(1, 6);
    aw_root->awar_int(AWAR_KL_STATIC_DEPTH3,  2, props)->set_minmax(1, 6);
    aw_root->awar_int(AWAR_KL_STATIC_DEPTH4,  2, props)->set_minmax(1, 6);
    aw_root->awar_int(AWAR_KL_STATIC_DEPTH5,  1, props)->set_minmax(1, 6);

    aw_root->awar_int(AWAR_KL_DYNAMIC_ENABLED, 1,   props);
    aw_root->awar_int(AWAR_KL_DYNAMIC_START,   100, props);
    aw_root->awar_int(AWAR_KL_DYNAMIC_MAXX,    6,   props);
    aw_root->awar_int(AWAR_KL_DYNAMIC_MAXY,    150, props);

    aw_root->awar_int(AWAR_KL_FUNCTION_TYPE, AP_QUADRAT_START, props);
}

static void pars_create_all_awars(AW_root *awr, AW_default aw_def, GBDATA *gb_main) {
    awr->awar_string(AWAR_SPECIES_NAME, "",     gb_main);
    awr->awar_string(AWAR_FOOTER,       "",     aw_def);

    // copy currently selected alignment to awar:
    {
        GB_transaction  ta(gb_main);
        char           *dali = GBT_get_default_alignment(gb_main);
        awr->awar_string(AWAR_ALIGNMENT, dali, gb_main)->write_string(dali);
        free(dali);
    }

    awt_create_filter_awars(awr, aw_def, AWAR_FILTER_NAME, AWAR_ALIGNMENT);
    awt_set_awar_to_valid_filter_good_for_tree_methods(gb_main, awr, AWAR_FILTER_NAME);

    awr->awar_int(AWAR_PARS_TYPE, PARS_WAGNER, gb_main);

    {
        GB_transaction  ta(gb_main);
        GBDATA         *gb_tree_name = GB_search(gb_main, AWAR_TREE, GB_STRING);
        char           *tree_name    = GB_read_string(gb_tree_name);

        awr->awar_string(AWAR_TREE, "", aw_def)->write_string(tree_name);
        free(tree_name);
    }

    awr->awar_int(AWAR_PARSIMONY,      0, aw_def);
    awr->awar_int(AWAR_BEST_PARSIMONY, 0, aw_def);
    awr->awar_int(AWAR_STACKPOINTER,   0, aw_def);

    awr->awar_int(AWAR_RAND_REPEAT,  1,  aw_def)->set_minmax(1, 1000000); // default value is overwritten by update_random_repeat()
    awr->awar_int(AWAR_RAND_PERCENT, 50, aw_def)->set_minmax(1, 100);

    create_optimize_vars(awr, aw_def);
    create_nds_vars(awr, aw_def, gb_main, false);

    TREE_create_awars(awr, gb_main);

#if defined(DEBUG)
    AWT_create_db_browser_awars(awr, aw_def);
#endif // DEBUG

    GB_ERROR error = ARB_init_global_awars(awr, aw_def, gb_main);
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
    GLOBAL_PARS = new ArbParsimony();

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

    GB_ERROR error = ap_main->open(db_server);
    if (!error) {
        GBDATA *gb_main = ap_main->get_gb_main();
        error           = configure_macro_recording(aw_root, "ARB_PARS", gb_main);

        if (!error) {
#if defined(DEBUG)
            AWT_announce_db_to_browser(gb_main, GBS_global_string("ARB-database (%s)", db_server));
#endif // DEBUG

            pars_create_all_awars(aw_root, AW_ROOT_DEFAULT, gb_main);

            AW_window *aww = create_pars_init_window(aw_root, &cmds);
            aww->show();

            AWT_install_cb_guards();
            aw_root->main_loop();
        }
    }

    if (error) aw_popup_exit(error);
    return EXIT_SUCCESS;
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <arb_file.h>
#include <arb_diff.h>
#include <test_unit.h>
#include <AP_seq_dna.hxx>
#include <AP_seq_protein.hxx>
#include "test_env.h"

// #define AUTO_UPDATE_IF_CHANGED // uncomment to auto update expected results

arb_test::match_expectation topologyEquals(AP_tree_nlen *root_node, const char *treefile_base) {
    using namespace   arb_test;
    expectation_group fulfilled;

    char *outfile  = GBS_global_string_copy("pars/%s.tree", treefile_base);
    char *expected = GBS_global_string_copy("%s.expected", outfile);
    bool  update   = false;

    {
        FILE *out    = fopen(outfile, "wt");
        fulfilled.add(that(out).does_differ_from_NULL());
        char *newick = GBT_tree_2_newick(root_node, NewickFormat(nLENGTH|nWRAP), false);
        fputs(newick, out);
        free(newick);
        fclose(out);
    }

    if (GB_is_regularfile(expected)) {
        bool match_exp_topo = textfiles_have_difflines(outfile,expected,0);
#if defined(AUTO_UPDATE_IF_CHANGED)
        if (!match_exp_topo) update = true;
#endif
        if (!update) fulfilled.add(that(match_exp_topo).is_equal_to(true));
    }
    else {
        update = true;
    }

    if (update) TEST_COPY_FILE(outfile, expected);
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(outfile));

    free(expected);
    free(outfile);

    return all().ofgroup(fulfilled);
}
#define TEST_EXPECT_SAVED_TOPOLOGY(env,exp_topo) TEST_EXPECTATION(topologyEquals(env.root_node(), exp_topo))
#define TEST_EXPECT_SAVED_TOPOLOGY__BROKEN(env,exp_topo,got_topo) TEST_EXPECTATION__BROKEN(topologyEquals(env.root_node(), exp_topo), topologyEquals(env.root_node(), got_topo))

#define TEST_EXPECT_PARSVAL(env,exp_pars)  TEST_EXPECT_SIMILAR(env.root_node()->costs(), exp_pars, 0.001);

enum TopoMod {
    MOD_REMOVE_MARKED,

    MOD_QUICK_READD,
    MOD_QUICK_ADD,
    MOD_READD_NNI,

    MOD_ADD_PARTIAL,

    MOD_CALC_LENS,
    MOD_OPTI_NNI,
    MOD_OPTI_GLOBAL,

    MOD_MIX_TREE,
};

template <typename SEQ>
static void modifyTopology(PARSIMONY_testenv<SEQ>& env, TopoMod mod) {
    switch (mod) {
        case MOD_REMOVE_MARKED:
            env.graphic_tree()->get_tree_root()->remove_leafs(AWT_REMOVE_MARKED);
            break;

        case MOD_QUICK_READD:
            nt_reAdd(env.graphic_tree(), NT_ADD_MARKED, true);
            break;

        case MOD_QUICK_ADD:
            nt_add(env.graphic_tree(), NT_ADD_MARKED, true);
            break;

        case MOD_READD_NNI:
            nt_reAdd(env.graphic_tree(), NT_ADD_MARKED, false);
            break;

        case MOD_ADD_PARTIAL:
            nt_add_partial(env.graphic_tree());
            break;

        case MOD_CALC_LENS:
            calc_branchlengths(env.graphic_tree());
            break;

        case MOD_OPTI_NNI: // only marked/unfolded
            recursiveNNI(env.graphic_tree(), env.get_KL_settings().whichEdges);
            break;

        case MOD_OPTI_GLOBAL:
            optimizeTree(env.graphic_tree(), env.get_KL_settings());
            break;

        case MOD_MIX_TREE: {
            long leafs = rootNode()->count_leafs();
            rootEdge()->mixTree(calculate_default_random_repeat(leafs), 100, ANY_EDGE);
            rootEdge()->calc_branchlengths();
            break;
        }
    }
}

template <typename SEQ>
static arb_test::match_expectation modifyingTopoResultsIn(TopoMod mod, const char *topo, int pars_expected, PARSIMONY_testenv<SEQ>& env, bool restore) {
    using namespace   arb_test;
    expectation_group fulfilled;

    TEST_EXPECT_VALID_TREE(env.root_node());

    Level upc = env.get_user_push_counter();
    Level fl  = env.get_frame_level();

    if (restore) {
        env.push();
        TEST_EXPECT_VALID_TREE(env.root_node());
    }

    modifyTopology(env, mod);
    if (topo) fulfilled.add(topologyEquals(env.root_node(), topo));

    fulfilled.add(that(allBranchlengthsAreDefined(env.root_node())).is_equal_to(true));

    if (pars_expected != -1) {
        fulfilled.add(that(env.root_node()->costs()).fulfills(epsilon_similar(0.001), pars_expected));
    }

    if (restore) {
        TEST_EXPECT_VALID_TREE(env.root_node());
        TEST_VALIDITY(env.pop_will_produce_valid_tree());
        env.pop();
        bool blen_def_after_pop = allBranchlengthsAreDefined(env.root_node());
        fulfilled.add(that(blen_def_after_pop).is_equal_to(true));
    }

    TEST_EXPECT_EQUAL(fl, env.get_frame_level());
    TEST_EXPECT_EQUAL(upc, env.get_user_push_counter());

    TEST_EXPECT_VALID_TREE(env.root_node());

    return all().ofgroup(fulfilled);
}

static arb_test::match_expectation movingRootDoesntAffectCosts(int pars_expected) {
    using namespace   arb_test;
    expectation_group fulfilled;

    int pars_min = INT_MAX;
    int pars_max = INT_MIN;

    for (int depth_first = 0; depth_first<=1; ++depth_first) {
        for (int push_local = 0; push_local<=1; ++push_local) {
            EdgeChain chain(rootEdge(), ANY_EDGE, depth_first);

            if (!push_local) ap_main->remember();
            while (chain) {
                AP_tree_edge *edge = *chain; ++chain;

                if (push_local) ap_main->remember();
                edge->set_root();
                int pars = rootNode()->costs();
                pars_min = std::min(pars, pars_min);
                pars_max = std::max(pars, pars_max);
                if (push_local) ap_main->revert();
            }
            if (!push_local) ap_main->revert();
        }
    }

    fulfilled.add(that(pars_min).is_equal_to(pars_expected));
    fulfilled.add(that(pars_max).is_equal_to(pars_expected));

    return all().ofgroup(fulfilled);
}

static GBDATA *copy_to(GBDATA *gb_species, const char *newShortname) {
    GBDATA *gb_species_data = GB_get_father(gb_species);
    GBDATA *gb_new_species  = GB_create_container(gb_species_data, "species");

    GB_ERROR error = NULL;
    if (!gb_new_species) {
        error = GB_await_error();
    }
    else {
        error = GB_copy(gb_new_species, gb_species);
        if (!error) error = GBT_write_string(gb_new_species, "name", newShortname);
    }

    ap_assert(contradicted(gb_new_species, error));
    return gb_new_species;
}

inline void mark_only(GBDATA *gb_species) {
    GBDATA         *gb_main = GB_get_root(gb_species);
    GB_transaction  ta(gb_main);
    GBT_mark_all(gb_main, 0);
    GB_write_flag(gb_species, 1);
}
inline void mark(GBDATA *gb_species) {
    GBDATA         *gb_main = GB_get_root(gb_species);
    GB_transaction  ta(gb_main);
    GB_write_flag(gb_species, 1);
}
inline void mark_all(GBDATA *gb_main) {
    GB_transaction  ta(gb_main);
    GBT_mark_all(gb_main, 1);
}

inline int is_partial(GBDATA *gb_species) {
    GB_transaction ta(gb_species);
    return GBT_is_partial(gb_species, -1, false);
}

template <typename SEQ>
static arb_test::match_expectation addedAsBrotherOf(const char *name, const char *allowedBrothers, PARSIMONY_testenv<SEQ>& env) {
    using namespace   arb_test;
    expectation_group fulfilled;

    AP_tree_nlen *node_in_tree = env.root_node()->findLeafNamed(name);
    fulfilled.add(that(node_in_tree).does_differ_from_NULL());

    const char *brother = node_in_tree->get_brother()->name;
    fulfilled.add(that(allowedBrothers).does_contain(brother));

    return all().ofgroup(fulfilled);
}

template <typename SEQ>
static arb_test::match_expectation addingPartialResultsIn(GBDATA *gb_added_species, const char *allowedBrothers, const char *topo, int pars_expected, PARSIMONY_testenv<SEQ>& env) {
    using namespace   arb_test;
    expectation_group fulfilled;

    mark_only(gb_added_species);
    env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)
    fulfilled.add(modifyingTopoResultsIn(MOD_ADD_PARTIAL, topo, pars_expected, env, false));
    fulfilled.add(that(is_partial(gb_added_species)).is_equal_to(1));

    const char *name = GBT_read_name(gb_added_species);
    fulfilled.add(addedAsBrotherOf(name, allowedBrothers, env));

    return all().ofgroup(fulfilled);
}

static GBDATA *createPartialSeqFrom(GBDATA *gb_main, const char *aliname, const char *dest_species, const char *source_species, int startPos, int endPos) {
    GB_transaction ta(gb_main);

    GBDATA *gb_result         = NULL;
    GBDATA *gb_source_species = GBT_expect_species(gb_main, source_species);

    if (gb_source_species) {
        GBDATA *gb_dest_species = copy_to(gb_source_species, dest_species);
        GBDATA *gb_dest_seq     = GBT_find_sequence(gb_dest_species, aliname); // =same as source seq
        char   *seq             = GB_read_string(gb_dest_seq);

        if (seq) {
            int maxPos = strlen(seq)-1;

            startPos = std::min(startPos, maxPos);
            endPos   = std::min(endPos, maxPos);

            if (startPos>0) memset(seq, '.', startPos);
            if (endPos<maxPos) memset(seq+endPos+1, '.', maxPos-endPos);

            GB_ERROR error     = GB_write_string(gb_dest_seq, seq);
            if (error) GB_export_error(error);
            else {
                gb_result = gb_dest_species; // success
#if defined(DEBUG)
                fprintf(stderr, "created partial '%s' from '%s' (seq='%s')\n", dest_species, source_species, seq);
#endif
            }

            free(seq);
        }
    }

    return gb_result;
}

static GB_ERROR modifyOneBase(GBDATA *gb_species, const char *aliname, char cOld, char cNew) {
    GB_transaction ta(gb_species);
    GB_ERROR       error = "failed to modifyOneBase";

    GBDATA *gb_seq = GBT_find_sequence(gb_species, aliname);
    if (gb_seq) {
        char *seq = GB_read_string(gb_seq);
        if (seq) {
            char *B = strchr(seq, cOld);
            if (!B) {
                error = "does not contain base in modifyOneBase";
            }
            else {
                B[0]  = cNew;
                error = GB_write_string(gb_seq, seq);
            }
            free(seq);
        }
    }

    return error;
}

static long unmark_unwanted(const char *, long cd_gbd, void*) {
    GBDATA *gbd = (GBDATA*)cd_gbd;
    GB_write_flag(gbd, 0);
    return 0;
}

void TEST_nucl_tree_modifications() {
    const char *aliname = "ali_5s";

    PARSIMONY_testenv<AP_sequence_parsimony> env("TEST_trees.arb", aliname);
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));
    TEST_EXPECT_SAVED_TOPOLOGY(env, "nucl-initial");

    const int PARSIMONY_ORG = 301;
    TEST_EXPECT_EQUAL(env.combines_performed(), 0);
    TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);
    TEST_EXPECT_EQUAL(env.combines_performed(), 14);

    // [NUCOPTI] opposed to protein tests below the initial tree here is NOT optimized! compare .@PROTOPTI
    // -> removing and adding species produces a better tree
    //
    // diff initial->removed  : http://bugs.arb-home.de/changeset/HEAD/branches/pars/UNIT_TESTER/run/pars/nucl-removed.tree.expected?old=HEAD&old_path=branches%2Fpars%2FUNIT_TESTER%2Frun%2Fpars%2Fnucl-initial.tree.expected
    // diff initial->add-quick: http://bugs.arb-home.de/changeset/HEAD/branches/pars/UNIT_TESTER/run/pars/nucl-add-quick.tree.expected?old=HEAD&old_path=branches%2Fpars%2FUNIT_TESTER%2Frun%2Fpars%2Fnucl-initial.tree.expected
    // diff initial->add-NNI:   http://bugs.arb-home.de/changeset/HEAD/branches/pars/UNIT_TESTER/run/pars/nucl-add-NNI.tree.expected?old=HEAD&old_path=branches%2Fpars%2FUNIT_TESTER%2Frun%2Fpars%2Fnucl-initial.tree.expected

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_REMOVE_MARKED, "nucl-removed",   PARSIMONY_ORG-93, env, true)); // test remove-marked only (same code as part of nt_reAdd)
    TEST_EXPECT_EQUAL(env.combines_performed(), 3);

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_QUICK_READD,     "nucl-add-quick", PARSIMONY_ORG-23, env, true)); // test quick-add
    TEST_EXPECT_EQUAL(env.combines_performed(), 562);

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_READD_NNI,       "nucl-add-NNI",   PARSIMONY_ORG-25, env, true)); // test add + NNI
    TEST_EXPECT_EQUAL(env.combines_performed(), 718);

    // test partial-add
    {
        GBDATA *gb_main = env.gbmain();

        // create 2 non-overlapping partial species
        const int  SPLIT    = 55;
        GBDATA    *CorGlutP = createPartialSeqFrom(gb_main, aliname, "CorGlutP", "CorGluta", 0,       SPLIT);
        GBDATA    *CloButyP = createPartialSeqFrom(gb_main, aliname, "CloButyP", "CloButyr", SPLIT+1, INT_MAX);
        GBDATA    *CloButyM = createPartialSeqFrom(gb_main, aliname, "CloButyM", "CloButyr", SPLIT+1, INT_MAX);
        TEST_EXPECT_NO_ERROR(modifyOneBase(CloButyM, aliname, 'G', 'C')); // change first 'G' into 'C'

        TEST_VALIDITY(env.all_available_pops_will_produce_valid_trees()); // no push yet (does nothing)

        // add CloButyP (and undo)
        {
            env.push();
            // CloButyr and CloButy2 do not differ in seq-range of partial -> any of both may be chosen as brother.
            // behavior should be changed with #605
            TEST_EXPECTATION(addingPartialResultsIn(CloButyP, "CloButyr;CloButy2", "nucl-addPart-CloButyP", PARSIMONY_ORG, env));
            TEST_EXPECT_EQUAL(env.combines_performed(), 6);
            env.pop();
        }

        {
            env.push();
            TEST_EXPECTATION(addingPartialResultsIn(CorGlutP, "CorGluta",          "nucl-addPart-CorGlutP",          PARSIMONY_ORG, env)); // add CorGlutP
            TEST_EXPECT_EQUAL(env.combines_performed(), 5); // @@@ partial-add should not perform combines at all
            TEST_EXPECTATION(addingPartialResultsIn(CloButyP, "CloButyr;CloButy2", "nucl-addPart-CorGlutP-CloButyP", PARSIMONY_ORG, env)); // also add CloButyP
            TEST_EXPECT_EQUAL(env.combines_performed(), 6);
            env.pop();
        }

        // now add CorGlutP as full, then CloButyP and CloButyM as partials
        {
            env.push();

            mark_only(CorGlutP);
            env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)
            {
                GB_transaction  ta(gb_main);
                TEST_EXPECT_NO_ERROR(GBT_write_int(CorGlutP, "ARB_partial", 0)); // revert species to "full"
            }

            TEST_EXPECTATION(modifyingTopoResultsIn(MOD_QUICK_READD, "nucl-addPartialAsFull-CorGlutP", PARSIMONY_ORG, env, false));
            TEST_EXPECT_EQUAL(env.combines_performed(), 228);
            TEST_EXPECT_EQUAL(is_partial(CorGlutP), 0); // check CorGlutP was added as full sequence
            TEST_EXPECTATION(addedAsBrotherOf("CorGlutP", "CorGluta", env)); // partial created from CorGluta gets inserted next to CorGluta

            // add CloButyP as partial.
            // as expected it is placed next to matching full sequences (does not differ in partial range)
            TEST_EXPECTATION(addingPartialResultsIn(CloButyP, "CloButyr;CloButy2", NULL, PARSIMONY_ORG, env));
            TEST_EXPECT_EQUAL(env.combines_performed(), 6);

            // CloButyM differs slightly in overlap with CloButyr/CloButy2, but has no overlap with CorGlutP
            // shows bug described in #609 is fixed:
            TEST_EXPECTATION(addingPartialResultsIn(CloButyM, "CloButyP", "nucl-addPart-bug609",
                                                    PARSIMONY_ORG+1, // @@@ known bug - partial should not affect parsimony value; possibly related to ../HELP_SOURCE/oldhelp/pa_partial.hlp@WARNINGS 
                                                    env));
            TEST_EXPECT_EQUAL(env.combines_performed(), 7);
            env.pop();
        }
    }

    TEST_EXPECT_SAVED_TOPOLOGY(env, "nucl-initial");

    const int PARSIMONY_NNI_MARKED   = PARSIMONY_ORG-18;
    const int PARSIMONY_NNI_ALL      = PARSIMONY_ORG-18;
    const int PARSIMONY_OPTI_MARKED  = PARSIMONY_ORG-25;
    const int PARSIMONY_OPTI_VISIBLE = PARSIMONY_ORG-26;
    const int PARSIMONY_OPTI_ALL     = PARSIMONY_ORG-36;

    {
        env.push();
        TEST_EXPECTATION(movingRootDoesntAffectCosts(PARSIMONY_ORG));
        TEST_EXPECT_EQUAL(env.combines_performed(), 342);
        env.pop();
    }

    // ------------------------------
    //      test optimize (some)

    // mark initially marked species
    {
        GB_transaction ta(env.gbmain());
        GBT_restore_marked_species(env.gbmain(), "CorAquat;CorGluta;CurCitre;CloButyr;CloButy2;CytAquat");
        env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)
    }

    TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);
    TEST_EXPECT_EQUAL(env.combines_performed(), 0);

    // test branchlength calculation
    // (optimizations below implicitely recalculates branchlengths)
    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_CALC_LENS, "nucl-calclength", PARSIMONY_ORG, env, false));
    TEST_EXPECT_EQUAL(env.combines_performed(), 120);

    // test whether branchlength calculation depends on root-position
    {
        AP_tree_edge *orgRootEdge = rootEdge();

        env.push();

        const char *tested_roots[] = {
            "CloButyr",
            "CloTyro4",
            "CloTyrob",
            "CloInnoc",
        };

        for (size_t r = 0; r<ARRAY_ELEMS(tested_roots); ++r) {
            const char *leafName = tested_roots[r];
            env.root_node()->findLeafNamed(leafName)->set_root();
            calc_branchlengths(env.graphic_tree());
            orgRootEdge->set_root();
            env.graphic_tree()->reorder_tree(BIG_BRANCHES_TO_TOP);

            TEST_EXPECT_SAVED_TOPOLOGY(env, "nucl-calclength");
        }
        TEST_EXPECT_EQUAL(env.combines_performed(), 517);

        env.pop();
    }

    // test optimize (some)
    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_NNI, "nucl-opti-NNI", PARSIMONY_NNI_MARKED, env, true)); // test recursive NNI
    TEST_EXPECT_EQUAL(env.combines_performed(), 208);

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_GLOBAL, "nucl-opti-marked-global", PARSIMONY_OPTI_MARKED, env, true)); // test recursive NNI+KL
    TEST_EXPECT_EQUAL(env.combines_performed(), 18518);

    {
        KL_Settings& KL = env.get_KL_settings();
        LocallyModify<EdgeSpec> target(KL.whichEdges, EdgeSpec(KL.whichEdges&~SKIP_UNMARKED_EDGES)); // ignore marks; skip folded

        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_GLOBAL, "nucl-opti-visible-global", PARSIMONY_OPTI_VISIBLE, env, true)); // same result as if all species marked (see below)
        TEST_EXPECT_EQUAL(env.combines_performed(), 34925);

        KL.whichEdges = EdgeSpec(KL.whichEdges&~SKIP_FOLDED_EDGES); // ignore marks and folding

        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_GLOBAL, "nucl-opti-global", PARSIMONY_OPTI_ALL, env, true)); // same result as if all species marked and all groups unfolded (see below)
        TEST_EXPECT_EQUAL(env.combines_performed(), 124811);
    }

    // -----------------------------
    //      test optimize (all)

    // mark all species
    mark_all(env.gbmain());
    // unmark species not in tree
    {
        GB_transaction  ta(env.gbmain());
        GB_HASH        *markedNotInTree = GBT_create_marked_species_hash(env.gbmain());
        NT_remove_species_in_tree_from_hash(env.root_node(), markedNotInTree);
        GBS_hash_do_loop(markedNotInTree, unmark_unwanted, NULL);
        GBS_free_hash(markedNotInTree);
    }
    env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)
    TEST_EXPECT_EQUAL(GBT_count_marked_species(env.gbmain()), 15);

    TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);
    TEST_EXPECT_EQUAL(env.combines_performed(), 0);

    // test branchlength calculation
    // (optimizations below implicitely recalculates branchlengths)
    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_CALC_LENS, "nucl-calclength", PARSIMONY_ORG, env, false));
    TEST_EXPECT_EQUAL(env.combines_performed(), 120);

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_NNI, "nucl-opti-all-NNI", PARSIMONY_NNI_ALL, env, true)); // test recursive NNI
    TEST_EXPECT_EQUAL(env.combines_performed(), 242);

    {
        env.push();
        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_GLOBAL, "nucl-opti-visible-global", PARSIMONY_OPTI_VISIBLE, env, false)); // test recursive NNI+KL
        TEST_EXPECT_EQUAL(env.combines_performed(), 34925);

        TEST_EXPECTATION(movingRootDoesntAffectCosts(PARSIMONY_OPTI_VISIBLE));
        TEST_EXPECT_EQUAL(env.combines_performed(), 336);
        env.pop();
    }

    // unfold groups
    {
        AP_tree_nlen *CloTyrob = env.root_node()->findLeafNamed("CloTyrob");
        AP_tree_nlen *group    = CloTyrob->get_father();
        ap_assert(group->gr.grouped);
        group->gr.grouped      = false; // unfold the only folded group

        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_GLOBAL, "nucl-opti-global", PARSIMONY_OPTI_ALL, env, true)); // test recursive NNI+KL
        TEST_EXPECT_EQUAL(env.combines_performed(), 124811);
    }

    // test re-add all (i.e. test "create tree from scratch")
    // Note: trees generated below are NO LONGER better than optimized trees! (see also r13651)
    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_QUICK_READD,     "nucl-readdall-quick", PARSIMONY_ORG-23, env, true)); // quick
    TEST_EXPECT_EQUAL(env.combines_performed(), 751);

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_READD_NNI,       "nucl-readdall-NNI",   PARSIMONY_ORG-27, env, true)); // + NNI
    TEST_EXPECT_EQUAL(env.combines_performed(), 926);

    // test adding a too short sequence
    // (has to be last test, because it modifies seq data)                    << ------------ !!!!!
    {
        env.push();

        AP_tree_nlen *CloTyrob = env.root_node()->findLeafNamed("CloTyrob");
        mark_only(CloTyrob->gb_node);
        env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)

        // modify sequence of CloTyrob (keep only some bases)
        {
            GB_transaction  ta(env.gbmain());
            GBDATA         *gb_seq = GBT_find_sequence(CloTyrob->gb_node, aliname);

            char *seq        = GB_read_string(gb_seq);
            int   keep_bases = MIN_SEQUENCE_LENGTH-1;

            for (int i = 0; seq[i]; ++i) {
                if (seq[i] != '.' && seq[i] != '-') {
                    if (keep_bases) --keep_bases;
                    else seq[i] = '.';
                }
            }
            GB_push_my_security(gb_seq);
            TEST_EXPECT_NO_ERROR(GB_write_string(gb_seq, seq));
            GB_pop_my_security(gb_seq);
            free(seq);
        }

        // remove CloTyrob
        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_REMOVE_MARKED, NULL, PARSIMONY_ORG-1, env, false));
        TEST_EXPECT_EQUAL(env.combines_performed(), 4);
        TEST_EXPECT_EQUAL(env.root_node()->count_leafs(), 14);

        // attempt to add CloTyrob (should fail because sequence too short) and CorGluta (should stay, because already in tree)
        TEST_REJECT_NULL(env.root_node()->findLeafNamed("CorGluta")); // has to be in tree
        {
            GB_transaction ta(env.gbmain());
            GBT_restore_marked_species(env.gbmain(), "CloTyrob;CorGluta");
            env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)
        }

        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_QUICK_ADD, NULL, PARSIMONY_ORG-1, env, false));
        TEST_EXPECT_EQUAL(env.combines_performed(), 110); // @@@ why does this perform combines at all?
        TEST_EXPECT_EQUAL(env.root_node()->count_leafs(), 14); // ok, CloTyrob was not added
        TEST_REJECT_NULL(env.root_node()->findLeafNamed("CorGluta")); // has to be in tree

        env.pop();
    }
}

void TEST_prot_tree_modifications() {
    const char *aliname = "ali_tuf_pro";

    PARSIMONY_testenv<AP_sequence_protein> env("TEST_prot.arb", aliname);
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_prot_opti"));
    TEST_EXPECT_SAVED_TOPOLOGY(env, "prot-initial");

    const int PARSIMONY_ORG = 1081;
    TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);
    TEST_EXPECT_EQUAL(env.combines_performed(), 10);

    // [PROTOPTI] opposed to nucleid tests above the initial tree here is already optimized! compare .@NUCOPTI
    // -> adding species approximately reproduces initial topology
    //
    // diff initial->add-quick: http://bugs.arb-home.de/changeset/HEAD/branches/pars/UNIT_TESTER/run/pars/prot-add-quick.tree.expected?old=HEAD&old_path=branches%2Fpars%2FUNIT_TESTER%2Frun%2Fpars%2Fprot-initial.tree.expected
    // diff initial->add-NNI:   http://bugs.arb-home.de/changeset/HEAD/branches/pars/UNIT_TESTER/run/pars/prot-add-NNI.tree.expected?old=HEAD&old_path=branches%2Fpars%2FUNIT_TESTER%2Frun%2Fpars%2Fprot-initial.tree.expected
    //
    // Note: comparing these two diffs also demonstrates why quick-adding w/o NNI suffers

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_REMOVE_MARKED, "prot-removed",   PARSIMONY_ORG-146, env, true)); // test remove-marked only (same code as part of nt_reAdd)
    TEST_EXPECT_EQUAL(env.combines_performed(), 5);

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_QUICK_READD,     "prot-add-quick", PARSIMONY_ORG,     env, true)); // test quick-add
    TEST_EXPECT_EQUAL(env.combines_performed(), 286);

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_READD_NNI,       "prot-add-NNI",   PARSIMONY_ORG,     env, true)); // test add + NNI
    TEST_EXPECT_EQUAL(env.combines_performed(), 336);

    // test partial-add
    {
        GBDATA *gb_main = env.gbmain();

        // create 2 non-overlapping partial species
        GBDATA    *MucRaceP = createPartialSeqFrom(gb_main, aliname, "MucRaceP", "MucRacem", 0, 60+4); // (+4 = dots inserted into DB at left end)
        GBDATA    *StrCoelP = createPartialSeqFrom(gb_main, aliname, "StrCoelP", "StrCoel9", 66-1+4, 184-1+4);
        GBDATA    *StrCoelM = createPartialSeqFrom(gb_main, aliname, "StrCoelM", "StrCoel9", 66-1+4, 184-1+4);
        TEST_EXPECT_NO_ERROR(modifyOneBase(StrCoelM, aliname, 'Y', 'H')); // change first 'Y' into 'H'

        // add StrCoelP (and undo)
        {
            env.push();
            // StrCoel9 and StrRamo3 do not differ in seq-range of partial -> any of both may be chosen as brother.
            // behavior should be changed with #605
            TEST_EXPECTATION(addingPartialResultsIn(StrCoelP, "StrCoel9;StrRamo3", "prot-addPart-StrCoelP", PARSIMONY_ORG,     env));
            TEST_EXPECT_EQUAL(env.combines_performed(), 4);
            env.pop();
        }

        {
            env.push();
            TEST_EXPECTATION(addingPartialResultsIn(MucRaceP, "MucRacem",          "prot-addPart-MucRaceP",          PARSIMONY_ORG,    env)); // add MucRaceP
            TEST_EXPECT_EQUAL(env.combines_performed(), 6);
            TEST_EXPECTATION(addingPartialResultsIn(StrCoelP, "StrCoel9;StrRamo3", "prot-addPart-MucRaceP-StrCoelP", PARSIMONY_ORG,    env)); // also add StrCoelP
            TEST_EXPECT_EQUAL(env.combines_performed(), 4);
            env.pop();
        }

        // now add MucRaceP as full, then StrCoelP and StrCoelM as partials
        {
            env.push();

            mark_only(MucRaceP);
            env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)
            {
                GB_transaction  ta(gb_main);
                TEST_EXPECT_NO_ERROR(GBT_write_int(MucRaceP, "ARB_partial", 0)); // revert species to "full"
            }

            TEST_EXPECTATION(modifyingTopoResultsIn(MOD_QUICK_READD, "prot-addPartialAsFull-MucRaceP", PARSIMONY_ORG,   env, false));
            TEST_EXPECT_EQUAL(env.combines_performed(), 158);
            TEST_EXPECT_EQUAL(is_partial(MucRaceP), 0); // check MucRaceP was added as full sequence
            TEST_EXPECTATION(addedAsBrotherOf("MucRaceP", "Eukarya EF-Tu", env)); // partial created from MucRacem gets inserted next to this group
            // Note: looks ok. group contains MucRacem, AbdGlauc and 4 other species

            // add StrCoelP as partial.
            // as expected it is placed next to matching full sequences (does not differ in partial range)
            TEST_EXPECTATION(addingPartialResultsIn(StrCoelP, "StrCoel9;StrRamo3", NULL, PARSIMONY_ORG, env));
            TEST_EXPECT_EQUAL(env.combines_performed(), 3);

            // StrCoelM differs slightly in overlap with StrCoel9/StrRamo3, but has no overlap with MucRaceP
            // shows bug described in #609 is fixed:
            TEST_EXPECTATION(addingPartialResultsIn(StrCoelM, "StrCoelP", "prot-addPart-bug609",
                                                    PARSIMONY_ORG+1,  // @@@ known bug - partial should not affect parsimony value; possibly related to ../HELP_SOURCE/oldhelp/pa_partial.hlp@WARNINGS
                                                    env));
            TEST_EXPECT_EQUAL(env.combines_performed(), 4);
            env.pop();
        }
    }

    TEST_EXPECT_SAVED_TOPOLOGY(env, "prot-initial");

    const unsigned mixseed = 1422292802;

    const int PARSIMONY_MIXED       = PARSIMONY_ORG + 1207;
    const int PARSIMONY_NNI_MARKED  = PARSIMONY_ORG + 1125;
    const int PARSIMONY_NNI_ALL     = PARSIMONY_ORG;
    const int PARSIMONY_OPTI_MARKED = PARSIMONY_ORG;
    const int PARSIMONY_OPTI_ALL    = PARSIMONY_ORG; // no gain (initial tree already is optimized)

    // ------------------------------------------------------
    //      mix tree (original tree already is optimized)

    GB_random_seed(mixseed);
    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_MIX_TREE, "prot-mixed", PARSIMONY_MIXED, env, false));
    TEST_EXPECT_EQUAL(env.combines_performed(), 90);

    {
        env.push();
        TEST_EXPECTATION(movingRootDoesntAffectCosts(PARSIMONY_MIXED));
        TEST_EXPECT_EQUAL(env.combines_performed(), 232);
        env.pop();
    }

    // ------------------------------
    //      test optimize (some)

    // mark initially marked species
    {
        GB_transaction ta(env.gbmain());

        GBT_restore_marked_species(env.gbmain(), "CytLyti6;StrRamo3;MucRace2;SacCere5");
        env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)
    }

    TEST_EXPECT_PARSVAL(env, PARSIMONY_MIXED);
    TEST_EXPECT_EQUAL(env.combines_performed(), 0);

    // test branchlength calculation
    // (optimizations below implicitely recalculates branchlengths)
    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_CALC_LENS, "prot-calclength", PARSIMONY_MIXED, env, false));
    TEST_EXPECT_EQUAL(env.combines_performed(), 80);

    // test whether branchlength calculation depends on root-position
    {
        AP_tree_edge *orgRootEdge = rootEdge();

        env.push();

        const char *tested_roots[] = {
            // "CytLyti6", // no effect on branchlengths
            "TaxOcell",
            "MucRace3",
            "StrCoel9",
        };

        for (size_t r = 0; r<ARRAY_ELEMS(tested_roots); ++r) {
            TEST_ANNOTATE(tested_roots[r]);
            const char *leafName = tested_roots[r];
            env.root_node()->findLeafNamed(leafName)->set_root();
            calc_branchlengths(env.graphic_tree());
            orgRootEdge->set_root();
            env.graphic_tree()->reorder_tree(BIG_BRANCHES_TO_TOP);

            TEST_EXPECT_SAVED_TOPOLOGY(env, "prot-calclength");
        }
        TEST_EXPECT_EQUAL(env.combines_performed(), 265);

        env.pop();
    }

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_NNI, "prot-opti-NNI", PARSIMONY_NNI_MARKED, env, true)); // test recursive NNI
    TEST_EXPECT_EQUAL(env.combines_performed(), 165);

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_GLOBAL, "prot-opti-marked-global", PARSIMONY_OPTI_MARKED, env, true)); // test recursive NNI+KL
    TEST_EXPECT_EQUAL(env.combines_performed(), 2535);

    // -----------------------------
    //      test optimize (all)

    // mark all species
    mark_all(env.gbmain());
    env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)
    TEST_EXPECT_EQUAL(GBT_count_marked_species(env.gbmain()), 14);

    TEST_EXPECT_PARSVAL(env, PARSIMONY_MIXED);
    TEST_EXPECT_EQUAL(env.combines_performed(), 0);

    // test branchlength calculation
    // (optimizations below implicitely recalculates branchlengths)
    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_CALC_LENS, "prot-calclength", PARSIMONY_MIXED, env, false));
    TEST_EXPECT_EQUAL(env.combines_performed(), 80);

    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_NNI, "prot-opti-all-NNI", PARSIMONY_NNI_ALL, env, true)); // test recursive NNI
    TEST_EXPECT_EQUAL(env.combines_performed(), 365);

    {
        env.push();
        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_OPTI_GLOBAL, "prot-opti-global", PARSIMONY_OPTI_ALL, env, false)); // test recursive NNI+KL
        TEST_EXPECT_EQUAL(env.combines_performed(), 2049);

        TEST_EXPECTATION(movingRootDoesntAffectCosts(PARSIMONY_OPTI_ALL));
        TEST_EXPECT_EQUAL(env.combines_performed(), 254);
        env.pop();
    }
}

void TEST_node_stack() {
    // test was used to fix #620

    const char *aliname = "ali_5s";
    PARSIMONY_testenv<AP_sequence_parsimony> env("TEST_trees.arb", aliname);
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));
    TEST_EXPECT_SAVED_TOPOLOGY(env, "nucl-initial");

    const int PARSIMONY_ORG = 301;
    TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);
    TEST_EXPECT_EQUAL(env.combines_performed(), 14);

    TEST_VALIDITY(env.root_node()->sequence_state_valid());

    // test set root to CytAquat + pop (works)
    {
        env.push();
        env.root_node()->findLeafNamed("CytAquat")->set_root();
        TEST_VALIDITY(env.root_node()->sequence_state_valid());
        env.pop();
        TEST_VALIDITY(env.root_node()->sequence_state_valid());
    }

    // test set root to CloButyr + pop (works)
    {
        env.push();
        env.root_node()->findLeafNamed("CloButyr")->set_root();
        TEST_VALIDITY(env.root_node()->sequence_state_valid());
        env.pop();
        TEST_VALIDITY(env.root_node()->sequence_state_valid());
    }

    // test set root to CloBifer + set root to CloTyrob + pop (works)
    // Note: both species are in same subtree (of root)
    {
        env.push();

        env.root_node()->findLeafNamed("CloBifer")->set_root();
        env.root_node()->findLeafNamed("CloTyrob")->set_root();

        TEST_VALIDITY(env.root_node()->sequence_state_valid());
        env.pop();
        TEST_VALIDITY(env.root_node()->sequence_state_valid());
    }

    // test set root to CytAquat + set root to CloButyr + pop (failed, fixed by [13138])
    TEST_EXPECT_EQUAL(env.combines_performed(), 0);
    for (int calcCostsBetween = 0; calcCostsBetween<2; ++calcCostsBetween) {
        TEST_ANNOTATE(GBS_global_string("calcCostsBetween=%i", calcCostsBetween));

        TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);

        env.push();

        env.root_node()->findLeafNamed("CytAquat")->set_root();

        if (calcCostsBetween) {
            TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);
            TEST_EXPECT_EQUAL(env.combines_performed(), 2);
        }

        env.root_node()->findLeafNamed("CloButyr")->set_root();

        TEST_VALIDITY(env.root_node()->sequence_state_valid());
        TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);
        TEST_EXPECT_EQUAL(env.combines_performed(), 6);

        env.pop();

        TEST_VALIDITY(env.root_node()->sequence_state_valid());
        TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);
        TEST_EXPECT_EQUAL(env.combines_performed(), 0);
        TEST_EXPECT_VALID_TREE(env.root_node());
    }
    {
        env.push();
        {
            env.push();

            env.root_node()->findLeafNamed("CloInnoc")->moveNextTo(env.root_node()->findLeafNamed("CytAquat"), 0.5);
            TEST_EXPECT_VALID_TREE(env.root_node());
            env.root_node()->findLeafNamed("CloInnoc")->set_root();
            TEST_EXPECT_VALID_TREE(env.root_node());
            env.root_node()->findLeafNamed("CytAquat")->moveNextTo(env.root_node()->findLeafNamed("CloPaste"), 0.5);
            TEST_EXPECT_VALID_TREE(env.root_node());
            env.root_node()->findLeafNamed("CloPaste")->set_root();
            TEST_EXPECT_VALID_TREE(env.root_node());
            env.root_node()->findLeafNamed("CloPaste")->moveNextTo(env.root_node()->findLeafNamed("CloInnoc"), 0.5);
            TEST_EXPECT_VALID_TREE(env.root_node());
            {
                AP_tree_nlen *son_of_brother;
                AP_tree_nlen *brother_of_father;

                // COVER1: son of root -> grandson of root
                {
                    AP_tree_nlen *son_of_root = env.root_node()->get_leftson();
                    ap_assert(son_of_root);

                    son_of_brother = son_of_root->get_brother()->get_leftson();
                    son_of_root->moveNextTo(son_of_brother, 0.5);
                    TEST_EXPECT_VALID_TREE(env.root_node());
                }

                // COVER2: grandson of root -> son of brother
                {
                    AP_tree_nlen *son_of_root      = env.root_node()->get_leftson();
                    AP_tree_nlen *grandson_of_root = son_of_root->get_brother()->get_rightson();
                    ap_assert(grandson_of_root);

                    son_of_brother = grandson_of_root->get_brother()->get_leftson();
                    grandson_of_root->moveNextTo(son_of_brother, 0.5);
                    TEST_EXPECT_VALID_TREE(env.root_node());
                }

                AP_tree_nlen *some_leaf = env.root_node()->findLeafNamed("CloBifer");
                ap_assert(some_leaf);

                // COVER3: some leaf -> son of brother
                son_of_brother = some_leaf->get_brother()->get_leftson();
                some_leaf->moveNextTo(son_of_brother, 0.5);
                TEST_EXPECT_VALID_TREE(env.root_node());

                // COVER4: some leaf -> son of brother
                brother_of_father = some_leaf->get_father()->get_brother();
                some_leaf->moveNextTo(brother_of_father, 0.5);
                TEST_EXPECT_VALID_TREE(env.root_node());

                // test forbidden moves:
                TEST_EXPECT_ERROR_CONTAINS(some_leaf->cantMoveNextTo(some_leaf->get_father()),  "Already there");
                TEST_EXPECT_ERROR_CONTAINS(some_leaf->cantMoveNextTo(some_leaf->get_brother()), "Already there");
            }

            env.pop();
            TEST_EXPECT_VALID_TREE(env.root_node());
        }
        env.pop();
        TEST_EXPECT_VALID_TREE(env.root_node());
    }

    // remove + quick add marked + pop() both works
    TEST_EXPECTATION(modifyingTopoResultsIn(MOD_QUICK_READD,     "nucl-add-quick", PARSIMONY_ORG-23, env, true)); // test quick-add

    // remove + quick-add marked + pop() quick-add -> corrupts tree
    // (root-edge is lost)
    {
        env.push();
        TEST_EXPECT_VALID_TREE(env.root_node());
        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_REMOVE_MARKED, NULL, -1, env, false)); // test remove-marked only (same code as part of nt_reAdd)
        TEST_EXPECT_VALID_TREE(env.root_node());

        TEST_VALIDITY(env.all_available_pops_will_produce_valid_trees());

        env.push();
        TEST_EXPECT_VALID_TREE(env.root_node());
        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_QUICK_READD, NULL, -1, env, false)); // test quick-add (same code as part of nt_reAdd)
        TEST_EXPECT_VALID_TREE(env.root_node());
        TEST_VALIDITY(env.all_available_pops_will_produce_valid_trees());
        env.pop();

        TEST_VALIDITY(env.root_node()->has_valid_edges());

        env.pop();
        TEST_EXPECT_VALID_TREE(env.root_node());
    }

    // same as above, but with only 1 species marked
    const char *testSingle[] = {
        "CytAquat",  // CytAquat is the only grandson of root (CytAquat located in lower subtree)
        "CloBifer",  // two father nodes between CloBifer and root (CloBifer located in upper subtree)
        "CloPaste",  // two father nodes between CloPaste and root (CloPaste located in upper subtree)
        "CorGluta",  // three father nodes between CorGluta and root (CorGluta located in lower subtree)
        "CelBiazo",  // two father nodes between CelBiazo and root
        NULL
    };

    for (int i = 0; testSingle[i]; ++i) {
        for (int swapped = 0; swapped<2; ++swapped) {
            TEST_ANNOTATE(GBS_global_string("single=%s swapped=%i", testSingle[i], swapped));

            env.push();
            TEST_EXPECT_VALID_TREE(env.root_node());
            {
                AP_tree_nlen *old_rightson = env.root_node()->get_rightson();
                env.root_node()->get_leftson()->get_rightson()->set_root();
                old_rightson->get_leftson()->set_root();
                old_rightson->set_root();

                ap_assert(env.root_node()->get_rightson() == old_rightson);
            }
            TEST_EXPECT_VALID_TREE(env.root_node());

            mark_only(env.root_node()->findLeafNamed(testSingle[i])->gb_node);
            env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)

            env.push();
            if (swapped) {
                env.root_node()->swap_sons();
            }

            TEST_EXPECT_VALID_TREE(env.root_node());
            TEST_EXPECTATION(modifyingTopoResultsIn(MOD_REMOVE_MARKED, NULL, -1, env, false)); // test remove-marked only (same code as part of nt_reAdd)
            TEST_EXPECT_VALID_TREE(env.root_node());

            env.push();
            TEST_EXPECT_VALID_TREE(env.root_node());
            TEST_EXPECTATION(modifyingTopoResultsIn(MOD_QUICK_READD, NULL, -1, env, false)); // test quick-add (same code as part of nt_reAdd)
            TEST_EXPECT_VALID_TREE(env.root_node());
            env.pop();

            TEST_EXPECT_VALID_TREE(env.root_node());
            env.pop();
            TEST_EXPECT_VALID_TREE(env.root_node());
            env.pop();
            TEST_EXPECT_VALID_TREE(env.root_node());
        }
    }

    // similar to above (remove+add a grandson of root; here grandson is a subtree with 4 species)

    for (int remove_from_lower_subtree = 0; remove_from_lower_subtree<2; ++remove_from_lower_subtree) {
        TEST_ANNOTATE(GBS_global_string("remove_from_lower_subtree=%i", remove_from_lower_subtree));

        // mark a complete subtree (which - as a whole - forms a grandson of root). subtree is located in upper part of the tree
        mark_only(env.root_node()->findLeafNamed("CloButy2")->gb_node);
        mark(env.root_node()->findLeafNamed("CloButyr")->gb_node);
        mark(env.root_node()->findLeafNamed("CloCarni")->gb_node);
        mark(env.root_node()->findLeafNamed("CloPaste")->gb_node);
        env.compute_tree(); // species marks affect order of node-chain (used in nni_rec)

        env.push();
        if (remove_from_lower_subtree) {
            env.root_node()->swap_sons();
        }
        TEST_EXPECT_VALID_TREE(env.root_node());
        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_REMOVE_MARKED, NULL, -1, env, false)); // test remove-marked only (same code as part of nt_reAdd)
        TEST_EXPECT_VALID_TREE(env.root_node());

        env.push();
        TEST_EXPECT_VALID_TREE(env.root_node());
        TEST_EXPECTATION(modifyingTopoResultsIn(MOD_QUICK_READD, NULL, -1, env, false)); // test quick-add (same code as part of nt_reAdd)
        TEST_EXPECT_VALID_TREE(env.root_node());
        env.pop();

        TEST_VALIDITY(env.root_node()->has_valid_edges()); // now always valid

        env.pop();
        TEST_EXPECT_VALID_TREE(env.root_node());
    }

    TEST_EXPECT_EQUAL(env.combines_performed(), 4122);
}

void TEST_node_edge_resources() {
    const char *aliname = "ali_5s";

    // document memsize of nodes and edges:

#define STATE_STACK_SIZE sizeof(StateStack) // 8 (Cxx11) or 16 (older C++); maybe 4/8 in 32bit

#if defined(ARB_64)
    TEST_EXPECT_EQUAL(sizeof(AP_tree_nlen), 184 + STATE_STACK_SIZE);
    TEST_EXPECT_EQUAL(sizeof(AP_tree), 136);
    TEST_EXPECT_EQUAL(sizeof(ARB_seqtree), 88);
    TEST_EXPECT_EQUAL(sizeof(TreeNode), 80);
#else // !defined(ARB_64)
    TEST_EXPECT_EQUAL(sizeof(AP_tree_nlen), 112 + STATE_STACK_SIZE);
    TEST_EXPECT_EQUAL(sizeof(AP_tree), 84);
    TEST_EXPECT_EQUAL(sizeof(ARB_seqtree), 48);
    TEST_EXPECT_EQUAL(sizeof(TreeNode), 44);
#endif


#if defined(ARB_64)
    TEST_EXPECT_EQUAL(sizeof(AP_tree_edge), 64);
#else // !defined(ARB_64)
    TEST_EXPECT_EQUAL(sizeof(AP_tree_edge), 32);
#endif

    PARSIMONY_testenv<AP_sequence_parsimony> env("TEST_trees.arb", aliname);
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));

    const int PARSIMONY_ORG = 301;

    TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);
    TEST_EXPECT_EQUAL(env.combines_performed(), 14);

    AP_tree_nlen *CloButyr = env.root_node()->findLeafNamed("CloButyr");
    AP_tree_nlen *CloButy2 = env.root_node()->findLeafNamed("CloButy2"); // brother of CloButyr

    AP_tree_nlen *CorAquat = env.root_node()->findLeafNamed("CorAquat");
    AP_tree_nlen *CurCitre = env.root_node()->findLeafNamed("CurCitre"); // brother of CorAquat

    CorAquat->REMOVE();

    for (int test = 1; test<=8; ++test) {
        // test == 1 -> provokes common nodes+edges in revert+accept
        // test == 2 -> provokes common nodes+edges in revert+accept
        // test == 3 -> provokes common nodes+edges in revert+accept
        // tests 4-7 do not provoke common nodes or edges

        for (int mode = 0; mode<=3; ++mode) {
            bool accept_outer = mode&2;
            bool accept_inner = mode&1;

            TEST_ANNOTATE(GBS_global_string("accept_outer=%i accept_inner=%i (mode=%i, test=%i)", accept_outer, accept_inner, mode, test));

            TEST_EXPECT_NULL(CorAquat->get_father());
            TEST_EXPECT(CloButyr->get_brother() == CloButy2);
            TEST_EXPECT_VALID_TREE(env.root_node());

            env.push();

            switch (test) {
                case 1: CorAquat->insert(CurCitre); break;
                case 2: CorAquat->insert(CurCitre); break;
                case 3: break;
                case 4: CloButyr->REMOVE(); break;
                case 5: CloButyr->REMOVE(); break;
                case 6: break;
                case 7: CloButyr->moveNextTo(CurCitre, 0.5); break;
                case 8: break;
                default: ap_assert(0); break;
            }
            TEST_EXPECT_VALID_TREE(env.root_node());

            {
                env.push();

                switch (test) {
                    case 1: CorAquat->REMOVE(); break;
                    case 2: break;
                    case 3: CorAquat->insert(CurCitre); break;
                    case 4: CloButyr->insert(CloButy2); break;
                    case 5: break;
                    case 6: CloButyr->REMOVE(); break;
                    case 7: CloButyr->moveNextTo(CloButy2, 0.5); break;
                    case 8: CorAquat->insert(CurCitre); CorAquat->REMOVE(); break;
                    default: ap_assert(0); break;
                }
                TEST_EXPECT_VALID_TREE(env.root_node());

                env.accept_if(accept_inner);
            }

            switch (test) {
                case 1: break;
                case 2: CorAquat->REMOVE(); break;
                case 3: if (CorAquat->father) CorAquat->REMOVE(); break;
                case 4: break;
                case 5: CloButyr->insert(CloButy2); break;
                case 6: if (!CloButyr->father) CloButyr->insert(CloButy2); break;
                case 7: CloButyr->REMOVE(); break;
                case 8: break;
                default: ap_assert(0); break;
            }
            TEST_EXPECT_VALID_TREE(env.root_node());

            env.accept_if(accept_outer);

            // manually revert changes (outside any stack frame)
            if (CorAquat->father) CorAquat->REMOVE();
            if (!CloButyr->father) CloButyr->insert(CloButy2);
        }
    }

    CorAquat->insert(CurCitre);

    TEST_EXPECT_PARSVAL(env, PARSIMONY_ORG);
    env.combines_performed(); // accept any no of combines
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

