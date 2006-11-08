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
#include <awt_www.hxx>

#include <fstream>
#include <iostream>
#include <iterator>

#include "nt_validNameParser.hxx"

using namespace std;

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define nt_assert(bed) arb_assert(bed)

#if defined(DEVEL_LOTHAR)
#define DUMP
#endif // DEVEL_LOTHAR

extern GBDATA* gb_main;

void NT_deleteValidNames(AW_window*, AW_CL, AW_CL)
{
    GB_ERROR error;

    GB_begin_transaction(gb_main);
    GBDATA* namesCont = GB_search(gb_main, "VALID_NAMES",GB_CREATE_CONTAINER);
    GB_write_security_delete(namesCont,6);
    error = GB_delete(namesCont);
    if (error != 0) {
        aw_message("Valid Names container was not removed from database\nProtection level 6 needed");
    }
    else{
        aw_message("Valid Names container was removed from database\nThink again before saving");
    }
    GB_commit_transaction(gb_main);

#if defined(DUMP)
    std::cout << "DeleteValidNames was selected" << std::endl;
#endif // DUMP

}

void NT_importValidNames(AW_window*, AW_CL, AW_CL) {
    using namespace        std;
    using                  validNames::Desco;
    typedef vector<string> StrL;
    typedef vector<Desco>  DescList;
    string                 tmpString;
    StrL                   fileContent;

    // Load LoVPBN (List of Validly Published Bacterial Names)
    char *fileName = GBS_global_string_copy("%s/lib/LoVPBN.txt", GB_getenvARBHOME());

    DescList myDescs;

    // file select dialog goes here
    try {
        ifstream namesFile(fileName);
        if (!namesFile.is_open()){
            throw string("cannot open file \"") + fileName + "\" to read";
        }
        namesFile.unsetf(ios::skipws); // keep white spaces
        // undefined iterator theEnd denotes end of stream
        istream_iterator<char> inIter(namesFile), theEnd;

        std::cout << "Reading valid names from '" << fileName << "'\n";

        for ( ;inIter != theEnd; ++inIter) {
            if (*inIter == '\r') continue; // remove empty lines due to dos \r
            if (*inIter == '\n'){
                if ( !tmpString.empty()) { // check for newline
                    fileContent.push_back(tmpString);
                    tmpString = "";
                }
            }
            else {
                tmpString += *inIter;
            }
        } // closes file automatically
        if (!tmpString.empty()) fileContent.push_back(tmpString); // if last line doesn't end with \n

        StrL::iterator it;
        bool isHeader = true;
        for (it = fileContent.begin(); it != fileContent.end(); it++){
            if (isHeader){
                string nameStart ("ABIOTROPHIA");
                if(it->find(nameStart.c_str(), 0, 11) != string::npos){
                    isHeader = false;
                    Desco myDesc =  validNames::determineType(*it);
#if defined(DUMP)
                    std::cout << string("valid name: ") << myDesc.getFirstName() << std::endl
                              << string("other name: \t\t") << myDesc.getSecondName() << std::endl;
#endif // DUMP
                    myDescs.push_back(myDesc);
                }
            }else{
                Desco myDesc =  validNames::determineType(*it);
#if defined(DUMP)
                std::cout << string("valid name: ") << myDesc.getFirstName() << std::endl
                          << string("other name: \t\t") << myDesc.getSecondName() << std::endl;
#endif // DUMP
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
        namesCont = GB_find(gb_main, "VALID_NAMES", 0, down_level);
        if(namesCont != NULL){
            aw_message("Container for Valid Names already exists\n Please delete old Valid Names first");
        }
        else {
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
    catch (string& err) { aw_message(err.c_str()); }
    catch (...) { aw_message("Unknown exception"); }

    free(fileName);
}


void NT_suggestValidNames(AW_window*, AW_CL, AW_CL) {
    vector<string> speciesNames;
    GB_begin_transaction(gb_main);

    GBDATA*  GB_validNamesCont = GB_find(gb_main, "VALID_NAMES", 0, down_level);
    GB_ERROR err               = 0;

    if (!GB_validNamesCont) err = "No valid names imported yet";

    for (GBDATA *GBT_species=GBT_first_species(gb_main);
         !err && GBT_species;
         GBT_species=GBT_next_species(GBT_species)){
        // retrieve species names
        GBDATA* GBT_fullName = GB_find(GBT_species,"full_name",0,down_level); // gb_fullname
        char *fullName =  GBT_fullName ? GB_read_string(GBT_fullName) : 0;
        if (!fullName) err = "Species has no fullname";

        // search validNames

        for (GBDATA *GB_validNamePair = GB_find(GB_validNamesCont, "pair", 0, down_level);
             GB_validNamePair && !err;
             GB_validNamePair = GB_find(GB_validNamePair,"pair" ,0,this_level|search_next)) {
            // retrieve list of all species names

            GBDATA* actDesc = GB_find(GB_validNamePair, "DESCTYPE", 0, down_level);
            char* typeString = GB_read_string(actDesc);
            if (strcmp(typeString, "NOTYPE") != 0){
                GBDATA* newName = GB_find(GB_validNamePair, "NEWNAME", 0, down_level);
                char* validName = newName ? GB_read_string(newName) : 0;
                GBDATA* oldName = GB_find(GB_validNamePair, "OLDNAME", 0, down_level);
                if (!oldName) { std::cout << "oldName not found" << std:: cout; return; }
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
