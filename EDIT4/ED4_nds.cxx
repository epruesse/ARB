#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_awars.hxx>
#include <aw_window.hxx>

#include "ed4_class.hxx"
#include "ed4_awars.hxx"

#define NDS_COUNT 10

static char *NDS_command = 0;
static int NDS_width = 1;

char *ED4_get_NDS_text(ED4_species_manager *species_man) {
    GBDATA *gbd = species_man->get_species_pointer();
    e4_assert(gbd);

    e4_assert(NDS_command);
    char *result = GB_command_interpreter(gb_main, "", NDS_command, gbd, 0);
    if (!result) {
        result = strdup("<error>");
    }
    return result;
}

void ED4_get_NDS_sizes(int *width, int *brackets) {
    *width = NDS_width;
    *brackets = ED4_ROOT->aw_root->awar(ED4_AWAR_NDS_BRACKETS)->read_int();
}

static void NDS_changed(AW_root *root, AW_CL refresh)
{
    int toggle = root->awar(ED4_AWAR_NDS_SELECT)->read_int();
    char buf[256];

    sprintf(buf, ED4_AWAR_NDS_ACI_TEMPLATE, toggle);
    if (NDS_command) {
        free(NDS_command);
    }
    NDS_command = root->awar(buf)->read_string();

    sprintf(buf, ED4_AWAR_NDS_WIDTH_TEMPLATE, toggle);
    NDS_width = root->awar(buf)->read_int();

    if (int(refresh)) {
        //	GB_transaction dummy(gb_main);

        ED4_calc_terminal_extentions();
        ED4_ROOT->main_manager->route_down_hierarchy(0,0,update_terminal_extension);
        ED4_gc_is_modified(ED4_ROOT->temp_aww, 0, 0);

#if 0
        ED4_terminal *term = ED4_ROOT->root_group_man->get_first_terminal();
        while (term) {
            if (term->is_species_name_terminal()) {
                update_terminal_extension(0,0,term);
                term->parent->resize_requested_by_child();
                term->set_refresh();
                term->parent->refresh_requested_by_child();
            }
            term = term->get_next_terminal();
        }

        ED4_ROOT->resize_all();
        ED4_ROOT->show_all();
#endif
    }
}

void ED4_create_NDS_awars(AW_root *root)
{
    int i;
    GB_transaction dummy(gb_main);

    root->awar_int(ED4_AWAR_NDS_SELECT, 0)->add_callback(NDS_changed,1);
    root->awar_int(ED4_AWAR_NDS_BRACKETS, 6)->set_minmax(0,99)->add_callback(NDS_changed,1);
    root->awar_int(ED4_AWAR_NDS_INFO_WIDTH, 5)->set_minmax(0,99)->add_callback(NDS_changed,1);
    root->awar_string(ED4_AWAR_NDS_ECOLI_NAME, "Ecoli (usually)")->add_callback(NDS_changed,1);

    for (i=0; i<NDS_COUNT; i++) {
        char buf[256];
        const char *desc;

        sprintf(buf, ED4_AWAR_NDS_SELECT_TEMPLATE, i);
        root->awar_int(buf, i==0);

        sprintf(buf, ED4_AWAR_NDS_DESCRIPTION_TEMPLATE, i);
        switch (i) {
            case 0: desc = "Short name"; break;
            case 1: desc = "Full name"; break;
            default: desc = ""; break;
        }
        root->awar_string(buf, desc);

        const char *aci;
        sprintf(buf, ED4_AWAR_NDS_ACI_TEMPLATE, i);
        switch (i) {
            case 0: aci = "readdb(name)"; break;
            case 1: aci = "readdb(full_name)"; break;
            default: aci = "\"<not defined>\""; break;
        }
        root->awar_string(buf, aci)->add_callback(NDS_changed,1);

        int len;
        sprintf(buf, ED4_AWAR_NDS_WIDTH_TEMPLATE, i);
        switch (i) {
            case 0: len = 9; break;
            case 1: len = 27; break;
            default: len = 20; break;
        }
        root->awar_int(buf, len)->add_callback(NDS_changed,1);
    }

    NDS_changed(root,0); // init globals
}

