/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#include "GAGenomFeatureTableSourceGenBank.h"
#include "GAGenomUtilities.h"

using namespace std;
using namespace gellisary;

void gellisary::GAGenomFeatureTableSourceGenBank::parse()
{
	string tmp_str;
	string del_str;
	string rep_str;
	vector<string> tmp_vector;
	tmp_str = GAGenomUtilities::toOneString(&row_lines,true);
	del_str = "\r";
	rep_str = " ";
	GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
	del_str = "=";
	GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
	GAGenomUtilities::trimString(&tmp_str);
	GAGenomUtilities::onlyOneDelimerChar(&tmp_str,' ');
	tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&tmp_str,' ',true);
	string t_str;
	int i = 0;
	for(i = 0; i < (int) tmp_vector.size(); i++)
	{
		t_str = tmp_vector[i];
		GAGenomUtilities::trimString(&t_str);
		if(t_str == "source")
		{
			t_str = tmp_vector[i+1];
			vector<int> locations = GAGenomUtilities::parseSourceLocation(&t_str);
			if(locations.size() > 0)
			{
				source_begin = locations[0];
				source_end = locations[1];
			}
			break;
		}
	}
	string tt_str;
	bool qual = false;
	for(i+=2; i < (int) tmp_vector.size(); i++)
	{
		t_str = tmp_vector[i];
		GAGenomUtilities::trimString(&t_str);
		if(t_str[0] == '/')
		{
			if(qual)
			{
				qualifiers[tt_str] = "none";
				tt_str = t_str;
				del_str = "/";
				rep_str = " ";
				GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
				GAGenomUtilities::trimString(&tt_str);
			}
			else
			{
				qual = true;
				tt_str = t_str;
				del_str = "/";
				rep_str = " ";
				GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
				GAGenomUtilities::trimString(&tt_str);
			}
		}
		else
		{
			if(!tt_str.empty())
			{
				del_str = "\"";
				rep_str = " ";
				GAGenomUtilities::replaceByString(&tt_str,&del_str,&rep_str);
				GAGenomUtilities::trimString(&t_str);
				qualifiers[tt_str] = t_str;
			}
		}
	}
	prepared = true;
}
