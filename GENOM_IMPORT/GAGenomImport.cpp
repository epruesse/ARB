#include <cstdio>
#include <arbdb.h>
#include <arbdbt.h>
#include <awt.hxx>
#include <awtc_rename.hxx>
#include <adGene.h>

#ifndef _CPP_VECTOR
#include <vector>
#endif
#ifndef _CPP_STRING
#include <string>
#endif

#include "GAGenomImport.h"
#include "GAParser.h"
#include "GAGenomEmbl.h"
#include "GAGenomUtilities.h"
#include "GAGenomFeatureTableSourceEmbl.h"
#include "GAGenomGeneEmbl.h"
#include "GAGenomGeneLocationEmbl.h"


namespace gellisary {
#warning Die class GAGenomImport ist ueberfluessig - definiere einfach alle Funktionen non-static direkt im namespace
    class GAGenomImport{
    public:
        static GB_ERROR executeQuery(GBDATA *, const char *,const char *);
        static void writeReferenceEmbl(GBDATA *, gellisary::GAGenomReferenceEmbl *);
        static void writeFeatureTableEmbl(GBDATA *, gellisary::GAGenomFeatureTableEmbl *);
        static void writeGeneEmbl(GBDATA *, gellisary::GAGenomGeneEmbl *);
        static bool writeLocationEmbl(GBDATA *, gellisary::GAGenomGeneLocationEmbl *, std::vector<int> *);
        static void writeByte(GBDATA *, std::string *, int);
        static void writeSourceEmbl(GBDATA *, gellisary::GAGenomFeatureTableSourceEmbl *);
        static void writeString(GBDATA *, std::string *, std::string *);
        static void writeInteger(GBDATA *, std::string *, int);
        static void writeGenomSequence(GBDATA *, std::string *, const char *);
    };

};

using namespace std;

/*bool gellisary::GAGenomImport::setFileName(std::string * source_str)
{
	std::string extension;

	for(int i = (file_name_size-1); i >= 0; i--)
	{
		if(filename[i] == '.')
		{
			for(int j = (i+1); j < file_name_size; j++)
			{
				extension.append(filename[j]);
			}
		}
	}
}

bool gellisary::GAGenomImport::setFileName(char * source_chars)
{
	std::string target_str = *source_chars;
	setFileName(&target_str);
}*/

void gellisary::GAGenomImport::writeReferenceEmbl(GBDATA * source_container, gellisary::GAGenomReferenceEmbl * reference)
{
	std::string r_nummer("reference_");
	r_nummer.append((GAGenomUtilities::integerToString(reference->getNumber())));
	r_nummer.append("_");

	std::string * tmp_str_pnt;
	tmp_str_pnt = reference->getAuthorsAsString();
	if(*tmp_str_pnt != "none")
	{
		std::string tmp_str = r_nummer;
		tmp_str.append("authors");
		gellisary::GAGenomImport::writeString(source_container,&tmp_str,tmp_str_pnt);
	}
	tmp_str_pnt = reference->getComment();
	if(*tmp_str_pnt != "none")
	{
		std::string tmp_str = r_nummer;
		tmp_str.append("comment");
		gellisary::GAGenomImport::writeString(source_container,&tmp_str,tmp_str_pnt);
	}
	tmp_str_pnt = reference->getCrossReferenceAsString();
	if(*tmp_str_pnt != "none")
	{
		std::string tmp_str = r_nummer;
		tmp_str.append("cross_reference");
		gellisary::GAGenomImport::writeString(source_container,&tmp_str,tmp_str_pnt);
	}
	tmp_str_pnt = reference->getGroup();
	if(*tmp_str_pnt != "none")
	{
		std::string tmp_str = r_nummer;
		tmp_str.append("group");
		gellisary::GAGenomImport::writeString(source_container,&tmp_str,tmp_str_pnt);
	}
	tmp_str_pnt = reference->getLocation();
	if(*tmp_str_pnt != "none")
	{
		std::string tmp_str = r_nummer;
		tmp_str.append("location");
		gellisary::GAGenomImport::writeString(source_container,&tmp_str,tmp_str_pnt);
	}
	tmp_str_pnt = reference->getTitle();
	if(*tmp_str_pnt != "none")
	{
		std::string tmp_str = r_nummer;
		tmp_str.append("title");
		gellisary::GAGenomImport::writeString(source_container,&tmp_str,tmp_str_pnt);
	}
}

