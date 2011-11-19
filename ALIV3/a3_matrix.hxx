// -----------------------------------------------------------------------------

#ifndef _A3_MATRIX_HXX
#define _A3_MATRIX_HXX

// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#include "a3_darray.hxx"

// -----------------------------------------------------------------------------
//  Datentypen
// -----------------------------------------------------------------------------

class A3Matrix              // Matrix fuer beliebige Elemente
{
    private:

    int      width,         // Spalten der Matrix
             height,        // Zeilen der Matrix
             free;          // Sollen Elemente explizit mit delete freigegeben werden
    vp      *matrix;        // Eigentliche Matrix

    void     Init           ( int        xlen,
                              int        ylen,
                              int        del );
    
    public:

             A3Matrix       ( int        len,
                              int        del );
             A3Matrix       ( int        xlen,
                              int        ylen,
                              int        del );
             A3Matrix       ( A3Matrix  &other );
            ~A3Matrix       ( void );

    int      Set            ( int        xpos,
                              int        ypos,
                              vp         val );
    
    vp       Get            ( int        xpos,
                              int        ypos );

    void     Clear          ( void );
    
    void     Dump           ( dumpfunc   edump );
};

// -----------------------------------------------------------------------------

#endif

// -----------------------------------------------------------------------------
