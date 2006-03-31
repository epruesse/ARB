#ifndef GAGENBANK_H
#include "GAGenBank.h"
#endif

gellisary::GAGenBank::GAGenBank(GALogger & nLogger, GAARB & nARB, std::ifstream & nARB_File) : GAFile(nLogger, nARB, nARB_File)
{
	line_identifiers["LOCUS"] = "identification";
	line_identifiers["ACCESSION"] = "accession_number";
	line_identifiers["VERSION"] = "sequence_version";
	line_identifiers["DEFINITION"] = "definition";
	line_identifiers["KEYWORDS"] = "keyword";
	line_identifiers["HINVVERSION"] = "hinvversion"; // ??
	line_identifiers["SOURCE"] = "organism_species";
	line_identifiers["ORGANISM"] = "organism_classification";
	line_identifiers["REFERENCE"] = "reference_number";
	line_identifiers["AUTHORS"] = "reference_author";
	line_identifiers["TITLE"] = "reference_title";
	line_identifiers["JOURNAL"] = "reference_journal";
	line_identifiers["MEDLINE"] = "reference_medline";
	line_identifiers["CONSRTM"] = "reference_consrtm"; // may be 'consortium'
	line_identifiers["PUBMED"] = "reference_pubmed";
	line_identifiers["COMMENT"] = "comments_or_notes";
	line_identifiers["//"] = "termination";

	type = EMPTY;
	complement = false;

	error_line_to_short = "line is to short (min 2 chars)";
	error_line_to_short_for_sequence = "line is to short for genome sequence (must be 80 chars)";
	error_line_to_long = "line is to long (max 80 chars)";
	error_wrong_line_key = "line key is invalid";
	error_chars_234_not_empty = "chars 2,3 and 4 are notempty";
	error_char_0_not_empty = "char 0 is nnot empty";
	error_char_1_not_empty = "char 1 is not empty";
	error_char_0_empty = "char 0 is empty";
	error_char_1_empty = "char 1 is not empty";
	error_miss_one_base = "miss one base";
	error_char_not_empty = "char is not empty";
	error_wrong_sequence_format = "sequence format is wrong";
	error_wrong_line_format = "line format is wrong";

	counter_a = 0;
	counter_c = 0;
	counter_g = 0;
	counter_t = 0;
	counter_other = 0;
	counter_line = 0;
	counter = 0;
	counter_character = 0;
	line_id = "";
	feature = "";
}

bool gellisary::GAGenBank::check_line_identifier(const std::string & source_line_identifier)
{
	std::map<std::string,std::string>::iterator line_identifiers_iter = line_identifiers.find(source_line_identifier);
	if(line_identifiers_iter != line_identifiers.end()) // wenn etwas gefunden wurde
	{
		if(source_line_identifier == "//")
		{
			new_type = END;
		}
		else
		{
			new_type = META;
		}
		return true;
	}
	else // wenn nicht existiert
	{
		return false;
	}
}

void gellisary::GAGenBank::dissectGenomeSequenceLine(const std::string & source_line)
{
	if(type == META)
	{
		emptySequence();
		type = SEQUENCE;
		line_id = "SE";
		type = SEQUENCE;
		counter = 0;
	}
	bool mustBeEnd = false;
	char t_base;
	int source_size = source_line.size();
	for(int i = 0; i < source_size; i++)
	{
		if(source_line[i] == ' ')
		{
			if(((i != 9) && (i != 20) && (i != 31) && (i != 42) && (i != 53) && (i != 64)) && ((i >= 75) || (i == 8)))
			{
				if(!mustBeEnd)
				{
					mustBeEnd = true;
				}
			}
		}
		else
		{
			if((i == 9) || (i == 20) || (i == 31) || (i == 42) || (i == 53) || (i == 64) || (i == 75))
			{
				// Fehler
				logger.add_log_entry(error_char_not_empty+" = "+source_line.substr(i,1), counter_line, i);
				break;
			}
			else if((i >= 0) && (i <= 8))
			{
				if(!isdigit(source_line[i]))
				{
					// Fehler
					logger.add_log_entry(error_wrong_sequence_format+" = "+source_line.substr(i,1), counter_line, i);
					break;
				}
			}
			else
			{
				if(mustBeEnd)
				{
					// Fehler
					logger.add_log_entry(error_wrong_sequence_format+" = "+source_line.substr(i,1), counter_line, i);
					break;
				}
				else
				{
					if(isalpha(source_line[i]))
					{
						counter++;
						t_base = std::tolower(source_line[i]);
						if(t_base == 'a')
						{
							counter_a++;
						}
						else if(t_base == 'c')
						{
							counter_c++;
						}
						else if(t_base == 'g')
						{
							counter_g++;
						}
						else if(t_base == 't')
						{
							counter_t++;
						}
						else
						{
							counter_other++;
						}
						value.push_back(t_base);
					}
				}
			}
		}
	}
}

