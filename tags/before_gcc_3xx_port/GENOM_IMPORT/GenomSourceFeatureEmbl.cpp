#include "GenomSourceFeatureEmbl.h"
#include "GenomUtilities.h"

using namespace std;

GenomSourceFeatureEmbl::GenomSourceFeatureEmbl()
{
	prepared = false;
	actual_qualifier = 0;
  source_begin = 0;
	source_end = 0;
}

GenomSourceFeatureEmbl::~GenomSourceFeatureEmbl()
{

}

const string* GenomSourceFeatureEmbl::getQualifier(void)
{
	if(!prepared)
	{
		prepareSource();
	}
  if(ki != qualifiers.end())
  {
    const string& key = ki->first;
    ki++;
    return &key;
  }
  else
  {
    return NULL;
  }
}

string* GenomSourceFeatureEmbl::getValue(const string *qu)
{
	if(!prepared)
	{
		prepareSource();
	}
	return &(qualifiers[*qu]);
}

void GenomSourceFeatureEmbl::updateContain(string &nl)
{
	row_lines.push_back(nl);
	prepared = false;
}

void GenomSourceFeatureEmbl::prepareSource(void)
{
	string t_str_0;
	string t_str_1;
	string t_str_2;
	string t_str_3;
//	int qsize = row_lines.size();
	int count_words = 0;
	vector<string> words;
	vector<long> locs;
	string function;
	string fae = "FT";
	char buff_0[10240];
//	char buff_1[10240];
	bool qual = false;

	vector<string> tvec;

	t_str_0 = row_lines[0];
	eliminateFeatureTableSign(&t_str_0,&fae);
	words = findAndSeparateWordsBy(&t_str_0,' ',true);
  count_words = words.size();
	for(int j = 0; j < count_words; j++)
	{
		if(words[j] == "source")
		{
			t_str_1 = words[j+1];
			vector<long> locs = parseSourceLocation(t_str_1);
			if(locs.size() > 0)
			{
				source_begin = locs[0];
				source_end = locs[1];
			}
		}
	}

	t_str_1 =	toOneString(row_lines);
	eliminateFeatureTableSign(&t_str_1,&fae);
	words = findAndSeparateWordsBy(&t_str_1,' ',true);
	for(int j = 2; j < (int)words.size(); j++)
	{
		t_str_2 = words[j];
		strcpy(buff_0,t_str_2.c_str());
		if(buff_0[0] == '/')
		{
			t_str_3 = t_str_2.substr(1);
			qual = true;
		}
		else
		{
			if(qual)
			{
				qualifiers[t_str_3] = t_str_2;
				qual = false;
			}
		}
	}
  ki = qualifiers.begin();
	prepared = true;
}
