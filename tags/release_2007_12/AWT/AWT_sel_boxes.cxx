#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>
#include <sys/stat.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <ad_config.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_global.hxx>
#include "awt.hxx"
#include "awtlocal.hxx"
#include "awt_sel_boxes.hxx"
#include "awt_changekey.hxx"

// ******************** selection boxes on alignments ********************

static void awt_create_selection_list_on_ad_cb(GBDATA *dummy, struct adawcbstruct *cbs)
{
    GBDATA *gb_alignment;
    GBDATA *gb_alignment_name;
    GBDATA *gb_alignment_type;
    char   *alignment_name;
    char   *alignment_type;

    AWUSE(dummy);

    cbs->aws->clear_selection_list(cbs->id);

    for (gb_alignment = GB_search(cbs->gb_main,"presets/alignment",GB_FIND);
         gb_alignment;
         gb_alignment = GB_find(gb_alignment,"alignment",0,this_level|search_next))
    {
        gb_alignment_type = GB_find(gb_alignment,"alignment_type",0,down_level);
        gb_alignment_name = GB_find(gb_alignment,"alignment_name",0,down_level);
        alignment_type    = GB_read_string(gb_alignment_type);
        alignment_name    = GB_read_string(gb_alignment_name);

        char *str = GBS_string_eval(alignment_type,cbs->comm,0);
        if (!*str){
            cbs->aws->insert_selection( cbs->id, alignment_name, alignment_name );
        }
        free(str);
        free(alignment_type);
        free(alignment_name);
    }
    cbs->aws->insert_default_selection( cbs->id, "????", "????" );
    cbs->aws->update_selection_list( cbs->id );
}

void awt_create_selection_list_on_ad(GBDATA *gb_main,AW_window *aws, const char *varname,const char *comm)
// if comm is set then only those alignments are taken
// which can be parsed by comm
{
    AW_selection_list *id;
    GBDATA            *gb_presets;

    GB_push_transaction(gb_main);

    id  = aws->create_selection_list(varname,0,"",20,3);
    struct adawcbstruct *cbs = new adawcbstruct;
    memset(cbs, 0, sizeof(*cbs));

    cbs->aws     = aws;
    cbs->awr     = aws->get_root();
    cbs->gb_main = gb_main;
    cbs->id      = id;
    cbs->comm    = 0; if (comm) cbs->comm = strdup(comm);

    awt_create_selection_list_on_ad_cb(0,cbs);

    gb_presets = GB_search(gb_main,"presets",GB_CREATE_CONTAINER);
    GB_add_callback(gb_presets,GB_CB_CHANGED, (GB_CB)awt_create_selection_list_on_ad_cb, (int *)cbs);

    GB_pop_transaction(gb_main);
}


// ******************** selection boxes on trees ********************

void awt_create_selection_list_on_trees_cb(GBDATA *dummy, struct adawcbstruct *cbs)
{
    AWUSE(dummy);

    cbs->aws->clear_selection_list(cbs->id);
    char **tree_names = GBT_get_tree_names(cbs->gb_main);
    if (tree_names) {
        for (char **tree= tree_names ; *tree; tree++){
            char *info = GBT_tree_info_string(cbs->gb_main, *tree);
            if (info) {
                cbs->aws->insert_selection( cbs->id, info, *tree );
                free(info);
            }else{
                cbs->aws->insert_selection( cbs->id, *tree, *tree );
            }
        }
        GBT_free_names(tree_names);
    }
    cbs->aws->insert_default_selection( cbs->id, "????", "????" );
    cbs->aws->update_selection_list( cbs->id );
}

void awt_create_selection_list_on_trees(GBDATA *gb_main,AW_window *aws,const char *varname)
{
    AW_selection_list *id;
    GBDATA            *gb_tree_data;
    
    GB_push_transaction(gb_main);

    id = aws->create_selection_list(varname,0,"",40,8);
    struct adawcbstruct *cbs = new adawcbstruct;
    memset(cbs, 0, sizeof(*cbs));

    cbs->aws     = aws;
    cbs->awr     = aws->get_root();
    cbs->gb_main = gb_main;
    cbs->id      = id;

    awt_create_selection_list_on_trees_cb(0,cbs);

    gb_tree_data = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    GB_add_callback(gb_tree_data,GB_CB_CHANGED,
                    (GB_CB)awt_create_selection_list_on_trees_cb, (int *)cbs);

    GB_pop_transaction(gb_main);
}

