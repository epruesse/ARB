#include <GEN.hxx>
#include <awt.hxx>
#include <ntree.hxx>
#include <aw_awars.hxx>
#include <arbdbt.h>
#include <probe_design.hxx>
#include "GEN_db.hxx"


static GBDATA *get_current_gene(AW_root *root) {
    GB_transaction dummy(gb_main);
    GBDATA *gb_gene = 0;
    char *species_name = root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species = GBT_find_species(gb_main,species_name);

    if (gb_species) {
	char *gene_name = root->awar(AWAR_GENE_NAME)->read_string();
	gb_gene = GEN_find_gene(gb_species,gene_name);
	delete gene_name;
    }
    delete species_name;

    return gb_gene;
}

static AW_CL ad_global_scannerid = 0;
static AW_root *ad_global_scannerroot = 0;
AW_CL ad_query_global_cbs = 0;

//create_species_window
//ad_create_query_window



AW_window *create_gene_rename_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "RENAME_GENE", "GENE RENAME", 100, 100 );
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("label");
    aws->create_button(0,"Please enter the new name\nof the gene");

    aws->at("input");
    aws->create_input_field(AWAR_GENE_DEST,15);
    aws->at("ok");
    aws->callback(gene_rename_cb);
    //aws->callback(species_rename_cb);
    aws->create_button("GO","GO","G");			   

    return (AW_window *)aws;
}

AW_window *create_gene_copy_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "COPY_GENE", "GENE COPY", 100, 100 );
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("label");
    aws->create_button(0,"Please enter the name\nof the new gene");

    aws->at("input");
    aws->create_input_field(AWAR_GENE_DEST,15);

    aws->at("ok");
    aws->callback(gene_copy_cb);
    aws->create_button("GO","GO","G");			   

    return (AW_window *)aws;
}

AW_window *create_gene_create_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "CREATE_GENE","GENE CREATE", 100, 100 );
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("label");
    aws->create_button(0,"Please enter the name\nof the new gene");

    aws->at("input");
    aws->create_input_field(AWAR_GENE_DEST,15);

    aws->at("ok");
    aws->callback(gene_create_cb);
    aws->create_button("GO","GO","G");			   

    return (AW_window *)aws;
}


void gene_rename_cb(AW_window *aww){
   //  GB_ERROR error = 0;
//     char *source = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
//     char *dest = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();
//     GBT_begin_rename_session(gb_main,0);
//     error = GBT_rename_species(source,dest);
//     if (!error) {
//         aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string(dest);
//     }
//     if (!error) GBT_commit_rename_session(0);
//     else	GBT_abort_rename_session();
//     if (error) aw_message(error);
//     delete source;
//     delete dest;
}

void gene_copy_cb(AW_window *aww){
//     GB_ERROR error = 0;
//     char *source = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
//     char *dest = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();
//     GB_begin_transaction(gb_main);
//     GBDATA *gb_species_data =	GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
//     GBDATA *gb_species =		GBT_find_species_rel_species_data(gb_species_data,source);
//     GBDATA *gb_dest =		GBT_find_species_rel_species_data(gb_species_data,dest);
//     if (gb_dest) {
//         error = "Sorry: species already exists";
//     }else 	if (gb_species) {
//         gb_dest = GB_create_container(gb_species_data,"species");
//         error = GB_copy(gb_dest,gb_species);
//         if (!error) {
//             GBDATA *gb_name =
//                 GB_search(gb_dest,"name",GB_STRING);
//             error = GB_write_string(gb_name,dest);
//         }
//         if (!error) {
//             aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string(dest);
//         }
//     }else{
//         error = "Please select a species first";
//     }
//     if (!error){
//         aww->hide();
//         GB_commit_transaction(gb_main);
//     }else{
//         GB_abort_transaction(gb_main);
//     }
//     if (error) aw_message(error);
//     delete source;
//     delete dest;
}


