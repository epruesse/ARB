#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include "awtc_rename.hxx"

#include <names_client.h>
#include <servercntrl.h>
#include <client.h>

struct ang_struct {
    aisc_com *link;
    T_AN_LOCAL locs;
    T_AN_MAIN com;
} ang = { 0,0,0} ;

int init_local_com_names()
{
    if (!ang.link) return 1;	/*** create and init local com structure ***/
    if( aisc_create(ang.link, AN_MAIN, ang.com,
		    MAIN_LOCAL, AN_LOCAL, &ang.locs,
		    LOCAL_WHOAMI, "i bin der arb_tree",
		    NULL)){
	return 1;
    }
    return 0;
}

static const char *connect_2_an(GBDATA *gb_main)
{
    char *servername;
    const char *name_server = "ARB_NAME_SERVER";

    if (arb_look_and_start_server(AISC_MAGIC_NUMBER,name_server,gb_main)){
	return "Sorry I can't start the NAME SERVER";
    }
    servername = (char *)GBS_read_arb_tcp(name_server);
    if (!servername) exit (-1);

    ang.link = (aisc_com *)aisc_open(servername, &ang.com,AISC_MAGIC_NUMBER);
    if (init_local_com_names()){
	free(servername);
	return "Sorry I can't start the NAME SERVER";
    }
    free(servername);
    return 0;
}

void disconnect_an(void)
{
    if (ang.link) aisc_close(ang.link);
    ang.link = 0;
}

GB_ERROR generate_one_name(GBDATA *gb_main, const char *full_name, const char *acc, char*& new_name) {
    // create a unique short name for 'full_name'    
    // the result is written into 'new_name' (as malloc-copy) 
    // if fails: GB_ERROR!=0 && new_name==0
    // acc may be 0
    
    new_name = 0;
    if (!acc) acc = "";
    
    aw_openstatus(GBS_global_string("Generating new name for '%s'", full_name)); 
    
    aw_status("Connecting to name server");
    aw_status((double)0);
    
    GB_ERROR err = connect_2_an(gb_main);
    if (err) return err;
    
    aw_status("Generating name");
    static char *shrt = 0;
    if (strlen(full_name)) {
        if (aisc_nput(ang.link, AN_LOCAL, ang.locs,
                      LOCAL_FULL_NAME,	full_name,
                      LOCAL_ACCESSION,	acc,
                      LOCAL_ADVICE,		"",
                      0)){
            err = "Connection Problems with the NAME_SERVER";
        }
        if (aisc_get(ang.link, AN_LOCAL, ang.locs,
                     LOCAL_GET_SHORT,	&shrt,
                     0)){
            err = "Connection Problems with the NAME_SERVER";
        }
    }
    
    disconnect_an();    
    
    if (err) {
        free(shrt);
    }
    else {
        if (shrt) {
            new_name = shrt;
            shrt = 0;
        }
        else {
            err = GB_export_error("Generation of short name for '%s' failed", full_name);
        }
    }
    
    aw_closestatus();
    
    return err;
}


