// =============================================================== //
//                                                                 //
//   File      : ED4_nds.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ed4_class.hxx"
#include "ed4_awars.hxx"

#include <arbdbt.h>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <nds.h>

#define NDS_COUNT 10

static char *NDS_command = 0;
static int NDS_width = 1;

char *ED4_get_NDS_text(ED4_species_manager *species_man) {
    GBDATA *gbd = species_man->get_species_pointer();
    e4_assert(gbd);

    e4_assert(NDS_command);
    char *result        = GB_command_interpreter(GLOBAL_gb_main, "", NDS_command, gbd, 0);
    if (!result) result = GBS_global_string_copy("<error: %s>", GB_await_error());

    return NDS_mask_nonprintable_chars(result);
}

void ED4_get_NDS_sizes(int *width, int *brackets) {
    *width = NDS_width;
    *brackets = ED4_ROOT->aw_root->awar(ED4_AWAR_NDS_BRACKETS)->read_int();
}

static void NDS_changed(AW_root *root, AW_CL refresh) {
    int toggle = root->awar(ED4_AWAR_NDS_SELECT)->read_int();

    char buf[256];
    sprintf(buf, ED4_AWAR_NDS_ACI_TEMPLATE, toggle);
    freeset(NDS_command, root->awar(buf)->read_string());

    sprintf(buf, ED4_AWAR_NDS_WIDTH_TEMPLATE, toggle);
    NDS_width = root->awar(buf)->read_int();

    if (refresh) {
        ED4_request_relayout();
    }
}

void ED4_create_NDS_awars(AW_root *root)
{
    int i;
    GB_transaction ta(GLOBAL_gb_main);

    root->awar_int(ED4_AWAR_NDS_SELECT, 0)->add_callback(NDS_changed, 1);
    root->awar_int(ED4_AWAR_NDS_BRACKETS, 6)->set_minmax(0, 99)->add_callback(NDS_changed, 1);
    root->awar_int(ED4_AWAR_NDS_INFO_WIDTH, 5)->set_minmax(0, 99)->add_callback(NDS_changed, 1);
    root->awar_string(ED4_AWAR_NDS_ECOLI_NAME, "Ecoli")->add_callback(NDS_changed, 1);

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
        root->awar_string(buf, aci)->add_callback(NDS_changed, 1);

        int len;
        sprintf(buf, ED4_AWAR_NDS_WIDTH_TEMPLATE, i);
        switch (i) {
            case 0: len = 9; break;
            case 1: len = 27; break;
            default: len = 20; break;
        }
        root->awar_int(buf, len)->add_callback(NDS_changed, 1);
    }

    NDS_changed(root, 0); // init globals
}

// a crazy implementation of a toggle field
static void ed4_nds_select_change(AW_window *aww, int selected) {
    AW_root *aw_root = aww->get_root();
    for (int i=0; i<NDS_COUNT; i++) {
        const char *awar_name = GBS_global_string(ED4_AWAR_NDS_SELECT_TEMPLATE, i);
        aw_root->awar(awar_name)->write_int((i==selected) ? 1 : 0);
    }
    aw_root->awar(ED4_AWAR_NDS_SELECT)->write_int(selected);
}

AW_window *ED4_create_nds_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "NDS_PROPS", "NDS");
    aws->load_xfig("edit4/nds.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("ed4_nds.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

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

    int dummy, fieldselectx, description_x, aci_x, length_x;
    aws->get_at_position(&fieldselectx, &dummy);

    for (int i=0; i<NDS_COUNT; ++i) {
        char buf[256];
        sprintf(buf, ED4_AWAR_NDS_SELECT_TEMPLATE, i);
        aws->callback(makeWindowCallback(ed4_nds_select_change, i));
        aws->create_toggle(buf);

        aws->get_at_position(&description_x, &dummy);
        sprintf(buf, ED4_AWAR_NDS_DESCRIPTION_TEMPLATE, i);
        aws->create_input_field(buf, 15);

        aws->get_at_position(&aci_x, &dummy);
        sprintf(buf, ED4_AWAR_NDS_ACI_TEMPLATE, i);
        aws->create_input_field(buf, 30);

        aws->get_at_position(&length_x, &dummy);
        sprintf(buf, ED4_AWAR_NDS_WIDTH_TEMPLATE, i);
        aws->create_input_field(buf, 3);

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

