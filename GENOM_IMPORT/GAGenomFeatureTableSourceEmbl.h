/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#ifndef GAGENOMFEATURETABLESOURCEEMBL_H
#define GAGENOMFEATURETABLESOURCEEMBL_H

#include "GAGenomFeatureTableSource.h"

namespace gellisary {

class GAGenomFeatureTableSourceEmbl : public GAGenomFeatureTableSource{
public:
//	GAGenomFeatureTableSourceEmbl();
	virtual ~GAGenomFeatureTableSourceEmbl(){}
	virtual void parse();
};

};

#endif // GAGENOMFEATURETABLESOURCEEMBL_H
