/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#ifndef GAGENOMDDBJ_H
#define GAGENOMDDBJ_H

#include "GAGenom.h"
#include "GAGenomReferenceDDBJ.h"
#include "GAGenomFeatureTableDDBJ.h"

namespace gellisary{

class GAGenomDDBJ : public GAGenom{
private:
	std::vector<int> sequence_header;					// embl:SQ	DDBJ,ddgj:BASE COUNT
	std::vector<gellisary::GAGenomReferenceDDBJ> references;
	gellisary::GAGenomFeatureTableDDBJ feature_table;
	int iter;
	GAGenomReferenceDDBJ tmp_ref;

public:

	GAGenomDDBJ(std::string *);
	virtual ~GAGenomDDBJ(){}
	virtual void parseFlatFile();
	std::vector<int> * getSequenceHeader();
	gellisary::GAGenomFeatureTableDDBJ * getFeatureTable();
	gellisary::GAGenomReferenceDDBJ * getReference();
	virtual void parseSequence(std::string *);
};

};

#endif // GAGENOMDDBJ_H
