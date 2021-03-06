/*
 * Definition of all objects belonging to this version of
 * the valid names text file
 *
 * 29. November 2002
 *
 * coded by Lothar Richter
 *
 * Copyright (C) 2002 Department of Microbiology (Technical University Munich)
 */

#if defined(DEVEL_LOTHAR)
#define DUMP
#endif // DEVEL_LOTHAR

#include "NT_validNameParser.h"
#include "NT_local.h"

#include <cstdlib>
#include <cstdlib>
#include <iostream>
#include <fstream>

using namespace std;

namespace validNames {


    TokLPtr tokenize(const std::string& description, TokLPtr tokenLP) {
        size_t tokenEnd = 0;
        size_t tokenBegin = 0;

        while (tokenEnd != description.size()) { // CC : warning: comparison between signed and unsigned (tokenEnd sollte nicht 'int' sondern 'unsigned' sein)
            tokenEnd = description.find_first_of(' ', tokenBegin);
            if (tokenEnd == string::npos) tokenEnd = description.size();
            int tokLength = tokenEnd - tokenBegin;
            if (tokLength != 0) {
                tokenLP->push_back(description.substr(tokenBegin, tokenEnd - tokenBegin));
            }
            tokenBegin = tokenEnd + 1;

        }
        return tokenLP;
    }





