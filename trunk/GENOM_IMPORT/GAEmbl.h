#ifndef GAEMBL_H
#define GAEMBL_H

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

#ifndef _CPP_MAP
#include <map>
#endif

#ifndef _CPP_FSTREAM
#include <fstream>
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
	class GAEmbl : public GAFile
	{
		private:
			bool check_line_identifier(const std::string &);
			void dissectGenomeSequenceLine(const std::string &);
			void dissectMetaLine(const std::string &);
			void dissectTableFeatureLine(const std::string &);
			bool line_examination(const std::string &);
			void emptyMeta();
			void emptyTable();
			void emptySequence();
			void dissectLocation(const std::string &);
			
		public:
	#if defined(DEBUG)
			GAEmbl(GALogger &, GAARB &, std::string &);
	#else
			GAEmbl(GAARB &, std::string &);
	#endif
			virtual ~GAEmbl();
			virtual void parse();
	};
};

#else
#error GAEmbl.h included twice
#endif /*GAEMBL_H*/
