#include <stdio.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include "ad_spec.hxx"
#include <awt.hxx>
#include <awt_www.hxx>
#include <awt_tree.hxx>
#include <awt_canvas.hxx>
#include <awt_dtree.hxx>
#include <awtlocal.hxx>
#include <awtc_next_neighbours.hxx>
#include <ntree.hxx>

extern GBDATA *gb_main;
#define AD_F_ALL (AW_active)-1

void create_species_var(AW_root *aw_root, AW_default aw_def)
{
    //	aw_root->awar_string( AWAR_SPECIES_NAME, "" ,	gb_main);
    aw_root->awar_string( AWAR_SPECIES_DEST, "" ,	aw_def);
    aw_root->awar_string( AWAR_SPECIES_INFO, "" ,	aw_def);
    aw_root->awar_string( AWAR_SPECIES_KEY, "" ,	aw_def);
    aw_root->awar_string( AWAR_FIELD_REORDER_SOURCE, "" ,	aw_def);
    aw_root->awar_string( AWAR_FIELD_REORDER_DEST, "" ,	aw_def);
    aw_root->awar_string( AWAR_FIELD_CREATE_NAME, "" ,	aw_def);
    aw_root->awar_int( AWAR_FIELD_CREATE_TYPE, GB_STRING, 	aw_def );
    aw_root->awar_string( AWAR_FIELD_DELETE, "" ,	aw_def);

}

void species_rename_cb(AW_window *aww){
    GB_ERROR error = 0;
    char *source = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    char *dest = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();
    GBT_begin_rename_session(gb_main,0);
    error = GBT_rename_species(source,dest);
    if (!error) {
        aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string(dest);
    }
    if (!error) GBT_commit_rename_session(0);
    else	GBT_abort_rename_session();
    if (error) aw_message(error);
    delete source;
    delete dest;
}

void species_copy_cb(AW_window *aww){
    GB_ERROR error = 0;
    char *source = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    char *dest = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();
    GB_begin_transaction(gb_main);
    GBDATA *gb_species_data =	GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    GBDATA *gb_species =		GBT_find_species_rel_species_data(gb_species_data,source);
    GBDATA *gb_dest =		GBT_find_species_rel_species_data(gb_species_data,dest);
    if (gb_dest) {
        error = "Sorry: species already exists";
    }else 	if (gb_species) {
        gb_dest = GB_create_container(gb_species_data,"species");
        error = GB_copy(gb_dest,gb_species);
        if (!error) {
            GBDATA *gb_name =
                GB_search(gb_dest,"name",GB_STRING);
            error = GB_write_string(gb_name,dest);
        }
        if (!error) {
            aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string(dest);
        }
    }else{
        error = "Please select a species first";
    }
    if (!error){
        aww->hide();
        GB_commit_transaction(gb_main);
    }else{
        GB_abort_transaction(gb_main);
    }
    if (error) aw_message(error);
    delete source;
    delete dest;
}

void move_species_to_extended(AW_window *aww){
    GB_ERROR error = 0;
    char *source = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    GB_begin_transaction(gb_main);

    GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    GBDATA *gb_species = GBT_find_species(gb_main,source);
    GBDATA *gb_dest = GBT_find_SAI_rel_exdata(gb_extended_data,source);
    if (gb_dest) {
        error = "Sorry: SAI already exists";
    }else 	if (gb_species) {
        gb_dest = GB_create_container(gb_extended_data,"extended");
        error = GB_copy(gb_dest,gb_species);
        if (!error) {
            error = GB_delete(gb_species);
            if (!error) {
                aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string("");
            }
        }
    }else{
        error = "Please select a species first";
    }

    if (!error) GB_commit_transaction(gb_main);
    else	GB_abort_transaction(gb_main);
    if (error) aw_message(error);
    delete source;
}


void species_create_cb(AW_window *aww){
    GB_ERROR error = 0;
    char *dest = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();
    GB_begin_transaction(gb_main);
    GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    GBDATA *gb_dest = GBT_find_species_rel_species_data(gb_species_data,dest);
    if (gb_dest) {
        error = "Sorry: species already exists";
    }else{
        gb_dest = GBT_create_species_rel_species_data(gb_species_data,dest);
        if (!gb_dest) error = GB_get_error();
        if (!error) {
            aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string(dest);
        }
    }
    if (!error) GB_commit_transaction(gb_main);
    else	GB_abort_transaction(gb_main);
    if (error) aw_message(error);
    delete dest;
}