void gellisary::GAGenomImport::writeFeatureTableEmbl(GBDATA * source_container, gellisary::GAGenomFeatureTableEmbl * feature_table)
{
	gellisary::GAGenomImport::writeSourceEmbl(source_container,feature_table->getFeatureTableSource());
	gellisary::GAGenomGeneEmbl * tmp_gene;
	std::string * tmp_str;
	while((tmp_str = feature_table->getGeneName()) != NULL)
	{
		tmp_gene = feature_table->getGeneByName(tmp_str);
		gellisary::GAGenomImport::writeGeneEmbl(source_container,tmp_gene);
	}
}

void gellisary::GAGenomImport::writeGeneEmbl(GBDATA * source_container, gellisary::GAGenomGeneEmbl * gene)
{
	std::string tmp_str;
	int tmp_int;
	std::string * tmp_str_pnt;
	std::string * tmp_str_pnt2;
	GBDATA * gene_container;

	tmp_str_pnt = gene->getNameOfGene();
	if(*tmp_str_pnt != "none")
	{
		tmp_str = "name";
		gene_container = GEN_create_gene(source_container, tmp_str_pnt->c_str());
//		gene_container = GB_create_container(source_container, tmp_str_pnt->c_str());
//		gellisary::GAGenomImport::writeString(gene_container,&tmp_str,tmp_str_pnt);
		tmp_int = gene->getGeneNumber();	// int
		if(tmp_int != -1)
		{
			tmp_str = "gene_number";
			gellisary::GAGenomImport::writeInteger(gene_container,&tmp_str,tmp_int);
		}
		tmp_str_pnt = gene->getGeneType();
		if(*tmp_str_pnt != "none")
		{
			tmp_str = "type";
			gellisary::GAGenomImport::writeString(gene_container,&tmp_str,tmp_str_pnt);
		}
		tmp_int = gene->getGeneTypeNumber();	// int
		if(tmp_int != -1)
		{
			tmp_str = "gene_type_number";
			gellisary::GAGenomImport::writeInteger(gene_container,&tmp_str,tmp_int);
		}
		tmp_str_pnt = gene->getLocationAsString();
		if(*tmp_str_pnt != "none")
		{
			tmp_str = "location";
			gellisary::GAGenomImport::writeString(gene_container,&tmp_str,tmp_str_pnt);
		}
		while((tmp_str_pnt = gene->getQualifierName()) != NULL)
		{
			if(*tmp_str_pnt != "translation")
			{
				tmp_str_pnt2 = gene->getQualifierValue(tmp_str_pnt);
				if(!(*tmp_str_pnt2).empty())
				{
					gellisary::GAGenomImport::writeString(gene_container,tmp_str_pnt,tmp_str_pnt2);
				}
				else
				{
					tmp_str = "yes";
					gellisary::GAGenomImport::writeString(gene_container,tmp_str_pnt,&tmp_str);
				}
			}
		}

		GAGenomGeneLocationEmbl * tmp_location;
		GAGenomGeneLocationEmbl * tmp_location2;
		tmp_location = gene->getLocation();
//		bool complement = false;
//		bool join = false;
		std::vector<int> joined;
		if(tmp_location->isSingleValue())
		{
			tmp_int = tmp_location->getSingleValue();
			tmp_str = "pos_begin";
			gellisary::GAGenomImport::writeInteger(gene_container,&tmp_str,tmp_int);
			tmp_str = "pos_end";
			gellisary::GAGenomImport::writeInteger(gene_container,&tmp_str,tmp_int);
		}
		else
		{
			while((tmp_location2 = tmp_location->getNextValue()) != NULL)
			{
				if(gellisary::GAGenomImport::writeLocationEmbl(gene_container,tmp_location2,&joined))
				{
					tmp_str = "complement";
					gellisary::GAGenomImport::writeByte(gene_container,&tmp_str,1);
				}
				else
				{
					tmp_str = "complement";
					gellisary::GAGenomImport::writeByte(gene_container,&tmp_str,0);
				}
			}
			if((int)joined.size() > 0)
			{
				tmp_str = "pos_joined";
				gellisary::GAGenomImport::writeInteger(gene_container,&tmp_str,((int)joined.size()/2));
				for(int k = 0; k < (int) joined.size(); k+=2)
				{
					if(k == 0)
					{
						tmp_str = "pos_begin";
						gellisary::GAGenomImport::writeInteger(gene_container,&tmp_str,tmp_int);
						tmp_str = "pos_end";
						gellisary::GAGenomImport::writeInteger(gene_container,&tmp_str,tmp_int);
					}
					else
					{
						tmp_str = "pos_begin"+GAGenomUtilities::integerToString(k);
						gellisary::GAGenomImport::writeInteger(gene_container,&tmp_str,joined[k]);
						tmp_str = "pos_end"+GAGenomUtilities::integerToString(k);
						gellisary::GAGenomImport::writeInteger(gene_container,&tmp_str,joined[k]);
					}
				}
			}
		}
	}
}

