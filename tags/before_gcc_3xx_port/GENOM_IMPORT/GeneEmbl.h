#ifndef GENEEMBL_H
#define GENEEMBL_H

#ifndef MAP
#include <map>
#endif
#ifndef STRING
#include <string>
#endif
#ifndef VECTOR
#include <vector>
#endif



#include <GenomLocation.h>

class GeneEmbl
{
	private:
		std::string type;												//	feature e.g. CDS, gene etc.
		int gen_ref_num;										//	ordinal number of this sequence
		int type_ref_num;										//	ordinal number of the type of this sequence
		std::string id;													//	identifier of this gene sequence
		std::map<std::string,std::string> qualifiers;			//	all qualifiers of this gene sequence
    std::vector<std::string> qualifier_names;
    GenomLocation location;

		std::vector<std::string> row_lines;						//	contains unparsed data

		bool prepared;
    int actual_qualifier;
    std::string tlocation;

	public:
		GeneEmbl(void);
//		virtual ~GeneEmbl();
		std::string* getID(void);
		int getRefNumOfType(void);
		int getRefNumOfGene(void);
		std::string getType(void);
		std::string* getQualifier(void);
		std::string* getValue(std::string*);
		void setRefNumOfType(int n) { type_ref_num = n;}
		void setRefNumOfGene(int n) { gen_ref_num = n;}
    std::string getTempLocation(void);
    void setID(std::string);
    void setGeneNumber(long);
    GenomLocation & getGeneLocation();

		void updateContain(std::string*);
		void prepareGene(void);	//	parses only each gene sequence
};

#else
#error GeneEmbl.h included twice
#endif
