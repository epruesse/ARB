#include "GAGenomUtilities.h"
#include <cstdio>

// #ifndef _CPP_CMATH
// #include <cmath>
// #endif

using namespace std;
using namespace gellisary;

void gellisary::GAGenomUtilities::replaceByString(string * source_str, string * delimer_str, string * replacing_str)
{
	int delimer_str_pos = 0;
	string target_str;
	for(int i = 0; i < (int) source_str->size(); i++)
	{
		if((source_str->c_str())[i] == (delimer_str->c_str())[delimer_str_pos++])
		{
			if(delimer_str_pos == (int) delimer_str->size())
			{
				delimer_str_pos = 0;
				for(int j = 0; j < (int) replacing_str->size(); j++)
				{
					target_str += (replacing_str->c_str())[j];
				}
			}
		}
		else
		{
			target_str += (source_str->c_str())[i];
		}
	}
	*source_str = target_str;
}

void gellisary::GAGenomUtilities::preparePropertyString(string * source_str, string * delimer_str, string * target_str)
{
	vector<string> tmp_vector;
	GAGenomUtilities::replaceByWhiteSpaceCleanly(source_str,delimer_str);
	tmp_vector = GAGenomUtilities::findAndSeparateWordsByChar(source_str,' ',true);
	*target_str = GAGenomUtilities::toOneString(&tmp_vector,true);
}

void gellisary::GAGenomUtilities::replaceByWhiteSpaceCleanly(string * source_str, string * delimer_str)
{
	string del_str = "\r";
	string rep_str = " ";
	GAGenomUtilities::replaceByString(source_str,delimer_str,&rep_str);
	GAGenomUtilities::replaceByString(source_str,&del_str,&rep_str);
	GAGenomUtilities::onlyOneDelimerChar(source_str,' ');
}

/*
 * Wenn die Methode oben 'replaceByString()' einwandfrei funktioniert, dann
 * kann die Methode 'eliminateFeatureTableSignInEmbl()' getroest geloescht werden.
 */
void gellisary::GAGenomUtilities::eliminateFeatureTableSignInEmbl(string * source_str, string * delimer_str)
{
	char char_source_str[source_str->size()+1];
	strcpy(char_source_str,source_str->c_str());
	char char_delimer_str[delimer_str->size()+1];
	strcpy(char_delimer_str,delimer_str->c_str());
	bool feature = true;
	bool first = true;
	for(int i = 0; i < (int)strlen(char_source_str); i++)
	{
		if(char_source_str[i] == (delimer_str->c_str())[0])
		{
			if(first)
			{
				feature = true;
				first = false;
			}
			else
			{
				if(feature)
				{
					char_source_str[i] = ' ';
					char_source_str[i-1] = ' ';
					first = true;
					feature = false;
				}
			}
		}
		else if(char_source_str[i] == (delimer_str->c_str())[1])
		{
			if(feature)
			{
				char_source_str[i] = ' ';
				char_source_str[i-1] = ' ';
				feature = false;
			}
		}
		else
		{
			feature = false;
			first = true;
		}
	}
	*source_str = char_source_str;
}

void gellisary::GAGenomUtilities::eliminateSign(std::string * source_str, char delimer_char)
{
	char char_source_str[source_str->size()+1];
	strcpy(char_source_str,source_str->c_str());
	for(int i = 0; i < (int)strlen(char_source_str); i++)
	{
		if(char_source_str[i] == delimer_char)
		{
			char_source_str[i] = ' ';
		}
	}
	*source_str = char_source_str;
}

string gellisary::GAGenomUtilities::toOneString(vector<string> * source_vector, bool withSpace)
{
	string target_str;
	for(int i = 0; i < (int) source_vector->size(); i++)
	{
		if(i != 0)
		{
			if(withSpace)
			{
				target_str += " ";
			}
		}
		target_str += source_vector->operator [] (i);
	}
	return target_str;
}