// ******************** selection boxes on pt-servers ********************

#define PT_SERVERNAME_LENGTH        23 // thats for buttons
#define PT_SERVERNAME_SELLIST_WIDTH 30 // this for lists
#define PT_SERVER_TRACKLOG_TIMER    10000 // every 10 seconds

struct selection_list_handle {
    AW_window                    *aws;
    AW_selection_list            *sellst;
    struct selection_list_handle *next;
};

static selection_list_handle *allPTserverSellists = 0; // all pt server selection lists

static void fill_pt_server_selection_list(AW_window *aws, AW_selection_list *id) {
    aws->clear_selection_list(id);

    const char * const *pt_servers = GBS_get_arb_tcp_entries("ARB_PT_SERVER*");
    int count = 0;
    while (pt_servers[count]) count++;

    for (int i=0; i<count; i++) {
        char *choice = GBS_ptserver_id_to_choice(i, 1);
        if (!choice) {
            GB_ERROR error = GB_get_error();
            if (!error) {
                error = GBS_global_string("Failed to get pt-server id #%i (please report)", i);
                gb_assert(0); // should not occur
            }
            aw_message(error);
            break;
        }

        aws->insert_selection(id,choice,(long)i);
        free(choice);
    }

    aws->insert_default_selection(id, "-undefined-", (long)-1);
    aws->update_selection_list(id);
}

void awt_refresh_all_pt_server_selection_lists() {
    // refresh content of all PT_server selection lists
    selection_list_handle *serverList = allPTserverSellists;

    while (serverList) {
        fill_pt_server_selection_list(serverList->aws, serverList->sellst);
        serverList = serverList->next;
    }
}

static void track_log_cb(AW_root *awr) {
    static long  last_ptserverlog_mod = 0;
    const char  *ptserverlog          = GBS_ptserver_logname();
    long         ptserverlog_mod      = GB_time_of_file(ptserverlog);

    if (ptserverlog_mod != last_ptserverlog_mod) {
#if defined(DEBUG)
        fprintf(stderr, "%s modified!\n", ptserverlog);
#endif // DEBUG
        awt_refresh_all_pt_server_selection_lists();
        last_ptserverlog_mod = ptserverlog_mod;
    }

    awr->add_timed_callback(PT_SERVER_TRACKLOG_TIMER, track_log_cb);
}

static void announce_pt_server_selection_list(AW_window *aws, AW_selection_list *id) {
#if defined(DEBUG)
    selection_list_handle *serverList = allPTserverSellists;
    while (serverList) {
        awt_assert(aws != serverList->aws || id != serverList->sellst); // oops - added twice
        serverList = serverList->next;
    }
#endif // DEBUG

    if (!allPTserverSellists) { // first pt server selection list -> install log tracker
        aws->get_root()->add_timed_callback(PT_SERVER_TRACKLOG_TIMER, track_log_cb);
    }

    selection_list_handle *newServerList = (selection_list_handle *)malloc(sizeof(*newServerList));

    newServerList->aws    = aws;
    newServerList->sellst = id;
    newServerList->next   = allPTserverSellists;

    allPTserverSellists = newServerList;
}

// --------------------------------------------------------------------------------

static char *readable_pt_servername(int index, int maxlength) {
    char *fullname = GBS_ptserver_id_to_choice(index, 0);
    if (!fullname) {
#ifdef DEBUG
      printf("awar given to awt_create_selection_list_on_pt_servers() does not contain a valid index\n");
#endif
      //        awt_assert(0); // awar given to awt_create_selection_list_on_pt_servers() does not contain a valid index
        return strdup("-undefined-");
    }

    int len = strlen(fullname);
    if (len <= maxlength) {
        return fullname;
    }

    int remove  = len-maxlength;
    fullname[0] = '.';
    fullname[1] = '.';
    strcpy(fullname+2, fullname+2+remove);

    return fullname;
}

static void update_ptserver_button(AW_root *aw_root, AW_CL cl_varname) {
    const char *varname              = (const char *)cl_varname;
    char       *awar_buttontext_name = GBS_global_string_copy("/tmp/%s_BUTTON", varname);
    char       *readable_name        = readable_pt_servername(aw_root->awar(varname)->read_int(), PT_SERVERNAME_LENGTH);

    aw_root->awar(awar_buttontext_name)->write_string(readable_name);

    free(readable_name);
    free(awar_buttontext_name);
}