GB_ERROR pars_names(GBDATA *gb_main, int update_status)
{
    GBDATA *gb_species;
    GBDATA *gb_full_name;
    GBDATA *gb_name;
    GBDATA *gb_acc;
    GB_HASH *hash;
    static 	char *shrt;

    char	*full_name;
    char	*name;
    char	*acc;

    GB_ERROR err;
    GB_ERROR err2;

    err = connect_2_an(gb_main);
    if (err) return err;

    err = GBT_begin_rename_session(gb_main,1);
    if (err){
        disconnect_an();
        return err;
    }

    char *ali_name = GBT_get_default_alignment(gb_main);
	
    hash = GBS_create_hash(10024,1);
    err2 = 0;

    long spcount = 0;
    long count = 0;
    if (update_status) spcount = GBT_count_species(gb_main);

    for (	gb_species = GBT_first_species(gb_main);
            gb_species&&!err;
            gb_species = GBT_next_species(gb_species)){
        if (update_status) aw_status(count++/(double)spcount);

        gb_full_name = GB_find(gb_species,"full_name",0,down_level);
        gb_name = GB_find(gb_species,"name",0,down_level);
        gb_acc = GBT_gen_accession_number(gb_species,ali_name);

        if (gb_acc)	acc = GB_read_string(gb_acc);
        else		acc = strdup("");
        if (gb_full_name) full_name = GB_read_string(gb_full_name);
        else		  full_name = strdup("");
        name = GB_read_string(gb_name);
        
        if (strlen(acc) + strlen(full_name) ) {
            if (aisc_nput(ang.link, AN_LOCAL, ang.locs,
                          LOCAL_FULL_NAME,	full_name,
                          LOCAL_ACCESSION,	acc,
                          LOCAL_ADVICE,		name,
                          0)){
                err = "Connection Problems with the NAME_SERVER";
            }
            if (aisc_get(ang.link, AN_LOCAL, ang.locs,
                         LOCAL_GET_SHORT,	&shrt,
                         0)){
                err = "Connection Problems with the NAME_SERVER";
            }
        }else{
            shrt = strdup(name);
        }
        if (!err) {
            //printf("full='%s' short='%s'\n", full_name, shrt);
            
            if (GBS_read_hash(hash,shrt)) {
                char *newshrt;
                int i;
                newshrt = (char *)GB_calloc(sizeof(char),strlen(shrt)+10);
                for (i= 1 ; ; i++) {
                    sprintf(newshrt,"%s.%i",shrt,i);
                    if (!GBS_read_hash(hash,newshrt))break;
                }
                err2 = "There are duplicated entries!!.\nThe duplicated entries contain a '.' character !!!";
                free(shrt); shrt = newshrt;
            }
            GBS_incr_hash(hash,shrt);
            err = GBT_rename_species(name,shrt);
        }
        if (name)	free(name);	name = 0;
        if (acc)	free(acc);	acc = 0;
        if (full_name)	free(full_name);full_name = 0;
        if (shrt)	free(shrt);	shrt = 0;
    }
    disconnect_an();
    delete ali_name; ali_name = 0;

    GBS_free_hash(hash);
    hash = 0;
    if (err) {
        GBT_abort_rename_session();
        return err;
    }else{
        aw_status("Renaming species in trees"); 
        aw_status((double)0);
        GBT_commit_rename_session(aw_status);
    }
    return err2;
}


void awt_rename_cb(AW_window *aww,GBDATA *gb_main)
{
    AWUSE(aww);
    //	int use_advice = (int)aww->get_root()->awar(AWT_RENAME_USE_ADVICE)->read_int();
    //	int save_data = (int)aww->get_root()->awar(AWT_RENAME_SAVE_DATA)->read_int();
    aw_openstatus("Generating new names");
    GB_ERROR error = pars_names(gb_main,1);
    aw_closestatus();
    if (error) aw_message(error);
}


AW_window *create_rename_window(AW_root *root, AW_CL gb_main)
{
    AWUSE(root);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "AUTORENAME_SPECIES", "AUTORENAME SPECIES", 10, 10 );

    aws->load_xfig("awtc/autoren.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"rename.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP","H");

    aws->at("go");
    aws->highlight();
    aws->callback((AW_CB1)awt_rename_cb,gb_main);
    aws->create_button("GO", "GO","G");

    //	aws->at("advice");
    //	aws->create_option_menu(AWT_RENAME_USE_ADVICE,0,0);
    //	aws->insert_option("Create totally new names","n",0);
    //	aws->insert_default_option("Use Advice","U",1);
    //	aws->update_option_menu();

    //	aws->at("save");
    //	aws->create_option_menu(AWT_RENAME_SAVE_DATA,0,0);
    //	aws->insert_option("dont update","f",0);
    //	aws->insert_default_option("update $ARBHOME/lib/nas/names.dat","s",1);
    //	aws->update_option_menu();

    return (AW_window *)aws;
}

void create_rename_variables(AW_root *root,AW_default db1){
    root->awar_int( AWT_RENAME_USE_ADVICE, 0  , 	db1);
    root->awar_int( AWT_RENAME_SAVE_DATA, 1  , 	db1);
}
