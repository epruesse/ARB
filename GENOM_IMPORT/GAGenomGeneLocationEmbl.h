#ifndef GAGENOMGENELOCATIONEMBL_H
#define GAGENOMGENELOCATIONEMBL_H

#include "GAGenomGeneLocation.h"

namespace gellisary {

class GAGenomGeneLocationEmbl : public GAGenomGeneLocation{
private:
	std::string reference;
	bool pointer;
	std::vector<GAGenomGeneLocationEmbl> locations;
	int current_value;
	std::vector<std::string> getParts(std::string *, int *);

public:

	GAGenomGeneLocationEmbl();
	virtual ~GAGenomGeneLocationEmbl(){}
	virtual void parse(std::string *);
	virtual void parse();
	bool isReference();
	void setReference(std::string *);
	std::string * getReference();
	bool hasMoreValues();
	GAGenomGeneLocationEmbl * getNextValue();
	void setValue(GAGenomGeneLocationEmbl *);
};

};

#endif // GAGENOMGENELOCATIONEMBL_H
