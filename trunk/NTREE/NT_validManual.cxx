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
//extern GBDATA* GB_selected_species;

static AW_selection_list    *validNamesList;
static struct selectValidNameStruct* svnp;

/*--------------------------creating and initializing AWARS----------------------------------------*/
void NT_createValidNamesAwars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string( AWAR_SELECTED_VALNAME,      "????" ,                        aw_def);
    aw_root->awar_string( AWAR_INPUT_INITIALS,      "" ,                        aw_def);
}


//struct selectValidNameStruct*
void fillSelNamList(struct selectValidNameStruct* svnp){


    const char* searchstr = svnp -> initials;
    size_t length = strlen(searchstr);
    svnp-> aws-> clear_selection_list(svnp->validNamesList);

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

            // comparison with searchstr goes here
            //                         ptr to list, item to display, item value (here: equal)

            if (strncmp (validName, searchstr, length) == 0){
            svnp->aws->insert_selection(validNamesList,validName, validName);
            }

            free (validName);
        }

        free (typeString);
    }




    if (err) {
        GB_abort_transaction(gb_main);
        aw_message(err);
    }
    else {
        GB_commit_transaction(gb_main);

        svnp->aws->insert_default_selection(validNamesList , "????", "????" );
        svnp->aws->sort_selection_list(validNamesList , 0);
        svnp->aws->update_selection_list(validNamesList);
    }

}



//void awarTestFunction(AW_CL vns){
// void awarTestFunction(AW_root *awr){

//     //    const char* selectedName = ((struct selectValidNameStruct *)vns)->awr->awar(AWAR_INPUT_INITIALS)->read_string();
//     const char* selectedName = awr->awar(AWAR_INPUT_INITIALS)->read_string();

//     aw_message(GBS_global_string("now selected: %s\n",selectedName));

// }

void updateValNameList(AW_root *awr){

    const char* selectedName = awr->awar(AWAR_INPUT_INITIALS)->read_string();

#ifdef DEBUG
    aw_message(GBS_global_string("now selected: %s\n",selectedName));
#endif
    svnp->initials = selectedName;
    fillSelNamList(svnp);
#ifdef DEBUG
    aw_message(GBS_global_string("SelectionList updated"));
#endif
}



struct selectValidNameStruct* createValNameList(GBDATA *gb_main,AW_window *aws, const char *awarName){
    aw_message("ValidNameSelectionList was created");
//     GBDATA *gb_presets;
//     AW_root *aw_root  = aws->get_root();
    validNamesList = aws->create_selection_list(awarName,0,"",10,20);

    svnp = new struct selectValidNameStruct; // declared static
    svnp->aws = aws;
    svnp->gb_main = gb_main;
    svnp->validNamesList = validNamesList;
    svnp->initials = "";

    fillSelNamList(svnp);
    return svnp;



}

// this function transfer a the selected valid name upon mouse click on select button to selected species
void selectValidNameFromList(AW_window* selManWindowRoot, AW_CL, AW_CL)

{
    // working
    const char* selectedValName = selManWindowRoot->get_root()->awar(AWAR_SELECTED_VALNAME)->read_string();

    const char * selectedSpeciesName = selManWindowRoot->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    //    aw_message(GBS_global_string("actual marked species is %s", selectedSpeciesName));

    if(strncmp(selectedSpeciesName,"",strlen(selectedSpeciesName))==0){
        aw_message("No species selected!");
        return;
    }
#ifdef DEBUG
    else{
        aw_message(GBS_global_string("actual marked species is %s", selectedSpeciesName));
    }
#endif

    GB_begin_transaction(gb_main);
    GB_ERROR err = 0;


    // begin own code
    GBDATA* GB_selectedSpecies = GBT_find_species(gb_main, selectedSpeciesName);
#ifdef DEBUG
    const char * test = GB_read_string(GB_find(GB_selectedSpecies,"full_name",0,down_level));
    aw_message(GBS_global_string("species %s in database found", test ));
    aw_message("----test----");
#endif

    if(! GB_selectedSpecies)
        {err = "species not found in database";
    }else{ GBDATA* GB_nameCont = GB_search(GB_selectedSpecies,"Valid_Name",GB_CREATE_CONTAINER);

       if(! GB_nameCont)
           {err = "could not create Valid Name container in database";}
       else{
           GBDATA* GB_valName = GB_find(GB_nameCont,"NameString",0,down_level);
           if(! GB_valName){GB_valName = GB_create(GB_nameCont, "NameString", GB_STRING);}

           GBDATA* GB_valType = GB_find(GB_nameCont,"DescType",0,down_level);
           if(! GB_valType){GB_valType = GB_create(GB_nameCont, "DescType", GB_STRING);}

           GB_write_string(GB_valName, selectedValName);
           GB_write_string(GB_valType, "MAN");
       }
    }
  // end own code
    if (err) {
      GB_abort_transaction(gb_main);
        aw_message(err);
    }
    else {
        GB_commit_transaction(gb_main);

    }
#ifdef DEBUG
    aw_message(GBS_global_string("Selection confirmed: %s", selectedValName));
    printf("Selection confirmed: %s\n", selectedValName);

#endif
}



// create the manual selsection window
AW_window *NT_searchManuallyNames(AW_root *aw_root /*, AW_CL*/ )
{

    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SEARCH_VALID_NAMES_MANUALLY", "Search Names Manually", 50,10 );
    aws->load_xfig("ad_selManNam.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN); // arb specific close callback, like a constant
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("nameList");
    // creates the selection list and asign AWAR_SELECTED_VALNAME

    struct selectValidNameStruct *vns = createValNameList(gb_main, (AW_window *)aws, AWAR_SELECTED_VALNAME);

    aws->at("select");
    aws->callback( selectValidNameFromList,0,0); // (... 0,0)
    aws->create_button("SELECT","SELECT");

    aws->at("enterInitial");
    aws->create_input_field(AWAR_INPUT_INITIALS , 30 );

    //Awar_CB1 f, AW_CL cd1
    //    aw_root->awar(AWAR_INPUT_INITIALS)->add_callback((Awar_CB1)fillSelNamList,(AW_CL)vns);//
    //    aw_root->awar(AWAR_INPUT_INITIALS)->add_callback((Awar_CB1)awarTestFunction,(AW_CL)vns);
    aw_root->awar(AWAR_INPUT_INITIALS)->add_callback(updateValNameList); // awr seems to be given automatically


    return (AW_window *)aws;

}
