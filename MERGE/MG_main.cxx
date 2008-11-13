#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <aw_global.hxx>
#include <awt.hxx>
#include <AW_rename.hxx>

#include "merge.hxx"

GBDATA *GLOBAL_gb_merge = NULL;
GBDATA *GLOBAL_gb_dest  = NULL;

void MG_exit(AW_window *aww, AW_CL cl_reload_db2, AW_CL) {
    int reload_db2 = (int)cl_reload_db2;
    if (GLOBAL_gb_main) { // running inside normal ARB (aka import)
        mg_assert(reload_db2 == 0);
        aww->hide();
    }
    else {
        if (reload_db2) {
            char       *db2_name = aww->get_root()->awar(AWAR_MAIN_DB"/file_name")->read_string();
            const char *cmd      = GBS_global_string("arb_ntree '%s' &", db2_name);
            int         result   = system(cmd);
            if (result != 0) fprintf(stderr, "Error running '%s'\n", cmd);
            free(db2_name);
        }
        exit (0);
    }
}

bool mg_save_enabled = true;

void MG_save_merge_cb(AW_window *aww)
{
    char *name = aww->get_root()->awar(AWAR_MERGE_DB"/file_name")->read_string();
    GB_begin_transaction(GLOBAL_gb_merge);
    GBT_check_data(GLOBAL_gb_merge,0);
    GB_commit_transaction(GLOBAL_gb_merge);
    GB_ERROR error = GB_save(GLOBAL_gb_merge, name, "b");
    if (error) aw_message(error);
    else awt_refresh_selection_box(aww->get_root(), AWAR_MERGE_DB);
    delete name;
}

AW_window *MG_save_source_cb(AW_root *aw_root, char *base_name)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "MERGE_SAVE_DB_I", "SAVE ARB DB I");
    aws->load_xfig("sel_box.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("save");aws->callback(MG_save_merge_cb);
    aws->create_button("SAVE","SAVE","S");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("cancel");
    aws->create_button("CLOSE","CANCEL","C");

    awt_create_selection_box((AW_window *)aws,base_name);

    return (AW_window *)aws;
}

void MG_save_cb(AW_window *aww)
{
    char *name = aww->get_root()->awar(AWAR_MAIN_DB"/file_name")->read_string();
    char *type = aww->get_root()->awar(AWAR_MAIN_DB"/type")->read_string();
    aw_openstatus("Saving database");
    GB_begin_transaction(GLOBAL_gb_dest);
    GBT_check_data(GLOBAL_gb_dest,0);
    GB_commit_transaction(GLOBAL_gb_dest);
    GB_ERROR error = GB_save(GLOBAL_gb_dest, name, type);
    aw_closestatus();
    if (error) aw_message(error);
    else    awt_refresh_selection_box(aww->get_root(), AWAR_MAIN_DB);
    delete name;
    delete type;
}

AW_window *MG_save_result_cb(AW_root *aw_root, char *base_name)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;
    aw_root->awar_string( AWAR_DB_COMMENT, "<no description>", GLOBAL_gb_dest);

    aws = new AW_window_simple;
    aws->init( aw_root, "MERGE_SAVE_WHOLE_DB", "SAVE WHOLE DATABASE");
    aws->load_xfig("sel_box.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    awt_create_selection_box((AW_window *)aws,base_name);

    aws->at("user");
    aws->label("Type");
    aws->create_option_menu(AWAR_MAIN_DB"/type");
    aws->insert_option("Binary","B","b");
    aws->insert_option("Bin (with FastLoad File)","f","bm");
    aws->update_option_menu();

    aws->at("user2");
    aws->create_button(0,"Database Description");



    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("cancel4");
    aws->create_button("CLOSE","CANCEL","C");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("cancel4");
    aws->create_button("CLOSE", "CANCEL","C");

    aws->at("save4");aws->callback(MG_save_cb);
    aws->create_button("SAVE","SAVE","S");


    aws->at("user3");
    aws->create_text_field(AWAR_DB_COMMENT);

    return (AW_window *)aws;
}

