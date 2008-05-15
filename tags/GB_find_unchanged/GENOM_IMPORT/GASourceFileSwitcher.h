#ifndef GASOURCEFILESWITCHER_H
#define GASOURCEFILESWITCHER_H

#ifndef _CPP_STRING
#include <string>
#endif

#ifndef _CPP_IOSTREAM
#include <iostream>
#endif

#ifndef _CPP_FSTREAM
#include <fstream>
#endif

namespace gellisary
{
	class GASourceFileSwitcher
	{
		private:
			std::ifstream flat_file; /* Wie der Name der VAriable sagt, es ist 
						die Referenz auf die einzulesende Datei.*/
			std::string extension;
			std::string file_name;
			
		public:
			static const int EMBL = 1;
			static const int GENBANK = 2;
			static const int DDBJ = 3;
			static const int DDBJ_WITHOUT_HEADER = 4;
			static const int UNKNOWN = 9;
			GASourceFileSwitcher(const char *);
			virtual ~GASourceFileSwitcher();
			int make_a_decision();
			bool containsHeader();
	};
};

#else
#error GASourceFileSwitcher.h included twice
#endif /*GASOURCEFILESWITCHER_H*/
