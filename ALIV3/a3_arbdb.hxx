// -----------------------------------------------------------------------------

#ifndef _A3_ARBDB_HXX
#define _A3_ARBDB_HXX

// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#include <arbdb.h>
#include <arbdbt.h>

// -----------------------------------------------------------------------------
//  Datentypen
// -----------------------------------------------------------------------------

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
                                      char *sequence,
                                      char *info = 0 );
};

// -----------------------------------------------------------------------------

#endif

// -----------------------------------------------------------------------------
