// =============================================================== //
//                                                                 //
//   File      : NT_validManual.cxx                                //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <awt.hxx>
#include <aw_msg.hxx>

#include <string>
#include <list>
#include <vector>
#include <fstream>
#include <iostream>

#define nt_assert(bed) arb_assert(bed)

#define AWAR_SELECTED_VALNAME "tmp/validNames/selectedName"
#define AWAR_INPUT_INITIALS   "tmp/validNames/inputInitials"

extern GBDATA *GLOBAL_gb_main;

struct selectValidNameStruct {
    GBDATA            *gb_main;
    AW_window         *aws;
    AW_root           *awr;
    AW_selection_list *validNamesList;
    const char        *initials;
};

/* --------------------------creating and initializing AWARS---------------------------------------- */
void NT_createValidNamesAwars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_SELECTED_VALNAME, "????",   aw_def);
    aw_root->awar_string(AWAR_INPUT_INITIALS,    "",      aw_def);
}

void fillSelNamList(selectValidNameStruct* svnp) {
    const char* searchstr = svnp -> initials;
    size_t length = strlen(searchstr);
    svnp-> aws-> clear_selection_list(svnp->validNamesList);

    GB_begin_transaction(GLOBAL_gb_main);

    GBDATA* GB_validNamesCont = GB_entry(GLOBAL_gb_main, "VALID_NAMES");
    if (!GB_validNamesCont) { std::cout << "validNames Container not found" << std:: cout; }

    GB_ERROR err = 0;

    // search validNames

    for (GBDATA *GB_validNamePair = GB_entry(GB_validNamesCont, "pair");
         GB_validNamePair && !err;
         GB_validNamePair = GB_nextEntry(GB_validNamePair))
    {
        // retrieve list of all species names
        GBDATA* actDesc = GB_entry(GB_validNamePair, "DESCTYPE");
        char* typeString = GB_read_string(actDesc);
        if (strcmp(typeString, "NOTYPE") != 0) {
            GBDATA* newName = GB_entry(GB_validNamePair, "NEWNAME");
            char* validName = newName ? GB_read_string(newName) : 0;

            if (!validName) {
                err = GBS_global_string("Invalid names entry");
            }

            // comparison with searchstr goes here
            // ptr to list, item to display, item value (here: equal)

            if (strncmp (validName, searchstr, length) == 0) {
                svnp->aws->insert_selection(svnp->validNamesList, validName, validName);
            }

            free(validName);
        }
        free(typeString);
    }

    err = GB_end_transaction(GLOBAL_gb_main, err);
    if (err) aw_message(err);
    else {
        svnp->aws->insert_default_selection(svnp->validNamesList,  "????", "????");
        svnp->aws->sort_selection_list(svnp->validNamesList, 0, 1);
        svnp->aws->update_selection_list(svnp->validNamesList);
    }
}

void updateValNameList(AW_root *awr, AW_CL cl_svnp) {
    selectValidNameStruct *svnp = reinterpret_cast<selectValidNameStruct*>(cl_svnp);
    const char* selectedName = awr->awar(AWAR_INPUT_INITIALS)->read_string();

#ifdef DUMP
    aw_message(GBS_global_string("now selected: %s\n", selectedName));
#endif
    svnp->initials = selectedName;
    fillSelNamList(svnp);
#ifdef DUMP
    aw_message(GBS_global_string("SelectionList updated"));
#endif
}

selectValidNameStruct* createValNameList(GBDATA *gb_main, AW_window *aws, const char *awarName) {
#if defined(DUMP)
    aw_message("ValidNameSelectionList was created");
#endif // DUMP

    static selectValidNameStruct* svnp = new selectValidNameStruct; // declared static

    svnp->aws            = aws;
    svnp->gb_main        = gb_main;
    svnp->validNamesList = aws->create_selection_list(awarName, 0, "", 10, 20);
    svnp->initials       = "";

    fillSelNamList(svnp);
    return svnp;
}

// this function transfer a the selected valid name upon mouse click on select button to selected species
void selectValidNameFromList(AW_window* selManWindowRoot, AW_CL, AW_CL)

{
    char *selectedValName     = selManWindowRoot->get_root()->awar(AWAR_SELECTED_VALNAME)->read_string();
    char *selectedSpeciesName = selManWindowRoot->get_root()->awar(AWAR_SPECIES_NAME)->read_string();

    GB_ERROR err = 0;
    if (selectedSpeciesName[0] == 0) err = "No species selected";
    else {
        err = GB_begin_transaction(GLOBAL_gb_main);
        if (!err) {
            GBDATA *gb_selected_species = GBT_find_species(GLOBAL_gb_main, selectedSpeciesName);
            if (!gb_selected_species) {
                err = GBS_global_string("species '%s' not found in database", selectedSpeciesName);
            }
            else {
                GBDATA *gb_name_cont = GB_search(gb_selected_species, "Valid_Name", GB_CREATE_CONTAINER);

                if (!gb_name_cont) err = "could not create Valid Name container in database";
                else {
                    err           = GBT_write_string(gb_name_cont, "NameString", selectedValName);
                    if (!err) err = GBT_write_string(gb_name_cont, "DescType", "MAN");
                }
            }
        }
        err = GB_end_transaction(GLOBAL_gb_main, err);
    }

    if (err) aw_message(err);

    free(selectedSpeciesName);
    free(selectedValName);
}

// create the manual selsection window
AW_window *NT_searchManuallyNames(AW_root *aw_root /* , AW_CL */)
{

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "SEARCH_VALID_NAMES_MANUALLY", "Search Names Manually");
    aws->load_xfig("ad_selManNam.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN); // arb specific close callback, like a constant
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"vn_search.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at("nameList");
    // creates the selection list and asign AWAR_SELECTED_VALNAME

    selectValidNameStruct *vns = createValNameList(GLOBAL_gb_main, aws, AWAR_SELECTED_VALNAME);

    aws->at("select");
    aws->callback(selectValidNameFromList, 0, 0); // (... 0,0)
    aws->create_button("SELECT", "SELECT");

    aws->at("enterInitial");
    aws->create_input_field(AWAR_INPUT_INITIALS,  30);

    aw_root->awar(AWAR_INPUT_INITIALS)->add_callback(updateValNameList, (AW_CL)vns);


    return aws;

}
