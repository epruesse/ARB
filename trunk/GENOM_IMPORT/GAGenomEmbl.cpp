#include "GAGenomEmbl.h"
#include "GAGenomUtilities.h"

#ifndef _CCP_IOSTREAM
#include <iostream>
#endif
#ifndef _CCP_IOMANIP
#include <iomanip>
#endif
#ifndef _CCP_FSTREAM
#include <fstream>
#endif

using namespace std;
using namespace gellisary;

gellisary::GAGenomEmbl::GAGenomEmbl(string * fname):GAGenom(fname)
{
	iter = 0;
}

void gellisary::GAGenomEmbl::parseFlatFile()
{
	ifstream flatfile(file_name.c_str());
	string tmp_str;
	string del_str;
	string rep_str;
	vector<string> tmp_vector;
	vector<string> tmp_lines_vector;
	string t_str;

	GAGenomReferenceEmbl *tmp_reference;
	
//	bool dt = false;
	bool de = false;
	bool kw = false;
	bool oc = false;
	bool co_cc = false;
	bool rn = false;
	bool cc = false;
	bool co = false;
	bool ft = false;
	bool sq = false;
	
	char tmp_line[128];
	
	while(!flatfile.eof())
	{
	flatfile.getline(tmp_line,128);
	tmp_str = tmp_line;
	switch(tmp_str[0])
	{
		case 'I':
			if(tmp_str[1] == 'D')
			{
				del_str = "ID";
				GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&identification);
			}
			break;
		case 'A':
			if(tmp_str[1] == 'C')
			{
				del_str = "AC";
				GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&accession_number);
			}
			break;
		case 'S':
			switch(tmp_str[1])
			{
				case 'V':
					del_str = "SV";
					GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&sequence_version);
					break;
				case 'Q':
					del_str = "SQ";
					GAGenomUtilities::replaceByWhiteSpaceCleanly(&tmp_str,&del_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,' ',true);
					for(int i = 1; i < 13; i = i + 2)
					{
						t_str = tmp_vector[i];
						sequence_header.push_back(GAGenomUtilities::stringToInteger(&t_str));
					}
					sq = true;
					break;
			}
			break;
		case 'D':
			switch(tmp_str[1])
			{
				case 'E':
					tmp_lines_vector.push_back(tmp_str);
					de = true;
					break;
			}
			break;
		case 'K':
			if(tmp_str[1] == 'W')
			{
				del_str = "KW";
				GAGenomUtilities::replaceByWhiteSpaceCleanly(&tmp_str,&del_str);
				tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,' ',true);
				if(tmp_vector.size() > 0 && tmp_vector[0] != ".")
				{
					key_words.push_back(GAGenomUtilities::toOneString(&tmp_vector,true));
				}
				kw = true;
			}
			break;
		case 'O':
			switch(tmp_str[1])
			{
				case 'S':
					del_str = "OS";
					GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&organism_species);
					break;
				case 'C':
					tmp_lines_vector.push_back(tmp_str);
					oc = true;
					break;
				case 'G':
					del_str = "OG";
					GAGenomUtilities::preparePropertyString(&tmp_str,&del_str,&organelle);
					break;
			}
			break;
		case 'R':
			switch(tmp_str[1])
			{
				case 'N':
				if(rn)
				{
					if(!tmp_lines_vector.empty())
					{
						tmp_reference = new GAGenomReferenceEmbl;
						for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
						{
							tmp_reference->update(&(tmp_lines_vector[m]));
						}
						tmp_reference->parse();
						references.push_back(*tmp_reference);
						tmp_lines_vector.clear();
						delete(tmp_reference);
					}
					tmp_lines_vector.push_back(tmp_str);
				}
				else
				{
					tmp_lines_vector.push_back(tmp_str);
					rn = true;
				}
				break;
			}
			break;
		case 'C':
			switch(tmp_str[1])
			{
				case 'O':
					tmp_lines_vector.push_back(tmp_str);
					co = true;
					co_cc = false;
					break;
				case 'C':
					if(cc)
					{
						tmp_lines_vector.push_back(tmp_str);
					}
					else if(co_cc)
					{
						tmp_lines_vector.push_back(tmp_str);
					}
					else if(co)
					{
						tmp_lines_vector.push_back(tmp_str);
					}
					else
					{
						t_str = tmp_str.substr(0,20);
						if(t_str.find("join(") == string::npos)
						{
							tmp_lines_vector.push_back(tmp_str);
							cc = true;
							co = false;
							co_cc = false;
						}
						else
						{
							tmp_lines_vector.push_back(tmp_str);
							co = true;
							co_cc = true;
						}
					}
					break;
			}
			break;
		case 'F':
			if(tmp_str[1] == 'E')
			{
				feature_table.update(&tmp_str);
				ft = true;
			}
			break;
		case '/':
			flatfile.close();
			break;
		case 'X':
			if(de)
			{
				if(!tmp_lines_vector.empty())
				{
					t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "DE";
					GAGenomUtilities::preparePropertyString(&t_str,&del_str,&description);
				}
				de = false;
				tmp_lines_vector.clear();
			}
			if(kw)
			{
				kw = false;
			}
			if(oc)
			{
				if(!tmp_lines_vector.empty())
				{
					t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "OC";
					GAGenomUtilities::trimString(&t_str);
					vector<string> tmp_s_vec;
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					tmp_s_vec = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					organism_classification = tmp_s_vec;
					t_str = GAGenomUtilities::toOneString(&tmp_s_vec,true);
					GAGenomUtilities::preparePropertyString(&t_str,&del_str,&organism_classification_as_one_string);
				}
				oc = false;
				tmp_lines_vector.clear();
			}
			if(rn)
			{
				if(!tmp_lines_vector.empty())
				{
					tmp_reference = new GAGenomReferenceEmbl;
					for(int m = 0; m < (int)tmp_lines_vector.size(); m++)
					{
						tmp_reference->update(&(tmp_lines_vector[m]));
					}
					tmp_reference->parse();
					references.push_back(*tmp_reference);
					delete(tmp_reference);
				}
				tmp_lines_vector.clear();
				rn = false;
			}
			if(cc)
			{
				if(!tmp_lines_vector.empty())
				{
					t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "CC";
					GAGenomUtilities::replaceByWhiteSpaceCleanly(&tmp_str,&del_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',true);
					free_text_comment.push_back(GAGenomUtilities::toOneString(&tmp_vector,true));
				}
				tmp_lines_vector.clear();
				cc = false;
			}
			if(co)
			{
				if(!tmp_lines_vector.empty())
				{
					t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					if(co_cc)
					{
						del_str = "CC";
					}
					else
					{
						del_str = "CO";
					}
					GAGenomUtilities::preparePropertyString(&t_str,&del_str,&contig);
				}
				tmp_lines_vector.clear();
				co = false;
				co_cc = false;
			}
			if(ft)
			{
				feature_table.parse();
				ft = false;
			}
			if(sq)
			{
				parseSequence(&tmp_str);
				sequence += tmp_str;
				prepared = true;
			}
			break;
		case ' ':
			if(sq)
			{
				parseSequence(&tmp_str);
				sequence += tmp_str;
				prepared = true;
			}
			break;
		}
	}
	prepared = true;
}

