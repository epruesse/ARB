#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include "nt_edconf.hxx"

#include <awt.hxx>
#include <awt_canvas.hxx>
#include <awt_tree.hxx>
#include <awt_dtree.hxx>

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define nt_assert(bed) arb_assert(bed)

extern GBDATA *gb_main;

static void init_config_awars(AW_root *root) {
    root->awar_string(AWAR_CONFIGURATION,"default_configuration",gb_main);
}

//  ---------------------------
//      class Store_species
//  ---------------------------
// stores a amount of species:

class Store_species
{
    GBT_TREE *node;
    Store_species *next;
public:
    Store_species(GBT_TREE *aNode) {
        node = aNode;
        next = 0;
    }
    ~Store_species();

    Store_species* add(Store_species *list) {
        nt_assert(next==0);
        next = list;
        return this;
    }

    Store_species* remove() {
        Store_species *follower = next;
        next = 0;
        return follower;
    }

    GBT_TREE *getNode() const { return node; }

    void call(void (*aPizza)(GBT_TREE*)) const;
};

Store_species::~Store_species() {
    delete next;
}

void Store_species::call(void (*aPizza)(GBT_TREE*)) const {
    aPizza(node);
    if (next) next->call(aPizza);
}

static void unmark_species(GBT_TREE *node) {
    nt_assert(node);
    nt_assert(node->gb_node);
    nt_assert(GB_read_flag(node->gb_node)!=0);
    GB_write_flag(node->gb_node, 0);
}

static void mark_species(GBT_TREE *node, Store_species **extra_marked_species) {
    nt_assert(node);
    nt_assert(node->gb_node);
    nt_assert(GB_read_flag(node->gb_node)==0);
    GB_write_flag(node->gb_node, 1);

    *extra_marked_species = (new Store_species(node))->add(*extra_marked_species);
}



/** Builds a configuration string from a tree. Returns the number of marked species */
/* Syntax:

   TAG  1 byte  'GFELS'

   G    group
   F    folded group
   E    end of group
   L    species
   S    SAI

   String
   Seperator (char)1

   X::  Y '\0'
   Y::  eps
   Y::  '\AL' 'Name' Y
   Y::  '\AS' 'Name' Y
   Y::  '\AG' 'Gruppenname' '\A' Y '\AE'
   Y::  '\AF' 'Gruppenname' '\A' Y '\AE'
*/

GBT_TREE *rightmost_leaf(GBT_TREE *node) {
    nt_assert(node);
    while (!node->is_leaf) {
        node = node->rightson;
        nt_assert(node);
    }
    return node;
}

GBT_TREE *left_neighbour_leaf(GBT_TREE *node) {
    if (node) {
        GBT_TREE *father = node->father;
        while (father) {
            if (father->rightson==node) {
                node = rightmost_leaf(father->leftson);
                nt_assert(node->is_leaf);
                if (!node->gb_node) { // Zombie
                    node = left_neighbour_leaf(node);
                }
                return node;
            }
            node = father;
            father = node->father;
        }
    }
    return 0;
}

