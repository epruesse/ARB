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
#include <arb_diff.h>
#include <arb_defs.h>

using namespace std;

static GBT_TREE *build_consensus_tree(const CharPtrArray& input_trees, GB_ERROR& error, size_t& different_species, double weight, char *&comment) {
    // read all input trees, generate and return consensus tree
    // (Note: the 'weight' used here doesn't matter atm, since all trees are added with the same weight)

    arb_assert(!error);
    error   = NULL;
    comment = NULL;

    GBT_TREE *consense_tree = NULL;
    if (input_trees.empty()) {
        error = "no trees given";
    }
    else {
        ConsensusTreeBuilder tree_builder;

        for (size_t i = 0; !error && i<input_trees.size(); ++i) {
            char *warnings = NULL;

            TreeRoot      *root = new TreeRoot(new SizeAwareNodeFactory, true); // will be deleted when tree gets deleted
            SizeAwareTree *tree = DOWNCAST(SizeAwareTree*, TREE_load(input_trees[i], *root, NULL, true, &warnings));
            if (!tree) {
                error = GBS_global_string("Failed to load tree '%s' (Reason: %s)", input_trees[i], GB_await_error());
            }
            else {
                if (warnings) {
                    GB_warningf("while loading tree '%s':\n%s", input_trees[i], warnings);
                    free(warnings);
                }
                tree_builder.add(tree, input_trees[i], weight);
            }
        }

        if (!error) consense_tree = tree_builder.get(different_species, error);
        if (!error) comment = tree_builder.get_remark();
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
        if (tree_name[0] == 0) freedup(tree_name, "tree_consensus");
    }

    // make sure tree name starts with 'tree_'
    if (!ARB_strBeginsWith(tree_name, "tree_")) {
        freeset(tree_name, GBS_global_string_copy("tree_%s", tree_name));
    }
    return tree_name;
}

