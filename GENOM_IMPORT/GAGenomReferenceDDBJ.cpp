#include "GAGenomReferenceDDBJ.h"
#include "GAGenomUtilities.h"

using namespace std;
using namespace gellisary;

void gellisary::GAGenomReferenceDDBJ::parse()
{
	string tmp_str;
	string tmp_str2;
	string del_str;
	string rep_str;
	vector<string> tmp_vector;
	vector<string> tmp_lines_vector;
	string t_str;
	bool ra = false;
	bool rt = false;
	bool rx = false;
	bool rl = false;
	bool rc = false;
//	bool rg = false;
	for(int i = 0; i < (int) row_lines.size(); i++)
	{
		tmp_str = row_lines[i];
		tmp_str2 = tmp_str.substr(0,12);
		if(tmp_str2[4] != ' ')
		{
			GAGenomUtilities::trimString(&tmp_str2);
			
			if((tmp_str2 == "REFERENCE") || (tmp_str2 == "reference"))
			{
				if(rl)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					string ttt_str;
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						ttt_str.append(tt_str);
					}
					reference_location = ttt_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rt)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_title = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rx)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ".";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
				if(ra)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						reference_authors.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					ra = false;
				}
				bool hasPosition = false;
				for(int l = 0; l < (int) tmp_str.size(); l++)
				{
					if(tmp_str[l] == '(')
					{
						hasPosition = true;
					}
				}
				del_str = "\r";
				rep_str = " ";
				GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
				GAGenomUtilities::trimString(&tmp_str);
				if(hasPosition)
				{
					GAGenomUtilities::onlyOneDelimerChar(&tmp_str,' ');
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,' ',false);
					if(tmp_vector.size() > 0)
					{
						t_str = tmp_vector[2];
						del_str = "(";
						rep_str = " ";
						GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
						del_str = ")";
						rep_str = " ";
						GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
						del_str = "bases";
						rep_str = " ";
						GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
						del_str = "to";
						rep_str = " ";
						GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
						GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
						GAGenomUtilities::trimString(&t_str);
						vector<string> t_vec = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
						if(t_vec.size() > 0)
						{
							t_str = t_vec[0];
							reference_position_begin.push_back(GAGenomUtilities::stringToInteger(&t_str));
							t_str = t_vec[1];
							reference_position_end.push_back(GAGenomUtilities::stringToInteger(&t_str));
						}
					}
				}
				else
				{
					GAGenomUtilities::onlyOneDelimerChar(&tmp_str,' ');
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,' ',false);
					if(tmp_vector.size() > 0)
					{
						t_str = tmp_vector[1];
						reference_number = GAGenomUtilities::stringToInteger(&t_str);
					}
					tmp_vector.clear();
				}
			}
			else if((tmp_str2 == "JOURNAL") || (tmp_str2 == "journal"))
			{
				if(rt)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_title = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rx)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ".";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
				/*if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_group = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rg = false;
				}*/
				if(ra)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						reference_authors.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					ra = false;
				}
				if(rc)
				{
					del_str = "RC";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_comment = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rc = false;
				}
				rl = true;
				tmp_lines_vector.push_back(tmp_str);
			}
			else if((tmp_str2 == "AUTHORS") || (tmp_str2 == "authors"))
			{
				if(rl)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					string ttt_str;
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						ttt_str.append(tt_str);
					}
					reference_location = ttt_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				/*if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_group = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rg = false;
				}*/
				if(rt)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_title = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rx)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ".";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
				if(rc)
				{
					del_str = "RC";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_comment = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rc = false;
				}
				ra = true;
				tmp_lines_vector.push_back(tmp_str);
			}
			else if((tmp_str2 == "TITLE") || (tmp_str2 == "title"))
			{
				if(rl)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					string ttt_str;
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						ttt_str.append(tt_str);
					}
					reference_location = ttt_str;
					tmp_lines_vector.clear();
					rl = false;
				}/*
				if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_group = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rg = false;
				}*/
				if(rx)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ".";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
				if(ra)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						reference_authors.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					ra = false;
				}
				if(rc)
				{
					del_str = "RC";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_comment = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rc = false;
				}
				rt = true;
				tmp_lines_vector.push_back(tmp_str);
			}
			else if((tmp_str2 == "CONSRTM") || (tmp_str2 == "consrtm"))
			{
				
			}
			else if((tmp_str2 == "PUBMED") || (tmp_str2 == "pubmed") || (tmp_str2 == "MEDLINE") || (tmp_str2 == "medline"))
			{
				if(rt)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_title = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rt = false;
				}
				/*if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_group = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rg = false;
				}*/
				if(rl)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					string ttt_str;
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						ttt_str.append(tt_str);
					}
					reference_location = ttt_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rc)
				{
					del_str = "RC";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_comment = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rc = false;
				}
				if(ra)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						reference_authors.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					ra = false;
				}
				rx = true;
				tmp_lines_vector.push_back(tmp_str);
			}
			else
			{
				if(ra)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						reference_authors.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					ra = false;
				}
				if(rl)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					string ttt_str;
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						ttt_str.append(tt_str);
					}
					reference_location = ttt_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rt)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					vector<string> tt_vector;
					for(int j = 1; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						tt_vector.push_back(tt_str);
					}
					reference_title = GAGenomUtilities::toOneString(&tt_vector,true);
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rx)
				{
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ".";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						GAGenomUtilities::trimString(&tt_str);
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
			}
		}
	}
	prepared = true;
}

string * gellisary::GAGenomReferenceDDBJ::getJournal()
{
	if(!prepared)
	{
		parse();
	}
	return &reference_journal;
}
vector<int> * gellisary::GAGenomReferenceDDBJ::getPubMeds()
{
	if(!prepared)
	{
		parse();
	}
	return &reference_pubmed;
}
string * gellisary::GAGenomReferenceDDBJ::getCrossReferenceAsString()
{
	if(!prepared)
	{
		parse();
	}
	return &reference_cross_reference_as_string;
}
vector<string> * gellisary::GAGenomReferenceDDBJ::getCrossReference()
{
	if(!prepared)
	{
		parse();
	}
	return &reference_cross_reference;
}
vector<int> * gellisary::GAGenomReferenceDDBJ::getMedLines()
{
	if(!prepared)
	{
		parse();
	}
	return &reference_medline;
}
