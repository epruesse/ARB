#ifndef GENOMREFERENCEEMBL_H
#define GENOMREFERENCEEMBL_H

#include <string>
#include <cstdlib>
#include <vector>

class GenomReferenceEmbl
{
	private:
		int reference_number;						//	RN
		string reference_comment;					//	RC
		vector<long> reference_position;			//	RP	//	begin-end, begin-end, ...
		vector<string> reference_cross_reference;	//	RX
    string reference_cross_reference_to_string;
		string reference_group;						//	RG
		vector<string> reference_authors;			//	RA
    string reference_authors_to_string;
		string reference_title;						//	RT
		string reference_location;					//	RL

		int actual_RP;
		int actual_RX;
		int actual_RA;
		vector<string> row_lines;
		bool prepared;

	public:
		GenomReferenceEmbl();
		~GenomReferenceEmbl();

		void updateReference(string*);
		void prepareReference(void);	          //	parses reference

		int getRN(void);
		string* getRC(void);
		long getRPBegin(void);
		long getRPEnd(void);
		string* getRX(void);
		string* getRG(void);
		string* getRA(void);
		string* getRT(void);
		string* getRL(void);
};

#else
#error GenomReferenceEmbl.h included twice
#endif
