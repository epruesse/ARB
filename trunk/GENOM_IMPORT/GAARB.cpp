#ifndef GAARB_H
#include "GAARB.h"
#endif

#warning fix class GAARB ctor and create_new_genome method - useless work done here

gellisary::GAARB::GAARB(GBDATA * gb_main, const char * ali_name)
{
	arb            = gb_main;
	alignment_name = ali_name;

	GBDATA             *gb_species_data = GB_search(arb, "species_data", GB_CREATE_CONTAINER);
    UniqueNameDetector  und(gb_species_data);
	char *new_species_name              = AWTC_makeUniqueShortName("genome", und); // @@@ expensive function, do not use like this
    
	genome               = GBT_create_species(arb, new_species_name);
	new_gene             = true;
	gene_location_string = "";
	gene_gene            = "nix";
	gene_product         = "nix";
	gene_feature         = "";
	real_write           = false;
	gene_to_store        = false;
}

bool gellisary::GAARB::create_new_genome()
{
	GBDATA             *gb_species_data  = GB_search(arb, "species_data", GB_CREATE_CONTAINER);
    UniqueNameDetector  und(gb_species_data);
	char               *new_species_name = AWTC_makeUniqueShortName("genome", und);
    
	genome               = GBT_create_species(arb, new_species_name);
	new_gene             = true;
	gene_location_string = "";
	gene_gene            = "nix";
	gene_product         = "nix";
	gene_feature         = "";
	real_write           = false;
	gene_to_store        = false;
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
		GB_write_int(gb_field, number);
	}
	else
	{
		if(real_write)
		{
			gb_field = GB_search(gene, field.c_str(), GB_INT);
			GB_write_int(gb_field, number);
		}
		else
		{
			gene_int_data[field] = number;
		}
	}
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
		GB_write_string(gb_field, word.c_str());
	}
	else
	{
		if(real_write)
		{
			gb_field = GB_search(gene, field.c_str(), GB_STRING);
			GB_write_string(gb_field, word.c_str());
		}
		else
		{
			gene_string_data[field] = word;
			if(field == "gene")
			{
				gene_gene = word;
			}
			else if(field == "product")
			{
				gene_product = word;
			}
		}
	}
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
		if(number)
		{
			GB_write_byte(gb_field, 1);
		}
		else
		{
			GB_write_byte(gb_field, 0);
		}
	}
	else
	{
		if(real_write)
		{
			gb_field = GB_search(gene, field.c_str(), GB_BYTE);
			if(number)
			{
				GB_write_byte(gb_field, 1);
			}
			else
			{
				GB_write_byte(gb_field, 0);
			}
		}
		else
		{
			gene_bool_data[field] = number;
		}
	}
	return true;
}

bool gellisary::GAARB::write_genome_sequence(const std::string & sequence, int c_all, int c_a, int c_c, int c_g, int c_t, int c_other)
{
	real_write = true;
	write_integer("sequence_length",c_all);
	write_integer("sequence_a",c_a);
	write_integer("sequence_c",c_c);
	write_integer("sequence_g",c_g);
	write_integer("sequence_t",c_t);
	write_integer("sequence_other",c_other);
	if(!sequence.empty() && sequence.size() > 0)
	{
		GB_write_string(GBT_add_data(genome, alignment_name.c_str(), "data", GB_STRING), sequence.c_str());
		real_write = false;
		return true;
	}
	else
	{
		real_write = false;
		return false;
	}
}

bool gellisary::GAARB::write_metadata_line(const std::string & line_type, const std::string & line_value, int counter = 0)
{
	if((!line_type.empty() && line_type.size() > 0) && (!line_value.empty() && line_value.size() > 0))
	{
		real_write = true;
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
		real_write = false;
		return true;
	}
	else
	{
		return false;
	}
}

bool gellisary::GAARB::write_next_gene(const std::string & feature, const std::string & raw_location, std::vector<int> & positions, std::vector<int> & complements, int counter)
{
	if((!feature.empty() && feature.size() > 0) && (!raw_location.empty() && raw_location.size() > 0) && (positions.size() > 0))
	{
		if(!new_gene) // First Gene
		{
			store_gene();
		}
		//gene = GEN_create_gene(genome, raw_location.c_str());
		gene_location_string = raw_location;
		gene_feature = feature;
		gene_string_data["type"] = feature;
		gene_string_data["location"] = raw_location;
		gene_int_data["gene_number"] = counter;
		int complements_size = complements.size();
		
		if(complements_size < 2)
		{
			gene_bool_data["complement"] = complements[0];
		}
		else
		{
			for(int k = 0; k < complements_size; k++)
			{
				if(k == 0)
				{
					gene_bool_data["complement"] = complements[0];
				}
				else
				{
					std::stringstream t_complement;
					t_complement << "complement" << (k+1);
					gene_bool_data[t_complement.str()] = complements[k];
				}
			}
		}
		
		int positions_size = positions.size();
		int gene_size = 0;
		if(positions_size > 2)
		{
			int positions_pair = (int) (positions_size / 2);
			gene_int_data["pos_joined"] = positions_pair;
			for(int i = 0; i < positions_pair; i++)
			{
				if(i == 0)
				{
					gene_size -= positions[i];
					gene_int_data["pos_begin"] = positions[i];
					gene_size += positions[i+1];
					gene_int_data["pos_end"] = positions[i+1];
				}
				else
				{
					std::stringstream t_pos_1;
					t_pos_1 << "pos_begin" << (i+1);
					gene_size -= positions[(i*2)];
					gene_int_data[t_pos_1.str()] = positions[(i*2)];
					gene_size += positions[(i*2)+1];
					std::stringstream t_pos_2;
					t_pos_2 << "pos_end" << (i+1);
					gene_int_data[t_pos_2.str()] = positions[(i*2)+1];
				}
			}
		}
		else if(positions_size > 0)
		{
			gene_size -= positions[0];
			gene_int_data["pos_begin"] = positions[0];
			gene_size += positions[1];
			gene_int_data["pos_end"] = positions[1];
		}
		gene_int_data["gene_length"] = ++gene_size;
		new_gene = false;
		gene_to_store = true;
		return true;
	}
	else
	{
		return false;
	}
}