AW_window *awt_popup_selection_list_on_pt_servers(AW_root *aw_root, const char *varname) {
    AW_window_simple *aw_popup = new AW_window_simple;

    aw_popup->init(aw_root, "SELECT_PT_SERVER", "Select a PT-Server");
    aw_popup->auto_space(10, 10);

    aw_popup->at_newline();
    aw_popup->callback((AW_CB0)AW_POPDOWN);
    AW_selection_list *id = aw_popup->create_selection_list(varname, 0, "", PT_SERVERNAME_SELLIST_WIDTH, 20);

    aw_popup->at_newline();
    aw_popup->callback((AW_CB0)AW_POPDOWN);
    aw_popup->create_button("CLOSE", "CLOSE", "C");

    aw_popup->window_fit();

    announce_pt_server_selection_list(aw_popup, id);
    fill_pt_server_selection_list(aw_popup, id);

    return aw_popup;
}

void awt_create_selection_list_on_pt_servers(AW_window *aws, const char *varname, AW_BOOL popup)
{
    if (popup) {
        AW_root *aw_root              = aws->get_root();
        char    *awar_buttontext_name = GBS_global_string_copy("/tmp/%s_BUTTON", varname);
        int      ptserver_index       = aw_root->awar(varname)->read_int();

        if (ptserver_index<0) { // fix invalid pt_server indices
            ptserver_index = 0;
            aw_root->awar(varname)->write_int(ptserver_index);
        }

        char *readable_name = readable_pt_servername(ptserver_index, PT_SERVERNAME_LENGTH);

        AW_CL cl_varname = (AW_CL)strdup(varname); // make copy of awar_name for callbacks

        aw_root->awar_string(awar_buttontext_name, readable_name, AW_ROOT_DEFAULT);
        aw_root->awar(varname)->add_callback(update_ptserver_button, cl_varname);

        int old_button_length = aws->get_button_length();

        aws->button_length(PT_SERVERNAME_LENGTH+1);
        aws->callback(AW_POPUP, (AW_CL)awt_popup_selection_list_on_pt_servers, cl_varname);
        aws->create_button("CURR_PT_SERVER", awar_buttontext_name);

        aws->button_length(old_button_length);

        free(readable_name);
        free(awar_buttontext_name);
    }
    else {
        AW_selection_list *id = aws->create_selection_list(varname);
        announce_pt_server_selection_list(aws, id);
        fill_pt_server_selection_list(aws, id);
    }
}


// ******************** selection boxes on tables ********************

void awt_create_selection_list_on_tables_cb(GBDATA *dummy, struct awt_sel_list_for_tables *cbs){
    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);
    GBDATA *gb_table;
    for (gb_table = GBT_first_table(cbs->gb_main);
         gb_table;
         gb_table = GBT_next_table(gb_table)){

        GBDATA *gb_name = GB_find(gb_table,"name",0,down_level);
        GBDATA *gb_description = GB_search(gb_table,"description",GB_STRING);
        if (!gb_name) continue;
        char *table_name = GB_read_string(gb_name);
        char *description = GB_read_string(gb_description);
        const char *info_text = GBS_global_string("%s: %s",table_name,description);
        cbs->aws->insert_selection(cbs->id,info_text,table_name);
        delete table_name;
        delete description;
    }
    cbs->aws->insert_default_selection( cbs->id, "", "" );
    cbs->aws->update_selection_list( cbs->id );
}

void awt_create_selection_list_on_tables(GBDATA *gb_main,AW_window *aws,const char *varname)
{
    AW_selection_list *id;
    GBDATA  *gb_table_data;
    struct awt_sel_list_for_tables *cbs;
    GB_push_transaction(gb_main);

    id = aws->create_selection_list(varname,0,"",40,8);
    cbs = new awt_sel_list_for_tables;
    cbs->aws = aws;
    cbs->gb_main = gb_main;
    cbs->id = id;

    awt_create_selection_list_on_tables_cb(0,cbs);

    gb_table_data = GB_search(gb_main,"table_data",GB_CREATE_CONTAINER);
    GB_add_callback(gb_table_data,GB_CB_CHANGED,(GB_CB)awt_create_selection_list_on_tables_cb, (int *)cbs);

    GB_pop_transaction(gb_main);
}
// ******************** selection boxes on tables ********************

