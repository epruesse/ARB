#include "GeneEmbl.h"
#include "GenomUtilities.h"

using namespace std;

GeneEmbl::GeneEmbl()
{
	prepared = false;
	gen_ref_num = 0;
  actual_qualifier = 0;
}

/*GeneEmbl::~GeneEmbl()
{

}                                   */

void GeneEmbl::setGeneNumber(long num)
{
  gen_ref_num = num;
}

string* GeneEmbl::getID(void)
{
	if(!prepared)
	{
		prepareGene();
	}
	return &id;
}

int GeneEmbl::getRefNumOfType(void)
{
	if(!prepared)
	{
		prepareGene();
	}
	return type_ref_num;
}

int GeneEmbl::getRefNumOfGene(void)
{
	if(!prepared)
	{
		prepareGene();
	}
	return gen_ref_num;
}

string GeneEmbl::getType(void)
{
	if(!prepared)
	{
		prepareGene();
	}
	return type;
}

string* GeneEmbl::getQualifier(void)
{
	if(!prepared)
	{
		prepareGene();
	}
	return &(qualifier_names[actual_qualifier++]);
}

string* GeneEmbl::getValue(string* qname)
{
	if(!prepared)
	{
		prepareGene();
	}
  return &(qualifiers[*qname]);
}

void GeneEmbl::updateContain(string* ul)
{
	prepared = false;
	row_lines.push_back(*ul);
}

string GeneEmbl::getTempLocation(void)
{
  if(!prepared)
	{
		prepareGene();
	}
  return tlocation;
}

GenomLocation & GeneEmbl::getGeneLocation()
{
  return location;
}

void GeneEmbl::setID(string nID)
{
  id = nID;
}

void GeneEmbl::prepareGene(void)
{
	string tstr = toOneString(row_lines);
	string fea("FT");
	string tstr0;
	vector<string> words;
	eliminateFeatureTableSign(&tstr,&fea);
	words = findAndSeparateWordsBy(&tstr,' ',true);
	int w_count = words.size();
	type = words[0];
	tstr0 = words[1];
	char tc;
	bool qual_val = false;

	string qualifier;
	string value;
    string tloc;
    tloc = tstr0;
	location.parseLocation(tstr0);
    tlocation = tloc;

	for(int i = 2; i < w_count; i++)
	{
		tstr0 = words[i];
		tc = tstr0.at(0);
		if(tc == '/')
		{
			if(qual_val)
			{
				qualifiers[qualifier] = "null";
				qualifier = tstr0.substr(1);
				qual_val = true; // unsinnig aber...
			}
			else
			{
				qualifier = tstr0.substr(1);
				qual_val = true;
			}
		}
		else
		{
			trimByDoubleQuote(tstr0);
			onlyOneSpace(tstr0);
			value = tstr0;
			qualifiers[qualifier] = value;
			qual_val = false;
		}
	}
	prepared = true;
}