static GB_ERROR save_tree_as_newick(GBT_TREE *tree, const char *savename, const char *comment) {
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
            error = GBT_write_tree_with_remark(gb_main, db_tree_name, tree, comment);
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
            size_t    species_count;
            char     *comment;
            GBT_TREE *cons_tree = build_consensus_tree(input_tree_names, error, species_count, 1.0, comment);

            if (!cons_tree) {
                error = GBS_global_string("Failed to build consensus tree (Reason: %s)", error);
            }
            else {
                size_t leafs   = GBT_count_leafs(cons_tree);
                double percent = size_t((leafs*1000)/species_count)/10.0;

                printf("Generated tree contains %.1f%% of species (%zu of %zu found in input trees)\n",
                       percent, leafs, species_count);

                if (savename) {
                    error = save_tree_as_newick(cons_tree, savename, comment);
                }
                else {
                    printf("sucessfully created consensus tree\n"
                           "(no savename specified -> tree not saved)\n");
                }
                delete cons_tree;
            }
            free(comment);
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

// #define TEST_AUTO_UPDATE // uncomment to update expected trees (if more than date differs)

static char *custom_tree_name(int dir, const char *name) { return GBS_global_string_copy("consense/%i/%s.tree", dir, name); }
static char *custom_numbered_tree_name(int dir, const char *name, int treeNr) { return GBS_global_string_copy("consense/%i/%s_%i.tree", dir, name, treeNr); }

static void add_inputnames(StrArray& to, int dir, const char *basename, int first_tree, int last_tree) {
    for (int t = first_tree; t <= last_tree; ++t) {
        to.put(custom_numbered_tree_name(dir, basename, t));
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

static arb_test::match_expectation consense_tree_generated(GBT_TREE *tree, GB_ERROR error, size_t species_count, size_t expected_species_count, double expected_intree_distance) {
    using namespace   arb_test;
    expectation_group expected;

    expected.add(that(error).is_equal_to_NULL());
    expected.add(that(tree).does_differ_from_NULL());

    if (tree) {
        expected.add(that(species_count).is_equal_to(expected_species_count));
        expected.add(that(GBT_count_leafs(tree)).is_equal_to(expected_species_count));
        expected.add(that(calc_intree_distance(tree)).fulfills(epsilon_similar(LENSUM_EPSILON), expected_intree_distance));
    }

    return all().ofgroup(expected);
}

static arb_test::match_expectation build_expected_consensus_tree(const int treedir, const char *basename, int first_tree, int last_tree, double weight, const char *outbasename, size_t expected_species_count, double expected_intree_distance) {
    using namespace       arb_test;
    expectation_group     expected;
    arb_suppress_progress hideProgress;

    GB_ERROR  error   = NULL;
    StrArray  input_tree_names;
    add_inputnames(input_tree_names, treedir, basename, first_tree, last_tree);

    size_t    species_count;
    char     *comment;
    GBT_TREE *tree = build_consensus_tree(input_tree_names, error, species_count, weight, comment);
    expected.add(consense_tree_generated(tree, error, species_count, expected_species_count, expected_intree_distance));

    char *saveas = custom_tree_name(treedir, outbasename);
    error        = save_tree_as_newick(tree, saveas, comment);
    expected.add(that(error).is_equal_to_NULL());

    if (!error) {
        char *expected_save        = custom_tree_name(treedir, GBS_global_string("%s_expected", outbasename));
        bool  exported_as_expected = arb_test::textfiles_have_difflines_ignoreDates(expected_save, saveas, 0);

#if defined(TEST_AUTO_UPDATE)
        if (!exported_as_expected) {
            ASSERT_RESULT(int, 0, system(GBS_global_string("cp %s %s", saveas, expected_save)));
        }
#else // !defined(TEST_AUTO_UPDATE)
        expected.add(that(exported_as_expected).is_equal_to(true));
#endif
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));
        free(expected_save);
    }

    free(saveas);
    free(comment);
    delete tree;

    return all().ofgroup(expected);
}

void TEST_consensus_tree_1() {
    TEST_EXPECTATION(build_expected_consensus_tree(1, "bootstrapped", 1, 5, 0.7, "consense1", 22, 0.925779));
    // ../UNIT_TESTER/run/consense/1/consense1.tree
}
void TEST_consensus_tree_1_single() {
    TEST_EXPECTATION(build_expected_consensus_tree(1, "bootstrapped", 1, 1, 0.01, "consense1_single", 22, 0.924610));
    // ../UNIT_TESTER/run/consense/1/consense1_single.tree
}

void TEST_consensus_tree_2() {
    TEST_EXPECTATION(build_expected_consensus_tree(2, "bootstrapped", 1, 4, 2.5, "consense2", 59, 2.849827));
    // ../UNIT_TESTER/run/consense/2/consense2.tree
}

void TEST_consensus_tree_3() {
    TEST_EXPECTATION(build_expected_consensus_tree(3, "bootstrapped", 1, 3, 137.772, "consense3", 128, 2.170685));
    // ../UNIT_TESTER/run/consense/3/consense3.tree
}

void TEST_consensus_tree_from_disjunct_trees() {
    TEST_EXPECTATION(build_expected_consensus_tree(4, "disjunct", 1, 2, 137.772, "disjunct_merged", 15, 2.034290));
    // ../UNIT_TESTER/run/consense/4/disjunct_merged.tree
}

void TEST_consensus_tree_from_partly_overlapping_trees() {
    // tree_disjunct_3 contains 7 species
    // (3 from upper subtree (tree_disjunct_1) and 4 from lower subtree (tree_disjunct_2))

    TEST_EXPECTATION(build_expected_consensus_tree(4, "disjunct", 1, 3, 137.772, "overlap_merged", 15, 2.596455));
    // ../UNIT_TESTER/run/consense/4/overlap_merged.tree
}

void TEST_consensus_tree_from_minimal_overlapping_trees() {
    // tree_disjunct_0 only contains 2 species (1 from upper and 1 from lower subtree).
    TEST_EXPECTATION(build_expected_consensus_tree(4, "disjunct", 0, 2, 137.772, "overlap_mini_merged", 15, 2.750745));
    // ../UNIT_TESTER/run/consense/4/overlap_mini_merged.tree
}

void TEST_consensus_tree_described_in_arbhelp() {
    // see ../HELP_SOURCE/oldhelp/consense_algo.hlp
    TEST_EXPECTATION(build_expected_consensus_tree(5, "help", 1, 2, 2.0, "help_merged", 6, 1.050000));
    // ../UNIT_TESTER/run/consense/5/help_merged.tree
}

void TEST_consensus_tree_from_trees_overlapping_by_twothirds() {
    // These 3 trees where copied from an existing tree.
    // From each copy one third of all species has been removed
    // (removed sets were disjunct)
    TEST_EXPECTATION(build_expected_consensus_tree(6, "overlap_two_thirds", 1, 3, 19.2, "overlap_twothirds_merged", 15, 3.561680));
    // ../UNIT_TESTER/run/consense/6/overlap_twothirds_merged.tree
}

void TEST_consensus_tree_from_mostly_overlapping_trees() {
    // the 3 trees were copied from tree_disjunct_source.
    // from each tree 2 (different) species were deleted.
    TEST_EXPECTATION(build_expected_consensus_tree(7, "disjunct_del2", 1, 3, 137.772, "overlap_mostly", 15, 1.820057));
    // ../UNIT_TESTER/run/consense/7/overlap_mostly.tree
}

void TEST_consensus_tree_from_mostly_overlapping_trees_2() {
    // the 3 trees were copied from tree_disjunct1
    // from each tree 1 (different) species was deleted.
    TEST_EXPECTATION(build_expected_consensus_tree(8, "overlap2", 1, 3, 137.772, "overlap2_mostly", 8, 0.529109));
    // ../UNIT_TESTER/run/consense/8/overlap2_mostly.tree
}



#define REPEATED_TESTS

#if defined(REPEATED_TESTS)
void TEST_consensus_tree_generation_is_deterministic() {
    TEST_consensus_tree_described_in_arbhelp();
    TEST_consensus_tree_from_minimal_overlapping_trees();
    TEST_consensus_tree_from_partly_overlapping_trees();
    TEST_consensus_tree_from_disjunct_trees();
    TEST_consensus_tree_3();
    TEST_consensus_tree_2();
    TEST_consensus_tree_1_single();
    TEST_consensus_tree_1();
}

void TEST_arb_consensus_tree() {
    TEST_STDOUT_CONTAINS("(arb_consensus_tree -x || true)", "Unknown switch '-x'");
    TEST_STDOUT_CONTAINS("(arb_consensus_tree -w sth || true)", "no input trees specified");

    {
        char *saveas   = custom_tree_name(1, "consense1");
        char *expected = custom_tree_name(1, "consense1_expected");
    
        TEST_STDOUT_CONTAINS("arb_consensus_tree"
                             " -w consense/1/consense1.tree"
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
        char *saveas   = custom_tree_name(2, "consense2");
        char *expected = custom_tree_name(2, "consense2_expected");
    
        TEST_STDOUT_CONTAINS("arb_consensus_tree"
                             " -w consense/2/consense2.tree"
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
#endif // REPEATED_TESTS

// #define TREEIO_AUTO_UPDATE // uncomment to auto-update expected test-results
// #define TREEIO_AUTO_UPDATE_IF_EXPORT_DIFFERS // uncomment to auto-update expected test-results
// #define TREEIO_AUTO_UPDATE_IF_REEXPORT_DIFFERS // uncomment to auto-update expected test-results

static const char *findFirstNameContaining(GBT_TREE *tree, const char *part) {
    const char *found = NULL;
    if (tree->name && strstr(tree->name, part)) {
        found = tree->name;
    }
    else if (!tree->is_leaf) {
        found             = findFirstNameContaining(tree->leftson, part);
        if (!found) found = findFirstNameContaining(tree->rightson, part);
    }
    return found;
}

void TEST_SLOW_treeIO_stable() {
    const char *dbname   = "trees/bootstrap_groups.arb";
    const char *treename = "tree_bootstrap_and_groups";
    const char *savename = "bg";

    GB_shell  shell;
    GBDATA   *gb_main = GB_open(dbname, "rw");

    TEST_REJECT_NULL(gb_main);

    GBT_TREE_NodeFactory nodeMaker;

    char *outfile = GBS_global_string_copy("trees/%s.tree", savename);

    for (int save_branchlengths = 0; save_branchlengths <= 1; ++save_branchlengths) {
        for (int save_bootstraps = 0; save_bootstraps <= 1; ++save_bootstraps) {
            for (int save_groupnames = 0; save_groupnames <= 1; ++save_groupnames) {
                bool quoting_occurs = save_bootstraps && save_groupnames;
                for (int pretty = 0; pretty <= 1; ++pretty) {

                    for (int quoting = TREE_DISALLOW_QUOTES; quoting <= (quoting_occurs ? TREE_DOUBLE_QUOTES : TREE_DISALLOW_QUOTES); ++quoting) {
                        TREE_node_quoting quoteMode = TREE_node_quoting(quoting);

                        char *paramID = GBS_global_string_copy("%s_%s%s%s_%i",
                                                               pretty ? "p" : "s",
                                                               save_bootstraps ? "Bs" : "",
                                                               save_groupnames ? "Grp" : "",
                                                               save_branchlengths ? "Len" : "",
                                                               quoteMode);

                        TEST_ANNOTATE(GBS_global_string("for paramID='%s'", paramID));

                        GB_ERROR export_error = TREE_write_Newick(gb_main, treename, NULL, save_branchlengths, save_bootstraps, save_groupnames, pretty, quoteMode, outfile);
                        TEST_EXPECT_NULL(export_error);

                        char *expectedfile = GBS_global_string_copy("trees/%s_exp_%s.tree", savename, paramID);

#if defined(TREEIO_AUTO_UPDATE)
                        system(GBS_global_string("cp %s %s", outfile, expectedfile));
#else // !defined(TREEIO_AUTO_UPDATE)
                        bool exported_as_expected = arb_test::textfiles_have_difflines_ignoreDates(expectedfile, outfile, 0);
#if defined(TREEIO_AUTO_UPDATE_IF_EXPORT_DIFFERS)
                        if (!exported_as_expected) {
                            system(GBS_global_string("cp %s %s", outfile, expectedfile));
                        }
#else // !defined(TREEIO_AUTO_UPDATE_IF_EXPORT_DIFFERS)
                        TEST_EXPECT(exported_as_expected);
#endif

                        // reimport exported tree
                        const char *reloaded_treename = "tree_reloaded";
                        {
                            char     *comment    = NULL;
                            GBT_TREE *tree       = TREE_load(expectedfile, nodeMaker, &comment, true, NULL);
                            GB_ERROR  load_error = tree ? NULL : GB_await_error();

                            TEST_EXPECTATION(all().of(that(tree).does_differ_from_NULL(),
                                                      that(load_error).is_equal_to_NULL()));
                            // store tree in DB
                            {
                                GB_transaction ta(gb_main);
                                GB_ERROR       store_error = GBT_write_tree_with_remark(gb_main, reloaded_treename, tree, comment);
                                TEST_EXPECT_NULL(store_error);
                            }
                            free(comment);

                            if (save_groupnames) {
                                const char *quotedGroup     = findFirstNameContaining(tree, "quoted");
                                const char *underscoreGroup = findFirstNameContaining(tree, "bs100");
                                TEST_EXPECT_EQUAL(quotedGroup, "quoted");
                                TEST_EXPECT_EQUAL(underscoreGroup, "__bs100");
                            }
                            const char *capsLeaf = findFirstNameContaining(tree, "Caps");
                            TEST_EXPECT_EQUAL(capsLeaf, "_MhuCaps");

                            delete tree;
                        }

                        // export again
                        GB_ERROR reexport_error = TREE_write_Newick(gb_main, reloaded_treename, NULL, save_branchlengths, save_bootstraps, save_groupnames, pretty, quoteMode, outfile);
                        TEST_EXPECT_NULL(reexport_error);

                        // eliminate comments added by loading/saving
                        char *outfile2 = GBS_global_string_copy("trees/%s2.tree", savename);
                        {
                            char *cmd = GBS_global_string_copy("cat %s"
                                                               " | grep -v 'Loaded from trees/.*_exp_'"
                                                               " | grep -v 'tree_reloaded saved to'"
                                                               " > %s", outfile, outfile2);
                            TEST_EXPECT_NO_ERROR(GBK_system(cmd));
                            free(cmd);
                        }

                        bool reexported_as_expected = arb_test::textfiles_have_difflines(expectedfile, outfile2, 0);

#if defined(TREEIO_AUTO_UPDATE_IF_REEXPORT_DIFFERS)
                        if (!reexported_as_expected) {
                            system(GBS_global_string("cp %s %s", outfile2, expectedfile));
                        }
#else // !defined(TREEIO_AUTO_UPDATE_IF_REEXPORT_DIFFERS)
                        TEST_EXPECT(reexported_as_expected);
#endif

                        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink(outfile2));
                        free(outfile2);
#endif
                        free(expectedfile);
                        free(paramID);
                    }
                }
            }
        }
    }
    TEST_ANNOTATE(NULL);

    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink(outfile));
    free(outfile);

    GB_close(gb_main);
}

void TEST_CONSENSUS_TREE_functionality() {
    // functionality wanted in RootedTree (for use in library CONSENSUS_TREE)

    char *comment = NULL;

    SizeAwareTree *tree = DOWNCAST(SizeAwareTree*, TREE_load("trees/bg_exp_p_GrpLen_0.tree",
                                                             *new TreeRoot(new SizeAwareNodeFactory, true),
                                                             &comment, false, NULL));
    // -> ../UNIT_TESTER/run/trees/bg_exp_p_GrpLen_0.tree

#define ORG_1111 "(AticSea6,(RblAerol,RblMesop))"
#define TOP_1111 "((RblAerol,RblMesop),AticSea6)"
#define BOT_1111 ORG_1111

#define ORG_11121 "((DnrShiba,RsbElon4),MmbAlkal)"
#define TOP_11121 ORG_11121
#define BOT_11121 "(MmbAlkal,(DnrShiba,RsbElon4))"

#define ORG_11122 "((MabPelag,MabSalin),PaoMaris)"
#define TOP_11122 ORG_11122
#define BOT_11122 "(PaoMaris,(MabPelag,MabSalin))"

#define ORG_1112 "(" ORG_11121 "," ORG_11122 ")"
#define TOP_1112 "(" TOP_11121 "," TOP_11122 ")"
#define BOT_1112 "(" BOT_11121 "," BOT_11122 ")"
#define EDG_1112 "(" TOP_11121 "," BOT_11122 ")"

#define ORG_111 "(" ORG_1111 "," ORG_1112 ")"
#define TOP_111 "(" TOP_1112 "," TOP_1111 ")"
#define BOT_111 "(" BOT_1111 "," BOT_1112 ")"
#define EDG_111 "(" EDG_1112 "," BOT_1111 ")"

#define ORG_112 "(OnlGran2,RsnAnta2)"
#define TOP_112 ORG_112
#define BOT_112 ORG_112

#define ORG_11 "(" ORG_111 "," ORG_112 ")"
#define TOP_11 "(" TOP_111 "," TOP_112 ")"
#define BOT_11 "(" BOT_112 "," BOT_111 ")"
#define EDG_11 "(" EDG_111 "," BOT_112 ")"

#define ORG_12 "(_MhuCaps,ThtNivea)"
#define TOP_12 "(ThtNivea,_MhuCaps)"
#define BOT_12 TOP_12

#define ORG_1 "(" ORG_11 "," ORG_12 ")"
#define TOP_1 "(" TOP_11 "," TOP_12 ")"
#define BOT_1 "(" BOT_12 "," BOT_11 ")"
#define EDG_1 "(" EDG_11 "," BOT_12 ")"

#define ORG_2 "((LbnMarin,LbnzAlb4),LbnAlexa)"
#define TOP_2 ORG_2
#define BOT_2 "(LbnAlexa,(LbnMarin,LbnzAlb4))"

    // test swap_sons
    TEST_ASSERT_VALID_TREE(tree);
    TEST_EXPECT_NEWICK(nSIMPLE, tree, "(" ORG_1 "," ORG_2 ");");
    tree->swap_sons();
    TEST_EXPECT_NEWICK(nSIMPLE, tree, "(" ORG_2 "," ORG_1 ");");

    // test reorder_tree
    TEST_ASSERT_VALID_TREE(tree);
    TreeOrder order[] = { BIG_BRANCHES_TO_TOP, BIG_BRANCHES_TO_BOTTOM, BIG_BRANCHES_TO_EDGE };

    for (size_t o1 = 0; o1<ARRAY_ELEMS(order); ++o1) {
        TreeOrder to_order = order[o1];
        for (size_t o2 = 0; o2<ARRAY_ELEMS(order); ++o2) {
            TreeOrder from_order = order[o2];

            for (int rotate = 0; rotate<=1; ++rotate) {
                tree->reorder_tree(from_order);
                if (rotate) tree->rotate_subtree();
                tree->reorder_tree(to_order);

                switch (to_order) {
                    case BIG_BRANCHES_TO_TOP:    TEST_EXPECT_NEWICK(nSIMPLE, tree, "(" TOP_1 "," TOP_2 ");"); break;
                    case BIG_BRANCHES_TO_EDGE:   TEST_EXPECT_NEWICK(nSIMPLE, tree, "(" EDG_1 "," BOT_2 ");"); break;
                    case BIG_BRANCHES_TO_BOTTOM: TEST_EXPECT_NEWICK(nSIMPLE, tree, "(" BOT_2 "," BOT_1 ");"); break;
                    default: TEST_REJECT(true); break;
                }

            }
        }
    }

    // test rotate_subtree
    TEST_ASSERT_VALID_TREE(tree);
    tree->reorder_tree(BIG_BRANCHES_TO_TOP);
    tree->rotate_subtree(); TEST_EXPECT_NEWICK(nSIMPLE, tree, "((LbnAlexa,(LbnzAlb4,LbnMarin)),((_MhuCaps,ThtNivea),((RsnAnta2,OnlGran2),((AticSea6,(RblMesop,RblAerol)),((PaoMaris,(MabSalin,MabPelag)),(MmbAlkal,(RsbElon4,DnrShiba)))))));");
    tree->rotate_subtree(); TEST_EXPECT_NEWICK(nSIMPLE, tree, "(" TOP_1 "," TOP_2 ");");


    // test set_root
    TEST_ASSERT_VALID_TREE(tree);
    RootedTree *AticSea6Grandpa = tree->findLeafNamed("AticSea6")->get_father()->get_father();
    TEST_REJECT_NULL(AticSea6Grandpa);
    TEST_ASSERT_VALID_TREE(AticSea6Grandpa);

    AticSea6Grandpa->set_root();
    TEST_EXPECT_NEWICK(nSIMPLE, tree,
                             "((" ORG_1112 "," TOP_1111 ")," // AticSea6 is direct son of TOP_1111
                             "((" ORG_2 "," TOP_12 ")," ORG_112 "));");

    // test auto-detection of "best" root
    TEST_ASSERT_VALID_TREE(tree);
    tree->get_tree_root()->find_innermost_edge().set_root();
    TEST_EXPECT_NEWICK(nLENGTH, tree,
                             "((((LbnMarin:0.019,LbnzAlb4:0.003):0.016,LbnAlexa:0.032):0.122,(ThtNivea:0.230,_MhuCaps:0.194):0.427):0.076,"
                             "(((((DnrShiba:0.076,RsbElon4:0.053):0.034,MmbAlkal:0.069):0.016,((MabPelag:0.001,MabSalin:0.009):0.095,PaoMaris:0.092):0.036):0.030,((RblAerol:0.085,RblMesop:0.042):0.238,AticSea6:0.111):0.018):0.036,(OnlGran2:0.057,RsnAnta2:0.060):0.021):0.076);");

    TEST_ASSERT_VALID_TREE(tree);
    delete tree;
    free(comment);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
