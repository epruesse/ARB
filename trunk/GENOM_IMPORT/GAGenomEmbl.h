#ifndef GAGENOMEMBL_H
#define GAGENOMEMBL_H

#include "GAGenom.h"
#include "GAGenomReferenceEmbl.h"
#include "GAGenomFeatureTableEmbl.h"

namespace gellisary {

class GAGenomEmbl : public GAGenom{
private:
	std::string date_of_creation;						// embl:DT	genbank,ddgj:?
	std::string date_of_last_update;					// embl:DT	genbank,ddgj:?
//	std::vector<std::string> organism_classification;	// embl:OC	genbank,ddgj:..
//	std::string organism_classification_as_one_string;
	std::string organelle;								// embl:OG	genbank,ddgj:?
	std::vector<std::string> database_cross_reference;	// embl:DR	genbank,ddgj:?
	std::string database_cross_reference_as_one_string;
	std::vector<int> sequence_header;					// embl:SQ	genbank,ddgj:BASE COUNT
	std::vector<gellisary::GAGenomReferenceEmbl> references;
	gellisary::GAGenomFeatureTableEmbl feature_table;
	int iter;
	GAGenomReferenceEmbl tmp_ref;
	
public:

	GAGenomEmbl(std::string *);
	virtual ~GAGenomEmbl(){}
	virtual void parseFlatFile();
	std::string * getDateOfCreation();
	std::string * getDateOfLastUpdate();
	std::string * getOrganelle();
	std::vector<std::string> * getDataCrossReference();
//	std::string * getOrganismClassificationAsOneString();
//	std::vector<std::string> * getOrganismClassification();
	std::string * getDataCrossReferenceAsOneString();
	std::vector<int> * getSequenceHeader();
	gellisary::GAGenomFeatureTableEmbl * getFeatureTable();
	gellisary::GAGenomReferenceEmbl * getReference();
	virtual void parseSequence(std::string *);
};

};

#endif // GAGENOMEMBL_H
