#include "GAGenomGene.h"

using namespace std;

string * gellisary::GAGenomGene::getGeneType()
{
	if(!prepared)
	{
		parse();
	}
	return &gene_type;
}

int gellisary::GAGenomGene::getGeneTypeNumber()
{
	if(!prepared)
	{
		parse();
	}
	return gene_type_number;
}

int gellisary::GAGenomGene::getGeneNumber()
{
	if(!prepared)
	{
		parse();
	}
	return gene_number;
}

string * gellisary::GAGenomGene::getQualifierName()
{
	if(!prepared)
	{
		parse();
	}
	if(iter != qualifiers.end())
	{
		tmp_key = iter->first;
		iter++;
		return &tmp_key;
	}
	else
	{
		return NULL;
	}
}

string * gellisary::GAGenomGene::getLocationAsString()
{
	if(!prepared)
	{
		parse();
	}
	return &location_as_string;
}

string * gellisary::GAGenomGene::getQualifierValue(string * source_str)
{
	if(!prepared)
	{
		parse();
	}
	return &(qualifiers[*source_str]);
}

void gellisary::GAGenomGene::setGeneNumber(int source_int)
{
	gene_number = source_int;
}

void gellisary::GAGenomGene::setGeneTypeNumber(int source_int)
{
	gene_type_number = source_int;
}

string * gellisary::GAGenomGene::getNameOfGene()
{
	if(!prepared)
	{
		parse();
	}
	return &name_of_gene;
}

void gellisary::GAGenomGene::setNameOfGene(string * source_str)
{
	name_of_gene = *source_str;
}
