/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#ifndef GAGENOMREFERENCEDDBJ_H
#define GAGENOMREFERENCEDDBJ_H

#ifndef GAGENOMREFERENCE_H
#include "GAGenomReference.h"
#endif

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

namespace gellisary{

class GAGenomReferenceDDBJ : public GAGenomReference{
private:
	std::vector<int> reference_medline;	// embl:RX	genbank,ddgj:MEDLINE,PUBMED
	std::vector<int> reference_pubmed;	// embl:RX	genbank,ddgj:MEDLINE,PUBMED
	std::string reference_journal;		// embl:RL	genbank,ddgj:JOURNAL
	std::vector<std::string> reference_cross_reference;	// embl:RX	genbank,ddgj:MEDLINE,PUBMED
	std::string reference_cross_reference_as_string;

public:

//	GAGenomReferenceDDBJ();
	virtual ~GAGenomReferenceDDBJ(){}
	void parse();
	std::vector<int> * getMedLines();
	std::vector<int> * getPubMeds();
	std::string * getJournal();
	std::vector<std::string> * getCrossReference();
	std::string * getCrossReferenceAsString();
};

};

#endif // GAGENOMREFERENCEDDBJ_H