void gene_create_cb(AW_window *aww){
   //  GB_ERROR error = 0;
//     char *dest = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();
//     GB_begin_transaction(gb_main);
//     GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
//     GBDATA *gb_dest = GBT_find_species_rel_species_data(gb_species_data,dest);
//     if (gb_dest) {
//         error = "Sorry: species already exists";
//     }else{
//         gb_dest = GBT_create_species_rel_species_data(gb_species_data,dest);
//         if (!gb_dest) error = GB_get_error();
//         if (!error) {
//             aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string(dest);
//         }
//     }
//     if (!error) GB_commit_transaction(gb_main);
//     else	GB_abort_transaction(gb_main);
//     if (error) aw_message(error);
//     delete dest;
}

void gene_delete_cb(AW_window *aww){
   //  if (aw_message("Are you sure to delete the gene","OK,CANCEL"))return;

//     GB_ERROR error = 0;
//     GB_begin_transaction(gb_main);
//     GBDATA *gb_gene = get_current_gene(aw_root);

	
//     if (gb_gene) error = GB_delete(gb_gene);
//     else		error = "Please select a gene first";

//     if (!error) GB_commit_transaction(gb_main);
//     else	GB_abort_transaction(gb_main);

//     if (error) aw_message(error);
//     delete gene_name;
}



void GEN_map_gene(AW_root *aw_root, AW_CL scannerid)
{
    GBDATA *gb_gene = get_current_gene(aw_root);
    if (gb_gene) awt_map_arbdb_scanner(scannerid,gb_gene,0);

//     GB_push_transaction(gb_main);
//     char *species_name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
//     GBDATA *gb_species = GBT_find_species(gb_main,species_name);

//     if (gb_species) {
// 	char *gene_name = aw_root->awar(AWAR_GENE_NAME)->read_string();
// 	GBDATA *gb_gene = GEN_find_gene(gb_species,gene_name);
// 	if (gb_gene) awt_map_arbdb_scanner(scannerid,gb_gene,0);
// 	delete gene_name;
//     }
//     GB_pop_transaction(gb_main);
//     delete species_name;
}

void GEN_create_field_items(AW_window *aws) {
    aws->insert_menu_topic("reorder_fields",	"Reorder    Fields ...",	"r","spaf_reorder.hlp",	AD_F_ALL,	AW_POPUP, (AW_CL)create_ad_list_reorder, 0 );
    aws->insert_menu_topic("delete_field",		"Delete/Hide Field ...","D","spaf_delete.hlp",	AD_F_ALL,	AW_POPUP, (AW_CL)create_ad_field_delete, 0 );
    aws->insert_menu_topic("create_field",		"Create Field ...",	"C","spaf_create.hlp",	AD_F_ALL,	AW_POPUP, (AW_CL)create_ad_field_create, 0 );
    aws->insert_menu_topic("unhide_fields",		"Scan Database for all Hidden Fields","S","scandb.hlp",AD_F_ALL,(AW_CB)awt_selection_list_rescan_cb, (AW_CL)gb_main, AWT_NDS_FILTER );
}


//*****************************wird von 'GEN_gene_next_neighbours_create' aus aufgerufen******************************************