int nt_build_conf_string_rek(GB_HASH *used,GBT_TREE *tree, void *memfile,
                             Store_species **extra_marked_species, // all extra marked species are inserted here
                             int use_species_aside, // # of species to mark left and right of marked species
                             int *auto_mark, // # species to extra-mark (if not already marked)
                             int marked_at_left, // # of species which were marked (looking to left)
                             int *marked_at_right) // # of species which are marked (when returning from rekursion)
{
    if (!tree) return 0;
    if (tree->is_leaf){
        if (!tree->gb_node) {
            *marked_at_right = marked_at_left;
            return 0;   // Zombie
        }

        if (!GB_read_flag(tree->gb_node)) { // unmarked species
            if (*auto_mark) {
                (*auto_mark)--;
                mark_species(tree, extra_marked_species);
            }
            else {
                *marked_at_right = 0;
                return 0;
            }
        }
        else { // marked species
            if (marked_at_left<use_species_aside) {
                // on the left side there are not as many marked species as needed!

                nt_assert(marked_at_left>=0);

                GBT_TREE *leaf_at_left = tree;
                int       step_over    = marked_at_left+1; // step over myself
                int       then_mark    = use_species_aside-marked_at_left;

                while (step_over--) { // step over self and over any adjacent, marked species
                    leaf_at_left = left_neighbour_leaf(leaf_at_left);
                }

                Store_species *marked_back = 0;
                while (leaf_at_left && then_mark--) { // then additionally mark some species
                    if (GB_read_flag(leaf_at_left->gb_node) == 0) { // if they are not marked yet
                        mark_species(leaf_at_left, extra_marked_species);
                        marked_back = (new Store_species(leaf_at_left))->add(marked_back);
                    }
                    leaf_at_left = left_neighbour_leaf(leaf_at_left);
                }

                while (marked_back) {
                    GBS_chrcat(memfile,1);              // Seperated by 1
                    GBS_strcat(memfile,"L");
                    GBS_strcat(memfile,marked_back->getNode()->name);
                    GBS_write_hash(used,marked_back->getNode()->name,1);        // Mark species

                    Store_species *rest = marked_back->remove();
                    delete marked_back;
                    marked_back = rest;
                }

                marked_at_left = use_species_aside;
            }
            // now use_species_aside species to left are marked!
            *auto_mark = use_species_aside;
        }

        GBS_chrcat(memfile,1);              // Seperated by 1
        GBS_strcat(memfile,"L");
        GBS_strcat(memfile,tree->name);
        GBS_write_hash(used,tree->name,1);      // Mark species

        *marked_at_right = marked_at_left+1;
        return 1;
    }

    long oldpos = GBS_memoffset(memfile);
    if (tree->gb_node && tree->name){       // but we are a group
        GBDATA *gb_grouped = GB_find(tree->gb_node,"grouped",0,down_level);
        GBS_chrcat(memfile,1);              // Seperated by 1
        if (gb_grouped && GB_read_byte(gb_grouped)) {
            GBS_strcat(memfile,"F");
        }else{
            GBS_strcat(memfile,"G");
        }

        GBS_strcat(memfile,tree->name);
    }

    int right_of_leftson;
    long nspecies = nt_build_conf_string_rek(used, tree->leftson, memfile, extra_marked_species, use_species_aside, auto_mark, marked_at_left, &right_of_leftson);
    nspecies += nt_build_conf_string_rek(used, tree->rightson, memfile, extra_marked_species, use_species_aside, auto_mark, right_of_leftson, marked_at_right);

    if (tree->gb_node && tree->name){       // but we are a group
        GBS_chrcat(memfile,1);          // Seperated by 1
        GBS_chrcat(memfile,'E');        // Group end indicated by 'E'
    }

    if (!nspecies) {
        long newpos = GBS_memoffset(memfile);
        GBS_str_cut_tail(memfile,newpos-oldpos);    // delete group info
    }
    return nspecies;
}

static void *nt_build_sai_middle_file;
static const char *nt_build_sai_last_group_name;
long nt_build_sai_sort_strings(const char *k0,long v0,const char *k1,long v1){
    AWUSE(v0); AWUSE(v1);
    return strcmp(k0,k1);
}

#ifdef __cplusplus
extern "C" {
#endif

    static long nt_build_sai_string_by_hash(const char *key,long val){
        char *sep = strchr(key,1);
        if (!sep) return val;   // what's wrong

        if (!nt_build_sai_last_group_name || strncmp(key,nt_build_sai_last_group_name,sep-key)){ // new group
            if (nt_build_sai_last_group_name){
                GBS_chrcat(nt_build_sai_middle_file,1);             // Seperated by 1
                GBS_chrcat(nt_build_sai_middle_file,'E');               // End of old group
            }
            GBS_chrcat(nt_build_sai_middle_file,1);             // Seperated by 1
            GBS_strcat(nt_build_sai_middle_file,"FSAI:");
            GBS_strncat(nt_build_sai_middle_file,key,sep-key);
            nt_build_sai_last_group_name = key;
        }
        GBS_chrcat(nt_build_sai_middle_file,1);             // Seperated by 1
        GBS_strcat(nt_build_sai_middle_file,"S");
        GBS_strcat(nt_build_sai_middle_file,sep+1);
        return val;
    }

#ifdef __cplusplus
}
#endif

