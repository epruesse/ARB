#ifndef GAGENOMFEATURETABLESOURCEDDBJ_H
#define GAGENOMFEATURETABLESOURCEDDBJ_H

#include "GAGenomFeatureTableSource.h"

namespace gellisary {

class GAGenomFeatureTableSourceDDBJ : public GAGenomFeatureTableSource{
public:

//	GAGenomFeatureTableSourceDDBJ();
	virtual ~GAGenomFeatureTableSourceDDBJ(){}
	virtual void parse();
};

};

#endif // GAGENOMFEATURETABLESOURCEDDBJ_H