void awt_create_selection_list_on_table_fields_cb(GBDATA *dummy, struct awt_sel_list_for_tables *cbs){
    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);
    GBDATA  *gb_table = GBT_open_table(cbs->gb_main,cbs->table_name, 1); // read only
    GBDATA *gb_table_field;
    for (gb_table_field = GBT_first_table_field(gb_table);
         gb_table_field;
         gb_table_field = GBT_next_table_field(gb_table_field))
    {
        GBDATA *gb_name = GB_find(gb_table_field,"name",0,down_level);
        GBDATA *gb_description = GB_search(gb_table_field,"description",GB_STRING);
        if (!gb_name) continue;
        char *table_name = GB_read_string(gb_name);
        char *description = GB_read_string(gb_description);
        const char *info_text = GBS_global_string("%s: %s",table_name,description);
        cbs->aws->insert_selection(cbs->id,info_text,table_name);
        delete table_name;
        delete description;
    }
    cbs->aws->insert_default_selection( cbs->id, "", "" );
    cbs->aws->update_selection_list( cbs->id );
}

void awt_create_selection_list_on_table_fields(GBDATA *gb_main,AW_window *aws,const char *table_name, const char *varname)
{
    AW_selection_list *id;
    struct awt_sel_list_for_tables *cbs;
    GB_push_transaction(gb_main);

    id = aws->create_selection_list(varname,0,"",40,8);
    cbs = new awt_sel_list_for_tables;
    cbs->aws = aws;
    cbs->gb_main = gb_main;
    cbs->id = id;
    cbs->table_name = strdup(table_name);

    awt_create_selection_list_on_table_fields_cb(0,cbs);

    GBDATA  *gb_table = GBT_open_table(gb_main,table_name, 1); // read only
    if (gb_table){
        GB_add_callback(gb_table,GB_CB_CHANGED,(GB_CB)awt_create_selection_list_on_table_fields_cb, (int *)cbs);
    }
    GB_pop_transaction(gb_main);
}

// ******************** selection boxes on editor configurations ********************

void awt_create_selection_list_on_configurations_cb(GBDATA *dummy, struct adawcbstruct *cbs) {
    AWUSE(dummy);
 restart:
    cbs->aws->clear_selection_list(cbs->id);
    GBDATA *gb_configuration_data = GB_search(cbs->gb_main,AWAR_CONFIG_DATA,GB_CREATE_CONTAINER);
    GBDATA *gb_config;

#if defined(DEVEL_RALF)
#warning use GBT_get_configuration_names here and skip renaming here
#endif // DEVEL_RALF


    for (gb_config = GB_find(gb_configuration_data,0,0,down_level);
         gb_config;
         gb_config = GB_find(gb_config,0,0,this_level | search_next))
    {
        GBDATA *gb_name = GB_find(gb_config,"name",0,down_level);
        if (!gb_name){
            aw_message("internal error: unnamed configuration (now renamed to 'unnamed_config')");
            gb_name = GB_create(gb_config, "name", GB_STRING);
            if (!gb_name) {
                char *err = strdup(GB_get_error());
                GB_CSTR question = GBS_global_string("Rename of configuration failed (reason: '%s')\n"
                                                     "Do you like do delete the unnamed configuration?", err);
                free(err);

                int del_config = 0==aw_message(question, "Yes,No");
                if (del_config) {
                    GB_delete(gb_config);
                    goto restart;
                }
                continue;
            }
            GB_write_string(gb_name, "unnamed_config");
        }
        char *name = GB_read_char_pntr(gb_name);
        cbs->aws->insert_selection( cbs->id, name, name );
    }

    cbs->aws->insert_default_selection( cbs->id, "????", "????" );
    cbs->aws->update_selection_list( cbs->id );
}