string * gellisary::GAGenomEmbl::getDateOfCreation()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &date_of_creation;
}

string * gellisary::GAGenomEmbl::getDateOfLastUpdate()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &date_of_last_update;
}

string * gellisary::GAGenomEmbl::getOrganelle()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &organelle;
}

vector<string> * gellisary::GAGenomEmbl::getDataCrossReference()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &database_cross_reference;
}
/*
string * gellisary::GAGenomEmbl::getOrganismClassificationAsOneString()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &organism_classification_as_one_string;
}
*/
GAGenomFeatureTableEmbl * gellisary::GAGenomEmbl::getFeatureTable()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &feature_table;
}

GAGenomReferenceEmbl * gellisary::GAGenomEmbl::getReference()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	if(iter < (int)references.size())
	{
		tmp_ref = references[iter];
		iter++;
		return &tmp_ref; // muss noch anschauen
	}
	else
	{
		return NULL;
	}
}
/*
vector<string> * gellisary::GAGenomEmbl::getOrganismClassification()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &organism_classification;
}
*/
string * gellisary::GAGenomEmbl::getDataCrossReferenceAsOneString()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &database_cross_reference_as_one_string;
}

vector<int> * gellisary::GAGenomEmbl::getSequenceHeader()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &sequence_header;
}

void gellisary::GAGenomEmbl::parseSequence(string * source_str)
{
	string target_str;
	char tmp_char;
	int i = 0;
	vector<string> tmp_vector;
	for(i = 0; i < (int) source_str->size(); i++)
	{
		tmp_char = source_str->operator[](i);
		if((tmp_char == 't') || (tmp_char == 'g') || (tmp_char == 'c') || (tmp_char == 'a') || (tmp_char == 'T') || (tmp_char == 'G') || (tmp_char == 'C') || (tmp_char == 'A'))
		{
			target_str += tmp_char;
		}
	}
	GAGenomUtilities::trimString(&target_str);
	GAGenomUtilities::onlyOneDelimerChar(&target_str,' ');
	tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&target_str,' ',false);
	target_str = GAGenomUtilities::toOneString(&tmp_vector,false);
	*source_str = target_str;
}