void
MG_save_quick_cb(AW_window *aww)
{
    char *name = aww->get_root()->awar(AWAR_MAIN_DB"/file_name")->read_string();
    aw_openstatus("Saving database");
    GB_begin_transaction(GLOBAL_gb_dest);
    GBT_check_data(GLOBAL_gb_dest,0);
    GB_commit_transaction(GLOBAL_gb_dest);
    GB_ERROR error = GB_save_quick_as(GLOBAL_gb_dest, name);
    aw_closestatus();
    if (error) aw_message(error);
    else    awt_refresh_selection_box(aww->get_root(), AWAR_MAIN_DB);
    delete name;
}

AW_window *MG_save_quick_result_cb(AW_root *aw_root, char *base_name)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "SAVE_CHANGES_OF_DB_II_AS", "SAVE CHANGES TO DB2 AS");
    aws->load_xfig("sel_box.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("save");aws->callback(MG_save_quick_cb);
    aws->create_button("SAVE","SAVE","S");

    awt_create_selection_box((AW_window *)aws,base_name);

    return (AW_window *)aws;
}


static void MG_create_db_dependent_awars(AW_root *aw_root, GBDATA *gb_merge, GBDATA *gb_dest) {
    MG_create_db_dependent_rename_awars(aw_root, gb_merge, gb_dest);
}

static void MG_popup_if_renamed(AW_window *aww, AW_CL cl_create_window) {
    GB_ERROR error = MG_expect_renamed();
    if (!error) {
        static GB_HASHI *popup_hash = GBS_create_hashi(10);
        AW_window       *aw_popup   = (AW_window*)GBS_read_hashi(popup_hash, cl_create_window);

        if (!aw_popup) { // window has not been created yet
            typedef AW_window *(*window_creator)(AW_root *);
            window_creator create_window = (window_creator)cl_create_window;
            aw_popup                     = create_window(aww->get_root());
            GBS_write_hashi(popup_hash, cl_create_window, (long)aw_popup);
        }
        
        aw_popup->show();
    }

    if (error) aw_message(error);
}

