#include "GAGenomFeatureTableGenBank.h"
#include "GAGenomUtilities.h"

using namespace std;
using namespace gellisary;

gellisary::GAGenomFeatureTableGenBank::GAGenomFeatureTableGenBank()
{
	features.push_back("attenuator");
	features.push_back("C_region");
	features.push_back("CAAT_signal");
	features.push_back("CDS");
	features.push_back("conflict");
	features.push_back("D-loop");
	features.push_back("D_segment");
	features.push_back("enhancer");
	features.push_back("exon");
	features.push_back("gap");
	features.push_back("GC_signal");
	features.push_back("gene");
	features.push_back("iDNA");
	features.push_back("intron");
	features.push_back("J_segment");
	features.push_back("LTR");
	features.push_back("mat_peptide");
	features.push_back("misc_binding");
	features.push_back("misc_difference");
	features.push_back("misc_feature");
	features.push_back("misc_recomb");
	features.push_back("misc_RNA");
	features.push_back("misc_signal");
	features.push_back("misc_structure");
	features.push_back("modified_base");
	features.push_back("mRNA");
	features.push_back("N_region");
	features.push_back("old_sequence");
	features.push_back("operon");
	features.push_back("oriT");
	features.push_back("polyA_signal");
	features.push_back("polyA_site");
	features.push_back("precursor_RNA");
	features.push_back("prim_transcript");
	features.push_back("primer_bind");
	features.push_back("promoter");
	features.push_back("protein_bind");
	features.push_back("RBS");
	features.push_back("repeat_region");
	features.push_back("repeat_unit");
	features.push_back("rep_region");
	features.push_back("rRNA");
	features.push_back("S_region");
	features.push_back("satellite");
	features.push_back("scRNA");
	features.push_back("siq_peptide");
	features.push_back("snRNA");
	features.push_back("snoRNA");
	features.push_back("source");
	features.push_back("stem_loop");
	features.push_back("STS");
	features.push_back("TATA_signal");
	features.push_back("terminator");
	features.push_back("trensit_prptide");
	features.push_back("tRNA");
	features.push_back("unsure");
	features.push_back("V_region");
	features.push_back("V_segment");
	features.push_back("variation");
	features.push_back("3'clip");
	features.push_back("3'UTR");
	features.push_back("5'clip");
	features.push_back("5'UTF");
	features.push_back("-10_signal");
	features.push_back("-35_signal");
	features.push_back("-");
	for(int i = 0; i < (int) features.size(); i++)
	{
		number_of_features.push_back(0);
	}
}

int gellisary::GAGenomFeatureTableGenBank::nameToNumberOfFeature(string * source_str)
{
	for(int i = 0; i < (int)features.size();i++)
	{
		if(features[i] == (*source_str))
		{
			return i;
		}
	}
	return -1;
}

void gellisary::GAGenomFeatureTableGenBank::parse()
{
	string tmp_str;
	string t_str;
	bool newgene = true;
	bool source_open = false;
	bool gene_open = false;
	int tmp_num = 0;
	GAGenomGeneGenBank *tmp_gene;
	for(int i = 0; i < (int) row_lines.size();i++)
	{
		tmp_str = row_lines[i];
		newgene = GAGenomUtilities::isNewGene(&tmp_str);
		if(newgene)
		{
			if(gene_open)
			{
				tmp_gene->setGeneNumber(number_of_genes++);
				tmp_gene->parse();
				tmp_num = nameToNumberOfFeature(tmp_gene->getGeneType());
				if(tmp_num != -1)
				{
					t_str = GAGenomUtilities::generateGeneID(tmp_gene->getLocationAsString(),tmp_num);
					tmp_gene->setNameOfGene(&t_str);
					number_of_features[tmp_num]++;
					tmp_gene->setGeneTypeNumber(number_of_features[tmp_num]);
					genes[*(tmp_gene->getNameOfGene())] = *tmp_gene;
					delete(tmp_gene);
				}
				gene_open = false;
			}
			if(source_open)
			{
				source_open = false;
			}
			if(GAGenomUtilities::isSource(&tmp_str))
			{
				source.update(&tmp_str);
				source.parse();
				source_open = true;
				gene_open = false;
			}
			else
			{
				tmp_gene = new GAGenomGeneGenBank;
				tmp_gene->update(&tmp_str);
				source_open = false;
				gene_open = true;
				number_of_genes++;
			}
		}
		else
		{
			if(source_open)
			{
				source.update(&tmp_str);
			}
			else if(gene_open)
			{
				tmp_gene->update(&tmp_str);
			}
		}
	}
	if(gene_open)
	{
		tmp_gene->setGeneNumber(number_of_genes++);
		tmp_gene->parse();
		tmp_num = nameToNumberOfFeature(tmp_gene->getGeneType());
		if(tmp_num != -1)
		{
			number_of_features[tmp_num]++;
			tmp_gene->setGeneTypeNumber(number_of_features[tmp_num]);
			genes[*(tmp_gene->getNameOfGene())] = *tmp_gene;
			gene_open = false;
			delete(tmp_gene);
		}
	}
	prepared = true;
}

GAGenomFeatureTableSourceGenBank * gellisary::GAGenomFeatureTableGenBank::getFeatureTableSource()
{
	if(!prepared)
	{
		parse();
	}
	return &source;
}

string * gellisary::GAGenomFeatureTableGenBank::getGeneName()
{
	if(!prepared)
	{
		parse();
	}
	if(iter != genes.end())
	{
		tmp_key = iter->first;
		iter++;
		return &tmp_key; // muss noch anschauen
	}
	else
	{
		return NULL;
	}
}

GAGenomGeneGenBank * gellisary::GAGenomFeatureTableGenBank::getGeneByName(std::string * source_str)
{
	if(!prepared)
	{
		parse();
	}
	return &(genes[*source_str]);
}
