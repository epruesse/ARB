/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#ifndef GAGENOMGENEEMBL_H
#define GAGENOMGENEEMBL_H

#include "GAGenomGene.h"
#include "GAGenomGeneLocationEmbl.h"

namespace gellisary {

class GAGenomGeneEmbl : public GAGenomGene{
private:
	GAGenomGeneLocationEmbl location;
	
public:

	GAGenomGeneEmbl(){}
	virtual ~GAGenomGeneEmbl(){}
	virtual void parse();
	GAGenomGeneLocationEmbl * getLocation();
};

};

#endif // GAGENOMGENEEMBL_H
