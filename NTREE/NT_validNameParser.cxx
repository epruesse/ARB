
/**
 * Definition of all objects belonging to this version of
 * the valid names text file
 *
 * 29. November 2002
 * 
 * copyright by Lothar Richter
 */

#include <stdlib.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include "nt_fileReader.hxx"
#include "nt_validNameParser.hxx"

namespace validNames{


  TokLPtr tokenize(const string& description, TokLPtr tokenLP)
  {
    //   TokLPtr tokenLP = new TokL;
    int tokenEnd = 0;
    int tokenBegin = 0;
    //    std::cout  <<description.size() << std::endl;
    while (tokenEnd != description.size()){
    tokenEnd = description.find_first_of(' ', tokenBegin);
    //    std::cout << string("TokenBegin:\t") << tokenBegin << "\t" <<
    //  string("TokenEnd:\t") << tokenEnd << std::endl;
    if (tokenEnd == string::npos) {tokenEnd = description.size();}
    int tokLength = tokenEnd - tokenBegin;
    if (tokLength != 0){
    tokenLP->push_back(description.substr(tokenBegin,tokenEnd - tokenBegin));
    //    std::cout << string("pushedToken:\t") << description.substr(tokenBegin,tokenEnd - tokenBegin) << std::endl;
    }
    //    std::cout << description.substr(tokenBegin,tokenEnd - tokenBegin) << std::endl;
    tokenBegin = tokenEnd + 1;

    }
    return tokenLP;
  } // end tokenize





