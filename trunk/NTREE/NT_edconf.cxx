#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

extern GBDATA *gb_main;

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
        assert(next==0);
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
    assert(node);
    assert(node->gb_node);
    assert(GB_read_flag(node->gb_node)!=0);
    GB_write_flag(node->gb_node, 0);
}

static void mark_species(GBT_TREE *node, Store_species **extra_marked_species) {
    assert(node);
    assert(node->gb_node);
    assert(GB_read_flag(node->gb_node)==0);
    GB_write_flag(node->gb_node, 1);

    *extra_marked_species = (new Store_species(node))->add(*extra_marked_species);
}


/** Builds a configuration string from a tree. Returns the number of marked species */
/* Syntax:

   TAG	1 byte	'GFELS'

   G	group
   F	folded group
   E	end of group
   L	species
   S	SAI

   String
   Seperator (char)1

   X::	Y '\0'
   Y::	eps
   Y::	'\AL' 'Name' Y
   Y::	'\AS' 'Name' Y
   Y::	'\AG' 'Gruppenname' '\A' Y '\AE'
   Y::	'\AF' 'Gruppenname' '\A' Y '\AE'
*/

GBT_TREE *rightmost_leaf(GBT_TREE *node) {
    assert(node);
    while (!node->is_leaf) {
        node = node->rightson;
        assert(node);
    }
    return node;
}

