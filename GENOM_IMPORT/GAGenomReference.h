#ifndef GAGENOMREFERENCE_H
#define GAGENOMREFERENCE_H

#include "GAParser.h"

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

namespace gellisary{
/*
 * Bemerkung zum Kommentar:
 * ? - keine Entsprechungen gefunden
 * .. - Platzhalter fuer 'leer'
 * In embl:
 * 	.RX   MEDLINE; 21156231.
 * 	.RX   PUBMED; 11258796.
 * In genbank und ddgj:
 * 	.REFERENCE   1  (bases 1 to 5498450)
 * 	.	...	// ein paar Zeilen dazwischen
 * 	.  MEDLINE   20198780
 * 	.  PUBMED    11258796
 * 	Wie man sieht steht embl: RN in genbank und in ddgj dierekt nach dem einfuehrenden
 * 	Schluesselwort 'REFERENCE'
 * 	Also:
 * 	embl:		.RN   [1]
 * 				.RP   1-5498450
 * 	genbank:	.REFERENCE   1  (bases 1 to 5498450)
 * 	ddgj:		.REFERENCE   1  (bases 1 to 5498450)
 * 
 * Bemerkung:
 * 	- in genbank fand ich 'CONSRTM'
 * 		.  CONSRTM   NCBI Microbial Genomes Annotation Project
 */
class GAGenomReference : public GAParser{
protected:
	int reference_number;								// embl:RN	genbank,ddgj:..
	std::string reference_comment;						// embl:RC	genbank,ddgj:?
	std::vector<int> reference_position_begin;			// embl:RP	genbank,ddgj:..
	std::vector<int> reference_position_end;			// embl:RP	genbank,ddgj:..
	std::vector<std::string> reference_authors;			// embl:RA	genbank,ddgj:AUTHORS
	std::string reference_authors_as_string;
	std::string reference_title;						// embl:RT	genbank,ddgj:TITLE
	std::string reference_location;						// embl:RL	genbank,ddgj:JOURNAL

public:

	GAGenomReference();
	virtual ~GAGenomReference(){}
	virtual void parse() = 0;
	int getNumber();
	std::string * getComment();
	std::string * getTitle();
	std::vector<int> * getPositionBegin();
	std::vector<int> * getPositionEnd();
	std::vector<std::string> * getAuthors();
	std::string * getAuthorsAsString();
	std::string * getLocation();
};

};

#endif // GAGENOMREFERENCE_H