  Desco determineType(const string& descriptionString)
  {// begin determineType
 
  DESCT actType = NOTYPE;
  TokLPtr tokenLP =  new TokL;
  tokenLP = tokenize(descriptionString, tokenLP);
  // remove all tokens in parentheses
  TokL::iterator it = tokenLP->begin();
  while( it != tokenLP->end()){
    if(((*it).at(0) == '(') && *it != string("(corrig.)")){it = tokenLP->erase(it);}
    else{it++;}
  }

  // check first word for upper case letters
  string descNames[6]; // first the valid genus, species, subsp. then the other names
  // stores occurrence of subsp. which is needed to retrieve the right tokens later on and status flags
  int sspPos[2] = {0, 0}; // token subsp. occurs maximum twice
  int ssp = 0;
  bool isValid = true;
  bool isRenamed = false;
  bool isHetero = false; 
  bool isHomo = false;
  bool isGenus = false;
  bool isSee = false;
  //  bool isSpecies = false;
  bool isCorr = false;    



  for( TokL::iterator it = tokenLP->begin();it != tokenLP->end(); it ++, ssp++)
    {//begin of token iteration
      //      std::cout << "entering analysis loop" << std::endl;
      if(isUpperCase(*it)) { isGenus = true;      std::cout << "genus detected" << std::endl;} // for genus descriptions


      else{ // begin operators
	if(*it == string("->")){
	  assert(isHetero == false);
	  assert(isHomo == false);
	  assert(isValid == true); // only one operator per line allowed
	  isRenamed = true;
	  isValid = false;
	  std::cout << "renaming detected" << std::endl;}
	else{
	  if(*it == string("=>")){
	    assert(isRenamed == false);
	    assert(isHomo == false);
	    assert(isValid == true);
	    isHetero = true;
	    isValid = false;
	    std::cout << "heteronym detected" << std::endl;}
	  else{
	    if(*it == string("=")){
	      assert(isRenamed == false);
	      assert(isHetero == false);
	      assert(isValid == true);
	      isHomo = true;
	      isValid = false;
	      std::cout << "homonym detected" << std::endl;}
	    else{
	      if(*it == string("(corrig.)")){
		isCorr = true;
		std::cout << "correction" << std::endl;}
	      else{
		if(*it == string("see:")){
		  isSee = true;
		  isValid = false;
		  std::cout << "reference" << std::endl;}
		else{
		  if(*it == string("subsp.")){
		    std::cout << "subspecies detected at position: >>>"<< ssp << "<<<" << std::endl;
		    ssp == 2 ? sspPos[0] = ssp : sspPos[1] = ssp;
		    //      : sspPos[1] == 0 ? sspPos[1] = ssp 
		    //      : 10000,exit (102); // bad hack to fake return value
		    // max. one subsp. on each operator side
		    std::cout << "position of subsp.: " << sspPos[0] << "\tand: " << sspPos[1] << std::endl;
		  }
		}
	      }
	    }
	  }
	}
      } // end operators
    } // end of token iteration



    if(isGenus) { // for genus descriptions
      std::cout << " GENUS description found " << std::endl; 
      if(isValid){
	descNames[0] = (*tokenLP)[0];
	actType = VALGEN;
	std::cout << "VALIDGEN type set to: " << actType << std::endl;}
      else{
	if(isHetero){
	  descNames[0] = (*tokenLP)[2];
	  descNames[3] = (*tokenLP)[0];
	  actType = HETGEN;	
	  std::cout << "HETERONYMGEN type set to: " << actType << std::endl;
	}else{
	  if(isHomo){
	    descNames[0] = (*tokenLP)[2];
	    descNames[3] = (*tokenLP)[0];
	    actType = HOMGEN;
	    std::cout << "HOMONYMGEN type set to: " << actType << std::endl;

	  }else{

	    if(isRenamed){
	      descNames[0] = (*tokenLP)[2];
	      descNames[3] = (*tokenLP)[0];
	      actType = RENGEN;
	      std::cout << "RENAMEDGEN type set to: " << actType << std::endl;
	    }else{
	      std::cout << "no meaningful combination of conditions reached" << std::endl
			<< "for line: " << descriptionString << std::endl;
	      std::cout << "description type is set to NOTYPE: " << NOTYPE << std::endl;
	      isValid = false;
	      std::cout << "isValid set to false "  << std::endl;
	      actType = NOTYPE;
	      //    exit(25);
	    }
	  }
	}
      }

    } // end of genus part
    else{ // begin of  species part

      //       just fancy experimental , maybe not 100% correct but looks good
      if (!(((sspPos[0] == 0) || (sspPos[0] == 2)) && (((sspPos[1] > 4)&&(sspPos[1]< 9))||(sspPos[1]==0))))
	{
	  std::cout << "subsp. at strange position found in line:" << std::endl<< descriptionString << endl;
	  std::cout << "description type is set to NOTYPE: " << NOTYPE << std::endl;
	  isValid = false;
	  std::cout << "isValid set to false "  << std::endl;
	  actType = NOTYPE;
	}

      if(isValid){
	descNames[0] = (*tokenLP)[0];
	descNames[1] = (*tokenLP)[1];
	if(sspPos[0] != 0) {descNames[2] = (*tokenLP)[sspPos[0]+1];} // only if subsp. exists
	actType = VALSPEC;
      }
      else{// begin else isHetero
	if(isHetero){
	  descNames[0] = (*tokenLP)[3 + sspPos[0]];
	  descNames[1] = (*tokenLP)[4 + sspPos[0]];
	  if(sspPos[1]!=0){descNames[2]=(*tokenLP)[6 + sspPos[0]];} // only if subsp. exists

	  descNames[3] = (*tokenLP)[0];
	  descNames[4] = (*tokenLP)[1];
	  if(sspPos[0]!=0){descNames[5]=(*tokenLP)[sspPos[0]+1];} // only if subsp. exists

	  actType = HETSPEC;
	}
	else{
	  if(isHomo){
	    descNames[0] = (*tokenLP)[3 + sspPos[0]];
	    descNames[1] = (*tokenLP)[4 + sspPos[0]];
	    if(sspPos[1]!=0){descNames[2]=(*tokenLP)[6 + sspPos[0]];} // only if subsp. exists
	    
	    descNames[3] = (*tokenLP)[0];
	    descNames[4] = (*tokenLP)[1];
	    if(sspPos[0]!=0){descNames[5]=(*tokenLP)[sspPos[0]+1];} // only if subsp. exists
	    
	    actType = HOMSPEC;

	  }
	  else{// else branch isHomo
	    if(isRenamed){
	      descNames[0] = (*tokenLP)[3 + sspPos[0]];
	      descNames[1] = (*tokenLP)[4 + sspPos[0]];
	      if(sspPos[1]!=0){descNames[2]=(*tokenLP)[6 + sspPos[0]];} // only if subsp. exists
	      
	      descNames[3] = (*tokenLP)[0];
	      descNames[4] = (*tokenLP)[1];
	      if(sspPos[0]!=0){descNames[5]=(*tokenLP)[sspPos[0]+1];} // only if subsp. exists
	      
	      actType = RENSPEC;
	      
	    }
	    else{// species remaining cases
	      std::cout << "not a valid description line detected" << std::endl;
	      std::cout << "isValid: " << isValid << std::endl;
	      std::cout << "isRenamed: " << isRenamed << std::endl;
	      std::cout << "isHetero: " << isHetero << std::endl;
	      std::cout << "isHomo: " << isHomo << std::endl;
	      std::cout << "isGenus: " << isGenus << std::endl;
	      std::cout << "isSee: " << isSee << std::endl;
	      std::cout << "isCorr: " << isCorr << std::endl;
 	      std::cout << "sspPos: " << sspPos[0]<< " and " << sspPos[1] << std::endl;
	      std::cout << descriptionString << std::endl;
	      actType = NOTYPE;
	    }
	    
	  }// end else isHomo
	}// end else isHetero
      }//end else is Valid
    }// end of species part


  // check line for -> or => or = 
  //}
  std::cout << descriptionString << std::endl;
  std::cout << "classified as " << actType << std::endl ;
  //Desco actDesc(actType);
    Desco actDesc(actType, isCorr, descNames[0],descNames[1],descNames[2],descNames[3],descNames[4],descNames[5] );
  delete tokenLP;
  return actDesc;
  //  return actType;
}; // end determineType


string Desco::getFirstName(){
  //  return nameParts[0] + nameParts[1] + nameParts[2];
  string tmp = firstgen;
  if (!firstspec.empty()){
    tmp = tmp + " " + firstspec;
    if(!firstsub.empty()){
      tmp = tmp + " " + "subsp." + " " + firstsub;
    }
  }
  

  return tmp;
} // end Desco::getFirstName()

string Desco::getSecondName(){
   string tmp = secondgen;
   if (!secondspec.empty()){
     tmp = tmp + " " + firstspec;
     if(!secondsub.empty()){
       tmp = tmp + " " + "subsp." + " " + secondsub;
     }
   }
   return tmp;
   //  return nameParts[3] + nameParts[4] + nameParts[5];
} // Desco::getSecondName()


bool isUpperCase(const string& input)
{
  for (int i=0;i<input.length();i++)
    {
      if(input[i]<'A' || input[i]>'Z'){return false;}
    }
  return true;
}


}  // end namespace
// test
