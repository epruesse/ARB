#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include "GenomReferenceEmbl.h"
#include "GenomFeatureTableEmbl.h"

class GenomEmbl
{
	private:
		std::string identification;										//	ID
		std::string accession_number;									//	AC
		std::string sequence_version;									//	SV
		std::string date_of_creation;									//	DT
		std::string date_of_last_update;								//	DT
		std::string description;												//	DE
		std::vector<std::string> key_words;									//	KW
    std::string key_words_to_string;
		std::string organism_species;									//	OS
		std::vector<std::string> organism_classification;		//	OC
    std::string organism_classification_to_string;
		std::string or_garnelle;												//	OG
		std::vector<GenomReferenceEmbl> references;		//	R...
		std::vector<std::string> database_cross_reference;	//	DR
    std::string database_cross_reference_to_string;
		std::string constructed;												//	CO
		std::vector<std::string> free_text_comments;				//	CC
    std::string free_text_comments_to_string;
		std::vector<long> sequence_header;							//	SQ
		std::vector<std::string> sequence_data_line;				//	...
		GenomFeatureTableEmbl feature_table;			//	FT

		std::string embl_flatfile;
		int actual_sq;
		int actual_dr;
		int actual_rr;
		int actual_cc;
		int actual_sdl;
		int actual_kw;
		int actual_oc;
		bool prepared;
	public:
		GenomEmbl(std::string);
		~GenomEmbl();
		std::string* getID(void);
		std::string* getAC(void);
		std::string* getSV(void);
		std::string* getDTCreation(void);
		std::string* getDTLastUpdate(void);
		std::string* getDE(void);
		std::string* getKW(void);
		std::string* getOS(void);
		std::string* getOC(void);
		std::string* getOG(void);
		GenomReferenceEmbl* getReference(void);
		std::string* getDR(void);
		std::string* getCO(void);
		std::string* getCC(void);
		long getSQ(void);
		std::string* get(void);
//		string* getFT(void);
		GenomFeatureTableEmbl* getFeatureTable(void);

		int prepareFlatFile(void);	//	initiate the parse process
};

