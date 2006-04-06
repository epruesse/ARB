#ifndef GAARB_H
#include "GAARB.h"
#endif

gellisary::GAARB::GAARB(GBDATA * gb_main, const char * ali_name)
{
	arb = gb_main;
	alignment_name = ali_name;
	GBDATA *gb_species_data = GB_search(arb, "species_data", GB_CREATE_CONTAINER);
	char *new_species_name = AWTC_makeUniqueShortName("genome", gb_species_data);
	genome = GBT_create_species(arb, new_species_name);
}

bool gellisary::GAARB::create_new_genome()
{
	GBDATA *gb_species_data = GB_search(arb, "species_data", GB_CREATE_CONTAINER);
	char *new_species_name = AWTC_makeUniqueShortName("genome", gb_species_data);
	genome = GBT_create_species(arb, new_species_name);
	//delete gene;
	return true;
}

/*
 * container:	true  - genome
 * 				false - gene
 */
bool gellisary::GAARB::write_integer(const std::string & field, int number, bool container = true)
{
	GBDATA * gb_field;
	if(container)
	{
		gb_field = GB_search(genome, field.c_str(), GB_INT);
	}
	else
	{
		gb_field = GB_search(gene, field.c_str(), GB_INT);
	}
	GB_write_int(gb_field, number);
    return true;
}

/*
 * container:	true  - genome
 * 				false - gene
 */
bool gellisary::GAARB::write_string(const std::string & field, const std::string & word, bool container = true)
{
	GBDATA * gb_field;
	if(container)
	{
		gb_field = GB_search(genome, field.c_str(), GB_STRING);
	}
	else
	{
		gb_field = GB_search(gene, field.c_str(), GB_STRING);
	}
	GB_write_string(gb_field, word.c_str());
    return true;
}

/*
 * container:	true  - genome
 * 				false - gene
 */
bool gellisary::GAARB::write_byte(const std::string & field, bool number, bool container = true)
{
	GBDATA * gb_field;
	if(container)
	{
		gb_field = GB_search(genome, field.c_str(), GB_BYTE);
	}
	else
	{
		gb_field = GB_search(gene, field.c_str(), GB_BYTE);
	}
	if(number)
	{
		GB_write_byte(gb_field, 1);
	}
	else
	{
		GB_write_byte(gb_field, 0);
	}
	return true;
}

bool gellisary::GAARB::write_genome_sequence(const std::string & sequence, int c_all, int c_a, int c_c, int c_g, int c_t, int c_other)
{
	write_integer("sequence_length",c_all);
	write_integer("sequence_a",c_a);
	write_integer("sequence_c",c_c);
	write_integer("sequence_g",c_g);
	write_integer("sequence_t",c_t);
	write_integer("sequence_other",c_other);
	if(!sequence.empty() && sequence.size() > 0)
	{
		GB_write_string(GBT_add_data(genome, alignment_name.c_str(), "data", GB_STRING), sequence.c_str());
		return true;
	}
	else
	{
		return false;
	}
}

bool gellisary::GAARB::write_metadata_line(const std::string & line_type, const std::string & line_value, int counter = 0)
{
	if((!line_type.empty() && line_type.size() > 0) && (!line_value.empty() && line_value.size() > 0))
	{
		if(counter > 0)
		{
			std::stringstream t_counter;
			t_counter <<line_type << "_" << counter;
			write_string(t_counter.str(),line_value);
		}
		else
		{
			write_string(line_type,line_value);
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool gellisary::GAARB::write_next_gene(const std::string & gene_name, const std::string & feature, const std::string & raw_location, std::vector<int> & positions, bool complement, int counter)
{
	if((!gene_name.empty() && gene_name.size() > 0) && (!feature.empty() && feature.size() > 0) && (!raw_location.empty() && raw_location.size() > 0) && (positions.size() > 0))
	{
		gene = GEN_create_gene(genome, gene_name.c_str());
		write_integer("gene_number",counter,false);
		write_string("type",feature.c_str(),false);
		write_string("location",raw_location.c_str(),false);
		write_byte("complement",complement,false);
		int positions_size = positions.size();
		int gene_size = 0;
		if(positions_size > 2)
		{
			int positions_pair = (int) (positions_size / 2);
			write_integer("pos_joined", positions_pair,false);
			for(int i = 0; i < positions_pair; i++)
			{
				if(i == 0)
				{
					gene_size -= positions[i];
					write_integer("pos_begin",positions[i],false);
					gene_size += positions[i+1];
					write_integer("pos_end",positions[i+1],false);
				}
				else
				{
					std::stringstream t_pos_1;
					t_pos_1 << "pos_begin" << (i+1);
					gene_size -= positions[(i*2)];
					write_integer(t_pos_1.str(),positions[(i*2)],false);
					gene_size += positions[(i*2)+1];
					std::stringstream t_pos_2;
					t_pos_2 << "pos_end" << (i+1);
					write_integer(t_pos_2.str(),positions[(i*2)+1],false);
				}
			}
		}
		else if(positions_size > 0)
		{
			gene_size -= positions[0];
			write_integer("pos_begin",positions[0],false);
			gene_size += positions[1];
			write_integer("pos_end",positions[1],false);
		}
		write_integer("gene_length",++gene_size);
		return true;
	}
	else
	{
		return false;
	}
}

bool gellisary::GAARB::write_qualifier(const std::string & qualifier, const std::string & value)
{
	if((!qualifier.empty() && qualifier.size() > 0) && (!value.empty() && value.size() > 0))
	{
		write_string(qualifier,value,false);
		return true;
	}
	else
	{
		return false;
	}
}

gellisary::GAARB::~GAARB()
{
}
