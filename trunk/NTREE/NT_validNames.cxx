

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
#include "nt_fileReader.hxx"
#include "nt_validNames.hxx"
#include "nt_validNameParser.hxx"

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

void NT_insertValidNames(AW_window*, AW_CL, AW_CL)
{
  using validNames::Desco;
  //  printf ("Hallo\n");
  NAMELIST oldFullNames; // stl container derived


  GB_begin_transaction(gb_main);
  GBDATA* GBT_actspecies = GBT_first_species(gb_main);

  char* GBT_oldName = GBT_read_name(GBT_actspecies);
  printf ("%s\n",  GBT_oldName);

  GBDATA* GB_full_name = GB_find(GBT_actspecies,"full_name",0,down_level);
  char* fullName =  GB_read_string(GB_full_name);
  printf ("%s\n",  fullName);

//     for (	gb_species = GBT_first_species(gb_main);
//             gb_species;
//             gb_species = GBT_next_species(gb_species) ){
//         GB_write_flag(gb_species,flag);
//     }

// strange loop
  for (GBDATA *spe=GBT_first_species(gb_main); 
       spe; 
       spe=GBT_next_species(spe)) ;

  GBDATA* GBT_next;
  while( (GBT_next = GBT_next_species(GBT_actspecies)) )
    {
      printf ("%s\n",  GBT_read_name(GBT_next));
      GBDATA* GB_full_name = GB_find(GBT_next,"full_name",0,down_level);
      char* fullName = (char*) GB_read_string(GB_full_name);
      printf ("%s\n",  fullName);
      oldFullNames.push_back(fullName);
      GBT_actspecies = GBT_next;

    }
  GB_commit_transaction(gb_main);

  
  
};


void NT_suggestValidNames(AW_window*, AW_CL, AW_CL)
{
  std::cout <<"Valid Names suggestion section" << std::endl;
  GBDATA* species;
  GBDATA* pair;
  GBDATA* oldName;
  GBDATA* newName;
  GBDATA* GBT_fullName;
  const char* fullName;
  GBDATA* desc; // string
  NAMELIST speciesNames;
  const char* typeStr;

  GB_begin_transaction(gb_main);
 
  for (GBDATA *GBT_species=GBT_first_species(gb_main); 
       GBT_species; 
       GBT_species=GBT_next_species(GBT_species)){
    // retrieve list of all species names
    GBDATA* GBT_fullName = GB_find(GBT_species,"full_name",0,down_level);
    fullName =  GB_read_string(GBT_fullName);
    printf ("%s\n",  fullName);
    GB_commit_transaction(gb_main);
  } ;

  /* 
     for (){
     // iterate all pairs to look for relevant names
     
     
     } ;
  */
 /*
   GB_begin_transaction(gb_main);
   // code goes here
   GB_commit_transaction(gb_main);
 */
}