void awtc_nn_search(AW_window *aww,AW_CL id ){
  //   GB_transaction dummy(gb_main);
//     int pts = aww->get_root()->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
//     int max_hits = aww->get_root()->awar("next_neighbours/max_hits")->read_int();
//     char *sequence = 0;
//     {
//         char *sel_species = 	aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
//         GBDATA *gb_species =	GBT_find_species(gb_main,sel_species);

//         if (!gb_species){
//             delete sel_species;
//             aw_message("Select a species first");
//             return;
//         }
//         char *ali_name = aww->get_root()->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
//         GBDATA *gb_data = GBT_read_sequence(gb_species,ali_name);
//         if (!gb_data){
//             aw_message(GBS_global_string("Species '%s' has no sequence '%s'",sel_species,ali_name));
//         }
//         delete sel_species;
//         delete ali_name;
//         if (!gb_data) return;
//         sequence = GB_read_string(gb_data);
//     }
    
//     AWTC_FIND_FAMILY ff(gb_main);
//     GB_ERROR error = ff.go(pts,sequence,GB_TRUE,max_hits);
//     delete sequence;
//     AW_selection_list* sel = (AW_selection_list *)id;
//     aww->clear_selection_list(sel);
//     if (error) {
//         aw_message(error);
//         aww->insert_default_selection(sel,"No hits found","");
//     }else{
//         AWTC_FIND_FAMILY_MEMBER *fm;
//         for (fm = ff.family_list; fm; fm = fm->next){
//             const char *dis = GBS_global_string("%-20s Score:%4i",fm->name,fm->matches);
//             aww->insert_selection(sel,(char *)dis,fm->name);
//         }
	
//         aww->insert_default_selection(sel,"No more hits","");
//     }
//     aww->update_selection_list(sel);
}

void awtc_move_hits(AW_window *,AW_CL id, AW_CL cbs){
    awt_copy_selection_list_2_queried_species((struct adaqbsstruct *)cbs,(AW_selection_list *)id);
}

void awtc_nn_search_all_listed(AW_window *aww,AW_CL _cbs  ){
   //  struct adaqbsstruct *cbs = (struct adaqbsstruct *)_cbs;
//     GB_begin_transaction(gb_main);
//     int pts = aww->get_root()->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
//     char *dest_field = aww->get_root()->awar("next_neighbours/dest_field")->read_string();
//     char *ali_name = aww->get_root()->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
//     GB_ERROR error = 0;
//     GB_TYPES dest_type = awt_get_type_of_changekey(gb_main,dest_field);
//     if (!dest_type){
//         error = GB_export_error("Please select a valid field");
//     }
//     long max = awt_count_queried_species(cbs);
    
//     if (strcmp(dest_field, "name")==0) {
//         int answer = aw_message("CAUTION! This will destroy all name-fields of the listed species.\n",
//                                 "Continue and destroy all name-fields,Abort");
	
//         if (answer==1) {
//             error = GB_export_error("Aborted by user");
//         }
//     } 
    
//     aw_openstatus("Finding next neighbours");
//     long count = 0;
//     GBDATA *gb_species;
//     for (gb_species = GBT_first_species(gb_main);
//          !error && gb_species;
//          gb_species = GBT_next_species(gb_species)){
//         if (!IS_QUERIED(gb_species,cbs)) continue;
//         count ++;
//         GBDATA *gb_data = GBT_read_sequence(gb_species,ali_name);
//         if (!gb_data)	continue;
//         if (count %10 == 0){
//             GBDATA *gb_name = GB_search(gb_species,"name",GB_STRING);
//             aw_status(GBS_global_string("Species '%s' (%i:%i)",
//                                         GB_read_char_pntr(gb_name),
//                                         count, max));
//         }
//         if (aw_status(count/(double)max)){
//             error = "operation aborted";
//             break;
//         }
//         {
//             char *sequence = GB_read_string(gb_data);
//             AWTC_FIND_FAMILY ff(gb_main);
//             error = ff.go(pts,sequence,GB_TRUE,1);	
//             if (error) break;
//             {
//                 AWTC_FIND_FAMILY_MEMBER *fm = ff.family_list;
//                 const char *value;
//                 if (fm){
//                     value = GBS_global_string("%i '%s'",fm->matches,fm->name);
//                 }else{
//                     value = "0";
//                 }
//                 GBDATA *gb_dest = GB_search(gb_species,dest_field,dest_type);
//                 error = GB_write_as_string(gb_dest,value);
//             }    
//             delete sequence;
//         }
//     }
//     aw_closestatus();
//     if (error){
//         GB_abort_transaction(gb_main);
//         aw_message(error);
//     }else{
//         GB_commit_transaction(gb_main);
//     }
//     delete dest_field;
//     delete ali_name;
}
//**********************************************************************************************************************************************************************************************

