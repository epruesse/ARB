#ifndef GADDBJ_H
#include "GADDBJ.h"
#endif

/*
 * Zu tun:
 * 	Sehr wichtig: in GASourceFileSwitcher soll noch die DDBJ-Datei auf
 * das Vorhandensein des Hauptheaders untersucht werden. Wenn er da ist, dann
 * wird die Datei ganz normal abgearbeitet. Wenn er aber fehlt,
 * muss wohl oder übel danach gesucht werden. Meistens wird der Hauptheader
 * in <Datei Name>.con Dateien gespeichert. Wenn diese Datei gefunden werden
 * kann, dann soll ich die beiden Dateien virtuell zusammen führen und 
 * ganz normal abarbeiten. 'Virtuell zusammen führen' bedeutet, es wird zuerst
 * die eine Dateie (<name>.con) geöffnet, abgearbeitet und geschlossen, dann
 * wird die zweite (<name>.ff) geöffnet, abgearbeitet und geschlossen. Das alles
 * ('Virtuell zusammen führen') soll ich in der Funktion: parse() behandeln.
 */
#if defined(DEBUG)
gellisary::GADDBJ::GADDBJ(GALogger & nLogger, GAARB & nARB, std::string & nARB_Filename, bool withHeader) : GAFile(nLogger, nARB, nARB_Filename)
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
	line_identifiers["PUBMED"] = "reference_pubmed";
	line_identifiers["CONSRTM"] = "reference_consrtm"; // may be 'consortium'
	line_identifiers["COMMENT"] = "comments_or_notes";
	line_identifiers["CONTIG"] = "contig";
	line_identifiers["//"] = "termination";
	
	type = EMPTY;
	complement = false;
	with_header = withHeader;
	
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
	counter_line = 1;
	counter = 0;
	counter_character = 0;
	line_id = "";
	feature = "";
	sequence_offset = 0;
	section_number = 0;
	genome_sequence = "";
	counter_all = 0;
	counter_feature = 0;
}
#else
gellisary::GADDBJ::GADDBJ(GAARB & nARB, std::string & nARB_Filename, bool withHeader) : GAFile(nARB, nARB_Filename)
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
	line_identifiers["PUBMED"] = "reference_pubmed";
	line_identifiers["CONSRTM"] = "reference_consrtm"; // may be 'consortium'
	line_identifiers["COMMENT"] = "comments_or_notes";
	line_identifiers["CONTIG"] = "contig";
	line_identifiers["//"] = "termination";
	
	type = EMPTY;
	complement = false;
	with_header = withHeader;
	
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
	counter_line = 1;
	counter = 0;
	counter_character = 0;
	line_id = "";
	feature = "";
	sequence_offset = 0;
	section_number = 0;
	genome_sequence = "";
	counter_all = 0;
	counter_feature = 0;
}
#endif

bool gellisary::GADDBJ::check_line_identifier(const std::string & source_line_identifier)
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

void gellisary::GADDBJ::dissectGenomeSequenceLine(const std::string & source_line)
{
	if(type == META)
	{
		//emptySequence();
		type = SEQUENCE;
		line_id = "SE";
		type = SEQUENCE;
		//counter_all = 0;
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
	#if defined(DEBUG)
				logger.add_log_entry(error_char_not_empty+" = "+source_line.substr(i,1), counter_line, i);
	#endif
				break;
			}
			else if((i >= 0) && (i <= 8))
			{
				if(!std::isdigit(source_line[i]))
				{
					if(!std::isspace(source_line[i]))
					{
						// Fehler
	#if defined(DEBUG)
						logger.add_log_entry(error_wrong_sequence_format+" tja = "+source_line, counter_line, i);
	#endif
						break;
					}
				}
			}
			else
			{
				if(mustBeEnd)
				{
					// Fehler
	#if defined(DEBUG)
					logger.add_log_entry(error_wrong_sequence_format+" 2 = "+source_line.substr(i,1), counter_line, i);
	#endif
					break;
				}
				else
				{
					if(std::isalpha(source_line[i]))
					{
						counter_all++;
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
						genome_sequence.push_back(t_base);
					}
				}
			}
		}
	}
}

