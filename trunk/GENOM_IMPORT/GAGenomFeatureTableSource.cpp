#include "GAGenomFeatureTableSource.h"

using namespace std;

int gellisary::GAGenomFeatureTableSource::getBegin()
{
	if(!prepared)
	{
		parse();
	}
	return source_begin;
}

int gellisary::GAGenomFeatureTableSource::getEnd()
{
	if(!prepared)
	{
		parse();
	}
	return source_end;
}

string * gellisary::GAGenomFeatureTableSource::getNameOfQualifier()
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

string * gellisary::GAGenomFeatureTableSource::getValueOfQualifier(string * source_str)
{
	if(!prepared)
	{
		parse();
	}
	return &(qualifiers[*source_str]);
}
