#ifndef GENOMUTILITIES_H
#define GENOMUTILITIES_H



#include <vector>
#include <string>


// #include <iostream>
//#include <GenomLocation.h>


void eliminateFeatureTableSign(std::string * source_string, std::string * fts, bool semi = true, bool equal = true);
std::string parseSequenceDataLine(std::string&);
bool isNewGene(std::string&);
bool isSource(std::string&);
std::vector<long> parseSourceLocation(std::string&);
std::string toOneString(std::vector<std::string>&, bool = true);
void trimByDoubleQuote(std::string&);
void onlyOneSpace(std::string&);
//GenomLocation splitLocation(string&);
std::string generateID(std::string,int);
std::string trimString(std::string);
std::vector<std::string> findAndSeparateWordsBy(std::string*,char,bool);
std::string intToString(int);

#else
#error GenomUtilities.h included twice
#endif