void gellisary::GAGenomUtilities::onlyOneDelimerChar(string * source_str, char delimer_char)
{
	bool current_stat = false;
	string target;
	for(int i = 0; i < (int) source_str->size(); i++)
	{
		if((source_str->c_str())[i] == delimer_char)
		{
			if(i == 0)
			{
				current_stat = true;
			}
			else
			{
				if(!current_stat)
				{
					current_stat = true;
				}
			}
		}
		else
		{
			if(current_stat)
			{
				current_stat = false;
				target += ' ';
				target += (source_str->c_str())[i];
			}
			else
			{
				target += (source_str->c_str())[i];
			}
		}
	}
	*source_str = target;
}

vector<string> gellisary::GAGenomUtilities::findAndSeparateWordsByChar(string * source_str, char sep_char, bool withInnerString)
{
	GAGenomUtilities::onlyOneDelimerChar(source_str,sep_char);
	vector<string> target_vector;
	bool inInnerString = false;
	string tmp_str;
	for(int i = 0; i < (int) source_str->size(); i++)
	{
		if(withInnerString)
		{
			if(inInnerString)
			{
				if((source_str->c_str())[i] == '"')
				{
					inInnerString = false;
					target_vector.push_back(tmp_str);
					tmp_str = "";
				}
				else
				{
					tmp_str += (source_str->c_str())[i];
				}
			}
			else
			{
				if((source_str->c_str())[i] == '"')
				{
					inInnerString = true;
					if(i > 0)
					{
						target_vector.push_back(tmp_str);
						tmp_str = "";
					}
				}
				else
				{
					if((source_str->c_str())[i] == sep_char)
					{
						if(i > 0)
						{
							target_vector.push_back(tmp_str);
							tmp_str = "";
						}
					}
					else
					{
						tmp_str += (source_str->c_str())[i];
					}
				}
			}
		}
		else
		{
			if((source_str->c_str())[i] == sep_char)
			{
				if(i > 0)
				{
					target_vector.push_back(tmp_str);
					tmp_str = "";
				}
			}
			else
			{
				tmp_str += (source_str->c_str())[i];
			}
		}
	}
	if(((int) tmp_str.size()) > 0)
	{
		target_vector.push_back(tmp_str);
	}
	return target_vector;
}

vector<string> gellisary::GAGenomUtilities::findAndSeparateWordsByString(string * source_str, string * sep_str, bool withInnerString)
{
	vector<string> target_vector;
	if(withInnerString)
	{
		bool isBegin = false;
		bool isEnd = false;
		vector<string> tmp_target_str;
		vector<string> tmp_target_str2;
		GAGenomUtilities::trimString(source_str);
		if((source_str->c_str())[0] == '"')
		{
			isBegin = true;
		}
		if((source_str->c_str())[((int) source_str->size())-1] == '"')
		{
			isEnd = true;
		}
		tmp_target_str = findAndSeparateWordsByChar(source_str,'"',false);
		if(tmp_target_str.size() > 0)
		{
			if((tmp_target_str.size() % 2) == 0)	//	gerade
			{
				if(isBegin && !isEnd)
				{
					for(int i = 0; i < (int) tmp_target_str.size(); i++)
					{
						if((i % 2) == 0)
						{
							target_vector.push_back(tmp_target_str[i]);
						}
						else
						{
							string tmp_str = tmp_target_str[i];
							tmp_target_str2 = GAGenomUtilities::findAndSeparateWordsByString(&tmp_str,sep_str,false);
							for(int j = 0; j < (int) tmp_target_str2.size(); j++)
							{
								target_vector.push_back(tmp_target_str2[j]);
							}
							tmp_target_str2.clear();
						}
					}
				}
				else if(!isBegin && isEnd)
				{
					for(int i = 0; i < (int) tmp_target_str.size(); i++)
					{
						if((i % 2) == 1)
						{
							target_vector.push_back(tmp_target_str[i]);
						}
						else
						{
							string tmp_str = tmp_target_str[i];
							tmp_target_str2 = GAGenomUtilities::findAndSeparateWordsByString(&tmp_str,sep_str,false);
							for(int j = 0; j < (int) tmp_target_str2.size(); j++)
							{
								target_vector.push_back(tmp_target_str2[j]);
							}
							tmp_target_str2.clear();
						}
					}
				}
			}
			else	//	ungerade
			{
				if(isBegin && isEnd)
				{
					for(int i = 0; i < (int) tmp_target_str.size(); i++)
					{
						if((i % 2) == 0)
						{
							target_vector.push_back(tmp_target_str[i]);
						}
						else
						{
							string tmp_str = tmp_target_str[i];
							tmp_target_str2 = GAGenomUtilities::findAndSeparateWordsByString(&tmp_str,sep_str,false);
							for(int j = 0; j < (int) tmp_target_str2.size(); j++)
							{
								target_vector.push_back(tmp_target_str2[j]);
							}
							tmp_target_str2.clear();
						}
					}
				}
				else if(!isBegin && !isEnd)
				{
					if(((int) tmp_target_str.size()) > 1)
					{
						for(int i = 0; i < (int) tmp_target_str.size(); i++)
						{
							if((i % 2) == 1)
							{
								target_vector.push_back(tmp_target_str[i]);
							}
							else
							{
								string tmp_str = tmp_target_str[i];
								tmp_target_str2 = GAGenomUtilities::findAndSeparateWordsByString(&tmp_str,sep_str,false);
								for(int j = 0; j < (int) tmp_target_str2.size(); j++)
								{
									target_vector.push_back(tmp_target_str2[j]);
								}
								tmp_target_str2.clear();
							}
						}
					}
					else
					{
						target_vector.push_back(tmp_target_str[0]);
					}
				}
			}
		}
	}
	else
	{
		string tmp_rep_str("\r");
		GAGenomUtilities::replaceByString(source_str,sep_str,&tmp_rep_str);
		target_vector = findAndSeparateWordsByChar(source_str,'\r',false);
	}
	return target_vector;
}