    Desco determineType(const string& descriptionString)
    { // begin determineType

        DESCT actType = NOTYPE;
        TokLPtr tokenLP =  new TokL;
        tokenLP = tokenize(descriptionString, tokenLP);
        // remove all tokens in parentheses
        {
            TokL::iterator it = tokenLP->begin();
            while (it != tokenLP->end()) {
                if (((*it).at(0) == '(') && *it != string("(corrig.)")) it = tokenLP->erase(it);
                else ++it;
            }
        }

        // check first word for upper case letters
        string descNames[6]; // first the valid genus, species, subsp. then the other names
        // stores occurrence of subsp. which is needed to retrieve the right tokens later on and status flags
        int sspPos[2] = { 0, 0 }; // token subsp. occurs maximum twice
        int ssp = 0;
        bool isValid = true;
        bool isRenamed = false;
        bool isHetero = false;
        bool isHomo = false;
        bool isGenus = false;
        // bool isSee = false;
        bool isCorr = false;



        for (TokL::iterator it = tokenLP->begin(); it != tokenLP->end(); ++it, ++ssp) {
            if (isUpperCase(*it)) {
                isGenus = true;
#if defined(DUMP)
                std::cout << "genus detected" << std::endl;
#endif // DUMP
            }


            else { // begin operators
                if (*it == string("->")) {
                    nt_assert(!isHetero);
                    nt_assert(!isHomo);
                    nt_assert(isValid); // only one operator per line allowed
                    isRenamed = true;
                    isValid   = false;
#if defined(DUMP)
                    std::cout << "renaming detected" << std::endl;
#endif // DUMP
                }
                else {
                    if (*it == string("=>")) {
                        nt_assert(!isRenamed);
                        nt_assert(!isHomo);
                        nt_assert(isValid);
                        isHetero = true;
                        isValid = false;
#if defined(DUMP)
                        std::cout << "heteronym detected" << std::endl;
#endif // DUMP
                    }
                    else {
                        if (*it == string("=")) {
                            nt_assert(!isRenamed);
                            nt_assert(!isHetero);
                            nt_assert(isValid);
                            isHomo = true;
                            isValid = false;
#if defined(DUMP)
                            std::cout << "homonym detected" << std::endl;
#endif // DUMP
                        }
                        else {
                            if (*it == string("(corrig.)")) {
                                isCorr = true;
#if defined(DUMP)
                                std::cout << "correction" << std::endl;
#endif // DUMP
                            }
                            else {
                                if (*it == string("see:")) {
                                    // isSee = true;
                                    isValid = false;
#if defined(DUMP)
                                    std::cout << "reference" << std::endl;
#endif // DUMP
                                }
                                else {
                                    if (*it == string("subsp.")) {
#if defined(DUMP)
                                        std::cout << "subspecies detected at position: >>>" << ssp << "<<<" << std::endl;
#endif // DUMP
                                        ssp == 2 ? sspPos[0] = ssp : sspPos[1] = ssp;
                                        // max. one subsp. on each operator side
#if defined(DUMP)
                                        std::cout << "position of subsp.: " << sspPos[0] << "\tand: " << sspPos[1] << std::endl;
#endif // DUMP
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }



        if (isGenus) {
#if defined(DUMP)
            std::cout << " GENUS description found " << std::endl;
#endif // DUMP
            if (isValid) {
                descNames[0] = (*tokenLP)[0];
                actType      = VALGEN;
#if defined(DUMP)
                std::cout << "VALIDGEN type set to: " << actType << std::endl;
#endif// DUMP
            }
            else {
                if (isHetero) {
                    descNames[0] = (*tokenLP)[2];
                    descNames[3] = (*tokenLP)[0];
                    actType = HETGEN;
#if defined(DUMP)
                    std::cout << "HETERONYMGEN type set to: " << actType << std::endl;
#endif // DUMP
                }
                else {
                    if (isHomo) {
                        descNames[0] = (*tokenLP)[2];
                        descNames[3] = (*tokenLP)[0];
                        actType = HOMGEN;
#if defined(DUMP)
                        std::cout << "HOMONYMGEN type set to: " << actType << std::endl;
#endif // DUMP

                    }
                    else {

                        if (isRenamed) {
                            descNames[0] = (*tokenLP)[2];
                            descNames[3] = (*tokenLP)[0];
                            actType = RENGEN;
#if defined(DUMP)
                            std::cout << "RENAMEDGEN type set to: " << actType << std::endl;
#endif // DUMP
                        }
                        else {
#if defined(DUMP)
                            std::cout << "no meaningful combination of conditions reached" << std::endl
                                      << "for line: " << descriptionString << std::endl;
                            std::cout << "description type is set to NOTYPE: " << NOTYPE << std::endl;
#endif // DUMP
                            isValid = false;
#if defined(DUMP)
                            std::cout << "isValid set to false "  << std::endl;
#endif // DUMP
                            actType = NOTYPE;
                        }
                    }
                }
            }
        }
        else {

            //       just fancy experimental , maybe not 100% correct but looks good
            if (!(((sspPos[0] == 0) || (sspPos[0] == 2)) && (((sspPos[1] > 4)&&(sspPos[1]< 9))||(sspPos[1]==0))))
            {
#if defined(DUMP)
                std::cout << "subsp. at strange position found in line:" << std::endl << descriptionString << endl;
                std::cout << "description type is set to NOTYPE: " << NOTYPE << std::endl;
#endif // DUMP
                isValid = false;
#if defined(DUMP)
                std::cout << "isValid set to false "  << std::endl;
#endif // DUMP
                actType = NOTYPE;
            }

            if (isValid) {
                descNames[0] = (*tokenLP)[0];
                descNames[1] = (*tokenLP)[1];
                if (sspPos[0] != 0) { descNames[2] = (*tokenLP)[sspPos[0]+1]; } // only if subsp. exists
                actType = VALSPEC;
            }
            else { // begin else isHetero
                if (isHetero) {
                    descNames[0] = (*tokenLP)[3 + sspPos[0]];
                    descNames[1] = (*tokenLP)[4 + sspPos[0]];
                    if (sspPos[1]!=0) { descNames[2]=(*tokenLP)[6 + sspPos[0]]; } // only if subsp. exists

                    descNames[3] = (*tokenLP)[0];
                    descNames[4] = (*tokenLP)[1];
                    if (sspPos[0]!=0) { descNames[5]=(*tokenLP)[sspPos[0]+1]; } // only if subsp. exists

                    actType = HETSPEC;
                }
                else {
                    if (isHomo) {
                        descNames[0] = (*tokenLP)[3 + sspPos[0]];
                        descNames[1] = (*tokenLP)[4 + sspPos[0]];
                        if (sspPos[1]!=0) { descNames[2]=(*tokenLP)[6 + sspPos[0]]; } // only if subsp. exists

                        descNames[3] = (*tokenLP)[0];
                        descNames[4] = (*tokenLP)[1];
                        if (sspPos[0]!=0) { descNames[5]=(*tokenLP)[sspPos[0]+1]; } // only if subsp. exists

                        actType = HOMSPEC;

                    }
                    else { // else branch isHomo
                        if (isRenamed) {
                            descNames[0] = (*tokenLP)[3 + sspPos[0]];
                            descNames[1] = (*tokenLP)[4 + sspPos[0]];
                            if (sspPos[1]!=0) { descNames[2]=(*tokenLP)[6 + sspPos[0]]; } // only if subsp. exists

                            descNames[3] = (*tokenLP)[0];
                            descNames[4] = (*tokenLP)[1];
                            if (sspPos[0]!=0) { descNames[5]=(*tokenLP)[sspPos[0]+1]; } // only if subsp. exists

                            actType = RENSPEC;

                        }
                        else { // species remaining cases
#if defined(DUMP)
                            std::cout << "not a valid description line detected" << std::endl;
                            std::cout << "isValid: " << isValid << std::endl;
                            std::cout << "isRenamed: " << isRenamed << std::endl;
                            std::cout << "isHetero: " << isHetero << std::endl;
                            std::cout << "isHomo: " << isHomo << std::endl;
                            std::cout << "isGenus: " << isGenus << std::endl;
                            std::cout << "isSee: " << isSee << std::endl;
                            std::cout << "isCorr: " << isCorr << std::endl;
                            std::cout << "sspPos: " << sspPos[0] << " and " << sspPos[1] << std::endl;
                            std::cout << descriptionString << std::endl;
#endif // DUMP
                            actType = NOTYPE;
                        }

                    }
                }
            }
        }


#if defined(DUMP)
        std::cout << descriptionString << std::endl;
        std::cout << "classified as " << actType << std::endl;
#endif // DUMP

        Desco actDesc(actType, isCorr, descNames[0], descNames[1], descNames[2], descNames[3], descNames[4], descNames[5]);
        delete tokenLP;
        return actDesc;
    }


    string Desco::getFirstName() {
        string tmp = firstgen;
        if (!firstspec.empty()) {
            tmp = tmp + " " + firstspec;
            if (!firstsub.empty()) {
                tmp = tmp + " " + "subsp." + " " + firstsub;
            }
        }


        return tmp;
    }

    string Desco::getSecondName() {
        string tmp = secondgen;
        if (!secondspec.empty()) {
            tmp = tmp + " " + firstspec;
            if (!secondsub.empty()) {
                tmp = tmp + " " + "subsp." + " " + secondsub;
            }
        }
        return tmp;
    }


    bool isUpperCase(const string& input)
    {
        for (size_t i=0; i<input.length(); ++i)
        {
            if (input[i]<'A' || input[i]>'Z') { return false; }
        }
        return true;
    }
}
