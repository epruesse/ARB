#ifndef GAGENOM_H
#define GAGENOM_H

#ifndef _CPP_STRING
#include <string>
#endif

#ifndef _CPP_VECTOR
#include <vector>
#endif

namespace gellisary{
/**
 * TO DO : 
 * 	- Ergaenzen den Eintrag ueber 'SOURCE ORGANISM'.
 * 	- References in gellisary::GAGenomReference Klasse.
 * 	- FeatureTable in gellisary::GAGenomFeatureTable Klasse.
 */
	
/*
 * Bemerkung zum Kommentar:
 * ? - keine Entsprechungen gefunden alias nicht genuegend Informationen
 * .. - Platzhalter fuer 'leer'
 * genbank und ddgj 'SOURCE ORGANISM': SOURCE wird in einer Zeile geschrieben, ORGANISM in der 
 * 	naechsten mit Vorrueckung = 2.
 * 	.SOURCE      Escherichia coli O157:H7
 * 	.  ORGANISM  Escherichia coli O157:H7
 * 	In allen gesehenen FlatFiles waren die Bezeichnungen redundant.
 * 	Gleich nach 'SOURCE ORGANISM' kommen meist mehrere Zeilen ohne Bezeichner,
 * 	sie entsprechen dem 'OC' aus embl.
 * 	.            Bacteria; Proteobacteria; gamma subdivision; Enterobacteriaceae;
 * 	.            Escherichia.
 * Sequence:
 * 	embl:		.SQ   Sequence 5498450 BP; 1361495 A; 1386237 C; 1392518 G; 1358200 T; 0 other;
 * 				.     agcttttcat tctgactgca acgggcaata tgtctctgtg tggattaaaa aaagagtctc        60
 * 				.     tgacagcagc ttctgaactg gttacctgcc gtgagtaaat taaaatttta ttgacttagg       120
 * 	genbank:	.BASE COUNT  1361495 a1386237 c1392518 g1358200 t
 * 				.ORIGIN
 * 				.        1 agcttttcat tctgactgca acgggcaata tgtctctgtg tggattaaaa aaagagtctc
 * 				.       61 tgacagcagc ttctgaactg gttacctgcc gtgagtaaat taaaatttta ttgacttagg
 * 	ddgj:		.BASE COUNT        67567 a        69654 c        75778 g        68531 t
 * 				.ORIGIN
 * 				.        1 agcttttcat tctgactgca acgggcaata tgtctctgtg tggattaaaa aaagagtctc
 * 				.       61 tgacagcagc ttctgaactg gttacctgcc gtgagtaaat taaaatttta ttgacttagg
 * 	Allerdings zu Bemerken, dass erstens 'BASE COUNT' nicht zwingend ist und zweitens sind die
 * 	Abstaende in 'BASE COUNT'-Zeile zwischen Zahlen und Buchstaben variabel.
 */
class GAGenom{
protected:
	std::string identification;							// embl:ID	genbank,ddgj:LOCUS
	std::string accession_number;						// embl:AC	genbank,ddgj:ACCESSION
	std::string sequence_version;						// embl:SV	genbank,ddgj:VERSION
//	std::string date_of_creation;						// embl:DT	genbank,ddgj:?
//	std::string date_of_last_update;					// embl:DT	genbank,ddgj:?
	std::string description;							// embl:DE	genbank,ddgj:DEFINITION
	std::vector<std::string> key_words;					// embl:KW	genbank,ddgj:KEYWORDS
	std::string key_words_as_string;
	std::string organism_species;						// embl:OS	genbank,ddgj:SOURCE ORGANISM
	std::vector<std::string> organism_classification;	// embl:OC	genbank,ddgj:..
	std::string organism_classification_as_one_string;
//	std::string organelle;								// embl:OG	genbank,ddgj:?
//	std::vector<std::string> database_cross_reference;	// embl:DR	genbank,ddgj:?
	std::string contig;									// embl:CO	genbank:?	ddgj:CONTIG
	std::vector<std::string> free_text_comment;			// embl:CC	genbank,ddgj:COMMENT
	std::string free_text_commnt_as_one_string;
//	std::vector<int> sequence_header;					// embl:SQ	genbank,ddgj:BASE COUNT
	std::string sequence;								// embl:..	genbank,ddgj:ORIGIN
	std::string file_name;
	bool prepared;

public:

	GAGenom(std::string *);
	virtual ~GAGenom(){}
	virtual void parseFlatFile()=0;
	std::string * getIdentification();
	std::string * getAccessionNumber();
	std::string * getSequenceVersion();
	std::string * getDescription();
	std::string * getOrganism();
	std::string * getContig();
	std::string * getSequence();
	std::string * getKeyWordsAsString();
	std::string * getCommentAsOneString();
	std::vector<std::string> * getKeyWords();
	std::vector<std::string> * getComment();
	std::string * getOrganismClassificationAsOneString();
	std::vector<std::string> * getOrganismClassification();
	virtual void parseSequence(std::string *)=0;
};

};

#endif // GAGENOM_H
