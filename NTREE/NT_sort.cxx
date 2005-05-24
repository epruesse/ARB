#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include <awt_changekey.hxx>

#include "nt_sort.hxx"

// test

struct customsort_struct {
    char *key1;
    char *key2;
    char *key3;
} custom_s_struct;

int
resort_data_by_customsub(GBDATA *first, GBDATA *second)
{
    GBDATA  *gb_0,*gb_1;
    char    *s0,*s1;
    double  f0,f1;
    long    i0,i1;
    int cmp;

    /* first key */
    gb_0 = GB_find(first,custom_s_struct.key1,0,down_level);
    gb_1 = GB_find(second,custom_s_struct.key1,0,down_level);
    if (gb_0 && !gb_1) return -1;
    if (!gb_0 && gb_1) return 1;
    if (gb_0 && gb_1) {
        switch (GB_read_type(gb_0)) {
            case GB_STRING:
                s0 = GB_read_string(gb_0);
                s1 = GB_read_string(gb_1);
                cmp = strcmp(s0,s1);
                free(s0);
                free(s1);
                if (cmp) return cmp;
                break;
            case GB_FLOAT:
                f0 = GB_read_float(gb_0);
                f1 = GB_read_float(gb_1);
                if (f0<f1) return -1;
                if (f1<f0) return 1;
                break;
            case GB_INT:
                i0 = GB_read_int(gb_0);
                i1 = GB_read_int(gb_1);
                if (i0<i1) return -1;
                if (i1<i0) return 1;
                break;
            default:
                break;
        }

    }
    /* second key */
    gb_0 = GB_find(first,custom_s_struct.key2,0,down_level);
    gb_1 = GB_find(second,custom_s_struct.key2,0,down_level);
    if (gb_0 && !gb_1) return -1;
    if (!gb_0 && gb_1) return 1;
    if (gb_0 && gb_1) {
        switch (GB_read_type(gb_0)) {
            case GB_STRING:
                s0 = GB_read_string(gb_0);
                s1 = GB_read_string(gb_1);
                cmp = strcmp(s0,s1);
                free(s0);
                free(s1);
                if (cmp) return cmp;
                break;
            case GB_FLOAT:
                f0 = GB_read_float(gb_0);
                f1 = GB_read_float(gb_1);
                if (f0<f1) return -1;
                if (f1<f0) return 1;
                break;
            case GB_INT:
                i0 = GB_read_int(gb_0);
                i1 = GB_read_int(gb_1);
                if (i0<i1) return -1;
                if (i1<i0) return 1;
                break;
            default:
                break;
        }

    }
    /* third key */
    gb_0 = GB_find(first,custom_s_struct.key3,0,down_level);
    gb_1 = GB_find(second,custom_s_struct.key3,0,down_level);
    if (gb_0 && !gb_1) return -1;
    if (!gb_0 && gb_1) return 1;
    if (gb_0 && gb_1) {
        switch (GB_read_type(gb_0)) {
            case GB_STRING:
                s0 = GB_read_string(gb_0);
                s1 = GB_read_string(gb_1);
                cmp = strcmp(s0,s1);
                free(s0);
                free(s1);
                if (cmp) return cmp;
                break;
            case GB_FLOAT:
                f0 = GB_read_float(gb_0);
                f1 = GB_read_float(gb_1);
                if (f0<f1) return -1;
                if (f1<f0) return 1;
                break;
            case GB_INT:
                i0 = GB_read_int(gb_0);
                i1 = GB_read_int(gb_1);
                if (i0<i1) return -1;
                if (i1<i0) return 1;
                break;
            default:
                break;
        }

    }
    return 0;
}

GBDATA **gb_resort_data_list;
long    gb_resort_data_count;

