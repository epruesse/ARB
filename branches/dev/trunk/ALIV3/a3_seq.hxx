// ================================================================ //
//                                                                  //
//   File      : a3_seq.hxx                                         //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef A3_SEQ_HXX
#define A3_SEQ_HXX

#ifndef A3_TYPES_H
#include "a3_types.h"
#endif
#ifndef _GLIBCXX_CSTRING
#include <cstring>
#endif

class Sequence              // Klasse fuer RNS-Sequenzen
{
    private:
    
    str     original;       // Originale Sequenz
    UINT    origlen;        // Laenge der originalen Sequenz

    str     compressed;     // Kompremiert Sequenz
    UINT    complen;        // Laenge der kompremierten Sequenz

    public:

            Sequence        ( void );                   // Konstruktor fuer leere Sequenz

            Sequence        ( str        seq );         // Konstruktor mit vorgegebener Sequenz

            Sequence        ( UINT       len );         // Konstruktor fuer Zufallssequenz

            Sequence        ( str        file,          // Konstruktor fuer Sequenz aus Datei
                              UINT       line );

            Sequence        ( Sequence  &sequence );    // Kopierkonstruktor

            ~Sequence       ( void );                   // Destruktor
    
    str     Original        ( void )    { return strdup(original); };   // Liefert Kopie der
                                                                        // originalen Sequenz

    UINT    OriginalLen     ( void )    { return origlen; };            // Liefert Laenge der
                                                                        // originalen Sequenz

    str     Compressed      ( void )    { return strdup(compressed); }; // Liefert Kopie der
                                                                        // kompremierten Sequenz

    UINT    CompressedLen   ( void )    { return complen; };            // Liefert Laenge der
                                                                        // kompremierten Sequenz

    int     Set             ( str        seq );                         // Ueberschreibt die
                                                                        // Klassenelemente mit neuen,
                                                                        // aus seq generierten.

    void    Dump            ( void );                                   // Gibt die Sequenz aus
};

#else
#error a3_seq.hxx included twice
#endif // A3_SEQ_HXX
