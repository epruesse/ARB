

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

#include <fstream>
#include <iostream>

#include "nt_validNames.hxx"
#include "nt_validNameParser.hxx"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define nt_assert(bed) arb_assert(bed)


extern GBDATA* gb_main;


void NT_deleteValidNames(AW_window*, AW_CL, AW_CL)
{
  GB_ERROR error;

  GB_begin_transaction(gb_main);
  GBDATA* namesCont = GB_search(gb_main, "VALID_NAMES",GB_CREATE_CONTAINER);
  GB_write_security_delete(namesCont,6);
  error = GB_delete(namesCont);
  if (error != 0) {aw_message("Valid Names container was not removed from database\nProtection level 6 needed");}
  else{
   aw_message("Valid Names container was removed from database\nThink again before saving");
  }
  GB_commit_transaction(gb_main);



  std::cout << "DeleteValidNames was selected" << std::endl;

}





// AW_window *nt_create_intro_window(AW_root *awr)
// {
//   AW_window_simple *aws = new AW_window_simple;
//   aws->init( awr, "ARB_INTRO", "ARB INTRO", 400, 100 );
//   aws->load_xfig("arb_intro.fig");

//   aws->callback( (AW_CB0)exit);
//   aws->at("close");
//   aws->create_button("CANCEL","CANCEL","C");

//   aws->at("help");
//   aws->callback(AW_POPUP_HELP,(AW_CL)"arb_intro.hlp");
//   aws->create_button("HELP","HELP","H");

//   awt_create_selection_box(aws,"tmp/nt/arbdb");

//   aws->button_length(0);

//   aws->at("logo");
//   aws->create_button(0,"#logo.bitmap");

//   // 	aws->button_length(25);

//   aws->at("old");
//   aws->callback(nt_intro_start_old);
//   aws->create_autosize_button("OPEN_SELECTED","OPEN SELECTED","O");

//   aws->at("del");
//   aws->callback(nt_delete_database);
//   aws->create_autosize_button("DELETE_SELECTED","DELETE SELECTED");

//   aws->at("new_complex");
//   aws->callback(nt_intro_start_import);
//   aws->create_autosize_button("CREATE_AND_IMPORT","CREATE AND IMPORT","I");

//   aws->at("merge");
//   aws->callback((AW_CB1)nt_intro_start_merge,0);
//   aws->create_autosize_button("MERGE_TWO_DATABASES","MERGE TWO ARB DATABASES","O");

//   aws->at("novice");
//   aws->create_toggle("NT/GB_NOVICE");

//   return (AW_window *)aws;
// }

// GB_ERROR create_nt_window(AW_root *aw_root){
//   AW_window *aww;
//   GB_ERROR error = GB_request_undo_type(gb_main, GB_UNDO_NONE);
//   if (error) aw_message(error);
//   create_all_awars(aw_root,AW_ROOT_DEFAULT);
//   if (GB_NOVICE){
//     aw_root->set_sensitive(AWM_BASIC);
//   }
//   aww = create_nt_main_window(aw_root,0);
//   aww->show();
//   error = GB_request_undo_type(gb_main, GB_UNDO_UNDO);
//   if (error) aw_message(error);
//   return error;
// }


void NT_importValidNames(AW_window*, AW_CL, AW_CL)

