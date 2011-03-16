// ================================================================ //
//                                                                  //
//   File      : a3_arbdb.hxx                                       //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef A3_ARBDB_HXX
#define A3_ARBDB_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

class A3Arbdb
{
    private:

        char   *alignment;

    public:

        GBDATA *gb_main;

                A3Arbdb             (void)  { alignment = 0;
                                              gb_main   = 0; };
        
               ~A3Arbdb             (void);

        int     open                ( char *name,
                                      char *use_alignment = 0 );

        void    close               ( void );

    void begin_transaction(void) {GB_begin_transaction(gb_main);};

    void commit_transaction(void) {GB_commit_transaction(gb_main);};

        char   *get_sequence_string ( const char *name,
                                      int   and_mark = 0 );

        int     put_sequence_string ( char *name,
                                      char *sequence);
};

#else
#error a3_arbdb.hxx included twice
#endif // A3_ARBDB_HXX