/** collect all Sais, place some SAI in top area, rest in middle */
void nt_build_sai_string(void *topfile, void *middlefile){
    GBDATA *gb_sai_data = GB_search(gb_main,"extended_data",GB_FIND);
    if (!gb_sai_data) return;

    GB_HASH *hash = GBS_create_hash(100,1);
    GBDATA  *gb_sai;

    for (gb_sai = GBT_first_SAI_rel_exdata(gb_sai_data); gb_sai; gb_sai = GBT_next_SAI(gb_sai)) {
        GBDATA *gb_name = GB_search(gb_sai,"name",GB_FIND);
        if (!gb_name) continue;

        char *name = GB_read_string(gb_name);

        if (strcmp(name,  "HELIX") == 0  || strcmp(name,  "HELIX_NR") == 0 || strcmp(name,  "ECOLI") == 0) {
            GBS_chrcat(topfile,1);              // Seperated by 1
            GBS_strcat(topfile,"S");
            GBS_strcat(topfile,name);
        }
        else {
            GBDATA *gb_gn = GB_search(gb_sai,"sai_group",GB_FIND);
            char   *gn;

            if (gb_gn)  gn = GB_read_string(gb_gn);
            else        gn = strdup("SAI's");

            char *cn = new char[strlen(gn) + strlen(name) + 2];
            sprintf(cn,"%s%c%s",gn,1,name);
            GBS_write_hash(hash,cn,1);
            free(name);
            delete [] cn;
            free(gn);
        }
    }

    // open surrounding SAI-group:
    GBS_chrcat(middlefile, 1);
    GBS_strcat(middlefile, "GSAI-Maingroup");

    nt_build_sai_last_group_name = 0;
    nt_build_sai_middle_file = middlefile;
    GBS_hash_do_sorted_loop(hash,nt_build_sai_string_by_hash,(gbs_hash_sort_func_type)nt_build_sai_sort_strings);
    if (nt_build_sai_last_group_name) {
        GBS_chrcat(middlefile,1);               // Seperated by 1
        GBS_chrcat(middlefile,'E');             // End of old group
    }

    // close surrounding SAI-group:
    GBS_chrcat(middlefile,1);
    GBS_chrcat(middlefile,'E');

    GBS_free_hash(hash);
}

void nt_build_conf_marked(GB_HASH *used, void *file){
    GBS_chrcat(file,1);             // Seperated by 1
    GBS_strcat(file,"FMore Sequences");
    GBDATA *gb_species;
    for (gb_species = GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species)){
        char *name = GBT_read_string(gb_species,"name");
        if (GBS_read_hash(used,name)){
            free(name);
            continue;
        }
        GBS_chrcat(file,1);
        GBS_strcat(file,"L");
        GBS_strcat(file,name);
        free(name);
    }

    GBS_chrcat(file,1); // Seperated by 1
    GBS_chrcat(file,'E');   // Group end indicated by 'E'
}

#define CONFNAME "default_configuration"

void nt_start_editor_on_configuration(AW_window *aww){
    aww->hide();
    char *cn = aww->get_root()->awar(AWAR_CONFIGURATION)->read_string();
    char *com = (char *)GBS_global_string("arb_edit4 -c '%s' &",cn);
    GBCMC_system(gb_main, com);
    delete cn;
}

