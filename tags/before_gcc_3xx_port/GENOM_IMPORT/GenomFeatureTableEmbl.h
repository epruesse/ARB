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
		std::vector<GeneEmbl> genes;
		std::vector<std::string> gene_ids;
		int type_ref_nums[80];
		int genes_num;
		std::vector<std::string> row_lines;
		std::vector<std::string> features;

		bool prepared;
		int actual_gene_id;
		int actual_gene;

		int featureNameToNumber(std::string);

	public:
		GenomFeatureTableEmbl();
		~GenomFeatureTableEmbl();

		GenomSourceFeatureEmbl* getSource(void);
		std::string* getGeneID(void);
		GeneEmbl* getGeneByID(std::string*);
		GeneEmbl* getGene(void);


		void updateContain(std::string*);
		void prepareFeatureTable(void);


};

#else
#error GenomFeatureTableEmbl.h included twice
#endif
