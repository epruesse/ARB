#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>
#include <vector>


#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>


#include <fstream>
#include <iostream>

#include "nt_validManual.hxx"
//#include "nt_validNameParser.hxx"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define nt_assert(bed) arb_assert(bed)


extern GBDATA* gb_main;

static AW_selection_list    *validNamesList;


/*--------------------------creating and initializing AWARS----------------------------------------*/
void NT_createValidNamesAwars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string( AWAR_SELECTED_VALNAME,      "unknown" ,                        aw_def);
} // "unknown"  stands probably for default value

// callback function for selection list event
void createNamesList_callBack(struct selectValidNameStruct *vns){
aw_message("ValidNameList was received");
// see NT_concatenate.cxx line 61

}


struct selectValidNameStruct* createSelNameList(GBDATA *gb_main,AW_window *aws, const char *awarName){
    aw_message("ValidNameSelectionList was created");
    GBDATA *gb_presets;
    AW_root *aw_root  = aws->get_root();
    validNamesList = aws->create_selection_list(awarName,0,"",10,20);
    


    struct selectValidNameStruct *svnp = 0;
    svnp = new struct selectValidNameStruct;
    svnp->aws = aws;
    svnp->gb_main = gb_main;
    svnp->validNamesList = validNamesList;

    GB_begin_transaction(gb_main);


    GBDATA* GB_validNamesCont = GB_find(gb_main, "VALID_NAMES", 0, down_level);
    if (!GB_validNamesCont){std::cout << "validNames Container not found" << std:: cout; }
    
    GB_ERROR err = 0;

    // search validNames

    for (GBDATA *GB_validNamePair = GB_find(GB_validNamesCont, "pair", 0, down_level);
         GB_validNamePair && !err;
         GB_validNamePair = GB_find(GB_validNamePair,"pair" ,0,this_level|search_next)) {
        //    if (!GB_validNamePair){std::cout << "GB_validNamePair not found" << std:: cout; return; } 
        // retrieve list of all species names
        
        GBDATA* actDesc = GB_find(GB_validNamePair, "DESCTYPE", 0, down_level);
        char* typeString = GB_read_string(actDesc);
        if (strcmp(typeString, "NOTYPE") != 0){
            GBDATA* newName = GB_find(GB_validNamePair, "NEWNAME", 0, down_level);
            char* validName = newName ? GB_read_string(newName) : 0;
            //if (!validName){std::cout << "validName not found" << std:: cout; return; } // string
            
            
            if (!validName) {
                err = GBS_global_string("Invalid names entry");
            }
            aws->insert_selection(validNamesList,validName, validName);            
            free (validName);
        }


        //        free (depName);
        free (typeString);                     
    }




    if (err) {
        GB_abort_transaction(gb_main);
        aw_message(err);
    }
    else {
        GB_commit_transaction(gb_main);

        svnp->aws->insert_default_selection(validNamesList , "????", "????" );
        svnp->aws->update_selection_list(validNamesList);
        createNamesList_callBack(svnp);
    }
        // adopt to own databse transaction
        /*    GB_push_transaction(gb_main);
              gb_presets = GB_search(gb_main,"presets",GB_CREATE_CONTAINER);
              GB_add_callback(gb_presets, GB_CB_CHANGED, createSelectionList_callBack_gb, (int *)cas);
              GB_pop_transaction(gb_main);
        */

        return svnp;


        //    return NULL;
    
}

// callback function for selection list event
void createSelectionList_callBack(struct selectValidNameStruct *vns){
aw_message("ValidNameSelectionList was received");
// see NT_concatenate.cxx line 61

}

// create the ValidName selectino window
AW_window *NT_searchManuallyNames(AW_root *aw_root /*, AW_CL*/)
{

    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SEARCH_VALID_NAMES_MANUALLY", "Search Names Manually", 50,10 );
    aws->load_xfig("ad_selManNam.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN); // arb specific close callback, like a constant
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("nameList");
    //    awt_create_selection_list_on_extendeds(gb_main,(AW_window *)aws,AWAR_SAI_SELECT);
    selectValidNameStruct *vns = createSelNameList(gb_main, (AW_window *)aws, AWAR_SELECTED_VALNAME);

    //   aws->at("label");
    //    aws->create_button(0,"Please enter the name\nfor the Color Translation Table");

    //    aws->at("input");
    //    aws->create_input_field(AWAR_SAI_COLOR_TRANS_TABLE_CREATE,15);

    //    aws->at("ok");
    //    aws->callback(createColorTranslationTable,0);
    //    aws->create_button("GO","GO","G");

    return (AW_window *)aws;

}
