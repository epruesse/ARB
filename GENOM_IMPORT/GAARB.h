#ifndef GAARB_H
#define GAARB_H

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _GLIBCXX_IOSTREAM
#include <iostream>
#endif

#ifndef _GLIBCXX_SSTREAM
#include <sstream>
#endif

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#ifndef arbdbt_h_included
#include <arbdbt.h>
#endif

#ifndef awt_hxx_included
#include <awt.hxx>
#endif

#ifndef AW_RENAME_HXX
#include <AW_rename.hxx>
#endif

#ifndef ADGENE_H
#include <adGene.h>
#endif

namespace gellisary
{
	class GAARB
	{
		private:
			GBDATA * arb;
			GBDATA * genome;
			GBDATA * gene;
			char * ali_name;
			std::string alignment_name;

		public:
			GAARB(GBDATA *, const char *);
			virtual ~GAARB();
			bool write_genome_sequence(const std::string &, int, int, int, int, int, int);
			bool write_next_gene(const std::string &, const std::string &, const std::string &, std::vector<int> &, bool, int);
			bool write_qualifier(const std::string &, const std::string &);
			bool write_metadata_line(const std::string &, const std::string &, int);
			bool write_integer(const std::string &, int, bool);
			bool write_string(const std::string &, const std::string &, bool);
			bool write_byte(const std::string &, bool, bool);
	};
};

#endif /*GAARB_H*/
