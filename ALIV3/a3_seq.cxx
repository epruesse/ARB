// -----------------------------------------------------------------------------
//	Include-Dateien
// -----------------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>

#include "a3_basen.h"
#include "a3_seq.hxx"

using std::ifstream;
using std::cout;

// -----------------------------------------------------------------------------
//	Erzeugen einer Zufallssequenz mit vorgebener Laenge
// -----------------------------------------------------------------------------
	static str RandomSequence ( uint len )
// -----------------------------------------------------------------------------
{
	str seq = new char [len + 1];

	if (seq)
	{
		static short init = 0;
		uint		 pos  = 0;
		
		if (!init)	// Zufallszahlengenerator nur einmal initialisieren
		{
			srand((int)time(NULL));	// Moeglichst zufaellige Initialiserung
//			srand(1);			// Immer die selbe Sequenz
			init = 1;
		}
		
		while (pos < len)
		    seq[pos++] = BCharacter[rand() % BASEN];	// Sequenz mit zufaellig aus-
														// gewaehlten Basen besetzen
		seq[pos] = 0;
	}
	
	return seq;
}

// -----------------------------------------------------------------------------
//	Liefert die Kopie einer Sequenz mit vorgegbener Laenge, aus der
//	alle ueberfluessigen und unzulaessigen Zeichen entfernt wurden.
// -----------------------------------------------------------------------------
	static str CompressSequence ( str  sequence,
								  uint length )
// -----------------------------------------------------------------------------
{
	str compressed = NULL;
	
	if (sequence && length)
	{
		str tmp = new char [length + 1];
		
		if (tmp)
		{
			str ptr = tmp;
			
			while (*sequence)	// Zeichenweises Lesen der Sequenz
			{
				switch (BIndex[*sequence])	// Bearbeitung ist abhaengig vom
				{							// Typ des jeweiligen Zeichens
					case ADENIN:
					case CYTOSIN:
					case GUANIN:
					case URACIL:
					case ONE:				// Zulaessige Zeichen werden uebrnommen
					{
						*ptr++ = *sequence;
						break;
					}
					case ANY:					// Aufeinanderfolgende, nicht sequenzierte
					{							// Bereiche werden zusammengefasst.

						if ((ptr == tmp) ||		// Nur ein ANY-Zeichen
							((ptr > tmp) &&
							 (BIndex[*(ptr - 1)] != ANY))) *ptr++ = *sequence;

						while(*sequence &&		// Ueberspringen
							  BIndex[*sequence] == ANY) sequence++;

						sequence--;
						break;
					}
					case INSERT:	break;		// Insertionen werden uebersprungen
					case INVALID:
					default:					// Ungueltige oder sonstige Zeichen werden
					{							// als eine beliebige Base dargestellt
						*ptr++ = BCharacter[ONE];
						break;
					}
				}
				
				sequence++;		// Naechste Base der Sequenz
			}

			*ptr = 0;			// Neue Sequnez mit 0 abschliessen

			compressed = strdup(tmp);	// Kopie der neuen Sequenz
			
			delete tmp;
		}
	}
	
	return compressed;
}

// -----------------------------------------------------------------------------
//	Einlesen einer Sequenz aus eine vorgegebene
//	Datei mit einer vorgegebene Zeilennummer
// -----------------------------------------------------------------------------
	static str ReadSequence ( str  file,
							  uint line )
// -----------------------------------------------------------------------------
{
	str seq = NULL;
	
	if (file && *file)
	{
		ifstream input (file);
		
		if (input)
		{
			int error = 0;
			str tmp   = NULL;
			
			while (!error && line)		// Ueberlesen bis zur gewuenschten Zeile
			{
				if (tmp) delete tmp, tmp = NULL;
				
//				if (!input.gets(&tmp)) error = 1;

				line--;
			}

			if (error && tmp)
			{
				if		(line)		   delete tmp, tmp = NULL;
				else if (!input.eof()) delete tmp, tmp = NULL;
			}

			if (((seq = tmp)			  != NULL) &&			// Sequenz zurueckgeben
				((tmp = strchr(seq,'\n')) != NULL)) *tmp = 0;	// Vorher LF entfernen
		}
	}

	return seq;
}

// -----------------------------------------------------------------------------
//	Umwandeln der T-Zeichen einer Sequenz in U-Zeichen
// -----------------------------------------------------------------------------
	static void T2U ( str sequence )
// -----------------------------------------------------------------------------
{
	int base;
	
	while ((base = *sequence) != 0)		// Zeichenweises Abarbeiten der Sequenz
	{
		if (BIndex[base] == URACIL)			// Alle Vorkommen von URACIL durch
		    *sequence = BCharacter[URACIL];	// dasselbe Zeichen ersetzen

		sequence++;
	}
}