void gellisary::GADDBJ::emptySequence()
{
	counter_all = 0;
	genome_sequence = "";
	counter_a = 0;
	counter_c = 0;
	counter_g = 0;
	counter_t = 0;
	counter_other = 0;
}

void gellisary::GADDBJ::dissectMetaLine(const std::string & source_line)
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
	/*if(type == END)
	{
		arb.create_new_genome();
		type = EMPTY;
	}*/
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
	#if defined(DEBUG)
			logger.add_log_entry(error_wrong_line_format, counter_line,0);
	#endif
		}
		type = META;
	}
	else if(type == EMPTY)
	{
		t_line_t = trim(t_line_id);
		if(t_line_t == "LOCUS")
		{
			arb.write_metadata_line("source_database", "ddbj", 0);
			arb.write_metadata_line("flatfile_name", flatfile_name,0);
			line_id = t_line_t;
			value = t_line;
		}
		else
		{
			// Fehler
	#if defined(DEBUG)
			logger.add_log_entry(error_wrong_line_key, counter_line,0);
	#endif
		}
		type = META;
	}
	else if(type == TABLE)
	{
		t_line_t = trim(t_line_id);
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
			if(section_number == 0)
			{
				arb.write_metadata_line(qualifier,value,0);
			}
		}
		type = META;
		line_id = t_line_t;
		value = t_line;
	}
	else if(type == END)
	{
		t_line_t = trim(t_line_id);
		line_id = t_line_t;
		value = t_line;
		type = META;
	}
}


void gellisary::GADDBJ::dissectTableFeatureLine(const std::string & source_line)
{
	std::string t_key = source_line.substr(0,21);
	std::string t_qualifier_line = trim(source_line.substr(21), " \n\t\r\"");
	std::string::size_type t_pos;
	std::string::size_type t_none_pos;
	std::string::size_type t_none_pos2;
	if(type == TABLE) // die vorherige Zeile war Table Feature
	{
		/*
		 * if() wird ausgeführt, wenn die ersten fünf Zeichen leer sind, das entspricht dem Format der Table Feature-Zeile
		 */
		if((t_key[0] == ' ') && (t_key[1] == ' ') && (t_key[2] == ' ') && (t_key[3] == ' ') && (t_key[4] == ' '))
		{
			/*
			 * wenn das fünfte Zeichen leer ist, dann ist es nicht der Anfang eines neuen Genes,
			 * sondern die aktuelle Zeile gehört noch dem aktuellen Gen an.
			 */
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
						t_none_pos2 = t_qualifier.find("-");
						if((t_none_pos == std::string::npos) && (t_none_pos2 == std::string::npos))
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
									if(section_number > 0)
									{
										std::stringstream t_name;
										t_name << "contig_" << section_number << "_" << qualifier;
										arb.write_metadata_line(t_name.str(),value,0);
									}
									else
									{
										arb.write_metadata_line(qualifier,value,0);
									}
								}
								qualifier = t_qualifier;
								value = t_value;
							}
						}
						else
						{
							// Fehler
							value += t_qualifier_line;
	#if defined(DEBUG)
							logger.add_log_entry(error_wrong_line_format,counter_line,0);
	#endif
						}
					}
					else
					{
						std::string t_qualifier = t_qualifier_line.substr(1);
						t_none_pos = t_qualifier.find(" ");
						if(t_none_pos == std::string::npos)
						{
							std::string t_value = trim(t_qualifier);
							if(t_value != qualifier)
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
								qualifier = t_value;
								value = "1";
							}
						}
						else
						{
							// Fehler
							//value += t_qualifier_line;
	#if defined(DEBUG)
							logger.add_log_entry(error_wrong_line_format,counter_line,0);
	#endif
						}
					}
				}
				else // Tritt bei Qualifier 'translation' zu Tage.
				{
					value += t_qualifier_line;
				}
			}
			else if(t_key[5] != ' ') // es muss eine Table Feature Key kommen.
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
						if(section_number > 0)
						{
							std::stringstream t_name;
							t_name << "contig_" << section_number << "_" << qualifier;
							arb.write_metadata_line(t_name.str(),value,0);
						}
						else
						{
							arb.write_metadata_line(qualifier,value,0);
						}
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
					counter_feature++;
					qualifier = "location";
					complement = false;
					dissectLocation(t_qualifier_line);
					//value = t_qualifier_line;
					modifyLocation(t_qualifier_line);
					std::stringstream t_name;
					t_name << feature << "_" << counter_feature;
					name = t_name.str();
					arb.write_next_gene(name, feature, value, positions, complement, counter_feature);
		   		}
		    }
		    else
			{
				// Fehler
	#if defined(DEBUG)
				logger.add_log_entry(error_wrong_line_format, counter_line,counter_character);
	#endif
			}
		}
		else
		{
			// Fehler
	#if defined(DEBUG)
			logger.add_log_entry(error_wrong_line_format, counter_line,counter_character);
	#endif
		}
	}
	else if(type == META) // die vorherige Zeile war eine Meta-Zeile
	{
		if(trim(t_key) == "FEATURES")
		{
			check_and_write_metadata_line();
		}
		else
		{
			// Fehler
	#if defined(DEBUG)
			logger.add_log_entry(error_wrong_line_key, counter_line,counter_character);
	#endif
		}
	}
	type = TABLE;
	line_id = "FEATURES";
}