bool gellisary::GAARB::store_gene()
{
	if((!gene_feature.empty() && gene_feature.size() > 0) && (!gene_location_string.empty() && gene_location_string.size() > 0))
	{
		real_write = true;
		std::string unique_gene_name = generate_gene_id(gene_location_string, gene_feature, gene_product, gene_gene);
		gene = GEN_create_gene(genome, unique_gene_name.c_str());
		std::string q_name;
		std::string q_s_value;
		int q_i_value;
		bool q_b_value;
		gene_string_data_iter = gene_string_data.begin();
		while(gene_string_data_iter != gene_string_data.end())
		{
			q_name = gene_string_data_iter->first; // key
			q_s_value = gene_string_data_iter->second; // string value
			write_string(q_name,q_s_value,false);
			gene_string_data_iter++;
		}
		gene_int_data_iter = gene_int_data.begin();
		while(gene_int_data_iter != gene_int_data.end())
		{
			q_name = gene_int_data_iter->first; // key
			q_i_value = gene_int_data_iter->second; // integer value
			write_integer(q_name,q_i_value,false);
			gene_int_data_iter++;
		}
		gene_bool_data_iter = gene_bool_data.begin();
		while(gene_bool_data_iter != gene_bool_data.end())
		{
			q_name = gene_bool_data_iter->first; // key
			q_b_value = gene_bool_data_iter->second; // bool value
			write_byte(q_name,q_b_value,false);
			gene_bool_data_iter++;
		}
		gene_string_data.clear();
		gene_int_data.clear();
		gene_bool_data.clear();
		gene_feature = "";
		gene_location_string = "";
		gene_product = "nix";
		gene_gene = "nix";
		gene_to_store = false;
		new_gene = true;
		real_write = false;
	}
	else
	{
		return false;
	}
	return true;
}

bool gellisary::GAARB::close()
{
	if(gene_to_store)
	{
		store_gene();
	}
	return true;
}

bool gellisary::GAARB::check_string_key(const std::string & string_key)
{
	int string_key_size = string_key.size();
	bool result = true;
	for(int i = 0; i < string_key_size; i++)
	{
		if(((string_key[i] >= 48) && (string_key[i] <= 57)) || ((string_key[i] >= 65) && (string_key[i] <= 90))
		 	|| ((string_key[i] >= 97) && (string_key[i] <= 122)) || (string_key[i] == '_'))
		{
			result = true;
		}
		else
		{
			return false;
		}
	}
	return result;
}

bool gellisary::GAARB::write_qualifier(const std::string & qualifier, const std::string & value)
{
	if((!qualifier.empty() && qualifier.size() > 0) && (!value.empty() && value.size() > 0))
	{
		if(check_string_key(qualifier))
		{
			write_string(qualifier,value,false);
			return true;
		}
		else
		{
			std::cout << "Q : " << qualifier << " # V : " << value << std::endl;
			return false;
		}
	}
	else
	{
		return false;
	}
}

std::string gellisary::GAARB::generate_gene_id(const std::string & location, const std::string & feature_type, const std::string & product, const std::string & gene)
{
	std::string result;
    bool next = true;
    int pointer = 0;
    std::ostringstream string_out_1;
    std::ostringstream string_out_2;
    std::string str_1;
    std::string str_2;
    bool drin = false;
    string_out_1 << feature_type;
    string_out_1 << '_';
    int i = 0;
    
    std::string::size_type product_size = product.size();
    std::string product_prepared;
    
	bool ziffer = false;
	if(gene != "nix")
	{
		for(int l = 0;l < (int)gene.size();l++)
		{
			if((gene[l] >= 48) || (gene[l] <= 57))
			{
				ziffer = true;
			}
		}
		if(!ziffer)
		{
			string_out_1 << gene;
			string_out_1 << '_';
		}
	}
    
    
    for(int j = 0; j < (int)product_size; j++)
    {
    	if(product[j] == ' ')
    	{
    		product_prepared.push_back('_');
    	}
    	else
    	{
    		product_prepared.push_back(product[j]);
    	}
    }
    while(next)
    {
        i = location[pointer++];
        if(i >= 48 && i <= 57)
        {
		    if(!drin)
		    {
		    	drin = true;
		    }
		    string_out_2 << (char)i;
        } 
        else if(drin)
        {
        	drin = false;
        	break;
        }
        if(pointer == (int) location.size())
        {
            break;
        }
    }
    str_1 = string_out_2.str();
    str_2 = string_out_1.str();
    
  	int rest = 29 - (int)str_2.size() - (int) str_1.size();
   	if(product_prepared != "nix")
   	{
   		if(rest < (int)product_prepared.size())
	   	{
	   		product_prepared.resize(rest);
	   		string_out_1 << product_prepared;
		    string_out_1 << '_';
	   	}
	   	else
	   	{
	   		string_out_1 << product_prepared;
		    string_out_1 << '_';
		}
   	}

   	string_out_1 << str_1;
    result = string_out_1.str();
    return result;
}

gellisary::GAARB::~GAARB()
{
}
