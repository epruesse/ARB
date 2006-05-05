#ifndef GAFILE_H
#define GAFILE_H

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _GLIBCXX_SSTREAM
#include <sstream>
#endif

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _GLIBCXX_IOSTREAM
#include <iostream>
#endif

#ifndef _GLIBCXX_MAP
#include <map>
#endif

#ifndef _GLIBCXX_FSTREAM
#include <fstream>
#endif

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

#ifndef GALOGGER_H
#include "GALogger.h"
#endif

#ifndef GAARB_H
#include "GAARB.h"
#endif

namespace gellisary
{
	class GAFile
	{
		protected:
			std::string error_line_to_short;
			std::string error_line_to_short_for_sequence;
			std::string error_line_to_long;
			std::string error_wrong_line_key;
			std::string error_chars_234_not_empty;
			std::string error_char_0_not_empty;
			std::string error_char_1_not_empty;
			std::string error_char_0_empty;
			std::string error_char_1_empty;
			std::string error_miss_one_base;
			std::string error_char_not_empty;
			std::string error_wrong_sequence_format;
			std::string error_wrong_line_format;
			
			std::string message_to_outside_world;
			
			//std::vector<std::string> line_names; /* Die Werte findet man im Konstruktor.*/
			//std::vector<std::string> feature_names; /* Die Werte findet man im Konstruktor.*/
			
			std::map<std::string,std::string> line_identifiers;
			//std::map<std::string,std::string>::iterator line_identifiers_iter;
			
			enum line_type {SEQUENCE,END,META,TABLE,EMPTY};
			
			line_type type;
			line_type new_type;
			std::string line_id;
			std::string value; /* Bei Meta-Zeilen entspricht dem Text,
				bei "Genome Squence"-Zeilen - die Sequenz und
				bei "Table"-Zeilen - der Wert des jeweiligen Qualifiers oder 
				die unverarbeitete 'Location"-Angabe.*/
			int counter; /* Bei Sequenz ist es die aktuelle Zahl der Basen, bei 
				bei Meta (eigentlich nur bei Referenz) - die Nummer der Referenz,
				bei Table - die aktuelle Zahl der Gene.*/
			std::string feature; /* Enthält den Namen des Features.*/
			std::string qualifier; /* Enthält den Namen des Qualifiers.*/
			bool complement; /* Für die "Location"-Angabe in der Table-Zeile.*/
			std::vector<int> positions; /* Für die "Location"-Angabe in der 
				Table-Zeile.*/
			std::string flatfile_name;	/*Der Name der Flat-Datei ohne Endung.*/
			std::string flatfile_fullname;	/*Der volle Name der Flat-Datei.*/
			std::string flatfile_basename;	/*Der Pfadteil aus dem vollen Namen.*/
			std::string flatfile_name_extension;	/*Die Erweiterung des Dateinamens.*/
			bool end_of_file;
			
			int counter_a; /* Für die Sequence-Zeile,
				die aktuelle Zahl der a-Basen*/
			int counter_c; /* Für die Sequence-Zeile,
				die aktuelle Zahl der c-Basen*/
			int counter_g; /* Für die Sequence-Zeile,
				die aktuelle Zahl der g-Basen*/
			int counter_t; /* Für die Sequence-Zeile,
				die aktuelle Zahl der t-Basen*/
			int counter_other; /* Für die Sequence-Zeile,
				die aktuelle Zahl der anderen Basen*/
			int counter_line; /* Zählt die Anzahl der Zeilen mit.*/
			int counter_character; /* Zähl die Zeichen in einer Zeile.*/
			std::string name; /* Steht für den Namen des einzelnen Gens / Feature.
				Er muss einmalig innerhalb des Genoms sein.*/
	#if defined(DEBUG)			
			GALogger & logger; /* Zuständig für das Protokollieren der Fehler
				und Ausnahmen.*/
	#endif
			GAARB & arb; /* Ein Wrapper Objekt für die benötigten ARB-Funktionen.*/
			std::ifstream arb_file; /* Wie der Name der VAriable sagt, es ist 
				die Referenz auf die einzulesende Datei.*/
			
			bool find_word(const std::string &, const std::string &);
			bool split_string(const std::string & a, std::vector<std::string> & b, const char * delims = " \t\r\n");
			std::string trim(const std::string & source, const char * delims = " \t\r\n");
			//std::string trim_begin(const std::string & source, const char * delims = " \t\r\n");
			//std::string trim_end(const std::string & source, const char * delims = " \t\r\n");
			//std::string generateGeneID(const std::string & name, const std::string & type, const std::string & product = "nix", const std::string & gene = "nix");
		
		public:
	#if defined(DEBUG)
			GAFile(GALogger &, GAARB &, std::string &);
	#else
			GAFile(GAARB &, std::string &);
	#endif
			//GAFile();
			virtual ~GAFile();
			virtual void parse() = 0;
			std::string get_message();
			bool has_messages();
	};
};

#endif /*GAFILE_H*/
