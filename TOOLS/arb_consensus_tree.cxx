// ============================================================= //
//                                                               //
//   File      : arb_consensus_tree.cxx                          //
//   Purpose   : build consensus tree with the same library      //
//               as ARB-NJ                                       //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include <cstdio>
#include <cstdlib>
#include <arb_msg.h>
#include <arb_str.h>
#include <arb_strarray.h>
#include <arbdbt.h>
#include <TreeWrite.h>
#include <CT_ctree.hxx>
#include <TreeRead.h>
#include <math.h>
#include <map>
#include <string>

using namespace std;

struct subtreestat {
    int      nodes;
    GBT_LEN  lensum;
    char    *smallestName;

    subtreestat()
        : nodes(0),
          lensum(-1), 
          smallestName(0)
    {}
};

static GBT_TREE *sort_tree(GBT_TREE *tree, subtreestat &subtree) {
    if (tree->is_leaf) {
        subtree.nodes        = 1;
        subtree.lensum       = 0;
        subtree.smallestName = tree->name;
    }
    else {
        subtreestat left, right;

        tree->leftson  = sort_tree(tree->leftson,  left);
        tree->rightson = sort_tree(tree->rightson, right);

        left.lensum  += tree->leftlen;
        right.lensum += tree->rightlen;

        subtree.nodes        = left.nodes+right.nodes;
        subtree.lensum       = left.lensum+right.lensum;
        subtree.smallestName = strcmp(left.smallestName, right.smallestName)<0 ? left.smallestName : right.smallestName;

        int cmp = left.nodes - right.nodes;
        if (!cmp) {
            GBT_LEN lendiff = left.lensum - right.lensum;
            if (lendiff != 0.0) {
                cmp = lendiff/fabs(lendiff)*2;
            }
            else {
                cmp = strcmp(tree->leftson->name, tree->rightson->name);
            }
        }

        arb_assert(cmp != 0); // otherwise unstable - use more data to sort
        if (cmp>0) { // swap branches
            std::swap(tree->leftlen, tree->rightlen);
            std::swap(tree->leftson, tree->rightson);
        }
    }

    arb_assert(subtree.nodes);
    arb_assert(subtree.lensum >= 0.0);
    arb_assert(subtree.smallestName);
    return tree;
}

inline GBT_TREE *sort_tree(GBT_TREE *tree) {
    subtreestat dummy;
    return sort_tree(tree, dummy);
}

