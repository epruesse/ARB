// =============================================================== //
//                                                                 //
//   File      : NT_sort.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"

#include <item_sel_list.h>
#include <aw_awar.hxx>
#include <arb_progress.h>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <TreeNode.h>
#include <arb_sort.h>

#define FIELD_FILTER_RESORT (1<<GB_STRING)|(1<<GB_INT)|(1<<GB_FLOAT) // field types supported by cmpByKey()

#define CUSTOM_CRITERIA 3

struct customCriterion {
    char *key;
    bool  reverse;

    customCriterion() : key(NULL), reverse(false) {}
    customCriterion(const char *key_, bool reverse_) : key(strdup(key_)), reverse(reverse_) {}
    customCriterion(const customCriterion& other) : key(strdup(other.key)), reverse(other.reverse) {}
    DECLARE_ASSIGNMENT_OPERATOR(customCriterion);
    ~customCriterion() { free(key); }
};

static int cmpByKey(GBDATA *gbd1, GBDATA *gbd2, const customCriterion& by) {
    GBDATA *gb_field1 = GB_entry(gbd1, by.key);
    GBDATA *gb_field2 = GB_entry(gbd2, by.key);

    int cmp;
    if (gb_field1) {
        if (gb_field2) {
            switch (GB_read_type(gb_field1)) {
                case GB_STRING: {
                    const char *s1 = GB_read_char_pntr(gb_field1);
                    const char *s2 = GB_read_char_pntr(gb_field2);

                    cmp = strcmp(s1, s2);
                    break;
                }
                case GB_FLOAT: {
                    float f1 = GB_read_float(gb_field1);
                    float f2 = GB_read_float(gb_field2);

                    cmp = f1<f2 ? -1 : (f1>f2 ? 1 : 0);
                    break;
                }
                case GB_INT: {
                    int i1 = GB_read_int(gb_field1);
                    int i2 = GB_read_int(gb_field2);

                    cmp = i1-i2;
                    break;
                }
                default:
                    cmp = 0; // other field type -> no idea how to compare
                    break;
            }

            if (by.reverse) cmp = -cmp;
        }
        else cmp = -1;           // existing < missing!
    }
    else cmp = gb_field2 ? 1 : 0;

    return cmp;
}

static int resort_data_by_customOrder(const void *v1, const void *v2, void *cd_sortBy) {
    GBDATA *gbd1 = (GBDATA*)v1;
    GBDATA *gbd2 = (GBDATA*)v2;

    const customCriterion *sortBy = (const customCriterion *)cd_sortBy;

    int cmp = 0;
    for (int c = 0; !cmp && c<CUSTOM_CRITERIA; ++c) {
        cmp = cmpByKey(gbd1, gbd2, sortBy[c]);
    }
    return cmp;
}


static GBDATA **gb_resort_data_list;
static long    gb_resort_data_count;

static void NT_resort_data_base_by_tree(TreeNode *tree, GBDATA *gb_species_data) {
    if (tree) {
        if (tree->is_leaf) {
            if (tree->gb_node) {
                gb_resort_data_list[gb_resort_data_count++] = tree->gb_node;
            }
        }
        else {
            NT_resort_data_base_by_tree(tree->get_leftson(), gb_species_data);
            NT_resort_data_base_by_tree(tree->get_rightson(), gb_species_data);
        }
    }
}


static GB_ERROR NT_resort_data_base(TreeNode *tree, const customCriterion *sortBy) {
    nt_assert(contradicted(tree, sortBy));

    GB_ERROR error = GB_begin_transaction(GLOBAL.gb_main);
    if (!error) {
        GBDATA *gb_sd     = GBT_get_species_data(GLOBAL.gb_main);
        if (!gb_sd) error = GB_await_error();
        else {
            if (tree) {
                gb_resort_data_count = 0;
                gb_resort_data_list  = (GBDATA **)calloc(sizeof(GBDATA *), GB_nsons(gb_sd) + 256);
                NT_resort_data_base_by_tree(tree, gb_sd);
            }
            else {
                gb_resort_data_list = GBT_gen_species_array(GLOBAL.gb_main, &gb_resort_data_count);
                GB_sort((void **)gb_resort_data_list, 0, gb_resort_data_count, resort_data_by_customOrder, (void*)sortBy);

            }
            error = GB_resort_data_base(GLOBAL.gb_main, gb_resort_data_list, gb_resort_data_count);
            free(gb_resort_data_list);
        }
    }
    return GB_end_transaction(GLOBAL.gb_main, error);
}

