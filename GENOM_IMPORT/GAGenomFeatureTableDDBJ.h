/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#ifndef GAGENOMFEATURETABLEDDBJ_H
#define GAGENOMFEATURETABLEDDBJ_H

#include "GAGenomFeatureTable.h"
#include "GAGenomFeatureTableSourceDDBJ.h"
#include "GAGenomGeneDDBJ.h"

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

class GAGenomFeatureTableDDBJ : public GAGenomFeatureTable{
private:
	GAGenomFeatureTableSourceDDBJ source;
	std::map<std::string,GAGenomGeneDDBJ> genes;
	std::vector<std::string> features;
	std::vector<int> number_of_features;
	std::map<std::string,GAGenomGeneDDBJ>::iterator iter;
	
	int nameToNumberOfFeature(std::string *);

public:

	GAGenomFeatureTableDDBJ();
	virtual ~GAGenomFeatureTableDDBJ(){}
	virtual void parse();
	GAGenomFeatureTableSourceDDBJ * getFeatureTableSource();
	std::string * getGeneName();
	GAGenomGeneDDBJ * getGeneByName(std::string *);
	void setIterator();
};

};

#endif // GAGENOMFEATURETABLEDDBJ_H
