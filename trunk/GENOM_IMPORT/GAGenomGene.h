#ifndef GAGENOMGENE_H
#define GAGENOMGENE_H

#include "GAParser.h"

#ifndef _CPP_STRING
#include <string>
#endif

#ifndef _CPP_MAP
#include <map>
#endif

#ifndef _CPP_VECTOR
#include <vector>
#endif

namespace gellisary {
/*
 * Bemerkung:
 * 	Schau dir die Implementation der Methode getQualifierName() an und korregiere es!!!
 */
class GAGenomGene : public GAParser{
protected:
	std::string gene_type;
	std::map<std::string,std::string> qualifiers;
	std::vector<std::string> names_of_qualifiers;
	int gene_type_number;
	int gene_number;
	std::map<std::string,std::string>::iterator iter;
	std::string location_as_string;
	std::string name_of_gene;
	std::string tmp_key;
	
public:
//	GAGenomGene();
	virtual ~GAGenomGene(){}
	virtual void parse() = 0;
	std::string * getGeneType();
	int getGeneTypeNumber();
	int getGeneNumber();
	std::string * getQualifierName();
	std::string * getQualifierValue(std::string *);
	void setGeneNumber(int);
	void setGeneTypeNumber(int);
	std::string * getLocationAsString();
	std::string * getNameOfGene();
	void setNameOfGene(std::string *);
};

};

#endif // GAGENOMGENE_H
