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

    createNamesList_callBack(svnp); // calling callback to get the alingments in the database

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
