#ifndef GAGENOMGENELOCATIONDDBJ_H
#define GAGENOMGENELOCATIONDDBJ_H

#include "GAGenomGeneLocation.h"

namespace gellisary {

class GAGenomGeneLocationDDBJ : public gellisary::GAGenomGeneLocation{
private:
	std::string reference;
	bool pointer;
	std::vector<GAGenomGeneLocationDDBJ> locations;
	int current_value;
	std::vector<std::string> getParts(std::string *, int *);
	
public:

	GAGenomGeneLocationDDBJ();
	virtual ~GAGenomGeneLocationDDBJ(){}
	virtual void parse(std::string *);
	virtual void parse();
	bool isReference();
	void setReference(std::string *);
	std::string * getReference();
	bool hasMoreValues();
	GAGenomGeneLocationDDBJ * getNextValue();
	void setValue(GAGenomGeneLocationDDBJ *);
};

};

#endif // GAGENOMGENELOCATIONDDBJ_H
