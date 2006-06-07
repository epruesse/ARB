#ifndef GAGENBANK_H
#define GAGENBANK_H

#ifndef _CPP_STRING
#include <string>
#endif

#ifndef _CPP_SSTREAM
#include <sstream>
#endif

#ifndef _CPP_VECTOR
#include <vector>
#endif

#ifndef _CPP_IOSTREAM
#include <iostream>
#endif

#ifndef _CPP_FSTREAM
#include <fstream>
#endif

#ifndef _CPP_MAP
#include <map>
#endif

#ifndef _CPP_CSTDLIB
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
	class GAGenBank : public GAFile
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
	#if defined(DEBUG)
			GAGenBank(GALogger &, GAARB &, std::string &);
	#else
			GAGenBank(GAARB &, std::string &);
	#endif
			virtual ~GAGenBank();
			virtual void parse();
	};
};

#else
#error GAGenBank.h included twice
#endif /*GAGENBANK_H*/
