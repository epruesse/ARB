/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#include "GAGenomGeneLocationDDBJ.h"
#include "GAGenomUtilities.h"

using namespace std;
using namespace gellisary;

gellisary::GAGenomGeneLocationDDBJ::GAGenomGeneLocationDDBJ(string * new_location_string):GAGenomGeneLocation(new_location_string)
{
	pointer = false;
	current_value = -1;
}

/*
 * Momntan belasse ich diese Methode so wie sie ist, ich habe sie fuer die Version 1 
 * der Input-Routine entwickelt. Sie funktioniert einwandfrei. Spaeter zum Ende des Projekts werde ich sie
 * 'optimieren'.
 */
void gellisary::GAGenomGeneLocationDDBJ::parse()
{
	int ls = location_as_string.size();
	int my_stack[ls];
	int levels[ls];
	int level_pointer = -1;
	int stack_pointer = -1;
	char t;
	string t2_str;
	int count_levels;
	int char_pointer = -1;
	int end_pointer = -1;
	int ref_pointer = -1;
	bool fore = true;
	bool until = false;
	bool has_sparator = false;
	bool has_ref = false;
	bool has_ref_atall = false;
	bool has_sep_atall = false;
	bool has_bs_atall = false;
	bool has_comma = false;
	int commas[location_as_string.size()];
	commas[0] = 0;
	vector<string> tvector;
	string substring;

	for(int i = 0; i < ls; i++)
	{
		t = location_as_string[i];
		switch(t)
		{
			case '(':
				my_stack[++stack_pointer] = i;
				break;
			case ')':
				levels[++level_pointer] = my_stack[stack_pointer--];
				levels[++level_pointer] = i;
				break;
			case '.': case '^':
				has_sparator = true;
				has_sep_atall = true;
				break;
			case ':':
				has_ref_atall = true;
				break;
			case '<': case '>':
				has_bs_atall = true;
				break;
		}
	}
	count_levels = (level_pointer+1) / 2;
	for(int h = 0; h < ls; h++)
	{
		t = location_as_string[h];
		if(t == ',')
		{
			if(count_levels > 0)
			{
				if((h < levels[(count_levels-1)*2]) || (h > levels[(count_levels-1)*2+1]))
				{
					has_comma = true;
					commas[++commas[0]] = h;
				}
			}
			else
			{
				has_comma = true;
				commas[++commas[0]] = h;
			}
		}
	}
  
	GAGenomGeneLocationDDBJ * tmp_location;

	if(has_comma)
	{
		tvector = getParts(&location_as_string,commas);
		single_value = false;
		collection = true;
		for(int k = 0; k < (int)tvector.size(); k++)
		{
			t2_str = tvector[k];
			tmp_location = new GAGenomGeneLocationDDBJ(&t2_str);
			tmp_location->parse();
			locations.push_back(*tmp_location);
			delete(tmp_location);
		}
		tvector.clear();
	}
	else
	{
		char_pointer = 0;
		end_pointer = ls-1;
		int current_level = -1;
		while(fore)
		{
			t = location_as_string[char_pointer];
			if(count_levels > 0)
			{
				if((char_pointer == levels[1]) && (t == ')'))
				{
					fore = false;
				}
			}
			switch(t)
			{
			case 'o':
				order = true;
				single_value = false;
				char_pointer += 5;
				current_level = -1;
				for(int j = 0; j < count_levels; j++)
				{
					if(levels[j*2] == char_pointer)
					{
						substring = location_as_string.substr((char_pointer+1),(levels[j*2+1]-char_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						current_level = j;
					}
				}
				fore = false;
				break;
			case 'j':
				join = true;
				single_value = false;
				char_pointer += 4;
				for(int l = 0; l < count_levels; l++)
				{
					if(levels[l*2] == char_pointer)
					{
						substring = location_as_string.substr((char_pointer+1),(levels[l*2+1]-char_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
					}
				}
				fore = false;
				break;
			case 'c':
				complement = true;
				single_value = false;
				char_pointer += 10;
				
				for(int k = 0; k < count_levels; k++)
				{
					if(levels[k*2] == char_pointer)
					{
						substring = location_as_string.substr((char_pointer+1),(levels[k*2+1]-char_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
					}
				}
				
				fore = false;
				break;
			case '.':
				if(until)
				{
					if(!has_ref)
					{
						range = true;
						single_value = false;
						substring = location_as_string.substr(0,(char_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						substring = location_as_string.substr((char_pointer+1),(ls-char_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						fore = false;
					}
					else
					{
						pointer = true;
						range = true;
						single_value = false;
						substring = location_as_string.substr(0,ref_pointer);
						reference = substring;
						substring = location_as_string.substr((ref_pointer+1),(char_pointer-ref_pointer-2));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						substring = location_as_string.substr((char_pointer+1),(ls-char_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						has_ref = false;
						fore = false;
					}
					until = false;
				}
				else
				{
					until = true;
				}
				char_pointer++;
				break;
			case '^':
				if(!has_ref)
				{
					roof = true;
					single_value = false;
					substring = location_as_string.substr(0,char_pointer);
					tmp_location = new GAGenomGeneLocationDDBJ(&substring);
					tmp_location->parse();
					locations.push_back(*tmp_location);
					delete(tmp_location);
					substring = location_as_string.substr((char_pointer+1),(ls-char_pointer-1));
					tmp_location = new GAGenomGeneLocationDDBJ(&substring);
					tmp_location->parse();
					locations.push_back(*tmp_location);
					delete(tmp_location);
					fore = false;
				}
				else
				{
					pointer = true;
					roof = true;
					single_value = false;
					substring = location_as_string.substr(0,ref_pointer);
					reference = substring;
					substring = location_as_string.substr((ref_pointer+1),(char_pointer-ref_pointer-1));
					tmp_location = new GAGenomGeneLocationDDBJ(&substring);
					tmp_location->parse();
					locations.push_back(*tmp_location);
					delete(tmp_location);
					substring = location_as_string.substr((char_pointer+1),(ls-char_pointer-1));
					tmp_location = new GAGenomGeneLocationDDBJ(&substring);
					tmp_location->parse();
					locations.push_back(*tmp_location);
					delete(tmp_location);
					has_ref = false;
					fore = false;
				}
				char_pointer++;
				break;
			case '<':
				if(!has_sparator)
				{
					if(until)
					{
						point = true;
						substring = location_as_string.substr(0,(char_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						substring = location_as_string.substr(char_pointer,(ls-char_pointer));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						until = false;
						fore = false;
					}
					else
					{
						if(char_pointer == 0)
						{
							smaller_begin = true;
							substring = location_as_string.substr((char_pointer+1),(ls-char_pointer-1));
							value = GAGenomUtilities::stringToInteger(&substring);
							fore = false;
						}
						else if(char_pointer == end_pointer)
						{
							smaller_end = true;
							substring = location_as_string.substr(0,char_pointer);
							value = GAGenomUtilities::stringToInteger(&substring);
							fore = false;
						}
					}
				}
				char_pointer++;
				break;
			case '>':
				if(!has_sparator)
				{
					if(until)
					{
						point = true;
						substring = location_as_string.substr(0,(char_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						substring = location_as_string.substr(char_pointer,(ls-char_pointer));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						until = false;
						fore = false;
					}
					else
					{
						if(char_pointer == 0)
						{
							bigger_begin = true;
							substring = location_as_string.substr((char_pointer+1),(ls-char_pointer-1));
							value = GAGenomUtilities::stringToInteger(&substring);
							fore = false;
						}
						else if(char_pointer == end_pointer)
						{
							bigger_end = true;
							substring = location_as_string.substr(0,char_pointer);
							value = GAGenomUtilities::stringToInteger(&substring);
							fore = false;
						}
					}
				}
				char_pointer++;
				break;
			case ',':
				char_pointer++;
				break;
			case ':':
				has_ref = true;
				ref_pointer = char_pointer;
				char_pointer++;
				break;
			default:
				if(until)
				{
					if(!has_ref)
					{
						point = true;
						value = false;
						single_value = false;
						substring = location_as_string.substr(0,(char_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						substring = location_as_string.substr(char_pointer,(ls-char_pointer));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						fore = false;
					}
					else
					{
						point = true;
						value = false;
						single_value = false;
						pointer = true;
						substring = location_as_string.substr(0,ref_pointer);
						reference = substring;
						substring = location_as_string.substr((ref_pointer+1),(char_pointer-ref_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						substring = location_as_string.substr((char_pointer+1),(ls-char_pointer-1));
						tmp_location = new GAGenomGeneLocationDDBJ(&substring);
						tmp_location->parse();
						locations.push_back(*tmp_location);
						delete(tmp_location);
						has_ref = false;
						fore = false;
					}
					until = false;
				}
				if(!has_ref_atall && !has_sep_atall && !has_bs_atall)
				{
					single_value = true;
					substring = location_as_string.substr(char_pointer,ls);
					value = GAGenomUtilities::stringToInteger(&substring);
					fore = false;
				}
				char_pointer++;
				break;
			}
		}
	}
	prepared = true;
}

bool gellisary::GAGenomGeneLocationDDBJ::isReference()
{
	if(!prepared)
	{
		parse();
	}
	return pointer;
}

void gellisary::GAGenomGeneLocationDDBJ::setReference(string * new_reference)
{
	reference = *new_reference;
}

string * gellisary::GAGenomGeneLocationDDBJ::getReference()
{
	if(!prepared)
	{
		parse();
	}
	return &reference;
}

bool gellisary::GAGenomGeneLocationDDBJ::hasMoreValues()
{
	if(!prepared)
	{
		parse();
	}
	if(current_value < (int) locations.size())
	{
		return true;
	}
	else
	{
		return false;
	}
}

vector<string> gellisary::GAGenomGeneLocationDDBJ::getParts(string * beginning, int * indecies)
{
	vector<string> tmp_vector;
	string tmp_str;
	int beginning_size = int (beginning->size());
	int commas[beginning_size];
	int begin = 0;
	for(int i = 0; i < indecies[0]; i++)
	{
		commas[i] = indecies[i+1];
	}
	if(indecies[0] > 0)
	{
		begin = 0;
		for(int i = 0; i < indecies[0]; i++)
		{
			tmp_str = beginning->substr(begin,(commas[i]-begin));
			begin = commas[i]+1;
			tmp_vector.push_back(tmp_str);
		}
		tmp_str = beginning->substr(begin,(beginning_size-1));
		tmp_vector.push_back(tmp_str);
	}
	return tmp_vector;
}

vector<GAGenomGeneLocationDDBJ> * gellisary::GAGenomGeneLocationDDBJ::getLocations()
{
	if(!prepared)
	{
		parse();
	}
	return &locations;
}

/*
GAGenomGeneLocationDDBJ * gellisary::GAGenomGeneLocationDDBJ::getNextValue()
{
	if(!prepared)
	{
		parse();
	}
	tmp_loc = locations[current_value++];
	return &tmp_loc;
}*/

void gellisary::GAGenomGeneLocationDDBJ::setValue(GAGenomGeneLocationDDBJ * new_value)
{
	locations.push_back(*new_value);
}
