#ifndef GENOMFEATURETABLEEMBL_H
#define GENOMFEATURETABLEEMBL_H

#ifndef VECTOR
#include <vector>
#endif
#ifndef STRING
#include <string>
#endif
#ifndef GENEEMBL_H
#include "GeneEmbl.h"
#endif
#ifndef GENOMSOURCEFEATUREEMBL_H
#include "GenomSourceFeatureEmbl.h"
#endif

class GenomFeatureTableEmbl
{
	private:
		GenomSourceFeatureEmbl source;	//	source feature of feature table
		vector<GeneEmbl> genes;
		vector<string> gene_ids;
		int type_ref_nums[80];
		int genes_num;
		vector<string> row_lines;
		vector<string> features;

		bool prepared;
		int actual_gene_id;
		int actual_gene;

		int featureNameToNumber(string);

	public:
		GenomFeatureTableEmbl();
		~GenomFeatureTableEmbl();

		GenomSourceFeatureEmbl* getSource(void);
		string* getGeneID(void);
		GeneEmbl* getGeneByID(string*);
		GeneEmbl* getGene(void);


		void updateContain(string*);
		void prepareFeatureTable(void);


};

#else
#error GenomFeatureTableEmbl.h included twice
#endif