static GBT_TREE *build_consensus_tree(const CharPtrArray& input_trees, GB_ERROR& error, bool sort_generated_tree, size_t& different_species) {
    // read all input trees, generate and return consensus tree

    arb_assert(!error);
    error = NULL;

    GBT_TREE *consense_tree = NULL;
    if (input_trees.empty()) {
        error = "no trees given";
    }
    else {
        GBT_TREE *tree[input_trees.size()];
        StrArray  comment;

        typedef map<string, size_t> OccurCount;
        OccurCount                  species_occurred;

        for (size_t i = 0; !error && i<input_trees.size(); ++i) {
            char *found_comment = NULL;
            char *warning       = NULL;

            tree[i] = TREE_load(input_trees[i], sizeof(*tree[i]), &found_comment, 1, &warning);
            if (!tree[i]) {
                error = GBS_global_string("Failed to load tree '%s'", input_trees[i]);
            }
            else {
                comment.put(found_comment);

                size_t   name_count;
                GB_CSTR *names = GBT_get_names_of_species_in_tree(tree[i], &name_count);

                for (size_t n = 0; n<name_count; ++n) {
                    const char *name = names[n];
                    if (species_occurred.find(name) == species_occurred.end()) {
                        species_occurred[name] = 1;
                    }
                    else {
                        species_occurred[name]++;
                    }
                }

                free(names);
            }
        }

        if (!error) {
            size_t        species_count = species_occurred.size();
            ConstStrArray species_names;

            for (OccurCount::iterator s = species_occurred.begin(); s != species_occurred.end(); ++s) {
                species_names.put(s->first.c_str());
            }

            species_names.sort((CharPtrArray_compare_fun)strcmp, NULL); // sort names to make results deterministic
            different_species = species_names.size();

            ctree_init(species_count, species_names);
            for (size_t i = 0; i<input_trees.size(); ++i) {
                insert_ctree(tree[i], 1);
            }

            consense_tree = get_ctree();
            ctree_cleanup();
            
            if (sort_generated_tree) {
                consense_tree = sort_tree(consense_tree); // bring tree into a stable form, there seems to be sth random in ctree
                // @@@ determine what behaves random, could be an error as well
            }
        }

        for (size_t i = 0; i<input_trees.size(); ++i) {
            GBT_delete_tree(tree[i]);
        }
    }
    arb_assert(contradicted(consense_tree, error));
    return consense_tree;
}
static char *create_tree_name(const char *savename) {
    // create a DB treename (using savename as hint)
    char *tree_name;
    {
        // as default use part behind '/' and remove file extension
        const char *lslash   = strrchr(savename, '/');
        if (lslash) savename = lslash+1;

        const char *ldot = strrchr(savename, '.');

        tree_name = ldot ? GB_strpartdup(savename, ldot-1) : strdup(savename);
        if (tree_name[0] == 0) freedup(tree_name, "tree_consense");
    }

    // make sure tree name starts with 'tree_'
    if (ARB_strscmp(tree_name, "tree_") != 0) {
        freeset(tree_name, GBS_global_string_copy("tree_%s", tree_name));
    }
    return tree_name;
}

static GB_ERROR save_tree_as_newick(GBT_TREE *tree, const char *savename) {
    // save a tree to a newick file

    // since ARB only saves trees out of a database,
    // i use a hack here:
    // - create temp DB
    // - save tree there
    // - save to newick as usual

    GB_shell  shell;
    GBDATA   *gb_main = GB_open("", "crw");
    GB_ERROR  error   = NULL;

    if (!gb_main) {
        error = GB_await_error();
    }
    else {
        char *db_tree_name = create_tree_name(savename);

        {
            GB_transaction ta(gb_main);
            error = GBT_write_tree(gb_main, 0, db_tree_name, tree);
        }
        if (!error) error = TREE_write_Newick(gb_main, db_tree_name, NULL, true, true, true, true, TREE_SINGLE_QUOTES, savename);

        free(db_tree_name);
        GB_close(gb_main);
    }

    if (error) {
        error = GBS_global_string("Failed to save tree to '%s' (Reason: %s)", savename, error);
    }

    return error;
}

