// -----------------------------------------------------------------------------

#ifndef _A3_HELIX_HXX
#define _A3_HELIX_HXX

// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#include "a3_types.h"
#include "a3_darray.hxx"

// -----------------------------------------------------------------------------
//  Datentypen
// -----------------------------------------------------------------------------

struct HMatch       // Ubereinstimmender Bereich von Sequenz und Helixstruktur
{
    int first,      // Erste Position des Bereichs
        last;       // Letzte Position des Bereichs

        HMatch ( void ) { first = last = 0; };
};

struct HelixS       // Helixbereich
{
    int open,       // Position der ersten oeffnenden Klammer
        close,      // Position der ersten schliessenden Klammer
        length;     // Laenge der Helix

        HelixS ( void )     { open = close = length = 0; };
        HelixS ( int    o,
                 int    c,
                 int    l ) { open   = o;
                              close  = c;
                              length = l; };
};

class A3Helix
{
    private:
    
    str      helix,         // Helix(klammer)struktur
             consensus;     // Haeufigste Familiensequenz
    int      length;        // Laenge von helix und consensus
    DArray   match;         // Array of HMatch*, sortiert nach first
    
    HMatch  &GetPart    ( int        part );
    
    public:

             A3Helix    ( void );

             A3Helix    ( str        hel,       
                          str        kon,
                          DArray    &mat );

            ~A3Helix    ( void );

    void     Set        ( DArray    &mat );

    int      Parts      ( void );               // Anzahl der unalignten Bereiche

    DArray  &Helices    ( int        part,
                          int        minlen );  // Liefert DArray of HelixS*
    
    void     Dump       ( int        all );
};

// -----------------------------------------------------------------------------
//  Funktionen
// -----------------------------------------------------------------------------

extern  int     intcmp      ( const void    *a,
                              const void    *b);

extern  int     hmatchcmp   ( const void    *a,
                              const void    *b);

extern  void    hmatchdump  ( vp    val );

// -----------------------------------------------------------------------------

#endif

// -----------------------------------------------------------------------------
