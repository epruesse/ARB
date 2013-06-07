// =============================================================== //
//                                                                 //
//   File      : NT_validNames.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"
#include "NT_validNameParser.h"

#include <arbdbt.h>
#include <aw_msg.hxx>
#include <awt_www.hxx>

#include <fstream>
#include <iostream>
#include <iterator>

using namespace std;

#if defined(DEVEL_LOTHAR)
#define DUMP
#endif // DEVEL_LOTHAR

void NT_deleteValidNames(AW_window*, AW_CL, AW_CL)
{
    GB_ERROR error;

    GB_begin_transaction(GLOBAL.gb_main);
    GBDATA* namesCont = GB_search(GLOBAL.gb_main, "VALID_NAMES", GB_CREATE_CONTAINER);
    GB_write_security_delete(namesCont, 6);
    error = GB_delete(namesCont);
    if (error != 0) {
        aw_message("Valid Names container was not removed from database\nProtection level 6 needed");
    }
    else {
        aw_message("Valid Names container was removed from database\nThink again before saving");
    }
    GB_commit_transaction(GLOBAL.gb_main);

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
    const char *fileName = GB_path_in_ARBLIB("LoVPBN.txt");

    DescList myDescs;

    // file select dialog goes here
    try {
        ifstream namesFile(fileName);
        if (!namesFile.is_open()) {
            throw string("cannot open file \"") + fileName + "\" to read";
        }
        namesFile.unsetf(ios::skipws); // keep white spaces
        // undefined iterator theEnd denotes end of stream
        istream_iterator<char> inIter(namesFile), theEnd;

        std::cout << "Reading valid names from '" << fileName << "'\n";

        for (; inIter != theEnd; ++inIter) {
            if (*inIter == '\r') continue; // remove empty lines due to dos \r
            if (*inIter == '\n') {
                if (!tmpString.empty()) {  // check for newline
                    fileContent.push_back(tmpString);
                    tmpString = "";
                }
            }
            else {
                tmpString += *inIter;
            }
        }
        if (!tmpString.empty()) fileContent.push_back(tmpString); // if last line doesn't end with \n

        StrL::iterator it;
        bool isHeader = true;
        for (it = fileContent.begin(); it != fileContent.end(); ++it) {
            if (isHeader) {
                string nameStart ("ABIOTROPHIA");
                if (it->find(nameStart.c_str(), 0, 11) != string::npos) {
                    isHeader = false;
                    Desco myDesc =  validNames::determineType(*it);
#if defined(DUMP)
                    std::cout << string("valid name: ") << myDesc.getFirstName() << std::endl
                              << string("other name: \t\t") << myDesc.getSecondName() << std::endl;
#endif // DUMP
                    myDescs.push_back(myDesc);
                }
            }
            else {
                Desco myDesc =  validNames::determineType(*it);
#if defined(DUMP)
                std::cout << string("valid name: ") << myDesc.getFirstName() << std::endl
                          << string("other name: \t\t") << myDesc.getSecondName() << std::endl;
#endif // DUMP
                myDescs.push_back(myDesc);
            }

        }
        // continue here with database interaction
        GB_ERROR error        = GB_begin_transaction(GLOBAL.gb_main);
        GBDATA*  gb_namesCont = GB_entry(GLOBAL.gb_main, "VALID_NAMES");

        if (gb_namesCont != NULL) {
            error = "Container for Valid Names already exists\n Please delete old Valid Names first";
        }
        else {
            gb_namesCont             = GB_create_container(GLOBAL.gb_main, "VALID_NAMES");
            if (!gb_namesCont) error = GB_await_error();
            else {
                DescList::iterator di;
                for (di = myDescs.begin(); di != myDescs.end() && !error; ++di) {
                    if (di->getType() < 10) {
                        GBDATA* gb_pair     = GB_create_container(gb_namesCont, "pair");
                        if (!gb_pair) error = GB_await_error();
                        else {
                            error             = GBT_write_string(gb_pair, "OLDNAME", di->getSecondName().c_str());
                            if (!error) error = GBT_write_string(gb_pair, "NEWNAME", di->getFirstName().c_str());
                            if (!error) {
                                const char* typeStr  = 0;
                                switch (di->getType()) {
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
                                    default: nt_assert(0); break;
                                }
                                error = GBT_write_string(gb_pair, "DESCTYPE", typeStr);
                            }
                        }
                    }
                }
            }
        }
        error = GB_end_transaction(GLOBAL.gb_main, error);
        if (error) aw_message(error);
    }
    catch (string& err) { aw_message(err.c_str()); }
    catch (...) { aw_message("Unknown exception"); }
}


void NT_suggestValidNames(AW_window*, AW_CL, AW_CL) {
    GB_begin_transaction(GLOBAL.gb_main);

    GBDATA*  gb_validNamesCont = GB_entry(GLOBAL.gb_main, "VALID_NAMES");
    GB_ERROR err               = 0;

    if (!gb_validNamesCont) err = "No valid names imported yet";

    for (GBDATA *gb_species = GBT_first_species(GLOBAL.gb_main);
         !err && gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        // retrieve species names
        GBDATA*  gb_fullName = GB_entry(gb_species, "full_name"); // gb_fullname
        char    *fullName     = gb_fullName ? GB_read_string(gb_fullName) : 0;
        if (!fullName) err    = "Species has no fullname";

        // search validNames

        for (GBDATA *gb_validNamePair = GB_entry(gb_validNamesCont, "pair");
             gb_validNamePair && !err;
             gb_validNamePair = GB_nextEntry(gb_validNamePair))
        {
            // retrieve list of all species names

            GBDATA* gb_actDesc = GB_entry(gb_validNamePair, "DESCTYPE");
            char*   typeString = GB_read_string(gb_actDesc);

            nt_assert(strcmp(typeString, "NOTYPE") != 0);

            char *validName = GBT_read_string(gb_validNamePair, "NEWNAME");
            char *depName   = GBT_read_string(gb_validNamePair, "OLDNAME");

            if (!validName || !depName) err = GB_await_error();
            else {
                // now compare all names
                if (!err && ((strcmp(fullName, validName) == 0)||(strcmp(fullName, depName) == 0))) {
                    // insert new database fields if necessary
                    GBDATA* gb_validNameCont   = GB_search(gb_species, "Valid_Name", GB_CREATE_CONTAINER);
                    if (!gb_validNameCont) err = GB_await_error();
                    else {
                        err           = GBT_write_string(gb_validNameCont, "NameString", validName);
                        if (!err) err = GBT_write_string(gb_validNameCont, "DescType", typeString);
                    }
                }
            }

            free(validName);
            free(depName);
            free(typeString);
        }

        free(fullName);
    }

    GB_end_transaction_show_error(GLOBAL.gb_main, err, aw_message);
}