int ARB_main(int argc, const char *argv[]) {
    GB_ERROR error = NULL;

    if (argc<2) {
        printf("Usage: arb_consensus_tree [options] [tree]+\n"
               "Purpose: Create a consensus tree out of multiple trees\n"
               "  options:\n"
               "  -w outfile      write consensus tree to outfile\n");

        // @@@ wanted options
        // - do not add relative frequency of used subtrees as bootstrap values
        // - multifurcate branches with bootstrap value below XXX
        // - eliminate branches with bootstrap value below YYY
        // - ... ?
    }
    else {
        char *savename = NULL;

        ConstStrArray input_tree_names;

        for (int a = 1; a<argc; ++a) {
            const char *arg = argv[a];
            if (arg[0] == '-') {
                switch (arg[1]) {
                    case 'w': savename = strdup(argv[++a]); break;
                    default : error = GBS_global_string("Unknown switch '-%c'", arg[1]); break;
                }
            }
            else {
                input_tree_names.put(argv[a]);
            }
        }

        if (!error && input_tree_names.empty()) error = "no input trees specified";

        if (!error) {
            size_t species_count;
            GBT_TREE *cons_tree = build_consensus_tree(input_tree_names, error, true, species_count);

            if (!cons_tree) {
                error = GBS_global_string("Failed to build consense tree (Reason: %s)", error);
            }
            else {
                size_t leafs   = GBT_count_leafs(cons_tree);
                double percent = size_t((leafs*1000)/species_count)/10.0;

                printf("Generated tree contains %.1f%% of species (%zu of %zu found in input trees)\n",
                       percent, leafs, species_count);

                if (savename) {
                    error = save_tree_as_newick(cons_tree, savename);
                }
                else {
                    printf("sucessfully created consense tree\n"
                           "(no savename specified -> tree not saved)\n");
                }
                GBT_delete_tree(cons_tree);
            }
        }
    }

    if (error) {
        printf("Error in arb_consensus_tree: %s\n", error);
    }

    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#include "command_output.h"

static char *savename(int dir) { return GBS_global_string_copy("consense/%i/consense.tree", dir); }
static char *expected_name(int dir) { return GBS_global_string_copy("consense/%i/consense_expected.tree", dir); }
static char *inputname(int dir, int tree) { return GBS_global_string_copy("consense/%i/bootstrapped_%i.tree", dir, tree); }

static void add_inputnames(StrArray& to, int dir, int first_tree, int last_tree) {
    for (int t = first_tree; t <= last_tree; ++t) {
        to.put(inputname(dir, t));
    }
}

void TEST_consensus_tree_1() {
    GB_ERROR error = NULL;
    StrArray input_tree_names;
    add_inputnames(input_tree_names, 1, 1, 5);

    {
        size_t species_count;
        GBT_TREE *tree = build_consensus_tree(input_tree_names, error, true, species_count);
        TEST_ASSERT(!error);
        TEST_ASSERT(tree);

        TEST_ASSERT_EQUAL(species_count, 22);
        TEST_ASSERT_EQUAL(GBT_count_leafs(tree), species_count);

        char *saveas   = savename(1);
        char *expected = expected_name(1);
        
        TEST_ASSERT_NO_ERROR(save_tree_as_newick(tree, saveas));

        // ../UNIT_TESTER/run/consense/1/consense.tree

        TEST_ASSERT_TEXTFILE_DIFFLINES(saveas, expected, 1);
        TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));
        
        free(expected);
        free(saveas);

        GBT_delete_tree(tree);
    }
}
static double calc_intree_distance(GBT_TREE *tree) {
    if (tree->is_leaf) return 0.0;
    return
        tree->leftlen +
        tree->rightlen +
        calc_intree_distance(tree->leftson) +
        calc_intree_distance(tree->rightson);
}

#define LENSUM_EPSILON .000001

void TEST_consensus_tree_1_single() {
    GB_ERROR error = NULL;
    StrArray input_tree_names;
    add_inputnames(input_tree_names, 1, 1, 1);

    {
        size_t species_count;
        GBT_TREE *tree = build_consensus_tree(input_tree_names, error, true, species_count);
        TEST_ASSERT(!error);
        TEST_ASSERT(tree);

        TEST_ASSERT_EQUAL(species_count, 22);
        TEST_ASSERT_EQUAL(GBT_count_leafs(tree), species_count);
        TEST_ASSERT_SIMILAR(calc_intree_distance(tree), 0.924610, LENSUM_EPSILON);

        char       *saveas   = savename(1);
        const char *expected = "consense/1/consense_expected_single.tree";

        TEST_ASSERT_NO_ERROR(save_tree_as_newick(tree, saveas));

        // ../UNIT_TESTER/run/consense/1/consense.tree

        TEST_ASSERT_TEXTFILE_DIFFLINES(saveas, expected, 1);
        TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));

        free(saveas);

        GBT_delete_tree(tree);
    }
}