AW_window *create_species_rename_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "RENAME_SPECIES", "SPECIES RENAME", 100, 100 );
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("label");
    aws->create_button(0,"Please enter the new name\nof the species");

    aws->at("input");
    aws->create_input_field(AWAR_SPECIES_DEST,15);

    aws->at("ok");
    aws->callback(species_rename_cb);
    aws->create_button("GO","GO","G");			   

    return (AW_window *)aws;
}

AW_window *create_species_copy_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "COPY_SPECIES", "SPECIES COPY", 100, 100 );
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("label");
    aws->create_button(0,"Please enter the name\nof the new species");

    aws->at("input");
    aws->create_input_field(AWAR_SPECIES_DEST,15);

    aws->at("ok");
    aws->callback(species_copy_cb);
    aws->create_button("GO","GO","G");			   

    return (AW_window *)aws;
}

AW_window *create_species_create_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "CREATE_SPECIES","SPECIES CREATE", 100, 100 );
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("label");
    aws->create_button(0,"Please enter the name\nof the new species");

    aws->at("input");
    aws->create_input_field(AWAR_SPECIES_DEST,15);

    aws->at("ok");
    aws->callback(species_create_cb);
    aws->create_button("GO","GO","G");			   

    return (AW_window *)aws;
}

void ad_species_delete_cb(AW_window *aww){
    if (aw_message("Are you sure to delete the species","OK,CANCEL"))return;
    GB_ERROR error = 0;
    char *source = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    GB_begin_transaction(gb_main);
    GBDATA *gb_species = GBT_find_species(gb_main,source);
	
    if (gb_species) error = GB_delete(gb_species);
    else		error = "Please select a species first";

    if (!error) GB_commit_transaction(gb_main);
    else	GB_abort_transaction(gb_main);

    if (error) aw_message(error);
    delete source;
}
static AW_CL ad_global_scannerid = 0;
static AW_root *ad_global_scannerroot = 0;

void AD_map_species(AW_root *aw_root, AW_CL scannerid)
{
    GB_push_transaction(gb_main);
    char *source = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species = GBT_find_species(gb_main,source);
    if (gb_species) 
        awt_map_arbdb_scanner(scannerid,gb_species,0);
    GB_pop_transaction(gb_main);
    delete source;
}

void AD_map_viewer(GBDATA *gbd,AD_MAP_VIEWER_TYPE type)
{
    GB_push_transaction(gb_main);
    if (ad_global_scannerid){
        GBDATA *gb_species_data =	GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
        if (gbd && GB_get_father(gbd)==gb_species_data) {
            // I got a species !!!!!!
            GBDATA *gb_name = GB_find(gbd,"name",0,down_level);
            if (gb_name) {
                char *name = GB_read_string(gb_name);
                ad_global_scannerroot->awar(AWAR_SPECIES_NAME)->write_string(name);
				// that will map the species
            }else{
                ad_global_scannerroot->awar(AWAR_SPECIES_NAME)->write_string("");
                awt_map_arbdb_scanner(ad_global_scannerid,gbd,0);
				// do it by hand
            }
        }else{
            ad_global_scannerroot->awar(AWAR_SPECIES_NAME)->write_string("");
            awt_map_arbdb_scanner(ad_global_scannerid,gbd,0);
            // do it by hand
        }
    }
    while (gbd && type == ADMVT_WWW){
        char *name = strdup("noname");
        GBDATA *gb_name = GB_find(gbd,"name",0,down_level);
        if (!gb_name) gb_name = GB_find(gbd,"group_name",0,down_level); // bad hack, should work
        if (gb_name){
            delete name;
            name = GB_read_string(gb_name);
        }
        GB_ERROR error = awt_openURL_by_gbd(nt.awr,gb_main,gbd, name);
        if (error) aw_message(error);
        break;
    }
    GB_pop_transaction(gb_main);
}

