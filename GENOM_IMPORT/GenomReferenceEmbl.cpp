#include "GenomReferenceEmbl.h"
#include "GenomUtilities.h"

using namespace std;

GenomReferenceEmbl::GenomReferenceEmbl()
{
	actual_RP = 0;
	actual_RX = 0;
	actual_RA = 0;
	reference_title = "";
	prepared = false;
  reference_number = 0;
  reference_comment = "";
}

GenomReferenceEmbl::~GenomReferenceEmbl()
{
	actual_RP = 0;
	actual_RX = 0;
	actual_RA = 0;
	reference_title = "";
	prepared = false;
  reference_number = 0;
  reference_comment = "";
  reference_position.clear();
  reference_cross_reference.clear();
  reference_cross_reference_to_string = "";
  reference_group = "";
  reference_authors.clear();
  reference_authors_to_string = "";
  reference_location = "";
  row_lines.clear();
}

void GenomReferenceEmbl::updateReference(string* ref_line)
{
	row_lines.push_back(*ref_line);
	prepared = false;
}

void GenomReferenceEmbl::prepareReference(void)
{
	int rsize = row_lines.size();
	char line[1024];
	char buff[1024];
	string str;
	string tstr;
	string feat;
	vector<string> words;
	vector<string> tvec;
  vector<string> trefvec;
  vector<string> ttitvec;
  bool rl = false;
  bool rt = false;
	for(int i = 0; i < rsize; i++)
	{
		str = row_lines[i];
		strcpy(line,str.c_str());
		if((line[0] == 'R') || (line[0] == 'r'))
		{
      switch(line[1])
			{
				case 'N': case 'n':
						feat = "RN";
						eliminateFeatureTableSign(&str,&feat);
						words = findAndSeparateWordsBy(&str,' ',true);
						for(int j = 0; j < (int)words.size(); j++)
						{
							tstr = words[j];
							tstr = trimString(tstr);
							strcpy(buff,tstr.c_str());
							for(int k = 0; k < (int)strlen(buff); k++)
							{
								switch(buff[k])
								{
									case '[':
											char zahl[2];
											zahl[0] = buff[k+1];
											zahl[1] = '\0';
											reference_number = atoi(zahl);
										break;
								}
							}
						}
					break;
				case 'C': case 'c':
						feat = "RC";
						eliminateFeatureTableSign(&str,&feat);
						words = findAndSeparateWordsBy(&str,' ',true);
						reference_comment = toOneString(words);
					break;
				case 'P': case 'p':
						feat = "RP";
						eliminateFeatureTableSign(&str,&feat);
						words = findAndSeparateWordsBy(&str,' ',true);

						for(int j = 0; j < (int)words.size(); j++)
						{
							tstr = words[j];
							tvec = findAndSeparateWordsBy(&tstr,'-',true);
							if(tvec.size() > 0)
							{
								reference_position.push_back(atol(tvec[0].c_str()));
							}
							if(tvec.size() > 1)
							{
								reference_position.push_back(atol(tvec[1].c_str()));
							}
              tvec.clear();
						}
					break;
				case 'X': case 'x':
						feat = "RX";
						eliminateFeatureTableSign(&str,&feat);
						words = findAndSeparateWordsBy(&str,' ',true);

						for(int j = 0; j < (int)words.size(); j++)
						{
							tstr = words[j];
							tvec = findAndSeparateWordsBy(&tstr,'.',true);
							for(int k = 0; k < (int)tvec.size(); k++)
							{
								reference_cross_reference.push_back(tvec[k]);
							}
              tvec.clear();
						}
					break;
				case 'G': case 'g':
						feat = "RG";
						eliminateFeatureTableSign(&str,&feat);
						words = findAndSeparateWordsBy(&str,' ',true);
						reference_group = toOneString(words);
					break;
				case 'A': case 'a':
						feat = "RA";
						eliminateFeatureTableSign(&str,&feat);
						words = findAndSeparateWordsBy(&str,',',true);
						for(int j = 0; j < (int)words.size(); j++)
						{
							tstr = words[j];
							reference_authors.push_back(tstr);
						}
					break;
				case 'T': case 't':
						ttitvec.push_back(str);
            rt = true;
					break;
				case 'L': case 'l':
            trefvec.push_back(str);
            rl = true;
					break;
			}
		}
	}
  if(rl)
  {
    feat = "RL";
    if(trefvec.size() > 0)
    {
      tstr = toOneString(trefvec);
			eliminateFeatureTableSign(&tstr,&feat);
   		words = findAndSeparateWordsBy(&tstr,' ',true);
  		reference_location = toOneString(words);
      trefvec.clear();
    }
    rl = false;
  }
  if(rt)
  {
    feat = "RT";
    if(ttitvec.size() > 0)
    {
      tstr = toOneString(ttitvec);
			eliminateFeatureTableSign(&tstr,&feat);
   		words = findAndSeparateWordsBy(&tstr,' ',true);
  		reference_title = toOneString(words);
      ttitvec.clear();
    }
    rt = false;
  }
	prepared = true;
}

int GenomReferenceEmbl::getRN(void)
{
//  cout << "in GenomReferenceEmbl::getRN()" << endl;
  if(!prepared)
	{
		prepareReference();
	}
//  cout << "in GenomReferenceEmbl::getRN() _ 2" << endl;
	return reference_number;
}

string* GenomReferenceEmbl::getRC(void)
{
	if(!prepared)
	{
		prepareReference();
	}
	return &reference_comment;
}

long GenomReferenceEmbl::getRPBegin(void)
{
	if(!prepared)
	{
		prepareReference();
	}
	if((int)reference_position.size() > actual_RP)
	{
		return reference_position[actual_RP++];
	}
	else
	{
		return 0;	// ob es geht?
	}
}

long GenomReferenceEmbl::getRPEnd(void)
{
	if(!prepared)
	{
		prepareReference();
	}
	if((int)reference_position.size() > actual_RP)
	{
		return reference_position[actual_RP++];
	}
	else
	{
		return 0;	// ob es geht?
	}
}

string* GenomReferenceEmbl::getRX(void)
{
	if(!prepared)
	{
		prepareReference();
	}
  reference_cross_reference_to_string = toOneString(reference_cross_reference);
  return &reference_cross_reference_to_string;
	/*if((int)reference_cross_reference.size() > actual_RX)
	{
		return &reference_cross_reference[actual_RX++];
	}
	else
	{
		return NULL;	//	or null
	}*/
}

string* GenomReferenceEmbl::getRG(void)
{
	if(!prepared)
	{
		prepareReference();
	}
	return &reference_group;
}

string* GenomReferenceEmbl::getRA(void)
{
	if(!prepared)
	{
		prepareReference();
	}
  reference_authors_to_string = toOneString(reference_authors);
  return &reference_authors_to_string;
  /*if((int)reference_authors.size() > actual_RA)
	{
		return &reference_authors[actual_RA++];
	}
	else
	{
		return NULL;	//	or null
	}*/
}

string* GenomReferenceEmbl::getRT(void)
{
	if(!prepared)
	{
		prepareReference();
	}
	return &reference_title;
}

string* GenomReferenceEmbl::getRL(void)
{
	if(!prepared)
	{
		prepareReference();
	}
	return &reference_location;
}