bool gellisary::GAGenomImport::writeLocationEmbl(GBDATA * source_container, gellisary::GAGenomGeneLocationEmbl * location, std::vector<int> * joined)
{
	GAGenomGeneLocationEmbl * tmp_location;
	std::string tmp_str;
	int tmp_int;
	if(location->isSingleValue())
	{
		tmp_int = location->getSingleValue();
		joined->push_back(tmp_int);
	}
	else
	{
		if(location->isComplement())
		{
			return true;
		}
		while((tmp_location = location->getNextValue()) != NULL)
		{
			if(gellisary::GAGenomImport::writeLocationEmbl(source_container,tmp_location,joined))
			{
				return true;
			}
		}
	}
	return false;
}

void gellisary::GAGenomImport::writeByte(GBDATA * source_container, std::string * target_field, int source_int)
{
	GBDATA *gb_field = GB_search(source_container, target_field->c_str(), GB_BYTE);
    GB_write_byte(gb_field, source_int);
}

void gellisary::GAGenomImport::writeSourceEmbl(GBDATA * source_container, gellisary::GAGenomFeatureTableSourceEmbl * source)
{
	std::string tmp_str;
	std::string * tmp_str_pnt;
	std::string * tmp_str_pnt2;
	while((tmp_str_pnt = source->getNameOfQualifier()) != NULL)
	{
		tmp_str_pnt2 = source->getValueOfQualifier(tmp_str_pnt);
		if(!(*tmp_str_pnt2).empty())
		{
			gellisary::GAGenomImport::writeString(source_container,tmp_str_pnt,tmp_str_pnt2);
		}
		else
		{
			tmp_str = "yes";
			gellisary::GAGenomImport::writeString(source_container,tmp_str_pnt,&tmp_str);
		}
	}
}

void gellisary::GAGenomImport::writeString(GBDATA * source_container, std::string * target_field, std::string * source_str)
{
	GBDATA *gb_field = GB_search(source_container, target_field->c_str(), GB_STRING);
	GB_write_string(gb_field, source_str->c_str());
}

void gellisary::GAGenomImport::writeInteger(GBDATA * source_container, std::string * target_field, int source_int)
{
	GBDATA *gb_field = GB_search(source_container, target_field->c_str(), GB_INT);
	GB_write_int(gb_field, source_int);
}

void gellisary::GAGenomImport::writeGenomSequence(GBDATA * source_container, std::string * source_str, const char * alignment_name)
{
	GB_write_string(GBT_add_data(source_container, alignment_name, "data", GB_STRING), source_str->c_str());
}

