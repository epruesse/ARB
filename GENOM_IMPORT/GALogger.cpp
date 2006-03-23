#ifndef GALOGGER_H
#include "GALogger.h"
#endif

gellisary::GALogger::GALogger()
{
	std::string user_home = std::getenv("HOME");
	time_t secs = std::time(NULL);
	std::stringstream t_name;
	t_name << user_home << "/" << "gi_" << secs << ".ga";
	log_file.open(t_name.str().c_str());
}

void gellisary::GALogger::openLogFile()
{
	time_t secs = std::time(NULL);
	std::string user_home = std::getenv("HOME");
	std::stringstream t_name;
	t_name << user_home << "/" << "gi_" << secs << ".ga";
	log_file.open(t_name.str().c_str());
}

void gellisary::GALogger::closeLogFile()
{
	if(entries.size() > 0)
	{
		for(int i = 0; i < (int) entries.size(); i++)
		{
			log_file << entries[i] << std::endl;
		}
	}
	log_file.close();
}

void gellisary::GALogger::add_log_entry(std::string entry, int line_number, int char_number)
{
	std::stringstream t_entry;
	t_entry << "line : " << line_number << ":" << char_number << " # " << entry;
	entries.push_back(t_entry.str().c_str());
	//GB_warning(t_entry.str().c_str());
}

bool gellisary::GALogger::hasLogEntries()
{
	if(entries.size() > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool gellisary::GALogger::isLogFileOpen()
{
	return log_file.is_open();
}


gellisary::GALogger::~GALogger()
{
	log_file.close();
}