AW_window *NT_start_editor_on_old_configuration(AW_root *awr){
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;
    awr->awar_string(AWAR_CONFIGURATION,"default_configuration",gb_main);
    aws = new AW_window_simple;
    aws->init( awr, "SELECT_CONFIFURATION", "SELECT A CONFIGURATION");
    aws->at(10,10);
    aws->auto_space(0,0);
    awt_create_selection_list_on_configurations(gb_main,(AW_window *)aws,AWAR_CONFIGURATION);
    aws->at_newline();

    aws->callback((AW_CB0)nt_start_editor_on_configuration);
    aws->create_button("START","START");

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->window_fit();
    return (AW_window *)aws;
}

void NT_start_editor_on_tree(AW_window *, GBT_TREE **ptree, int use_species_aside){
    GB_ERROR error = NT_create_configuration(0,ptree,CONFNAME, use_species_aside);
    if (!error) {
        GBCMC_system(gb_main, "arb_edit4 -c "CONFNAME" &");
    }
}

enum extractType {
    CONF_EXTRACT,
    CONF_MARK,
    CONF_UNMARK,
    CONF_INVERT,
    CONF_COMBINE // logical AND
};

void nt_extract_configuration(AW_window *aww, AW_CL cl_extractType){
    GB_transaction  dummy2(gb_main); // open close transaction
    char           *cn               = aww->get_root()->awar(AWAR_CONFIGURATION)->read_string();
    GBDATA         *gb_configuration = GBT_find_configuration(gb_main,cn);
    if (!gb_configuration){
        aw_message(GBS_global_string("Configuration '%s' not fould in the database",cn));
    }else{
        GBDATA *gb_middle_area = GB_search(gb_configuration,"middle_area",GB_STRING);
        char *md = 0;
        if (gb_middle_area){
            extractType ext_type = extractType(cl_extractType);

            GB_HASH *was_marked = 0; // only used for CONF_COMBINE

            switch (ext_type) {
                case CONF_EXTRACT:
                    GBT_mark_all(gb_main,0); // unmark all for extract
                    break;
                case CONF_COMBINE: {
                    // store all marked species in hash and unmark them
                    was_marked = GBS_create_hash(GBS_SPECIES_HASH_SIZE,1);
                    for (GBDATA *gbd = GBT_first_marked_species(gb_main); gbd; gbd = GBT_next_marked_species(gbd)) {
                        int marked = GB_read_flag(gbd);
                        if (marked) {
                            char *name = GBT_read_name(gbd);
                            if (name) {
                                GBS_write_hash(was_marked, name, 1);
                                free(name);
                            }
                            GB_write_flag(gbd, 0);
                        }
                    }

                    break;
                }
                default :
                    break;
            }

            md = GB_read_string(gb_middle_area);
            if (md){
                char *p;
                char tokens[2];
                tokens[0] = 1;
                tokens[1] = 0;
                for ( p = strtok(md,tokens); p; p = strtok(NULL,tokens)){
                    if (p[0] != 'L') continue;
                    GBDATA *gb_species = GBT_find_species(gb_main,p+1);
                    if (gb_species){
                        int mark = GB_read_flag(gb_species);
                        switch (ext_type) {
                            case CONF_EXTRACT:
                            case CONF_MARK:     mark = 1; break;
                            case CONF_UNMARK:   mark = 0; break;
                            case CONF_INVERT:   mark = 1-mark; break;
                            case CONF_COMBINE:  {
                                nt_assert(mark == 0); // should have been unmarked above
                                mark = GBS_read_hash(was_marked, p+1); // mark if was_marked
                                break;
                            }
                            default: nt_assert(0); break;
                        }
                        GB_write_flag(gb_species,mark);
                    }
                }
            }

            GBS_free_hash(was_marked);
        }
        free(md);
    }
    free(cn);
}

void nt_delete_configuration(AW_window *aww){
    GB_transaction transaction_var(gb_main);
    char *cn = aww->get_root()->awar(AWAR_CONFIGURATION)->read_string();
    GBDATA *gb_configuration = GBT_find_configuration(gb_main,cn);
    if (gb_configuration){
        GB_ERROR error = GB_delete(gb_configuration);
        if (error) aw_message(error);
    }
    free(cn);
}