void gellisary::GAGenomUtilities::trimString(string * source_str)
{
	GAGenomUtilities::trimStringByChar(source_str, ' ');
}

void gellisary::GAGenomUtilities::trimStringByChar(string * source_str, char trim_char)
{
	string target;
	int begin = -1;
	int end = -1;
	for(int i = 0; i < (int) source_str->size(); i++)
	{
		if((source_str->c_str())[i] == trim_char)
		{
			begin = i;
		}
		else
		{
			break;
		}
	}
	for(int j = (((int) source_str->size())-1); j >= 0; j--)
	{
		if((source_str->c_str())[j] == trim_char)
		{
			end = j;
		}
		else
		{
			break;
		}
	}
	if(begin == -1)
	{
		begin = 0;
	}
	if(end == -1)
	{
		end = (int) source_str->size();
	}
	for(int k = (begin+1); k < end; k++)
	{
		target.push_back((source_str->c_str())[k]);
	}
	*source_str = target;
}

int gellisary::GAGenomUtilities::stringToInteger(string * source_str)
{
    return atoi(source_str->c_str());

// 	int target_int = 0;
// 	int tmp_int = 0;
// 	for(int i = (((int) source_str->size()) - 1); i >= 0; i--)
// 	{
// 		switch((source_str->c_str())[i])
// 		{
// 			case '0':
// 				tmp_int = 0;
// 				break;
// 			case '1':
// 				tmp_int = 1;
// 				break;
// 			case '2':
// 				tmp_int = 2;
// 				break;
// 			case '3':
// 				tmp_int = 3;
// 				break;
// 			case '4':
// 				tmp_int = 4;
// 				break;
// 			case '5':
// 				tmp_int = 5;
// 				break;
// 			case '6':
// 				tmp_int = 6;
// 				break;
// 			case '7':
// 				tmp_int = 7;
// 				break;
// 			case '8':
// 				tmp_int = 8;
// 				break;
// 			case '9':
// 				tmp_int = 9;
// 				break;
// 			default:
// 				break;
// 		}
// 		target_int += (tmp_int * (int)pow((double)10,i));
// 	}
// 	return target_int;
}