void TEST_consensus_tree_2_lost_branches() {
    GB_ERROR      error = NULL;
    StrArray input_tree_names;
    add_inputnames(input_tree_names, 2, 1, 4);

    {
        size_t species_count;
        // build_consensus_tree here fails the disabled assertion in ../CONSENSUS_TREE/CT_rbtree.cxx@disabled_multifurc_assertion
        GBT_TREE *tree = build_consensus_tree(input_tree_names, error, true, species_count);
        TEST_ASSERT(!error);
        TEST_ASSERT(tree);

        TEST_ASSERT_EQUAL(species_count, 59);
        TEST_ASSERT_EQUAL(GBT_count_leafs(tree), species_count);
        TEST_ASSERT_SIMILAR(calc_intree_distance(tree), 2.917387, LENSUM_EPSILON);

        char *saveas   = savename(2);
        char *expected = expected_name(2);
        
        TEST_ASSERT_NO_ERROR(save_tree_as_newick(tree, saveas));

        // ../UNIT_TESTER/run/consense/2/consense.tree

        TEST_ASSERT_TEXTFILE_DIFFLINES(saveas, expected, 1);
        TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));

        free(expected);
        free(saveas);

        GBT_delete_tree(tree);
    }
}

void TEST_consensus_tree_3() {
    GB_ERROR      error = NULL;
    StrArray input_tree_names;
    add_inputnames(input_tree_names, 3, 1, 3);

    {
        size_t species_count;
        GBT_TREE *tree = build_consensus_tree(input_tree_names, error, true, species_count);
        TEST_ASSERT(!error);
        TEST_ASSERT(tree);

        TEST_ASSERT_EQUAL(species_count, 128);
        TEST_ASSERT_EQUAL(GBT_count_leafs(tree), species_count);
        TEST_ASSERT_SIMILAR(calc_intree_distance(tree), 2.169135, LENSUM_EPSILON);

        char *saveas   = savename(3);
        char *expected = expected_name(3);
        
        TEST_ASSERT_NO_ERROR(save_tree_as_newick(tree, saveas));

        TEST_ASSERT_TEXTFILE_DIFFLINES(saveas, expected, 1);
        TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));

        free(expected);
        free(saveas);

        GBT_delete_tree(tree);
    }
}

#define REPEATED_TESTS

#if defined(REPEATED_TESTS)
void TEST_consensus_tree_generation_is_deterministic() {
    TEST_consensus_tree_3();
    TEST_consensus_tree_2_lost_branches();
    TEST_consensus_tree_1();
    TEST_consensus_tree_1_single();
}

void TEST_arb_consensus_tree() {
    TEST_STDOUT_CONTAINS("(arb_consensus_tree -x || true)", "Unknown switch '-x'");
    TEST_STDOUT_CONTAINS("(arb_consensus_tree -w sth || true)", "no input trees specified");

    {
        char *saveas   = savename(1);
        char *expected = expected_name(1);
    
        TEST_STDOUT_CONTAINS("arb_consensus_tree"
                             " -w consense/1/consense.tree"
                             " consense/1/bootstrapped_1.tree"
                             " consense/1/bootstrapped_2.tree"
                             " consense/1/bootstrapped_3.tree"
                             " consense/1/bootstrapped_4.tree"
                             " consense/1/bootstrapped_5.tree"
                             ,
                             "database  created");

        TEST_ASSERT_TEXTFILE_DIFFLINES(saveas, expected, 1);
        TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));

        free(expected);
        free(saveas);
    }

    {
        char *saveas   = savename(2);
        char *expected = expected_name(2);
    
        TEST_STDOUT_CONTAINS("arb_consensus_tree"
                             " -w consense/2/consense.tree"
                             " consense/2/bootstrapped_1.tree"
                             " consense/2/bootstrapped_2.tree"
                             " consense/2/bootstrapped_3.tree"
                             " consense/2/bootstrapped_4.tree"
                             ,
                             "database  created");

        TEST_ASSERT_TEXTFILE_DIFFLINES(saveas, expected, 1);
        TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));
        
        free(expected);
        free(saveas);
    }
}
#endif

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------



