#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <GenomReferenceEmbl.h>
#include <GenomFeatureTableEmbl.h>

class GenomEmbl
{
	private:
		string identification;										//	ID
		string accession_number;									//	AC
		string sequence_version;									//	SV
		string date_of_creation;									//	DT
		string date_of_last_update;								//	DT
		string description;												//	DE
		vector<string> key_words;									//	KW
    string key_words_to_string;
		string organism_species;									//	OS
		vector<string> organism_classification;		//	OC
    string organism_classification_to_string;
		string or_garnelle;												//	OG
		vector<GenomReferenceEmbl> references;		//	R...
		vector<string> database_cross_reference;	//	DR
    string database_cross_reference_to_string;
		string constructed;												//	CO
		vector<string> free_text_comments;				//	CC
    string free_text_comments_to_string;
		vector<long> sequence_header;							//	SQ
		vector<string> sequence_data_line;				//	...
		GenomFeatureTableEmbl feature_table;			//	FT

		string embl_flatfile;
		int actual_sq;
		int actual_dr;
		int actual_rr;
		int actual_cc;
		int actual_sdl;
		int actual_kw;
		int actual_oc;
		bool prepared;
	public:
		GenomEmbl(string);
		~GenomEmbl();
		string* getID(void);
		string* getAC(void);
		string* getSV(void);
		string* getDTCreation(void);
		string* getDTLastUpdate(void);
		string* getDE(void);
		string* getKW(void);
		string* getOS(void);
		string* getOC(void);
		string* getOG(void);
		GenomReferenceEmbl* getReference(void);
		string* getDR(void);
		string* getCO(void);
		string* getCC(void);
		long getSQ(void);
		string* get(void);
//		string* getFT(void);
		GenomFeatureTableEmbl* getFeatureTable(void);

		int prepareFlatFile(void);	//	initiate the parse process
};