/* a crazy implementation of a toggle field */
void ed4_nds_select_change(AW_window *aww,AW_CL selected){
    int i;
    AW_root *aw_root = aww->get_root();
    for (i=0;i<NDS_COUNT;i++){
        const char *awar_name = GBS_global_string(ED4_AWAR_NDS_SELECT_TEMPLATE,i);
        aw_root->awar(awar_name)->write_int( (i==selected)? 1:0);
    }
    aw_root->awar(ED4_AWAR_NDS_SELECT)->write_int(selected);
}

AW_window *ED4_create_nds_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    int description_x, aci_x, length_x;
    int i, dummy, fieldselectx;
    //int y[NDS_COUNT];
    //int toggle_y, data_x;

    aws->init(root, "NDS_PROPS", "NDS");
    aws->load_xfig("edit4/nds.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"ed4_nds.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP","H");

    aws->at("brackets");
    aws->label("Used maximum group depth");
    aws->create_input_field(ED4_AWAR_NDS_BRACKETS, 3);

    aws->at("infowidth");
    aws->label("Display width used for info-field");
    aws->create_input_field(ED4_AWAR_NDS_INFO_WIDTH, 3);

    aws->at("ecoli_name");
    aws->label("Name displayed for SAI: ECOLI");
    aws->create_input_field(ED4_AWAR_NDS_ECOLI_NAME, 20);

    aws->auto_space(10, 2);

    aws->at("toggle");
    aws->at_newline();
    aws->get_at_position(&fieldselectx, &dummy);

    for (i=0; i<NDS_COUNT; ++i) {
        char buf[256];
        sprintf(buf, ED4_AWAR_NDS_SELECT_TEMPLATE, i);
        aws->callback(ed4_nds_select_change,i);
        aws->create_toggle(buf);

        aws->get_at_position( &description_x,&dummy );
        sprintf(buf,ED4_AWAR_NDS_DESCRIPTION_TEMPLATE,i);
        aws->create_input_field(buf,15);

        aws->get_at_position( &aci_x,&dummy );
        sprintf(buf,ED4_AWAR_NDS_ACI_TEMPLATE,i);
        aws->create_input_field(buf,30);

        aws->get_at_position( &length_x,&dummy );
        sprintf(buf,ED4_AWAR_NDS_WIDTH_TEMPLATE,i);
        aws->create_input_field(buf,3);

        aws->at_newline();
    }

    aws->at("head");
    aws->at_x(description_x);
    aws->create_button(0, "DESCRIPTION");
    aws->at_x(aci_x);
    aws->create_button(0, "ACI PROGRAM");
    aws->at_x(length_x);
    aws->create_button(0, "WIDTH");

    return (AW_window*)aws;
}

/*
    aws->auto_increment(0, 25);

    aws->at("toggle");
    aws->get_at_position(&dummy, &toggle_y);

    aws->auto_space(10, 2);

    // calculate y-offset between two NDS toggles
    {
        aws->at_newline();
        aws->create_button(0, "");

        int y_after_toggle;
        aws->get_at_position(&dummy, &y_after_toggle);

        int y_per_toggle = y_after_toggle-toggle_y;
        for (i=0; i<NDS_COUNT; i++) {
            y[i] = toggle_y + y_per_toggle*i;
        }
    }

    // insert toggles for NDS
    char toggle_name[] = "A";
    aws->at("toggle");
    aws->create_toggle_field(ED4_AWAR_NDS_TOGGLE, 0);
    aws->insert_default_toggle(toggle_name, "", 0);
    for (i=1; i<NDS_COUNT; i++) {
        toggle_name[0]++;
        aws->insert_toggle(toggle_name, "", i);
    }
    aws->update_toggle_field();

    aws->at("data");
    aws->get_at_position(&data_x, &dummy);

    // add data fields behind each toggle
    aws->auto_space(10,0);

    for (i=0; i<NDS_COUNT; i++) {
        char buf[256];

        aws->at(data_x, y[i]);

        sprintf(buf, ED4_AWAR_NDS_DESCRIPTION_TEMPLATE, i);
        aws->get_at_position(&description_x, &dummy);
        aws->create_input_field(buf, 15);

        sprintf(buf, ED4_AWAR_NDS_ACI_TEMPLATE, i);
        aws->get_at_position(&aci_x, &dummy);
        aws->create_input_field(buf, 30);

        sprintf(buf, ED4_AWAR_NDS_WIDTH_TEMPLATE, i);
        aws->get_at_position(&length_x, &dummy);
        aws->create_input_field(buf, 3);
    }

    aws->auto_off();

    return (AW_window*)aws;

*/