char *NT_resort_data_base_by_tree(GBT_TREE *tree,GBDATA *gb_species_data)
{
    if (!tree) return 0;
    if ( tree->is_leaf ) {
        if (!tree->gb_node) return 0;
        gb_resort_data_list[gb_resort_data_count++] = tree->gb_node;
        return 0;
    }else{
        char *error =0;
        if (!error) error = NT_resort_data_base_by_tree(tree->leftson,gb_species_data);
        if (!error) error = NT_resort_data_base_by_tree(tree->rightson,gb_species_data);
        return error;
    }
}

GB_ERROR
NT_resort_data_base(GBT_TREE *tree, char *key1, char *key2, char *key3)
{
    GB_begin_transaction(gb_main);
    GBDATA *gb_sd = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    custom_s_struct.key1=key1;
    custom_s_struct.key2=key2;
    custom_s_struct.key3=key3;
    GB_ERROR error = 0;
    if (tree){
        gb_resort_data_count = 0;
        gb_resort_data_list = (GBDATA **)calloc( sizeof(GBDATA *), GB_nsons(gb_sd) + 256);
        error = NT_resort_data_base_by_tree(tree,gb_sd);
    }else{
        gb_resort_data_list = GBT_gen_species_array(gb_main,&gb_resort_data_count);
        GB_mergesort((void **)gb_resort_data_list,0,gb_resort_data_count,(gb_compare_two_items_type)resort_data_by_customsub,0);
    }
    error = GB_resort_data_base(gb_main,gb_resort_data_list,gb_resort_data_count);
    free(gb_resort_data_list);

    if (error){
        GB_abort_transaction(gb_main);
    }else{
        GB_commit_transaction(gb_main);
    }
    return error;
}

void NT_resort_data_by_phylogeny(AW_window *dummy, GBT_TREE **ptree){
    aw_openstatus("resorting data");
    AWUSE(dummy);
    GB_ERROR error = 0;
    GBT_TREE *tree = *ptree;
    if (!tree) error = "Please select/build a tree first";
    if (!error) error = NT_resort_data_base(tree,0,0,0);
    aw_closestatus();
    if (error) aw_message(error);

}

void NT_resort_data_by_costum(AW_window *aw){
    aw_openstatus("resorting data");
    GB_ERROR error = 0;
    char *s1 = aw->get_root()->awar("ad_tree/sort_1")->read_string();
    char *s2 = aw->get_root()->awar("ad_tree/sort_2")->read_string();
    char *s3 = aw->get_root()->awar("ad_tree/sort_3")->read_string();
    if (!error) error = NT_resort_data_base(0,s1,s2,s3);
    aw_closestatus();
    if (error) aw_message(error);
}

void NT_build_resort_awars(AW_root *awr, AW_default aw_def){
    awr->awar_string( "ad_tree/sort_1", "name",     aw_def);
    awr->awar_string( "ad_tree/sort_2", "name",     aw_def);
    awr->awar_string( "ad_tree/sort_3", "name",     aw_def);
}

AW_window *NT_build_resort_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init( awr, "SORT_DATABASE", "SORT DATABASE");
    aws->load_xfig("nt_sort.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"sp_sort_fld.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("go");
    aws->callback((AW_CB0)NT_resort_data_by_costum);
    aws->create_button("GO","GO","G");

    awt_create_selection_list_on_scandb(gb_main,
                                        (AW_window*)aws,"ad_tree/sort_1",
                                        NT_RESORT_FILTER,
                                        "key1",0, &AWT_species_selector, 20, 10);

    awt_create_selection_list_on_scandb(gb_main,
                                        (AW_window*)aws,"ad_tree/sort_2",
                                        NT_RESORT_FILTER,
                                        "key2",0, &AWT_species_selector, 20, 10);

    awt_create_selection_list_on_scandb(gb_main,
                                        (AW_window*)aws,"ad_tree/sort_3",
                                        NT_RESORT_FILTER,
                                        "key3",0, &AWT_species_selector, 20, 10);

    return (AW_window *)aws;

}