// uses gb_dest and gb_merge
void MG_start_cb2(AW_window *aww,AW_root *aw_root, bool save_enabled, bool dest_is_new) {
    GB_ERROR error = 0;

    mg_save_enabled = save_enabled;

    {
        GB_transaction ta_merge(GLOBAL_gb_merge);
        GB_transaction ta_dest(GLOBAL_gb_dest);

        GBT_mark_all(GLOBAL_gb_dest,0); // unmark everything in dest DB

        // set DB-type to non-genome (compatibility to old DBs)
        // when exporting to new DB (dest_is_new == true) -> use DB-type of merge-DB
        bool merge_is_genome = GEN_is_genome_db(GLOBAL_gb_merge, 0);

        int dest_genome = 0;
        if (dest_is_new) {
            if (merge_is_genome) {
                dest_genome = aw_question("Enter destination DB-type", "Normal,Genome");
            }
            else {
                dest_genome = 0; // from non-genome -> to non-genome
            }
        }

        GEN_is_genome_db(GLOBAL_gb_dest, dest_genome); // does not change anything if type is already defined
    }

    if (!error) {
        GB_transaction ta_merge(GLOBAL_gb_merge);
        GB_transaction ta_dest(GLOBAL_gb_dest);
            
        GB_change_my_security(GLOBAL_gb_dest,6,"passwd");
        GB_change_my_security(GLOBAL_gb_merge,6,"passwd");
        if (aww) aww->hide();

        MG_create_db_dependent_awars(aw_root, GLOBAL_gb_merge, GLOBAL_gb_dest);
    }

    if (!error) {
        GB_transaction ta_merge(GLOBAL_gb_merge);
        GB_transaction ta_dest(GLOBAL_gb_dest);

        MG_set_renamed(false, aw_root, "Not renamed yet.");

        AW_window_simple_menu *awm = new AW_window_simple_menu();
        awm->init(aw_root,"ARB_MERGE", "ARB_MERGE");
        awm->load_xfig("merge/main.fig");

        awm->create_menu(       0,   "File",     "F", "merge_file.hlp",  AWM_ALL );
#if defined(DEBUG)
        awm->insert_menu_topic("db_browser", "Browse loaded database(s)", "", "db_browser.hlp", AWM_ALL, AW_POPUP, (AW_CL)AWT_create_db_browser, 0);
        awm->insert_separator();
#endif // DEBUG
        if (mg_save_enabled && GB_read_clients(GLOBAL_gb_merge)>=0) {
            awm->insert_menu_topic("save_DB1","Save Data Base I ...",       "S","save_as.hlp",AWM_ALL, AW_POPUP, (AW_CL)MG_save_source_cb,(AW_CL)AWAR_MERGE_DB );
        }

        awm->insert_menu_topic("quit","Quit",               "Q","quit.hlp", AWM_ALL, MG_exit, 0, 0 );
        if (mg_save_enabled) {
            awm->insert_menu_topic("quit","Quit & Start DB II", "D","quit.hlp", AWM_ALL, MG_exit, 1, 0 );
        }

        awm->insert_menu_topic("save_props","Save properties (to ~/.arb_prop/ntree.arb)",               "p","savedef.hlp",  AWM_ALL,      (AW_CB)AW_save_defaults, 0, 0 );

        awm->button_length(30);

        if (GB_read_clients(GLOBAL_gb_merge)>=0){ // merge 2 database
            awm->at("alignment");
            awm->callback((AW_CB1)AW_POPUP,(AW_CL)MG_merge_alignment_cb);
            awm->help_text("mg_alignment.hlp");
            awm->create_button("CHECK_ALIGNMENTS", "Check Alignments ...");

            awm->at("names");
            awm->callback((AW_CB1)AW_POPUP,(AW_CL)MG_merge_names_cb);
            awm->help_text("mg_names.hlp");
            awm->create_button("CHECK_NAMES", "Check Names ...");
        }
        else { // export into new database
            MG_set_renamed(true, aw_root, "Not necessary"); // a newly created database needs no renaming
        }

        awm->at("species");
        awm->callback(MG_popup_if_renamed, (AW_CL)MG_merge_species_cb);
        awm->help_text("mg_species.hlp");
        awm->create_button("TRANSFER_SPECIES", "Transfer Species ... ");

        awm->at("extendeds");
        awm->callback((AW_CB1)AW_POPUP,(AW_CL)MG_merge_extendeds_cb);
        awm->help_text("mg_extendeds.hlp");
        awm->create_button("TRANSFER_SAIS", "Transfer SAIs ...");

        awm->at("trees");
        awm->callback(MG_popup_if_renamed, (AW_CL)MG_merge_trees_cb);
        awm->help_text("mg_trees.hlp");
        awm->create_button("TRANSFER_TREES", "Transfer Trees ...");

        awm->at("configs");
        awm->callback(MG_popup_if_renamed, (AW_CL)MG_merge_configs_cb);
        awm->help_text("mg_configs.hlp");
        awm->create_button("TRANSFER_CONFIGS", "Transfer Configurations ...");

        if (mg_save_enabled && GB_read_clients(GLOBAL_gb_dest)>=0){       // No need to save when importing data
            awm->at("save");
            awm->callback(AW_POPUP,(AW_CL)MG_save_result_cb,(AW_CL)AWAR_MAIN_DB);
            awm->create_button("SAVE_WHOLE_DB2", "Save Whole DB II ...");

            awm->at("save_quick");
            awm->highlight();
            awm->callback(AW_POPUP,(AW_CL)MG_save_quick_result_cb,(AW_CL)AWAR_MAIN_DB);
            awm->create_button("SAVE_CHANGES_OF_DB2", "Save Changes of DB II as ...");
        }

        awm->button_length(15);

        awm->at("db1");
        awm->create_button(0,AWAR_MERGE_DB"/file_name");

        awm->at("db2");
        awm->create_button(0,AWAR_MAIN_DB"/file_name");

        awm->button_length(0);
        awm->shadow_width(1);
        awm->at("icon");
        awm->callback(AW_POPUP_HELP, (AW_CL)"mg_main.hlp");
        awm->create_button("HELP_MERGE", "#merge/icon.bitmap");

        awm->show();
    }

    if (error) aw_message(error);
}

