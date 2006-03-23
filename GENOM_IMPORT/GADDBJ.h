#ifndef GADDBJ_H
#define GADDBJ_H

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _GLIBCXX_SSTREAM
#include <sstream>
#endif

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _GLIBCXX_IOSTREAM
#include <iostream>
#endif

#ifndef _GLIBCXX_FSTREAM
#include <fstream>
#endif

#ifndef _GLIBCXX_MAP
#include <map>
#endif

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

#ifndef GAFILE_H
#include "GAFile.h"
#endif

#ifndef GALOGGER_H
#include "GALogger.h"
#endif

#ifndef GAARB_H
#include "GAARB.h"
#endif

namespace gellisary
{
	class GADDBJ : public GAFile
	{
		private:
			bool check_line_identifier(const std::string &);
			void dissectGenomeSequenceLine(const std::string &);
			void dissectMetaLine(const std::string &);
			void dissectTableFeatureLine(const std::string &);
			bool line_examination(const std::string &);
			void dissectLocation(const std::string &);
			void emptySequence();
			void check_and_write_metadata_line();
			
		public:
			GADDBJ(GALogger &, GAARB &, std::ifstream &);
			virtual ~GADDBJ();
			void parse();
	};
};

#endif /*GADDBJ_H*/
