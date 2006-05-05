#ifndef GAEMBL_H
#include "GAEmbl.h"
#endif

#if defined(DEBUG)
gellisary::GAEmbl::GAEmbl(GALogger & nLogger, GAARB & nARB, std::string & nARB_Filename) : GAFile(nLogger, nARB, nARB_Filename)
{
	line_identifiers["ID"] = "identification";
	line_identifiers["AC"] = "accession_number";
	line_identifiers["SV"] = "sequence_version";
	line_identifiers["DT"] = "date";
	line_identifiers["DE"] = "description";
	line_identifiers["KW"] = "keyword";
	line_identifiers["OS"] = "organism_species";
	line_identifiers["OC"] = "organism_classification";
	line_identifiers["OG"] = "organelle";
	line_identifiers["RN"] = "reference_number";
	line_identifiers["RC"] = "reference_comment";
	line_identifiers["RP"] = "reference_positions";
	line_identifiers["RX"] = "reference_cross_reference";
	line_identifiers["RG"] = "reference_group";
	line_identifiers["RA"] = "reference_author";
	line_identifiers["RT"] = "reference_title";
	line_identifiers["RL"] = "reference_location";
	line_identifiers["DR"] = "database_cross_reference";
	line_identifiers["AH"] = "assemlby_header";
	line_identifiers["AS"] = "assembly_information";
	line_identifiers["CO"] = "contig_construct";
	line_identifiers["FH"] = "feature_table_header";
	line_identifiers["FT"] = "feature_table_data";
	line_identifiers["SQ"] = "sequence_header";
	line_identifiers["CC"] = "comments_or_notes";
	line_identifiers["XX"] = "spacer_line";
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
	counter_line = 1;
	counter = 0;
	counter_character = 0;
}
#else
gellisary::GAEmbl::GAEmbl(GAARB & nARB, std::string & nARB_Filename) : GAFile(nARB, nARB_Filename)
{
	line_identifiers["ID"] = "identification";
	line_identifiers["AC"] = "accession_number";
	line_identifiers["SV"] = "sequence_version";
	line_identifiers["DT"] = "date";
	line_identifiers["DE"] = "description";
	line_identifiers["KW"] = "keyword";
	line_identifiers["OS"] = "organism_species";
	line_identifiers["OC"] = "organism_classification";
	line_identifiers["OG"] = "organelle";
	line_identifiers["RN"] = "reference_number";
	line_identifiers["RC"] = "reference_comment";
	line_identifiers["RP"] = "reference_positions";
	line_identifiers["RX"] = "reference_cross_reference";
	line_identifiers["RG"] = "reference_group";
	line_identifiers["RA"] = "reference_author";
	line_identifiers["RT"] = "reference_title";
	line_identifiers["RL"] = "reference_location";
	line_identifiers["DR"] = "database_cross_reference";
	line_identifiers["AH"] = "assemlby_header";
	line_identifiers["AS"] = "assembly_information";
	line_identifiers["CO"] = "contig_construct";
	line_identifiers["FH"] = "feature_table_header";
	line_identifiers["FT"] = "feature_table_data";
	line_identifiers["SQ"] = "sequence_header";
	line_identifiers["CC"] = "comments_or_notes";
	line_identifiers["XX"] = "spacer_line";
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
	counter_line = 1;
	counter = 0;
	counter_character = 0;
}
#endif