void gellisary::GADDBJ::check_and_write_metadata_line()
{
	if(feature == "")
	{
		if(check_line_identifier(line_id))
		{
			if(section_number > 0)
			{
				std::stringstream t_name;
				t_name << "contig_" << section_number << "_" << line_identifiers[line_id];
				arb.write_metadata_line(t_name.str(),value,counter);
			}
			else
			{
				arb.write_metadata_line(line_identifiers[line_id],value,counter);
			}
		}
		else
		{
			int line_id_size = line_id.size();
			std::string v_l_line_id;
			for(int j = 0; j < line_id_size; j++)
			{
				v_l_line_id.push_back(std::tolower(line_id[j]));
			}
			if(section_number > 0)
			{
				std::stringstream t_name;
				t_name << "contig_" << section_number << "_" << v_l_line_id;
				arb.write_metadata_line(t_name.str(),value,counter);
			}
			else
			{
				arb.write_metadata_line(v_l_line_id,value,counter);
			}
		}
	}
	else
	{
		if(check_line_identifier(feature))
		{
			if(section_number > 0)
			{
				std::stringstream t_name;
				t_name << "contig_" << section_number << "_" << line_identifiers[feature];
				arb.write_metadata_line(t_name.str(),value,counter);
			}
			else
			{
				arb.write_metadata_line(line_identifiers[feature],value,counter);
			}
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
			if(section_number > 0)
			{
				std::stringstream t_name;
				t_name << "contig_" << section_number << "_" << v_l_line_id;
				arb.write_metadata_line(t_name.str(),value,counter);
			}
			else
			{
				arb.write_metadata_line(v_l_line_id,value,counter);
			}
		}
	}
}

void gellisary::GADDBJ::dissectLocation(const std::string & source)
{
	if(find_word(source,"complement"))
	{
		complement = true;
	}
	std::string t_source(source);
	int t_source_size = t_source.size();
	for(int i = 0; i < t_source_size; i++)
	{
		if(!std::isdigit(t_source[i]))
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
			positions.push_back((std::atoi((t_positions[i]).c_str())+sequence_offset));
		}
	}
}