void gellisary::GAGenBank::emptySequence()
{
	counter = 0;
	value = "";
	counter_a = 0;
	counter_c = 0;
	counter_g = 0;
	counter_t = 0;
	counter_other = 0;
}

void gellisary::GAGenBank::dissectMetaLine(const std::string & source_line)
{
	std::string t_line_id = source_line.substr(0,12);
	std::string t_pre_line = source_line.substr(12);
	std::string t_line = trim(t_pre_line);
	std::string::size_type space_pos = t_line.find(" ");
	bool has_space = true;
	std::string t_line_t;
	if(space_pos == std::string::npos)
	{
		has_space = false;
	}
	if(type == META)
	{
		if((t_line_id[0] == ' ') && (t_line_id[1] == ' ') && (t_line_id[2] == ' '))
		{
			if(has_space)
			{
				value.append(" "+t_line);
			}
			else
			{
				value.append(t_line);
			}
		}
		else if((t_line_id[0] == ' ') && (t_line_id[1] == ' ') && (t_line_id[2] != ' '))
		{
			// Feature
			t_line_t = trim(t_line_id);
			if(feature == "")
			{
				if(line_id != "")
				{
					if(line_id != "REFERENCE")
					{
						check_and_write_metadata_line();
					}
					type = META;
					feature = t_line_t;
					value = t_line;
				}
			}
			else
			{
				if(line_id != "")
				{
					check_and_write_metadata_line();
				}
				type = META;
				feature = t_line_t;
				value = t_line;
			}
		}
		else if((t_line_id[0] != ' ') && (t_line_id[1] != ' '))
		{
			t_line_t = trim(t_line_id);
			if(t_line_t == "SOURCE")
			{
				arb.write_metadata_line("full_name", t_line, 0);
			}
			if(t_line_t == "ACCESSION")
			{
				arb.write_metadata_line("acc", t_line, 0);
			}
			if(feature != "")
			{
				check_and_write_metadata_line();
			}
			else if(line_id != "")
			{
				check_and_write_metadata_line();
			}
			feature = "";
			counter = 0;
			if(t_line_t == "REFERENCE")
			{
				space_pos = t_line.find_first_of(" ");
				std::string::size_type klamma_pos = t_line.find_first_of("(");
				if(space_pos != std::string::npos)
				{
					if(klamma_pos != std::string::npos)
					{
						if(space_pos < klamma_pos)
						{
							value = t_line.substr(0,space_pos);
						}
						else
						{
							value = t_line.substr(0,klamma_pos);
						}
					}
					else
					{
						value = t_line.substr(0,space_pos);
					}
				}
				else if(klamma_pos != std::string::npos)
				{
					value = t_line.substr(0,klamma_pos);
				}
				else
				{
					value = t_line;
				}
				counter = std::atoi(value.c_str());
				line_id = t_line_t;
			}
			type = META;
			line_id = t_line_t;
			value = t_line;
		}
		else
		{
			// Fehler
			logger.add_log_entry(error_wrong_line_format, counter_line,0);
		}
		type = META;
	}
	else if(type == EMPTY)
	{
		t_line_t = trim(t_line_id);
		if(t_line_t == "LOCUS")
		{
			arb.write_metadata_line("source_database", "genbank", 0);
			line_id = t_line_t;
			value = t_line;
		}
		else
		{
			// Fehler
			logger.add_log_entry(error_wrong_line_key, counter_line,0);
		}
		type = META;
	}
	else if(type == TABLE)
	{
		t_line_t = trim(t_line_id);
		arb.write_qualifier(qualifier,value);
		type = META;
		line_id = t_line_t;
		value = t_line;
	}
}


