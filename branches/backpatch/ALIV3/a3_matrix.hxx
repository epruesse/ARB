// ================================================================ //
//                                                                  //
//   File      : a3_matrix.hxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef A3_MATRIX_HXX
#define A3_MATRIX_HXX

#ifndef A3_DARRAY_HXX
#include "a3_darray.hxx"
#endif

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

#else
#error a3_matrix.hxx included twice
#endif // A3_MATRIX_HXX
