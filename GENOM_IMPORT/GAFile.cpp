#ifndef GAFILE_H
#include "GAFile.h"
#endif

#if defined(DEBUG)
gellisary::GAFile::GAFile(GALogger & nLogger, GAARB & nARB, std::string & nARB_Filename) : logger(nLogger), arb(nARB)
{
	flatfile_fullname = nARB_Filename;
	arb_file.open(flatfile_fullname.c_str());
	std::string::size_type pos_last = flatfile_fullname.find_last_of("/");
	std::string::size_type pos_point = flatfile_fullname.find_last_of(".");
	if(pos_last != std::string::npos && pos_point != std::string::npos)
	{
		flatfile_basename = flatfile_fullname.substr(0,pos_last);
		flatfile_name = flatfile_fullname.substr(++pos_last,(pos_point-pos_last-1));
	}
	else if(pos_last != std::string::npos)
	{
		flatfile_basename = flatfile_fullname.substr(0,pos_last);
		flatfile_name = flatfile_fullname.substr(++pos_last);
	}
	else
	{
		flatfile_name = "";
		flatfile_basename = "";
	}
}
#else
gellisary::GAFile::GAFile(GAARB & nARB, std::string & nARB_Filename) : arb(nARB)
{
	flatfile_fullname = nARB_Filename;
	arb_file.open(flatfile_fullname.c_str());
	std::string::size_type pos_last = flatfile_fullname.find_last_of("/");
	std::string::size_type pos_point = flatfile_fullname.find_last_of(".");
	if(pos_last != std::string::npos && pos_point != std::string::npos)
	{
		flatfile_basename = flatfile_fullname.substr(0,pos_last);
		flatfile_name = flatfile_fullname.substr(++pos_last,(pos_point-pos_last-1));
	}
	else if(pos_last != std::string::npos)
	{
		flatfile_basename = flatfile_fullname.substr(0,pos_last);
		flatfile_name = flatfile_fullname.substr(++pos_last);
	}
	else
	{
		flatfile_name = "";
		flatfile_basename = "";
	}
}
#endif

//gellisary::GAFile::GAFile(){}

std::string gellisary::GAFile::generateGeneID(const std::string & location, const std::string & feature_type, const std::string & product, const std::string & gene)
{
	std::string result;
    bool next = true;
    int pointer = 0;
    std::ostringstream string_out_1;
    std::ostringstream string_out_2;
    std::string str_1;
    std::string str_2;
    bool drin = false;
    string_out_1 << feature_type;
    string_out_1 << '_';
    int i = 0;
    
    std::string::size_type product_size = product.size();
    std::string product_prepared;
    for(int j = 0; j < product_size; j++)
    {
    	if(product[j] == ' ')
    	{
    		product_prepared.push_back('_');
    	}
    	else
    	{
    		product_prepared.push_back(product[j]);
    	}
    }
    while(next)
    {
        i = location[pointer++];
        if(i >= 48 && i <= 57)
        {
		    if(!drin)
		    {
		    	drin = true;
		    }
		    string_out_2 << (char)i;
        } 
        else if(drin)
        {
        	drin = false;
        	break;
        }
        if(pointer == (int) location.size())
        {
            break;
        }
    }
    str_1 = string_out_2.str();
    str_2 = string_out_1.str();
    
  	int rest = 29 - (int)str_2.size() - (int) str_1.size();
   	if(product_prepared != "nix")
   	{
   		if(rest < (int)product_prepared.size())
	   	{
	   		product_prepared.resize(rest);
	   		string_out_1 << product_prepared;
		    string_out_1 << '_';
	   	}
	   	else
	   	{
	   		string_out_1 << product_prepared;
		    string_out_1 << '_';
		}
   	}

   	string_out_1 << str_1;
    result = string_out_1.str();
    return result;
}

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
