#ifndef GENOMREFERENCEEMBL_H
#define GENOMREFERENCEEMBL_H

#include <string>
#include <cstdlib>
#include <vector>

class GenomReferenceEmbl
{
	private:
		int reference_number;						//	RN
		std::string reference_comment;					//	RC
		std::vector<long> reference_position;			//	RP	//	begin-end, begin-end, ...
		std::vector<std::string> reference_cross_reference;	//	RX
    std::string reference_cross_reference_to_string;
		std::string reference_group;						//	RG
		std::vector<std::string> reference_authors;			//	RA
    std::string reference_authors_to_string;
		std::string reference_title;						//	RT
		std::string reference_location;					//	RL

		int actual_RP;
		int actual_RX;
		int actual_RA;
		std::vector<std::string> row_lines;
		bool prepared;

	public:
		GenomReferenceEmbl();
		~GenomReferenceEmbl();

		void updateReference(std::string*);
		void prepareReference(void);	          //	parses reference

		int getRN(void);
		std::string* getRC(void);
		long getRPBegin(void);
		long getRPEnd(void);
		std::string* getRX(void);
		std::string* getRG(void);
		std::string* getRA(void);
		std::string* getRT(void);
		std::string* getRL(void);
};

#else
#error GenomReferenceEmbl.h included twice
#endif