void ad_list_reorder_cb(AW_window *aws) {
    GB_begin_transaction(gb_main);
    char *source = aws->get_root()->awar(AWAR_FIELD_REORDER_SOURCE)->read_string();
    char *dest = aws->get_root()->awar(AWAR_FIELD_REORDER_DEST)->read_string();
    GB_ERROR warning = 0;

    GBDATA *gb_source = awt_get_key(gb_main,source);
    GBDATA *gb_dest = awt_get_key(gb_main,dest);
    GBDATA *gb_key_data =
        GB_search(gb_main,CHANGE_KEY_PATH,GB_CREATE_CONTAINER);
    if (!gb_source) {
        aw_message("Please select an item you want to move (left box)");
    }
    if (gb_source &&!gb_dest) {
        aw_message("Please select a destination where to place your item (right box)");
    }
    if (gb_source && gb_dest && (gb_dest !=gb_source) ) {
        GBDATA **new_order;
        int nitems = 0;
        GBDATA *gb_cnt;
        for (	gb_cnt	= GB_find(gb_key_data,0,0,down_level);
                gb_cnt;
                gb_cnt = GB_find(gb_cnt,0,0,this_level|search_next)){
            nitems++;
        }
        new_order = new GBDATA *[nitems];
        nitems = 0;
        for (	gb_cnt	= GB_find(gb_key_data,0,0,down_level);
                gb_cnt;
                gb_cnt = GB_find(gb_cnt,0,0,this_level|search_next)){
            if (gb_cnt == gb_source) continue;
            new_order[nitems++] = gb_cnt;
            if (gb_cnt == gb_dest) {
                new_order[nitems++] = gb_source;
            }
        }
        warning = GB_resort_data_base(gb_main,new_order,nitems);
        delete new_order;
    }

    delete source;
    delete dest;
    GB_commit_transaction(gb_main);
    if (warning) aw_message(warning);
}

AW_window *create_ad_list_reorder(AW_root *root)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;
    aws = new AW_window_simple;
    aws->init( root, "REORDER_FIELDS", "REORDER FIELDS",600, 200 );
    aws->load_xfig("ad_kreo.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("doit");
    aws->button_length(0);
    aws->callback(ad_list_reorder_cb);
    aws->help_text("spaf_reorder.hlp");
    aws->create_button("MOVE_TO_NEW_POSITION",
                       "MOVE  SELECTED LEFT  ITEM\nAFTER SELECTED RIGHT ITEM","P");

    awt_create_selection_list_on_scandb(gb_main,
                                        (AW_window*)aws,AWAR_FIELD_REORDER_SOURCE,
                                        AWT_NDS_FILTER,
                                        "source",0);
    awt_create_selection_list_on_scandb(gb_main,
                                        (AW_window*)aws,AWAR_FIELD_REORDER_DEST,
                                        AWT_NDS_FILTER,
                                        "dest",0);

    return (AW_window *)aws;
}

void ad_field_only_delete(AW_window *aws)
{
    AWUSE(aws);

    GB_begin_transaction(gb_main);
    char *source = aws->get_root()->awar(AWAR_FIELD_DELETE)->read_string();
    GB_ERROR error = 0;

    GBDATA *gb_source = awt_get_key(gb_main,source);
    if (!gb_source) {
        aw_message("Please select an item you want to delete");
    }else{
        error = GB_delete(gb_source);
    }
    delete source;
    if (error) {
        GB_abort_transaction(gb_main);
        if (error) aw_message(error);
    }else{
        GB_commit_transaction(gb_main);
    }
}

void ad_field_delete(AW_window *aws)
{
    AWUSE(aws);

    GB_begin_transaction(gb_main);
    char *source = aws->get_root()->awar(AWAR_FIELD_DELETE)->read_string();
    GB_ERROR error = 0;

    GBDATA *gb_source = awt_get_key(gb_main,source);
    if (!gb_source) {
        aw_message("Please select an item you want to delete");
    }else{
        error = GB_delete(gb_source);
    }
    GBDATA *gb_species;

    for (gb_species = GBT_first_species(gb_main);
         !error && gb_species;
         gb_species = GBT_next_species(gb_species)){
        GBDATA *gbd = GB_search(gb_species,source,GB_FIND);
        if (gbd){
            error = GB_delete(gbd);
        }
    }

    delete source;
    if (error) {
        GB_abort_transaction(gb_main);
        if (error) aw_message(error);
    }else{
        GB_commit_transaction(gb_main);
    }
}

