// ============================================================= //
//                                                               //
//   File      : CT_ctree.cxx                                    //
//   Purpose   : consensus tree                                  //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "CT_ctree.hxx"
#include "CT_hash.hxx"
#include "CT_ntree.hxx"
#include <arb_strbuf.h>

ConsensusTree::ConsensusTree(const CharPtrArray& names_)
    : overall_weight(0),
      Name_hash(NULL),
      size(NULL),
      names(names_),
      insertProgress(NULL)
{
    // names = leafnames (=species names)

    int leaf_count = names.size();
    Name_hash = GBS_create_hash(leaf_count, GB_MIND_CASE);
    for (int i=0; i< leaf_count; i++) {
        GBS_write_hash(Name_hash, names[i], i+1);
    }

    size       = new PartitionSize(leaf_count);
    allSpecies = size->create_root();
    registry   = new PartRegistry;
}

ConsensusTree::~ConsensusTree() {
    arb_assert(!insertProgress);

    delete registry;
    delete allSpecies;
    delete size;

    if (Name_hash) {
        GBS_free_hash(Name_hash);
        Name_hash  = 0;
    }
}

GB_ERROR ConsensusTree::insert_tree_weighted(const GBT_TREE *tree, int leafs, double weight, bool provideProgress) {
    // Insert a GBT-tree in the Hash-Table
    // The GBT-tree is destructed afterwards!
    arb_assert(GBT_count_leafs(tree) == size_t(leafs));

    overall_weight += weight;

    PART *wholeTree = create_tree_PART(tree, weight);
    if (wholeTree->get_members() != leafs) {
        arb_assert(wholeTree->get_members() < leafs);
        return "tree contains duplicated species";
    }

    if (provideProgress) {
        int nodeCount  = leafs_2_nodes(wholeTree->get_members(), ROOTED);
        insertProgress = new arb_progress(nodeCount);
    }

    bool contains_all_species = wholeTree->equals(allSpecies);

    if (contains_all_species) {
        deconstruct_full_rootnode(tree, weight);
    }
    else {
        deconstruct_partial_rootnode(tree, weight, wholeTree);
    }

    if (provideProgress) {
        delete insertProgress;
        insertProgress = NULL;
    }

    delete wholeTree;
    return NULL;
}

SizeAwareTree *ConsensusTree::get_consensus_tree(GB_ERROR& error) {
    // Get new consensus-tree -> SizeAwareTree

    /* This function is little bit tricky:
       the root-partition consist of 111....111 so it must have two sons
       that represent the same partition son1 == ~son2 to do this we must split
       the fist son-partition in two parts through logical calculation there
       could only be one son! */

    arb_assert(!error);
    ntree_init(size);

    registry->build_sorted_list(overall_weight);

    arb_progress progress(registry->size()+2);
    {
#if defined(DUMP_PART_INSERTION)
        PART::start_pretty_printing(names);
#endif

        PART *p = registry->get_part();
        while (p != NULL && !progress.aborted()) {
            insert_ntree(p);
            ++progress;
            p = registry->get_part();
        }

#if defined(DUMP_PART_INSERTION)
        PART::stop_pretty_printing();
#endif
    }

    SizeAwareTree *result_tree = NULL;

    error = progress.error_if_aborted();
    if (!error) {
        const NT_NODE *n = ntree_get();

        arb_assert(ntree_count_sons(n) == 2);

#if defined(NTREE_DEBUG_FUNCTIONS)
        arb_assert(is_well_formed(n));
#endif

        result_tree = rb_gettree(n);

        ++progress;

        result_tree->get_tree_root()->find_innermost_edge().set_root();
        result_tree->reorder_tree(BIG_BRANCHES_TO_BOTTOM);

        ++progress;
    }

    ntree_cleanup();
    arb_assert(contradicted(result_tree, error));
    return result_tree;
}


char *ConsensusTreeBuilder::get_remark() const {
    GBS_strstruct remark(1000);
    {
        char *build_info = GBS_global_string_copy("ARB consensus tree build from %zu trees:", tree_info.size());
        char *dated      = GBS_log_dated_action_to(NULL, build_info);
        remark.cat(dated);
        free(dated);
        free(build_info);
    }

    size_t allCount = species_count();

    size_t maxnamelen = 0;
    for (size_t t = 0; t<tree_info.size(); ++t) {
        maxnamelen = std::max(maxnamelen, strlen(tree_info[t].name()));
    }
    for (size_t t = 0; t<tree_info.size(); ++t) {
        const TreeInfo& tree = tree_info[t];
        remark.cat(" - ");
        remark.nprintf(maxnamelen, "%-*s", int(maxnamelen), tree.name());
        if (tree.species_count()<allCount) {
            double pc = tree.species_count() / double(allCount);
            remark.nprintf(50, " (%.1f%%; %zu/%zu)", pc*100.0, tree.species_count(), allCount);
        }
        remark.put('\n');
    }

    return remark.release();
}