bool gellisary::GAEmbl::check_line_identifier(const std::string & source_line_identifier)
{
	std::map<std::string,std::string>::iterator line_identifiers_iter = line_identifiers.find(source_line_identifier);
	if(line_identifiers_iter != line_identifiers.end()) // wenn etwas gefunden wurde
	{
		if(source_line_identifier == "//")
		{
			new_type = END;
		}
		else if(source_line_identifier == "XX")
		{
			new_type = EMPTY;
		}
		else if((source_line_identifier == "FT") || (source_line_identifier == "FH"))
		{
			new_type = TABLE;
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

void gellisary::GAEmbl::dissectGenomeSequenceLine(const std::string & source_line)
{
	if(type == META)
	{
		emptySequence();
		type = SEQUENCE;
		line_id = "SE";
	}
	bool mustBeEnd = false;
	char t_base;
	
	for(int i = 5; i < (int) source_line.size(); i++)
	{
		if(source_line[i] == ' ')
		{
			if(((i != 15) && (i != 26) && (i != 37) && (i != 48) && (i != 59) && (i != 70)) || (i >= 78))
			{
				if(!mustBeEnd)
				{
					mustBeEnd = true;
				}
			}
		}
		else
		{
			if((i == 15) || (i == 26) || (i == 37) || (i == 48) || (i == 59) || (i == 70))
			{
				// Fehler
	#if defined(DEBUG)
				logger.add_log_entry(error_char_not_empty+" = "+source_line.substr(i,1), counter_line, i);
	#endif
				break;
			}
			else if((i > 70) && (i <= 79))
			{
				if(!std::isdigit(source_line[i]))
				{
					// Fehler
	#if defined(DEBUG)
					logger.add_log_entry(error_wrong_sequence_format+" = "+source_line.substr(i,1), counter_line, i);
	#endif
					break;
				}
			}
			else
			{
				if(mustBeEnd)
				{
					// Fehler
	#if defined(DEBUG)
					logger.add_log_entry(error_wrong_sequence_format+" = "+source_line.substr(i,1), counter_line, i);
	#endif
					break;
				}
				else
				{
					if(std::isalpha(source_line[i]))
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

void gellisary::GAEmbl::dissectMetaLine(const std::string & source_line)
{
	std::string t_line_id = source_line.substr(0,2);
	std::string t_pre_line = source_line.substr(5);
	std::string t_line = trim(t_pre_line);
	
	std::string::size_type space_pos = t_line.find(" ");
	bool has_space = true;
	if(space_pos == std::string::npos)
	{
		has_space = false;
	}
	if(type == META)
	{
		if(t_line_id == "DT")
		{
			if(line_id == "DT")
			{
				arb.write_metadata_line("date_of_creation", value,0);
				type = META;
				line_id = t_line_id;
				value = t_line;
			}
		}
		else if(t_line_id == line_id)
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
		else if(t_line_id == "RN")
		{
			type = META;
			line_id = t_line_id;
			value = trim(t_line," []");
			counter = std::atoi(value.c_str());
		}
		else if(t_line_id == "OS")
		{
			arb.write_metadata_line("full_name", t_line,0);
			type = META;
			line_id = t_line_id;
			value = t_line;
		}
		else if(t_line_id == "AC")
		{
			arb.write_metadata_line("acc", t_line,0);
			type = META;
			line_id = t_line_id;
			value = t_line;
		}
		else if(source_line[0] == 'R')
		{
			if(line_id != "RN")
			{
				arb.write_metadata_line(line_identifiers[line_id], value, counter);
				type = META;
				line_id = t_line_id;
				value = t_line;
			}
			else
			{
				type = META;
				line_id = t_line_id;
				value = t_line;
			}
		}
		else
		{
			arb.write_metadata_line(line_identifiers[line_id], value,0);
			type = META;
			line_id = t_line_id;
			value = t_line;
		}
	}
	else if(type == EMPTY)
	{
		if(t_line_id == "RN")
		{
			type = META;
			line_id = t_line_id;
			value = trim(t_line," []");
			counter = std::atoi(value.c_str());
		}
		else if(t_line_id == "AC")
		{
			type = META;
			line_id = t_line_id;
			value = trim(t_line," \t\r\n;");
			arb.write_metadata_line("acc", value,0);
		}
		else if(t_line_id == "DT")
		{
			type = META;
			line_id = t_line_id;
			value = t_line;
		}
		else if(t_line_id == "ID")
		{
			arb.write_metadata_line("source_database", "embl",0);
			arb.write_metadata_line("flatfile_name", flatfile_name+"."+flatfile_name_extension,0);
			type = META;
			line_id = t_line_id;
			value = t_line;
		}
		else if(t_line_id == "OS")
		{
			arb.write_metadata_line("full_name", t_line,0);
			type = META;
			line_id = t_line_id;
			value = t_line;
		}
		else
		{
			type = META;
			line_id = t_line_id;
			value = t_line;
		}
	}
	else if(type == TABLE)
	{
		arb.write_qualifier(qualifier,value);
		arb.close();
		type = META;
		line_id = t_line_id;
		value = t_line;
	}
}


void gellisary::GAEmbl::dissectTableFeatureLine(const std::string & source_line)
{
	std::string t_key = source_line.substr(5,16);
	std::string t_qualifier_line = trim(source_line.substr(20), " \n\t\r\"");
	std::string::size_type t_pos;
	std::string::size_type t_none_pos;
	std::string::size_type t_none_pos2;
	if(type == EMPTY)
	{
		if(source_line.substr(0,2) != "FH")
		{
			if(trim(t_key) == "source")
			{
				feature = "source";
				counter = 0;
				qualifier = "location";
				value = t_qualifier_line;
			}
			else
			{
				// Fehler
	#if defined(DEBUG)
				logger.add_log_entry(error_wrong_line_format,counter_line,0);
	#endif
			}
		}
	}
	else if(type == TABLE)
	{
		if(t_key[0] == ' ')
		{
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
							value += " "+t_value;
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
								if(qualifier != "location")
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
					t_none_pos = t_qualifier_line.find(" ");
					if(t_none_pos == std::string::npos)
					{
						t_none_pos = t_qualifier_line.find(":");
						if(t_none_pos == std::string::npos)
						{
							if(t_qualifier_line != qualifier)
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
								qualifier = t_qualifier_line;
								value = "1";
							}
						}
						else
						{
							value += t_qualifier_line;
						}
					}
					else
					{
						// Fehler
						//value += t_qualifier_line;
						value += t_qualifier_line;
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
		else
		{
			// feature key with location
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
			dissectLocation(value);
			//name = generateGeneID(value,feature);
			//std::stringstream t_name;
			//t_name << feature << "_" << counter;
			//name = t_name.str();
			arb.write_next_gene(feature, value, positions, complement, counter);
		}
	}
	else if(type == META)
	{
		if(source_line.substr(0,2) != "FH")
		{
			if(trim(t_key) == "source")
			{
				feature = "source";
				counter = 0;
				qualifier = "location";
				value = t_qualifier_line;
			}
			else
			{
				// Fehler
			}
		}
	}
	type = TABLE;
	line_id = "FT";
}

/*
 * Gerade habe ich die Funktion f�r das Suchen eines ganzen Wortes innerhalb
 * einer Zeichenkette geschrieben.
 * Nun soll ich die split-Funktion schreiben.
 * Ausserdem will ich die location-Angabe durchgehen und jedes Zeichen, das
 * keine Ziffer ist durch Leerzeichen ersetzen, damit ich location sp�ter
 * leichter split-ten kann.
 */
void gellisary::GAEmbl::dissectLocation(const std::string & source)
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
			positions.push_back(std::atoi((t_positions[i]).c_str()));
		}
	}
}

void gellisary::GAEmbl::emptyMeta()
{
	counter = 0;
	type = EMPTY;
	line_id = "XX";
	value = "";
}

void gellisary::GAEmbl::emptyTable()
{
	arb.close();
	counter = 0;
	type = EMPTY;
	line_id = "XX";
	value = "";
	qualifier = "";
	complement = false;
	name = "";
	positions.clear();
}

void gellisary::GAEmbl::emptySequence()
{
	counter = 0;
	type = EMPTY;
	line_id = "XX";
	value = "";
	counter_a = 0;
	counter_c = 0;
	counter_g = 0;
	counter_t = 0;
	counter_other = 0;
}

bool gellisary::GAEmbl::line_examination(const std::string & source_line)
{
	end_of_file = false;
	int line_size = source_line.size();
	if(line_size < 2)
	{
		end_of_file = true;
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
	else if(line_size == 2 || line_size < 6)
	{
		std::string t_line_id = source_line.substr(0,2);
		if(check_line_identifier(t_line_id))
		{
			/* 
			 * Wenn der Zeiletyp 'XX' ist, muss das bereits gespeichert
			 * in ARB reingeschrieben werden. Meist der Inhalt einer Meta-Zeile.
			 * Und dann zu der n�chsten Zeile.
			 * Wenn - '//' beteutet das, dass es das Ende der Datei ist und
			 * die Genomsequenz muss dann reingeschrieben werden, ausserdem soll
			 * die Datei deschlossen werden. Ende des Programms.
			 * Bei 'FH' muss nichts detan werden, als die n�chste Zeile
			 * eingelesen werden.
			 */
			
		 	if(new_type == END)
		 	{
		 		if(type == SEQUENCE)
		 		{
		 			arb.write_genome_sequence(value,counter,counter_a,counter_c,counter_g,counter_t,counter_other);
		 			emptySequence();
		 			// Alles muss geschlossen werden.
		 			end_of_file = true;
		 		}
		 		type = END;
		 	}
		 	else if(new_type == EMPTY)
		 	{
	 			if(type == META)
	 			{
	 				if(line_id[0] == 'R')
	 				{
	 					if(arb.write_metadata_line(line_identifiers[line_id],value,counter))
	 					{
	 						emptyMeta();
	 					}
	 				}
	 				else if(line_id == "DT")
	 				{
	 					arb.write_metadata_line("date_of_last_update", value,0);
						emptyMeta();
					}
	 				else
	 				{
	 					if(arb.write_metadata_line(line_identifiers[line_id],value,0))
	 					{
	 						emptyMeta();
	 					}
	 				}
	 			}
	 			else if(type == TABLE)
	 			{
	 				if(qualifier != "location" && t_line_id != "TH")
	 				{
	 					arb.write_qualifier(qualifier,value);
	 					emptyTable();
	 				}
	 				else
	 				{
	 					arb.close();
	 				}
	 			}
		 	}
			return true;
		}
		else
		{
	#if defined(DEBUG)
			logger.add_log_entry(error_wrong_line_key,counter_line,0);
	#endif
			return false;
		}
	}
	else // Normaler Fall, hier m�ssen drei verbliebene Zeileformate voneinander getrennt werden.
	{
		if((source_line[2] == ' ') && (source_line[3] == ' ') && (source_line[4] == ' '))
		{
			// Alles in Ordnung, g�ltiges Format
			char character_0 = source_line[0];
			char character_1 = source_line[1];
			if(character_0 == ' ')
			{
				if(character_1 == ' ')
				{
					if(source_line.size() == 80)
					{
						// Muss die "Genome Sequence"-Zeile sein
						new_type = SEQUENCE;
						dissectGenomeSequenceLine(source_line);
						return true;
					}
					else
					{
						// Fehler
	#if defined(DEBUG)
						logger.add_log_entry(error_line_to_short_for_sequence,counter_line,0);
	#endif
						return false;
					}
				}
				else
				{
					// Fehler
	#if defined(DEBUG)
					logger.add_log_entry(error_char_1_not_empty,counter_line,0);
	#endif
					return false;
				}
			}
			else
			{
				if(character_1 != ' ')
				{
					// Muss entweder die Meta-Zeile oder die "Table Feature / Header"-Zeile sein
					/* 
					 * Zeilenidentifierer : first_character und second_character
					 * werden bereits f�r sp�tere Benutzung gespeichert.
					 */
					//third_examination(source_line);
					//second_and_half_examination(source_line);
					if(character_0 == 'F')
					{
						if(character_1 == 'T')
						{
							// Muss "Table Feature"-Zeile sein.
							new_type = TABLE;
							dissectTableFeatureLine(source_line);
							return true;
						}
						else if(character_1 == 'H')
						{
							// Es ist die erste "Table Header"-Zeile.
							new_type = TABLE;
							return true;
						}
						else
						{
							// Fehler
	#if defined(DEBUG)
							logger.add_log_entry(error_wrong_line_key,counter_line,0);
	#endif
							return false;
						}
					}
					else
					{
						// Es ist eine Meta-Zeile.
						new_type = META;
						dissectMetaLine(source_line);
						return true;
					}
				}
				else
				{
					// Fehler
	#if defined(DEBUG)
					logger.add_log_entry(error_char_1_empty,counter_line,0);
	#endif
					return false;
				}
			}
		}
		else
		{
			// Fehler
	#if defined(DEBUG)
			logger.add_log_entry(error_chars_234_not_empty, counter_line,0);
	#endif
			return false;
		}
	}
}

void gellisary::GAEmbl::parse()
{
	char buffer[240];
	while(!arb_file.eof())
	{
		arb_file.getline(buffer,240);
		std::string t_line(buffer);
		line_examination(t_line);
		counter_line++;
	}
	if(!end_of_file)
	{
		if(value != "")
		{
			arb.write_genome_sequence(value,counter,counter_a,counter_c,counter_g,counter_t,counter_other);
			emptySequence();
		}
		message_to_outside_world = "the flatfile "+flatfile_fullname+" is not complete";
	}
	arb_file.close();
}

gellisary::GAEmbl::~GAEmbl()
{
	
}