GB_ERROR gellisary::GAGenomImport::executeQuery(GBDATA * gb_main, const char * file_name, const char * ali_name)
{
//	gb_name = gb_main;
//	alignement_name = *ali_name;

	GB_ERROR error = NULL;
	int file_name_size = strlen(file_name);

	std::string extension;

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

	if(extension == "embl")
	{
		string ffname = file_name;
		gellisary::GAGenomEmbl genomembl(&ffname);
		genomembl.parseFlatFile();
//		int typeOfDatabase = 1;
		GBDATA        *gb_species_data  = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
		char          *new_species_name = AWTC_makeUniqueShortName("genom", gb_species_data);
		GBDATA        *gb_species       = GBT_create_species(gb_main, new_species_name);
//		int rcounter = 0;

		std::vector<std::string> tmp_str_vector;
		std::vector<int> tmp_int_vector;
		std::string tmp_string;
		std::string * tmp_string_pnt;

//		char *content;
//		char nummer[] = {'0','1','2','3','4','5','6','7','8','9'};
		std::string field;

		field = "identification";
		tmp_string_pnt = genomembl.getIdentification();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "accession_number";
		tmp_string_pnt = genomembl.getIdentification();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "free_text_comment";
		tmp_string_pnt = genomembl.getCommentAsOneString();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "contig";
		tmp_string_pnt = genomembl.getContig();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "database_cross_reference";
		tmp_string_pnt = genomembl.getDataCrossReferenceAsOneString();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "date_of_creation";
		tmp_string_pnt = genomembl.getDateOfCreation();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "date_of_last_update";
		tmp_string_pnt = genomembl.getDateOfLastUpdate();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "description";
		tmp_string_pnt = genomembl.getDescription();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "key_words";
		tmp_string_pnt = genomembl.getKeyWordsAsString();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "orgarnelle";
		tmp_string_pnt = genomembl.getOrganelle();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "full_name";
		tmp_string_pnt = genomembl.getOrganism();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "organism_classification";
		tmp_string_pnt = genomembl.getOrganismClassificationAsOneString();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}
		field = "sequence_version";
		tmp_string_pnt = genomembl.getSequenceVersion();
		if(*tmp_string_pnt != "none")
		{
			gellisary::GAGenomImport::writeString(gb_species,&field,tmp_string_pnt);
		}

		gellisary::GAGenomImport::writeGenomSequence(gb_species,genomembl.getSequence(),ali_name);

		tmp_int_vector = *(genomembl.getSequenceHeader());
		tmp_str_vector.clear();
		tmp_str_vector.push_back("sequence_lenght");
		tmp_str_vector.push_back("sequence_a");
		tmp_str_vector.push_back("sequence_c");
		tmp_str_vector.push_back("sequence_g");
		tmp_str_vector.push_back("sequence_t");
		tmp_str_vector.push_back("sequence_other");
		int tmp_int;
		for(int i = 0; i < (int)tmp_int_vector.size(); i++)
		{
			field = tmp_str_vector[i];
			tmp_int = tmp_int_vector[i];
			if(tmp_int != 0)
			{
				gellisary::GAGenomImport::writeInteger(gb_species,&field,tmp_int);
			}
		}

		gellisary::GAGenomImport::writeFeatureTableEmbl(gb_species,genomembl.getFeatureTable());

		GAGenomReferenceEmbl * tmp_reference;
		while((tmp_reference = genomembl.getReference()) != NULL)
		{
			gellisary::GAGenomImport::writeReferenceEmbl(gb_species,tmp_reference);
		}
		delete (tmp_reference);
	}
	else if(extension == "gbk")
	{

	}
	else if(extension == "ff")
	{

	}
	return error;
}


GB_ERROR GI_importGenomeFile(GBDATA * gb_main, const char * file_name, const char * ali_name) {
    return gellisary::GAGenomImport::executeQuery(gb_main, file_name, ali_name);
}