// -----------------------------------------------------------------------------
//	Konstruktor zum Erzeugen einer leeren Instanz der Klasse Sequenz
// -----------------------------------------------------------------------------
	Sequence::Sequence ( void )
// -----------------------------------------------------------------------------
{
	original   = NULL;
	origlen	   = 0;
	compressed = NULL;
	complen    = 0;
}
// -----------------------------------------------------------------------------
//	Konstruktor zum Erzeugen einer Instanz der
//	Klasse Sequenz aus einer vorgegebenen Sequenz
// -----------------------------------------------------------------------------
	Sequence::Sequence ( str seq )
// -----------------------------------------------------------------------------
{
	original   = NULL;
	origlen	   = 0;
	compressed = NULL;
	complen    = 0;

	if (seq) original = strdup(seq);	// Sequenz kopieren
	
	if (original)
	{
		origlen = strlen(original);

		T2U(original);		// T- nach U-Zeichen umwandeln

													// Sequenz kompremieren		
		if ((compressed = CompressSequence(original,origlen)) != NULL)
		    complen = strlen(compressed);
	}
}

// -----------------------------------------------------------------------------
//	Konstruktor zum Erzeugen einer zufaellig besetzten
//	Instanz der	Klasse Sequenz mit vorgebener Sequenzlaenge
// -----------------------------------------------------------------------------
	Sequence::Sequence ( uint len )
// -----------------------------------------------------------------------------
{
	original   = NULL;
	origlen	   = 0;
	compressed = NULL;
	complen    = 0;
	
	if ((original = RandomSequence(len)) != NULL)	// Zufaellige Sequenz erzeugen
	{
		origlen = len;
													// Sequenz kompremieren
		if ((compressed = CompressSequence(original,origlen)) != NULL)
		    complen = strlen(compressed);
	}
}

// -----------------------------------------------------------------------------
//	Konstruktor zum Erzeugen einer Instanz der Klasse Sequenz
//	aus einer vorgebener Datei mit vorgegebener Zeilennummer
// -----------------------------------------------------------------------------
	Sequence::Sequence ( str  file,
						 uint line )
// -----------------------------------------------------------------------------
{
	original   = NULL;
	origlen	   = 0;
	compressed = NULL;
	complen    = 0;
	
	if ((original = ReadSequence(file,line)) != NULL)	// Sequenz einlesen
	{
		origlen = strlen(original);

		T2U(original);		// T- nach U-Zeichen umwandeln

													// Sequenz kompremieren		
		if ((compressed = CompressSequence(original,origlen)) != NULL)
		    complen = strlen(compressed);
	}
}

// -----------------------------------------------------------------------------
//	Kopierkonstruktor der Klasse Sequenz
// -----------------------------------------------------------------------------
	Sequence::Sequence ( Sequence &sequence )
// -----------------------------------------------------------------------------
{
	if (sequence.original) original = strdup(sequence.original);
	origlen = sequence.origlen;
	
	if (sequence.compressed) compressed = strdup(sequence.compressed);
	complen = sequence.complen;
}

// -----------------------------------------------------------------------------
//	Destruktor der Klasse Sequenz
// -----------------------------------------------------------------------------
	Sequence::~Sequence	( void )
// -----------------------------------------------------------------------------
{
	if (original)	delete original;
	if (compressed)	delete compressed;
}

// -----------------------------------------------------------------------------
//	Ersetzen der alten Sequenz
// -----------------------------------------------------------------------------
	int Sequence::Set ( str seq )
// -----------------------------------------------------------------------------
{
	int error = 0;
	
	if (!seq) error = 1;	// Ungueltige Sequenz
	else
	{
		str tmp = strdup(seq);	// Kopieren der Sequenz
		
		if (!tmp) error = 2;	// Kopieren ist schiefgelaufen
		else
		{
			if (original)   delete original;	// Altlasten freigeben
			if (compressed) delete compressed;

			original = tmp;						// Sequenz neu besetzen
			origlen  = strlen(original);
			
			T2U(original);		// T- nach U-Zeichen umwandeln

													// Sequenz kompremieren		
			if ((compressed = CompressSequence(original,origlen)) != NULL)
		        complen = strlen(compressed);
			else
		        complen = 0;
		}
	}

	return error;
}

// -----------------------------------------------------------------------------
//	Ausgabefunktion fur eine Instanz der Klasse Sequenz zu debug-Zwecken
// -----------------------------------------------------------------------------
	void Sequence::Dump ( void )
// -----------------------------------------------------------------------------
{
	cout << "\noriginal   = " << original;
	cout << "\noriglen    = " << origlen;

	cout << "\ncompressed = " << compressed;
	cout << "\ncomplen    = " << complen;

	cout << "\n\n";
}
