#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>

#include <servercntrl.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt_canvas.hxx>

#include <awt_tree.hxx>
#include <awt_dtree.hxx>
#include <awt.hxx>
#include <awti_import.hxx>

#include "ntree.hxx"
#include "nt_cb.hxx"
#include <mg_merge.hxx>
#include <seer.hxx>

AW_HEADER_MAIN

#define NT_SERVE_DB_TIMER 50
#define NT_CHECK_DB_TIMER 200

GBDATA *gb_main;
NT_global nt = { 0,0,0};


void
serve_db_interrupt(AW_root *awr){
    GB_BOOL succes = GBCMS_accept_calls(gb_main,GB_FALSE);
    while (succes){
	awr->check_for_remote_command((AW_default)gb_main,AWAR_NT_REMOTE_BASE);
	succes = GBCMS_accept_calls(gb_main,GB_TRUE);
    }

    awr->add_timed_callback(NT_SERVE_DB_TIMER,(AW_RCB)serve_db_interrupt,0,0);
}

void
check_db_interrupt(AW_root *awr){
    awr->check_for_remote_command((AW_default)gb_main,AWAR_NT_REMOTE_BASE);
    awr->add_timed_callback(NT_CHECK_DB_TIMER,(AW_RCB)check_db_interrupt,0,0);
}

GB_ERROR create_nt_window(AW_root *aw_root){
	AW_window *aww;
	GB_ERROR error = GB_request_undo_type(gb_main, GB_UNDO_NONE);
	if (error) aw_message(error);
	create_all_awars(aw_root,AW_ROOT_DEFAULT);
	if (GB_NOVICE){
	    aw_root->set_sensitive(AWM_BASIC);
	}
	aww = create_nt_main_window(aw_root,0);
	aww->show();
	error = GB_request_undo_type(gb_main, GB_UNDO_UNDO);
	if (error) aw_message(error);
	return error;
}

// after intro
void nt_main_startup_main_window(AW_root *aw_root){
    create_nt_window(aw_root);

    if (GB_read_clients(gb_main)==0) {
        GB_ERROR error = GBCMS_open(":",0,gb_main);
        if (error) {
            aw_message(
                       "THIS PROGRAMM HAS PROBLEMS TO OPEN INTERCLIENT COMMUNICATION !!!\n"
                       "(MAYBE THERE IS A SERVER RUNNING ALREADY)\n\n"
                       "You cannot use any EDITOR or other external SOFTWARE with this dataset!");
        }else{
            aw_root->add_timed_callback(NT_SERVE_DB_TIMER,(AW_RCB)serve_db_interrupt,0,0);
        }
    }else{
        aw_root->add_timed_callback(NT_CHECK_DB_TIMER,(AW_RCB)check_db_interrupt,0,0);
    }
}

int main_load_and_startup_main_window(AW_root *aw_root)	// returns 0 when successfull
{


	char *db_server = aw_root->awar(AWAR_DB_PATH)->read_string();
	gb_main = GBT_open(db_server,"rw","$(ARBHOME)/lib/pts/*");

	if (!gb_main) {
		aw_message(GB_get_error(),"OK");
		return -1;
	}

	aw_root->awar(AWAR_DB_PATH)->write_string(db_server);
	delete db_server;
	nt_main_startup_main_window(aw_root);


	return 0;
}

void nt_delete_database(AW_window *aww){
    char *db_server = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
    if (strlen(db_server)){
	if (aw_message(GBS_global_string("Are you sure to delete database %s\nNote: there is no way to undelete it afterwards",db_server),"Delete Database,Cancel") == 0){
	    GB_ERROR error = 0;
	    error = GB_delete_database(db_server);
	    if (error){
		aw_message(error);
	    }else{
		aww->get_root()->awar(AWAR_DB"filter")->touch();
	    }
	}
    }else{
	aw_message("No database selected");
    }
    delete db_server;
}

		// after import !!!!!
