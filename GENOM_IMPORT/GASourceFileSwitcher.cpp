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
    for(int i = (file_name_size-1); i >= 0; i--)
    {
        if(file_name[i] == '/')
        {
            for(int j = (i+1); j < file_name_size; j++)
            {
                file_name += file_name[j];
            }
            break;
        }
    }
    if((extension != "embl") && (extension != "gbk") && (extension != "ff") && (extension != "ddbj"))
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
    if(extension == "embl")
    {
    	return EMBL;
    }
    else if(extension == "gbk")
    {
    	return GENBANK;
    }
    else if((extension == "ff") || (extension == "ddbj"))
    {
    	return DDBJ;
    }
    else
    {
    	return UNKNOWN;
    }
}

gellisary::GASourceFileSwitcher::~GASourceFileSwitcher()
{
	
}
