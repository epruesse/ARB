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
	GAGenomUtilities::trimString(&tmp_str);
	gene_type = tmp_str;
	tmp_str = tmp_vector[1];
	GAGenomUtilities::trimString(&tmp_str);
	location_as_string = tmp_str;
	GAGenomGeneLocationDDBJ * new_location;
	new_location = new GAGenomGeneLocationDDBJ();
	new_location->parse(&tmp_str);
	location = *new_location;
	delete(new_location);
	
	for(int i = 2; i < (int) tmp_vector.size(); i++)
	{
		tmp_str = tmp_vector[i];
		GAGenomUtilities::trimString(&tmp_str);
		if(tmp_str[0] == '/')
		{
			if(qual)
			{
				qualifiers[t_str] = "none";
				t_str = tmp_str;
				del_str = "/";
				rep_str = " ";
				GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
				GAGenomUtilities::trimString(&t_str);
			}
			else
			{
				qual = true;
				t_str = tmp_str;
				del_str = "/";
				rep_str = " ";
				GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
				GAGenomUtilities::trimString(&t_str);
			}
		}
		else
		{
			if(!t_str.empty())
			{
				del_str = "\"";
				rep_str = " ";
				GAGenomUtilities::replaceByString(&t_str,&del_str,&rep_str);
				GAGenomUtilities::trimString(&tmp_str);
				qualifiers[t_str] = tmp_str;
			}
		}
	}
	iter = qualifiers.begin();
	prepared = true;
}

GAGenomGeneLocationDDBJ * gellisary::GAGenomGeneDDBJ::getLocation()
{
	return &location;
}
