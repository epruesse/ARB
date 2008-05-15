#ifndef GASOURCEFILESWITCHER_H
#include "GASourceFileSwitcher.h"
#endif

gellisary::GASourceFileSwitcher::GASourceFileSwitcher(const char * nFile_name)
{
	file_name = nFile_name;
}

int gellisary::GASourceFileSwitcher::make_a_decision()
{
	int file_name_size = file_name.size();
	
	for(int i = (file_name_size-1); i >= 0; i--)
    {
        if(file_name[i] == '.')
        {
            for(int j = (i+1); j < file_name_size; j++)
            {
                extension += file_name[j];
            }
        }
    }
    if((extension != "embl") && (extension != "gbk") && (extension != "dat") && (extension != "ff") && (extension != "ddbj"))
    {
        flat_file.open(file_name.c_str());
        char tmp_line[128];
        flat_file.getline(tmp_line,128);
        std::string t_string(tmp_line);
        if(t_string.size() > 0)
        {
            if((t_string[0] == 'I') && (t_string[1] == 'D') && (t_string[2] == ' '))
            {
                extension = "embl";
            }
            else if((t_string[0] == 'L') && (t_string[1] == 'O') && (t_string[2] == 'C') && (t_string[3] == 'U') && (t_string[4] == 'S') && (t_string[5] == ' '))
            {
                extension = "gbk";
            }
        }
        flat_file.close();
    }
    if(extension == "embl" || extension == "dat")
    {
    	return EMBL;
    }
    else if(extension == "gbk")
    {
    	return GENBANK;
    }
    else if((extension == "ff") || (extension == "ddbj"))
    {
    	if(containsHeader())
    	{
	    	return DDBJ;
    	}
    	else
    	{
    		return DDBJ_WITHOUT_HEADER;
    	}
    }
    else
    {
    	return UNKNOWN;
    }
}

bool gellisary::GASourceFileSwitcher::containsHeader()
{
	
	flat_file.open(file_name.c_str());
	char buffer[100];
	while(!flat_file.eof())
	{
		flat_file.getline(buffer,100);
		std::string t_line(buffer);
		std::string::size_type pos_first = t_line.find_first_of(" ");
		if(pos_first != std::string::npos)
		{
			std::string key_string = t_line.substr(0,pos_first);
			if(key_string == "ACCESSION" || key_string == "accession")
			{
				pos_first += 3;
				std::string value_string = t_line.substr(pos_first);
				std::string::size_type pos_second = value_string.find_first_of(" ");
				if(pos_second != std::string::npos)
				{
					flat_file.close();
					return false;
				}
				else
				{
					flat_file.close();
					return true;
				}
				break;
			}
			else
			{
				continue;
			} 
		}
	}
	if(flat_file.is_open())
	{
		flat_file.close();
	}
	return false;
}

gellisary::GASourceFileSwitcher::~GASourceFileSwitcher()
{
	
}