void NT_resort_data_by_phylogeny(AW_window*, AWT_canvas *ntw) {
    arb_progress  progress("Sorting data");
    GB_ERROR      error = 0;
    TreeNode     *tree  = NT_get_tree_root_of_canvas(ntw);

    if (!tree)  error = "Please select/build a tree first";
    if (!error) error = NT_resort_data_base(tree, NULL);
    if (error) aw_message(error);
}

#define AWAR_TREE_SORT1 "ad_tree/sort_1"
#define AWAR_TREE_SORT2 "ad_tree/sort_2"
#define AWAR_TREE_SORT3 "ad_tree/sort_3"

#define AWAR_TREE_REV1 "ad_tree/rev1"
#define AWAR_TREE_REV2 "ad_tree/rev2"
#define AWAR_TREE_REV3 "ad_tree/rev3"

static void NT_resort_data_by_user_criteria(AW_window *aw) {
    arb_progress progress("Sorting data");

    AW_root *aw_root = aw->get_root();

    customCriterion sortBy[CUSTOM_CRITERIA];
    sortBy[0] = customCriterion(aw_root->awar(AWAR_TREE_SORT1)->read_char_pntr(), aw_root->awar(AWAR_TREE_REV1)->read_int());
    sortBy[1] = customCriterion(aw_root->awar(AWAR_TREE_SORT2)->read_char_pntr(), aw_root->awar(AWAR_TREE_REV2)->read_int());
    sortBy[2] = customCriterion(aw_root->awar(AWAR_TREE_SORT3)->read_char_pntr(), aw_root->awar(AWAR_TREE_REV3)->read_int());

    GB_ERROR error = NT_resort_data_base(NULL, sortBy);
    if (error) aw_message(error);
}

void NT_create_resort_awars(AW_root *awr, AW_default aw_def) {
    awr->awar_string(AWAR_TREE_SORT1, "name",      aw_def);
    awr->awar_string(AWAR_TREE_SORT2, "full_name", aw_def);
    awr->awar_string(AWAR_TREE_SORT3, "",          aw_def);

    awr->awar_int(AWAR_TREE_REV1, 0, aw_def);
    awr->awar_int(AWAR_TREE_REV2, 0, aw_def);
    awr->awar_int(AWAR_TREE_REV3, 0, aw_def);
}

AW_window *NT_create_resort_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(awr, "SORT_DATABASE", "SORT DATABASE");
    aws->load_xfig("nt_sort.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("sp_sort_fld.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    create_selection_list_on_itemfields(GLOBAL.gb_main, aws, AWAR_TREE_SORT1, true, SPECIES_get_selector(), FIELD_FILTER_RESORT, SF_STANDARD, "key1", 20, 10, NULL);
    create_selection_list_on_itemfields(GLOBAL.gb_main, aws, AWAR_TREE_SORT2, true, SPECIES_get_selector(), FIELD_FILTER_RESORT, SF_STANDARD, "key2", 20, 10, NULL);
    create_selection_list_on_itemfields(GLOBAL.gb_main, aws, AWAR_TREE_SORT3, true, SPECIES_get_selector(), FIELD_FILTER_RESORT, SF_STANDARD, "key3", 20, 10, NULL);

    aws->at("rev1"); aws->label("Reverse"); aws->create_toggle(AWAR_TREE_REV1);
    aws->at("rev2"); aws->label("Reverse"); aws->create_toggle(AWAR_TREE_REV2);
    aws->at("rev3"); aws->label("Reverse"); aws->create_toggle(AWAR_TREE_REV3);

    aws->at("go");
    aws->callback(NT_resort_data_by_user_criteria);
    aws->create_button("GO", "GO", "G");

    return aws;
}