void awt_create_selection_list_on_configurations(GBDATA *gb_main,AW_window *aws,const char *varname)
{
    AW_selection_list *id;
    GBDATA  *gb_configuration_data;
    GB_push_transaction(gb_main);

    id = aws->create_selection_list(varname,0,"",40,15);
    
    struct adawcbstruct *cbs = new adawcbstruct;
    memset(cbs, 0, sizeof(*cbs));

    cbs->aws     = aws;
    cbs->awr     = aws->get_root();
    cbs->gb_main = gb_main;
    cbs->id      = id;

    awt_create_selection_list_on_configurations_cb(0,cbs);

    gb_configuration_data = GB_search(gb_main,AWAR_CONFIG_DATA,GB_CREATE_CONTAINER);
    GB_add_callback(gb_configuration_data,GB_CB_CHANGED,
                    (GB_CB)awt_create_selection_list_on_configurations_cb, (int *)cbs);

    GB_pop_transaction(gb_main);
}

char *awt_create_string_on_configurations(GBDATA *gb_main) {
    GB_push_transaction(gb_main);
 restart:
    char   *result                = 0;
    GBDATA *gb_configuration_data = GB_search(gb_main,AWAR_CONFIG_DATA,GB_CREATE_CONTAINER);
    GBDATA *gb_config;

#if defined(DEVEL_RALF)
#warning use GBT_get_configuration_names here and skip renaming here
#endif // DEVEL_RALF

    for (gb_config = GB_find(gb_configuration_data,0,0,down_level);
         gb_config;
         gb_config = GB_find(gb_config,0,0,this_level | search_next))
    {
        GBDATA *gb_name = GB_find(gb_config,"name",0,down_level);
        if (!gb_name){
            aw_message("internal error: unnamed configuration (now renamed to 'unnamed_config')");
            gb_name = GB_create(gb_config, "name", GB_STRING);
            if (!gb_name) {
                char *err = strdup(GB_get_error());
                GB_CSTR question = GBS_global_string("Rename of configuration failed (reason: '%s')\n"
                                                     "Do you like do delete the unnamed configuration?", err);
                free(err);

                int del_config = 0==aw_message(question, "Yes,No");
                if (del_config) {
                    GB_delete(gb_config);
                    goto restart;
                }
                continue;
            }
            GB_write_string(gb_name, "unnamed_config");
        }
        char *name = GB_read_char_pntr(gb_name);
        if (result) {
            char *neu = GB_strdup(GBS_global_string("%s;%s", result, name));
            free(result);
            result    = neu;
        }
        else {
            result = GB_strdup(name);
        }
    }
    GB_pop_transaction(gb_main);
    return result;
}

// ******************** selection boxes on SAIs ********************

void awt_create_selection_list_on_extendeds_update(GBDATA *dummy, void *cbsid)
{
    printf("start awt_create_selection_list_on_extendeds_update\n"); // @@@

    struct awt_sel_list_for_sai *cbs              = (struct awt_sel_list_for_sai *)cbsid;

    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);

    for (GBDATA *gb_extended = GBT_first_SAI(cbs->gb_main);
         gb_extended;
         gb_extended = GBT_next_SAI(gb_extended))
    {
        GBDATA *gb_name = GB_find(gb_extended,"name",0,down_level);
        if (!gb_name) continue;
        if (cbs->filter_poc) {
            char *res = cbs->filter_poc(gb_extended,cbs->filter_cd);
            if (res) {
                char *name = GB_read_char_pntr(gb_name);
                cbs->aws->insert_selection( cbs->id, res, name );
                free(res);
            }
        }
        else {
            const char *name     = GB_read_char_pntr(gb_name);
            GBDATA     *gb_group = GB_find(gb_extended, "sai_group", 0, down_level);
            if (gb_group) {
                const char *group          = GB_read_char_pntr(gb_group);
                char       *group_and_name = GBS_global_string_copy("[%s] %s", group, name);
                cbs->aws->insert_selection(cbs->id, group_and_name, name);
                free(group_and_name);
            }
            else {
                cbs->aws->insert_selection( cbs->id, name, name );
            }
        }
    }
    cbs->aws->sort_selection_list(cbs->id, 0, 0);

    if (cbs->add_selected_species) {
        GBDATA *gb_sel = GB_search(cbs->gb_main,AWAR_SPECIES_NAME,GB_STRING);
        char   *name   = GB_read_string(gb_sel);
        if (strlen(name)){
            char *sname = (char *)calloc(1, strlen(name)+2);
            sprintf(sname+1,"%s",name);
            sname[0] = 1;
            char *text = (char *)GBS_global_string("Selected Species: '%s'",name);
            cbs->aws->insert_selection( cbs->id, text, sname );
            delete name;
        }
        delete name;
    }
    cbs->aws->insert_default_selection( cbs->id, "- none -", "none" );
    cbs->aws->update_selection_list( cbs->id );
    printf("done  awt_create_selection_list_on_extendeds_update\n"); // @@@
}