void gellisary::GAGenBank::dissectTableFeatureLine(const std::string & source_line)
{
	std::string t_key = source_line.substr(0,21);
	std::string t_qualifier_line = trim(source_line.substr(21), " \n\t\r\"");
	std::string::size_type t_pos;
	std::string::size_type t_none_pos;
	if(type == TABLE)
	{
		if((t_key[0] == ' ') && (t_key[1] == ' ') && (t_key[2] == ' ') && (t_key[3] == ' ') && (t_key[4] == ' '))
		{
			if(t_key[5] == ' ')
			{
				// Ohne Feature
				// cannot be location
				if(t_qualifier_line[0] == '/')
				{
					t_pos = t_qualifier_line.find_first_of("=");
					if(t_pos != std::string::npos)
					{
						std::string t_qualifier = t_qualifier_line.substr(1,(t_pos-1));
						t_none_pos = t_qualifier.find(" ");
						if(t_none_pos == std::string::npos)
						{
							std::string t_value = trim(t_qualifier_line.substr(++t_pos)," \n\t\r\"");
							if(t_qualifier == qualifier)
							{
								value.append(" "+t_value);
							}
							else
							{
								if(feature != "source")
								{
									if(qualifier != "translation")
									{
										arb.write_qualifier(qualifier,value);
									}
									else
									{
										qualifier = "";
										value = "";
									}
								}
								else
								{
									arb.write_metadata_line(qualifier,value,0);
								}
								qualifier = t_qualifier;
								value = t_value;
							}
						}
						else
						{
							// Fehler
							value += t_qualifier_line;
							logger.add_log_entry(error_wrong_line_format,counter_line,0);
						}
					}
					else
					{
						// Fehler
						logger.add_log_entry(error_wrong_line_format,counter_line,0);
					}
				}
				else // Tritt bei Qualifier 'translation' zu Tage.
				{
					value += t_qualifier_line;
				}
			}
			else if(t_key[5] != ' ')
		    {
		   		// Mit Feature
		   		if(trim(t_key) == "source")
		   		{
			   		feature = "source";
					counter = 0;
					qualifier = "location";
					value = t_qualifier_line;
		   		}
		   		else
		   		{
		   			if(feature == "source")
					{
						arb.write_metadata_line(qualifier,value,0);
					}
					else
					{
						if(qualifier != "translation")
						{
							arb.write_qualifier(qualifier,value);
						}
						else
						{
							qualifier = "";
							value = "";
						}
					}
					feature = trim(t_key);
					counter++;
					qualifier = "location";
					complement = false;
					value = t_qualifier_line;
					dissectLocation(t_qualifier_line);
					std::stringstream t_name;
					t_name << feature << "_" << counter;
					name = t_name.str();
					arb.write_next_gene(name, feature, value, positions, complement, counter);
		   		}
		    }
		    else
			{
				// Fehler
				logger.add_log_entry(error_wrong_line_format, counter_line,0);
			}
		}
		else
		{
			// Fehler
			logger.add_log_entry(error_wrong_line_format, counter_line,0);
		}
	}
	else if(type == META)
	{
		if(trim(t_key) == "FEATURES")
		{
			check_and_write_metadata_line();
		}
		else
		{
			// Fehler
			logger.add_log_entry(error_wrong_line_key, counter_line,0);
		}
	}
	type = TABLE;
	line_id = "FEATURES";
}

