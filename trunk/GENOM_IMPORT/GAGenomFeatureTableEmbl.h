#ifndef GAGENOMFEATURETABLEEMBL_H
#define GAGENOMFEATURETABLEEMBL_H

#include "GAGenomFeatureTable.h"
#include "GAGenomFeatureTableSourceEmbl.h"
#include "GAGenomGeneEmbl.h"

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _GLIBCXX_MAP
#include <map>
#endif

namespace gellisary {

class GAGenomFeatureTableEmbl : public GAGenomFeatureTable{
private:
	GAGenomFeatureTableSourceEmbl source;
	std::map<std::string,gellisary::GAGenomGeneEmbl> genes;
	std::vector<std::string> features;
	std::vector<int> number_of_features;
	std::map<std::string,gellisary::GAGenomGeneEmbl>::iterator iter;
	
	int nameToNumberOfFeature(std::string *);
	
public:

	GAGenomFeatureTableEmbl();
	virtual ~GAGenomFeatureTableEmbl(){}
	virtual void parse();
	GAGenomFeatureTableSourceEmbl * getFeatureTableSource();
	std::string * getGeneName();
	GAGenomGeneEmbl * getGeneByName(std::string *);
};

};

#endif // GAGENOMFEATURETABLEEMBL_H