GB_ERROR NT_create_configuration(AW_window *, GBT_TREE **ptree,const char *conf_name, int use_species_aside){
    GBT_TREE *tree = *ptree;
    char     *to_free = 0;

    if (!conf_name) {
        char *existing_configs = awt_create_string_on_configurations(gb_main);
        conf_name              = to_free = aw_string_selection("CREATE CONFIGURATION", "Enter name of configuration:", AWAR_CONFIGURATION, "default_configuration", existing_configs, 0, 0);
        free(existing_configs);
    }
    if (!conf_name) return GB_export_error("no config name");

    if (use_species_aside==-1) {
        static int last_used_species_aside = 3;
        {
            const char *val                    = GBS_global_string("%i", last_used_species_aside);
            char       *use_species            = aw_input("Enter number of extra species to view aside marked:", 0, val);
            if (use_species) use_species_aside = atoi(use_species);
            free(use_species);
        }

        if (use_species_aside<1) return GB_export_error("illegal # of species aside");

        last_used_species_aside = use_species_aside; // remember for next time
    }

    GB_transaction dummy2(gb_main);     // open close transaction
    GB_HASH *used = GBS_create_hash(GBS_SPECIES_HASH_SIZE,0);
    void *topfile = GBS_stropen(1000);
    void *topmid = GBS_stropen(10000);
    {
        void *middlefile = GBS_stropen(10000);
        nt_build_sai_string(topfile,topmid);

        if (use_species_aside) {
            Store_species *extra_marked_species = 0;
            int auto_mark = 0;
            int marked_at_right;
            nt_build_conf_string_rek(used, tree, middlefile, &extra_marked_species, use_species_aside, &auto_mark, use_species_aside, &marked_at_right);
            if (extra_marked_species) {
                extra_marked_species->call(unmark_species);
                delete extra_marked_species;
            }
        }
        else {
            int dummy_1=0, dummy_2;
            nt_build_conf_string_rek(used, tree, middlefile, 0, 0, &dummy_1, 0, &dummy_2);
        }
        nt_build_conf_marked(used,topmid);
        char *mid = GBS_strclose(middlefile);
        GBS_strcat(topmid,mid);
        free(mid);
    }

    char *middle = GBS_strclose(topmid);
    char *top = GBS_strclose(topfile);

    GBDATA *gb_configuration = GBT_create_configuration(gb_main,conf_name);
    GB_ERROR error = 0;
    if (!gb_configuration){
        error = GB_get_error();
    }else{
        GBDATA *gb_top_area = GB_search(gb_configuration,"top_area",GB_STRING);
        GBDATA *gb_middle_area = GB_search(gb_configuration,"middle_area",GB_STRING);
        error = GB_write_string(gb_top_area,top);
        if (!error){
            error = GB_write_string(gb_middle_area,middle);
        }
    }
    free(to_free);
    free(middle);
    free(top);
    GBS_free_hash(used);
    if (error) aw_message(error);
    return error;
}

void nt_store_configuration(AW_window */*aww*/, AW_CL cl_GBT_TREE_ptr) {
    GBT_TREE **ptree = (GBT_TREE**)cl_GBT_TREE_ptr;
    GB_ERROR   err   = NT_create_configuration(0, ptree, 0, 0);
    if (err) aw_message(err);
}

