#ifndef GAGENOMDDBJ_H
#define GAGENOMDDBJ_H

#include "GAGenom.h"
#include "GAGenomReferenceDDBJ.h"
#include "GAGenomFeatureTableDDBJ.h"

namespace gellisary {

class GAGenomDDBJ : public GAGenom{
private:
//	std::vector<std::string> organism_classification;	// embl:OC	genbank,ddgj:..
//	std::string organism_classification_as_one_string;
//	std::string organelle;								// embl:OG	genbank,ddgj:?
//	std::vector<std::string> database_cross_reference;	// embl:DR	genbank,ddgj:?
//	std::string database_cross_reference_as_one_string;
	std::vector<int> sequence_header;					// embl:SQ	genbank,ddgj:BASE COUNT
	std::vector<gellisary::GAGenomReferenceDDBJ> references;
	gellisary::GAGenomFeatureTableDDBJ feature_table;
	int iter;
	GAGenomReferenceDDBJ tmp_ref;

public:

	GAGenomDDBJ(std::string *);
	virtual ~GAGenomDDBJ(){}
	virtual void parseFlatFile();
//	std::string * getDateOfCreation();
//	std::string * getDateOfLastUpdate();
//	std::string * getOrganelle();
//	std::vector<std::string> * getDataCrossReference();
//	std::string * getOrganismClassificationAsOneString();
//	std::vector<std::string> * getOrganismClassification();
//	std::string * getDataCrossReferenceAsOneString();
	std::vector<int> * getSequenceHeader();
	gellisary::GAGenomFeatureTableDDBJ * getFeatureTable();
	gellisary::GAGenomReferenceDDBJ * getReference();
	virtual void parseSequence(std::string *);
};

};

#endif // GAGENOMDDBJ_H
