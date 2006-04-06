#ifndef GADDBJ_H
#define GADDBJ_H

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

#ifndef _GLIBCXX_FSTREAM
#include <fstream>
#endif

#ifndef _GLIBCXX_MAP
#include <map>
#endif

#ifndef _GLIBCXX_CSTDLIB
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
	/*
	 * Da die DDBJ-Datei aus mehreren sogenannten sections besteht,
	 * wirft sich eine Frage auf, wie soll man die zus�tlichen Informationen
	 * abspeichern und ob man vielleicht etwas davon vernachl�ssigen
	 * kann. Jede DDBJ-Datei hat einen Haupt-Header, danach kommen
	 * '//'-Zeichen, die normalerweise das Ende der Datei gekennzeichnen.
	 * Hier aber nicht, hier sind sie sowohl das Ende der Datei als auch die
	 * Trennung zwischen einzelnen sections. Jede Sektion besitzt einen eigenen
	 * Header, eine Feature-Tabelle und eine Genome Sequence.
	 * Nach l�ngerem Diskutieren, wurde entschieden, dass man die einzelnen
	 * Genomsequenzen zu einer grossen Sequenz zusammenf�gt. Und da jede Unter-
	 * sequenz mit eins anf�ngt, brauche ich offset-Adresse, um dann location-
	 * Angaben der Genen anzupassen. Die Unterheader seien momentan zu vernach-
	 * l�ssigen. Allerdings werde ich sie ebenfalls abspeichern. Die Header-
	 * Eintr�ge der Unterheader bekommen ein Pr�fix: 
	 * 		"sub_"+section_number+<Header-Eintrag>
	 */
	class GADDBJ : public GAFile
	{
		private:
			int sequence_offset;	/*Siehe Klassenkommentar*/
			int section_number;		/*Siehe Klassenkommentar*/
			std::string genome_sequence;
			int counter_all; /*only for genome sequence, counts all chars*/
			int counter_feature;
			bool with_header;
			
			bool check_line_identifier(const std::string &);
			void dissectGenomeSequenceLine(const std::string &);
			void dissectMetaLine(const std::string &);
			void dissectTableFeatureLine(const std::string &);
			bool line_examination(const std::string &);
			void dissectLocation(const std::string &);
			void modifyLocation(const std::string &);
			void emptySequence();
			void check_and_write_metadata_line();
			
		public:
			GADDBJ(GALogger &, GAARB &, std::string &, bool);
			virtual ~GADDBJ();
			void parse();
	};
};

#endif /*GADDBJ_H*/
