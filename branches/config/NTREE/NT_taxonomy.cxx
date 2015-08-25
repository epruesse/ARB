// =========================================================== //
//                                                             //
//   File      : NT_taxonomy.cxx                               //
//   Purpose   : Compare two trees by taxonomy                 //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2015   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#include "ad_trees.h"
#include "NT_local.h"

#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <awt_sel_boxes.hxx>
#include <TreeCallbacks.hxx>
#include <arb_progress.h>
#include <item_sel_list.h>


#include <set>
#include <map>

#define TREE_COMPARE_PREFIX     "ad_tree/compare/"
#define TREE_COMPARE_PREFIX_TMP "tmp/" TREE_COMPARE_PREFIX

#define AWAR_TREE_COMPARE_LEFT   TREE_COMPARE_PREFIX_TMP "left"
#define AWAR_TREE_COMPARE_RIGHT  TREE_COMPARE_PREFIX_TMP "right"

#define AWAR_TREE_COMPARE_ACTION         TREE_COMPARE_PREFIX "action"
#define AWAR_TREE_COMPARE_MIN_TAX_LEVELS TREE_COMPARE_PREFIX "taxdist"
#define AWAR_TREE_COMPARE_WRITE_FIELD    TREE_COMPARE_PREFIX "field"

enum Action { // uses same values as NT_mark_all_cb; see ../SL/TREEDISP/TreeCallbacks.cxx@MARK_MODE_LOWER_BITS
    UNMARK = 0,
    MARK   = 1,
    INVERT = 2,
};

enum Target {
    ALL,
    TAX,
    COMMON,
    MISSING_LEFT,
    MISSING_RIGHT,
};

static TreeNode *findParentGroup(TreeNode *node) {
    TreeNode *parent_group = NULL;

    while (!parent_group && node->father) {
        node = node->father;
        if (node->is_named_group()) {
            parent_group = node;
        }
    }

    return parent_group;
}

static int countTaxLevel(TreeNode *node) {
    int       taxlevel     = node->is_leaf ? 0 : node->is_named_group();
    TreeNode *parent_group = findParentGroup(node);
    while (parent_group) {
        ++taxlevel;
        parent_group = findParentGroup(parent_group);
    }
    return taxlevel;
}

static int calcTaxDifference(TreeNode *g1, TreeNode *g2) {
    // returns difference in taxonomy-levels
    //
    // difference defined such that:
    //   diff("/A/B", "/A/B") == 0
    //   diff("/A/B", "/A"  ) == 1
    //   diff("/A/B", "/A/C") == 2
    //   diff("/A/B/C", "/A/D/E") == 4
    //   diff("/A/B/C", "/A/D/C") == 4
    //   diff("/A/B/C", "/A/C") == 3

    nt_assert(g1->is_named_group() && g2->is_named_group()); // has to be called with root-nodes of groups!

    TreeNode *p1 = findParentGroup(g1);
    TreeNode *p2 = findParentGroup(g2);

    int taxdiff = 0;
    if (p1) {
        if (p2) {
            int pdiff  = calcTaxDifference(p1, p2);
            int p1diff = calcTaxDifference(p1, g2) + 1;
            int p2diff = calcTaxDifference(g1, p2) + 1;

            if (pdiff || strcmp(g1->name, g2->name) != 0) {
                // if parent-taxonomy differs -> ignore names of g1/g2 -> diff=2
                // if parent-taxonomy matches -> if names of g1 and g2 match -> no diff
                //                               if names of g1 and g2 differ -> diff=2
                pdiff += 2;
            }

            taxdiff = std::min(pdiff, std::min(p1diff, p2diff));
        }
        else {
            taxdiff = calcTaxDifference(p1, g2) + 1;
        }
    }
    else {
        if (p2) {
            taxdiff = calcTaxDifference(g1, p2) + 1;
        }
        else {
            if (strcmp(g1->name, g2->name) != 0) { // logic similar to (p1 && p2) above
                taxdiff = 2;
            }
        }
    }

    return taxdiff;
}

class SpeciesInTwoTrees {
    TreeNode *tree1;
    TreeNode *tree2;

public:
    SpeciesInTwoTrees()
        : tree1(NULL),
          tree2(NULL)
    {}

    void setSpecies(TreeNode *species, bool first) {
        nt_assert(species->is_leaf);
        if (first) {
            nt_assert(!tree1);
            tree1 = species;
        }
        else {
            nt_assert(!tree2);
            tree2 = species;
        }
    }

