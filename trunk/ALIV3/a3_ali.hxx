// ================================================================ //
//                                                                  //
//   File      : a3_ali.hxx                                         //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef A3_ALI_HXX
#define A3_ALI_HXX

#ifndef A3_PTREE_HXX
#include "a3_ptree.hxx"
#endif
#ifndef A3_HELIX_HXX
#include "a3_helix.hxx"
#endif

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

extern int  psolcmp(const void *a, const void *b);
extern void psoldump(vp val);

#else
#error a3_ali.hxx included twice
#endif // A3_ALI_HXX