{
  using namespace std;
  using validNames::Desco;
  //  using validNames::typeNames;
  //  typedef list<string> StrL;
  typedef vector<string> StrL;
  typedef vector<Desco> DescList;
  //  typedef StrL* StrLPtr;
  string tmpString;
  StrL fileContent;
  const char* fileName;
  fileName = "unixnames.txt";
  DescList myDescs;
  // file select dialog goes here




  try{
    ifstream namesFile(fileName);
    if (!namesFile.is_open()){
      throw "cannot open file \"" + string(fileName) + "\" to read";
    }
    namesFile.unsetf(ios::skipws); // keep white spaces
    // undefined iterator theEnd denotes end of stream
    istream_iterator<char> inIter(namesFile), theEnd;

    for ( ;inIter != theEnd; ++inIter) {
      if (*inIter == '\r') continue; // remove empty lines due to dos \r
      if (*inIter == '\n'){
	if ( !tmpString.empty())   // check for newline
	  {
	    fileContent.push_back(tmpString);
	    //	  std::cout << tmpString << std::endl;
	    tmpString = "";
	  }else{};
      }else{
	tmpString += *inIter;
      }
    } // closes file automatically
    // while (getline(namesFile, tmpString)){
    if (!tmpString.empty()) fileContent.push_back(tmpString); // if last line doesn't end with \n



    StrL::iterator it;
    bool isHeader = true;
    for (it = fileContent.begin(); it != fileContent.end(); it++){
      //   std::cout << *it << std::endl;
      //    std::cout << determineType(*it) << std::endl;
      if (isHeader){
	string nameStart ("ABIOTROPHIA");
	if(it->find(nameStart.c_str(), 0, 11) != string::npos){
	  isHeader = false;
	  Desco myDesc =  validNames::determineType(*it);
	  std::cout << string("valid name: ") << myDesc.getFirstName() << std::endl
		    << string("other name: \t\t") << myDesc.getSecondName() << std::endl;

	  myDescs.push_back(myDesc);
	}
      }else{
	Desco myDesc =  validNames::determineType(*it);
	std::cout << string("valid name: ") << myDesc.getFirstName() << std::endl
		  << string("other name: \t\t") << myDesc.getSecondName() << std::endl;

	  myDescs.push_back(myDesc);
     }

    }
    // continue here with database interaction

    GBDATA* namesCont;
    GBDATA* pair;
    GBDATA* oldName;
    GBDATA* newName;
    GBDATA* descType;
    DescList::iterator di;
    const char* typeStr;
    GB_begin_transaction(gb_main);
    //    namesCont = GB_search(gb_main, "VALID_NAMES",GB_CREATE_CONTAINER);
    namesCont = GB_find(gb_main, "VALID_NAMES", 0, down_level);
    if(namesCont != NULL){
      aw_message("Container for Valid Names already exists\n Please delete old Valid Names first");
    }else{
      namesCont = GB_create_container(gb_main, "VALID_NAMES");
      for ( di = myDescs.begin(); di != myDescs.end(); di++){
	if((*di).getType() < 10){
	  pair = GB_create_container(namesCont,"pair");
	  oldName = GB_create(pair, "OLDNAME", GB_STRING);
	  GB_write_string(oldName, ((*di).getSecondName()).c_str());
	  newName = GB_create(pair, "NEWNAME", GB_STRING);
	  GB_write_string(newName, ((*di).getFirstName()).c_str());
	  descType = GB_create(pair, "DESCTYPE", GB_STRING);
	  switch((*di).getType()) {
	  case 0: typeStr = "VALGEN"; break;
	  case 1: typeStr = "HETGEN"; break;
	  case 2: typeStr = "HOMGEN"; break;
	  case 3: typeStr = "RENGEN"; break;
	  case 4: typeStr = "CORGEN"; break;
	  case 5: typeStr = "VALSPEC"; break;
	  case 6: typeStr = "HETSPEC"; break;
	  case 7: typeStr = "HOMSPEC"; break;
	  case 8: typeStr = "RENSPEC"; break;
	  case 9: typeStr = "CORSPEC"; break;
	  default: typeStr = "NOTYPE"; break;
	  }

	  GB_write_string(descType, typeStr);
	}
      }
    }
    GB_commit_transaction(gb_main);



  }
  catch (...) {
    aw_message("unable to open Valid Names File");
    //    std::cout << "unable to open Valid Names File" << std::endl;
  }

}

// void NT_insertValidNames(AW_window*, AW_CL, AW_CL)
// {
//   using validNames::Desco;
//   //  printf ("Hallo\n");
//   NAMELIST oldFullNames; // stl container derived


//   GB_begin_transaction(gb_main);
//   GBDATA* GBT_actspecies = GBT_first_species(gb_main);

//   char* GBT_oldName = GBT_read_name(GBT_actspecies);
//   printf ("%s\n",  GBT_oldName);

//   GBDATA* GB_full_name = GB_find(GBT_actspecies,"full_name",0,down_level);
//   char* fullName =  GB_read_string(GB_full_name);
//   printf ("%s\n",  fullName);

