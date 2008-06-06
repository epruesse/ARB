#include <stdio.h>
#include <memory.h>
#include <stdlib.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_canvas.hxx>
#include <awt_tree.hxx>
#include <awt_dtree.hxx>


#include <ntree.hxx>
#include <seer.hxx>
#include <seer_interface.hxx>

SEER_GLOBAL seer_global;

void SEER_query_seer_cb(AW_window *aww){
    aw_openstatus("Query Database");
    char *alignment_name     = aww->get_root()->awar(AWAR_SEER_ALIGNMENT_NAME)->read_string();
    char *attribute_name     = aww->get_root()->awar(AWAR_SEER_QUERY_ATTRIBUTE)->read_string();
    char *query_string       = aww->get_root()->awar(AWAR_SEER_QUERY_STRING)->read_string();
    int   load_sequence_flag = aww->get_root()->awar(AWAR_SEER_LOAD_SEQUENCES)->read_int();

    GB_ERROR error = SEER_query(alignment_name,attribute_name,query_string,load_sequence_flag);
    delete alignment_name;
    delete attribute_name;
    delete query_string;
    aw_closestatus();
    if (error){
        aw_message(error);
    }else{
        seer_global.query_seer_window->hide();
    }
}

void SEER_load_marked(AW_window *aww){
    char *alignment_name     = aww->get_root()->awar(AWAR_SEER_ALIGNMENT_NAME)->read_string();
    int   load_sequence_flag = aww->get_root()->awar(AWAR_SEER_LOAD_SEQUENCES)->read_int();
    SEER_load_marked(alignment_name,load_sequence_flag);
    delete alignment_name;
}

void SEER_create_skeleton_for_tree_cb(AW_window *){
    GB_begin_transaction(gb_main);
    if (nt.tree && nt.tree->tree_root){
        SEER_create_skeleton_for_tree((GBT_TREE *)nt.tree->tree_root);
    }else{
        aw_message("Please select a valid tree");
    }
    GB_commit_transaction(gb_main);
}

long seer_extract_attributes_to_query(const char *,long val){
    SeerInterfaceAttribute *siat = (SeerInterfaceAttribute *)val;
    if (siat->queryable){
        char buffer[256];
        if (siat->requested){
            sprintf(buffer,"* %s",siat->name);
        }else{
            sprintf(buffer,"  %s",siat->name);
        }
        seer_global.query_seer_window->insert_selection(seer_global.tmp_selection_list,buffer,siat->name);
    }
    return val;
}

