/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#include "GAGenomReferenceEmbl.h"
#include "GAGenomUtilities.h"

using namespace std;
using namespace gellisary;

void gellisary::GAGenomReferenceEmbl::parse()
{
	string tmp_str;
	string del_str;
	string rep_str;
	vector<string> tmp_vector;
	vector<string> tmp_lines_vector;
	string t2_str;
	bool ra = false;
	bool rt = false;
	bool rx = false;
	bool rl = false;
	bool rc = false;
	bool rg = false;
	for(int i = 0; i < (int) row_lines.size(); i++)
	{
		tmp_str = row_lines[i];
		switch(tmp_str[1])
		{
			case 'N':
				if(rl)
				{
					del_str = "RL";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_location = t_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rt)
				{
					del_str = "RT";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_title = t_str;
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_group = t_str;
					tmp_lines_vector.clear();
					rg = false;
				}
				if(rx)
				{
					del_str = "RX";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
				if(ra)
				{
					del_str = "RA";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,',',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
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
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_comment = t_str;
					tmp_lines_vector.clear();
					rc = false;
				}
				del_str = "RN";
				rep_str = " ";
				GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
				del_str = "[";
				GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
				del_str = "]";
				GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
				del_str = "\r";
				GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
				GAGenomUtilities::trimString(&tmp_str);
				reference_number = GAGenomUtilities::stringToInteger(&tmp_str);
				break;
			case 'L':
				if(rt)
				{
					del_str = "RT";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_title = t_str;
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rx)
				{
					del_str = "RX";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
				if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_group = t_str;
					tmp_lines_vector.clear();
					rg = false;
				}
				if(ra)
				{
					del_str = "RA";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,',',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
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
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_comment = t_str;
					tmp_lines_vector.clear();
					rc = false;
				}
				rl = true;
				tmp_lines_vector.push_back(tmp_str);
				break;
			case 'G':
				if(rl)
				{
					del_str = "RL";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_location = t_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rt)
				{
					del_str = "RT";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_title = t_str;
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rx)
				{
					del_str = "RX";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
				if(ra)
				{
					del_str = "RA";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,',',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
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
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_comment = t_str;
					tmp_lines_vector.clear();
					rc = false;
				}
				rg = true;
				tmp_lines_vector.push_back(tmp_str);
				break;
			case 'P':
				if(rl)
				{
					del_str = "RL";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_location = t_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rt)
				{
					del_str = "RT";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_title = t_str;
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_group = t_str;
					tmp_lines_vector.clear();
					rg = false;
				}
				if(rx)
				{
					del_str = "RX";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
				if(ra)
				{
					del_str = "RA";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,',',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
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
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_comment = t_str;
					tmp_lines_vector.clear();
					rc = false;
				}
				del_str = "RP";
				rep_str = " ";
				GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
				tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,',',false);
				for(int j = 0; j < (int) tmp_vector.size(); j++)
				{
					string tt_str = tmp_vector[j];
					vector<string> tt_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tt_str,'-',false);
					t2_str = tt_vector[0];
					reference_position_begin.push_back(GAGenomUtilities::stringToInteger(&t2_str));
					t2_str = tt_vector[1];
					int la = GAGenomUtilities::stringToInteger(&t2_str);
					reference_position_end.push_back(la);
				}
				tmp_vector.clear();
				break;
			case 'A':
				if(rl)
				{
					del_str = "RL";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_location = t_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_group = t_str;
					tmp_lines_vector.clear();
					rg = false;
				}
				if(rt)
				{
					del_str = "RT";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_title = t_str;
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rx)
				{
					del_str = "RX";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
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
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_comment = t_str;
					tmp_lines_vector.clear();
					rc = false;
				}
				ra = true;
				tmp_lines_vector.push_back(tmp_str);
				break;
			case 'T':
				if(rl)
				{
					del_str = "RL";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_location = t_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_group = t_str;
					tmp_lines_vector.clear();
					rg = false;
				}
				if(rx)
				{
					del_str = "RX";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
				if(ra)
				{
					del_str = "RA";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,',',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
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
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_comment = t_str;
					tmp_lines_vector.clear();
					rc = false;
				}
				rt = true;
				tmp_lines_vector.push_back(tmp_str);
				break;
			case 'X':
				if(rt)
				{
					del_str = "RT";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_title = t_str;
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_group = t_str;
					tmp_lines_vector.clear();
					rg = false;
				}
				if(rl)
				{
					del_str = "RL";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_location = t_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rc)
				{
					del_str = "RC";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_comment = t_str;
					tmp_lines_vector.clear();
					rc = false;
				}
				if(ra)
				{
					del_str = "RA";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,',',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
						reference_authors.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					ra = false;
				}
				rx = true;
				tmp_lines_vector.push_back(tmp_str);
				break;
			case 'C':
				if(rl)
				{
					del_str = "RL";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_location = t_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_group = t_str;
					tmp_lines_vector.clear();
					rg = false;
				}
				if(rt)
				{
					del_str = "RT";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_title = t_str;
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rx)
				{
					del_str = "RX";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
						reference_cross_reference.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					rx = false;
				}
				if(ra)
				{
					del_str = "RA";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,',',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
						reference_authors.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					ra = false;
				}
				rc = true;
				tmp_lines_vector.push_back(tmp_str);
				break;
			default:
				if(ra)
				{
					del_str = "RA";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,',',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
						reference_authors.push_back(tt_str);
					}
					tmp_vector.clear();
					tmp_lines_vector.clear();
					ra = false;
				}
				if(rl)
				{
					del_str = "RL";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_location = t_str;
					tmp_lines_vector.clear();
					rl = false;
				}
				if(rt)
				{
					del_str = "RT";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\"";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_title = t_str;
					tmp_lines_vector.clear();
					rt = false;
				}
				if(rx)
				{
					del_str = "RX";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
					for(int j = 0; j < (int)tmp_vector.size(); j++)
					{
						string tt_str = tmp_vector[j];
						if(tt_str[0] == ' ')
						{
							GAGenomUtilities::trimString(&tt_str);
						}
						else
						{
							GAGenomUtilities::trimString2(&tt_str);
						}
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
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_comment = t_str;
					tmp_lines_vector.clear();
					rc = false;
				}
				if(rg)
				{
					del_str = "RG";
					rep_str = " ";
					string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = "\r";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					del_str = ";";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
					GAGenomUtilities::trimString(&t_str);
					reference_group = t_str;
					tmp_lines_vector.clear();
					rg = false;
				}
				break;
		}
	}
	if(rg)
	{
		del_str = "RG";
		rep_str = " ";
		string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		del_str = "\r";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		del_str = ";";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
		GAGenomUtilities::trimString(&t_str);
		reference_group = t_str;
		tmp_lines_vector.clear();
		rg = false;
	}
	if(rx)
	{
		del_str = "RX";
		rep_str = " ";
		string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		del_str = "\r";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		del_str = ";";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
		GAGenomUtilities::trimString(&t_str);
		tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,' ',false);
		for(int j = 0; j < (int)tmp_vector.size(); j++)
		{
			string tt_str = tmp_vector[j];
			if(tt_str[0] == ' ')
			{
				GAGenomUtilities::trimString(&tt_str);
			}
			else
			{
				GAGenomUtilities::trimString2(&tt_str);
			}
			reference_cross_reference.push_back(tt_str);
		}
		tmp_vector.clear();
		tmp_lines_vector.clear();
		rx = false;
	}
	if(rl)
	{
		del_str = "RL";
		rep_str = " ";
		string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		del_str = "\r";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
		GAGenomUtilities::trimString(&t_str);
		reference_location = t_str;
		tmp_lines_vector.clear();
		rl = false;
	}
	if(rc)
	{
		del_str = "RC";
		rep_str = " ";
		string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		del_str = "\r";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
		GAGenomUtilities::trimString(&t_str);
		reference_comment = t_str;
		tmp_lines_vector.clear();
		rc = false;
	}
	if(rt)
	{
		del_str = "RT";
		rep_str = " ";
		string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,false);
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		del_str = "\r";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		del_str = "\"";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		del_str = ";";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
		GAGenomUtilities::trimString(&t_str);
		reference_title = t_str;
		tmp_lines_vector.clear();
		rt = false;
	}
	if(ra)
	{
		del_str = "RA";
		string t_str = GAGenomUtilities::toOneString(&tmp_lines_vector,true);
		del_str = "\r";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		del_str = ";";
		GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
		GAGenomUtilities::onlyOneDelimerChar(&t_str,' ');
		GAGenomUtilities::trimString(&t_str);
		tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&t_str,',',false);
		for(int j = 0; j < (int)tmp_vector.size(); j++)
		{
			string tt_str = tmp_vector[j];
			if(tt_str[0] == ' ')
			{
				GAGenomUtilities::trimString(&tt_str);
			}
			else
			{
				GAGenomUtilities::trimString2(&tt_str);
			}
			reference_authors.push_back(tt_str);
		}
		tmp_vector.clear();
		tmp_lines_vector.clear();
		ra = false;
	}
	prepared = true;
}
/*
string * gellisary::GAGenomReferenceEmbl::getLocation()
{
	if(!prepared)
	{
		parse();
	}
	return &reference_location;
}
*/
string * gellisary::GAGenomReferenceEmbl::getGroup()
{
	if(!prepared)
	{
		parse();
	}
	return &reference_group;
}
string * gellisary::GAGenomReferenceEmbl::getCrossReferenceAsString()
{
	if(!prepared)
	{
		parse();
	}
	reference_cross_reference_as_string = GAGenomUtilities::toOneString(&reference_cross_reference,true);
	return &reference_cross_reference_as_string;
}
vector<string> * gellisary::GAGenomReferenceEmbl::getCrossReference()
{
	if(!prepared)
	{
		parse();
	}
	return &reference_cross_reference;
}