void *awt_create_selection_list_on_extendeds(GBDATA *gb_main,AW_window *aws, const char *varname,
                                             char *(*filter_poc)(GBDATA *gb_ext, AW_CL), AW_CL filter_cd,
                                             AW_BOOL add_sel_species)
{
    AW_selection_list           *id;
    GBDATA                      *gb_extended_data;
    struct awt_sel_list_for_sai *cbs;

    GB_push_transaction(gb_main);

    id  = aws->create_selection_list(varname,0,"",40,8);
    cbs = new awt_sel_list_for_sai;

    cbs->aws                  = aws;
    cbs->gb_main              = gb_main;
    cbs->id                   = id;
    cbs->filter_poc           = filter_poc;
    cbs->filter_cd            = filter_cd;
    cbs->add_selected_species = add_sel_species;

    awt_create_selection_list_on_extendeds_update(0,(void *)cbs);

    gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    GB_add_callback(gb_extended_data,GB_CB_CHANGED, (GB_CB)awt_create_selection_list_on_extendeds_update, (int *)cbs);

    if (add_sel_species){       // update box if another species is selected
        GBDATA *gb_sel = GB_search(gb_main,AWAR_SPECIES_NAME,GB_STRING);
        GB_add_callback(gb_sel,GB_CB_CHANGED, (GB_CB)awt_create_selection_list_on_extendeds_update, (int *)cbs);
    }
    GB_pop_transaction(gb_main);

    return (void *)cbs;
}




// ******************** selection boxes on saving selection lists ********************

void create_save_box_for_selection_lists_save(AW_window *aws,AW_CL selidcd,AW_CL basenamecd)
{
    AW_selection_list *selid       = (AW_selection_list *)selidcd;
    char              *awar_prefix = (char *)basenamecd;

    char bline_anz[GB_PATH_MAX];
    sprintf(bline_anz,"%s/line_anz",awar_prefix);

    AW_root *aw_root  = aws->get_root();
    long     lineanz  = aw_root->awar(bline_anz)->read_int();
    char    *filename = awt_get_selected_fullname(aw_root, awar_prefix);

    GB_ERROR error = aws->save_selection_list(selid,filename,lineanz);

    if (error) {
        aw_message(error);
    }
    else {
        awt_refresh_selection_box(aw_root, awar_prefix);
        aws->hide();
    }
    free(filename);
}

