// =============================================================== //
//                                                                 //
//   File      : AW_names_admin.cxx                                //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AW_rename.hxx"

#include <aw_window.hxx>
#include <aw_advice.hxx>
#include <aw_edit.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <awt_sel_boxes.hxx>

#include <arb_file.h>
#include <aw_question.hxx>

#define AWAR_NAMESERVER_STATUS "tmp/nameserver/status"

static char *namesFilename(GBDATA *gb_main) {
    const char *field    = AW_get_nameserver_addid(gb_main);
    const char *filename = field[0] ? GBS_global_string("names_%s.dat", field) : "names.dat";
    char       *fullname = nulldup(GB_path_in_ARBLIB("nas", filename));

    return fullname;
}

inline bool continue_with_namesDat_destruction(const char *kindOfDestruction) {
    return aw_ask_sure("MOD_NAMES_DAT",
        GBS_global_string("This may DESTROY the name-consistency for ALL USERS of names.dat!\n"
                          "Names will have to be regenerated for all databases.\n"
                          "\n"
                          "Are you REALLY sure you like to continue %s names.dat?", kindOfDestruction));
}

static void awtc_delete_names_file(AW_window*, GBDATA *gb_main) {
    if (continue_with_namesDat_destruction("deleting")) {
        char     *path    = namesFilename(gb_main);
        char     *newpath = GBS_string_eval(path, "*=*%", 0);
        GB_ERROR  error   = GB_rename_file(path, newpath);
        if (error) aw_message(error);
        free(newpath);
        free(path);
    }
}

static void awtc_edit_names_file(AW_window*, GBDATA *gb_main) {
    if (continue_with_namesDat_destruction("editing")) {
        char *path = namesFilename(gb_main);
        AW_edit(path);
        free(path);
    }
}

static void awtc_remove_arb_acc(AW_window*, GBDATA *gb_main) {
    if (continue_with_namesDat_destruction("filtering")) {
        char *path    = namesFilename(gb_main);
        char *newpath = GBS_string_eval(path, "*=*%", 0);
        char *command = GBS_global_string_copy("cp %s %s;"
                                               "arb_replace -l '"
                                               "*ACC {}*=:" // remove entries w/o acc
                                               "*ACC {ARB*='" // remove entries with 'ARB_' prefix (Note: Nameserver does not store the '_'!)
                                               " %s",
                                               path, newpath, path);
        GB_ERROR error = GBK_system(command);
        if (error) aw_message(error);

        free(command);
        free(newpath);
        free(path);
    }
}

static void addid_changed_cb(AW_root *awr, GBDATA *gb_main, bool show_advice) {
    GB_ERROR error = AW_test_nameserver(gb_main);

    awr->awar(AWAR_NAMESERVER_STATUS)->write_string(error ? error : "ok");
    if (!error && show_advice) {
        AW_advice("Calling 'Species/Synchronize IDs' is highly recommended", AW_ADVICE_TOGGLE_AND_HELP, 0, "namesadmin.hlp");
    }
}

void AW_create_namesadmin_awars(AW_root *awr, GBDATA *gb_main) {
    awr->awar_string(AWAR_NAMESERVER_ADDID, "", gb_main);
    awr->awar_string(AWAR_NAMESERVER_STATUS, "<unchecked>", gb_main);
}

AW_window *AW_create_namesadmin_window(AW_root *root, GBDATA *gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "NAME_SERVER_ADMIN", "NAME_SERVER ADMIN");

    aws->load_xfig("awtc/names_admin.fig");

    aws->at("help");
    aws->callback(makeHelpCallback("namesadmin.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->button_length(30);

    aws->sens_mask(AWM_EXP);
    {
        aws->at("delete");
        aws->callback(makeWindowCallback(awtc_delete_names_file, gb_main));
        aws->create_button("DELETE_OLD_NAMES_FILE", "Delete old names file");

        aws->at("edit");
        aws->callback(makeWindowCallback(awtc_edit_names_file, gb_main));
        aws->create_button("EDIT_NAMES_FILE", "Edit names file");

        aws->at("remove_arb");
        aws->callback(makeWindowCallback(awtc_remove_arb_acc, gb_main));
        aws->create_button("REMOVE_SUPERFLUOUS_ENTRIES_IN_NAMES_FILE",
                           "Remove all entries with an\n'ARB*' accession number\nfrom names file");

        aws->at("config");
        aws->callback(makeWindowCallback(awt_edit_arbtcpdat_cb, gb_main));
        aws->create_button("CREATE_TEMPLATE", "Configure arb_tcp.dat");
    }
    aws->sens_mask(AWM_ALL);

    AW_awar *awar_addid = root->awar(AWAR_NAMESERVER_ADDID);
    awar_addid->add_callback(makeRootCallback(addid_changed_cb, gb_main, true));

    aws->at("add_field");
    aws->create_input_field(AWAR_NAMESERVER_ADDID, 20); // @@@ use field selection

    aws->at("status");
    aws->button_length(50);
    aws->create_button(NULL, AWAR_NAMESERVER_STATUS);

    addid_changed_cb(root, gb_main, false); // force status update

    return aws;
}
