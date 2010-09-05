// -----------------------------------------------------------------------------

#ifndef _A3_ALI_HXX
#define _A3_ALI_HXX

// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#include "a3_ptree.hxx"
#include "a3_helix.hxx"

// -----------------------------------------------------------------------------
//  Datentypen
// -----------------------------------------------------------------------------

struct PSolution    // Loesung des Prealigners
{
    int      score; // Bewertung der Loesung
    DArray   match; // Array of HMatch*, sortiert nach first

             PSolution  ( void ) : match () { score = 0;
                                              match.Free(DARRAY_FREE);
                                              match.Null(DARRAY_NONULL); };
    void     Dump       ( void );
};

class Aligner       // Sekundaerstrukturalignment einer Sequenz
{
    private:
    
    Postree     postree;        // Positionsbaum der Sequenz
    DArray      prealigned;     // Array of PSolution*, sortiert nach score
    A3Helix     helix;          // Helixstruktur und haeufigste Familiensequenz

                Aligner ( Aligner   &other );   // Kopien verbieten
    
    public:

                Aligner ( void );

                Aligner ( str        seq,   // Zu alignde Sequenz
                          str        hel,   // Sekundaerstruktur
                          str        kon,   // Haeufigste Familiensequenz 
                          DArray    &pre ); // Array of PSolution, sortiert nach score

    void        Set     ( str        seq,   // Zu alignde Sequenz
                          str        hel,   // Sekundaerstruktur
                          str        kon,   // Haeufigste Familiensequenz 
                          DArray    &pre ); // Array of PSolution, sortiert nach score

    DArray     &Align   ( void );

    void        Dump    ( void );
};

// -----------------------------------------------------------------------------
//  Funktionen
// -----------------------------------------------------------------------------

extern  int     psolcmp     ( const void *a,
                              const void *b );

extern  void    psoldump    ( vp    val );

// -----------------------------------------------------------------------------

#endif

// -----------------------------------------------------------------------------
