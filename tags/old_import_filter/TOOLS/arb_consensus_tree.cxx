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

#include <CT_ctree.hxx>
#include <TreeRead.h>
#include <TreeWrite.h>
#include <arb_str.h>

using namespace std;

static GBT_TREE *build_consensus_tree(const CharPtrArray& input_trees, GB_ERROR& error, size_t& different_species, double weight) {
    // read all input trees, generate and return consensus tree
    // (Note: the 'weight' used here doesn't matter atm, since all trees are added with the same weight)

    arb_assert(!error);
    error = NULL;

    GBT_TREE *consense_tree = NULL;
    if (input_trees.empty()) {
        error = "no trees given";
    }
    else {
        ConsensusTreeBuilder tree_builder;

        for (size_t i = 0; !error && i<input_trees.size(); ++i) {
            char *warning = NULL;

            GBT_TREE *tree = TREE_load(input_trees[i], sizeof(*tree), NULL, 1, &warning);
            if (!tree) {
                error = GBS_global_string("Failed to load tree '%s'", input_trees[i]);
            }
            else {
                tree_builder.add(tree, weight);
            }
        }

        if (!error) {
            consense_tree = tree_builder.get(different_species);
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
    if (!ARB_strBeginsWith(tree_name, "tree_")) {
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

int ARB_main(int argc, char *argv[]) {
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
            GBT_TREE *cons_tree = build_consensus_tree(input_tree_names, error, species_count, 1.0);

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
        free(savename);
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

static double calc_intree_distance(GBT_TREE *tree) {
    if (tree->is_leaf) return 0.0;
    return
        tree->leftlen +
        tree->rightlen +
        calc_intree_distance(tree->leftson) +
        calc_intree_distance(tree->rightson);
}

#define LENSUM_EPSILON .000001

// #define TEST_AUTO_UPDATE // uncomment to update expected trees

void TEST_consensus_tree_1() {
    GB_ERROR error = NULL;
    StrArray input_tree_names;
    add_inputnames(input_tree_names, 1, 1, 5);

    size_t    species_count;
    GBT_TREE *tree = build_consensus_tree(input_tree_names, error, species_count, 0.7);

    TEST_REJECT(error);
    TEST_REJECT_NULL(tree);

    TEST_EXPECT_EQUAL(species_count, 22);
    TEST_EXPECT_EQUAL(GBT_count_leafs(tree), species_count);

    TEST_EXPECT_SIMILAR(calc_intree_distance(tree), 0.925779, LENSUM_EPSILON);
        
    char *saveas   = savename(1);
    char *expected = expected_name(1);

    TEST_EXPECT_NO_ERROR(save_tree_as_newick(tree, saveas));

    // ../UNIT_TESTER/run/consense/1/consense.tree

#if defined(TEST_AUTO_UPDATE)
    system(GBS_global_string("cp %s %s", saveas, expected));
#else // !defined(TEST_AUTO_UPDATE)
    TEST_EXPECT_TEXTFILE_DIFFLINES(saveas, expected, 1);
#endif
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));
        
    free(expected);
    free(saveas);

    GBT_delete_tree(tree);
}
void TEST_consensus_tree_1_single() {
    GB_ERROR error = NULL;
    StrArray input_tree_names;
    add_inputnames(input_tree_names, 1, 1, 1);

    {
        size_t species_count;
        GBT_TREE *tree = build_consensus_tree(input_tree_names, error, species_count, 0.01);
        TEST_REJECT(error);
        TEST_REJECT_NULL(tree);

        TEST_EXPECT_EQUAL(species_count, 22);
        TEST_EXPECT_EQUAL(GBT_count_leafs(tree), species_count);
        TEST_EXPECT_SIMILAR(calc_intree_distance(tree), 0.924610, LENSUM_EPSILON);

        char       *saveas   = savename(1);
        const char *expected = "consense/1/consense_expected_single.tree";

        TEST_EXPECT_NO_ERROR(save_tree_as_newick(tree, saveas));

        // ../UNIT_TESTER/run/consense/1/consense.tree

#if defined(TEST_AUTO_UPDATE)
        system(GBS_global_string("cp %s %s", saveas, expected));
#else // !defined(TEST_AUTO_UPDATE)
        TEST_EXPECT_TEXTFILE_DIFFLINES(saveas, expected, 1);
#endif
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));

        free(saveas);

        GBT_delete_tree(tree);
    }
}

void TEST_consensus_tree_2() {
    GB_ERROR      error = NULL;
    StrArray input_tree_names;
    add_inputnames(input_tree_names, 2, 1, 4);

    {
        size_t species_count;
        GBT_TREE *tree = build_consensus_tree(input_tree_names, error, species_count, 2.5);
        TEST_REJECT(error);
        TEST_REJECT_NULL(tree);

        TEST_EXPECT_EQUAL(species_count, 59);
        TEST_EXPECT_EQUAL(GBT_count_leafs(tree), species_count);
        TEST_EXPECT_SIMILAR(calc_intree_distance(tree), 2.789272, LENSUM_EPSILON);

        char *saveas   = savename(2);
        char *expected = expected_name(2);
        
        TEST_EXPECT_NO_ERROR(save_tree_as_newick(tree, saveas));

        // ../UNIT_TESTER/run/consense/2/consense.tree

#if defined(TEST_AUTO_UPDATE)
        system(GBS_global_string("cp %s %s", saveas, expected));
#else // !defined(TEST_AUTO_UPDATE)
        TEST_EXPECT_TEXTFILE_DIFFLINES(saveas, expected, 1);
#endif
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));

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
        GBT_TREE *tree = build_consensus_tree(input_tree_names, error, species_count, 137.772);
        TEST_REJECT(error);
        TEST_REJECT_NULL(tree);

        TEST_EXPECT_EQUAL(species_count, 128);
        TEST_EXPECT_EQUAL(GBT_count_leafs(tree), species_count);
        TEST_EXPECT_SIMILAR(calc_intree_distance(tree), 2.171485, LENSUM_EPSILON);

        char *saveas   = savename(3);
        char *expected = expected_name(3);
        
        TEST_EXPECT_NO_ERROR(save_tree_as_newick(tree, saveas));

#if defined(TEST_AUTO_UPDATE)
        system(GBS_global_string("cp %s %s", saveas, expected));
#else // !defined(TEST_AUTO_UPDATE)
        TEST_EXPECT_TEXTFILE_DIFFLINES(saveas, expected, 1);
#endif
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));

        free(expected);
        free(saveas);

        GBT_delete_tree(tree);
    }
}

#define REPEATED_TESTS

#if defined(REPEATED_TESTS)
void TEST_consensus_tree_generation_is_deterministic() {
    TEST_consensus_tree_3();
    TEST_consensus_tree_2();
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

        TEST_EXPECT_TEXTFILE_DIFFLINES_IGNORE_DATES(saveas, expected, 0);
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));

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

        TEST_EXPECT_TEXTFILE_DIFFLINES_IGNORE_DATES(saveas, expected, 0);
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));

        free(expected);
        free(saveas);
    }
}
#endif

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------