string gellisary::GAGenomUtilities::integerToString(int source_int)
{
    char buffer[50];
    sprintf(buffer, "%i", source_int);
    return string(buffer);

// 	string target_str;
// 	bool before = false;
// 	long source_double = (long) source_int;	// nicht unbedingt notwendig trotzdem ...
// 	for(long i = 9; i >= 0; i++)
// 	{
//             if((source_double >= pow(10,i)) && (source_double < pow((double)10,(i+1))))
//   		{
// 		    target_str += (char)(((int)(source_double / pow((double)10,i)))+48);
// 		    source_double = source_double % ((long)pow((double)10,i));
// 		    before = true;
// 		}
// 		else
// 		{
// 			if(before)
// 			{
// 				target_str += '0';
// 			}
// 		}
// 	}
// 	return target_str;
}

string gellisary::GAGenomUtilities::generateGeneID(string * source_str, int gene_type)
{
	string target_str("a");
	bool next = true;
	int pointer = 0;
	bool point = false;
	while(next)
	{
		switch((source_str->c_str())[pointer])
		{
			case 'c':
				if(point)
				{
					target_str.append("d");
					point = false;
				}
				target_str.append("c");
				pointer = pointer + 10;
				break;
			case 'o':
				if(point)
				{
					target_str.append("d");
					point = false;
				}
				target_str.append("o");
				pointer = pointer + 5;
				break;
			case 'j':
				if(point)
				{
					target_str.append("d");
					point = false;
				}
				target_str.append("j");
				pointer = pointer + 4;
				break;
			case ',':
				if(point)
				{
					target_str.append("d");
					point = false;
				}
				target_str.append("h");
				pointer = pointer + 1;
				break;
			case '^':
				if(point)
				{
					target_str.append("d");
					point = false;
				}
				target_str.append("e");
				pointer = pointer + 1;
				break;
			case '<':
				if(point)
				{
					target_str.append("d");
					point = false;
				}
				target_str.append("f");
				pointer = pointer + 1;
				break;
			case '>':
				if(point)
				{
					target_str.append("d");
					point = false;
				}
				target_str.append("g");
				pointer = pointer + 1;
				break;
			case '.':
				if(point)
				{
					target_str.append("d");
					point = false;
				}
				else
				{
					point = true;
				}
				pointer = pointer + 1;
				break;
			case '(': case ')':
				if(point)
				{
					target_str.append("d");
					point = false;
				}
				pointer = pointer + 1;
				break;
			default:
				if(point)
				{
					target_str.append("d");
					point = false;
				}
				target_str += ((source_str->c_str())[pointer]);
				pointer = pointer + 1;
				break;
		}
		if(pointer == (int) source_str->size())
		{
			next = false;
		}
	}
	target_str.append("y");
	target_str.append(GAGenomUtilities::integerToString(gene_type));
	target_str.append("z");
	return target_str;
}

vector<int> gellisary::GAGenomUtilities::parseSourceLocation(string * source_str)
{
	vector<int> target_vector;
	GAGenomUtilities::trimString(source_str);
	bool point = false;
	string tmp_str;
	for(int i = 0; i < (int) source_str->size(); i++)
	{
		if((source_str->c_str())[i] == '.')
		{
			if(!point)
			{
				point = true;
				if(tmp_str.size() > 0)
				{
					target_vector.push_back(GAGenomUtilities::stringToInteger(&tmp_str));
					tmp_str = "";
				}
			}
		}
		else
		{
			tmp_str.push_back((source_str->c_str())[i]);
		}
	}
	if(tmp_str.size() > 0)
	{
		target_vector.push_back(GAGenomUtilities::stringToInteger(&tmp_str));
		tmp_str = "";
	}
	return target_vector;
}

bool gellisary::GAGenomUtilities::isNewGene(string * source_str)
{
	string tmp_str = source_str->substr(3,20);
	GAGenomUtilities::trimString(&tmp_str);
	if(tmp_str[0] == ' ')
	{
		if(tmp_str[1] == ' ')
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		return true;
	}
}

bool gellisary::GAGenomUtilities::isSource(string * source_str)
{
	string tmp_str = source_str->substr(3,20);
	GAGenomUtilities::trimString(&tmp_str);
	if(tmp_str.find("source") != string::npos)
	{
		return true;
	}
	else
	{
		return false;
	}
}