AW_window *create_save_box_for_selection_lists(AW_root *aw_root,AW_CL selid)
{
    char *awar_base_name = GBS_global_string_copy("tmp/save_box_sel_%li", long(selid)); // don't free (passed to callback)
    char *awar_line_anz  = GBS_global_string_copy("%s/line_anz", awar_base_name);
    {
        aw_create_selection_box_awars(aw_root, awar_base_name, ".", GBS_global_string("noname.list"), "list");
        aw_root->awar_int(awar_line_anz, 0, AW_ROOT_DEFAULT);
    }

    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SAVE_SELECTCION_BOX", "SAVE BOX");
    aws->load_xfig("sl_s_box.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("save");
    aws->highlight();
    aws->callback(create_save_box_for_selection_lists_save,selid,(AW_CL)awar_base_name); // loose ownership of awar_base_name!
    aws->create_button("SAVE", "SAVE","S");

    aws->at("nlines");
    aws->create_option_menu(awar_line_anz, 0, "");
    aws->insert_default_option("all","a",0);
    aws->insert_option(   "50", "a",    50);
    aws->insert_option(  "100", "a",   100);
    aws->insert_option(  "500", "a",   500);
    aws->insert_option( "1000", "a",  1000);
    aws->insert_option( "5000", "a",  5000);
    aws->insert_option("10000", "a", 10000);
    aws->update_option_menu();

    awt_create_selection_box((AW_window *)aws,awar_base_name);

    free(awar_line_anz);

    return (AW_window *)aws;
}

void AWT_load_list(AW_window *aww, AW_CL sel_id, AW_CL ibase_name)
{
    AW_selection_list * selid       = (AW_selection_list *)sel_id;
    char *basename = (char *)ibase_name;

    AW_root     *aw_root    = aww->get_root();
    GB_ERROR    error;

    //     char     bfile_name[GB_PATH_MAX];
    //     sprintf(bfile_name,"%s/file_name",basename);
    //     char *filename = aw_root->awar(bfile_name)->read_string();

    char *filename = awt_get_selected_fullname(aw_root, basename);
    error          = aww->load_selection_list(selid,filename);

    if (error) aw_message(error);

    AW_POPDOWN(aww);

    delete filename;
}

AW_window *create_load_box_for_selection_lists(AW_root *aw_root, AW_CL selid)
{
    char base_name[100];
    sprintf(base_name,"tmp/load_box_sel_%li",(long)selid);

    aw_create_selection_box_awars(aw_root, base_name, ".", "list", "");

    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "LOAD_SELECTION_BOX", "Load box");
    aws->load_xfig("sl_l_box.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("load");
    aws->highlight();
    aws->callback(AWT_load_list,selid,(AW_CL)strdup(base_name));
    aws->create_button("LOAD", "LOAD","L");

    awt_create_selection_box((AW_window *)aws,base_name);
    return (AW_window*) aws;
}


void create_print_box_for_selection_lists(AW_window *aw_window,AW_CL selid){
    AW_root *aw_root = aw_window->get_root();
    char *data = aw_window->get_selection_list_contents((AW_selection_list *)selid);
    AWT_create_ascii_print_window(aw_root,data,"no title");
    delete data;
}



/* ************************************************** */
AW_window *awt_create_load_box(AW_root *aw_root, const char *load_what, const char *file_extension, char **set_file_name_awar,
                               void (*callback)(AW_window*),
                               AW_window* (*create_popup)(AW_root *, AW_default))

    /*      You can either provide a normal callback or a create_popup-callback
     *      (the not-used callback has to be 0)
     */


{
    char *base_name = GBS_global_string_copy("tmp/load_box_%s",load_what);

    aw_create_selection_box_awars(aw_root, base_name, ".", file_extension, "");

    if (set_file_name_awar) {
        *set_file_name_awar = GBS_global_string_copy("%s/file_name", base_name);
    }

    AW_window_simple *aws = new AW_window_simple;
    {
        char title[100];
        sprintf(title, "Load %s", load_what);
        aws->init( aw_root, title, title);
        aws->load_xfig("load_box.fig");
    }

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"");
    aws->create_button("HELP","HELP");

    aws->at("go");
    aws->highlight();

    if (callback) {
        awt_assert(!create_popup);
        aws->callback((AW_CB0)callback);
    }
    else  {
        awt_assert(create_popup);
        aws->callback((AW_CB1)AW_POPUP, (AW_CL)create_popup);
    }

    aws->create_button("LOAD", "LOAD","L");

    awt_create_selection_box((AW_window *)aws,base_name);
    free(base_name);
    return (AW_window*) aws;
}

/* ************************************************** */



void awt_set_long(AW_window *aws, AW_CL varname, AW_CL value)   // set an awar
{
    aws->get_root()->awar((char *)varname)->write_int((long) value);
}

void awt_write_string( AW_window *aws, AW_CL varname, AW_CL value)  // set an awar
{
    aws->get_root()->awar((char *)varname)->write_string((char *)value);
}

struct fileChanged_cb_data {
    char               *fpath;  // full name of edited file
    int                 lastModtime; // last known modification time of 'fpath'
    bool                editorTerminated; // do not free before this has been set to 'true'
    awt_fileChanged_cb  callback;

    fileChanged_cb_data(char **fpath_ptr, awt_fileChanged_cb cb) {
        fpath            = *fpath_ptr;
        *fpath_ptr       = 0;   // take ownage
        lastModtime      = getModtime();
        editorTerminated = false;
        callback         = cb;
    }

    ~fileChanged_cb_data() {
        free(fpath);
    }

    int getModtime() {
        struct stat st;
        if (stat(fpath, &st) == 0) return st.st_mtime;
        return 0;
    }