void main3(AW_root *aw_root)
	{

	nt.awr = aw_root;
	create_nt_window(aw_root);

	if (GB_read_clients(gb_main)==0) {
		GB_ERROR error = GBCMS_open(":",0,gb_main);
		if (error) {
			aw_message("THIS PROGRAMM IS NOT THE ONLY DB SERVER !!!\nDONT USE ANY ARB PROGRAMM !!!!");
		}else{
			aw_root->add_timed_callback(NT_SERVE_DB_TIMER,
				(AW_RCB)serve_db_interrupt,0,0);
		}
	}
	return;
}




void nt_intro_start_old(AW_window *aws)
	{
	aws->hide();
	if (main_load_and_startup_main_window(aws->get_root())) {
		aws->show();
	}
}

void nt_intro_start_merge(AW_window *aws,AW_root *awr){
    if (aws) awr = aws->get_root();
    create_MG_main_window(awr);
    if (aws) aws->hide();
}

void nt_intro_start_import(AW_window *aws)
	{
	aws->hide();
	aws->get_root()->awar_string( AWAR_DB_PATH )->write_string( "noname.arb");
	aws->get_root()->awar_int(AWAR_READ_GENOM_DB, 2); // Default toggle  in window  "Create&import" is Non-Genom
	gb_main = open_AWTC_import_window(aws->get_root(),"",1,(AW_RCB)main3,0,0);
}

AW_window *nt_create_intro_window(AW_root *awr)
	{
	AW_window_simple *aws = new AW_window_simple;
	aws->init( awr, "ARB_INTRO", "ARB INTRO", 400, 100 );
	aws->load_xfig("arb_intro.fig");

	aws->callback( (AW_CB0)exit);
	aws->at("close");
	aws->create_button("CANCEL","CANCEL","C");

	aws->at("help");
	aws->callback(AW_POPUP_HELP,(AW_CL)"arb_intro.hlp");
	aws->create_button("HELP","HELP","H");

	awt_create_selection_box(aws,"tmp/nt/arbdb");

	aws->button_length(0);

	aws->at("logo");
	aws->create_button(0,"#logo.bitmap");


// 	aws->button_length(25);

	aws->at("old");
	aws->callback(nt_intro_start_old);
	aws->create_autosize_button("OPEN_SELECTED","OPEN SELECTED","O");

	aws->at("del");
	aws->callback(nt_delete_database);
	aws->create_autosize_button("DELETE_SELECTED","DELETE SELECTED");

	aws->at("new_complex");
	aws->callback(nt_intro_start_import);
	aws->create_autosize_button("CREATE_AND_IMPORT","CREATE AND IMPORT","I");

	aws->at("merge");
	aws->callback((AW_CB1)nt_intro_start_merge,0);
	aws->create_autosize_button("MERGE_TWO_DATABASES","MERGE TWO ARB DATABASES","O");

	aws->at("novice");
	aws->create_toggle("NT/GB_NOVICE");

	return (AW_window *)aws;
}

void AD_set_default_root(AW_root *aw_root);

//  ----------------------------------------------------------
//      static void AWAR_DB_PATH_changed_cb(AW_root *awr)
//  ----------------------------------------------------------
static void AWAR_DB_PATH_changed_cb(AW_root *awr) {
    char    *value        = awr->awar(AWAR_DB_PATH)->read_string();
    char    *lslash       = strrchr(value, '/');

    lslash = lslash ? lslash+1 : value;
    awr->awar(AWAR_DB_NAME)->write_string(lslash);
}