AW_window *SEER_query_seer_and_get(AW_root *aw_root){
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SEER_QUERY", "QUERY SEER", 400, 100 );
    seer_global.query_seer_window = aws;

    aws->load_xfig("seer/query_seer.fig");

    aws->callback( AW_POPDOWN);
    aws->at("close");
    aws->create_button("CANCEL","CANCEL","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"seer/query_seers.hlp");
    aws->create_button("HELP","HELP","H");

    aws->button_length(0);
    aws->at("logo");
    aws->create_button(0,"#seer/seer.bitmap");

    aws->button_length(20);

    aw_root->awar_string(AWAR_SEER_QUERY_ATTRIBUTE,"id");
    aw_root->awar_string(AWAR_SEER_QUERY_STRING,"");

    aws->at("attributes");
    AW_selection_list *sellst = aws->create_selection_list(AWAR_SEER_QUERY_ATTRIBUTE);
    seer_global.tmp_selection_list = sellst;

    GBS_hash_do_sorted_loop(seer_global.attribute_hash,seer_extract_attributes_to_query,seer_sort_attributes);
    aws->insert_default_selection(sellst,"","");
    aws->update_selection_list(sellst);

    aws->at("query_string");
    aws->create_input_field(AWAR_SEER_QUERY_STRING);

    aws->at("go");
    aws->callback(SEER_query_seer_cb);
    aws->create_button("QUERY_SEER_AND_GET","QUERY AND GET");

    return aws;
};

void SEER_strip_and_save(AW_window *aww){
    if (aw_message("Are you sure to delete all\n"
                   "     species, SAI\n","DELETE DATA,CANCEL") == 1){
        return;
    }
    SEER_strip_arb_file();
    NT_create_save_as(aww->get_root(),"tmp/nt/arbdb")->show(); // Create save whole database as window
}

void seer_exit(AW_window *){
    if (gb_main) {
        if (aw_message(AW_ERROR_BUFFER,"QUIT SEER,DO NOT QUIT")) return;
    }
    seer_global.interface->endSession();
    exit(0);
}

AW_window *SEER_populate_tables_and_open_main_panels(AW_root *aw_root){

    {/* mark attributes as requested ***/
        const char *attributes;
        for (attributes = seer_global.attribute_sellection_list->first_selected();
             attributes;
             attributes = seer_global.attribute_sellection_list->next_element()){
            SeerInterfaceAttribute *siat = (SeerInterfaceAttribute *)GBS_read_hash(seer_global.attribute_hash,attributes);
            siat->requested = 1;
        }
    }

    GB_ERROR error = SEER_populate_tables(aw_root);
    if (error){
        aw_message(error,"OK");
        SEER_logout();
    }


    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SEER_MAIN", "SEER MAIN", 400, 100 );

    aws->auto_space(0,10);
    aws->button_length(25);

    aws->create_button("HELP","#seer/seer.bitmap","H");
    aws->at_newline();

    aws->callback(AW_POPUP_HELP,(AW_CL)"seer/main_help.hlp");
    aws->create_button("HELP","HELP","H");
    aws->at_newline();

    aws->callback(SEER_create_skeleton_for_tree_cb);
    aws->create_button("REVIVE_ZOMBIES","CREATE SKELETON SPECIES","H");
    aws->at_newline();

    aws->callback(SEER_load_marked);
    aws->create_button("GET_MARKED","GET MARKED FROM SEERS","H");
    aws->at_newline();

    aws->callback(AW_POPUP,(AW_CL)SEER_query_seer_and_get,0);
    aws->create_button("QUERY_GET","QUERY SEERS AND GET","H");
    aws->at_newline();

    aws->callback(SEER_upload_to_seer_create_window);
    aws->create_button("SAVE","SAVE SOME TO SEERS","H");
    aws->at_newline();

    aws->callback(SEER_strip_and_save);
    aws->create_button("STRIP_SKELETON","STRIP AND SAVE","H");
    aws->at_newline();

    aws->callback(seer_exit);
    aws->create_button("QUIT","QUIT","Q");
    aws->at_newline();


    nt_main_startup_main_window(aw_root);
    aws->window_fit();

    seer_global.seer_select_attributes_window->hide();
    return aws;
}

void seer_select_all_attributes(AW_window *,AW_CL selectFlag){
    if (selectFlag){
        seer_global.attribute_sellection_list->selectAll();
    }else{
        seer_global.attribute_sellection_list->deselectAll();
    }
}

AW_window *SEER_load_arb_and_create_selector_panel(AW_root *aw_root,SEER_OPENS_ARB_TYPE open_type){


    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SEER_SELECT_ATTRIBUTES", "SEER SELECT ATTRIBUTES", 400, 100 );
    aws->load_xfig("seer/select_attributes.fig");


    aws->callback( (AW_CB0)SEER_logout);
    aws->at("close");
    aws->create_button("CANCEL","CANCEL","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"seer/select_attributes.hlp");
    aws->create_button("HELP","HELP","H");

    aws->button_length(0);
    aws->at("logo");
    aws->create_button(0,"#seer/seer.bitmap");

    seer_global.interface->beginReadOnlyTransaction(); {
        /* ************ Read the alignment list *************/
        {
            aw_root->awar_string(AWAR_SEER_ALIGNMENT_NAME,"");
            aws->at("alignments");
            AW_selection_list *sellist = aws->create_selection_list(AWAR_SEER_ALIGNMENT_NAME);
            SeerInterfaceAlignment *al;
            SeerInterfaceAlignment *last_al;
            for (al = seer_global.interface->firstAlignment();
                 al;
                 al = seer_global.interface->nextAlignment()){
                aws->insert_selection(sellist,al->name,al->name);
                GBS_write_hash(seer_global.alignment_hash,al->name,(long)al);
                last_al = al;
            }
            if (last_al){
                aw_root->awar(AWAR_SEER_ALIGNMENT_NAME)->write_string(last_al->name);
            }

            aws->insert_default_selection(sellist,"no alignment selected","");
            aws->update_selection_list(sellist);
        }

        /* ************ Read the table list *************/
        {
            char at_buffer[256];
            char awar_buffer[256];
            SeerInterfaceTableDescription *td;
            int table_nr = 0;
            for (td = seer_global.interface->firstTableDescription();
                 td;
                 td = seer_global.interface->nextTableDescription()){
                td->sort_key = table_nr;
                sprintf(at_buffer,"table_%i",table_nr);
                sprintf(awar_buffer,"%s/%s",AWAR_SEER_TABLE_PREFIX,td->tablename);

                aws->at(at_buffer);
                aw_root->awar_int(awar_buffer,1);
                aws->create_toggle(awar_buffer);
                GBS_write_hash(seer_global.table_hash,td->tablename,(long)td);
                td->attribute_hash = GBS_create_hash(256, GB_MIND_CASE);
                SeerInterfaceAttribute *at;

                for (at = td->firstAttribute();
                     at;
                     at = td->nextAttribute()){
                    GBS_write_hash(td->attribute_hash,at->name,(long)at);
                }
            }
        }
        /* ************ Read the attribute list *************/
        {
            aws->at("attributes");
            SeerInterfaceAttribute *at;
            AW_selection_list *sellst = aws->create_multi_selection_list();
            seer_global.attribute_sellection_list = sellst;
            for (at = seer_global.interface->firstAttribute();
                 at;
                 at = seer_global.interface->nextAttribute()){
                aws->insert_selection(sellst,at->name,at->name);
                GBS_write_hash(seer_global.attribute_hash,at->name,(long)at);
            }
            aws->update_selection_list(sellst);
        }
        aws->at("select");
        aws->callback(seer_select_all_attributes,1);
        aws->create_button("INVERT_ALL_ATTRIBUTES","Invert All");

        aws->at("deselect");
        aws->callback(seer_select_all_attributes,0);
        aws->create_button("DESELECT_ALL_ATTRIBUTES","Deselect All");

    }   seer_global.interface->commitReadOnlyTransaction();

    aw_root->awar_int(AWAR_SEER_TABLE_LOAD_BEHAVIOUR,SEER_TABLE_LOAD_AT_STARTUP);
    aw_root->awar_int(AWAR_SEER_LOAD_SEQUENCES,1);
    aws->at("table_load");
    aws->create_toggle(AWAR_SEER_TABLE_LOAD_BEHAVIOUR);

    aws->at("sequence_load");
    aws->create_toggle(AWAR_SEER_LOAD_SEQUENCES);

    aws->at("go");
    aws->button_length(15);
    aws->callback(AW_POPUP,(AW_CL)SEER_populate_tables_and_open_main_panels,0);
    aws->create_button("SEER_GO_TO_ARB","CONTINUE");

    seer_global.arb_login_window->hide();
    SEER_opens_arb(aw_root,open_type);
    seer_global.seer_select_attributes_window = aws;
    return aws;
}

AW_window *seer_login_and_create_arb_load_window(AW_root *awr){
    {
        char     *username   = awr->awar(AWAR_SEER_USER_NAME)->read_string();
        char     *userpasswd = awr->awar(AWAR_SEER_USER_PASSWD)->read_string();
        GB_ERROR  error      = SEER_open(username,userpasswd);
        if (error){
            aw_message(error,"OK");
            exit(-1);
        }
        delete userpasswd;
        delete username;
    }

    AW_window_simple *aws = new AW_window_simple;
    aws->init( awr, "LOAD_ARB_SKELETON", "LOAD_ARB_SKELETON", 400, 100 );
    aws->load_xfig("seer/arb_intro.fig");

    aws->callback( (AW_CB0)SEER_logout);
    aws->at("close");
    aws->create_button("CANCEL","CANCEL","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"arb_intro.hlp");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box(aws,"tmp/nt/arbdb");

    aws->button_length(0);

    aws->at("logo");
    //    aws->create_button(0,"#seer/seer.bitmap");
    aws->create_button(0,"#logo.xpm");


    aws->button_length(20);

    aws->at("old");
    aws->callback(AW_POPUP,(AW_CL)SEER_load_arb_and_create_selector_panel,(AW_CL)SEER_OPENS_ARB_FULL);
    aws->create_button("OPEN_SELECTED","OPEN SELECTED","O");

    aws->at("skeleton");
    aws->callback(AW_POPUP,(AW_CL)SEER_load_arb_and_create_selector_panel,(AW_CL)SEER_OPENS_ARB_SKELETON);
    aws->create_button("OPEN_SKELETON","OPEN + STRIP","O");

    aws->at("new");
    aws->callback(AW_POPUP,(AW_CL)SEER_load_arb_and_create_selector_panel,(AW_CL)SEER_OPENS_NEW_ARB);
    aws->create_button("CREATE_NEW","CREATE NEW","O");

    seer_global.arb_login_window = aws;
    seer_global.login_window->hide();
    aw_openstatus("Read Additional data from ARB");
    seer_global.interface->writeToArbDirectly(gb_main);
    aw_closestatus();

    return (AW_window *)aws;
}

AW_window *SEER_create_login_window(AW_root *aw_root,AW_CL cd1){
    AWUSE(cd1);
    aw_root->awar_string(AWAR_SEER_USER_NAME,"guest");
    aw_root->awar_string(AWAR_SEER_USER_PASSWD);
    memset((char *)&seer_global,0,sizeof(seer_global));
    AW_window_simple *aws = new AW_window_simple();

    aws->init( aw_root, "SEER_LOGIN", "SEER LOGIN", 400, 100 );
    aws->load_xfig("seer/login.fig");

    aws->callback( (AW_CB0)exit);
    aws->at("close");
    aws->create_button("CANCEL","CANCEL","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"seer/login.hlp");
    aws->create_button("HELP","HELP","H");

    aws->button_length(0);

    aws->at("logo");
    aws->create_button(0,"#seer/seer.bitmap");


    //      aws->button_length(25);

    aws->at("passwd");
    aws->create_input_field(AWAR_SEER_USER_PASSWD);

    aws->at("name");
    aws->create_input_field(AWAR_SEER_USER_NAME);

    aws->at("login");
    aws->callback(AW_POPUP,(AW_CL)seer_login_and_create_arb_load_window,0);
    aws->create_button(0,"#seer/login.bitmap");

    nt.extern_quit_button = AW_TRUE;
    seer_global.login_window = aws;
    return (AW_window *)aws;
}

NT_install_window_creator seer_dummy_global(SEER_create_login_window);
