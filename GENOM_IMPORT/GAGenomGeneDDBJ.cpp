/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#include "GAGenomGeneDDBJ.h"
#include "GAGenomUtilities.h"

using namespace std;
using namespace gellisary;

void gellisary::GAGenomGeneDDBJ::parse()
{
	string tmp_str;
	string del_str;
	string rep_str;
	string source_str;
	source_str = GAGenomUtilities::toOneString(&row_lines,true);
	rep_str = " ";
	vector<string> tmp_vector;
	bool qual = false; 
	string t_str;
	
	del_str = "\r";
	GAGenomUtilities::replaceByString(&source_str,&del_str,&rep_str);
	del_str = "=";
	GAGenomUtilities::replaceByString(&source_str,&del_str,&rep_str);
	GAGenomUtilities::onlyOneDelimerChar(&source_str,' ');
	tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(&source_str,' ',true);
	tmp_str = tmp_vector[0];
	if(tmp_str[0] == ' ')
	{
		GAGenomUtilities::trimString(&tmp_str);
	}
	else
	{
		GAGenomUtilities::trimString2(&tmp_str);
	}
	gene_type = tmp_str;
	tmp_str = tmp_vector[1];
	if(tmp_str[0] == ' ')
	{
		GAGenomUtilities::trimString(&tmp_str);
	}
	else
	{
		GAGenomUtilities::trimString2(&tmp_str);
	}
	location_as_string = tmp_str;
	GAGenomGeneLocationDDBJ * new_location;
	new_location = new GAGenomGeneLocationDDBJ(&tmp_str);
	new_location->parse();
	location = *new_location;
	delete(new_location);
	
	for(int i = 2; i < (int) tmp_vector.size(); i++)
	{
		tmp_str = tmp_vector[i];
		if(t_str[0] == ' ')
		{
			GAGenomUtilities::trimString(&tmp_str);
		}
		else
		{
			GAGenomUtilities::trimString2(&tmp_str);
		}
		if(!tmp_str.empty())
		{
			if(tmp_str[0] == '/')
			{
				if(qual)
				{
					t_str = tmp_str;
					del_str = "/";
					rep_str = " ";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					if(t_str[0] == ' ')
					{
						GAGenomUtilities::trimString(&t_str);
					}
					else
					{
						GAGenomUtilities::trimString2(&t_str);
					}
					qualifiers[t_str] = "yes";
				}
				else
				{
					qual = true;
					t_str = tmp_str;
					del_str = "/";
					rep_str = " ";
					GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
					if(t_str[0] == ' ')
					{
						GAGenomUtilities::trimString(&t_str);
					}
					else
					{
						GAGenomUtilities::trimString2(&t_str);
					}
				}
			}
			else
			{
				if(!t_str.empty())
				{
					del_str = "\"";
					rep_str = " ";
					GAGenomUtilities::replaceByString(&tmp_str,&del_str,&rep_str);
					qualifiers[t_str] = tmp_str;
				}
				qual = false;
			}
		}
	}
	prepared = true;
}

GAGenomGeneLocationDDBJ * gellisary::GAGenomGeneDDBJ::getLocation()
{
	return &location;
}