    bool occursInBothTrees() const { return tree1 && tree2; }
    int calcTaxDiffLevel() const {
        TreeNode *parent_group1 = findParentGroup(tree1);
        TreeNode *parent_group2 = findParentGroup(tree2);

        int taxdiff = 0;

        if (parent_group1) {
            if (parent_group2) {
                taxdiff = calcTaxDifference(parent_group1, parent_group2);
            }
            else {
                taxdiff = countTaxLevel(parent_group1);
            }
        }
        else {
            if (parent_group2) {
                taxdiff = countTaxLevel(parent_group2);
            }
            // else -> both outside any group -> no diff
        }

        return taxdiff;
    }
};

typedef std::set<const char *, charpLess> NameSet;
typedef std::map<const char *, SpeciesInTwoTrees, charpLess> TwoTreeMap;

static void mapTree(TreeNode *node, TwoTreeMap& tmap, bool first) {
    if (node->is_leaf) {
        nt_assert(node->name);
        tmap[node->name].setSpecies(node, first);
    }
    else {
        mapTree(node->leftson, tmap, first);
        mapTree(node->rightson, tmap, first);
    }
}

static void mark_action(AW_window *aws, AWT_canvas *ntw, Target target) {
    AW_root *aw_root = aws->get_root();

    Action action = Action(aw_root->awar(AWAR_TREE_COMPARE_ACTION)->read_int());

    arb_progress progress("Mark species");
    if (target == ALL) {
        NT_mark_all_cb(NULL, ntw, action);
    }
    else {
        progress.subtitle("Loading trees");

        const char *treename_left  = aw_root->awar(AWAR_TREE_COMPARE_LEFT)->read_char_pntr();
        const char *treename_right = aw_root->awar(AWAR_TREE_COMPARE_RIGHT)->read_char_pntr();

        GBDATA         *gb_main = ntw->gb_main;
        GB_transaction  ta(gb_main);

        GBDATA *gb_species_data = GBT_get_species_data(gb_main);

        TreeNode *tree_left  = GBT_read_tree(gb_main, treename_left,  new SimpleRoot);
        TreeNode *tree_right = GBT_read_tree(gb_main, treename_right, new SimpleRoot);

        size_t missing   = 0;
        size_t targetted = 0;

        if (target == TAX) {
            int min_tax_levels = atoi(aw_root->awar(AWAR_TREE_COMPARE_MIN_TAX_LEVELS)->read_char_pntr());

            TwoTreeMap tmap;
            mapTree(tree_left,  tmap, true);
            mapTree(tree_right, tmap, false);

            size_t commonSpeciesCount = 0;
            for (TwoTreeMap::iterator s = tmap.begin(); s != tmap.end(); ++s) {
                if (s->second.occursInBothTrees()) commonSpeciesCount++;
            }

            const char *fieldName    = aw_root->awar(AWAR_TREE_COMPARE_WRITE_FIELD)->read_char_pntr();
            bool        writeToField = fieldName[0];

            arb_progress subprogress("Comparing taxonomy info", commonSpeciesCount);
            GB_ERROR     error = NULL;

            for (TwoTreeMap::iterator s = tmap.begin(); s != tmap.end() && !error; ++s) {
                const SpeciesInTwoTrees& species = s->second;

                if (species.occursInBothTrees()) {
                    int taxDiffLevel = species.calcTaxDiffLevel();
                    if (taxDiffLevel>min_tax_levels) {
                        ++targetted;
                        GBDATA *gb_species = GBT_find_species_rel_species_data(gb_species_data, s->first);
                        if (!gb_species) {
                            ++missing;
                        }
                        else {
                            switch (action) {
                                case UNMARK: GB_write_flag(gb_species, 0);                         break;
                                case MARK:   GB_write_flag(gb_species, 1);                         break;
                                case INVERT: GB_write_flag(gb_species, !GB_read_flag(gb_species)); break;
                            }
                            if (writeToField) {
                                error = GBT_write_int(gb_species, fieldName, taxDiffLevel);
                            }
                        }
                    }
                    subprogress.inc_and_check_user_abort(error);
                }
            }
            aw_message_if(error);
        }
        else {
            progress.subtitle("Intersecting tree members");

            NameSet in_left;
            NameSet in_right;
            {
                size_t   count_left, count_right;
                GB_CSTR *names_left  = GBT_get_names_of_species_in_tree(tree_left,  &count_left);
                GB_CSTR *names_right = GBT_get_names_of_species_in_tree(tree_right, &count_right);

                for(size_t i= 0; i<count_left;  ++i) in_left .insert(names_left[i]);
                for(size_t i= 0; i<count_right; ++i) in_right.insert(names_right[i]);

                free(names_right);
                free(names_left);
            }

            {
                NameSet& in_one   = target == MISSING_LEFT ? in_right : in_left;
                NameSet& in_other = target == MISSING_LEFT ? in_left : in_right;

                for (NameSet::const_iterator i = in_one.begin(); i != in_one.end(); ++i) {
                    bool is_in_other = in_other.find(*i) != in_other.end();
                    bool is_target   = is_in_other == (target == COMMON);

                    if (is_target) {
                        ++targetted;
                        GBDATA *gb_species = GBT_find_species_rel_species_data(gb_species_data, *i);
                        if (!gb_species) {
                            ++missing;
                        }
                        else {
                            switch (action) {
                                case UNMARK: GB_write_flag(gb_species, 0);                         break;
                                case MARK:   GB_write_flag(gb_species, 1);                         break;
                                case INVERT: GB_write_flag(gb_species, !GB_read_flag(gb_species)); break;
                            }
                        }
                    }
                }
            }
        }

        if (!targetted) {
            aw_message("Warning: no species targetted");
        }
        else if (missing) {
            aw_message(GBS_global_string("Warning: %zu targeted species could not be found\n"
                                         "(might be caused by zombies in your trees)", missing));
        }

        destroy(tree_right);
        destroy(tree_left);
    }
}