AW_window *create_ad_field_delete(AW_root *root)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;
    aws = new AW_window_simple;
    aws->init( root, "DELETE_FIELD", "DELETE FIELD", 600, 200 );
    aws->load_xfig("ad_delof.fig");
    aws->button_length(6);
	
    aws->at("close");aws->callback( AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("help");aws->callback( AW_POPUP_HELP,(AW_CL)"spaf_delete.hlp");
    aws->create_button("HELP","HELP","H");			   

    aws->at("doit");
    aws->callback(ad_field_only_delete);
    aws->help_text("rm_field_only.hlp");
    aws->create_button("HIDE_FIELD","HIDE FIELD\n(NOTHING DELETED)","F");

    aws->at("delf");
    aws->callback(ad_field_delete);
    aws->help_text("rm_field_cmpt.hlp");
    aws->create_button("DELETE_FIELD", "DELETE FIELD\n(DATA DELETED)","C");

    awt_create_selection_list_on_scandb(gb_main,
                                        (AW_window*)aws,AWAR_FIELD_DELETE,
                                        -1,
                                        "source",0);

    return (AW_window *)aws;
}

void ad_field_create_cb(AW_window *aws)
{
    GB_push_transaction(gb_main);
    char *name = aws->get_root()->awar(AWAR_FIELD_CREATE_NAME)->read_string();
    GB_ERROR error = GB_check_key(name);
    GB_ERROR error2 = GB_check_hkey(name);
    if (error && !error2) {
        aw_message("Warning: Your key contain a '/' character,\n"
                   "	that means it is a hierarchical key");
        error = 0;
    }

    int type = (int)aws->get_root()->awar(AWAR_FIELD_CREATE_TYPE)->read_int();

    if(!error) error = awt_add_new_changekey(gb_main, name, type);
    if (error) {
        aw_message(error);
    }else{
        aws->hide();
    }
    delete name;
    GB_pop_transaction(gb_main); 
}

AW_window *create_ad_field_create(AW_root *root)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;
    aws = new AW_window_simple;
    aws->init( root, "CREATE_FIELD","CREATE A NEW FIELD", 400, 100 );
    aws->load_xfig("ad_fcrea.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");			   


    aws->at("input");
    aws->label("FIELD NAME");
    aws->create_input_field(AWAR_FIELD_CREATE_NAME,15);

    aws->at("type");
    aws->create_toggle_field(AWAR_FIELD_CREATE_TYPE,"FIELD TYPE","F");
    aws->insert_toggle("Ascii Text","S",		(int)GB_STRING);
    aws->insert_toggle("Link","L",			(int)GB_LINK);
    aws->insert_toggle("Rounded Numerical","N",	(int)GB_INT);
    aws->insert_toggle("Numerical","R",		(int)GB_FLOAT);
    aws->insert_toggle("MASK = 01 Text","0",	(int)GB_BITS);
    aws->update_toggle_field();


    aws->at("ok");
    aws->callback(ad_field_create_cb);
    aws->create_button("CREATE","CREATE","C");			   

    return (AW_window *)aws;
}

void ad_spec_create_field_items(AW_window *aws) {
    aws->insert_menu_topic("reorder_fields",	"Reorder    Fields ...",	"r","spaf_reorder.hlp",	AD_F_ALL,	AW_POPUP, (AW_CL)create_ad_list_reorder, 0 );
    aws->insert_menu_topic("delete_field",		"Delete/Hide Field ...","D","spaf_delete.hlp",	AD_F_ALL,	AW_POPUP, (AW_CL)create_ad_field_delete, 0 );
    aws->insert_menu_topic("create_field",		"Create Field ...",	"C","spaf_create.hlp",	AD_F_ALL,	AW_POPUP, (AW_CL)create_ad_field_create, 0 );
    aws->insert_menu_topic("unhide_fields",		"Scan Database for all Hidden Fields","S","scandb.hlp",AD_F_ALL,(AW_CB)awt_selection_list_rescan_cb, (AW_CL)gb_main, AWT_NDS_FILTER );
}

#include <probe_design.hxx>