    bool fileWasChanged() {
        int  modtime = getModtime();
        bool changed = modtime != lastModtime;
        lastModtime  = modtime;
        return changed;
    }
};

static void editor_terminated_cb(const char *IF_DEBUG(message), void *cb_data) {
    fileChanged_cb_data *data = (fileChanged_cb_data*)cb_data;

#if defined(DEBUG)
    printf("editor_terminated_cb: message='%s' fpath='%s'\n", message, data->fpath);
#endif // DEBUG

    data->callback(data->fpath, data->fileWasChanged(), true);
    data->editorTerminated = true; // trigger removal of check_file_changed_cb
}

#define AWT_CHECK_FILE_TIMER 700 // in ms

static void check_file_changed_cb(AW_root *aw_root, AW_CL cl_cbdata) {
    fileChanged_cb_data *data = (fileChanged_cb_data*)cl_cbdata;

    if (data->editorTerminated) {
        delete data;
    }
    else {
        bool changed = data->fileWasChanged();

        if (changed) data->callback(data->fpath, true, false);
        aw_root->add_timed_callback(AWT_CHECK_FILE_TIMER, check_file_changed_cb, cl_cbdata);
    }
}

void AWT_edit(const char *path, awt_fileChanged_cb callback, AW_window *aww, GBDATA *gb_main) {
    // Start external editor on file 'path' (asynchronously)
    // if 'callback' is specified, it is called everytime the file is changed
    // [aww and gb_main may be 0 if callback is 0]

    const char          *editor  = GB_getenvARB_TEXTEDIT();
    char                *fpath   = GBS_eval_env(path);
    char                *command = 0;
    fileChanged_cb_data *cb_data = 0;
    GB_ERROR             error   = 0;

    if (callback) {
        awt_assert(aww);
        awt_assert(gb_main);

        cb_data = new fileChanged_cb_data(&fpath, callback); // fpath now is 0 and belongs to cb_data

        char *arb_notify = GB_generate_notification(gb_main, editor_terminated_cb, "editor terminated", (void*)cb_data);
        if (arb_notify) {
            char *arb_message = GBS_global_string_copy("arb_message \"Could not start editor '%s'\"", editor);
            
            command = GBS_global_string_copy("((%s %s || %s); %s)&", editor, cb_data->fpath, arb_message, arb_notify);
            free(arb_message);
            free(arb_notify);
        }
        else {
            error = GB_get_error();
        }
    }
    else {
        command = GBS_global_string_copy("%s %s &", editor, fpath);
    }

    if (command) {
        GB_information("Executing '%s'", command);
        if (system(command) != 0) {
            aw_message(GBS_global_string("Could not start editor command '%s'", command));
            if (callback) {
                error = GB_remove_last_notification(gb_main);
            }
        }
        else { // sucessfully started editor
            // Can't be sure editor really started when callback is used (see command above).
            // But it doesnt matter, cause arb_notify is called anyway and removes all callbacks
            if (callback) {
                // add timed callback tracking file change
                AW_root *aw_root = aww->get_root();
                aw_root->add_timed_callback(AWT_CHECK_FILE_TIMER, check_file_changed_cb, (AW_CL)cb_data);
                cb_data          = 0; // now belongs to check_file_changed_cb 
            }
        }
    }

    if (error) {
        aw_message(error);
    }

    free(command);
    delete cb_data;
    free(fpath);
}


void AWT_popup_select_species_field_window(AW_window *aww, AW_CL cl_awar_name, AW_CL cl_gb_main)
{
    static AW_window_simple *aws = 0;

    // everytime map selection awar to latest user awar:
    AW_root    *aw_root   = aww->get_root();
    const char *awar_name = (const char *)cl_awar_name;
    aw_root->awar("tmp/viewkeys/key_text_select")->map(awar_name);

    if (!aws) {
        aws = new AW_window_simple;
        
        aws->init( aw_root, "SELECT_SPECIES_FIELD", "Select species field");
        aws->load_xfig("awt/nds_sel.fig");
        aws->button_length(13);

        aws->callback( AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE","C");

        awt_create_selection_list_on_scandb((GBDATA *)cl_gb_main,
                                            aws,
                                            "tmp/viewkeys/key_text_select",
                                            AWT_NDS_FILTER,
                                            "scandb","rescandb", &AWT_species_selector, 20, 10);
    }
    aws->show();
}