// //     for (	gb_species = GBT_first_species(gb_main);
// //             gb_species;
// //             gb_species = GBT_next_species(gb_species) ){
// //         GB_write_flag(gb_species,flag);
// //     }

// // strange loop
//   for (GBDATA *spe=GBT_first_species(gb_main);
//        spe;
//        spe=GBT_next_species(spe)) ;

//   GBDATA* GBT_next;
//   while( (GBT_next = GBT_next_species(GBT_actspecies)) )
//     {
//       printf ("%s\n",  GBT_read_name(GBT_next));
//       GBDATA* GB_full_name = GB_find(GBT_next,"full_name",0,down_level);
//       char* fullName = (char*) GB_read_string(GB_full_name);
//       printf ("%s\n",  fullName);
//       oldFullNames.push_back(fullName);
//       GBT_actspecies = GBT_next;

//     }
//   GB_commit_transaction(gb_main);



// };


void NT_suggestValidNames(AW_window*, AW_CL, AW_CL)
{
    std::cout <<"Valid Names suggestion section" << std::endl;
    GBDATA* species;
    GBDATA* pair;
    GBDATA* oldName;
    GBDATA* newName;
    //  GBDATA* validName;
    GBDATA* GBT_fullName;
    // char* fullName;
    GBDATA* desc; // string
    NAMELIST speciesNames;
    // char* typeStr;

    GB_begin_transaction(gb_main);


    GBDATA* GB_validNamesCont = GB_find(gb_main, "VALID_NAMES", 0, down_level);
    if (!GB_validNamesCont){std::cout << "validNames Container not found" << std:: cout; return; }

    GB_ERROR err = 0;

    for (GBDATA *GBT_species=GBT_first_species(gb_main);
         GBT_species;
         GBT_species=GBT_next_species(GBT_species)){
        // retrieve species names
        GBDATA* GBT_fullName = GB_find(GBT_species,"full_name",0,down_level); // gb_fullname
        char *fullName =  GBT_fullName ? GB_read_string(GBT_fullName) : 0;
        if (!fullName) err = "Species has no fullname";

        //   printf ("%s\n",  fullName);

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
                GBDATA* oldName = GB_find(GB_validNamePair, "OLDNAME", 0, down_level);
                if (!oldName){std::cout << "oldName not found" << std:: cout; return; }  
                char* depName = GB_read_string(oldName);
                if (!depName){std::cout << "deprecatedName not found" << std:: cout; return; } 
                //           printf ("%s\n",  validName);

                if (!validName || !depName) {
                    err = GBS_global_string("Invalid names entry for %s",fullName);
                }
                
                // now compare all names
                if(!err && ( (strcmp(fullName, validName) == 0)||(strcmp(fullName, depName) == 0))) {
                    //insert new database fields if necessary

                    GBDATA* GB_speciesValidNameCont = GB_find(GBT_species,"Valid_Name",0, down_level);
                    if (GB_speciesValidNameCont == 0){GB_speciesValidNameCont = GB_create_container(GBT_species, "Valid_Name");}

                    GBDATA* GB_speciesValidName = GB_find(GB_speciesValidNameCont, "NameString", 0, down_level);
                    if (GB_speciesValidName == 0){GB_speciesValidName = GB_create(GB_speciesValidNameCont, "NameString", GB_STRING);}
                    GB_write_string(GB_speciesValidName, validName);
                
                    GBDATA* GB_speciesDescType = GB_find(GB_speciesValidNameCont, "DescType", 0, down_level);
                    if (GB_speciesDescType == 0){GB_speciesDescType = GB_create(GB_speciesValidNameCont, "DescType", GB_STRING);}
                    GB_write_string(GB_speciesDescType, typeString);

                    if(strcmp(fullName, validName) == 0){std::cout << "validspeciesname found" << std::endl;}
                    if(strcmp(fullName, depName) == 0){std::cout << "depspeciesname found" << std::endl;}

 
 
               }

                free (validName);
                free (depName);

            }
            free (typeString);             
        }

        free (fullName);
    } 

    if (err) {
        GB_abort_transaction(gb_main);
        aw_message(err);
    }
    else {
        GB_commit_transaction(gb_main);
    }
}