void awtc_nn_search_all_listed(AW_window *aww,AW_CL _cbs  ){
    struct adaqbsstruct *cbs = (struct adaqbsstruct *)_cbs;
    GB_begin_transaction(gb_main);
    int pts = aww->get_root()->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
    char *dest_field = aww->get_root()->awar("next_neighbours/dest_field")->read_string();
    char *ali_name = aww->get_root()->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
    GB_ERROR error = 0;
    GB_TYPES dest_type = awt_get_type_of_changekey(gb_main,dest_field);
    if (!dest_type){
        error = GB_export_error("Please select a valid field");
    }
    long max = awt_count_queried_species(cbs);
    
    if (strcmp(dest_field, "name")==0) {
        int answer = aw_message("CAUTION! This will destroy all name-fields of the listed species.\n",
                                "Continue and destroy all name-fields,Abort");
	
        if (answer==1) {
            error = GB_export_error("Aborted by user");
        }
    } 
    
    aw_openstatus("Finding next neighbours");
    long count = 0;
    GBDATA *gb_species;
    for (gb_species = GBT_first_species(gb_main);
         !error && gb_species;
         gb_species = GBT_next_species(gb_species)){
        if (!IS_QUERIED(gb_species,cbs)) continue;
        count ++;
        GBDATA *gb_data = GBT_read_sequence(gb_species,ali_name);
        if (!gb_data)	continue;
        if (count %10 == 0){
            GBDATA *gb_name = GB_search(gb_species,"name",GB_STRING);
            aw_status(GBS_global_string("Species '%s' (%i:%i)",
                                        GB_read_char_pntr(gb_name),
                                        count, max));
        }
        if (aw_status(count/(double)max)){
            error = "operation aborted";
            break;
        }
        {
            char *sequence = GB_read_string(gb_data);
            AWTC_FIND_FAMILY ff(gb_main);
            error = ff.go(pts,sequence,GB_TRUE,1);	
            if (error) break;
            {
                AWTC_FIND_FAMILY_MEMBER *fm = ff.family_list;
                const char *value;
                if (fm){
                    value = GBS_global_string("%i '%s'",fm->matches,fm->name);
                }else{
                    value = "0";
                }
                GBDATA *gb_dest = GB_search(gb_species,dest_field,dest_type);
                error = GB_write_as_string(gb_dest,value);
            }    
            delete sequence;
        }
    }
    aw_closestatus();
    if (error){
        GB_abort_transaction(gb_main);
        aw_message(error);
    }else{
        GB_commit_transaction(gb_main);
    }
    delete dest_field;
    delete ali_name;
}
void awtc_nn_search(AW_window *aww,AW_CL id ){
    GB_transaction dummy(gb_main);
    int pts = aww->get_root()->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
    int max_hits = aww->get_root()->awar("next_neighbours/max_hits")->read_int();
    char *sequence = 0;
    {
        char *sel_species = 	aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
        GBDATA *gb_species =	GBT_find_species(gb_main,sel_species);

        if (!gb_species){
            delete sel_species;
            aw_message("Select a species first");
            return;
        }
        char *ali_name = aww->get_root()->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
        GBDATA *gb_data = GBT_read_sequence(gb_species,ali_name);
        if (!gb_data){
            aw_message(GBS_global_string("Species '%s' has no sequence '%s'",sel_species,ali_name));
        }
        delete sel_species;
        delete ali_name;
        if (!gb_data) return;
        sequence = GB_read_string(gb_data);
    }
    
    AWTC_FIND_FAMILY ff(gb_main);
    GB_ERROR error = ff.go(pts,sequence,GB_TRUE,max_hits);
    delete sequence;
    AW_selection_list* sel = (AW_selection_list *)id;
    aww->clear_selection_list(sel);
    if (error) {
        aw_message(error);
        aww->insert_default_selection(sel,"No hits found","");
    }else{
        AWTC_FIND_FAMILY_MEMBER *fm;
        for (fm = ff.family_list; fm; fm = fm->next){
            const char *dis = GBS_global_string("%-20s Score:%4i",fm->name,fm->matches);
            aww->insert_selection(sel,(char *)dis,fm->name);
        }
	
        aww->insert_default_selection(sel,"No more hits","");
    }
    aww->update_selection_list(sel);
}
void awtc_move_hits(AW_window *,AW_CL id, AW_CL cbs){
    awt_copy_selection_list_2_queried_species((struct adaqbsstruct *)cbs,(AW_selection_list *)id);
}

