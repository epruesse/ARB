#ifndef GENOMUTILITIES_H
#define GENOMUTILITIES_H

#ifndef VECTOR
#include <vector>
#endif
#ifndef STRING
#include <string>
#endif

// #include <iostream>
//#include <GenomLocation.h>


void eliminateFeatureTableSign(string * source_string, string * fts, bool semi = true, bool equal = true);
string parseSequenceDataLine(string&);
bool isNewGene(string&);
bool isSource(string&);
vector<long> parseSourceLocation(string&);
string toOneString(vector<string>&, bool = true);
void trimByDoubleQuote(string&);
void onlyOneSpace(string&);
//GenomLocation splitLocation(string&);
string generateID(string,int);
string trimString(string);
vector<string> findAndSeparateWordsBy(string*,char,bool);
string intToString(int);

#else
#error GenomUtilities.h included twice
#endif