void MG_start_cb(AW_window *aww)
{
    AW_root  *awr   = aww->get_root();
    GB_ERROR  error = 0;
    {
        char *merge = awr->awar(AWAR_MERGE_DB"/file_name")->read_string();
        if (!strlen(merge) || (strcmp(merge,":") && GB_size_of_file(merge)<=0)) {
            error = GBS_global_string("Cannot find DB I '%s'", merge);
        }
        else {
            aw_openstatus("Loading databases");
                
            aw_status("DATABASE I");
            GLOBAL_gb_merge = GBT_open(merge, "rw", "$(ARBHOME)/lib/pts/*");
            if (!GLOBAL_gb_merge) {
                error = GB_get_error();
            }
            else {
                AWT_announce_db_to_browser(GLOBAL_gb_merge, GBS_global_string("Database I (source; %s)", merge));

                aw_status("DATABASE II");

                char       *main      = awr->awar(AWAR_MAIN_DB"/file_name")->read_string();
                const char *open_mode = 0;

                if (main[0] == 0) {
                    error = "You have to specify a name for DB II";
                }
                else if (strcmp(main,":") != 0 && GB_size_of_file(main) <= 0 ) {
                    aw_message(GBS_global_string("Cannot find DB II '%s' -> creating empty database", main));
                    open_mode = "wc";
                }
                else {
                    open_mode = "rwc";
                }

                if (!error) {
                    GLOBAL_gb_dest = GBT_open(main, open_mode, "$(ARBHOME)/lib/pts/*");
                    if (!GLOBAL_gb_dest) {
                        error = GB_get_error();
                    }
                    else {
                        AWT_announce_db_to_browser(GLOBAL_gb_dest, GBS_global_string("Database II (destination; %s)", main));
                    }
                }
                free(main);
            }
            aw_closestatus();
        }
        free(merge);
    }

    if (error) aw_message(error);
    else MG_start_cb2(aww, awr, true, false);
}


AW_window *create_merge_init_window(AW_root *awr)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( awr, "MERGE_SELECT_DATABASES", "MERGE SELECT TWO DATABASES");
    aws->load_xfig("merge/startup.fig");

    aws->button_length( 10 );
    aws->label_length( 10 );

    aws->callback( (AW_CB0)exit);
    aws->at("close");
    aws->create_button("QUIT","QUIT","A");

    aws->callback(AW_POPUP_HELP,(AW_CL)"arb_merge.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box(aws,AWAR_MERGE_DB,"");
    aws->at("type");
    aws->create_option_menu(AWAR_MERGE_DB"/filter");
    aws->insert_option("ARB","A","arb");
    aws->insert_default_option("OTHER","O","");
    aws->update_option_menu();

    awt_create_selection_box(aws,AWAR_MAIN_DB,"m");
    aws->at("mtype");
    aws->create_option_menu(AWAR_MAIN_DB"/filter");
    aws->insert_option("ARB","A","arb");
    aws->insert_default_option("OTHER","O","");
    aws->update_option_menu();

    aws->callback(MG_start_cb);

    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->create_button("GO","#merge/icon_vertical.bitmap","G");

    return (AW_window *)aws;
}

void MG_create_all_awars(AW_root *awr, AW_default aw_def,const char *fname_one, const char *fname_two)
{
    aw_create_selection_box_awars(awr, AWAR_MAIN_DB, "", ".arb", fname_two, aw_def);
    awr->awar_string( AWAR_MAIN_DB"/type", "b",aw_def);

    aw_create_selection_box_awars(awr, AWAR_MERGE_DB, "", ".arb", fname_one, aw_def);

    MG_create_trees_awar(awr,aw_def);
    MG_create_config_awar(awr,aw_def);
    MG_create_extendeds_awars(awr,aw_def);
    MG_create_alignment_awars(awr,aw_def);
    MG_create_species_awars(awr,aw_def);
    MG_create_gene_species_awars(awr, aw_def);

    MG_create_rename_awars(awr, aw_def);
    AWTC_create_rename_awars(awr, aw_def);
    AWT_create_db_browser_awars(awr, aw_def);
}

AW_window *create_MG_main_window(AW_root *aw_root)
{
    MG_create_all_awars(aw_root,AW_ROOT_DEFAULT);
    AW_window *aww=create_merge_init_window(aw_root);
    aww->show();
    return aww;
}

