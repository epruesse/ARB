#ifndef GAGENOMFEATURETABLEGENBANK_H
#define GAGENOMFEATURETABLEGENBANK_H

#include "GAGenomFeatureTable.h"
#include "GAGenomFeatureTableSourceGenBank.h"
#include "GAGenomGeneGenBank.h"

#ifndef _CPP_STRING
#include <string>
#endif

#ifndef _CPP_VECTOR
#include <vector>
#endif

#ifndef _CPP_MAP
#include <map>
#endif

namespace gellisary{

class GAGenomFeatureTableGenBank : public GAGenomFeatureTable{
private:
	GAGenomFeatureTableSourceGenBank source;
	std::map<std::string,GAGenomGeneGenBank> genes;
	std::vector<std::string> features;
	std::vector<int> number_of_features;
	std::map<std::string,GAGenomGeneGenBank>::iterator iter;
	
	int nameToNumberOfFeature(std::string *);

public:

	GAGenomFeatureTableGenBank();
	virtual ~GAGenomFeatureTableGenBank(){}
	virtual void parse();
	GAGenomFeatureTableSourceGenBank * getFeatureTableSource();
	std::string * getGeneName();
	GAGenomGeneGenBank * getGeneByName(std::string *);
};

};

#endif // GAGENOMFEATURETABLEGENBANK_H