GBT_TREE *left_neighbour_leaf(GBT_TREE *node) {
    if (node) {
        GBT_TREE *father = node->father;
        while (father) {
            if (father->rightson==node) {
                node = rightmost_leaf(father->leftson);
                assert(node->is_leaf);
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
            return 0;	// Zombie
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
                assert(marked_at_left>=0);
                GBT_TREE *leaf_at_left = tree;
                int step_over = marked_at_left+1; // step over myself
                int then_mark = use_species_aside-marked_at_left;

                while (step_over--) {
                    leaf_at_left = left_neighbour_leaf(leaf_at_left);
                }

                Store_species *marked_back = 0;
                while (leaf_at_left && then_mark--) {
                    mark_species(leaf_at_left, extra_marked_species);
                    marked_back = (new Store_species(leaf_at_left))->add(marked_back);
                    leaf_at_left = left_neighbour_leaf(leaf_at_left);
                }

                while (marked_back) {
                    GBS_chrcat(memfile,1);				// Seperated by 1
                    GBS_strcat(memfile,"L");
                    GBS_strcat(memfile,marked_back->getNode()->name);
                    GBS_write_hash(used,marked_back->getNode()->name,1);		// Mark species

                    Store_species *rest = marked_back->remove();
                    delete marked_back;
                    marked_back = rest;
                }

                marked_at_left = use_species_aside;
            }
            // now use_species_aside species to left are marked!
            *auto_mark = use_species_aside;
        }

        GBS_chrcat(memfile,1);				// Seperated by 1
        GBS_strcat(memfile,"L");
        GBS_strcat(memfile,tree->name);
        GBS_write_hash(used,tree->name,1);		// Mark species

        *marked_at_right = marked_at_left+1;
        return 1;
    }

    long oldpos = GBS_memoffset(memfile);
    if (tree->gb_node && tree->name){		// but we are a group
        GBDATA *gb_grouped = GB_find(tree->gb_node,"grouped",0,down_level);
        GBS_chrcat(memfile,1);				// Seperated by 1
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

    if (tree->gb_node && tree->name){		// but we are a group
        GBS_chrcat(memfile,1);			// Seperated by 1
        GBS_chrcat(memfile,'E');		// Group end indicated by 'E'
    }

    if (!nspecies) {
        long newpos = GBS_memoffset(memfile);
        GBS_str_cut_tail(memfile,newpos-oldpos);	// delete group info
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
        if (!sep) return val;	// what's wrong

        if (!nt_build_sai_last_group_name || strncmp(key,nt_build_sai_last_group_name,sep-key)){ // new group
            if (nt_build_sai_last_group_name){
                GBS_chrcat(nt_build_sai_middle_file,1);				// Seperated by 1
                GBS_chrcat(nt_build_sai_middle_file,'E');				// End of old group
            }
            GBS_chrcat(nt_build_sai_middle_file,1);				// Seperated by 1
            GBS_strcat(nt_build_sai_middle_file,"FSAI:");
            GBS_strncat(nt_build_sai_middle_file,key,sep-key);
            nt_build_sai_last_group_name = key;
        }
        GBS_chrcat(nt_build_sai_middle_file,1);				// Seperated by 1
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
	GBDATA *gb_sai;
	for (	gb_sai = GBT_first_SAI_rel_exdata(gb_sai_data);
            gb_sai;
            gb_sai = GBT_next_SAI(gb_sai)){
		GBDATA *gb_name = GB_search(gb_sai,"name",GB_FIND);
		if (!gb_name) continue;
		char *name = GB_read_string(gb_name);
		if (
			(!strcmp(name,	"HELIX")) ||
			(!strcmp(name,	"HELIX_NR")) ||
			(!strcmp(name,	"ECOLI"))
			){
			GBS_chrcat(topfile,1);				// Seperated by 1
			GBS_strcat(topfile,"S");
			GBS_strcat(topfile,name);
		}else{
		    GBDATA *gb_gn = GB_search(gb_sai,"sai_group",GB_FIND);
		    char *gn;
		    if (gb_gn){
                gn = GB_read_string(gb_gn);
		    }else{
                gn = strdup("SAI's");
		    }
		    char *cn = new char[strlen(gn) + strlen(name) + 2];
		    sprintf(cn,"%s%c%s",gn,1,name);
		    GBS_write_hash(hash,cn,1);
		    delete name;
		    delete cn;
		    delete gn;
		}
	}

	// open surrounding SAI-group:
	GBS_chrcat(middlefile, 1);
	GBS_strcat(middlefile, "GSAI-Maingroup");

	nt_build_sai_last_group_name = 0;
	nt_build_sai_middle_file = middlefile;
	GBS_hash_do_sorted_loop(hash,nt_build_sai_string_by_hash,(gbs_hash_sort_func_type)nt_build_sai_sort_strings);
	if (nt_build_sai_last_group_name){
	    GBS_chrcat(middlefile,1);				// Seperated by 1
	    GBS_chrcat(middlefile,'E');				// End of old group
	}

	// close surrounding SAI-group:
	GBS_chrcat(middlefile,1);
	GBS_chrcat(middlefile,'E');

	GBS_free_hash(hash);
}

void nt_build_conf_marked(GB_HASH *used, void *file){
    GBS_chrcat(file,1);				// Seperated by 1
    GBS_strcat(file,"FMore Sequences");
    GBDATA *gb_species;
    for (gb_species = GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species)){
        char *name = GBT_read_string(gb_species,"name");
        if (GBS_read_hash(used,name)){
            delete name;
            continue;
        }
        GBS_chrcat(file,1);
        GBS_strcat(file,"L");
        GBS_strcat(file,name);
        delete name;
    }

    GBS_chrcat(file,1);	// Seperated by 1
    GBS_chrcat(file,'E');	// Group end indicated by 'E'
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
    aws->init( awr, "SELECT_CONFIFURATION", "SELECT A CONFIGURATION", 400, 200 );
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

// void nt_save_configuration(AW_window *aww){
//     aww->hide();
//     char *cn = aww->get_root()->awar(AWAR_CONFIGURATION)->read_string();
//     delete cn;
// }
// AW_window *NT_open_save_configuration_window(AW_root *awr){
//     static AW_window_simple *aws = 0;
//     if (aws) return (AW_window *)aws;

//     awr->awar_string(AWAR_CONFIGURATION,"default_configuration",gb_main);
//     aws = new AW_window_simple;
//     aws->init( awr, "SAVE_CONFIFURATION", "SAVE A CONFIGURATION", 400, 200 );
//     aws->at(10,10);
//     aws->auto_space(0,0);

//     awt_create_selection_list_on_configurations(gb_main,(AW_window *)aws,AWAR_CONFIGURATION);
//     aws->at_newline();

//     aws->callback((AW_CB0)nt_save_configuration);
//     aws->create_button("SAVE","SAVE");

//     aws->callback(AW_POPDOWN);
//     aws->create_button("CLOSE","CLOSE","C");

//     aws->window_fit();
//     return (AW_window *)aws;
// }

// static char *nt_get_configuration_name() {
//     return aw_string_selection("Enter Name of Config.",0, 0);
// }

GB_ERROR NT_create_configuration(AW_window *, GBT_TREE **ptree,const char *conf_name, int use_species_aside){
	GBT_TREE *tree = *ptree;
	char     *to_free = 0;

	if (!conf_name) {
        char *existing_configs = awt_create_string_on_configurations(gb_main);
        conf_name              = to_free = aw_string_selection("Enter Name of Config.", 0, "default_configuration", existing_configs);
        free(existing_configs);
    }
	if (!conf_name) return GB_export_error("no config name");

	if (use_species_aside==-1) {
	    char *use_species = aw_input("Enter number of extra species to view aside marked:", 0);

	    if (use_species) use_species_aside = atoi(use_species);
	    if (use_species_aside<1) return GB_export_error("illegal # of species aside");
	}

 	GB_transaction dummy2(gb_main);		// open close transaction
	GB_HASH *used = GBS_create_hash(10000,0);
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
	    char *mid = GBS_strclose(middlefile,0);
	    GBS_strcat(topmid,mid);
	    delete mid;
	}

	char *middle = GBS_strclose(topmid,0);
	char *top = GBS_strclose(topfile,0);

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
	delete to_free;
	delete middle;
	delete top;
	GBS_free_hash(used);
	if (error) aw_message(error);
	return error;
}

void NT_start_editor_on_tree(AW_window *, GBT_TREE **ptree, int use_species_aside){
    GB_ERROR error = NT_create_configuration(0,ptree,CONFNAME, use_species_aside);
    if (!error) {
        GBCMC_system(gb_main, "arb_edit4 -c "CONFNAME" &");
    }
}

void nt_extract_configuration(AW_window *aww){
    aww->hide();
    GB_transaction dummy2(gb_main);		// open close transaction
    char *cn = aww->get_root()->awar(AWAR_CONFIGURATION)->read_string();
    GBDATA *gb_configuration = GBT_find_configuration(gb_main,cn);
    if (!gb_configuration){
        aw_message(GBS_global_string("Configuration '%s' not fould in the database",cn));
    }else{
        GBDATA *gb_middle_area = GB_search(gb_configuration,"middle_area",GB_STRING);
        char *md = 0;
        if (gb_middle_area){
            GBT_mark_all(gb_main,0);
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
                        GB_write_flag(gb_species,1);
                    }
                }
            }
        }
        delete md;
    }
    delete cn;
}

void nt_delete_configuration(AW_window *aww){
    GB_transaction transaction_var(gb_main);
    char *cn = aww->get_root()->awar(AWAR_CONFIGURATION)->read_string();
    GBDATA *gb_configuration = GBT_find_configuration(gb_main,cn);
    if (gb_configuration){
        GB_ERROR error = GB_delete(gb_configuration);
        if (error) aw_message(error);
    }
    delete cn;
}

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