void NT_create_compare_taxonomy_awars(AW_root *aw_root, AW_default props) {
    char *currTree = aw_root->awar(AWAR_TREE_NAME)->read_string();

    aw_root->awar_string(AWAR_TREE_COMPARE_LEFT,           currTree, props);
    aw_root->awar_string(AWAR_TREE_COMPARE_RIGHT,          currTree, props);
    aw_root->awar_int   (AWAR_TREE_COMPARE_ACTION,         MARK,     props);
    aw_root->awar_string(AWAR_TREE_COMPARE_MIN_TAX_LEVELS, "0",      props);
    aw_root->awar_string(AWAR_TREE_COMPARE_WRITE_FIELD,    "",      props);

    free(currTree);
}

AW_window *NT_create_compare_taxonomy_window(AW_root *aw_root, AWT_canvas *ntw) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "COMPARE_TAXONOMY", "Compare taxonomy");
    aws->load_xfig("compare_taxonomy.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("compare_taxonomy.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("action");
    aws->create_toggle_field(AWAR_TREE_COMPARE_ACTION, NULL, "");
    aws->insert_default_toggle("mark",   "m", MARK);
    aws->insert_toggle        ("unmark", "u", UNMARK);
    aws->insert_toggle        ("invert", "i", INVERT);
    aws->update_toggle_field();

    aws->at("all");       aws->callback(makeWindowCallback(mark_action, ntw, ALL));           aws->create_autosize_button("all",       "all species");
    aws->at("tax");       aws->callback(makeWindowCallback(mark_action, ntw, TAX));           aws->create_autosize_button("tax",       "species with taxonomy changed");
    aws->at("common");    aws->callback(makeWindowCallback(mark_action, ntw, COMMON));        aws->create_autosize_button("common",    "common species");
    aws->at("missleft");  aws->callback(makeWindowCallback(mark_action, ntw, MISSING_LEFT));  aws->create_autosize_button("missleft",  "species missing in left tree");
    aws->at("missright"); aws->callback(makeWindowCallback(mark_action, ntw, MISSING_RIGHT)); aws->create_autosize_button("missright", "species missing in right tree");

    aws->at("levels");
    aws->create_input_field(AWAR_TREE_COMPARE_MIN_TAX_LEVELS, 3);

    create_selection_list_on_itemfields(ntw->gb_main, aws, AWAR_TREE_COMPARE_WRITE_FIELD, true, 1<<GB_INT, "field", NULL, SPECIES_get_selector(), 20, 30, SF_STANDARD, "choose_field");

    aws->at("left");
    awt_create_TREE_selection_list(ntw->gb_main, aws, AWAR_TREE_COMPARE_LEFT, true);

    aws->at("right");
    awt_create_TREE_selection_list(ntw->gb_main, aws, AWAR_TREE_COMPARE_RIGHT, true);

    return aws;
}