//  ---------------------------------------
//      int main(int argc, char **argv)
//  ---------------------------------------
int main(int argc, char **argv)
{
	AW_root *aw_root;
	AW_default aw_default;

	const char *db_server =":";

	unsigned long mtime = GB_time_of_file("$(ARBHOME)/lib/message");
	unsigned long rtime = GB_time_of_file("$(HOME)/.arb_prop/msgtime");
	if (mtime > rtime){
	    GB_edit("${ARBHOME}/lib/message");
	    system("touch ${HOME}/.arb_prop/msgtime");
	}
	aw_initstatus();
	GB_set_verbose();

	aw_root = new AW_root;
	nt.awr  = aw_root;
    AD_set_default_root(aw_root); // set default for AD_map_viewer (as long as no info-box was opened)
	aw_default = aw_root->open_default(".arb_prop/ntree.arb");
	aw_root->init_variables(aw_default);
	aw_root->init("ARB_NT");

	aw_root->awar_string( AWAR_DB_PATH, "noname.arb", aw_default);
	aw_root->awar_string( AWAR_DB"directory", "", aw_default);
	aw_root->awar_string( AWAR_DB"filter", "arb", aw_default);
	aw_root->awar_string( AWAR_DB"type", "b", aw_default);
	aw_root->awar_int( "NT/GB_NOVICE", 		0, aw_default)	->add_target_var(&GB_NOVICE);

    aw_root->awar_string(AWAR_DB_NAME, "noname.arb", aw_default);
    aw_root->awar(AWAR_DB_PATH)->add_callback(AWAR_DB_PATH_changed_cb);

	if (argc==3) {		// looks like merge
	    MG_create_all_awars(aw_root,aw_default,argv[1],argv[2]);
	    nt_intro_start_merge(0,aw_root);
	    aw_root->main_loop();
	}
	printf("argc=%i\n",argc);
	if (argc>=2) {
        if (strcmp(argv[1], "--help")==0 || strcmp(argv[1], "-h")==0) {
            fprintf(stderr,
                    "\n"
                    "arb_ntree commandline reference:\n"
                    "--------------------------------\n"
                    "\n"
                    "db.arb             => start ARB_NTREE with database db.arb\n"
                    "db1.arb db2.arb    => merge databases db1.arb and db2.arb\n"
                    "-export            => connect to existing ARB server and export database to noname.arb\n"
                    "w/o arguments      => start database browser\n"
                    "\n"
                    );

            exit(1);
        }


		if ( strcmp(argv[1],"-export")==0) {
			MG_create_all_awars(aw_root,aw_default,":","noname.arb");
			gb_merge = GBT_open(":","rw",0);
			if (!gb_merge) {
				aw_message(GB_get_error(),"OK");
				exit(0);
			}
			gb_dest = GBT_open("noname.arb","cw",0);

			MG_start_cb2(0,aw_root);
			aw_root->main_loop();
		}

		db_server = argv[1];
		if (GBT_check_arb_file(db_server)) {
			char msg[1024];
			sprintf(msg,"Your file is not an original arb file\n%s",
                    GB_get_error());
			switch(aw_message(msg,	"Continue (dangerous),Start Converter,Exit")) {
				case 0:	break;
				case 2: return 0;
				case 1:
                    aw_root->awar_int(AWAR_READ_GENOM_DB, 2);
					gb_main = open_AWTC_import_window(aw_root,db_server, 1,(AW_RCB)main3,0,0);
					aw_root->main_loop();
				default:	break;
			}
		}
	}else{
	    char *latest = GB_find_latest_file(".","/\\.a[r0-9][b0-9]$/");
	    if (latest){
            int l = strlen(latest);
            latest[l-1] = 'b';
            latest[l-2] = 'r';
            latest[l-3] = 'a';
            aw_root->awar(AWAR_DB_PATH)->write_string(latest);
            delete latest;
	    }
	    AW_window *iws;
	    if (nt.window_creator){
            iws = nt.window_creator(aw_root,0);
	    }else{
            iws = nt_create_intro_window(aw_root);
	    }
	    iws->show();
	    aw_root->main_loop();
	}

	aw_root->awar(AWAR_DB_PATH)->write_string(db_server);

	if (main_load_and_startup_main_window(aw_root)) return -1;
	aw_root->main_loop();

	return 0;
}
