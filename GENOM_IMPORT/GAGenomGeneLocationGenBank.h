#ifndef GAGENOMGENELOCATIONGENBANK_H
#define GAGENOMGENELOCATIONGENBANK_H

#include "GAGenomGeneLocation.h"

namespace gellisary{

class GAGenomGeneLocationGenBank : public GAGenomGeneLocation{
private:
	std::string reference;
	bool pointer;
	std::vector<GAGenomGeneLocationGenBank> locations;
	int current_value;
	std::vector<std::string> getParts(std::string *, int *);
	
public:

	GAGenomGeneLocationGenBank();
	virtual ~GAGenomGeneLocationGenBank(){}
	virtual void parse(std::string *);
	virtual void parse();
	bool isReference();
	void setReference(std::string *);
	std::string * getReference();
	bool hasMoreValues();
	GAGenomGeneLocationGenBank * getNextValue();
	void setValue(GAGenomGeneLocationGenBank *);
};

};

#endif // GAGENOMGENELOCATIONGENBANK_H