AW_window *ad_spec_next_neighbours_listed_create(AW_root *aw_root,AW_CL cbs){
    static AW_window_simple *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aw_root->awar_int(AWAR_PROBE_ADMIN_PT_SERVER);
    aw_root->awar_string("next_neighbours/dest_field","tmp");
	
    aws = new AW_window_simple;
    aws->init( aw_root, "SEARCH_NEXT_RELATIVES_OF_LISTED", "Search Next Neighbours of Listed", 600, 0 );
    aws->load_xfig("ad_spec_nnm.fig");
	
    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"next_neighbours_marked.hlp");
    aws->create_button("HELP","HELP","H");			   

    aws->at("pt_server");
    probe_design_build_pt_server_choices(aws,AWAR_PROBE_ADMIN_PT_SERVER,AW_FALSE);


	
    aws->at("field");
    awt_create_selection_list_on_scandb(gb_main,aws,"next_neighbours/dest_field",
                                        (1<<GB_INT) | (1<<GB_STRING), "field",0);


    aws->at("go");
    aws->callback(awtc_nn_search_all_listed,cbs);
    aws->create_button("SEARCH","SEARCH");			   
	
    return (AW_window *)aws;
}

AW_window *ad_spec_next_neighbours_create(AW_root *aw_root,AW_CL cbs){
    static AW_window_simple *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aw_root->awar_int(AWAR_PROBE_ADMIN_PT_SERVER);
    aw_root->awar_int("next_neighbours/max_hits",20);
	
    aws = new AW_window_simple;
    aws->init( aw_root, "SEARCH_NEXT_RELATIVE_OF_SELECTED", "Search Next Neighbours", 600, 0 );
    aws->load_xfig("ad_spec_nn.fig");
	
    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"next_neighbours.hlp");
    aws->create_button("HELP","HELP","H");			   

    aws->at("pt_server");
    probe_design_build_pt_server_choices(aws,AWAR_PROBE_ADMIN_PT_SERVER,AW_FALSE);

    aws->at("max_hit");
    aws->create_input_field("next_neighbours/max_hits",5);
	
    aws->at("hits");
    AW_selection_list *id = aws->create_selection_list(AWAR_SPECIES_NAME);
    aws->insert_default_selection(id,"No hits found","");
    aws->update_selection_list(id);

    aws->at("go");
    aws->callback(awtc_nn_search,(AW_CL)id);
    aws->create_button("SEARCH","SEARCH");			   

    aws->at("move");
    aws->callback(awtc_move_hits,(AW_CL)id,cbs);
    aws->create_button("MOVE_TO_HITLIST","MOVE TO HITLIST");			   
	
    return (AW_window *)aws;
}
AW_window *create_species_window(AW_root *aw_root)
{
    static AW_window_simple_menu *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aws = new AW_window_simple_menu;
    aws->init( aw_root, "SPECIES_INFORMATION", "SPECIES INFORMATION", 0,0,800, 0 );
    aws->load_xfig("ad_spec.fig");

    aws->button_length(8);

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("search");
    aws->callback(AW_POPUP, (AW_CL)ad_create_query_window, 0);
    aws->create_button("SEARCH","SEARCH","S");			   

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"species_info.hlp");
    aws->create_button("HELP","HELP","H");			   


    AW_CL scannerid = awt_create_arbdb_scanner(gb_main, aws, "box",0,"field","enable",AWT_VIEWER,0,"mark",AWT_NDS_FILTER);
    ad_global_scannerid = scannerid;
    ad_global_scannerroot = aws->get_root();

    aws->create_menu(       0,   "SPECIES",     "E", "spa_species.hlp",  AD_F_ALL );
    aws->insert_menu_topic("species_delete",	"Delete",	"D","spa_delete.hlp",	AD_F_ALL,	(AW_CB)ad_species_delete_cb, 0, 0);
    aws->insert_menu_topic("species_rename",	"Rename ...",	"R","spa_rename.hlp",	AD_F_ALL,	AW_POPUP, (AW_CL)create_species_rename_window, 0);
    aws->insert_menu_topic("species_copy",		"Copy ...",	"C","spa_copy.hlp",	AD_F_ALL,	AW_POPUP, (AW_CL)create_species_copy_window, 0);
    aws->insert_menu_topic("species_create",	"Create ...",	"R","spa_create.hlp",	AD_F_ALL, 	AW_POPUP, (AW_CL)create_species_create_window, 0);
    aws->insert_menu_topic("species_convert_2_sai",	"Convert to SAI","E","sp_sp_2_ext.hlp",AD_F_ALL, 	(AW_CB)move_species_to_extended, 0, 0);
    aws->insert_separator();

    aws->create_menu(       0,   "FIELDS",     "I", "spa_fields.hlp",  AD_F_ALL );
    ad_spec_create_field_items(aws);

    aws->get_root()->awar(AWAR_SPECIES_NAME)->add_callback(	AD_map_species,scannerid);
	
    aws->show();
    AD_map_species(aws->get_root(),scannerid);
    return (AW_window *)aws;
}

