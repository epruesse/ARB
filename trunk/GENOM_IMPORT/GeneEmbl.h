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
		string type;												//	feature e.g. CDS, gene etc.
		int gen_ref_num;										//	ordinal number of this sequence
		int type_ref_num;										//	ordinal number of the type of this sequence
		string id;													//	identifier of this gene sequence
		map<string,string> qualifiers;			//	all qualifiers of this gene sequence
    vector<string> qualifier_names;
    GenomLocation location;

		vector<string> row_lines;						//	contains unparsed data

		bool prepared;
    int actual_qualifier;
    string tlocation;

	public:
		GeneEmbl(void);
//		virtual ~GeneEmbl();
		string* getID(void);
		int getRefNumOfType(void);
		int getRefNumOfGene(void);
		string getType(void);
		string* getQualifier(void);
		string* getValue(string*);
		void setRefNumOfType(int n) { type_ref_num = n;}
		void setRefNumOfGene(int n) { gen_ref_num = n;}
    string getTempLocation(void);
    void setID(string);
    void setGeneNumber(long);
    GenomLocation & getGeneLocation();

		void updateContain(string*);
		void prepareGene(void);	//	parses only each gene sequence
};

#else
#error GeneEmbl.h included twice
#endif
