// ================================================================ //
//                                                                  //
//   File      : a3_darray.hxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef A3_DARRAY_HXX
#define A3_DARRAY_HXX

#define DARRAY_SIZE     10
#define DARRAY_INC       2

#define DARRAY_NONULL    0
#define DARRAY_NULL      1

#define DARRAY_NOFREE    0
#define DARRAY_FREE      1

typedef void *vp;                                   // Element eines DArray
typedef void (*dumpfunc)(vp);                       // Ausgabefunktion fuer ein Element
typedef int  (*cmpfunc) (const void*, const void*); // Vergleichsfunktion fuer zwei Elemente

class DArray        // Dynamisches Array fuer beliebige Elemente
{
    private:

    int      elements,      // Anzahl der Elemente
             nextfree,      // Naechster freier Index
             increment,     // Schrittweite fuer Vergroesserung
             null,          // Sollen 0-Elemente beruecksichtigt werden
             free;          // Sollen Elemente explizit mit delete freigegeben werden
    vp      *array;         // Eigentliches Array

    void     Init           ( int        num,
                              int        inc,
                              int        del );
    void     Grow           ( void );
    void     Shrink         ( int        num );
    
    public:

             DArray         ( void );
             DArray         ( int        num );
             DArray         ( int        num,
                              int        inc,
                              int        del );
             DArray         ( DArray    &other );
            ~DArray         ( void );

    void     Free           ( int        del ) { free = !!del; };
    void     Null           ( int        nul ) { null = !!nul; };

    int      Elements       ( void );
    
    int      Add            ( vp         elem );

    int      Set            ( vp         elem,
                              int        index );

    int      Del            ( int        index );
    
    vp       operator []    ( int        index );
    DArray  &operator =     ( DArray    &other );

    void     Sort           ( cmpfunc    cmp );

    void     Clear          ( void );
    
    void     Dump           ( dumpfunc   edump );
};

#else
#error a3_darray.hxx included twice
#endif // A3_DARRAY_HXX
