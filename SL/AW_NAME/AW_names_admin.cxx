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

static char *namesFilename(AW_CL cl_gb_main) {
    const char *field    = AW_get_nameserver_addid((GBDATA*)cl_gb_main);
    const char *filename = field[0] ? GBS_global_string("names_%s.dat", field) : "names.dat";
    char       *fullname = nulldup(GB_path_in_ARBLIB("nas", filename));

    return fullname;
}

static void awtc_delete_names_file(AW_window */*aws*/, AW_CL cl_gb_main) {
    char     *path    = namesFilename(cl_gb_main);
    char     *newpath = GBS_string_eval(path, "*=*%", 0);
    GB_ERROR  error   = GB_rename_file(path, newpath);
    if (error) aw_message(error);
    free(newpath);
    free(path);
}

static void awtc_edit_names_file(AW_window * /* aws */, AW_CL cl_gb_main) {
    char *path = namesFilename(cl_gb_main);
    AW_edit(path);
    free(path);
}

static void awtc_remove_arb_acc(AW_window * /* aws */, AW_CL cl_gb_main) {
    char *path    = namesFilename(cl_gb_main);
    char *newpath = GBS_string_eval(path, "*=*%", 0);
    char *command = GBS_global_string_copy("cp %s %s;"
                                           "arb_replace -l '"
                                           "*ACC {}*=:" // remove entries w/o acc
                                           "*ACC {ARB*='" // remove entries with 'ARB_' prefix (Note: Nameserver does not store the '_'!)
                                           " %s",
                                           path, newpath, path);
    GB_ERROR error = GB_system(command);
    if (error) aw_message(error);

    free(command);
    free(newpath);
    free(path);
}

static void addid_changed_cb(AW_root *, AW_CL cl_gb_main) {
    GBDATA   *gb_main = (GBDATA*)cl_gb_main;
    GB_ERROR  error   = AW_test_nameserver(gb_main);

    if (error) aw_message(error);
    else AW_advice("Calling 'Species/Generate New Names' is highly recommended", AW_ADVICE_TOGGLE|AW_ADVICE_HELP, 0, "namesadmin.hlp");
}

void AW_create_namesadmin_awars(AW_root *awr, GBDATA *gb_main) {
    awr->awar_string(AWAR_NAMESERVER_ADDID, "", gb_main);
}

AW_window *AW_create_namesadmin_window(AW_root *root, AW_CL cl_gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "NAME_SERVER_ADMIN", "NAME_SERVER ADMIN");

    aws->load_xfig("awtc/names_admin.fig");

    aws->callback(AW_POPUP_HELP, (AW_CL)"namesadmin.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->button_length(30);

    aws->at("delete");
    aws->callback(awtc_delete_names_file, cl_gb_main);
    aws->create_button("DELETE_OLD_NAMES_FILE", "Delete old names file");

    aws->at("edit");
    aws->callback(awtc_edit_names_file, cl_gb_main);
    aws->create_button("EDIT_NAMES_FILE", "Edit names file");

    aws->at("remove_arb");
    aws->callback(awtc_remove_arb_acc, cl_gb_main);
    aws->create_button("REMOVE_SUPERFLUOUS_ENTRIES_IN_NAMES_FILE",
                       "Remove all entries with an\n'ARB*' accession number\nfrom names file");

    AW_awar *awar_addid = root->awar(AWAR_NAMESERVER_ADDID);
    awar_addid->add_callback(addid_changed_cb, cl_gb_main);

    aws->at("add_field");
    aws->create_input_field(AWAR_NAMESERVER_ADDID, 20);

    aws->at("config");
    aws->callback(awt_edit_arbtcpdat_cb, cl_gb_main);
    aws->create_button("CREATE_TEMPLATE", "Configure arb_tcp.dat");

    return aws;
}
