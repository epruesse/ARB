#ifndef GALOGGER_H
#define GALOGGER_H

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _GLIBCXX_FSTREAM
#include <fstream>
#endif

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

#ifndef _GLIBCXX_CTIME
#include <ctime>
#endif

#ifndef _GLIBCXX_SSTREAM
#include <sstream>
#endif

namespace gellisary
{
	class GALogger
	{
		private:
			std::vector<std::string> entries;
			std::ofstream log_file;
			
		public:
			GALogger();
			virtual ~GALogger();
			void openLogFile();
			void closeLogFile();
			void add_log_entry(std::string, int, int);
			bool hasLogEntries();
			bool isLogFileOpen();
	};

};

#endif /*GALOGGER_H*/
