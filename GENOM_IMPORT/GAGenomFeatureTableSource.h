#ifndef GAGENOMFEATURETABLESOURCE_H
#define GAGENOMFEATURETABLESOURCE_H

#include "GAParser.h"

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _GLIBCXX_MAP
#include <map>
#endif

namespace gellisary {
/*
 * Bemerkung:
 * 	Schau dir die Implementation der Methode getQualifierName() an und korregiere es!!!
 */
class GAGenomFeatureTableSource : public GAParser{
protected:
	std::map<std::string,std::string> qualifiers;
	int source_begin;
	int source_end;
	std::map<std::string,std::string>::iterator iter;
	std::string tmp_key;
	
public:

//	GAGenomFeatureTableSource();
	virtual ~GAGenomFeatureTableSource(){}
	virtual void parse() = 0;
	int getBegin();
	int getEnd();
	std::string * getNameOfQualifier();
	std::string * getValueOfQualifier(std::string *);
};

};

#endif // GAGENOMFEATURETABLESOURCE_H