void gellisary::GADDBJ::modifyLocation(const std::string & source)
{
	std::string t_source_pre;
	int source_size = source.size();
	bool same_digit = false;
	for(int i = 0; i < source_size; i++)
	{
		if(!std::isdigit(source[i]))
		{
			t_source_pre.push_back(source[i]);
			same_digit = false;
		}
		else
		{
			if(!same_digit)
			{
				t_source_pre.push_back('|');
				same_digit = true;
			}
		}
	}
	std::string t_source(source);
	int t_source_size = t_source.size();
	for(int i = 0; i < t_source_size; i++)
	{
		if(!std::isdigit(t_source[i]))
		{
			t_source[i] = ' ';
		}
	}
	std::vector<std::string> t_positions;
	std::stringstream t_source_post;
	if(split_string(t_source,t_positions))
	{
		int j = 0;
		for(int i = 0; i < (int) t_source_pre.size(); i++)
		{
			if(t_source_pre[i] == '|')
			{
				if(j < (int) t_positions.size())
				{
					t_source_post << (std::atoi((t_positions[j++]).c_str())+sequence_offset);
				}
			}
			else
			{
				t_source_post << t_source_pre[i];
			}
		}
		value = t_source_post.str();
	}
}

bool gellisary::GADDBJ::line_examination(const std::string & source_line)
{
	int line_size = source_line.size();
	if(line_size < 2)
	{
		if(type != END)
		{
	#if defined(DEBUG)
			logger.add_log_entry(error_line_to_short,counter_line,line_size);
	#endif
			return false;
		}
		else
		{
			return true;
		}
	}
	/*else if(line_size > 80)
	{
	#if defined(DEBUG)
		logger.add_log_entry(error_line_to_long, counter_line, line_size);
	#endif
		return false;
	}*/
	else if(line_size == 2)
	{
		if(check_line_identifier(source_line))
		{
			if(new_type == END)
		 	{
		 		if(type == META)
		 		{
		 			if(feature != "" || line_id != "")
					{
						check_and_write_metadata_line();
					}
					line_id = "";
					feature = "";
					counter = 0;
			 	}
			 	
		 		section_number++;
		 		type = END;
		 		sequence_offset = counter_all;
		 		return true;
		 	}
		 	else
		 	{
		 		// Fehler
	#if defined(DEBUG)
		 		logger.add_log_entry(error_wrong_line_key,counter_line,counter_character);
	#endif
				return false;
		 	}
		}
		else
		{
			// Fehler
	#if defined(DEBUG)
			logger.add_log_entry(error_wrong_line_key,counter_line,counter_character);
	#endif
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
		else
		{
			return false;
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
			if(!find_word(source_line,"BASE COUNT"))
			{
				// Fehler
	#if defined(DEBUG)
				logger.add_log_entry(error_wrong_line_format, counter_line,counter_character);
	#endif
			}
			return false;
		}
	}
	return false;
}

void gellisary::GADDBJ::parse()
{
	char buffer[100];
	if(!with_header)
	{
		
		std::ifstream con_file((flatfile_basename+"/"+flatfile_name+".con").c_str());
		if(con_file.is_open())
		{
			while(!con_file.eof())
			{
				con_file.getline(buffer,100);
				std::string t_line(buffer);
				line_examination(t_line);
				counter_line++;
			}
			con_file.close();
		}
		else
		{
			// Fehler
	#if defined(DEBUG)
			logger.add_log_entry("the file : "+flatfile_basename+"/"+flatfile_name+".con is not exists", counter_line,0);
	#endif
		}
		type = END;
	}
	while(!arb_file.eof())
	{
		arb_file.getline(buffer,100);
		std::string t_line(buffer);
		line_examination(t_line);
		counter_line++;
	}
	arb_file.close();
	if(type == END)
	{
		arb.write_genome_sequence(genome_sequence,counter_all,counter_a,counter_c,counter_g,counter_t,counter_other);
	}
}

gellisary::GADDBJ::~GADDBJ(){}
