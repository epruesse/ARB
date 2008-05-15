// -----------------------------------------------------------------------------

#ifndef _A3_PTREE_HXX
#define _A3_PTREE_HXX

// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#include "a3_seq.hxx"

// -----------------------------------------------------------------------------
//  Datentypen
// -----------------------------------------------------------------------------

typedef enum
{
    P_ADENIN  = 0x0001,
    P_CYTOSIN = 0x0002,
    P_GUANIN  = 0x0004,
    P_URACIL  = 0x0008,
    P_ONE     = 0x0010,
    P_ANY     = 0x0020,
    P_FINISH  = 0x0040,

    N_ADENIN  = 0x0080,
    N_CYTOSIN = 0x0100,
    N_GUANIN  = 0x0200,
    N_URACIL  = 0x0400,
    N_ONE     = 0x0800,
    N_ANY     = 0x1000
}
Mask;

struct PtNode   // Struktur eines Baumknotens
{
    Mask        mask;           // Maske fuer die Positionen und Verweise
    int        *position;       // Positionsfeld
    PtNode    **next;           // Verweise auf darueberliegende Knoten

                PtNode      ( void )            // Konstruktor fuer leeren Knoten
                {
                    mask     = (Mask)0;
                    position = NULL;
                    next     = NULL;
                };
                PtNode      ( str     seq,      // Konstruktor fur Baum aus Sequenz
                              int    *pos,      // Basenpositionen
                              int     num,      // Anzahl der Basenpositionen
                              int     rec );    // Rekursionsstufe
                PtNode      ( PtNode &node );   // Kopierkonstruktor
               ~PtNode      ( void );           // Destruktor

    int         NumOfPos    ( void );           // Anzahl der Positionen
    int         IndexOfPos  ( Mask   base );    // Index einer Position
    int         NumOfNext   ( void );           // Anzahl der Verzweigungen
    int         IndexOfNext ( Mask   base );    // Index einer verzweigung
    void        Dump        ( void );           // Ausgabe des Baums elementeweise
    void        Show        ( int    rec,       // Ausgabe des Baums
                              int   *where );
};

class Postree   // Klasse fuer einen Positionsbaum einer RNS-Sequenzen
{
    private:

    Sequence    sequence;   // Die zugehoerige Sequenz
    PtNode     *topnode;    // Oberster Knoten des Positionsbaumes

    public:

            Postree         ( void );                   // Konstruktor fuer leeren Positionsbaum

            Postree         ( str        seq );         // Konstruktor fuer Positionsbaum
                                                        // einer vorgegebenen Sequenz

            Postree         ( uint       len );         // Konstruktor fuer Positionsbaum einer
                                                        // Zufallssequenz mit vorgegebener Laenge

            Postree         ( str        file,          // Konstruktor fuer Positionsbaum
                              uint       line );        // einer Sequenz aus einer Datei
                                                        // mit vorgegebener Zeilennummer

            Postree         ( Postree   &postree );     // Kopierkonstruktor

            ~Postree        ( void );                   // Destruktor

    int     SeqLen          ( void );                   // Liefert Laenge der kompremierten Sequenz
    
    void    Set             ( str        seq );         // Positionsbaum fuer neue Sequenz anlegen

    int     Find            ( str        seq,           // Sucht nach dem exakten Vorkommen einer
                              int      **pos,           // Teilsequenz und liefert deren moegliche
                              int       *anz,           // Positionen und Laenge zurueck
                              int       *len,
                              int        sort );

    int     OneMismatch     ( str        seq,           // Sucht nach dem Vorkommen einer Teil-
                              int      **pos,           // sequenz, wobei 1 Fehler toleriert wird
                              int       *anz,           // und liefert deren moegliche Positionen
                              int       *len );         // und Laenge zurueck

    int     OneSubstitution ( str        seq,           // Sucht nach dem Vorkommen einer Teil-
                              int      **pos,           // sequenz, wobei 1 Substitituton toleriert
                              int       *anz,           // wird und liefert deren moegliche Positionen
                              int       *len );         // und Laenge zurueck

    int     OneDeletion     ( str        seq,           // Sucht nach dem Vorkommen einer Teil-
                              int      **pos,           // sequenz, wobei 1 Deletion toleriert
                              int       *anz,           // wird und liefert deren moegliche Positionen
                              int       *len );         // und Laenge zurueck

    int     OneInsertion    ( str        seq,           // Sucht nach dem Vorkommen einer Teil-
                              int      **pos,           // sequenz, wobei 1 Insertion toleriert
                              int       *anz,           // wird und liefert deren moegliche Positionen
                              int       *len );         // und Laenge zurueck
    
    void    Dump            ( void );                   // Gibt die Elemente eines Positionsbaum aus

    void    Show            ( int        mode );        // Zeigt einen Positionsbaum an
                                                        // 0 Baum, 1 Sequenz, 2 beides
};

// -----------------------------------------------------------------------------

#endif

// -----------------------------------------------------------------------------