void nt_rename_configuration(AW_window *aww) {
    AW_awar  *awar_curr_cfg = aww->get_root()->awar(AWAR_CONFIGURATION);
    char     *old_name      = awar_curr_cfg->read_string();
    GB_ERROR  err           = 0;

    GB_transaction dummy(gb_main);

    char *new_name = aw_input("Rename selection", "Enter the new name of the selection", 0, old_name);
    if (new_name) {
        GBDATA *gb_existing_cfg = GBT_find_configuration(gb_main, new_name);
        if (gb_existing_cfg) {
            err = GBS_global_string("There is already a selection named '%s'", new_name);
        }
        else {
            GBDATA *gb_old_cfg = GBT_find_configuration(gb_main, old_name);
            if (gb_old_cfg) {
                GBDATA *gb_name = GB_find(gb_old_cfg, "name", 0, down_level);
                if (gb_name) {
                    err = GB_write_string(gb_name, new_name);
                    if (!err) awar_curr_cfg->write_string(new_name);
                }
                else {
                    err = "Selection has no name";
                }
            }
            else {
                err = "Can't find that selection";
            }
        }
        free(new_name);
    }

    if (err) aw_message(err);

    free(new_name);
    free(old_name);
}
//  --------------------------------------------------------------------------------------
//      AW_window *create_configuration_admin_window(AW_root *root, GBT_TREE **ptree)
//  --------------------------------------------------------------------------------------
AW_window *create_configuration_admin_window(AW_root *root, GBT_TREE **ptree) {
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    init_config_awars(root);

    aws = new AW_window_simple;
    aws->init(root, "SPECIES_SELECTIONS", "Species Selections");
    aws->load_xfig("nt_selection.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"configuration.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("list");
    awt_create_selection_list_on_configurations(gb_main, aws, AWAR_CONFIGURATION);

    aws->at("store");
    aws->callback(nt_store_configuration, (AW_CL)ptree);
    aws->create_button("STORE","STORE","S");

    aws->at("extract");
    aws->callback(nt_extract_configuration, CONF_EXTRACT);
    aws->create_button("EXTRACT","EXTRACT","E");

    aws->at("mark");
    aws->callback(nt_extract_configuration, CONF_MARK);
    aws->create_button("MARK","MARK","M");

    aws->at("unmark");
    aws->callback(nt_extract_configuration, CONF_UNMARK);
    aws->create_button("UNMARK","UNMARK","U");

    aws->at("invert");
    aws->callback(nt_extract_configuration, CONF_INVERT);
    aws->create_button("INVERT","INVERT","I");

    aws->at("combine");
    aws->callback(nt_extract_configuration, CONF_COMBINE);
    aws->create_button("COMBINE","COMBINE","C");

    aws->at("delete");
    aws->callback(nt_delete_configuration);
    aws->create_button("DELETE","DELETE","D");

    aws->at("rename");
    aws->callback(nt_rename_configuration);
    aws->create_button("RENAME","RENAME","R");

    return aws;
}

void NT_configuration_admin(AW_window *aw_main, AW_CL cl_GBT_TREE_ptr, AW_CL) {
    GBT_TREE  **ptree   = (GBT_TREE**)cl_GBT_TREE_ptr;
    AW_root    *aw_root = aw_main->get_root();
    AW_window  *aww     = create_configuration_admin_window(aw_root, ptree);

    aww->show();
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

#if 0

// obsolete:
AW_window *NT_extract_configuration(AW_root *awr){
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;
    awr->awar_string(AWAR_CONFIGURATION,"default_configuration",gb_main);
    aws = new AW_window_simple;
    aws->init( awr, "EXTRACT_CONFIGURATION", "EXTRACT A CONFIGURATION", 400, 200 );
    aws->at(10,10);
    aws->auto_space(0,0);
    awt_create_selection_list_on_configurations(gb_main,(AW_window *)aws,AWAR_CONFIGURATION);
    aws->at_newline();

    aws->callback((AW_CB0)nt_extract_configuration);
    aws->create_button("EXTRACT","EXTRACT","E");

    aws->callback(nt_delete_configuration);
    aws->create_button("DELETE","DELETE");

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback(AW_POPUP_HELP,(AW_CL)"configuration.hlp");
    aws->create_button("HELP","HELP","H");

    aws->window_fit();
    return (AW_window *)aws;
}
// obsolete:
GB_ERROR NT_create_configuration_cb(AW_window *aww, AW_CL cl_GBT_TREE_ptr, AW_CL cl_use_species_aside) {
    GBT_TREE **ptree             = (GBT_TREE**)(cl_GBT_TREE_ptr);
    int        use_species_aside = int(cl_use_species_aside);

    init_config_awars(aww->get_root());
    return NT_create_configuration(0, ptree, 0, use_species_aside);
}

#endif
