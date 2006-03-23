#ifndef GAFILE_H
#include "GAFile.h"
#endif

gellisary::GAFile::GAFile(GALogger & nLogger, GAARB & nARB, std::ifstream & nARB_File) : logger(nLogger), arb(nARB), arb_file(nARB_File)
{
	
}

//gellisary::GAFile::GAFile(){}

bool gellisary::GAFile::find_word(const std::string & source, const std::string & word_to_find)
{
	int source_size = source.size();
	bool result = false;
	int word_to_find_size = word_to_find.size();
	for(int i = 0, j = 0; i < source_size; i++)
	{
		if(source[i] == word_to_find[j])
		{
			if((j+1) == word_to_find_size)
			{
				result = true;
			}
			else
			{
				j++;
				if(j >= word_to_find_size)
				{
					break;
				}
			}
		}
		else
		{
			j = 0;
			if(source[i] == word_to_find[j])
			{
				j++;
				if(j >= word_to_find_size)
				{
					break;
				}
				
			}
		}
	}
	return result;
}

bool gellisary::GAFile::split_string(const std::string & n_source, std::vector<std::string> & result, const char * delims)
{
	std::string t_word;
	std::string::size_type pos_begin = 0;
	std::string::size_type pos_end;
	std::string::size_type source_size = n_source.size();
	std::string source(trim(n_source));
	if(source_size <= 0)
	{
		return false;
	}
	do
	{
		pos_end = source.find_first_of(delims,pos_begin);
		
		if(pos_end != std::string::npos)
		{
			t_word = source.substr(pos_begin,pos_end);
			result.push_back(t_word);
			if(pos_end == source_size)
			{
				break;
			}
			else
			{
				pos_begin = source.find_first_not_of(delims,pos_end);
				if(pos_begin == std::string::npos)
				{
					break;
				}
			}
		}
		else
		{
			break;
		}
	}
	while(true);
	if(pos_end == std::string::npos && pos_begin < source_size)
	{
		t_word = source.substr(pos_begin);
		result.push_back(t_word);
	}
	if(result.size() > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::string gellisary::GAFile::trim(const std::string & source, const char * delims)
{
	std::string result(source);
	std::string::size_type index = result.find_last_not_of(delims);
	if(index != std::string::npos)
	{
		result.erase(++index);
	}
	index = result.find_first_not_of(delims);
	if(index != std::string::npos)
	{
		result.erase(0, index);
	}
	else
	{
		result.erase();
	}
	return result;
}

/*std::string gellisary::GAFile::trim_begin(const std::string & source, const char * delims)
{
	std::string result(source);
	std::string::size_type index = result.find_first_not_of(delims);
	if(index != std::string::npos)
	{
		result.erase(0, index);
	}
	else
	{
		result.erase();
	}
	return result;
}

std::string gellisary::GAFile::trim_end(const std::string & source, const char * delims)
{
	std::string result(source);
	std::string::size_type index = result.find_last_not_of(delims);
	if(index != std::string::npos)
	{
		result.erase(++index);
	}
	else
	{
		result.erase();
	}
	return result;
}*/

gellisary::GAFile::~GAFile()
{
	
}