void gellisary::GAGenBank::check_and_write_metadata_line()
{
	if(feature == "")
	{
		if(check_line_identifier(line_id))
		{
			arb.write_metadata_line(line_identifiers[line_id],value,counter);
		}
		else
		{
			int line_id_size = line_id.size();
			std::string v_l_line_id;
			for(int j = 0; j < line_id_size; j++)
			{
				v_l_line_id.push_back(std::tolower(line_id[j]));
			}
			arb.write_metadata_line(v_l_line_id,value,counter);
		}
	}
	else
	{
		if(check_line_identifier(feature))
		{
			arb.write_metadata_line(line_identifiers[feature],value,counter);
		}
		else
		{
			int line_id_size = line_id.size();
			std::string v_l_line_id;
			for(int j = 0; j < line_id_size; j++)
			{
				v_l_line_id.push_back(std::tolower(line_id[j]));
			}
			int feature_size = feature.size();
			v_l_line_id.push_back('_');
			for(int j = 0; j < feature_size; j++)
			{
				v_l_line_id.push_back(std::tolower(feature[j]));
			}
			arb.write_metadata_line(v_l_line_id,value,counter);
		}
	}
}

void gellisary::GAGenBank::dissectLocation(const std::string & source)
{
	if(find_word(source,"complement"))
	{
		complement = true;
	}
	std::string t_source(source);
	int t_source_size = t_source.size();
	for(int i = 0; i < t_source_size; i++)
	{
		if(!isdigit(t_source[i]))
		{
			t_source[i] = ' ';
		}
	}
	positions.clear();
	std::vector<std::string> t_positions;
	if(split_string(t_source,t_positions))
	{
		for(int i = 0; i < (int) t_positions.size(); i++)
		{
			positions.push_back(std::atoi((t_positions[i]).c_str()));
		}
	}
}

bool gellisary::GAGenBank::line_examination(const std::string & source_line)
{
	int line_size = source_line.size();
	if(line_size < 2)
	{
		logger.add_log_entry(error_line_to_short,counter_line,line_size);
		return false;
	}
	else if(line_size > 80)
	{
		logger.add_log_entry(error_line_to_long, counter_line, line_size);
		return false;
	}
	else if(line_size == 2)
	{
		if(check_line_identifier(source_line))
		{
			if(new_type == END)
		 	{
		 		if(type == SEQUENCE)
		 		{
		 			arb.write_genome_sequence(value,counter,counter_a,counter_c,counter_g,counter_t,counter_other);
		 			// Alles muss geschlossen werden.
		 		}
		 		return true;
		 	}
		 	else
		 	{
		 		// Fehler
		 		logger.add_log_entry(error_wrong_line_key,counter_line,0);
				return false;
		 	}
		}
		else
		{
			// Fehler
			logger.add_log_entry(error_wrong_line_key,counter_line,0);
			return false;
		}
	}
	else if(line_size <= 12)
	{
		if(trim(source_line) == "ORIGIN") // ORIGIN
		{
			new_type = META;
			type = META;
			line_id = "ORIGIN";
			counter = 0;
			return true;
		}
		else // Genome Sequence
		{
			new_type = SEQUENCE;
			dissectGenomeSequenceLine(source_line);
			return true;
		}
	}
	else // Normaler Fall, hier müssen drei verbliebene Zeileformate voneinander getrennt werden.
	{
		if((line_size >= 13) && (source_line[11] == ' ') && (source_line[12] != ' '))
		{
			// Meta-Zeile
			// Alles in Ordnung, gültiges Format
			new_type = META;
			dissectMetaLine(source_line);
			return true;
		}
		else if((line_size >= 11) && (source_line[8] != ' ') && (source_line[9] == ' ') && (source_line[10] != ' '))
		{
			// Genome Sequence-Zeile
			new_type = SEQUENCE;
			dissectGenomeSequenceLine(source_line);
			return true;
		}
		else if((line_size >= 22) && (source_line[20] == ' ') && (source_line[21] != ' '))
		{
			// Feature Table-Zeile
			new_type = TABLE;
			dissectTableFeatureLine(source_line);
			return true;
		}
		else
		{
			// Fehler
			logger.add_log_entry(error_wrong_line_format, counter_line,0);
			return false;
		}
	}
}

void gellisary::GAGenBank::parse()
{
	char buffer[100];
	while(!arb_file.eof())
	{
		arb_file.getline(buffer,100);
		std::string t_line(buffer);
		line_examination(t_line);
		counter_line++;
	}
}

gellisary::GAGenBank::~GAGenBank()
{

}
