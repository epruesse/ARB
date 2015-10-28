// =============================================================== //
//                                                                 //
//   File      : NT_validManual.cxx                                //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <aw_select.hxx>
#include <arbdbt.h>

#include <string>
#include <list>
#include <vector>
#include <fstream>
#include <iostream>

#define AWAR_SELECTED_VALNAME "tmp/validNames/selectedName"
#define AWAR_INPUT_INITIALS   "tmp/validNames/inputInitials"

struct selectValidNameStruct {
    GBDATA            *gb_main;
    AW_window         *aws;
    AW_root           *awr;
    AW_selection_list *validNamesList;
    const char        *initials;
};

// --------------------------creating and initializing AWARS----------------------------------------
void NT_createValidNamesAwars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_SELECTED_VALNAME, "????",   aw_def);
    aw_root->awar_string(AWAR_INPUT_INITIALS,    "",      aw_def);
}

static void fillSelNamList(selectValidNameStruct* svnp) {
    const char* searchstr = svnp -> initials;
    size_t length = strlen(searchstr);
    svnp->validNamesList->clear();

    GB_begin_transaction(GLOBAL.gb_main);

    GBDATA* GB_validNamesCont = GB_entry(GLOBAL.gb_main, "VALID_NAMES");
    if (!GB_validNamesCont) { std::cout << "validNames Container not found\n"; }

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
            char* validName = newName ? GB_read_string(newName) : NULL;

            if (!validName) {
                err = GBS_global_string("Invalid names entry");
            }
            else {
                // comparison with searchstr goes here
                // ptr to list, item to display, item value (here: equal)

                if (strncmp (validName, searchstr, length) == 0) {
                    svnp->validNamesList->insert(validName, validName);
                }

                free(validName);
            }
        }
        free(typeString);
    }

    err = GB_end_transaction(GLOBAL.gb_main, err);
    if (err) aw_message(err);
    else {
        svnp->validNamesList->insert_default("????", "????");
        svnp->validNamesList->sort(false, true);
        svnp->validNamesList->update();
    }
}

static void updateValNameList(AW_root *awr, selectValidNameStruct *svnp) {
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

static selectValidNameStruct* createValNameList(GBDATA *gb_main, AW_window *aws, const char *awarName) {
#if defined(DUMP)
    aw_message("ValidNameSelectionList was created");
#endif // DUMP

    static selectValidNameStruct* svnp = new selectValidNameStruct; // declared static

    svnp->aws            = aws;
    svnp->gb_main        = gb_main;
    svnp->validNamesList = aws->create_selection_list(awarName, 10, 20, true);
    svnp->initials       = "";

    fillSelNamList(svnp);
    return svnp;
}

static void selectValidNameFromList(AW_window* selManWindowRoot) {
    // transfers the selected valid name to selected species
    char *selectedValName     = selManWindowRoot->get_root()->awar(AWAR_SELECTED_VALNAME)->read_string();
    char *selectedSpeciesName = selManWindowRoot->get_root()->awar(AWAR_SPECIES_NAME)->read_string();

    GB_ERROR err = 0;
    if (selectedSpeciesName[0] == 0) err = "No species selected";
    else {
        err = GB_begin_transaction(GLOBAL.gb_main);
        if (!err) {
            GBDATA *gb_selected_species = GBT_find_species(GLOBAL.gb_main, selectedSpeciesName);
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
        err = GB_end_transaction(GLOBAL.gb_main, err);
    }

    if (err) aw_message(err);

    free(selectedSpeciesName);
    free(selectedValName);
}

AW_window *NT_create_searchManuallyNames_window(AW_root *aw_root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "SEARCH_VALID_NAMES_MANUALLY", "Search Names Manually");
    aws->load_xfig("ad_selManNam.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN); // arb specific close callback, like a constant
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("vn_search.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("nameList");
    // creates the selection list and asign AWAR_SELECTED_VALNAME

    selectValidNameStruct *vns = createValNameList(GLOBAL.gb_main, aws, AWAR_SELECTED_VALNAME);

    aws->at("select");
    aws->callback(selectValidNameFromList);
    aws->create_button("SELECT", "SELECT");

    aws->at("enterInitial");
    aws->create_input_field(AWAR_INPUT_INITIALS,  30);

    aw_root->awar(AWAR_INPUT_INITIALS)->add_callback(makeRootCallback(updateValNameList, vns));


    return aws;

}