AW_window *GEN_create_gene_window(AW_root *aw_root) {
static AW_window_simple_menu *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aws = new AW_window_simple_menu;
    aws->init( aw_root, "GENE_INFORMATION", "GENE INFORMATION", 0,0,800, 0 );
    aws->load_xfig("gene_info.fig");

    aws->button_length(8);

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");			   

    aws->at("search");
    aws->callback(AW_POPUP, (AW_CL)GEN_create_gene_query_window, 0);
    aws->create_button("SEARCH","SEARCH","S");			   

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"gene_info.hlp");
    aws->create_button("HELP","HELP","H");			   


    AW_CL scannerid = awt_create_arbdb_scanner(gb_main, aws, "box",0,"field","enable",AWT_VIEWER,0,"mark",AWT_NDS_FILTER);
    ad_global_scannerid = scannerid;
    ad_global_scannerroot = aws->get_root();

    aws->create_menu(       0,   "GENE",     "E", "spa_gene.hlp",  AD_F_ALL );
    aws->insert_menu_topic("gene_delete",	"Delete",	"D","genadm_delete.hlp",	AD_F_ALL,	(AW_CB)gene_delete_cb, 0, 0);
    aws->insert_menu_topic("gene_rename",	"Rename ...",	"R","genadm_rename.hlp",	AD_F_ALL,	AW_POPUP, (AW_CL)create_gene_rename_window, 0);
    aws->insert_menu_topic("gene_copy",		"Copy ...",	"C","genadm_copy.hlp",	AD_F_ALL,	AW_POPUP, (AW_CL)create_gene_copy_window, 0);
    aws->insert_menu_topic("gene_create",	"Create ...",	"R","genadm_create.hlp",	AD_F_ALL, 	AW_POPUP, (AW_CL)create_gene_create_window, 0);
    aws->insert_separator();

    aws->create_menu(       0,   "FIELDS",     "I", "gena_fields.hlp",  AD_F_ALL );
    GEN_create_field_items(aws);

    aws->get_root()->awar(AWAR_SPECIES_NAME)->add_callback(	AD_map_species,scannerid);
    aws->get_root()->awar(AWAR_GENE_NAME)->add_callback(	GEN_map_gene,scannerid);
	
    aws->show();
    AD_map_species(aws->get_root(),scannerid);
    return (AW_window *)aws;
}

AW_window *GEN_create_gene_query_window(AW_root *aw_root) {

 static AW_window_simple_menu *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aws = new AW_window_simple_menu;
    aws->init( aw_root, "GEN_QUERY", "SEARCH and QUERY", 0,0,500, 0 );
    aws->create_menu(0,"MORE_FUNCTIONS","F");
    aws->load_xfig("ad_query.fig");


    awt_query_struct awtqs;

    awtqs.gb_main		= gb_main;
    awtqs.gene_name	= AWAR_GENE_NAME;
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
    awtqs.create_view_window= (AW_CL)GEN_create_gene_window;
    AW_CL cbs = (AW_CL)awt_create_query_box((AW_window*)aws,&awtqs);
    ad_query_global_cbs = cbs;
    aws->create_menu(       0,   "MORE_SEARCH",     "S" );
    aws->insert_menu_topic( "search_equal_fields_within_db","Search For Equal Fields and Mark Duplikates",			"E",0,	-1, (AW_CB)awt_search_equal_entries, cbs, 0 );
    aws->insert_menu_topic( "search_equal_words_within_db", "Search For Equal Words Between Fields and Mark Duplikates",	"W",0,	-1, (AW_CB)awt_search_equal_entries, cbs, 1 );
    
    aws->button_length(7);

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");
    aws->at("help");
    aws->callback( AW_POPUP_HELP,(AW_CL)"gene_search.hlp");
    aws->create_button("HELP","HELP","H");			   

    return (AW_window *)aws;

}