AW_CL ad_query_global_cbs = 0;

void ad_unquery_all(){
    awt_unquery_all(0,(struct adaqbsstruct *)ad_query_global_cbs);
}

void ad_query_update_list(){
    awt_query_update_list(NULL,(struct adaqbsstruct *)ad_query_global_cbs);
}

AW_window *ad_create_query_window(AW_root *aw_root)
{
    static AW_window_simple_menu *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aws = new AW_window_simple_menu;
    aws->init( aw_root, "SPECIES_QUERY", "SEARCH and QUERY", 0,0,500, 0 );
    aws->create_menu(0,"MORE_FUNCTIONS","F");
    aws->load_xfig("ad_query.fig");


    awt_query_struct awtqs;

    awtqs.gb_main		= gb_main;
    awtqs.species_name	= AWAR_SPECIES_NAME;
    awtqs.select_bit	= 1;
    awtqs.use_menu		= 1;
    awtqs.ere_pos_fig	= "ere";
    awtqs.by_pos_fig	= "by";
    awtqs.qbox_pos_fig	= "qbox";
    awtqs.rescan_pos_fig	= 0;
    awtqs.key_pos_fig	= 0;
    awtqs.query_pos_fig	= "content";
    awtqs.result_pos_fig	= "result";
    awtqs.count_pos_fig	= "count";
    awtqs.do_query_pos_fig	= "doquery";
    awtqs.do_mark_pos_fig	= "domark";
    awtqs.do_unmark_pos_fig	= "dounmark";
    awtqs.do_delete_pos_fig	= "dodelete";
    awtqs.do_set_pos_fig	= "doset";
    awtqs.do_refresh_pos_fig= "dorefresh";
    awtqs.open_parser_pos_fig= "openparser";
    awtqs.create_view_window= (AW_CL)create_species_window;
    AW_CL cbs = (AW_CL)awt_create_query_box((AW_window*)aws,&awtqs);
    ad_query_global_cbs = cbs;
    aws->create_menu(       0,   "MORE_SEARCH",     "S" );
    aws->insert_menu_topic( "search_equal_fields_within_db","Search For Equal Fields and Mark Duplikates",			"E",0,	-1, (AW_CB)awt_search_equal_entries, cbs, 0 );
    aws->insert_menu_topic( "search_equal_words_within_db", "Search For Equal Words Between Fields and Mark Duplikates",	"W",0,	-1, (AW_CB)awt_search_equal_entries, cbs, 1 );
    aws->insert_menu_topic( "search_next_relativ_of_sel",	"Search Next Relatives of SELECTED Species in PT_Server ...",	"R",0,	-1, (AW_CB)AW_POPUP, 	(AW_CL)ad_spec_next_neighbours_create, cbs );
    aws->insert_menu_topic( "search_next_relativ_of_listed","Search Next Relatives of LISTED Species in PT_Server ...",		"M",0,	-1, (AW_CB)AW_POPUP, 	(AW_CL)ad_spec_next_neighbours_listed_create, cbs );

    aws->button_length(7);

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("help");
    aws->callback( AW_POPUP_HELP,(AW_CL)"sp_search.hlp");
    aws->create_button("HELP","HELP","H");			   

    return (AW_window *)aws;
}
