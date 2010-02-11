// =============================================================== //
//                                                                 //
//   File      : NT_sort.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ntree.hxx"
#include "nt_cb.hxx"

#include <awt_item_sel_list.hxx>
#include <awt.hxx>

#define NT_RESORT_FILTER (1<<GB_STRING)|(1<<GB_INT)|(1<<GB_FLOAT)

// test

struct customsort_struct {
    const char *key1;
    const char *key2;
    const char *key3;
};

static int cmpByKey(GBDATA *gbd1, GBDATA *gbd2, const char *field) {
    GBDATA *gb_field1 = GB_entry(gbd1, field);
    GBDATA *gb_field2 = GB_entry(gbd2, field);

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
                    double d1 = GB_read_float(gb_field1);
                    double d2 = GB_read_float(gb_field2);

                    cmp = d1<d2 ? -1 : (d1>d2 ? 1 : 0);
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
        }
        else cmp = -1;           // existing < missing!
    }
    else cmp = gb_field2 ? 1 : 0;

    return cmp;
}

static int resort_data_by_customsub(const void *v1, const void *v2, void *cd_sortBy) {
    GBDATA            *gbd1   = (GBDATA*)v1;
    GBDATA            *gbd2   = (GBDATA*)v2;
    customsort_struct *sortBy = (customsort_struct*)cd_sortBy;

    int cmp = cmpByKey(gbd1, gbd2, sortBy->key1);
    if (!cmp) {
        cmp = cmpByKey(gbd1, gbd2, sortBy->key2);
        if (!cmp) {
            cmp = cmpByKey(gbd1, gbd2, sortBy->key3);
        }
    }
    return cmp;
}


static GBDATA **gb_resort_data_list;
static long    gb_resort_data_count;

static void NT_resort_data_base_by_tree(GBT_TREE *tree, GBDATA *gb_species_data) {
    if (tree) {
        if (tree->is_leaf) {
            if (tree->gb_node) {
                gb_resort_data_list[gb_resort_data_count++] = tree->gb_node;
            }
        }
        else {
            NT_resort_data_base_by_tree(tree->leftson, gb_species_data);
            NT_resort_data_base_by_tree(tree->rightson, gb_species_data);
        }
    }
}


GB_ERROR NT_resort_data_base(GBT_TREE *tree, const char *key1, const char *key2, const char *key3) {
    customsort_struct sortBy;

    sortBy.key1 = key1;
    sortBy.key2 = key2;
    sortBy.key3 = key3;

    GB_ERROR error = GB_begin_transaction(GLOBAL_gb_main);
    if (!error) {
        GBDATA *gb_sd     = GB_search(GLOBAL_gb_main, "species_data", GB_CREATE_CONTAINER);
        if (!gb_sd) error = GB_await_error();
        else {
            if (tree) {
                gb_resort_data_count = 0;
                gb_resort_data_list = (GBDATA **)calloc(sizeof(GBDATA *), GB_nsons(gb_sd) + 256);
                NT_resort_data_base_by_tree(tree, gb_sd);
            }
            else {
                gb_resort_data_list = GBT_gen_species_array(GLOBAL_gb_main, &gb_resort_data_count);
                GB_sort((void **)gb_resort_data_list, 0, gb_resort_data_count, resort_data_by_customsub, &sortBy);

            }
            error = GB_resort_data_base(GLOBAL_gb_main, gb_resort_data_list, gb_resort_data_count);
            free(gb_resort_data_list);
        }
    }
    return GB_end_transaction(GLOBAL_gb_main, error);
}

void NT_resort_data_by_phylogeny(AW_window *, AW_CL, AW_CL) {
    aw_openstatus("resorting data");
    GB_ERROR error = 0;
    GBT_TREE *tree = nt_get_current_tree_root();
    if (!tree) error = "Please select/build a tree first";
    if (!error) error = NT_resort_data_base(tree, 0, 0, 0);
    aw_closestatus();
    if (error) aw_message(error);

}

void NT_resort_data_by_user_criteria(AW_window *aw) {
    aw_openstatus("resorting data");
    GB_ERROR error = 0;
    char *s1 = aw->get_root()->awar("ad_tree/sort_1")->read_string();
    char *s2 = aw->get_root()->awar("ad_tree/sort_2")->read_string();
    char *s3 = aw->get_root()->awar("ad_tree/sort_3")->read_string();
    if (!error) error = NT_resort_data_base(0, s1, s2, s3);
    aw_closestatus();
    if (error) aw_message(error);
}

void NT_build_resort_awars(AW_root *awr, AW_default aw_def) {
    awr->awar_string("ad_tree/sort_1", "name",      aw_def);
    awr->awar_string("ad_tree/sort_2", "name",      aw_def);
    awr->awar_string("ad_tree/sort_3", "name",      aw_def);
}

AW_window *NT_build_resort_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(awr, "SORT_DATABASE", "SORT DATABASE");
    aws->load_xfig("nt_sort.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"sp_sort_fld.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("go");
    aws->callback((AW_CB0)NT_resort_data_by_user_criteria);
    aws->create_button("GO", "GO", "G");

    awt_create_selection_list_on_itemfields(GLOBAL_gb_main,
                                            aws, "ad_tree/sort_1",
                                            NT_RESORT_FILTER,
                                            "key1", 0, &AWT_species_selector, 20, 10);

    awt_create_selection_list_on_itemfields(GLOBAL_gb_main,
                                            aws, "ad_tree/sort_2",
                                            NT_RESORT_FILTER,
                                            "key2", 0, &AWT_species_selector, 20, 10);

    awt_create_selection_list_on_itemfields(GLOBAL_gb_main,
                                            aws, "ad_tree/sort_3",
                                            NT_RESORT_FILTER,
                                            "key3", 0, &AWT_species_selector, 20, 10);

    return (AW_window *)aws;

}
