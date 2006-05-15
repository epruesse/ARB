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

#ifndef _GLIBCXX_MAP
#include <map>
#endif

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
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
			std::string gene_location_string;
			std::string gene_gene;
			std::string gene_product;
			std::string gene_feature;
			bool new_gene;
			bool real_write;
			bool gene_to_store;
			
			std::map<std::string,std::string> gene_string_data;
			std::map<std::string,std::string>::iterator gene_string_data_iter;
			std::map<std::string,int> gene_int_data;
			std::map<std::string,int>::iterator gene_int_data_iter;
			std::map<std::string,bool> gene_bool_data;
			std::map<std::string,bool>::iterator gene_bool_data_iter;
			
			std::string generate_gene_id(const std::string &, const std::string &, const std::string &, const std::string &);
			bool store_gene();
			bool check_string_key(const std::string &);
			
		public:
			GAARB(GBDATA *, const char *);
			virtual ~GAARB();
			bool write_genome_sequence(const std::string &, int, int, int, int, int, int);
			bool write_next_gene(const std::string &, const std::string &, std::vector<int> &, bool, int);
			bool create_new_genome();
			bool write_qualifier(const std::string &, const std::string &);
			bool write_metadata_line(const std::string &, const std::string &, int);
			bool write_integer(const std::string &, int, bool);
			bool write_string(const std::string &, const std::string &, bool);
			bool write_byte(const std::string &, bool, bool);
			bool close();
	};
};

#endif /*GAARB_H*/
